#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_log.h"

#include "rc522.h"

static const char *TAG = "esp-rc522";

struct rc522 {
    bool running;
    rc522_config_t *config;
    spi_device_handle_t spi;
    TaskHandle_t task_handle;
    bool scan_started;
    bool tag_was_present_last_time;
};

typedef struct rc522 *rc522_handle_t;

static rc522_handle_t hndl;

#define rc522_fw_version() rc522_read(0x37)

static esp_err_t rc522_spi_init(void);
static esp_err_t rc522_write_n(uint8_t addr, uint8_t n, uint8_t *data);
static esp_err_t rc522_write(uint8_t addr, uint8_t val);
static uint8_t *rc522_read_n(uint8_t addr, uint8_t n);
static uint8_t rc522_read(uint8_t addr);
static esp_err_t rc522_set_bitmask(uint8_t addr, uint8_t mask);
static esp_err_t rc522_clear_bitmask(uint8_t addr, uint8_t mask);
static esp_err_t rc522_antenna_on(void);
static uint8_t *rc522_calculate_crc(uint8_t *data, uint8_t n);
static uint8_t *rc522_card_write(uint8_t cmd, uint8_t *data, uint8_t n, uint8_t *res_n);
static uint8_t *rc522_request(uint8_t *res_n);
static uint8_t *rc522_anticoll(void);
static uint8_t *rc522_get_tag(void);
static void rc522_task(void *arg);

bool rc522_is_inited(void)
{
    return hndl != NULL;
}

static esp_err_t rc522_spi_init(void)
{
    if (!hndl || !hndl->config) {
        ESP_LOGE(TAG, "Invalid RC522 handle");
        return ESP_ERR_INVALID_STATE;
    }

    spi_bus_config_t buscfg = {
        .miso_io_num = hndl->config->miso_io,
        .mosi_io_num = hndl->config->mosi_io,
        .sclk_io_num = hndl->config->sck_io,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 5000000,
        .mode = 0,
        .spics_io_num = hndl->config->sda_io,
        .queue_size = 7,
        .flags = SPI_DEVICE_HALFDUPLEX,
    };

    esp_err_t err = spi_bus_initialize(hndl->config->spi_host_id, &buscfg, 0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    err = spi_bus_add_device(hndl->config->spi_host_id, &devcfg, &hndl->spi);
    if (err != ESP_OK) {
        spi_bus_free(hndl->config->spi_host_id);
        hndl->spi = NULL;
    }

    return err;
}

static esp_err_t rc522_write_n(uint8_t addr, uint8_t n, uint8_t *data)
{
    uint8_t *buffer = (uint8_t *)malloc(n + 1);
    if (!buffer) {
        return ESP_ERR_NO_MEM;
    }

    buffer[0] = (addr << 1) & 0x7E;
    for (uint8_t i = 1; i <= n; i++) {
        buffer[i] = data[i - 1];
    }

    spi_transaction_t t = {0};
    t.length = 8 * (n + 1);
    t.tx_buffer = buffer;

    esp_err_t ret = spi_device_transmit(hndl->spi, &t);
    free(buffer);
    return ret;
}

static esp_err_t rc522_write(uint8_t addr, uint8_t val)
{
    return rc522_write_n(addr, 1, &val);
}

static uint8_t *rc522_read_n(uint8_t addr, uint8_t n)
{
    if (n == 0) {
        return NULL;
    }

    spi_transaction_t t = {0};
    uint8_t *buffer = (uint8_t *)malloc(n);
    if (!buffer) {
        return NULL;
    }

    t.flags = SPI_TRANS_USE_TXDATA;
    t.length = 8;
    t.tx_data[0] = ((addr << 1) & 0x7E) | 0x80;
    t.rxlength = 8 * n;
    t.rx_buffer = buffer;

    esp_err_t ret = spi_device_transmit(hndl->spi, &t);
    if (ret != ESP_OK) {
        free(buffer);
        return NULL;
    }

    return buffer;
}

static uint8_t rc522_read(uint8_t addr)
{
    uint8_t *buffer = rc522_read_n(addr, 1);
    if (!buffer) {
        return 0;
    }
    uint8_t res = buffer[0];
    free(buffer);
    return res;
}

static esp_err_t rc522_set_bitmask(uint8_t addr, uint8_t mask)
{
    return rc522_write(addr, rc522_read(addr) | mask);
}

static esp_err_t rc522_clear_bitmask(uint8_t addr, uint8_t mask)
{
    return rc522_write(addr, rc522_read(addr) & (uint8_t)(~mask));
}

static esp_err_t rc522_antenna_on(void)
{
    if (~(rc522_read(0x14) & 0x03)) {
        ESP_RETURN_ON_ERROR(rc522_set_bitmask(0x14, 0x03), TAG, "antenna on failed");
    }
    return rc522_write(0x26, 0x60);
}

esp_err_t rc522_init(rc522_config_t *config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }
    if (hndl) {
        ESP_LOGW(TAG, "RC522 already initialized");
        return ESP_ERR_INVALID_STATE;
    }

    hndl = calloc(1, sizeof(struct rc522));
    if (!hndl) {
        return ESP_ERR_NO_MEM;
    }

    hndl->config = calloc(1, sizeof(rc522_config_t));
    if (!hndl->config) {
        rc522_destroy();
        return ESP_ERR_NO_MEM;
    }

    hndl->config->callback = config->callback;
    hndl->config->miso_io = config->miso_io == 0 ? RC522_DEFAULT_MISO : config->miso_io;
    hndl->config->mosi_io = config->mosi_io == 0 ? RC522_DEFAULT_MOSI : config->mosi_io;
    hndl->config->sck_io = config->sck_io == 0 ? RC522_DEFAULT_SCK : config->sck_io;
    hndl->config->sda_io = config->sda_io == 0 ? RC522_DEFAULT_SDA : config->sda_io;
    hndl->config->spi_host_id = config->spi_host_id == 0 ? RC522_DEFAULT_SPI_HOST : config->spi_host_id;
    hndl->config->scan_interval_ms =
        config->scan_interval_ms < 50 ? RC522_DEFAULT_SCAN_INTERVAL_MS : config->scan_interval_ms;
    hndl->config->task_stack_size =
        config->task_stack_size == 0 ? RC522_DEFAULT_TACK_STACK_SIZE : config->task_stack_size;
    hndl->config->task_priority =
        config->task_priority == 0 ? RC522_DEFAULT_TACK_STACK_PRIORITY : config->task_priority;

    ESP_RETURN_ON_ERROR(rc522_spi_init(), TAG, "spi init failed");

    const uint8_t test_addr = 0x24;
    const uint8_t test_val = 0x25;
    for (uint8_t i = test_val; i < test_val + 2; i++) {
        esp_err_t err = rc522_write(test_addr, i);
        if (err != ESP_OK || rc522_read(test_addr) != i) {
            ESP_LOGE(TAG, "rc522 rw test failed");
            rc522_destroy();
            return err == ESP_OK ? ESP_FAIL : err;
        }
    }

    rc522_write(0x01, 0x0F);
    rc522_write(0x2A, 0x8D);
    rc522_write(0x2B, 0x3E);
    rc522_write(0x2D, 0x1E);
    rc522_write(0x2C, 0x00);
    rc522_write(0x15, 0x40);
    rc522_write(0x11, 0x3D);

    ESP_RETURN_ON_ERROR(rc522_antenna_on(), TAG, "antenna on failed");

    hndl->running = true;
    if (xTaskCreate(rc522_task, "rc522_task", hndl->config->task_stack_size, NULL,
                    hndl->config->task_priority, &hndl->task_handle) != pdTRUE) {
        rc522_destroy();
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "RC522 initialized (fw=0x%x)", rc522_fw_version());
    return ESP_OK;
}

uint64_t rc522_sn_to_u64(uint8_t *sn)
{
    if (!sn) {
        return 0;
    }

    uint64_t result = 0;
    for (int i = 4; i >= 0; i--) {
        result |= ((uint64_t)sn[i] << (i * 8));
    }
    return result;
}

static uint8_t *rc522_calculate_crc(uint8_t *data, uint8_t n)
{
    rc522_clear_bitmask(0x05, 0x04);
    rc522_set_bitmask(0x0A, 0x80);

    rc522_write_n(0x09, n, data);
    rc522_write(0x01, 0x03);

    uint8_t i = 255;
    uint8_t nn = 0;

    while (i != 0) {
        nn = rc522_read(0x05);
        i--;
        if (nn & 0x04) {
            break;
        }
    }

    uint8_t *res = (uint8_t *)malloc(2);
    if (!res) {
        return NULL;
    }

    res[0] = rc522_read(0x22);
    res[1] = rc522_read(0x21);
    return res;
}

static uint8_t *rc522_card_write(uint8_t cmd, uint8_t *data, uint8_t n, uint8_t *res_n)
{
    uint8_t *result = NULL;
    uint8_t irq = 0x00;
    uint8_t irq_wait = 0x00;
    uint8_t last_bits = 0;
    uint8_t nn = 0;

    if (cmd == 0x0E) {
        irq = 0x12;
        irq_wait = 0x10;
    } else if (cmd == 0x0C) {
        irq = 0x77;
        irq_wait = 0x30;
    }

    rc522_write(0x02, irq | 0x80);
    rc522_clear_bitmask(0x04, 0x80);
    rc522_set_bitmask(0x0A, 0x80);
    rc522_write(0x01, 0x00);

    rc522_write_n(0x09, n, data);
    rc522_write(0x01, cmd);

    if (cmd == 0x0C) {
        rc522_set_bitmask(0x0D, 0x80);
    }

    uint16_t i = 1000;
    while (i != 0) {
        nn = rc522_read(0x04);
        i--;
        if ((nn & 0x01) || (nn & irq_wait)) {
            break;
        }
    }

    rc522_clear_bitmask(0x0D, 0x80);

    if (i != 0 && (rc522_read(0x06) & 0x1B) == 0x00) {
        if (cmd == 0x0C) {
            nn = rc522_read(0x0A);
            last_bits = rc522_read(0x0C) & 0x07;

            if (last_bits != 0) {
                *res_n = (nn - 1) + last_bits;
            } else {
                *res_n = nn;
            }

            result = (uint8_t *)malloc(*res_n);
            if (!result) {
                return NULL;
            }

            for (i = 0; i < *res_n; i++) {
                result[i] = rc522_read(0x09);
            }
        }
    }

    return result;
}

static uint8_t *rc522_request(uint8_t *res_n)
{
    rc522_write(0x0D, 0x07);

    uint8_t req_mode = 0x26;
    uint8_t *result = rc522_card_write(0x0C, &req_mode, 1, res_n);

    if (!result || *res_n * 8 != 0x10) {
        free(result);
        return NULL;
    }

    return result;
}

static uint8_t *rc522_anticoll(void)
{
    uint8_t res_n;

    rc522_write(0x0D, 0x00);
    uint8_t *result = rc522_card_write(0x0C, (uint8_t[]){0x93, 0x20}, 2, &res_n);

    if (result && res_n != 5) {
        free(result);
        return NULL;
    }

    return result;
}

static uint8_t *rc522_get_tag(void)
{
    uint8_t *res_data = NULL;
    uint8_t res_data_n;

    res_data = rc522_request(&res_data_n);
    if (!res_data) {
        return NULL;
    }
    free(res_data);

    uint8_t *result = rc522_anticoll();
    if (!result) {
        return NULL;
    }

    uint8_t buf[] = {0x50, 0x00, 0x00, 0x00};
    uint8_t *crc = rc522_calculate_crc(buf, 2);
    if (!crc) {
        free(result);
        return NULL;
    }

    buf[2] = crc[0];
    buf[3] = crc[1];
    free(crc);

    res_data = rc522_card_write(0x0C, buf, 4, &res_data_n);
    free(res_data);

    rc522_clear_bitmask(0x08, 0x08);
    return result;
}

esp_err_t rc522_start(rc522_start_args_t start_args)
{
    esp_err_t err = rc522_init(&start_args);
    return err == ESP_OK ? rc522_start2() : err;
}

esp_err_t rc522_start2(void)
{
    if (!hndl) {
        return ESP_ERR_INVALID_STATE;
    }
    hndl->scan_started = true;
    return ESP_OK;
}

esp_err_t rc522_pause(void)
{
    if (!hndl) {
        return ESP_ERR_INVALID_STATE;
    }
    hndl->scan_started = false;
    return ESP_OK;
}

void rc522_destroy(void)
{
    if (!hndl) {
        return;
    }

    rc522_pause();
    hndl->running = false;

    if (hndl->spi) {
        spi_bus_remove_device(hndl->spi);
        spi_bus_free(hndl->config->spi_host_id);
        hndl->spi = NULL;
    }

    free(hndl->config);
    hndl->config = NULL;

    free(hndl);
    hndl = NULL;
}

static void rc522_task(void *arg)
{
    (void)arg;
    while (hndl && hndl->running) {
        if (!hndl->scan_started) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        uint8_t *serial_no = rc522_get_tag();

        if (serial_no && !hndl->tag_was_present_last_time) {
            rc522_tag_callback_t cb = hndl->config->callback;
            if (cb) {
                cb(serial_no);
            }
        }

        hndl->tag_was_present_last_time = (serial_no != NULL);
        if (serial_no) {
            free(serial_no);
            serial_no = NULL;
        }

        int delay_interval_ms = hndl->config->scan_interval_ms;
        if (hndl->tag_was_present_last_time) {
            delay_interval_ms *= 2;
        }

        vTaskDelay(pdMS_TO_TICKS(delay_interval_ms));
    }

    vTaskDelete(NULL);
}
