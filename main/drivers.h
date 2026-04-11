#pragma once

// Motor
void motor_init(void);
void motor_set_tracks(int left, int right);
void motor_set_turret(int speed);
void motor_set_mount(int speed);

// Servo & LED
void servo_led_init(void);
void servo_set_angle(int angle);
void led_set_brightness(int pin, int brightness);

// Sound
void sound_init(void);
void sound_play_index(int index);
void sound_play_with_loop(int index, bool loop);
void sound_set_volume(int volume);
