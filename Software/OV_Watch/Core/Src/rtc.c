/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    rtc.c
  * @brief   This file provides code for the configuration
  *          of the RTC instances.
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
 * 文件名称：rtc.c
 * 功能描述：RTC 实时时钟初始化与时间/日期设置
 * ----------------------------------------------------------------------------
 * RTC 时钟源选择"外部低速晶振"LSE，频率 32.768kHz。
 * 异步预分频 127+1，得到 256Hz；同步预分频 255+1，得到 1Hz 的日历时钟。
 * BKP_DR0 写入 0x5050 作为"已初始化"标志，防止 VBAT 掉电后重新初始化。
 * 首次上电或 VBAT 掉电后，重新设置时间/日期并写入标志；之后上电跳过。
 * 备份域由 VBAT 供电，主电源掉电后 RTC 继续走时。
 * 初始时间设置为 2023-05-21 11:59:55。
 * 唤醒定时器时钟为 RTCCLK/16 = 2048Hz，计数值 2000，约 0.98s 触发 RTC_WKUP 中断。
 * 注意：main.c 中 MX_RTC_Init() 需在 osKernelStart() 之前调用，
 *       或在 HardwareInitTask 中调用。
 * 注意：RTC_SetTime/SetDate 使用 BIN 格式，HAL 内部自动转换为 BCD 存储。
 *       若使用 BCD 格式，0x11 表示 11，0x17 表示 17。
 *       读取时间请使用 HAL_RTC_GetTime 和 HAL_RTC_GetDate 获取原始值。
 * ============================================================================
 */
/* Includes ------------------------------------------------------------------*/
#include "rtc.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

RTC_HandleTypeDef hrtc;

/* RTC init function */
/**
  * @brief  初始化 RTC 时间和日期，并设置唤醒定时器。
  * @note   HAL_RTC_Init() 只负责 RTC 初始化。
  *         使用 BKP_DR0 = 0x5050 作为标志：首次上电时初始化，之后跳过。
  *         需在 main.c 的 osKernelStart() 之前调用。
  * @retval None
  */
void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;                          // 选择 RTC 外设
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;     // 24 小时制，范围 0~23，不使用 AM/PM
  hrtc.Init.AsynchPrediv = 127;                 // 异步预分频 7 位：LSE 32768Hz / (127+1) = 256Hz
  hrtc.Init.SynchPrediv = 255;                  // 同步预分频 15 位：256Hz / (255+1) = 1Hz，即 1 秒
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;        // 禁用 RTC 输出，PC13 引脚不被占用
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH; // 输出极性高（本工程未启用输出，仅作配置）
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;    // 开漏输出，适合 1.8V/3.3V 电平
  if (HAL_RTC_Init(&hrtc) != HAL_OK)            // 调用 HAL_RTC_MspInit 完成 LSE、RTC 时钟和 NVIC 配置
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */
	// BKP_DR0 未被写入 0x5050 时，说明是首次上电或 VBAT 掉电，需要重新初始化 RTC 时间和日期；
	// 否则跳过初始化，保持 RTC 继续走时。
	// 若希望每次上电都重新设置时间，可去掉此标志的检查与写入。
	if(HAL_RTCEx_BKUPRead(&hrtc,RTC_BKP_DR0)!=0X5050)//判断是否首次上电
  {
  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  // 注意：RTC 内部以 BCD 格式存储。0x11 表示"11"，0x17 表示"17"。
  // 这里使用 RTC_FORMAT_BCD，因此直接写 BCD 值；
  // 若使用 RTC_FORMAT_BIN，则写二进制值，HAL 会自动转换为 BCD。
  sTime.Hours = 0x11;                           // 11 点（BCD 格式）
  sTime.Minutes = 0x59;                         // 59 分（BCD 格式）
  sTime.Seconds = 0x55;                         // 55 秒（BCD 格式）
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE; // 不使用夏令时
  sTime.StoreOperation = RTC_STOREOPERATION_RESET; // 存储操作复位
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_SUNDAY;           // 2023-05-21 是星期日，HAL 中星期日对应 7
  sDate.Month = RTC_MONTH_MAY;                  // 5 月（BCD 0x05）
  sDate.Date = 0x21;                            // 21 日（BCD 0x21）
  sDate.Year = 0x23;                            // 2023 年，RTC 只存两位年份，即 23

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable the WakeUp
  */
  // 唤醒定时器时钟：RTCCLK(32768)/16 = 2048Hz，计数 2000，约 0.98s 触发一次。
  // RTC_WKUP 中断通过 EXTI22 连接到 NVIC，在 stm32f4xx_it.c 中调用 HAL 处理。
  // 用户可在 HAL_RTCEx_WakeUpTimerEventCallback 回调中处理"唤醒事件"。
  // 中断使能建议放在 HardwareInitTask 中，避免在系统初始化阶段触发。
  if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 2000, RTC_WAKEUPCLOCK_RTCCLK_DIV16) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */
	HAL_RTCEx_BKUPWrite(&hrtc,RTC_BKP_DR0,0X5050);//写入标志，防止下次上电重新初始化
	// 注意：如果希望每次上电都重新设置时间，可去掉此处写入和上面的检查。
  }
  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief  RTC 底层初始化（MSP = MCU Support Package），由 HAL_RTC_Init 调用。
  * @note   配置 RTC 时钟源、使能 RTC 时钟和 NVIC 中断。
  *         RTC 时钟源选择 LSE，LSE 在 SystemClock_Config 中已配置。
  *         LSE 精度高，适合 RTC 走时。
  * @param  rtcHandle: RTC 句柄指针
  * @retval None
  */
void HAL_RTC_MspInit(RTC_HandleTypeDef* rtcHandle)
{

  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspInit 0 */

  /* USER CODE END RTC_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;   // 选择 RTC 外设时钟
    PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;   // RTC 时钟源选 LSE（32.768kHz）
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)  // 配置 LSE 作为 RTC 时钟
    {
      Error_Handler();
    }

    /* RTC clock enable */
    __HAL_RCC_RTC_ENABLE();                    // 使能 RTC 时钟

    /* RTC interrupt Init */
    HAL_NVIC_SetPriority(RTC_WKUP_IRQn, 5, 0); // 优先级 5，低于 FreeRTOS 的 configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY，ISR 中可调用 RTOS API
    HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);         // 使能 EXTI22（RTC 唤醒）中断
  /* USER CODE BEGIN RTC_MspInit 1 */

  /* USER CODE END RTC_MspInit 1 */
  }
}

/**
  * @brief  RTC 底层反初始化，恢复 RTC 时钟和中断配置。
  * @note   仅在需要关闭 RTC 时调用；CubeMX 默认不生成调用。
  * @param  rtcHandle: RTC 句柄指针
  * @retval None
  */
void HAL_RTC_MspDeInit(RTC_HandleTypeDef* rtcHandle)
{

  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspDeInit 0 */

  /* USER CODE END RTC_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_RTC_DISABLE();                   // 禁用 RTC 时钟

    /* RTC interrupt Deinit */
    HAL_NVIC_DisableIRQ(RTC_WKUP_IRQn);        // 禁用 RTC 唤醒中断
  /* USER CODE BEGIN RTC_MspDeInit 1 */

  /* USER CODE END RTC_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
/**
  * @brief  计算星期几，返回 1~7，7 表示星期日。
  * @param  y: 年份后两位（23 表示 2023）
  * @param  m: 月份 1~12
  * @param  d: 日期 1~31
  * @param  c: 世纪（20 表示 20xx）
  * @note   1、2 月按上一年的 13、14 月计算（y--，m+=12）。
  *         返回值 1=周一 ... 7=周日，与 HAL 的 RTC_WEEKDAY_SUNDAY=7 对应。
  * @retval 星期几（1~7）
  */
uint8_t weekday_calculate(int y,int m,int d,int c)
{
	int w;
	if (m == 1 || m == 2)
	{y--, m += 12;}
	w = y + y / 4 + c / 4  + 26*(m + 1)/10 + d - 1 - 2 * c;
	while(w<0)
		w+=7;
	w%=7;
	w=(w==0)?7:w;
	return w;
}

/**
  * @brief  设置 RTC 时间（时/分/秒），使用 BIN 格式。
  * @param  hours: 小时 0~23（二进制值）
  * @param  minutes: 分钟 0~59
  * @param  seconds: 秒 0~59
  * @note   用 {0} 初始化结构体，未设置的字段为 0。
  *         HAL_RTC_SetTime 使用 RTC_FORMAT_BIN，HAL 内部自动转换为 BCD 存储。
  * @retval None
  */
void RTC_SetTime(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
	RTC_TimeTypeDef time={0};//用 {0} 初始化结构体，未设置字段为 0
	time.Hours=hours;
	time.Minutes=minutes;
	time.Seconds=seconds;
	if (HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  设置 RTC 日期（年/月/日），使用 BIN 格式。
  * @param  year: 年份后两位（23 表示 2023）
  * @param  month: 月份 1~12
  * @param  date: 日期 1~31
  * @note   内部调用 weekday_calculate 计算星期。
  *         年份只支持 2000~2099（传入年份的后两位）。
  *         用 {0} 初始化结构体，WeekDay 由计算得出。
  * @retval None
  */
void RTC_SetDate(uint8_t year, uint8_t month, uint8_t date)
{
	RTC_DateTypeDef setdate={0};//用 {0} 初始化结构体，未设置字段为 0
	setdate.Date=date;
	setdate.Month=month;
	setdate.Year=year;

	//计算星期几
	setdate.WeekDay = weekday_calculate(year,month,date,20); // 世纪参数 20，表示 20xx 年

	if (HAL_RTC_SetDate(&hrtc, &setdate, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
}
/* USER CODE END 1 */