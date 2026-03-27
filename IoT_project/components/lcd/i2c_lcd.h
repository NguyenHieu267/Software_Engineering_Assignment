#ifndef I2C_LCD_H
#define I2C_LCD_H

#include "driver/gpio.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LCD_DEFAULT_I2C_ADDRESS         0x27
#define LCD_DEFAULT_I2C_PORT            0
#define LCD_DEFAULT_I2C_SCL_GPIO        GPIO_NUM_22
#define LCD_DEFAULT_I2C_SDA_GPIO        GPIO_NUM_21
#define LCD_DEFAULT_I2C_FREQ_HZ         400000

esp_err_t lcd_init(void);
void lcd_send_cmd(char cmd);
void lcd_send_data(char data);
void lcd_send_string(const char *str);
void lcd_put_cursor(int row, int col);
void lcd_clear(void);
void lcd_write_line(int row, const char *str16);

#ifdef __cplusplus
}
#endif

#endif
