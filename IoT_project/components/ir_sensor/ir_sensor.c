#include "ir_sensor.h"

#include "esp_timer.h"

static gpio_num_t s_gpio = GPIO_NUM_NC;
static bool s_active_low = true;
static uint32_t s_debounce_ms = 50;
static int s_last_stable_level = 1;
static int64_t s_last_change_us = 0;

esp_err_t ir_sensor_init(const ir_sensor_config_t *cfg)
{
    if (!cfg) {
        return ESP_ERR_INVALID_ARG;
    }

    gpio_config_t io_cfg = {
        .pin_bit_mask = 1ULL << cfg->gpio,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_cfg));

    s_gpio = cfg->gpio;
    s_active_low = cfg->active_low;
    s_debounce_ms = cfg->debounce_ms;
    s_last_stable_level = gpio_get_level(s_gpio);
    s_last_change_us = esp_timer_get_time();

    return ESP_OK;
}

bool ir_sensor_is_occupied(void)
{
    if (s_gpio == GPIO_NUM_NC) {
        return false;
    }

    int level = gpio_get_level(s_gpio);
    int64_t now = esp_timer_get_time();

    if (level != s_last_stable_level) {
        int64_t elapsed_us = now - s_last_change_us;
        if (elapsed_us >= (int64_t)s_debounce_ms * 1000) {
            s_last_stable_level = level;
            s_last_change_us = now;
        }
    }

    bool active = s_active_low ? (s_last_stable_level == 0) : (s_last_stable_level == 1);
    return active;
}
