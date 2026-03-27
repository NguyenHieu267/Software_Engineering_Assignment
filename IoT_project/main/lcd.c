#include "lcd_task.h"

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "i2c_lcd.h"
#include "parking_state.h"

void lcd(void *pvParameters)
{
    (void)pvParameters;

    ESP_ERROR_CHECK(lcd_init());
    lcd_clear();

    while (1) {
        parking_snapshot_t snapshot = {0};
        parking_state_get_snapshot(&snapshot);

        char row0[17];
        char row1[17];

        snprintf(row0, sizeof(row0), "Empty:%02d/%02d", snapshot.empty_spaces, snapshot.total_spaces);
        snprintf(row1, sizeof(row1), "UID:%.11s", snapshot.last_uid[0] ? snapshot.last_uid : "none");

        lcd_write_line(0, row0);
        lcd_write_line(1, row1);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
