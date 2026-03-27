#include "servo_task.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "app_queues.h"
#include "esp_log.h"
#include "servo.h"

static const char *TAG = "task_servo";

#define GATE_OPEN_ANGLE 90
#define GATE_CLOSED_ANGLE 0
#define GATE_HOLD_MS 5000

void servo(void *pvParameters)
{
    (void)pvParameters;

    ESP_ERROR_CHECK(servo_init(SERVO_DEFAULT_GPIO));
    ESP_ERROR_CHECK(servo_set_angle(GATE_CLOSED_ANGLE));

    servo_cmd_t cmd;
    while (1) {
        if (xQueueReceive(g_servo_cmd_queue, &cmd, portMAX_DELAY) == pdTRUE) {
            if (cmd == SERVO_CMD_OPEN_GATE) {
                ESP_LOGI(TAG, "Opening gate");
                ESP_ERROR_CHECK(servo_set_angle(GATE_OPEN_ANGLE));
                vTaskDelay(pdMS_TO_TICKS(GATE_HOLD_MS));

                ESP_LOGI(TAG, "Closing gate");
                ESP_ERROR_CHECK(servo_set_angle(GATE_CLOSED_ANGLE));
            }
        }
    }
}
