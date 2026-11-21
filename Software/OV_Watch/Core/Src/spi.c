/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    spi.c
  * @brief   This file provides code for the configuration
  *          of the SPI instances.
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
 * 【中文导读】spi.c —— SPI1 驱动 LCD（ST7789）
 * ----------------------------------------------------------------------------
 * SPI1 工作在主机模式、8 位数据、CPOL=1/CPHA=1（即模式 3）、软件 NSS、MSB 先行，
 * 时钟 = PCLK2(100MHz) / 2 = 50MHz，是"刷屏要多快有多快"的暴力配置。
 * 只用了 SCK(PB3) 和 MOSI(PB5) 两根线：LCD 是单向写设备，不需要 MISO；
 * 片选 CS、数据/命令 DC、复位 RST 都由 BSP 用普通 GPIO 控制，所以 NSS 选软件。
 * 像素数据由 LCD_Color_Fill() 走 HAL_SPI_Transmit_DMA，用 DMA2_Stream2/通道2
 * 搬运，CPU 只在传输完成回调里等 BSY 清零，然后通知 LVGL 可以刷下一块。
 * 坑：PB3 上电默认是 JTDO，必须是"Serial Wire"调试模式才能当 SCK 用；50MHz 已
 * 超出 ST7789 手册的典型写周期，靠排线短和屏的容忍度，换板子可能要降分频。
 * ============================================================================
 */
/* Includes ------------------------------------------------------------------*/
#include "spi.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_tx;

/* SPI1 init function */
/**
  * @brief  配置 SPI1 为主机模式，用于向 ST7789 写命令和像素。
  * @note   CPOL=HIGH + CPHA=2EDGE 就是常说的 SPI 模式 3：时钟空闲为高，第二个
  *         边沿采样，ST7789 支持模式 0/3，选 3 与常见例程一致。
  *         NSS 用软件：LCD 的 CS 脚由 BSP 单独拉低/拉高，这样一条 SPI 总线将来
  *         还能挂别的从机。BaudRatePrescaler=2 时 SCK=100MHz/2=50MHz，是本工程
  *         能给出的最高频率，用来压榨刷屏帧率。
  * @retval None
  */
void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  hspi1.Instance = SPI1;                        // 绑定 SPI1 寄存器组（挂在 APB2，100MHz）
  hspi1.Init.Mode = SPI_MODE_MASTER;            // 主机模式：时钟由 STM32 产生，LCD 只是从机
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;  // 双线模式（全双工）；MISO 没接，实际只用到 MOSI
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;      // 8 位一帧：ST7789 的命令是 8 位，颜色拆成高低两个字节发
  hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;   // 空闲时 SCK 为高（模式 3 的一半）
  hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;        // 第二个边沿采样（模式的另一半），与 ST7789 的写时序匹配
  hspi1.Init.NSS = SPI_NSS_SOFT;                // 软件片选：CS 由 BSP 的 GPIO 控制，方便一总线多设备
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2; // 100MHz/2 = 50MHz，刷屏瓶颈就在这
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;       // 高位先发，ST7789 规定
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;       // 不用 TI 的另一种帧格式，普通 Motorola SPI 即可
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE; // 屏不校验 CRC，开了反而多一个字节
  hspi1.Init.CRCPolynomial = 10;                // CRC 关闭时此值无效，仅 CubeMX 占位
  if (HAL_SPI_Init(&hspi1) != HAL_OK)           // 内部回调 HAL_SPI_MspInit：开时钟、配 PB3/PB5、配 TX DMA
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief  SPI1 的 MSP 底层初始化：外设时钟、SCK/MOSI 复用、TX DMA 通道。
  * @note   只配 PB3/PB5，不配 PB4(MISO)——LCD 只写不读，省一个引脚也省一次配置。
  *         PB3 复位后默认是 JTDO，能当 SPI1_SCK 的前提是调试口被设成 Serial Wire
  *         （SWD 只用 PA13/PA14），否则引脚会被 JTAG 占用。
  * @param  spiHandle: SPI 句柄指针
  * @retval None
  */
void HAL_SPI_MspInit(SPI_HandleTypeDef* spiHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(spiHandle->Instance==SPI1)
  {
  /* USER CODE BEGIN SPI1_MspInit 0 */

  /* USER CODE END SPI1_MspInit 0 */
    /* SPI1 clock enable */
    __HAL_RCC_SPI1_CLK_ENABLE();                 // 先开 SPI1 时钟，后续寄存器才可写

    __HAL_RCC_GPIOB_CLK_ENABLE();                // PB 口时钟：PB3/PB5 要当复用脚
    /**SPI1 GPIO Configuration
    PB3     ------> SPI1_SCK
    PB5     ------> SPI1_MOSI
    */
    GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_5;  // 时钟线和数据线一起配；MISO(PB4)不用
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;      // 复用推挽：SCK/MOSI 都由 STM32 主动驱动
    GPIO_InitStruct.Pull = GPIO_NOPULL;          // 50MHz 下不加内部上下拉，避免拖慢边沿
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH; // 高速档是 50MHz 信号完整性的前提
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;   // AF5 是 SPI1 的固定复用编号
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* SPI1 DMA Init */
    /* SPI1_TX Init */
    // 刷屏是"大块内存 → 外设"，所以方向 MEMORY_TO_PERIPH；外设地址固定（DR），
    // 内存地址递增；NORMAL 模式表示一块传完就停，正好对应 LCD_Color_Fill 一次调用。
    hdma_spi1_tx.Instance = DMA2_Stream2;        // SPI1_TX 固定映射到 DMA2 Stream2 通道2
    hdma_spi1_tx.Init.Channel = DMA_CHANNEL_2;
    hdma_spi1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_spi1_tx.Init.PeriphInc = DMA_PINC_DISABLE;      // 外设侧永远是 SPI1->DR
    hdma_spi1_tx.Init.MemInc = DMA_MINC_ENABLE;          // 显存缓冲区逐字节往后搬
    hdma_spi1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE; // SPI 数据寄存器按字节访问
    hdma_spi1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;    // LVGL 的颜色缓冲区是 uint8_t*
    hdma_spi1_tx.Init.Mode = DMA_NORMAL;                 // 单次传输，一次刷一块
    hdma_spi1_tx.Init.Priority = DMA_PRIORITY_LOW;       // 低优先级：即便刷屏，也别挡住串口/其它流
    hdma_spi1_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;   // 关 FIFO，字节流直接模式，避免半字对齐问题
    if (HAL_DMA_Init(&hdma_spi1_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(spiHandle,hdmatx,hdma_spi1_tx); // 挂到 SPI 句柄，HAL_SPI_Transmit_DMA 才知道用哪条流

  /* USER CODE BEGIN SPI1_MspInit 1 */

  /* USER CODE END SPI1_MspInit 1 */
  }
}

/**
  * @brief  SPI1 反初始化：关时钟、还原引脚、释放 TX DMA。
  * @note   运行期不调用；注意这里没有关 DMA2_Stream2 的 NVIC，因为在 dma.c 里
  *         统一使能，反初始化只复位流本身。
  * @param  spiHandle: SPI 句柄指针
  * @retval None
  */
void HAL_SPI_MspDeInit(SPI_HandleTypeDef* spiHandle)
{

  if(spiHandle->Instance==SPI1)
  {
  /* USER CODE BEGIN SPI1_MspDeInit 0 */

  /* USER CODE END SPI1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_SPI1_CLK_DISABLE();

    /**SPI1 GPIO Configuration
    PB3     ------> SPI1_SCK
    PB5     ------> SPI1_MOSI
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_3|GPIO_PIN_5);

    /* SPI1 DMA DeInit */
    HAL_DMA_DeInit(spiHandle->hdmatx);
  /* USER CODE BEGIN SPI1_MspDeInit 1 */

  /* USER CODE END SPI1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
