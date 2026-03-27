#ifndef IR_SENSOR_COMPONENT_H
#define IR_SENSOR_COMPONENT_H

#include <stdbool.h>
#include "driver/gpio.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    gpio_num_t gpio;
    bool active_low;
    uint32_t debounce_ms;
} ir_sensor_config_t;

esp_err_t ir_sensor_init(const ir_sensor_config_t *cfg);
bool ir_sensor_is_occupied(void);

#ifdef __cplusplus
}
#endif

#endif
