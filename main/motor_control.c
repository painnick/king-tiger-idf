#include "driver/mcpwm_prelude.h"
#include "pin_config.h"
#include "esp_log.h"

static const char *TAG = "MOTOR";

static mcpwm_cmpr_handle_t track_l_cmpr[2];
static mcpwm_cmpr_handle_t track_r_cmpr[2];
static mcpwm_cmpr_handle_t turret_cmpr[2];
static mcpwm_cmpr_handle_t mount_cmpr[2];

static void init_mcpwm_group(int group_id, int gpio_a, int gpio_b, mcpwm_cmpr_handle_t out_cmpr[2]) {
    mcpwm_timer_handle_t timer = NULL;
    mcpwm_timer_config_t timer_config = {
        .group_id = group_id,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = 1000000,
        .period_ticks = 20000, // 50Hz for safety, can be higher for DC
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    };
    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer));

    mcpwm_oper_handle_t oper = NULL;
    mcpwm_operator_config_t oper_config = {
        .group_id = group_id,
    };
    ESP_ERROR_CHECK(mcpwm_new_operator(&oper_config, &oper));
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(oper, timer));

    mcpwm_gen_handle_t gen[2] = {NULL, NULL};
    mcpwm_generator_config_t gen_config[2] = {
        {.gen_gpio_num = gpio_a},
        {.gen_gpio_num = gpio_b}
    };
    for (int i = 0; i < 2; i++) {
        ESP_ERROR_CHECK(mcpwm_new_generator(oper, &gen_config[i], &gen[i]));
    }

    mcpwm_comparator_config_t cmpr_config = {
        .flags.update_cmp_on_tez = true,
    };
    for (int i = 0; i < 2; i++) {
        ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &cmpr_config, &out_cmpr[i]));
    }

    // Set generators to stay low by default
    for (int i = 0; i < 2; i++) {
        ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(gen[i],
            MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
        ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(gen[i],
            MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, out_cmpr[i], MCPWM_GEN_ACTION_LOW)));
    }

    ESP_ERROR_CHECK(mcpwm_timer_enable(timer));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP));
}

void motor_init(void) {
    init_mcpwm_group(0, TRACK_AIN1_PIN, TRACK_AIN2_PIN, track_l_cmpr);
    init_mcpwm_group(0, TRACK_BIN1_PIN, TRACK_BIN2_PIN, track_r_cmpr);
    init_mcpwm_group(1, TURRET_AIN1_PIN, TURRET_AIN2_PIN, turret_cmpr);
    init_mcpwm_group(1, MOUNT_BIN1_PIN, MOUNT_BIN2_PIN, mount_cmpr);
    ESP_LOGI(TAG, "MCPWM motors initialized");
}

static void set_speed(mcpwm_cmpr_handle_t cmpr[2], int speed) {
    if (speed > 0) {
        mcpwm_comparator_set_compare_value(cmpr[0], (speed * 20000) / 1024);
        mcpwm_comparator_set_compare_value(cmpr[1], 0);
    } else if (speed < 0) {
        mcpwm_comparator_set_compare_value(cmpr[0], 0);
        mcpwm_comparator_set_compare_value(cmpr[1], (-speed * 20000) / 1024);
    } else {
        mcpwm_comparator_set_compare_value(cmpr[0], 0);
        mcpwm_comparator_set_compare_value(cmpr[1], 0);
    }
}

void motor_set_tracks(int left, int right) {
    set_speed(track_l_cmpr, left);
    set_speed(track_r_cmpr, right);
}

void motor_set_turret(int speed) {
    set_speed(turret_cmpr, speed);
}

void motor_set_mount(int speed) {
    set_speed(mount_cmpr, speed);
}
