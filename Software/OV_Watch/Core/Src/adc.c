/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    adc.c
  * @brief   This file provides code for the configuration
  *          of the ADC instances.
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
 * 【中文导读】adc.c —— ADC1 采集电池电压
 * ----------------------------------------------------------------------------
 * 电池电压经电阻分压（1/2）后接到 PA1（ADC1_IN1），ADC 读原始码值，
 * power.c 里按 "dat * 2 * 3.3 / 4096" 还原成电池电压，再查表换算电量百分比。
 * 配置要点：12 位分辨率、右对齐、单通道单次转换、软件触发、不扫描不连续，
 * 这样每次 HAL_ADC_Start 只转换一次，适合"要用才测"的低功耗场景。
 * ADC 时钟 = PCLK2(100MHz)/4 = 25MHz，低于 F4 的 36MHz 上限。
 * 采样时间只有 3 个周期，是"最快但最挑信号源"的档位；分压电阻较大的话，
 * 采样电容来不及充到位，读数会偏低——所以 power.c 连测 8 次取平均来压噪声。
 * 坑：本工程没有调用 HAL_ADCEx_Calibration_Start 做校准，绝对精度依赖出厂值；
 * 分压后的电压必须 < 3.3V（VDDA），否则会打坏输入保护二极管。
 * ============================================================================
 */
/* Includes ------------------------------------------------------------------*/
#include "adc.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

ADC_HandleTypeDef hadc1;

/* ADC1 init function */
/**
  * @brief  配置 ADC1 的全局参数与通道 1（PA1），用于电池电压采样。
  * @note   整套配置刻意保持"最简"：单通道、单次、软件触发。这样调用方
  *         （power.c 的 BatCheck）可以在需要时 Start→Poll→GetValue→Stop，
  *         不占 DMA、不占中断。
  * @retval None
  */
void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;                        // 绑定 ADC1（APB2 外设）
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4; // 100MHz/4 = 25MHz，低于 36MHz 上限，兼顾速度与精度
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;   // 12 位：0~4095，配合 3.3V 参考约 0.8mV/LSB
  hadc1.Init.ScanConvMode = DISABLE;            // 不扫描：只测一个通道，省得配序列
  hadc1.Init.ContinuousConvMode = DISABLE;      // 不连续转换：测一次就停，符合"按需测电量"
  hadc1.Init.DiscontinuousConvMode = DISABLE;   // 不用间断模式，单通道场景没有意义
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE; // 无外部触发边沿
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START; // 由 HAL_ADC_Start 写 SWSTART 位启动
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;   // 右对齐：读 DR 直接就是 0~4095，方便换算
  hadc1.Init.NbrOfConversion = 1;               // 规则组只有 1 个转换
  hadc1.Init.DMAContinuousRequests = DISABLE;   // 不用 DMA，也就没有"连续请求"一说
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV; // 单次转换结束就置 EOC，PollForConversion 靠它返回
  if (HAL_ADC_Init(&hadc1) != HAL_OK)           // 内部回调 HAL_ADC_MspInit：开 ADC 时钟、配 PA1 为模拟输入
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_1;              // 通道 1 对应 PA1，电池分压后的电压就接在这
  sConfig.Rank = 1;                             // 规则序列第 1 个（也是唯一一个）
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES; // 3 周期采样：最快档，要求信号源阻抗足够低
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief  ADC1 的 MSP 底层初始化：开 ADC 时钟、把 PA1 配成模拟输入。
  * @note   PA1 必须配成 GPIO_MODE_ANALOG：这样引脚的数字输入缓冲和上下拉都会被
  *         断开，避免漏电流和分压电阻被内部电阻影响。ADC 引脚不需要配 Speed
  *         和 Alternate，因为模拟通道不经过复用器。
  * @param  adcHandle: ADC 句柄指针
  * @retval None
  */
void HAL_ADC_MspInit(ADC_HandleTypeDef* adcHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(adcHandle->Instance==ADC1)
  {
  /* USER CODE BEGIN ADC1_MspInit 0 */

  /* USER CODE END ADC1_MspInit 0 */
    /* ADC1 clock enable */
    __HAL_RCC_ADC1_CLK_ENABLE();                 // 开 ADC1 时钟；F4 的 ADC 时钟来自 APB2 经 ADC 预分频

    __HAL_RCC_GPIOA_CLK_ENABLE();                // PA 口时钟：PA1 要当模拟输入
    /**ADC1 GPIO Configuration
    PA1     ------> ADC1_IN1
    */
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;     // 模拟模式：断开数字输入缓冲，读数才准
    GPIO_InitStruct.Pull = GPIO_NOPULL;          // 模拟通道上下拉无效，写 NOPULL 只是明确意图
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN ADC1_MspInit 1 */

  /* USER CODE END ADC1_MspInit 1 */
  }
}

/**
  * @brief  ADC1 反初始化：关时钟、把 PA1 还原为默认态。
  * @note   运行期不调用；关掉 ADC 时钟后之前配置的通道序列仍然保留在寄存器里，
  *         重新开时钟即可继续用。
  * @param  adcHandle: ADC 句柄指针
  * @retval None
  */
void HAL_ADC_MspDeInit(ADC_HandleTypeDef* adcHandle)
{

  if(adcHandle->Instance==ADC1)
  {
  /* USER CODE BEGIN ADC1_MspDeInit 0 */

  /* USER CODE END ADC1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_ADC1_CLK_DISABLE();

    /**ADC1 GPIO Configuration
    PA1     ------> ADC1_IN1
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_1);

  /* USER CODE BEGIN ADC1_MspDeInit 1 */

  /* USER CODE END ADC1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
