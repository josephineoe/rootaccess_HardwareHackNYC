/**
 * @file    servo.h
 * @brief   SG90 servo driver — TIM3_CH1 PWM on PB4 (Arduino D5).
 *
 * Hardware connection:
 * ┌──────────────────┬──────────────────────────────────────────────┐
 * │ SG90 wire        │ NUCLEO-G474RE pin                            │
 * ├──────────────────┼──────────────────────────────────────────────┤
 * │ Red   (VCC)      │ 5 V  — CN6 pin 5                             │
 * │ Brown (GND)      │ GND  — CN6 pin 6                             │
 * │ Orange (Signal)  │ PB4  — Arduino D5 / CN9 pin 6  (TIM3_CH1)   │
 * └──────────────────┴──────────────────────────────────────────────┘
 *
 * PWM timing (50 Hz, 20 ms period):
 *   SYSCLK = 170 MHz, APB1 divider = 1 → TIM3 clock = 170 MHz
 *   PSC = 169  →  tick = 1 µs
 *   ARR = 19999 →  period = 20 000 µs = 50 Hz
 *
 *   CCR:  1000 = −90°  |  1500 = 0°  |  2000 = +90°
 */

#ifndef SERVO_H
#define SERVO_H

#include "stm32g4xx_hal.h"
#include <stdint.h>

/* ------------------------------------------------------------------ */
/* Hardware constants                                                   */
/* ------------------------------------------------------------------ */
#define SERVO_TIM_INSTANCE      TIM3
#define SERVO_TIM_CHANNEL       TIM_CHANNEL_1
#define SERVO_GPIO_PORT         GPIOB
#define SERVO_GPIO_PIN          GPIO_PIN_4
#define SERVO_GPIO_AF           GPIO_AF2_TIM3   /* AF2 = TIM3 on STM32G474 */

/* ------------------------------------------------------------------ */
/* Timing constants                                                     */
/* ------------------------------------------------------------------ */
#define SERVO_PSC               169U            /* 170 MHz / 170 = 1 MHz  */
#define SERVO_ARR               19999U          /* 20 000 ticks = 20 ms   */

#define SERVO_PULSE_MIN_US      1000U           /* −90°  */
#define SERVO_PULSE_MID_US      1500U           /*   0°  */
#define SERVO_PULSE_MAX_US      2000U           /* +90°  */

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

/**
 * @brief  Initialise TIM3_CH1 PWM on PB4 for SG90 servo control.
 *         Servo starts centred at 0° (1500 µs pulse).
 *         Call once after SystemClock_Config().
 */
void Servo_Init(void);

/**
 * @brief  Set servo angle.
 * @param  angle_deg  Target angle in degrees, clamped to [−90, +90].
 */
void Servo_SetAngle(int16_t angle_deg);

/**
 * @brief  Set servo pulse width directly.
 * @param  pulse_us  Pulse width in microseconds, clamped to [1000, 2000].
 */
void Servo_SetPulse(uint16_t pulse_us);

#endif /* SERVO_H */
