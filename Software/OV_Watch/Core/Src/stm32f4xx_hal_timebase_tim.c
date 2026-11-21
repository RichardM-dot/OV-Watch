/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f4xx_hal_timebase_TIM.c
  * @brief   HAL time base based on the hardware TIM.
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
 * 【中文导读】stm32f4xx_hal_timebase_tim.c —— 用 TIM1 给 HAL 提供 1ms 时基
 * ----------------------------------------------------------------------------
 * 为什么不用 SysTick：SysTick 被 FreeRTOS 独占做调度节拍（configTICK_RATE_HZ=
 * 1000）。CubeMX 里把 Timebase Source 改成 TIM1 后，HAL 的 uwTick 就改由 TIM1
 * 的更新中断来累加，两套时基互不干扰——这是"RTOS + HAL 混用"的标准做法。
 * 时基计算：TIM1 挂在 APB2（本工程 100MHz），PSC = 1000000-1 让计数器跑 1MHz，
 * ARR = 1000-1 让它每 1000 个计数溢出一次，正好 1ms。
 * 关键点：HAL_Init() 和每次 HAL_RCC_ClockConfig() 都会回调 HAL_InitTick()，
 * 因为主频变了，PSC 必须按新的 PCLK2 重新算——所以这里不能用写死的分频值。
 * 中断优先级取 TickPriority（本工程 TICK_INT_PRIORITY=15，最低），保证 HAL 的
 * 超时计时不会抢占任何业务中断。
 * 注意：htim1 是全局变量，stm32f4xx_it.c 里的 TIM1_UP_TIM10_IRQHandler 会用到它；
 * 该 ISR 还兼做 KEY1 长按关机，改 TIM1 的配置会同时影响 HAL_Delay 和长按判定。
 * ============================================================================
 */

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_tim.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef        htim1;
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  重新配置 TIM1 为 1ms 时基（HAL_Init 与每次时钟变更时都会调用）。
  * @note   为什么分频必须动态算：HAL_Init 时主频还是 HSI 16MHz，
  *         SystemClock_Config 之后才是 100MHz；同一个函数会被调用两次，
  *         写死分频必然错一次，算出来的 HAL_Delay 也会跟着错。
  * @param  TickPriority: 时基中断优先级（本工程 TICK_INT_PRIORITY=15）
  * @retval HAL_OK 成功；HAL_ERROR 优先级非法或启动失败
  */
/**
  * @brief  This function configures the TIM1 as a time base source.
  *         The time source is configured  to have 1ms time base with a dedicated
  *         Tick interrupt priority.
  * @note   This function is called  automatically at the beginning of program after
  *         reset by HAL_Init() or at any time when clock is configured, by HAL_RCC_ClockConfig().
  * @param  TickPriority: Tick interrupt priority.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
  RCC_ClkInitTypeDef    clkconfig;
  uint32_t              uwTimclock = 0U;

  uint32_t              uwPrescalerValue = 0U;
  uint32_t              pFLatency;
  HAL_StatusTypeDef     status;

  /* Enable TIM1 clock */
  __HAL_RCC_TIM1_CLK_ENABLE();                   // 时基外设自己的时钟必须先开

  /* Get clock configuration */
  HAL_RCC_GetClockConfig(&clkconfig, &pFLatency); // 读出当前 AHB/APB 分频，下面算 TIM1 时钟要用

  /* Compute TIM1 clock */
  uwTimclock = HAL_RCC_GetPCLK2Freq();           // TIM1 在 APB2；APB2 分频≠1 时定时器时钟会自动×2（HAL 已处理）

  /* Compute the prescaler value to have TIM1 counter clock equal to 1MHz */
  uwPrescalerValue = (uint32_t) ((uwTimclock / 1000000U) - 1U); // 100MHz → 99，计数频率 1MHz，1 个数 = 1us

  /* Initialize TIM1 */
  htim1.Instance = TIM1;

  /* Initialize TIMx peripheral as follow:
  + Period = [(TIM1CLK/1000) - 1]. to have a (1/1000) s time base.
  + Prescaler = (uwTimclock/1000000 - 1) to have a 1MHz counter clock.
  + ClockDivision = 0
  + Counter direction = Up
  */
  htim1.Init.Period = (1000000U / 1000U) - 1U;   // 1MHz 下数 999 次 = 1ms，也就是一次 HAL tick
  htim1.Init.Prescaler = uwPrescalerValue;       // 动态计算：HAL_Init 时主频还是 16MHz，时钟配置后会被重新调用
  htim1.Init.ClockDivision = 0;                  // 不用死区/滤波分频
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;   // 向上计数到 ARR 产生更新事件
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE; // 关预装载，改 ARR 立即生效

  status = HAL_TIM_Base_Init(&htim1);            // 只做时基，不用 PWM/输入捕获，所以是 Base_Init
  if (status == HAL_OK)
  {
    /* Start the TIM time Base generation in interrupt mode */
    status = HAL_TIM_Base_Start_IT(&htim1);      // 启动计数并使能更新中断（每 1ms 进一次 TIM1_UP）
    if (status == HAL_OK)
    {
    /* Enable the TIM1 global Interrupt */
        HAL_NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);  // 打开 NVIC 里的 TIM1 更新中断线
      /* Configure the SysTick IRQ priority */
      if (TickPriority < (1UL << __NVIC_PRIO_BITS)) // 优先级必须能塞进 4 位（0~15），否则返回错误
      {
        /* Configure the TIM IRQ priority */
        HAL_NVIC_SetPriority(TIM1_UP_TIM10_IRQn, TickPriority, 0U); // 用传入的 TickPriority（本工程 15）
        uwTickPrio = TickPriority;               // 记下来，HAL_Delay 判断"能否在中断里调用"时要用
      }
      else
      {
        status = HAL_ERROR;
      }
    }
  }

 /* Return function status */
  return status;
}

/**
  * @brief  暂停 HAL 时基（关 TIM1 更新中断，uwTick 不再增加）。
  * @note   典型用途是进入低功耗前暂停计数，避免醒来后补偿一大堆 tick；
  *         本工程运行期没有调用。
  * @param  None
  * @retval None
  */
/**
  * @brief  Suspend Tick increment.
  * @note   Disable the tick increment by disabling TIM1 update interrupt.
  * @param  None
  * @retval None
  */
void HAL_SuspendTick(void)
{
  /* Disable TIM1 update Interrupt */
  __HAL_TIM_DISABLE_IT(&htim1, TIM_IT_UPDATE);   // 只关中断源，定时器本身还在数
}

/**
  * @brief  恢复 HAL 时基（重新使能 TIM1 更新中断）。
  * @note   与 HAL_SuspendTick 配对使用；恢复后 uwTick 从当前值继续累加，
  *         不会把暂停期间的时间补回来。
  * @param  None
  * @retval None
  */
/**
  * @brief  Resume Tick increment.
  * @note   Enable the tick increment by Enabling TIM1 update interrupt.
  * @param  None
  * @retval None
  */
void HAL_ResumeTick(void)
{
  /* Enable TIM1 Update interrupt */
  __HAL_TIM_ENABLE_IT(&htim1, TIM_IT_UPDATE);    // 重新允许更新中断，HAL_IncTick 继续被调用
}

