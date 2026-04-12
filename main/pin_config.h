#pragma once

// Motor Driver #1: Tracks (Left/Right)
// Physical sequence: 27, 26, 25, 33
#define TRACK_AIN1_PIN  27
#define TRACK_AIN2_PIN  26
#define TRACK_BIN1_PIN  25
#define TRACK_BIN2_PIN  33

// Motor Driver #2: Turret and Po-Mount
// Tracks & Turret & Port Mount (DRV8833)
#define TURRET_AIN1_PIN 22
#define TURRET_AIN2_PIN 21

#define PORT_BIN1_PIN   19
#define PORT_BIN2_PIN   18

// LEDs
#define MG_LED_PIN        16
#define HEADLIGHT_LED_PIN 5
#define CANNON_LED_PIN    17
#define BACKLIGHT_LED_PIN 4

// Sound (JQ6500)
#define SOUND_TX_PIN 23
#define SOUND_UART_PORT UART_NUM_1

// Servo
#define CANNON_SERVO_PIN 32
