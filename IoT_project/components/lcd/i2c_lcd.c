#include "i2c_lcd.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "driver/i2c.h"
#include "esp_check.h"
#include "esp_log.h"

#define LCD_CMD_CLEAR_DISPLAY 0x01
#define LCD_CMD_ENTRY_MODE_SET 0x06
#define LCD_CMD_DISPLAY_ON 0x0C
#define LCD_CMD_DISPLAY_OFF 0x08
#define LCD_CMD_FUNCTION_SET 0x28
#define LCD_CMD_SET_CURSOR 0x80
#define LCD_CMD_INIT_8_BIT_MODE 0x30
#define LCD_CMD_INIT_4_BIT_MODE 0x20

static const char *TAG = "lcd";
static bool s_lcd_ready;

static esp_err_t i2c_master_init(void)
{
    if (s_lcd_ready) {
        return ESP_OK;
    }

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = LCD_DEFAULT_I2C_SDA_GPIO,
        .scl_io_num = LCD_DEFAULT_I2C_SCL_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = LCD_DEFAULT_I2C_FREQ_HZ,
    };

    ESP_ERROR_CHECK(i2c_param_config(LCD_DEFAULT_I2C_PORT, &conf));
    esp_err_t err = i2c_driver_install(
        LCD_DEFAULT_I2C_PORT,
        conf.mode,
        0,
        0,
        0
    );

    if (err == ESP_OK || err == ESP_ERR_INVALID_STATE) {
        s_lcd_ready = true;
        return ESP_OK;
    }

    return err;
}

void lcd_send_cmd(char cmd)
{
    uint8_t data_u = cmd & 0xF0;
    uint8_t data_l = (cmd << 4) & 0xF0;
    uint8_t data_t[4];

    data_t[0] = data_u | 0x0C;
    data_t[1] = data_u | 0x08;
    data_t[2] = data_l | 0x0C;
    data_t[3] = data_l | 0x08;

    esp_err_t err = i2c_master_write_to_device(
        LCD_DEFAULT_I2C_PORT,
        LCD_DEFAULT_I2C_ADDRESS,
        data_t,
        sizeof(data_t),
        pdMS_TO_TICKS(100)
    );

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "lcd_send_cmd failed: %s", esp_err_to_name(err));
    }
}

void lcd_send_data(char data)
{
    uint8_t data_u = data & 0xF0;
    uint8_t data_l = (data << 4) & 0xF0;
    uint8_t data_t[4];

    data_t[0] = data_u | 0x0D;
    data_t[1] = data_u | 0x09;
    data_t[2] = data_l | 0x0D;
    data_t[3] = data_l | 0x09;

    esp_err_t err = i2c_master_write_to_device(
        LCD_DEFAULT_I2C_PORT,
        LCD_DEFAULT_I2C_ADDRESS,
        data_t,
        sizeof(data_t),
        pdMS_TO_TICKS(100)
    );

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "lcd_send_data failed: %s", esp_err_to_name(err));
    }
}

void lcd_clear(void)
{
    lcd_send_cmd(LCD_CMD_CLEAR_DISPLAY);
    usleep(5000);
}

void lcd_put_cursor(int row, int col)
{
    if (row == 0) {
        col |= LCD_CMD_SET_CURSOR;
    } else {
        col |= (LCD_CMD_SET_CURSOR | 0x40);
    }

    lcd_send_cmd((char)col);
}

esp_err_t lcd_init(void)
{
    ESP_RETURN_ON_ERROR(i2c_master_init(), TAG, "i2c init failed");

    usleep(50000);
    lcd_send_cmd(LCD_CMD_INIT_8_BIT_MODE);
    usleep(5000);
    lcd_send_cmd(LCD_CMD_INIT_8_BIT_MODE);
    usleep(200);
    lcd_send_cmd(LCD_CMD_INIT_8_BIT_MODE);
    usleep(10000);
    lcd_send_cmd(LCD_CMD_INIT_4_BIT_MODE);
    usleep(10000);

    lcd_send_cmd(LCD_CMD_FUNCTION_SET);
    usleep(1000);
    lcd_send_cmd(LCD_CMD_DISPLAY_OFF);
    usleep(1000);
    lcd_send_cmd(LCD_CMD_CLEAR_DISPLAY);
    usleep(1000);
    lcd_send_cmd(LCD_CMD_ENTRY_MODE_SET);
    usleep(1000);
    lcd_send_cmd(LCD_CMD_DISPLAY_ON);
    usleep(1000);

    return ESP_OK;
}

void lcd_send_string(const char *str)
{
    if (!str) {
        return;
    }

    while (*str) {
        lcd_send_data(*str++);
    }
}

void lcd_write_line(int row, const char *str16)
{
    char buf[17] = {0};
    if (str16) {
        snprintf(buf, sizeof(buf), "%-16.16s", str16);
    } else {
        snprintf(buf, sizeof(buf), "%-16s", "");
    }

    lcd_put_cursor(row, 0);
    lcd_send_string(buf);
}
