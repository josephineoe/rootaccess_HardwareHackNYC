/**
 * @file    mpu6050.h
 * @brief   MPU-6050 driver for STM32 HAL (I2C, polling mode)
 *
 * Register addresses verified against RM-MPU-6000A-00 Rev 4.0.
 *
 * Default configuration applied by MPU6050_Init():
 *   - Clock source  : PLL with X-axis gyro reference (CLKSEL = 1)
 *   - Accel range   : ±2 g   → 16384 LSB/g
 *   - Gyro range    : ±250 °/s → 131 LSB/(°/s)
 *   - DLPF          : 44 Hz bandwidth (DLPF_CFG = 3)
 *   - Sample rate   : 1 kHz / (1 + 0) = 1 kHz
 *   - Sleep mode    : disabled
 */

#ifndef MPU6050_H
#define MPU6050_H

#include "stm32g4xx_hal.h"
#include <stdint.h>

/* ------------------------------------------------------------------ */
/* I2C address                                                          */
/* ------------------------------------------------------------------ */
/* AD0 pin LOW  → 0x68, AD0 pin HIGH → 0x69.                          */
/* The 7-bit address is shifted left by 1 for HAL calls.               */
#define MPU6050_I2C_ADDR_LOW   (0x68U << 1)   /* AD0 = 0 (default)   */
#define MPU6050_I2C_ADDR_HIGH  (0x69U << 1)   /* AD0 = 1             */

/* ------------------------------------------------------------------ */
/* Register map (RM-MPU-6000A-00 §3)                                   */
/* ------------------------------------------------------------------ */
#define MPU6050_REG_SMPLRT_DIV    0x19U
#define MPU6050_REG_CONFIG        0x1AU
#define MPU6050_REG_GYRO_CONFIG   0x1BU
#define MPU6050_REG_ACCEL_CONFIG  0x1CU
#define MPU6050_REG_ACCEL_XOUT_H  0x3BU   /* burst-read start        */
#define MPU6050_REG_TEMP_OUT_H    0x41U
#define MPU6050_REG_GYRO_XOUT_H   0x43U
#define MPU6050_REG_PWR_MGMT_1    0x6BU
#define MPU6050_REG_WHO_AM_I      0x75U

/* WHO_AM_I expected value (bits [6:1] = 0x34 → byte = 0x68)          */
#define MPU6050_WHO_AM_I_VAL      0x68U

/* ------------------------------------------------------------------ */
/* Sensitivity constants (from datasheet §4.18 / §4.20)                */
/* ------------------------------------------------------------------ */
#define MPU6050_ACCEL_SENS_2G     16384.0f   /* LSB/g  at ±2 g        */
#define MPU6050_GYRO_SENS_250DPS  131.0f     /* LSB/(°/s) at ±250 °/s */

/* ------------------------------------------------------------------ */
/* Data structures                                                      */
/* ------------------------------------------------------------------ */
typedef struct {
    float accel_x;   /* [g]    */
    float accel_y;
    float accel_z;
    float gyro_x;    /* [°/s]  */
    float gyro_y;
    float gyro_z;
    float temp_c;    /* [°C]   */
} MPU6050_Data_t;

/* ------------------------------------------------------------------ */
/* Return codes                                                         */
/* ------------------------------------------------------------------ */
typedef enum {
    MPU6050_OK    = 0,
    MPU6050_ERR   = 1,
    MPU6050_WRONG_ID = 2
} MPU6050_Status_t;

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

/**
 * @brief  Initialise the MPU-6050.
 * @param  hi2c   Pointer to the HAL I2C handle.
 * @param  addr   I2C address (MPU6050_I2C_ADDR_LOW or _HIGH).
 * @retval MPU6050_OK on success, MPU6050_WRONG_ID if WHO_AM_I mismatch,
 *         MPU6050_ERR on I2C error.
 */
MPU6050_Status_t MPU6050_Init(I2C_HandleTypeDef *hi2c, uint8_t addr);

/**
 * @brief  Read accelerometer, gyroscope, and temperature in one burst.
 * @param  hi2c   Pointer to the HAL I2C handle.
 * @param  addr   I2C address used during Init.
 * @param  out    Pointer to the output data structure.
 * @retval MPU6050_OK on success, MPU6050_ERR on I2C error.
 */
MPU6050_Status_t MPU6050_ReadAll(I2C_HandleTypeDef *hi2c, uint8_t addr,
                                  MPU6050_Data_t *out);

#endif /* MPU6050_H */
