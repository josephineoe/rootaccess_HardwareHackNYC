/**
 * @file    servo.c
 * @brief   SG90 servo driver — TIM3_CH1 PWM on PB4 (Arduino D5).
 *
 * See servo.h for wiring and timing details.
 */

#include "servo.h"
#include <stddef.h>

/* ------------------------------------------------------------------ */
/* Private handle                                                       */
/* ------------------------------------------------------------------ */
static TIM_HandleTypeDef htim3;

/* ------------------------------------------------------------------ */
/* Servo_Init                                                           */
/* ------------------------------------------------------------------ */
void Servo_Init(void)
{
    /* --- GPIO: PB4, AF2 (TIM3_CH1) --------------------------------- */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin       = SERVO_GPIO_PIN;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_LOW;
    gpio.Alternate = SERVO_GPIO_AF;
    HAL_GPIO_Init(SERVO_GPIO_PORT, &gpio);

    /* --- TIM3 base -------------------------------------------------- */
    __HAL_RCC_TIM3_CLK_ENABLE();

    htim3.Instance               = SERVO_TIM_INSTANCE;
    htim3.Init.Prescaler         = SERVO_PSC;
    htim3.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim3.Init.Period            = SERVO_ARR;
    htim3.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&htim3);

    /* --- TIM3 CH1 PWM channel --------------------------------------- */
    TIM_OC_InitTypeDef oc = {0};
    oc.OCMode       = TIM_OCMODE_PWM1;
    oc.Pulse        = SERVO_PULSE_MID_US;   /* start centred at 0° */
    oc.OCPolarity   = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode   = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim3, &oc, SERVO_TIM_CHANNEL);

    /* Start PWM output */
    HAL_TIM_PWM_Start(&htim3, SERVO_TIM_CHANNEL);
}

/* ------------------------------------------------------------------ */
/* Servo_SetPulse                                                       */
/* ------------------------------------------------------------------ */
void Servo_SetPulse(uint16_t pulse_us)
{
    /* Clamp to safe range */
    if (pulse_us < SERVO_PULSE_MIN_US) pulse_us = SERVO_PULSE_MIN_US;
    if (pulse_us > SERVO_PULSE_MAX_US) pulse_us = SERVO_PULSE_MAX_US;

    /* Write CCR1 directly — no need to stop/restart PWM */
    __HAL_TIM_SET_COMPARE(&htim3, SERVO_TIM_CHANNEL, pulse_us);
}

/* ------------------------------------------------------------------ */
/* Servo_SetAngle                                                       */
/* ------------------------------------------------------------------ */
void Servo_SetAngle(int16_t angle_deg)
{
    /* Clamp to [−90, +90] */
    if (angle_deg < -90) angle_deg = -90;
    if (angle_deg >  90) angle_deg =  90;

    /*
     * Linear map: −90° → 1000 µs, 0° → 1500 µs, +90° → 2000 µs
     *   pulse = 1500 + (angle * 500 / 90)
     * Use int32 to avoid overflow before the division.
     */
    int32_t pulse = 1500 + ((int32_t)angle_deg * 500) / 90;
    Servo_SetPulse((uint16_t)pulse);
}
