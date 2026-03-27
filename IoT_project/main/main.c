#include <stdio.h>

#include "nvs_flash.h"

#include "app_queues.h"
#include "coreiot_task.h"
#include "ir_sensor_task.h"
#include "lcd_task.h"
#include "parking_state.h"
#include "rfid_task.h"
#include "servo_task.h"

#define TOTAL_PARKING_SPACES 50

QueueHandle_t g_servo_cmd_queue;
QueueHandle_t g_coreiot_evt_queue;
QueueHandle_t g_rfid_uid_queue;

typedef void (*task_entry_fn_t)(void *);

typedef struct {
    const char *name;
    task_entry_fn_t fn;
    uint32_t stack;
    UBaseType_t priority;
} task_spec_t;

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    parking_state_init(TOTAL_PARKING_SPACES);

    g_servo_cmd_queue = xQueueCreate(8, sizeof(servo_cmd_t));
    g_coreiot_evt_queue = xQueueCreate(16, sizeof(app_event_t));
    g_rfid_uid_queue = xQueueCreate(8, sizeof(char[32]));

    task_spec_t task_table[] = {
        {.name = "lcd", .fn = lcd, .stack = 4096, .priority = 3},
        {.name = "servo", .fn = servo, .stack = 4096, .priority = 3},
        {.name = "ir_sensor", .fn = ir_sensor, .stack = 4096, .priority = 4},
        {.name = "rfid", .fn = rfid, .stack = 6144, .priority = 4},
        {.name = "coreIoT", .fn = coreIoT, .stack = 8192, .priority = 4},
    };

    size_t count = sizeof(task_table) / sizeof(task_table[0]);
    for (size_t i = 0; i < count; i++) {
        xTaskCreate(task_table[i].fn, task_table[i].name, task_table[i].stack, NULL,
                    task_table[i].priority, NULL);
    }
}
