/**
 * @file    main.c
 * @brief   NUCLEO-G474RE + MPU-6050 + SG90 servo sweep over UART
 *
 * Hardware connections:
 * ┌─────────────────┬──────────────────────────────────────────────┐
 * │ MPU-6050 pin    │ NUCLEO-G474RE pin                            │
 * ├─────────────────┼──────────────────────────────────────────────┤
 * │ VCC             │ 3.3 V  (CN6 pin 4)                           │
 * │ GND             │ GND    (CN6 pin 6)                           │
 * │ SCL             │ PB8  – Arduino D15 / CN5 pin 10  (I2C1_SCL) │
 * │ SDA             │ PB9  – Arduino D14 / CN5 pin 9   (I2C1_SDA) │
 * │ AD0             │ GND  → I2C address 0x68 (default)            │
 * │ INT             │ not connected (polling mode)                  │
 * └─────────────────┴──────────────────────────────────────────────┘
 * ┌─────────────────┬──────────────────────────────────────────────┐
 * │ SG90 wire       │ NUCLEO-G474RE pin                            │
 * ├─────────────────┼──────────────────────────────────────────────┤
 * │ Red   (VCC)     │ 5 V    (CN6 pin 5)                           │
 * │ Brown (GND)     │ GND    (CN6 pin 6)                           │
 * │ Orange (Signal) │ PB4  – Arduino D5  / CN9 pin 6  (TIM3_CH1)  │
 * └─────────────────┴──────────────────────────────────────────────┘
 *
 * UART output is routed through the ST-LINK Virtual COM Port (USART2,
 * PA2/PA3) at 115200 8N1 — open any serial terminal to see the data.
 *
 * Sweep behaviour:
 *   - Servo steps 1° every 20 ms (full 180° sweep ≈ 3.6 s each way).
 *   - IMU data + servo angle printed every 200 ms.
 *   - No HAL_Delay() in the main loop — timing via HAL_GetTick().
 *
 * CubeMX / CubeIDE peripheral settings required:
 *   • I2C1  : Standard Mode (100 kHz), PB8=SCL, PB9=SDA
 *   • USART2: Asynchronous, 115200 baud, 8N1 (ST-LINK VCP)
 *   • TIM3  : PWM on CH1 / PB4  (configured entirely in servo.c)
 *   • SYS   : Timebase source = SysTick
 *
 * Build: STM32CubeIDE with STM32CubeG4 HAL package.
 */

/* ------------------------------------------------------------------ */
/* Includes                                                             */
/* ------------------------------------------------------------------ */
#include "main.h"
#include "mpu6050.h"
#include "servo.h"
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Private peripheral handles                                           */
/* ------------------------------------------------------------------ */
I2C_HandleTypeDef  hi2c1;
UART_HandleTypeDef huart2;

/* ------------------------------------------------------------------ */
/* Private function prototypes                                          */
/* ------------------------------------------------------------------ */
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);
static void uart_print(const char *str);

/* ------------------------------------------------------------------ */
/* Sweep configuration                                                  */
/* ------------------------------------------------------------------ */
#define SWEEP_STEP_DEG      1       /* degrees per step               */
#define SWEEP_STEP_MS       20U     /* ms between steps (50 Hz servo) */
#define PRINT_INTERVAL_MS   200U    /* ms between UART prints         */

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();
    MX_I2C1_Init();
    Servo_Init();   /* TIM3_CH1 on PB4 — starts centred at 0° */

    uart_print("\r\n=== NUCLEO-G474RE + MPU-6050 + SG90 ===\r\n");

    /* Initialise MPU-6050 */
    MPU6050_Status_t status = MPU6050_Init(&hi2c1, MPU6050_I2C_ADDR_LOW);
    if (status == MPU6050_WRONG_ID)
    {
        uart_print("[ERROR] WHO_AM_I mismatch — check wiring and AD0 pin.\r\n");
        Error_Handler();
    }
    else if (status != MPU6050_OK)
    {
        uart_print("[ERROR] I2C communication failed — check pull-ups.\r\n");
        Error_Handler();
    }

    uart_print("[OK] MPU-6050 initialised.\r\n");
    uart_print("[OK] SG90 servo on PB4 (D5) — TIM3_CH1.\r\n\r\n");
    uart_print("Servo  Accel[g]                    Gyro[deg/s]              Temp[C]\r\n");
    uart_print("[deg]  Ax        Ay        Az       Gx        Gy        Gz\r\n");
    uart_print("------ --------- --------- -------- --------- --------- --------\r\n");

    MPU6050_Data_t data;
    char line[160];

    int16_t  angle_deg    = 0;
    int16_t  sweep_dir    = SWEEP_STEP_DEG;   /* +1 or −1 */
    uint32_t last_step_ms = 0U;
    uint32_t last_print_ms = 0U;

    while (1)
    {
        uint32_t now = HAL_GetTick();

        /* ---- Servo step ------------------------------------------- */
        if ((now - last_step_ms) >= SWEEP_STEP_MS)
        {
            last_step_ms = now;

            angle_deg += sweep_dir;

            if (angle_deg >= 90)
            {
                angle_deg = 90;
                sweep_dir = -SWEEP_STEP_DEG;
            }
            else if (angle_deg <= -90)
            {
                angle_deg = -90;
                sweep_dir = SWEEP_STEP_DEG;
            }

            Servo_SetAngle(angle_deg);
        }

        /* ---- UART print ------------------------------------------- */
        if ((now - last_print_ms) >= PRINT_INTERVAL_MS)
        {
            last_print_ms = now;

            if (MPU6050_ReadAll(&hi2c1, MPU6050_I2C_ADDR_LOW, &data) == MPU6050_OK)
            {
                snprintf(line, sizeof(line),
                         "%+6d  %+8.3f  %+8.3f  %+8.3f  "
                         "%+8.2f  %+8.2f  %+8.2f  %6.2f\r\n",
                         (int)angle_deg,
                         data.accel_x, data.accel_y, data.accel_z,
                         data.gyro_x,  data.gyro_y,  data.gyro_z,
                         data.temp_c);
                uart_print(line);
            }
            else
            {
                uart_print("[WARN] IMU read failed — retrying...\r\n");
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/* UART helper                                                          */
/* ------------------------------------------------------------------ */
static void uart_print(const char *str)
{
    HAL_UART_Transmit(&huart2, (const uint8_t *)str, (uint16_t)strlen(str),
                      HAL_MAX_DELAY);
}

/* ------------------------------------------------------------------ */
/* Peripheral initialisations                                           */
/* ------------------------------------------------------------------ */

/**
 * @brief  System clock: 24 MHz HSE → PLL → 170 MHz SYSCLK.
 */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    /* 24 MHz × 85 / 6 / 2 = 170 MHz */
    RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV6;
    RCC_OscInitStruct.PLL.PLLN = 85;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
        Error_Handler();

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK  | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
        Error_Handler();
}

/**
 * @brief  I2C1 — Standard Mode 100 kHz, PB8/PB9.
 */
static void MX_I2C1_Init(void)
{
    hi2c1.Instance              = I2C1;
    hi2c1.Init.Timing           = 0x30A0A7FB; /* 100 kHz @ 170 MHz PCLK1 */
    hi2c1.Init.OwnAddress1      = 0;
    hi2c1.Init.AddressingMode   = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode  = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2      = 0;
    hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c1.Init.GeneralCallMode  = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
        Error_Handler();

    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
        Error_Handler();
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
        Error_Handler();
}

/**
 * @brief  USART2 — 115200 8N1 (ST-LINK VCP, PA2/PA3).
 */
static void MX_USART2_UART_Init(void)
{
    huart2.Instance          = USART2;
    huart2.Init.BaudRate     = 115200;
    huart2.Init.WordLength   = UART_WORDLENGTH_8B;
    huart2.Init.StopBits     = UART_STOPBITS_1;
    huart2.Init.Parity       = UART_PARITY_NONE;
    huart2.Init.Mode         = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    huart2.Init.OneBitSampling        = UART_ONE_BIT_SAMPLE_DISABLE;
    huart2.Init.ClockPrescaler        = UART_PRESCALER_DIV1;
    huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart2) != HAL_OK)
        Error_Handler();
}

/**
 * @brief  GPIO — user LED LD2 on PA5.
 */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin   = GPIO_PIN_5;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
}

/* ------------------------------------------------------------------ */
/* MSP callbacks                                                        */
/* ------------------------------------------------------------------ */

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (hi2c->Instance == I2C1)
    {
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_I2C1_CLK_ENABLE();

        /* PB8 = I2C1_SCL, PB9 = I2C1_SDA (AF4) */
        GPIO_InitStruct.Pin       = GPIO_PIN_8 | GPIO_PIN_9;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C1)
    {
        __HAL_RCC_I2C1_FORCE_RESET();
        __HAL_RCC_I2C1_RELEASE_RESET();
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8 | GPIO_PIN_9);
    }
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (huart->Instance == USART2)
    {
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_USART2_CLK_ENABLE();

        /* PA2 = USART2_TX, PA3 = USART2_RX (AF7) */
        GPIO_InitStruct.Pin       = GPIO_PIN_2 | GPIO_PIN_3;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        __HAL_RCC_USART2_FORCE_RESET();
        __HAL_RCC_USART2_RELEASE_RESET();
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2 | GPIO_PIN_3);
    }
}

/* ------------------------------------------------------------------ */
/* Error handler                                                        */
/* ------------------------------------------------------------------ */
void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        for (volatile uint32_t i = 0; i < 500000UL; i++);
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file; (void)line;
}
#endif
