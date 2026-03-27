#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/spi_master.h"
#include "esp_err.h"

#define RC522_DEFAULT_MISO                 (19)
#define RC522_DEFAULT_MOSI                 (23)
#define RC522_DEFAULT_SCK                  (18)
#define RC522_DEFAULT_SDA                  (21)
#define RC522_DEFAULT_SPI_HOST             (VSPI_HOST)
#define RC522_DEFAULT_SCAN_INTERVAL_MS     (125)
#define RC522_DEFAULT_TACK_STACK_SIZE      (4 * 1024)
#define RC522_DEFAULT_TACK_STACK_PRIORITY  (4)

typedef void (*rc522_tag_callback_t)(uint8_t *);

typedef struct {
    int miso_io;
    int mosi_io;
    int sck_io;
    int sda_io;
    spi_host_device_t spi_host_id;
    rc522_tag_callback_t callback;
    uint16_t scan_interval_ms;
    size_t task_stack_size;
    uint8_t task_priority;
} rc522_config_t;

typedef rc522_config_t rc522_start_args_t;

esp_err_t rc522_init(rc522_config_t *config);
uint64_t rc522_sn_to_u64(uint8_t *sn);
bool rc522_is_inited(void);
esp_err_t rc522_start(rc522_start_args_t start_args);
esp_err_t rc522_start2(void);
#define rc522_resume() rc522_start2()
esp_err_t rc522_pause(void);
void rc522_destroy(void);

#ifdef __cplusplus
}
#endif
