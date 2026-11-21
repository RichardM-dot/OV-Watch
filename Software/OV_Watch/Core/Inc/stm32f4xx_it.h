/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f4xx_it.h
  * @brief   中断服务函数声明。
  *
  * 中文说明：
  *   集中声明本工程实际用到的 Cortex-M4 内核异常与 STM32 外设中断：
  *   - 系统异常：NMI / HardFault / MemManage / BusFault / UsageFault 等
  *   - RTC 唤醒、定时器、串口、DMA
  *   - 外部中断 EXTI（按键、触摸、传感器中断）
  *   函数实体在 stm32f4xx_it.c 中实现。
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
#ifndef __STM32F4xx_IT_H
#define __STM32F4xx_IT_H	// 防止重复包含

#ifdef __cplusplus
 extern "C" {
#endif

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
extern uint8_t HardInt_receive_str[25];	// 串口中断接收缓冲区（25 字节）
/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void NMI_Handler(void);					// 不可屏蔽中断
void HardFault_Handler(void);			// 硬件错误（非法访问/取指等致命异常）
void MemManage_Handler(void);			// 存储器管理错误（MPU 访问违规）
void BusFault_Handler(void);			// 总线错误
void UsageFault_Handler(void);			// 用法错误（未定义指令/非法状态等）
void DebugMon_Handler(void);			// 调试监控异常
void RTC_WKUP_IRQHandler(void);			// RTC 唤醒中断（可用于周期唤醒）
void TIM1_UP_TIM10_IRQHandler(void);	// TIM1 更新 / TIM10 中断
void USART1_IRQHandler(void);			// 串口 1 中断（收发）
void DMA2_Stream2_IRQHandler(void);		// DMA2 流 2 中断
void DMA2_Stream5_IRQHandler(void);		// DMA2 流 5 中断
void DMA2_Stream7_IRQHandler(void);		// DMA2 流 7 中断
/* USER CODE BEGIN EFP */
void EXTI2_IRQHandler(void);			// 外部中断线 2（PA2 充电检测）
void EXTI4_IRQHandler(void);			// 外部中断线 4（PA4 唤醒键）
void EXTI9_5_IRQHandler(void);			// 外部中断线 5~9（触摸/传感器中断）
void EXTI15_10_IRQHandler(void);		// 外部中断线 10~15
/* USER CODE END EFP */

#ifdef __cplusplus
}
#endif

#endif /* __STM32F4xx_IT_H */
