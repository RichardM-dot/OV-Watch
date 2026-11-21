/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    tim.h
  * @brief   tim.c 的函数声明。
  *
  * 中文说明：
  *   TIM3 输出 PWM 用于 LCD 背光亮度调节（PWM_PERIOD 为自动重装值，
  *   PWM_COMPARE 为初始比较值，占空比 = COMPARE / PERIOD）。
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __TIM_H__
#define __TIM_H__	// 防止重复包含

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#define PWM_PERIOD 300	// PWM 周期（自动重装值），决定背光 PWM 频率
#define PWM_COMPARE 0	// PWM 初始比较值（0 表示初始占空比为 0）
/* USER CODE END Includes */

extern TIM_HandleTypeDef htim3;	// TIM3 句柄（背光 PWM）

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

/** 初始化 TIM3：预分频、周期、PWM 输出通道 */
void MX_TIM3_Init(void);

/** HAL 定时器 MSP 后置初始化：配置 PWM 输出引脚（由 HAL_TIM_Base_MspInit 调用） */
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __TIM_H__ */
