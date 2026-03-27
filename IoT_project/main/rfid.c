#include "rfid_task.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "app_queues.h"
#include "esp_log.h"
#include "parking_state.h"
#include "rc522.h"

static const char *TAG = "task_rfid";

static void uid_to_string(const uint8_t *serial_no, char *out, size_t out_len)
{
    if (!serial_no || !out || out_len < 11) {
        return;
    }
    snprintf(out, out_len, "%02X%02X%02X%02X%02X", serial_no[0], serial_no[1], serial_no[2], serial_no[3], serial_no[4]);
}

static void rfid_tag_handler(uint8_t *serial_no)
{
    char uid[32] = {0};
    uid_to_string(serial_no, uid, sizeof(uid));
    xQueueSend(g_rfid_uid_queue, uid, 0);
}

void rfid(void *pvParameters)
{
    (void)pvParameters;

    const rc522_start_args_t start_args = {
        .miso_io = 19,
        .mosi_io = 23,
        .sck_io = 18,
        .sda_io = 21,
        .callback = &rfid_tag_handler,
    };

    ESP_ERROR_CHECK(rc522_start(start_args));
    ESP_LOGI(TAG, "RFID reader started");

    char uid[32];
    while (1) {
        if (xQueueReceive(g_rfid_uid_queue, uid, portMAX_DELAY) == pdTRUE) {
            bool open_gate = false;
            app_event_t evt = {0};

            parking_state_on_rfid(uid, &open_gate, &evt);

            if (open_gate) {
                servo_cmd_t cmd = SERVO_CMD_OPEN_GATE;
                xQueueSend(g_servo_cmd_queue, &cmd, 0);
            }

            xQueueSend(g_coreiot_evt_queue, &evt, 0);
            ESP_LOGI(TAG, "RFID UID=%s", uid);
        }
    }
}
