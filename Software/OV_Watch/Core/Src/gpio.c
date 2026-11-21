/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
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
 * 【中文导读】gpio.c —— CubeMX 生成的"全局 GPIO"初始化
 * ----------------------------------------------------------------------------
 * 这里只做两件事：打开 A/B/C 三个端口的时钟，并把 PC13 配成推挽输出（默认低）。
 * PC13 在本工程里是板载 LED（freertos.c 的默认任务会翻转它做心跳），也可以
 * 用来判断程序有没有跑起来——上电后闪就说明调度器正常。
 * PC13 属于 F4 的备份域引脚，驱动能力只有 3mA 左右、速度上限也低，所以 Speed
 * 必须用 LOW；它不适合驱动大电流负载。
 * 其它引脚（I2C、按键、LCD 控制线、EXTI）都不在这里配：BSP 各自初始化时
 * 单独申请引脚，避免 CubeMX 重新生成时把 BSP 的配置冲掉。
 * 调用顺序：main.c 里 MX_GPIO_Init() 最先调用，保证后面各外设配置时端口时钟已开。
 * ============================================================================
 */

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
/**
  * @brief  初始化所有"通用"GPIO：时钟 + PC13 输出。
  * @note   先开时钟再写寄存器是 GPIO 的硬性顺序；GPIO_InitStruct 在时钟之后
  *         才定义，是 CubeMX 生成代码的习惯（不影响功能）。
  * @retval None
  */
void MX_GPIO_Init(void)
{

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();                  // PC13(LED) 所在端口
  __HAL_RCC_GPIOA_CLK_ENABLE();                  // PA：串口/ADC/按键/I2C 等，先开时钟供各 BSP 使用
  __HAL_RCC_GPIOB_CLK_ENABLE();                  // PB：SPI1/背光/I2C 等
	
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	
	/*Configure GPIO pin Output Level */
  // 先把输出锁存器写成低电平，再切换成输出模式：这样引脚不会在"变输出"的瞬间
  // 冒出高电平毛刺，LED 不会闪一下。
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;             // 板载 LED
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;    // 推挽输出：能主动拉高/拉低，不用外部上拉
  GPIO_InitStruct.Pull = GPIO_NOPULL;            // 推挽输出不需要内部上下拉
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;   // PC13 在备份域，速度/驱动能力有限，必须低速
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
