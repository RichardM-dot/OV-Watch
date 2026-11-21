/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   Interrupt Service Routines.
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
 * 【中文导读】stm32f4xx_it.c —— 中断/异常入口总表
 * ----------------------------------------------------------------------------
 * 本工程所有 ISR 都集中在这里，原则是"ISR 只做最短的事"：清标志、发事件标志，
 * 真正的解析、界面刷新交给 FreeRTOS 任务（HardIntEventHandle + osEventFlagsSet）。
 * 分工：RTC_WKUP 是 RTC 唤醒定时器（约 1s 一次）；TIM1_UP 是 HAL 的 1ms 时基
 * （SysTick 让给了 FreeRTOS），同时兼做 KEY1 长按 3 秒关机；USART1 用 IDLE 检测
 * 一帧结束并重启 DMA 收下一帧；DMA2_Stream2/5/7 分别是 SPI1_TX、USART1_RX/TX；
 * EXTI2 充电器插拔、EXTI4 KEY2、EXTI9_5 KEY1、EXTI15_10 MPU6050 中断。
 * 除 TIM1 时基（优先级 15，最低）外，其余外设中断都是 5，正好等于 FreeRTOS 的
 * configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY，所以 ISR 里可调 osEventFlagsSet；
 * 若改成 0~4（数值更小、优先级更高）会触发 FreeRTOS 的优先级断言。
 * 坑：EXTI4/EXTI9_5 只清中断标志，没有实现 HAL_GPIO_EXTI_Callback——按键是任务
 * 里轮询 KeyScan 的，EXTI 在这里只起"把 CPU 从睡眠中叫醒/清 pending"的作用。
 * ============================================================================
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "key.h"
#include "power.h"
#include "user_TasksInit.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern RTC_HandleTypeDef hrtc;
extern DMA_HandleTypeDef hdma_spi1_tx;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;
extern UART_HandleTypeDef huart1;
extern TIM_HandleTypeDef htim1;                  // TIM1 是 HAL 的 1ms 时基（不是 TIM3 那个背光 PWM）

/* USER CODE BEGIN EV */
uint8_t HardInt_receive_str[25];                 // 串口接收缓冲区：DMA 直接往里写，全局零初始化保证初始有 '\0'

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/*
 * 下面这组是内核异常（不是外设中断）：NMI、HardFault、MemManage、BusFault、
 * UsageFault、DebugMon。处理策略统一是"停在 while(1) 里等调试器"：这类错误
 * 通常意味着野指针、栈溢出或非对齐访问，继续跑只会把现场搅乱；原地停住可以
 * 保留 LR/PC 和 SCB->CFSR 供 Keil 查看。量产固件里则依赖看门狗把系统复位。
 */
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
  while (1) // NMI 一般来自时钟安全系统(CSS)或外部强干扰，无法软件恢复
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1) // 硬件错误：查 SCB->CFSR/HFSR 和压栈的 PC，定位到具体指令
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1) // MPU 访问违规（本工程没开 MPU，基本不会进这里）
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1) // 总线错误：常见于访问了未开时钟的外设或非法地址
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1) // 用法错误：除零、非对齐访问、执行了非法指令
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f4xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles RTC wake-up interrupt through EXTI line 22.
  */
/**
  * @brief  RTC 唤醒定时器中断（约 0.98s 一次，见 rtc.c）。
  * @note   为什么只转给 HAL：RTC 的中断标志（WUTF）和 EXTI 线 22 的 pending 位
  *         需要按"读状态→清标志"的固定顺序处理，HAL_RTCEx_WakeUpTimerIRQHandler
  *         已经封装好了；本工程没有实现 EventCallback，所以这里不会通知任务。
  *         如果将来要"定时唤醒刷屏"，应实现 HAL_RTCEx_WakeUpTimerEventCallback，
  *         而不是往这个 ISR 里塞业务代码。
  */
void RTC_WKUP_IRQHandler(void)
{
  /* USER CODE BEGIN RTC_WKUP_IRQn 0 */
  /* USER CODE END RTC_WKUP_IRQn 0 */
  HAL_RTCEx_WakeUpTimerIRQHandler(&hrtc);        // 清 WUTF + EXTI22 pending，必要时回调用户函数
  /* USER CODE BEGIN RTC_WKUP_IRQn 1 */

  /* USER CODE END RTC_WKUP_IRQn 1 */
}

/**
  * @brief This function handles TIM1 update interrupt and TIM10 global interrupt.
  */
/**
  * @brief  TIM1 更新中断：HAL 的 1ms 时基 + KEY1 长按 3 秒关机。
  * @note   为什么长按逻辑放在 ISR：即使调度器被高优先级任务/临界区卡住，长按
  *         关机也必须可靠。KEY1 低电平有效，每 1ms 加一，累计 3000 次即 3 秒，
  *         调用 Power_DisEnable() 拉低 TPS63020 的 EN 脚，整机断电（再按开机键
  *         才能重新上电）——所以这里没有"松手取消"的业务，只有计数清零。
  *         计数变量用 static：函数退出后仍保留，是"ISR 私有状态"的常见写法。
  *         最后必须调 HAL_TIM_IRQHandler，由它清更新标志并触发
  *         HAL_TIM_PeriodElapsedCallback → HAL_IncTick()（见 main.c）。
  */
void TIM1_UP_TIM10_IRQHandler(void)
{
  /* USER CODE BEGIN TIM1_UP_TIM10_IRQn 0 */
  static uint16_t key1_long_press;               // 静态计数：只在本 ISR 内使用，掉电才清零
  if(!KEY1)                                      // KEY1 按下为低电平
  {
    key1_long_press += 1;                        // 1ms 一次，3000 次 = 3 秒
    if(key1_long_press >= 3000)
    {
      Power_DisEnable();                         // 拉低电源使能脚，硬件断电（比软件复位更彻底）
    }
  }
  else
  {
    key1_long_press = 0;                         // 松手立即清零，避免多次短按累计成"长按"
  }
  /* USER CODE END TIM1_UP_TIM10_IRQn 0 */
  HAL_TIM_IRQHandler(&htim1);                    // 清 TIM1 更新标志 → 回调 HAL_IncTick()，uwTick 加 1ms
  /* USER CODE BEGIN TIM1_UP_TIM10_IRQn 1 */

  /* USER CODE END TIM1_UP_TIM10_IRQn 1 */
}

/**
  * @brief This function handles USART1 global interrupt.
  */
/**
  * @brief  USART1 中断：只关心 IDLE（空闲线）标志，用它判断"一帧收完了"。
  * @note   为什么用 IDLE 而不是收满 25 字节：蓝牙命令长度不固定（"OV" 只有 2
  *         字节），DMA 配的是 25 字节 NORMAL 模式，短帧永远等不到传输完成。
  *         IDLE 在总线连续 1 个字符时间为空闲时置位，正好表示"对方发完了"。
  *         处理顺序：先置事件标志通知 MessageSendTask → 清 IDLE（F4 要求
  *         "读 SR 再读 DR"的序列，HAL 宏已封装）→ 停掉本次 DMA → 重新装载
  *         25 字节的接收，准备下一帧。
  *         坑：HardInt_receive_str 不会补 '\0'，且上一帧的残留内容还在，任务里
  *         用 strcmp/printf("%s") 时可能读到旧数据或越界。
  */
void USART1_IRQHandler(void)
{
  /* USER CODE BEGIN USART1_IRQn 0 */
  if(__HAL_UART_GET_FLAG(&huart1,UART_FLAG_IDLE)!=RESET) // 检测到总线空闲 = 一帧结束
  {
    if(HardIntEventHandle != NULL)               // 事件组还没创建时（调度器启动前）不能调 RTOS API
    {
      osEventFlagsSet(HardIntEventHandle, HARDINT_EVENT_UART); // 只置位，不解析：解析交给 MessageSendTask
    }
    __HAL_UART_CLEAR_FLAG(&huart1,UART_FLAG_IDLE); // 清 IDLE：先读 SR 再读 DR，硬件才认账
    HAL_UART_DMAStop(&huart1);                   // 先停 DMA，否则重新装载会与正在进行的搬运冲突
    HAL_UART_Receive_DMA(&huart1, HardInt_receive_str, 25); // 重新武装 25 字节接收，等待下一帧
  }
  /* USER CODE END USART1_IRQn 0 */
  HAL_UART_IRQHandler(&huart1);                  // 处理溢出/帧错误/TC 等其余标志，并调用 HAL 回调
  /* USER CODE BEGIN USART1_IRQn 1 */

  /* USER CODE END USART1_IRQn 1 */
}

/**
  * @brief This function handles DMA2 stream2 global interrupt.
  */
/**
  * @brief  DMA2_Stream2 中断：SPI1 的发送 DMA 完成/半完成/出错。
  * @note   实际业务不在 ISR 里：HAL_DMA_IRQHandler 清标志后会调用
  *         HAL_SPI_TxCpltCallback（lcd.c 中实现），那里等 SPI 的 BSY 位清零，
  *         再通知 LVGL "这块缓冲区可以复用了"。ISR 保持极短，避免拖慢刷屏。
  */
void DMA2_Stream2_IRQHandler(void)
{
  /* USER CODE BEGIN DMA2_Stream2_IRQn 0 */

  /* USER CODE END DMA2_Stream2_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_spi1_tx);             // 判断是 TC/HT/TE 哪种事件，清对应标志并回调
  /* USER CODE BEGIN DMA2_Stream2_IRQn 1 */

  /* USER CODE END DMA2_Stream2_IRQn 1 */
}

/**
  * @brief This function handles DMA2 stream5 global interrupt.
  */
/**
  * @brief  DMA2_Stream5 中断：USART1 接收 DMA 事件。
  * @note   NORMAL 模式下，只有正好收满 25 字节才会进"传输完成"；正常命令都短于
  *         25 字节，所以实际是 IDLE 中断在干活，这条流的中断主要用来兜底
  *         溢出/错误。ISR 里只转给 HAL，不做数据处理。
  */
void DMA2_Stream5_IRQHandler(void)
{
  /* USER CODE BEGIN DMA2_Stream5_IRQn 0 */

  /* USER CODE END DMA2_Stream5_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_usart1_rx);           // 清 RX 流标志，出错时 HAL 会中止接收
  /* USER CODE BEGIN DMA2_Stream5_IRQn 1 */

  /* USER CODE END DMA2_Stream5_IRQn 1 */
}

/**
  * @brief This function handles DMA2 stream7 global interrupt.
  */
/**
  * @brief  DMA2_Stream7 中断：USART1 发送 DMA 事件。
  * @note   每帧最后一个字节移出后置 TC，HAL 在这里回调 HAL_UART_TxCpltCallback。
  *         本工程的 printf 走的是阻塞发送（fputc），这条流给将来"整包 DMA 发送"
  *         预留；保留中断使能可以避免 HAL_DMA_Init 之后状态机卡住。
  */
void DMA2_Stream7_IRQHandler(void)
{
  /* USER CODE BEGIN DMA2_Stream7_IRQn 0 */

  /* USER CODE END DMA2_Stream7_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_usart1_tx);           // 清 TX 流标志并触发发送完成回调
  /* USER CODE BEGIN DMA2_Stream7_IRQn 1 */

  /* USER CODE END DMA2_Stream7_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/**
  * @brief  EXTI2 中断：充电器插入/拔出（PA2，上升沿和下降沿都触发）。
  * @note   为什么先把事件置上再清标志：充电状态变化对用户是"要立刻刷界面"的，
  *         置事件只需几十个周期，放在清 pending 之前可以把延迟压到最小；
  *         清 pending 由 HAL_GPIO_EXTI_IRQHandler 负责（先清 EXTI->PR 再回调），
  *         顺序反了也不会丢事件，因为事件标志是独立的软件对象。
  *         真正的"读 PA2 电平、算电量、改界面"由 user_ChargCheckTask 做。
  *         另外 user_RunModeTasks.c 在软件里也会主动置这个事件（模拟插拔）。
  */
void EXTI2_IRQHandler(void)
{
  if(HardIntEventHandle != NULL)                 // 调度器启动前事件组还不存在，不能调用
  {
    osEventFlagsSet(HardIntEventHandle, HARDINT_EVENT_CHARG); // 只置位，处理交给充电检测任务
  }

  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);          // 清 EXTI2 pending，然后回调（本工程未实现回调，等于只清标志）

}

/**
  * @brief This function handles EXTI line4 interrupt.Key2 interrupt
  */
/**
  * @brief  EXTI4 中断：KEY2（PA4，唤醒键）上升沿。
  * @note   这里只调 HAL_GPIO_EXTI_IRQHandler 清 pending，没有置事件、也没有实现
  *         HAL_GPIO_EXTI_Callback。按键的消抖/短按/长按判断是 user_KeyTask
  *         在任务里轮询 KeyScan() 完成的；EXTI 的作用是让 CPU 从低功耗/空闲
  *         状态立刻醒来，并防止 pending 位一直挂着反复进中断。
  */
void EXTI4_IRQHandler(void)
{

  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);          // 清 PA4 的 EXTI pending；业务在 KeyTask 里轮询

}


/**
  * @brief This function handles EXTI line[9:5] interrupts.Key1 interrupt
  */
/**
  * @brief  EXTI9_5 中断：KEY1（PA5，功能键）下降沿。
  * @note   EXTI5~9 共用一个中断向量，所以函数名是 EXTI9_5；这里只需处理 PIN_5。
  *         与 EXTI4 一样，只清标志不办业务：长按关机在 TIM1 的 1ms 中断里
  *         单独计数（见 TIM1_UP_TIM10_IRQHandler），短按/双击由 KeyTask 轮询。
  */
void EXTI9_5_IRQHandler(void)
{

  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_5);          // 清 PA5 的 EXTI pending

}

/**
  * @brief This function handles EXTI line[15:10] interrupts.MPU Interrupt
  */
/**
  * @brief  EXTI15_10 中断：MPU6050 的 INT 引脚（PB12，下降沿有效），抬腕/计步中断。
  * @note   MPU6050 的 DMP 检测到动作后把 INT 拉低，这里置 HARDINT_EVENT_MPU 通知
  *         传感器任务去读数据；ISR 里绝不碰 I2C（读 MPU 要走几百微秒，会拖垮
  *         中断响应）。同样用"先置事件、后清 pending"的顺序。
  *         EXTI10~15 共用一个向量，所以只处理 PIN_12 这一路。
  */
void EXTI15_10_IRQHandler(void)
{
  if(HardIntEventHandle != NULL)                 // 事件组未创建时跳过，避免空指针
  {
    osEventFlagsSet(HardIntEventHandle, HARDINT_EVENT_MPU); // 唤醒传感器任务，由其读取 MPU 数据
  }
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_12);         // 清 PB12 的 EXTI pending

}

/* USER CODE END 1 */
