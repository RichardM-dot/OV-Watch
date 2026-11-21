/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h" // 包含 FreeRTOS 的 CMSIS-RTOS API 头文件，用于任务管理、信号量、消息队列等功能

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "user_TasksInit.h"
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
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle; // 定义一个不返回值的线程ID类型变量 defaultTaskHandle，用于存储默认任务的句柄。
const osThreadAttr_t defaultTask_attributes = { // 创建线程时的属性/配置参数，用于指定线程的名称、堆栈大小和优先级等信息。
  .name = "defaultTask", // 线程名称为 "defaultTask"。
  .stack_size = 128 * 4, // 线程堆栈大小为 128 个字（每个字为 4 字节），即 512 字节。
  .priority = (osPriority_t) osPriorityNormal, // 线程优先级为 osPriorityNormal，表示普通优先级。
}; // C99 指定初始化器：.成员名 = 值，按名字赋值，顺序无关，未指定成员自动为 0/NULL。

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void vApplicationIdleHook(void); // 声明空闲任务钩子函数 vApplicationIdleHook()，在空闲任务循环中被调用，可用于执行低优先级的后台任务。
void vApplicationTickHook(void); // 声明系统节拍钩子函数 vApplicationTickHook()，在每个系统节拍中断时被调用，可用于执行周期性任务或计时器操作。

/* USER CODE BEGIN 2 */
void vApplicationIdleHook( void ) // freertos的空闲任务钩子函数，空闲任务在没有其他任务可运行时执行。可以在这个函数体里添加自己的空闲处理逻辑，但必须遵：不阻塞、能返回。
{                                 // 不是所有的 FreeRTOS 配置都启用了空闲任务钩子函数，必须在 FreeRTOSConfig.h 中将 configUSE_IDLE_HOOK 设置为 1 才会启用。
   /* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
   to 1 in FreeRTOSConfig.h. It will be called on each iteration of the idle
   task. It is essential that code added to this hook function never attempts
   to block in any way (for example, call xQueueReceive() with a block time
   specified, or call vTaskDelay()). If the application makes use of the
   vTaskDelete() API function (as this demo application does) then it is also
   important that vApplicationIdleHook() is permitted to return to its calling
   function, because it is the responsibility of the idle task to clean up
   memory allocated by the kernel to any task that has since been deleted. */
} //目的是为了在空闲任务中执行一些低优先级的后台任务或维护操作，同时确保不会阻塞系统的正常运行。
/* USER CODE END 2 */

/* USER CODE BEGIN 3 */
void vApplicationTickHook( void )
{
   /* This function will be called by each tick interrupt if
   configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h. User code can be
   added here, but the tick hook is called from an interrupt context, so
   code must not attempt to block, and only the interrupt safe FreeRTOS API
   functions can be used (those that end in FromISR()). */
	TaskTickHook(); // 调用自定义的任务节拍钩子函数 TaskTickHook()，用于在每个系统节拍中断时执行特定的任务处理逻辑。
} // 目的是为了在每个系统节拍中断时执行特定的任务处理逻辑，例如更新计时器、处理定时事件或执行周期性任务，同时确保不会阻塞系统的正常运行。
/* USER CODE END 3 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
	User_Tasks_Init(); // 调用自定义的用户任务初始化函数 User_Tasks_Init()，用于创建和初始化用户定义的任务、消息队列、信号量等资源。
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes); // 定义一个默认任务 defaultTask，并将其句柄存储在 defaultTaskHandle 中。使用 osThreadNew() 函数创建线程，传入线程函数 StartDefaultTask、参数 NULL 和线程属性 defaultTask_attributes。

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument) // 定义默认任务的线程函数 StartDefaultTask()，用于执行默认任务的逻辑。参数 argument 未使用。
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
		HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13); //该函数只是创建一个简单的默认任务，循环中不断切换 GPIOC 的第 13 个引脚的电平状态，实现 LED 灯的闪烁效果。通过调用 HAL_GPIO_TogglePin() 函数来切换引脚状态。
    osDelay(500);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

