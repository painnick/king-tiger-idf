#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "uni.h"
#include "btstack_port_esp32.h"
#include "btstack_run_loop.h"
#include "drivers.h"
#include "pin_config.h"

static const char *TAG = "APP_MAIN";

// Controller data handling
static void handle_controller_data(uni_hid_device_t* d, uni_controller_t* ctl) {
    if (ctl->klass != UNI_CONTROLLER_CLASS_GAMEPAD) return;

    uni_gamepad_t* gp = &ctl->gamepad;

    // 1. Drive Control (Left Stick)
    int throttle = -gp->axis_y;
    int steer = gp->axis_x;
    motor_set_tracks(throttle + steer, throttle - steer);

    // 2. Turret & Mount Control (Right Stick)
    motor_set_turret(gp->axis_rx / 2);
    motor_set_mount(-gp->axis_ry / 2);

    // 3. Cannon Firing Logic (Button A with 1s Cooldown)
    static uint32_t last_cannon_fire = 0;
    static uint32_t cannon_seq_timer = 0;
    static bool cannon_in_sequence = false;
    uint32_t now = esp_log_timestamp();

    if ((gp->buttons & BUTTON_A) && (now - last_cannon_fire > 1000)) {
        last_cannon_fire = now;
        cannon_seq_timer = now;
        cannon_in_sequence = true;
        
        led_set_brightness(CANNON_LED_PIN, 255);
        sound_play_index(1);
        servo_set_angle(50); // Recoil start
        ESP_LOGI(TAG, "Cannon Fired!");
    }

    if (cannon_in_sequence) {
        if (now - cannon_seq_timer > 200) {
            led_set_brightness(CANNON_LED_PIN, 0); // LED off after 200ms
        }
        if (now - cannon_seq_timer > 600) {
            servo_set_angle(0); // Servo back after 600ms
            cannon_in_sequence = false;
        }
    }

    // 4. Machine Gun Logic (Button X with flickering)
    if (gp->buttons & BUTTON_X) {
        static uint32_t last_mg_sound = 0;
        static int mg_flicker = 0;
        
        led_set_brightness(MG_LED_PIN, (mg_flicker++ % 2) ? 255 : 0);
        if (now - last_mg_sound > 150) { // Repeat sound every 150ms
            sound_play_index(2);
            last_mg_sound = now;
        }
    } else {
        led_set_brightness(MG_LED_PIN, 0);
    }

    // 5. Headlight Logic (Button B - Toggle)
    static bool headlight_on = false;
    static uint32_t last_b_press = 0;
    if ((gp->buttons & BUTTON_B) && (now - last_b_press > 500)) {
        headlight_on = !headlight_on;
        led_set_brightness(HEADLIGHT_LED_PIN, headlight_on ? 128 : 0);
        last_b_press = now;
    }

    // 6. Backlight Logic (Button Y - Toggle)
    static bool backlight_on = false;
    static uint32_t last_y_press = 0;
    if ((gp->buttons & BUTTON_Y) && (now - last_y_press > 500)) {
        backlight_on = !backlight_on;
        led_set_brightness(BACKLIGHT_LED_PIN, backlight_on ? 128 : 0);
        last_y_press = now;
        ESP_LOGI(TAG, "Backlight Toggle: %s", backlight_on ? "ON" : "OFF");
    }

    // 7. Volume Control (DPAD Up/Down)
    static int current_vol = 20;
    static uint32_t last_vol_press = 0;
    if (now - last_vol_press > 200) {
        if (gp->dpad & DPAD_UP) {
            current_vol = (current_vol < 30) ? current_vol + 1 : 30;
            sound_set_volume(current_vol);
            last_vol_press = now;
        } else if (gp->dpad & DPAD_DOWN) {
            current_vol = (current_vol > 0) ? current_vol - 1 : 0;
            sound_set_volume(current_vol);
            last_vol_press = now;
        }
    }
}

// Bluepad32 Platform setup
static void my_platform_on_init_complete(void) {
    uni_bt_start_scanning_and_autoconnect_unsafe();
    uni_bt_allow_incoming_connections(true);
}

// Track 1: Engine, Track 2: Cannon, Track 3: MG, Track 4: Connection
static uni_error_t my_platform_on_device_ready(uni_hid_device_t* d) {
    ESP_LOGI(TAG, "Controller Connected!");
    sound_play_with_loop(4, false); // Play Connection sound once
    return UNI_ERROR_SUCCESS;
}

static void my_platform_on_device_disconnected(uni_hid_device_t* d) {
    ESP_LOGI(TAG, "Controller Disconnected!");
    sound_play_with_loop(1, true); // Resume Engine sound loop
}

static void my_platform_on_controller_data(uni_hid_device_t* d, uni_controller_t* ctl) {
    handle_controller_data(d, ctl);
}

static struct uni_platform* get_my_platform(void) {
    static struct uni_platform plat = {
        .name = "KingTiger",
        .on_init_complete = my_platform_on_init_complete,
        .on_device_ready = my_platform_on_device_ready,
        .on_device_disconnected = my_platform_on_device_disconnected,
        .on_controller_data = my_platform_on_controller_data,
    };
    return &plat;
}

static void startup_sound_task(void* arg) {
    vTaskDelay(pdMS_TO_TICKS(3000));
    ESP_LOGI(TAG, "Starting Engine Sound...");
    sound_play_with_loop(1, true); // Start Engine loop
    vTaskDelete(NULL);
}

void app_main(void) {
    // 1. Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Initialize Drivers
    motor_init();
    servo_led_init();
    sound_init();
    sound_set_volume(20);

    ESP_LOGI(TAG, "Hardware Drivers Initialized");

    // 3. Start Startup Sound Task (Wait 3s)
    xTaskCreate(startup_sound_task, "startup_sound", 2048, NULL, 5, NULL);

    // 4. Initialize Bluepad32
    btstack_init();
    uni_platform_set_custom(get_my_platform());
    uni_init(0, NULL);

    ESP_LOGI(TAG, "Bluepad32 Initialized. Waiting for controller...");

    // 5. Start Run Loop (Does not return)
    btstack_run_loop_execute();
}
