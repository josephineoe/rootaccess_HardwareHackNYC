/**
 * @file    mpu6050.c
 * @brief   MPU-6050 driver implementation for STM32 HAL (I2C, polling)
 *
 * All register addresses and sensitivity values are taken directly from
 * the InvenSense register map document RM-MPU-6000A-00 Rev 4.0.
 */

#include "mpu6050.h"

/* ------------------------------------------------------------------ */
/* Internal helpers                                                     */
/* ------------------------------------------------------------------ */

#define I2C_TIMEOUT_MS  100U

/**
 * Write a single byte to a register.
 */
static MPU6050_Status_t write_reg(I2C_HandleTypeDef *hi2c, uint8_t addr,
                                   uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };
    if (HAL_I2C_Master_Transmit(hi2c, addr, buf, 2, I2C_TIMEOUT_MS) != HAL_OK)
        return MPU6050_ERR;
    return MPU6050_OK;
}

/**
 * Read one or more bytes starting at a register address.
 * The MPU-6050 auto-increments the internal register pointer on burst reads.
 */
static MPU6050_Status_t read_regs(I2C_HandleTypeDef *hi2c, uint8_t addr,
                                   uint8_t reg, uint8_t *buf, uint16_t len)
{
    /* Send register address */
    if (HAL_I2C_Master_Transmit(hi2c, addr, &reg, 1, I2C_TIMEOUT_MS) != HAL_OK)
        return MPU6050_ERR;
    /* Receive data */
    if (HAL_I2C_Master_Receive(hi2c, addr, buf, len, I2C_TIMEOUT_MS) != HAL_OK)
        return MPU6050_ERR;
    return MPU6050_OK;
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

MPU6050_Status_t MPU6050_Init(I2C_HandleTypeDef *hi2c, uint8_t addr)
{
    uint8_t who_am_i = 0;

    /* 1. Verify device identity ------------------------------------ */
    if (read_regs(hi2c, addr, MPU6050_REG_WHO_AM_I, &who_am_i, 1) != MPU6050_OK)
        return MPU6050_ERR;

    if (who_am_i != MPU6050_WHO_AM_I_VAL)
        return MPU6050_WRONG_ID;

    /* 2. Wake the device up (reset value of PWR_MGMT_1 = 0x40 = SLEEP)
     *    CLKSEL = 1 → PLL with X-axis gyro reference (recommended)   */
    if (write_reg(hi2c, addr, MPU6050_REG_PWR_MGMT_1, 0x01U) != MPU6050_OK)
        return MPU6050_ERR;

    HAL_Delay(10); /* allow oscillator to stabilise */

    /* 3. Sample rate divider: SMPLRT_DIV = 0 → Fs = 1 kHz           */
    if (write_reg(hi2c, addr, MPU6050_REG_SMPLRT_DIV, 0x00U) != MPU6050_OK)
        return MPU6050_ERR;

    /* 4. DLPF: DLPF_CFG = 3 → accel BW 44 Hz, gyro BW 42 Hz        */
    if (write_reg(hi2c, addr, MPU6050_REG_CONFIG, 0x03U) != MPU6050_OK)
        return MPU6050_ERR;

    /* 5. Gyro full-scale: FS_SEL = 0 → ±250 °/s                     */
    if (write_reg(hi2c, addr, MPU6050_REG_GYRO_CONFIG, 0x00U) != MPU6050_OK)
        return MPU6050_ERR;

    /* 6. Accel full-scale: AFS_SEL = 0 → ±2 g                       */
    if (write_reg(hi2c, addr, MPU6050_REG_ACCEL_CONFIG, 0x00U) != MPU6050_OK)
        return MPU6050_ERR;

    return MPU6050_OK;
}

MPU6050_Status_t MPU6050_ReadAll(I2C_HandleTypeDef *hi2c, uint8_t addr,
                                  MPU6050_Data_t *out)
{
    /*
     * Burst-read 14 bytes starting at ACCEL_XOUT_H (0x3B):
     *   [0-1]  ACCEL_XOUT  H/L
     *   [2-3]  ACCEL_YOUT  H/L
     *   [4-5]  ACCEL_ZOUT  H/L
     *   [6-7]  TEMP_OUT    H/L
     *   [8-9]  GYRO_XOUT   H/L
     *   [10-11] GYRO_YOUT  H/L
     *   [12-13] GYRO_ZOUT  H/L
     *
     * All values are 16-bit two's complement (RM-MPU-6000A-00 §4.18–4.20).
     */
    uint8_t raw[14];

    if (read_regs(hi2c, addr, MPU6050_REG_ACCEL_XOUT_H, raw, 14) != MPU6050_OK)
        return MPU6050_ERR;

    int16_t ax_raw = (int16_t)((raw[0]  << 8) | raw[1]);
    int16_t ay_raw = (int16_t)((raw[2]  << 8) | raw[3]);
    int16_t az_raw = (int16_t)((raw[4]  << 8) | raw[5]);
    int16_t t_raw  = (int16_t)((raw[6]  << 8) | raw[7]);
    int16_t gx_raw = (int16_t)((raw[8]  << 8) | raw[9]);
    int16_t gy_raw = (int16_t)((raw[10] << 8) | raw[11]);
    int16_t gz_raw = (int16_t)((raw[12] << 8) | raw[13]);

    /* Convert to physical units */
    out->accel_x = (float)ax_raw / MPU6050_ACCEL_SENS_2G;
    out->accel_y = (float)ay_raw / MPU6050_ACCEL_SENS_2G;
    out->accel_z = (float)az_raw / MPU6050_ACCEL_SENS_2G;

    /* Temperature formula from RM-MPU-6000A-00 §4.19 */
    out->temp_c  = (float)t_raw / 340.0f + 36.53f;

    out->gyro_x  = (float)gx_raw / MPU6050_GYRO_SENS_250DPS;
    out->gyro_y  = (float)gy_raw / MPU6050_GYRO_SENS_250DPS;
    out->gyro_z  = (float)gz_raw / MPU6050_GYRO_SENS_250DPS;

    return MPU6050_OK;
}
