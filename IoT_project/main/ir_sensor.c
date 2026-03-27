#include "ir_sensor_task.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_queues.h"
#include "esp_log.h"
#include "ir_sensor.h"
#include "parking_state.h"

#define IR_SENSOR_GPIO GPIO_NUM_32

static const char *TAG = "task_ir";

void ir_sensor(void *pvParameters)
{
    (void)pvParameters;

    ir_sensor_config_t cfg = {
        .gpio = IR_SENSOR_GPIO,
        .active_low = true,
        .debounce_ms = 50,
    };

    ESP_ERROR_CHECK(ir_sensor_init(&cfg));

    bool prev = ir_sensor_is_occupied();
    ESP_LOGI(TAG, "Initial IR occupied=%d", prev);

    while (1) {
        bool occupied = ir_sensor_is_occupied();
        if (occupied != prev) {
            app_event_t evt = {0};
            parking_state_on_ir_change(occupied, &evt);
            xQueueSend(g_coreiot_evt_queue, &evt, 0);

            ESP_LOGI(TAG, "IR changed: occupied=%d", occupied);
            prev = occupied;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
