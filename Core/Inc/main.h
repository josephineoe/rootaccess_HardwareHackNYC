/**
 * @file    main.h
 * @brief   Top-level header for NUCLEO-G474RE + MPU-6050 + SG90 project.
 *
 * Mirrors the file CubeIDE generates at Core/Inc/main.h.
 * Provides the HAL umbrella include and the Error_Handler prototype
 * so every translation unit can call it without a circular dependency.
 */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* HAL umbrella include                                                 */
/* ------------------------------------------------------------------ */
#include "stm32g4xx_hal.h"

/* ------------------------------------------------------------------ */
/* Exported function prototypes                                         */
/* ------------------------------------------------------------------ */
void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
