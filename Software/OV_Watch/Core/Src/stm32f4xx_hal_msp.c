/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file         stm32f4xx_hal_msp.c
  * @brief        This file provides code for the MSP Initialization
  *               and de-Initialization codes.
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
 * 【中文导读】stm32f4xx_hal_msp.c —— 全局 MSP（MCU 支撑包）初始化
 * ----------------------------------------------------------------------------
 * HAL_MspInit 由 HAL_Init() 在最早期调用（早于 SystemClock_Config 和所有外设
 * 初始化），只做三件"所有外设都依赖"的事：
 *   1) 开 SYSCFG 时钟：F4 的 EXTI 要把某个端口映射到中断线（EXTI->EXTICR），
 *      这些寄存器在 SYSCFG 里，不开时钟则 EXTI2/4/9_5/15_10 全部失效；
 *   2) 开 PWR 时钟：访问 PWR 寄存器（电压等级 VOS、备份域写保护 DBP）需要它；
 *      rtc.c 里写 BKP_DR0 能成功，靠的就是这里先把 PWR 时钟打开；
 *   3) 把 PendSV 设成最低优先级 15：FreeRTOS 用 PendSV 做任务上下文切换，
 *      它必须能被所有外设中断打断，否则高优先级中断会被"换任务"拖住。
 * 各外设自己的引脚/时钟/DMA 不在这里，而在 usart.c/spi.c/adc.c 等文件的
 * HAL_xxx_MspInit 里，CubeMX 按外设拆分，便于重新生成时互不影响。
 * ============================================================================
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN Define */

/* USER CODE END Define */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN Macro */

/* USER CODE END Macro */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* External functions --------------------------------------------------------*/
/* USER CODE BEGIN ExternalFunctions */

/* USER CODE END ExternalFunctions */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */
/**
  * Initializes the Global MSP.
  */
/**
  * @brief  HAL 全局 MSP 初始化，HAL_Init() 内部第一个调用。
  * @note   此时系统还跑在 HSI 16MHz 上，所以这里不能做依赖主频的计算（比如
  *         波特率、定时器预分频），那些留给 SystemClock_Config 之后再配。
  * @retval None
  */
void HAL_MspInit(void)
{
  /* USER CODE BEGIN MspInit 0 */

  /* USER CODE END MspInit 0 */

  __HAL_RCC_SYSCFG_CLK_ENABLE();                 // EXTI 的端口映射寄存器在 SYSCFG，不开时钟所有 EXTI 都不工作
  __HAL_RCC_PWR_CLK_ENABLE();                    // 访问 PWR 寄存器（VOS/DBP）的前提；RTC 备份寄存器写入也依赖 DBP

  /* System interrupt init*/
  /* PendSV_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0);      // 上下文切换必须最低优先级，保证任何外设中断都能抢占它

  /* USER CODE BEGIN MspInit 1 */

  /* USER CODE END MspInit 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
