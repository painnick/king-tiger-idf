#include "driver/uart.h"
#include "pin_config.h"
#include "esp_log.h"

static const char *TAG = "SOUND";

void sound_init(void) {
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_param_config(SOUND_UART_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(SOUND_UART_PORT, SOUND_TX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(SOUND_UART_PORT, 256, 0, 0, NULL, 0));
    ESP_LOGI(TAG, "JQ6500 UART initialized on Pin %d", SOUND_TX_PIN);
}

void sound_play_index(int index) {
    // CMD: 7E 04 03 00 [INDEX] EF (Index 1-based)
    uint8_t cmd[] = {0x7E, 0x04, 0x03, 0x00, (uint8_t)index, 0xEF};
    uart_write_bytes(SOUND_UART_PORT, (const char *)cmd, sizeof(cmd));
    ESP_LOGI(TAG, "Playing sound index: %d", index);
}

void sound_set_loop_mode(uint8_t mode) {
    // CMD: 7E 03 11 [MODE] EF
    // 02: Single Loop, 04: One Stop
    uint8_t cmd[] = {0x7E, 0x03, 0x11, mode, 0xEF};
    uart_write_bytes(SOUND_UART_PORT, (const char *)cmd, sizeof(cmd));
    ESP_LOGD(TAG, "Loop mode set to: %d", mode);
}

void sound_play_with_loop(int index, bool loop) {
    sound_play_index(index);
    vTaskDelay(pdMS_TO_TICKS(50)); // Small delay for module to process
    sound_set_loop_mode(loop ? 0x02 : 0x04);
}

void sound_set_volume(int volume) {
    // CMD: 7E 03 06 [VOL] EF (Vol 0-30)
    if (volume > 30) volume = 30;
    uint8_t cmd[] = {0x7E, 0x03, 0x06, (uint8_t)volume, 0xEF};
    uart_write_bytes(SOUND_UART_PORT, (const char *)cmd, sizeof(cmd));
    ESP_LOGI(TAG, "Volume set to: %d", volume);
}
