#ifndef APP_QUEUES_H
#define APP_QUEUES_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "parking_state.h"

typedef enum {
    SERVO_CMD_OPEN_GATE = 1,
} servo_cmd_t;

extern QueueHandle_t g_servo_cmd_queue;
extern QueueHandle_t g_coreiot_evt_queue;
extern QueueHandle_t g_rfid_uid_queue;

#endif
