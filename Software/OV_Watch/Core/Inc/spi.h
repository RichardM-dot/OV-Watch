/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    spi.h
  * @brief   spi.c 的函数声明。
  *
  * 中文说明：
  *   SPI1 用于 LCD 刷屏输出（配合 DMA 发送像素数据），
  *   hspi1 为 SPI 句柄，hdma_spi1_tx 为发送专用 DMA 句柄。
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
#ifndef __SPI_H__
#define __SPI_H__	// 防止重复包含

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern SPI_HandleTypeDef hspi1;	// SPI1 句柄（驱动 LCD）

/* USER CODE BEGIN Private defines */
extern DMA_HandleTypeDef hdma_spi1_tx;	// SPI1 发送 DMA 句柄
/* USER CODE END Private defines */

/** 初始化 SPI1：主机模式、数据位宽、时钟极性/相位、波特率 */
void MX_SPI1_Init(void);

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __SPI_H__ */
