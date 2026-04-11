#include "driver/ledc.h"
#include "pin_config.h"
#include "esp_log.h"

static const char *TAG = "SERVO_LED";

#define SERVO_MIN_PULSE 500  // 0.5ms
#define SERVO_MAX_PULSE 2500 // 2.5ms
#define SERVO_TIMER     LEDC_TIMER_0
#define SERVO_MODE      LEDC_LOW_SPEED_MODE

void servo_led_init(void) {
    // Timer for Servo (50Hz)
    ledc_timer_config_t servo_timer = {
        .speed_mode       = SERVO_MODE,
        .timer_num        = SERVO_TIMER,
        .duty_resolution  = LEDC_TIMER_14_BIT,
        .freq_hz          = 50,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&servo_timer));

    // Servo Channel
    ledc_channel_config_t servo_ch = {
        .speed_mode     = SERVO_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = SERVO_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = CANNON_SERVO_PIN,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&servo_ch));

    // Timer for LEDs (5kHz)
    ledc_timer_config_t led_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_1,
        .duty_resolution  = LEDC_TIMER_8_BIT,
        .freq_hz          = 5000,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&led_timer));

    // LED Channels
    int led_pins[] = {MG_LED_PIN, HEADLIGHT_LED_PIN, CANNON_LED_PIN, BACKLIGHT_LED_PIN};
    for (int i = 0; i < 4; i++) {
        ledc_channel_config_t led_ch = {
            .speed_mode     = LEDC_LOW_SPEED_MODE,
            .channel        = LEDC_CHANNEL_1 + i,
            .timer_sel      = LEDC_TIMER_1,
            .intr_type      = LEDC_INTR_DISABLE,
            .gpio_num       = led_pins[i],
            .duty           = 0,
            .hpoint         = 0
        };
        ESP_ERROR_CHECK(ledc_channel_config(&led_ch));
    }
    ESP_LOGI(TAG, "LEDC initialized for Servo and 4 LED channels");
}

void servo_set_angle(int angle) {
    // Angle 0-180 to duty
    // 50Hz -> 20ms period. 14-bit resolution -> 16384 ticks.
    // 1ms is approx 819 ticks. 2ms is 1638.
    uint32_t duty = (angle * (1638 - 819) / 180) + 819;
    ledc_set_duty(SERVO_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(SERVO_MODE, LEDC_CHANNEL_0);
}

void led_set_brightness(int pin, int brightness) {
    ledc_channel_t ch;
    if (pin == MG_LED_PIN) ch = LEDC_CHANNEL_1;
    else if (pin == HEADLIGHT_LED_PIN) ch = LEDC_CHANNEL_2;
    else if (pin == CANNON_LED_PIN) ch = LEDC_CHANNEL_3;
    else if (pin == BACKLIGHT_LED_PIN) ch = LEDC_CHANNEL_4;
    else return;

    ledc_set_duty(LEDC_LOW_SPEED_MODE, ch, brightness);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, ch);
}
