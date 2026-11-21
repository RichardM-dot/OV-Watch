/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    rtc.h
  * @brief   rtc.c 的函数声明。
  *
  * 中文说明：
  *   RTC 提供手表的时间/日期基准，掉电后由纽扣电池或超级电容维持走时。
  *   除 CubeMX 生成的 MX_RTC_Init 外，本工程还封装了设置时间/日期
  *   以及计算星期的辅助函数。
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
#ifndef __RTC_H__
#define __RTC_H__	// 防止重复包含

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern RTC_HandleTypeDef hrtc;	// RTC 句柄

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

/** 初始化 RTC：时钟源、预分频（1Hz 计数）、初始时间/日期 */
void MX_RTC_Init(void);

/* USER CODE BEGIN Prototypes */
/** 设置 RTC 时间（24 小时制） */
void RTC_SetTime(uint8_t hours, uint8_t minutes, uint8_t seconds);
/** 设置 RTC 日期（年份为 0~99 的两位表示） */
void RTC_SetDate(uint8_t year, uint8_t month, uint8_t date);
/** 计算星期：y 年、m 月、d 日、c 世纪，返回 0~6（0 表示周日） */
uint8_t weekday_calculate(int y,int m,int d,int c);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __RTC_H__ */
