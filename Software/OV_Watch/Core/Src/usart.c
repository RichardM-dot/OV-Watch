/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
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
 * 【中文导读】usart.c —— 串口1（接 KT6328 蓝牙模块）
 * ----------------------------------------------------------------------------
 * USART1 用 PA9(TX)/PA10(RX)，115200-8-N-1、无硬件流控，是手表和手机 App 之间
 * 唯一的数据通道：手机发 "OV"、"OV+SEND" 等 ASCII 命令，手表用 printf 回数据。
 * 收发都走 DMA：RX 用 DMA2_Stream5/通道4（外设→内存），TX 用 DMA2_Stream7/通道4
 * （内存→外设），CPU 不用一个字节一个字节地等。NVIC 优先级 5，正好等于 FreeRTOS
 * 的 configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY，所以 ISR 里可以调 osEventFlagsSet。
 * 收包策略是"定长 25 字节 + IDLE 判帧尾"：一帧结束置 HARDINT_EVENT_UART 事件，
 * 由 MessageSendTask 解析；DMA 配成 NORMAL 模式，每次收完必须手动重启才能收下一帧。
 * 坑：DMA 不会自动补 '\0'，正好 25 字节的帧会让 printf("%s") 读越界；printf 底层
 * 是阻塞发送（超时 0xffff），任务里大量打印会拖慢界面刷新。
 * ============================================================================
 */
/* Includes ------------------------------------------------------------------*/
#include "usart.h"

/* USER CODE BEGIN 0 */
#include "stdio.h"
/* USER CODE END 0 */

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;

/* USART1 init function */

/**
  * @brief  配置 USART1 的帧格式与波特率，并调用 HAL_UART_Init 完成初始化。
  * @note   波特率 115200 是 KT6328 蓝牙模块的出厂默认值（模块内部默认 115200，
  *         注释里提到的 AT+CT01 才是改成 9600 的命令）。帧格式 8-N-1 是串口
  *         最通用的组合；OverSampling=16 是 F4 的默认过采样方式，比 8 倍采样
  *         抗噪更好。本函数只配置，不启动收发——真正的 HAL_UART_Receive_DMA
  *         在 HardwareInitTask 里调用。
  * @retval None
  */
void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;                     // 绑定 USART1 寄存器组
  huart1.Init.BaudRate = 115200;                // 波特率 115200：双方必须一致，APB2=100MHz 下误差很小
  huart1.Init.WordLength = UART_WORDLENGTH_8B;  // 8 位数据位，ASCII 命令足够
  huart1.Init.StopBits = UART_STOPBITS_1;       // 1 位停止位
  huart1.Init.Parity = UART_PARITY_NONE;        // 无校验：蓝牙模块透传一般不校验，靠上层协议保证
  huart1.Init.Mode = UART_MODE_TX_RX;           // 收发都要，命令是双向的
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;  // 不用 RTS/CTS，模块只引出了 TX/RX 两根线
  huart1.Init.OverSampling = UART_OVERSAMPLING_16; // 16 倍过采样，波特率误差容忍度比 8 倍高
  if (HAL_UART_Init(&huart1) != HAL_OK)         // 内部回调 HAL_UART_MspInit：开时钟、配引脚、配 DMA、配 NVIC
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief  USART1 的 MSP 底层初始化：时钟、GPIO 复用、DMA 通道、NVIC。
  * @note   USART1 挂在 APB2 上（本工程 100MHz），所以引脚要配成"复用推挽 +
  *         最高速度"，否则 115200 波形边沿会变缓；复用号 AF7 是 F4 固定的
  *         USART1 映射，填错就收不到数据。
  * @param  uartHandle: UART 句柄指针
  * @retval None
  */
void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */
    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();               // 先开外设时钟，否则后面写 USART 寄存器全部无效

    __HAL_RCC_GPIOA_CLK_ENABLE();                // PA 口时钟：PA9/PA10 要当复用脚用
    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10; // TX/RX 一起配，省一次 HAL_GPIO_Init
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;      // 复用推挽：TX 要主动驱动，RX 由外设接管
    GPIO_InitStruct.Pull = GPIO_NOPULL;          // 模块端已有上拉/驱动，这里不加内部上下拉以免分压
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH; // 高速档，保证 115200 的上升沿足够陡
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1; // AF7 是 USART1~3 的固定复用编号
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART1 DMA Init */
    /* USART1_RX Init */
    // 接收：外设（DR 寄存器）→ 内存（HardInt_receive_str），所以是 PERIPH_TO_MEMORY。
    // 外设地址固定不递增、内存地址递增，两边都是 1 字节；NORMAL 模式表示收满
    // 25 个字节（或手动停止）就结束，需要重新 HAL_UART_Receive_DMA 才能继续。
    hdma_usart1_rx.Instance = DMA2_Stream5;      // USART1_RX 在 F4 上固定映射到 DMA2 Stream5 通道4
    hdma_usart1_rx.Init.Channel = DMA_CHANNEL_4;
    hdma_usart1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart1_rx.Init.PeriphInc = DMA_PINC_DISABLE;   // 外设侧永远是 USART1->DR 同一个地址
    hdma_usart1_rx.Init.MemInc = DMA_MINC_ENABLE;       // 内存侧每个字节往后挪一格，才能拼成一帧
    hdma_usart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE; // 串口一次 8 位
    hdma_usart1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;    // 内存按字节对齐，避免半字搬运错位
    hdma_usart1_rx.Init.Mode = DMA_NORMAL;              // 单次模式：收满就停，由 IDLE 中断负责重启
    hdma_usart1_rx.Init.Priority = DMA_PRIORITY_LOW;    // 串口数据量小，优先级让给 LCD 刷屏
    hdma_usart1_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE; // 关闭 FIFO：字节流用直接模式最简单，也省延迟
    if (HAL_DMA_Init(&hdma_usart1_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmarx,hdma_usart1_rx); // 把 DMA 句柄挂到 UART 句柄上，HAL_UART_Receive_DMA 才知道用哪条流

    /* USART1_TX Init */
    // 发送方向反过来：内存 → 外设（DR）。其余参数与 RX 对称。
    hdma_usart1_tx.Instance = DMA2_Stream7;      // USART1_TX 固定映射到 DMA2 Stream7 通道4
    hdma_usart1_tx.Init.Channel = DMA_CHANNEL_4;
    hdma_usart1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart1_tx.Init.MemInc = DMA_MINC_ENABLE;         // 连续发送时内存地址要递增
    hdma_usart1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart1_tx.Init.Mode = DMA_NORMAL;
    hdma_usart1_tx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_usart1_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart1_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmatx,hdma_usart1_tx);

    /* USART1 interrupt Init */
    // 优先级 5 = FreeRTOS 的 configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY（数值越小越
    // 高）。取"等于"而不是更低，是为了让 USART1 的 ISR 能合法调用 osEventFlagsSet；
    // 若改成 0~4，中断会屏蔽内核临界区，从 ISR 调 RTOS API 会触发断言。
    HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspInit 1 */

  /* USER CODE END USART1_MspInit 1 */
  }
}

/**
  * @brief  USART1 反初始化：关时钟、还原引脚、释放 DMA、关中断。
  * @note   运行期不会调用，属于 CubeMX 模板；注意它没有恢复引脚为默认态，
  *         只是把 GPIO 复用配置清掉（HAL_GPIO_DeInit）。
  * @param  uartHandle: UART 句柄指针
  * @retval None
  */
void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);

    /* USART1 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmarx);           // 通过句柄里的指针找到 RX/TX 两条流，逐条复位
    HAL_DMA_DeInit(uartHandle->hdmatx);

    /* USART1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspDeInit 1 */

  /* USER CODE END USART1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
/**
  * @brief  printf 重定向：把标准库 putchar 的出口接到 USART1。
  * @param  ch: 要发送的字符
  * @param  f: 文件指针（标准库传入，这里不用）
  * @note   Keil 的 microlib / 标准库在 printf 时逐个字符调用 fputc，所以整个
  *         工程可以直接 printf 调试和回蓝牙数据。注意 HAL_UART_Transmit 是
  *         阻塞发送，超时 0xffff 个 HAL tick（约 65s），在任务里频繁打印会
  *         占着 CPU；更不能在中断里调用。
  * @retval 原样返回 ch，表示"这个字符已处理"
  */
int fputc(int ch, FILE *f)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xffff); // 一次发 1 字节，阻塞直到发完或超时
  return ch;
}
/* USER CODE END 1 */
