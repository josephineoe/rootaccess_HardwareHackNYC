# Plan: SG90 Servo Sweep Project (STM32G474RE / NUCLEO-G474RE)

## Goal
Add a self-contained SG90 servo driver and sweep loop to the existing project.
The servo PWM signal must use a pin that does **not** conflict with:
- **PB8 / PB9** — I2C1 SCL/SDA (MPU-6050)
- **PA2 / PA3** — USART2 TX/RX (ST-LINK VCP)
- **PA5**        — User LED LD2

---

## Pin & Timer Selection

| Signal | MCU Pin | Arduino Header | Timer Channel | AF |
|--------|---------|----------------|---------------|----|
| Servo PWM | **PA0** | **D1 (CN9 pin 1)** | TIM2_CH1 | AF1 |

**Why PA0 / TIM2_CH1:**
- Confirmed free in PeripheralPins.c (no I2C, UART, or LED conflict)
- TIM2 is a 32-bit general-purpose timer — ideal for precise servo timing
- AF1 = `GPIO_AF1_TIM2` on STM32G474

### PWM Timing for SG90 (50 Hz, 20 ms period)

```
SYSCLK  = 170 MHz
PCLK1   = 170 MHz (APB1 divider = 1, so TIM2 clock = 170 MHz)
PSC     = 169   → timer tick = 170 MHz / 170 = 1 MHz (1 µs per tick)
ARR     = 19999 → period = 20 000 µs = 20 ms = 50 Hz

CCR (pulse width):
  -90°  →  1000 ticks (1.0 ms)
    0°  →  1500 ticks (1.5 ms)
  +90°  →  2000 ticks (2.0 ms)
```

---

## Wiring — SG90 to NUCLEO-G474RE

| SG90 Wire | NUCLEO-G474RE Pin | Connector |
|-----------|-------------------|-----------|
| Red (VCC) | 5V | CN6 pin 5 (or USB 5V via morpho) |
| Brown (GND) | GND | CN6 pin 6 |
| Orange (Signal) | **PA0** | CN9 pin 1 (Arduino D1) |

> ⚠️ SG90 runs on 4.8–6V. Use the board's 5V rail (from USB), **not** 3.3V.
> The STM32 GPIO output is 3.3V logic — the SG90 signal input accepts this.

---

## Files to Create / Modify

### New files
1. `Core/Inc/servo.h`   — Servo driver public API
2. `Core/Src/servo.c`   — Servo driver implementation (TIM2 PWM init + angle setter)

### Modified files
3. `Core/Src/main.c`    — Add servo init, sweep loop, and UART status prints

---

## Step-by-Step Implementation

### Step 1 — `Core/Inc/servo.h`

Define:
- `SERVO_TIM_INSTANCE`  = `TIM2`
- `SERVO_TIM_CHANNEL`   = `TIM_CHANNEL_1`
- `SERVO_GPIO_PORT`     = `GPIOA`
- `SERVO_GPIO_PIN`      = `GPIO_PIN_0`
- `SERVO_GPIO_AF`       = `GPIO_AF1_TIM2`
- `SERVO_PULSE_MIN_US`  = `1000`  (−90°)
- `SERVO_PULSE_MID_US`  = `1500`  (0°)
- `SERVO_PULSE_MAX_US`  = `2000`  (+90°)
- `SERVO_PERIOD_US`     = `20000` (50 Hz)
- `SERVO_PSC`           = `169`
- `SERVO_ARR`           = `19999`

Public API:
```c
void             Servo_Init(void);
void             Servo_SetAngle(int16_t angle_deg);  // -90 to +90
void             Servo_SetPulse(uint16_t pulse_us);  // 1000–2000
```

Return type is `void`; errors are silent (clamp out-of-range inputs).

### Step 2 — `Core/Src/servo.c`

**`Servo_Init()`**
1. Enable GPIOA clock (`__HAL_RCC_GPIOA_CLK_ENABLE()`)
2. Configure PA0 as AF push-pull, no pull, high speed, AF1
3. Enable TIM2 clock (`__HAL_RCC_TIM2_CLK_ENABLE()`)
4. Init `TIM_HandleTypeDef htim2`:
   - Instance = TIM2
   - Prescaler = `SERVO_PSC` (169)
   - CounterMode = UP
   - Period = `SERVO_ARR` (19999)
   - AutoReloadPreload = ENABLE
5. Call `HAL_TIM_PWM_Init(&htim2)`
6. Configure OC channel:
   - Mode = PWM1
   - Pulse = `SERVO_PULSE_MID_US` (start centred)
   - Polarity = HIGH
   - FastMode = DISABLE
7. Call `HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, SERVO_TIM_CHANNEL)`
8. Call `HAL_TIM_PWM_Start(&htim2, SERVO_TIM_CHANNEL)`

**`Servo_SetAngle(int16_t angle_deg)`**
- Clamp angle to [−90, +90]
- Map linearly: `pulse_us = 1500 + (angle_deg * 500) / 90`
  - angle=−90 → 1000 µs, angle=0 → 1500 µs, angle=+90 → 2000 µs
- Call `Servo_SetPulse(pulse_us)`

**`Servo_SetPulse(uint16_t pulse_us)`**
- Clamp to [SERVO_PULSE_MIN_US, SERVO_PULSE_MAX_US]
- Write directly: `__HAL_TIM_SET_COMPARE(&htim2, SERVO_TIM_CHANNEL, pulse_us)`
  - This is a single register write, safe to call from main context at 10 Hz

> **Note:** `htim2` is declared `static` inside `servo.c` — it is not exposed
> globally. `main.c` only calls the public API functions.

### Step 3 — `Core/Src/main.c` modifications

Add `#include "servo.h"` after the existing includes.

In `main()`, after `MX_USART2_UART_Init()` / `MX_I2C1_Init()`:
```c
Servo_Init();
uart_print("[OK] Servo initialised on PA0 (TIM2_CH1).\r\n");
```

Replace (or augment) the existing `while(1)` loop with a sweep pattern:

```
Sweep logic (non-blocking style using HAL_GetTick):
  - State machine: SWEEP_TO_MAX → HOLD → SWEEP_TO_MIN → HOLD → repeat
  - Step size: 1° per 20 ms → full 180° sweep in ~3.6 s
  - Every step: call Servo_SetAngle(current_angle)
  - Every 500 ms: print current angle + IMU data over UART
```

Concrete loop structure:
```c
int16_t  angle     = -90;
int8_t   direction = +1;          /* +1 = sweeping toward +90 */
uint32_t last_step = HAL_GetTick();
uint32_t last_print = HAL_GetTick();

while (1)
{
    uint32_t now = HAL_GetTick();

    /* Move servo 1° every 20 ms */
    if (now - last_step >= 20U)
    {
        last_step = now;
        Servo_SetAngle(angle);
        angle += direction;
        if (angle >= 90)  { angle = 90;  direction = -1; }
        if (angle <= -90) { angle = -90; direction = +1; }
    }

    /* Print IMU + angle every 200 ms */
    if (now - last_print >= 200U)
    {
        last_print = now;
        MPU6050_Data_t data;
        if (MPU6050_ReadAll(&hi2c1, MPU6050_I2C_ADDR_LOW, &data) == MPU6050_OK)
        {
            snprintf(line, sizeof(line),
                     "Angle:%+4d  Ax:%+6.2f  Ay:%+6.2f  Az:%+6.2f  "
                     "Gx:%+7.1f  Gy:%+7.1f  Gz:%+7.1f  T:%5.1f\r\n",
                     angle,
                     data.accel_x, data.accel_y, data.accel_z,
                     data.gyro_x,  data.gyro_y,  data.gyro_z,
                     data.temp_c);
            uart_print(line);
        }
    }
}
```

The `line` buffer stays at 128 bytes (the format above produces ~95 chars max — fits safely).

**No `HAL_Delay()` in the loop** — timing is tick-based so the servo steps and
UART prints are decoupled and neither blocks the other.

---

## Timer Clock Note

TIM2 is on APB1. In `SystemClock_Config()`, `APB1CLKDivider = RCC_HCLK_DIV1`,
so APB1 = 170 MHz and the TIM2 input clock = 170 MHz. The prescaler of 169
gives exactly 1 MHz → 1 µs resolution. No changes to `SystemClock_Config()` needed.

---

## Summary of Changes

| File | Action | Key content |
|------|--------|-------------|
| `Core/Inc/servo.h` | **Create** | Macros, struct-free API declarations |
| `Core/Src/servo.c` | **Create** | TIM2 PWM init, angle→pulse mapping, CCR write |
| `Core/Src/main.c`  | **Modify** | Add `#include "servo.h"`, `Servo_Init()` call, tick-based sweep loop |

No changes to `mpu6050.h`, `mpu6050.c`, linker script, or clock config.
