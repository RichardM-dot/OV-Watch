/* USER CODE BEGIN Header */
/*
 * FreeRTOS Kernel V10.3.1
 * Portion Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 * Portion Copyright (C) 2019 StMicroelectronics, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */
/* USER CODE END Header */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * 中文说明（本工程的 FreeRTOS 配置要点）：
 *   - 抢占式调度，Tick 1kHz（1ms 一个系统节拍）
 *   - 支持静态 + 动态内存分配，堆使用 heap_4（共 16KB）
 *   - 56 级任务优先级，最小任务栈 128 字
 *   - 使用互斥量、递归互斥量、计数信号量、软件定时器
 *   - 最高可调用 FreeRTOS API 的中断优先级为 5（数值越小优先级越高），
 *     优先级数值高于 5 的中断中禁止调用带 FromISR 的 API。
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * These parameters and more are described within the 'configuration' section of the
 * FreeRTOS API documentation available on the FreeRTOS.org web site.
 *
 * See http://www.freertos.org/a00110.html
 *----------------------------------------------------------*/

/* USER CODE BEGIN Includes */
/* Section where include file can be added */
/* USER CODE END Includes */

/* Ensure definitions are only used by the compiler, and not by the assembler. */
#if defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__)
  #include <stdint.h>
  extern uint32_t SystemCoreClock;
#endif
#ifndef CMSIS_device_header
#define CMSIS_device_header "stm32f4xx.h"
#endif /* CMSIS_device_header */

#define configENABLE_FPU                         0	// 不启用 FPU 上下文保存（内核按软件方式处理）
#define configENABLE_MPU                         0	// 不启用 MPU 内存保护

#define configUSE_PREEMPTION                     1	// 1：抢占式调度（高优先级任务可打断低优先级任务）
#define configSUPPORT_STATIC_ALLOCATION          1	// 支持静态创建任务/队列（内存由用户提供）
#define configSUPPORT_DYNAMIC_ALLOCATION         1	// 支持动态创建（从 FreeRTOS 堆分配）
#define configUSE_IDLE_HOOK                      1	// 启用空闲任务钩子（可在此进入低功耗）
#define configUSE_TICK_HOOK                      1	// 启用系统节拍钩子
#define configCPU_CLOCK_HZ                       ( SystemCoreClock )	// CPU 主频（由系统时钟提供）
#define configTICK_RATE_HZ                       ((TickType_t)1000)	// 系统节拍 1000Hz，即 1ms 一次
#define configMAX_PRIORITIES                     ( 56 )	// 最大任务优先级个数（0~55）
#define configMINIMAL_STACK_SIZE                 ((uint16_t)128)	// 空闲任务最小栈深度（单位：字）
#define configTOTAL_HEAP_SIZE                    ((size_t)16*1024)	// FreeRTOS 堆总大小 16KB
#define configMAX_TASK_NAME_LEN                  ( 16 )	// 任务名最大长度（含结束符）
#define configUSE_TRACE_FACILITY                 1	// 启用任务状态统计/追踪接口
#define configUSE_16_BIT_TICKS                   0	// 使用 32 位 TickType_t
#define configUSE_MUTEXES                        1	// 启用互斥量
#define configQUEUE_REGISTRY_SIZE                8	// 队列注册表大小（调试用，可注册 8 个队列）
#define configUSE_RECURSIVE_MUTEXES              1	// 启用递归互斥量
#define configUSE_COUNTING_SEMAPHORES            1	// 启用计数型信号量
#define configUSE_PORT_OPTIMISED_TASK_SELECTION  0	// 不使用硬件前导零指令优化任务选择
/* USER CODE BEGIN MESSAGE_BUFFER_LENGTH_TYPE */
/* Defaults to size_t for backward compatibility, but can be changed
   if lengths will always be less than the number of bytes in a size_t. */
#define configMESSAGE_BUFFER_LENGTH_TYPE         size_t
/* USER CODE END MESSAGE_BUFFER_LENGTH_TYPE */

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES                    0	// 不使用协程（协程已被官方标记为过时）
#define configMAX_CO_ROUTINE_PRIORITIES          ( 2 )	// 协程优先级个数（未启用）

/* Software timer definitions. */
#define configUSE_TIMERS                         1	// 启用软件定时器
#define configTIMER_TASK_PRIORITY                ( 2 )	// 定时器服务任务优先级
#define configTIMER_QUEUE_LENGTH                 10	// 定时器命令队列长度
#define configTIMER_TASK_STACK_DEPTH             256	// 定时器服务任务栈深度（字）

/* CMSIS-RTOS V2 flags */
#define configUSE_OS2_THREAD_SUSPEND_RESUME  1	// 允许 CMSIS-RTOS2 挂起/恢复线程
#define configUSE_OS2_THREAD_ENUMERATE       1	// 允许 CMSIS-RTOS2 枚举线程
#define configUSE_OS2_EVENTFLAGS_FROM_ISR    1	// 允许在中断中设置事件标志
#define configUSE_OS2_THREAD_FLAGS           1	// 允许 CMSIS-RTOS2 线程标志
#define configUSE_OS2_TIMER                  1	// 允许 CMSIS-RTOS2 定时器接口
#define configUSE_OS2_MUTEX                  1	// 允许 CMSIS-RTOS2 互斥量接口

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */
#define INCLUDE_vTaskPrioritySet             1	// 允许修改任务优先级
#define INCLUDE_uxTaskPriorityGet            1	// 允许查询任务优先级
#define INCLUDE_vTaskDelete                  1	// 允许删除任务
#define INCLUDE_vTaskCleanUpResources        0	// 不编译资源清理接口（节省空间）
#define INCLUDE_vTaskSuspend                 1	// 允许挂起/恢复任务
#define INCLUDE_vTaskDelayUntil              1	// 允许绝对延时（固定周期任务）
#define INCLUDE_vTaskDelay                   1	// 允许相对延时
#define INCLUDE_xTaskGetSchedulerState       1	// 允许查询调度器状态
#define INCLUDE_xTimerPendFunctionCall       1	// 允许在定时器中回调函数
#define INCLUDE_xQueueGetMutexHolder         1	// 允许查询互斥量持有者
#define INCLUDE_uxTaskGetStackHighWaterMark  1	// 允许查询任务栈历史最小剩余量
#define INCLUDE_xTaskGetCurrentTaskHandle    1	// 允许获取当前任务句柄
#define INCLUDE_eTaskGetState                1	// 允许查询任务状态

/*
 * The CMSIS-RTOS V2 FreeRTOS wrapper is dependent on the heap implementation used
 * by the application thus the correct define need to be enabled below
 */
#define USE_FreeRTOS_HEAP_4	// 使用 heap_4 内存管理方案（支持碎片合并）

/* Cortex-M specific definitions. */
#ifdef __NVIC_PRIO_BITS
 /* __BVIC_PRIO_BITS will be specified when CMSIS is being used. */
 #define configPRIO_BITS         __NVIC_PRIO_BITS	// 使用 CMSIS 定义的优先级位数
#else
 #define configPRIO_BITS         4	// 默认 4 位优先级（STM32F4 实际为 4 位）
#endif

/* The lowest interrupt priority that can be used in a call to a "set priority"
function. */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY   15	// 最低中断优先级（数值最大）

/* The highest interrupt priority that can be used by any interrupt service
routine that makes calls to interrupt safe FreeRTOS API functions.  DO NOT CALL
INTERRUPT SAFE FREERTOS API FUNCTIONS FROM ANY INTERRUPT THAT HAS A HIGHER
PRIORITY THAN THIS! (higher priorities are lower numeric values. */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5	// 可调用 FreeRTOS API 的最高优先级（数值最小为 5）

/* Interrupt priorities used by the kernel port layer itself.  These are generic
to all Cortex-M ports, and do not rely on any particular library functions. */
#define configKERNEL_INTERRUPT_PRIORITY 		( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )	// 内核（PendSV/SysTick）使用最低优先级
/* !!!! configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to zero !!!!
See http://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html. */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )	// 换算到寄存器实际使用的优先级值

/* Normal assert() semantics without relying on the provision of an assert.h
header file. */
/* USER CODE BEGIN 1 */
#define configASSERT( x ) if ((x) == 0) {taskDISABLE_INTERRUPTS(); for( ;; );}	// 断言失败时关中断并死循环，便于调试定位
/* USER CODE END 1 */

/* Definitions that map the FreeRTOS port interrupt handlers to their CMSIS
standard names. */
#define vPortSVCHandler    SVC_Handler		// FreeRTOS SVC 异常映射到 CMSIS 名称
#define xPortPendSVHandler PendSV_Handler	// FreeRTOS 上下文切换异常映射到 CMSIS 名称

/* IMPORTANT: After 10.3.1 update, Systick_Handler comes from NVIC (if SYS timebase = systick), otherwise from cmsis_os2.c */

#define USE_CUSTOM_SYSTICK_HANDLER_IMPLEMENTATION 0	// SysTick 由 CMSIS-RTOS2 统一处理

/* USER CODE BEGIN Defines */
#define configUSE_STATS_FORMATTING_FUNCTIONS 1	// 允许把任务统计信息格式化为字符串（调试用）

/* Section where parameter definitions can be added (for instance, to override default ones in FreeRTOS.h) */
/* USER CODE END Defines */

#endif /* FREERTOS_CONFIG_H */
