/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    tim.c
  * @brief   This file provides code for the configuration
  *          of the TIM instances.
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
 * 【中文导读】tim.c —— TIM3 输出 PWM，调 LCD 背光
 * ----------------------------------------------------------------------------
 * TIM3 的 CH3 从 PB0 输出 PWM，接到 ST7789 屏的背光使能脚（LEDA/BL），
 * 用占空比调亮度——比"开/关"两档省电，也避免低亮度下屏闪。
 * 时基：TIM3 挂在 APB1（本工程 50MHz），但 APB1 预分频≠1 时定时器时钟会自动
 * 倍频到 100MHz；PSC=99 得 1MHz 计数，ARR=300 得 301 个计数一周期，即约
 * 3.32kHz——高于人眼闪烁感知，也不会让背光驱动电路叫。
 * 本函数只"配置"不"启动"：HAL_TIM_PWM_Start 在 HardwareInitTask 里调用，
 * 初始 Pulse=0 意味着上电瞬间背光是灭的，等系统就绪再点亮。
 * 亮度接口是 lcd_init.c 的 LCD_Set_Light(dc)：dc 取 5~100，内部换算成
 * dc*PWM_PERIOD/100 的比较值（PWM_PERIOD=300）。
 * 坑：改 PSC/ARR 会同时改变亮度-占空比映射，LCD_Set_Light 里的百分比换算要跟着改。
 * ============================================================================
 */
/* Includes ------------------------------------------------------------------*/
#include "tim.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

TIM_HandleTypeDef htim3;

/* TIM3 init function */
/**
  * @brief  配置 TIM3 为 PWM 输出模式，通道 3 驱动 LCD 背光。
  * @note   只配置、不启动。之所以把"启动"留到 HardwareInitTask，是因为 LCD 的
  *         GPIO/初始化要在背光之前完成，否则会先亮出一片雪花。
  * @retval None
  */
void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;                        // 绑定 TIM3（APB1 外设，定时器时钟 100MHz）
  htim3.Init.Prescaler = 99;                    // 100MHz/(99+1) = 1MHz 计数频率，1 个计数 = 1us
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;  // 向上计数：从 0 数到 ARR 产生溢出
  htim3.Init.Period = 300;                      // ARR=300 → 每 301us 一个周期，PWM 频率约 3.32kHz
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1; // 死区/滤波采样时钟不分频；PWM 输出用不到
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE; // 关闭 ARR 预装载：改 ARR 立即生效，本项目运行期不改
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)       // 内部回调 HAL_TIM_PWM_MspInit：只负责开 TIM3 时钟
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;          // 不向其它定时器输出触发信号（TRGO 复位）
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE; // 不做主从级联：TIM3 独立跑
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;           // PWM 模式1：计数值 < CCR 时输出有效电平，占空比=CCR/(ARR+1)
  sConfigOC.Pulse = 0;                          // 初始比较值 0 → 占空比 0%，上电时背光全灭
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;   // 有效电平为高：CCR 越大越亮，符合直觉
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;    // 关闭快速模式：PWM 不需要在强制无效时立刻响应
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);                  // 单独配 PB0 为 TIM3_CH3 复用输出（CubeMX 把 GPIO 放在 PostInit）

}

/**
  * @brief  TIM3 的 PWM 底层初始化：只需要打开 TIM3 外设时钟。
  * @note   定时器不像 GPIO 需要配引脚模式，所以这里只有时钟；引脚在
  *         HAL_TIM_MspPostInit 里配置（CubeMX 的固定分工）。
  * @param  tim_pwmHandle: 定时器句柄指针
  * @retval None
  */
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef* tim_pwmHandle)
{

  if(tim_pwmHandle->Instance==TIM3)
  {
  /* USER CODE BEGIN TIM3_MspInit 0 */

  /* USER CODE END TIM3_MspInit 0 */
    /* TIM3 clock enable */
    __HAL_RCC_TIM3_CLK_ENABLE();                 // 使能 TIM3 时钟后，PSC/ARR/CCR 才能写入并开始计数
  /* USER CODE BEGIN TIM3_MspInit 1 */

  /* USER CODE END TIM3_MspInit 1 */
  }
}
/**
  * @brief  TIM3 通道 3 的引脚复用配置（PB0 → TIM3_CH3）。
  * @note   单独放在 PostInit 是因为定时器初始化中途才确定用哪个通道，CubeMX
  *         把"补引脚复用"这一步推迟到最后，避免通道配置失败时引脚已经配好。
  *         速度选 LOW 就够了：PWM 只有 3.32kHz，不需要陡边沿。
  * @param  timHandle: 定时器句柄指针
  * @retval None
  */
void HAL_TIM_MspPostInit(TIM_HandleTypeDef* timHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(timHandle->Instance==TIM3)
  {
  /* USER CODE BEGIN TIM3_MspPostInit 0 */

  /* USER CODE END TIM3_MspPostInit 0 */

    __HAL_RCC_GPIOB_CLK_ENABLE();                // PB 口时钟：PB0 要当复用脚
    /**TIM3 GPIO Configuration
    PB0     ------> TIM3_CH3
    */
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;      // 复用推挽：由定时器外设直接驱动背光脚
    GPIO_InitStruct.Pull = GPIO_NOPULL;          // 背光驱动端自带下拉/驱动，无需内部上下拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; // 3.32kHz 的方波，低速档足够，还能减小 EMI
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;   // AF2 是 TIM3 的固定复用编号
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN TIM3_MspPostInit 1 */

  /* USER CODE END TIM3_MspPostInit 1 */
  }

}

/**
  * @brief  TIM3 反初始化：关外设时钟。
  * @note   运行期不调用；关时钟后 PB0 会停在当前电平，背光状态由外部电路决定。
  * @param  tim_pwmHandle: 定时器句柄指针
  * @retval None
  */
void HAL_TIM_PWM_MspDeInit(TIM_HandleTypeDef* tim_pwmHandle)
{

  if(tim_pwmHandle->Instance==TIM3)
  {
  /* USER CODE BEGIN TIM3_MspDeInit 0 */

  /* USER CODE END TIM3_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_TIM3_CLK_DISABLE();
  /* USER CODE BEGIN TIM3_MspDeInit 1 */

  /* USER CODE END TIM3_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
