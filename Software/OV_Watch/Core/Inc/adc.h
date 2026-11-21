/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    adc.h
  * @brief   adc.c 的函数声明。
  *
  * 中文说明：
  *   ADC1 用于采集电池电压（PA1 分压输入），
  *   hadc1 为 CubeMX 生成的 ADC 句柄，业务层通过 HAL_ADC 接口读取。
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
#ifndef __ADC_H__
#define __ADC_H__	// 防止重复包含

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern ADC_HandleTypeDef hadc1;	// ADC1 句柄（电池电压采样使用）

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

/** 初始化 ADC1：时钟、通道、采样时间等参数 */
void MX_ADC1_Init(void);

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __ADC_H__ */
