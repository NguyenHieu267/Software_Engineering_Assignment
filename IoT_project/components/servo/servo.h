#ifndef SERVO_COMPONENT_H
#define SERVO_COMPONENT_H

#include "driver/gpio.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SERVO_DEFAULT_GPIO GPIO_NUM_23
#define SERVO_MIN_ANGLE 0
#define SERVO_MAX_ANGLE 180

esp_err_t servo_init(gpio_num_t gpio);
esp_err_t servo_set_angle(int angle_deg);

#ifdef __cplusplus
}
#endif

#endif
