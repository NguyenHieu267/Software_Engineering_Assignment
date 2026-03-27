#include "servo.h"

#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "servo_comp";
static bool s_servo_ready;
static ledc_channel_t s_channel = LEDC_CHANNEL_0;

static uint32_t angle_to_duty(int angle)
{
    if (angle < SERVO_MIN_ANGLE) {
        angle = SERVO_MIN_ANGLE;
    }
    if (angle > SERVO_MAX_ANGLE) {
        angle = SERVO_MAX_ANGLE;
    }

    float pulse_width_ms = 0.5f + ((float)angle / 180.0f) * 2.0f;
    return (uint32_t)((pulse_width_ms / 20.0f) * 4096.0f);
}

esp_err_t servo_init(gpio_num_t gpio)
{
    ledc_timer_config_t timer_cfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_12_BIT,
        .freq_hz = 50,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    ledc_channel_config_t channel_cfg = {
        .channel = s_channel,
        .duty = 0,
        .gpio_num = gpio,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .hpoint = 0,
        .timer_sel = LEDC_TIMER_0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_cfg));

    s_servo_ready = true;
    ESP_LOGI(TAG, "Servo initialized on GPIO %d", gpio);
    return ESP_OK;
}

esp_err_t servo_set_angle(int angle_deg)
{
    if (!s_servo_ready) {
        return ESP_ERR_INVALID_STATE;
    }

    uint32_t duty = angle_to_duty(angle_deg);
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, s_channel, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, s_channel));
    return ESP_OK;
}
