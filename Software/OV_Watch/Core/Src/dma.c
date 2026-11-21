/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    dma.c
  * @brief   This file provides code for the configuration
  *          of all the requested memory to memory DMA transfers.
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
/*
 * ============================================================================
 * 【中文导读】dma.c —— DMA2 控制器与三条流的 NVIC 使能
 * ----------------------------------------------------------------------------
 * 本工程的外设都在 APB2 上（USART1、SPI1），它们的 DMA 请求只能走 DMA2，
 * 所以这里只开 DMA2 的时钟，不碰 DMA1。
 * 三条流各司其职：Stream2 = SPI1_TX（刷屏）、Stream5 = USART1_RX（收蓝牙）、
 * Stream7 = USART1_TX（发蓝牙）。通道/方向/数据宽度的具体配置不在这里，而在
 * usart.c / spi.c 的 MspInit 里——本文件只负责"开时钟 + 开中断"。
 * 中断优先级统一给 5，等于 FreeRTOS 的 configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY，
 * 保证 DMA 完成回调里可以安全调用 RTOS API（HAL 回调最终会走到 LVGL 的刷新通知）。
 * 调用顺序很重要：main.c 里 MX_DMA_Init() 必须排在 MX_SPI1_Init/MX_USART1_UART_Init
 * 之前，否则 HAL_DMA_Init 写 DMA 寄存器时时钟还没开，配置会丢。
 * ============================================================================
 */

/* Includes ------------------------------------------------------------------*/
#include "dma.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure DMA                                                              */
/*----------------------------------------------------------------------------*/

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/**
  * Enable DMA controller clock
  */
/**
  * @brief  打开 DMA2 时钟并使能三条流的中断。
  * @note   为什么是 DMA2：USART1/SPI1 挂在 APB2，其 DMA 请求固定连到 DMA2。
  *         三条流的中断处理函数在 stm32f4xx_it.c 里，只是把中断转给
  *         HAL_DMA_IRQHandler，由 HAL 去调完成/半完成/错误回调。
  *         优先级 5 = configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY：HAL 回调最终会
  *         调用 LVGL 的 lv_disp_flush_ready；取 5 是"允许在 ISR 里调 RTOS API"的
  *         上限，数值再小就会屏蔽内核临界区，DMA 回调里就不能碰 FreeRTOS 了。
  * @retval None
  */
void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();                   // 不开这个时钟，后面 HAL_DMA_Init 的所有配置都写不进去

  /* DMA interrupt init */
  /* DMA2_Stream2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 5, 0); // SPI1_TX：一帧像素搬完触发传输完成中断
  HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);
  /* DMA2_Stream5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream5_IRQn, 5, 0); // USART1_RX：收满 25 字节或出错时进中断
  HAL_NVIC_EnableIRQ(DMA2_Stream5_IRQn);
  /* DMA2_Stream7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 5, 0); // USART1_TX：printf 的最后一个字节发完时进中断
  HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */

