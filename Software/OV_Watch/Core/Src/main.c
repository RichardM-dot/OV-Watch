/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component. 在这软件组件的根目录下。
  * If no LICENSE file comes with this software, it is provided AS-IS.
  * AS-IS = 现在是什么样，就按什么样给你
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "adc.h"
#include "dma.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

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
void SystemClock_Config(void); // 负责配置系统时钟的函数声明。
void MX_FREERTOS_Init(void); // 负责初始化FreeRTOS的函数声明。
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	//this must set same as keil setting
	SCB->VTOR = 0x0000C000U; // 把一个绝对地址当成结构体指针来用，通过结构体成员去访问硬件寄存器,告诉内核：中断向量表的起始地址在 0x0000C000
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
	__enable_irq(); // 使能全局中断，允许中断请求被响应。
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_RTC_Init();
  MX_SPI1_Init();
  MX_TIM3_Init(); // 计数模式、预分频（PSC）、自动重装载值（ARR）、时钟分频、PWM/输入捕获/输出比较通道配置、中断等
  MX_USART1_UART_Init(); // 配置波特率、数据位、停止位、校验位、硬件流控制等参数，并使能UART模块。
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Init scheduler */
  // 初始化 RTOS 内核，为后续创建任务、队列、信号量等内核对象做好准备。
  osKernelInitialize();  /* Call init function for freertos objects (in freertos.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart(); // 启动 RTOS 调度器，让已经创建好的任务开始按优先级被调度运行

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void) // 把芯片从“复位后的默认时钟（HSI 16MHz）”切换到“PLL 倍频后的 100MHz 系统时钟”，并配置好 AHB/APB 总线分频和 Flash 等待周期。
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON; // 开启外部时钟
  RCC_OscInitStruct.HSIState = RCC_HSI_ON; // 开启内部高速时钟
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT; // 内部高速时钟的校准值，使用默认值即可
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON; // 开启 PLL（锁相环）模块，用于生成更高频率的系统时钟
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI; // 设置 PLL 的输入时钟源为 HSI（内部高速时钟）
  RCC_OscInitStruct.PLL.PLLM = 8; // 设置 PLL 的输入分频系数为 8，将 HSI 的 16MHz 分频为 2MHz
  RCC_OscInitStruct.PLL.PLLN = 100; // 设置 PLL 的倍频系数为 100，将 2MHz 的输入时钟倍频为 200MHz
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2; // 设置 PLL 的主输出分频系数为 2，将 200MHz 的时钟分频为 100MHz，作为系统时钟 SYSCLK
  RCC_OscInitStruct.PLL.PLLQ = 4; // 设置 PLL 的 USB/SDIO/RNG 输出分频系数为 4，将 200MHz 的时钟分频为 50MHz，作为 USB/SDIO/RNG 外设的时钟
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) // 调用 HAL 库函数配置 RCC 振荡器，如果配置失败则进入错误处理函数
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2; // 指定要配置的时钟类型，包括 AHB 总线时钟 HCLK、系统时钟 SYSCLK、APB1 总线时钟 PCLK1 和 APB2 总线时钟 PCLK2
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK) // 设置 Flash 的访问延时为 3 个时钟周期，以适应 100MHz 的系统时钟频率
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) // 当 TIM1 的计数周期结束时被调用的回调函数，用于在非阻塞模式下处理定时器中断
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq(); // 禁用全局中断，防止在错误处理过程中被其他中断打断，确保错误处理的原子性。
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT // 用于在调试阶段检查函数参数的有效性，如果启用了 USE_FULL_ASSERT 宏，则会在参数检查失败时调用 assert_failed 函数。
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
