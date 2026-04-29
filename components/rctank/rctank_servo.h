/**
 * @file rctank_servo.h
 * @brief RC Tank 서보 제어 (LEDC, C 함수)
 */
#ifndef RCTANK_SERVO_H
#define RCTANK_SERVO_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 포신 서보 기본 각도 (당기기 전) */
#define RCTANK_SERVO_GUN_DEG_REST 90

esp_err_t rctank_servo_init(void);

/**
 * @brief 포신 서보 각도 설정 (B 버튼 당기기용)
 * @param degree 0~180, 기본 90
 */
void rctank_servo_gun_set_degree(int degree);

/**
 * @brief 포신 서보 활성화/비활성화
 * @param enable true: 활성화(토크 유지), false: 비활성화(풀림)
 */
void rctank_servo_gun_enable(int enable);

#ifdef __cplusplus
}
#endif

#endif /* RCTANK_SERVO_H */
