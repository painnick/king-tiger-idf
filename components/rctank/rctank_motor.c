/**
 * @file rctank_motor.c
 * @brief RC Tank MCPWM 트랙/터렛 제어 (DRV8833)
 */
#include "rctank_motor.h"
#include "rctank_pins.h"

#include "driver/mcpwm_prelude.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "rctank_motor";

#define MCPWM_FREQ_HZ          (20000)
#define MCPWM_RESOLUTION_HZ   (1000000)
#define MCPWM_PERIOD_TICKS    (MCPWM_RESOLUTION_HZ / MCPWM_FREQ_HZ)

/* 스틱 범위 ±512 → 듀티 비율 (0~100%) */
#define AXIS_MAX              512

static mcpwm_cmpr_handle_t left_cmpr_a = NULL;
static mcpwm_cmpr_handle_t left_cmpr_b = NULL;
static mcpwm_cmpr_handle_t right_cmpr_a = NULL;
static mcpwm_cmpr_handle_t right_cmpr_b = NULL;

static mcpwm_timer_handle_t timer0 = NULL;

static void set_motor_duty(mcpwm_cmpr_handle_t cmpr_a, mcpwm_cmpr_handle_t cmpr_b, int32_t speed)
{
    uint32_t ticks;
    if (speed > 0) {
        ticks = ((uint32_t)speed * MCPWM_PERIOD_TICKS) / AXIS_MAX;
        if (ticks > MCPWM_PERIOD_TICKS) ticks = MCPWM_PERIOD_TICKS;
        ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(cmpr_a, ticks));
        ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(cmpr_b, 0));
    } else if (speed < 0) {
        ticks = ((uint32_t)(-speed) * MCPWM_PERIOD_TICKS) / AXIS_MAX;
        if (ticks > MCPWM_PERIOD_TICKS) ticks = MCPWM_PERIOD_TICKS;
        ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(cmpr_a, 0));
        ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(cmpr_b, ticks));
    } else {
        ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(cmpr_a, 0));
        ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(cmpr_b, 0));
    }
}

esp_err_t rctank_motor_init(void)
{
    esp_err_t ret;

    mcpwm_timer_config_t timer_config = {
        .group_id = 0,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = MCPWM_RESOLUTION_HZ,
        .period_ticks = MCPWM_RESOLUTION_HZ / MCPWM_FREQ_HZ,
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    };
    ret = mcpwm_new_timer(&timer_config, &timer0);
    ESP_RETURN_ON_ERROR(ret, TAG, "mcpwm_new_timer 0");

    mcpwm_oper_handle_t left_op = NULL;
    mcpwm_oper_handle_t right_op = NULL;
    mcpwm_operator_config_t op_config = {
        .group_id = 0,
    };

    ret = mcpwm_new_operator(&op_config, &left_op);
    ESP_RETURN_ON_ERROR(ret, TAG, "mcpwm_new_operator left");
    ret = mcpwm_new_operator(&op_config, &right_op);
    ESP_RETURN_ON_ERROR(ret, TAG, "mcpwm_new_operator right");

    ret = mcpwm_operator_connect_timer(left_op, timer0);
    ESP_RETURN_ON_ERROR(ret, TAG, "connect timer left");
    ret = mcpwm_operator_connect_timer(right_op, timer0);
    ESP_RETURN_ON_ERROR(ret, TAG, "connect timer right");

    mcpwm_comparator_config_t cmpr_config = {
        .flags.update_cmp_on_tez = true,
    };

    ret = mcpwm_new_comparator(left_op, &cmpr_config, &left_cmpr_a);
    ESP_RETURN_ON_ERROR(ret, TAG, "left_cmpr_a");
    ret = mcpwm_new_comparator(left_op, &cmpr_config, &left_cmpr_b);
    ESP_RETURN_ON_ERROR(ret, TAG, "left_cmpr_b");
    ret = mcpwm_new_comparator(right_op, &cmpr_config, &right_cmpr_a);
    ESP_RETURN_ON_ERROR(ret, TAG, "right_cmpr_a");
    ret = mcpwm_new_comparator(right_op, &cmpr_config, &right_cmpr_b);
    ESP_RETURN_ON_ERROR(ret, TAG, "right_cmpr_b");

    mcpwm_generator_config_t gen_config = {
        .gen_gpio_num = -1,
    };
    mcpwm_gen_handle_t left_gen_a, left_gen_b, right_gen_a, right_gen_b;

    gen_config.gen_gpio_num = RCTANK_PIN_LEFT_IN1;
    ret = mcpwm_new_generator(left_op, &gen_config, &left_gen_a);
    ESP_RETURN_ON_ERROR(ret, TAG, "left_gen_a");
    gen_config.gen_gpio_num = RCTANK_PIN_LEFT_IN2;
    ret = mcpwm_new_generator(left_op, &gen_config, &left_gen_b);
    ESP_RETURN_ON_ERROR(ret, TAG, "left_gen_b");

    gen_config.gen_gpio_num = RCTANK_PIN_RIGHT_IN1;
    ret = mcpwm_new_generator(right_op, &gen_config, &right_gen_a);
    ESP_RETURN_ON_ERROR(ret, TAG, "right_gen_a");
    gen_config.gen_gpio_num = RCTANK_PIN_RIGHT_IN2;
    ret = mcpwm_new_generator(right_op, &gen_config, &right_gen_b);
    ESP_RETURN_ON_ERROR(ret, TAG, "right_gen_b");

    /* Turret & Port GPIO Init */
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << RCTANK_PIN_TURRET_IN1) | (1ULL << RCTANK_PIN_TURRET_IN2) |
                        (1ULL << RCTANK_PIN_PORT_IN1) | (1ULL << RCTANK_PIN_PORT_IN2),
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    gpio_config(&io_conf);

    mcpwm_generator_set_actions_on_timer_event(left_gen_a,
        MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH),
        MCPWM_GEN_TIMER_EVENT_ACTION_END());
    mcpwm_generator_set_actions_on_compare_event(left_gen_a,
        MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, left_cmpr_a, MCPWM_GEN_ACTION_LOW),
        MCPWM_GEN_COMPARE_EVENT_ACTION_END());
    mcpwm_generator_set_actions_on_timer_event(left_gen_b,
        MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH),
        MCPWM_GEN_TIMER_EVENT_ACTION_END());
    mcpwm_generator_set_actions_on_compare_event(left_gen_b,
        MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, left_cmpr_b, MCPWM_GEN_ACTION_LOW),
        MCPWM_GEN_COMPARE_EVENT_ACTION_END());

    mcpwm_generator_set_actions_on_timer_event(right_gen_a,
        MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH),
        MCPWM_GEN_TIMER_EVENT_ACTION_END());
    mcpwm_generator_set_actions_on_compare_event(right_gen_a,
        MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, right_cmpr_a, MCPWM_GEN_ACTION_LOW),
        MCPWM_GEN_COMPARE_EVENT_ACTION_END());
    mcpwm_generator_set_actions_on_timer_event(right_gen_b,
        MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH),
        MCPWM_GEN_TIMER_EVENT_ACTION_END());
    mcpwm_generator_set_actions_on_compare_event(right_gen_b,
        MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, right_cmpr_b, MCPWM_GEN_ACTION_LOW),
        MCPWM_GEN_COMPARE_EVENT_ACTION_END());

    ret = mcpwm_timer_enable(timer0);
    ESP_RETURN_ON_ERROR(ret, TAG, "timer0_enable");
    ret = mcpwm_timer_start_stop(timer0, MCPWM_TIMER_START_NO_STOP);
    ESP_RETURN_ON_ERROR(ret, TAG, "timer0_start");

    rctank_motor_left_track_set(0);
    rctank_motor_right_track_set(0);
    rctank_motor_turret_set(0);
    rctank_motor_port_set(0);

    ESP_LOGI(TAG, "motor init ok");
    return ESP_OK;
}

void rctank_motor_left_track_set(int32_t speed)
{
    if (left_cmpr_a && left_cmpr_b) {
        set_motor_duty(left_cmpr_a, left_cmpr_b, speed);
    }
}

void rctank_motor_right_track_set(int32_t speed)
{
    if (right_cmpr_a && right_cmpr_b) {
        set_motor_duty(right_cmpr_a, right_cmpr_b, speed);
    }
}

void rctank_motor_turret_set(int32_t speed)
{
    if (speed > 0) {
        gpio_set_level(RCTANK_PIN_TURRET_IN1, 1);
        gpio_set_level(RCTANK_PIN_TURRET_IN2, 0);
    } else if (speed < 0) {
        gpio_set_level(RCTANK_PIN_TURRET_IN1, 0);
        gpio_set_level(RCTANK_PIN_TURRET_IN2, 1);
    } else {
        gpio_set_level(RCTANK_PIN_TURRET_IN1, 0);
        gpio_set_level(RCTANK_PIN_TURRET_IN2, 0);
    }
}

void rctank_motor_port_set(int32_t speed)
{
    if (speed > 0) {
        gpio_set_level(RCTANK_PIN_PORT_IN1, 1);
        gpio_set_level(RCTANK_PIN_PORT_IN2, 0);
    } else if (speed < 0) {
        gpio_set_level(RCTANK_PIN_PORT_IN1, 0);
        gpio_set_level(RCTANK_PIN_PORT_IN2, 1);
    } else {
        gpio_set_level(RCTANK_PIN_PORT_IN1, 0);
        gpio_set_level(RCTANK_PIN_PORT_IN2, 0);
    }
}
