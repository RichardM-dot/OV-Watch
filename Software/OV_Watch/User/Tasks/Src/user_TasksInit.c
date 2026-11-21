/*
 * ============================================================================
 * 【中文导读】user_TasksInit.c —— FreeRTOS 任务总装（先读这个文件！）
 * ----------------------------------------------------------------------------
 * 整块手表的任务体系在这里创建：硬件初始化任务最先跑（初始化外设后自杀），
 * 然后常驻任务按优先级排列：屏幕刷新/传感器更新/按键/存数/充电检测/消息发送。
 * 每个任务的栈大小、优先级、周期一目了然——学习"小项目任务划分"的最佳范本。
 * ============================================================================
 */

/* Private includes -----------------------------------------------------------*/
//includes
#include "user_TasksInit.h"
//sys
#include "sys.h"
#include "stdio.h"
#include "lcd.h"
#include "WDOG.h"
//gui
#include "lvgl.h"
#include "ui_TimerPage.h"
//tasks
#include "user_HardwareInitTask.h"
#include "user_RunModeTasks.h"
#include "user_KeyTask.h"
#include "user_ScrRenewTask.h"
#include "user_SensUpdateTask.h"
#include "user_ChargCheckTask.h"
#include "user_MessageSendTask.h"
#include "user_DataSaveTask.h"

/* Private typedef -----------------------------------------------------------*/


/* Private define ------------------------------------------------------------*/


/* Private variables ---------------------------------------------------------*/


/* Tasks ---------------------------------------------------------------------*/
// 用户显示创建的任务句柄和属性结构体，FreeRTOS 通过这些信息来管理任务的创建和调度，并不是freertos管理的全部任务。
// Hardwares initialization硬件初始化任务	
osThreadId_t HardwareInitTaskHandle; // 任务句柄，在函数 osThreadNew() 创建任务时返回，用于后续操作（如挂起、删除等）。
const osThreadAttr_t HardwareInitTask_attributes = { // 结构体变量告诉 FreeRTOS 任务的属性，包括任务名、栈大小、优先级等，只是存放参数而不是指向任务。
  .name = "HardwareInitTask",
  .stack_size = 128 * 10,
  .priority = (osPriority_t) osPriorityHigh3,
};

//LVGL Handler taskLVGL 界面处理任务	
osThreadId_t LvHandlerTaskHandle; 
const osThreadAttr_t LvHandlerTask_attributes = {
  .name = "LvHandlerTask",
  .stack_size = 128 * 24,
  .priority = (osPriority_t) osPriorityLow,
};

//WDOG Feed task看门狗喂狗任务	
osThreadId_t WDOGFeedTaskHandle;
const osThreadAttr_t WDOGFeedTask_attributes = {
  .name = "WDOGFeedTask",
  .stack_size = 128 * 1,
  .priority = (osPriority_t) osPriorityHigh2,
};

//ScrRenew task屏幕刷新任务
osThreadId_t ScrRenewTaskHandle;
const osThreadAttr_t ScrRenewTask_attributes = {
  .name = "ScrRenewTask",
  .stack_size = 128 * 10,
  .priority = (osPriority_t) osPriorityLow1,
};

//SensorDataRenew task传感器数据任务	
osThreadId_t SensorDataTaskHandle;
const osThreadAttr_t SensorDataTask_attributes = {
  .name = "SensorDataTask",
  .stack_size = 128 * 5,
  .priority = (osPriority_t) osPriorityLow1,
};

//HRDataRenew task心率数据任务	
osThreadId_t HRDataTaskHandle;
const osThreadAttr_t HRDataTask_attributes = {
  .name = "HRDataTask",
  .stack_size = 128 * 5,
  .priority = (osPriority_t) osPriorityLow1,
};

//ChargPageEnterTask充电页面进入任务	
osThreadId_t ChargPageEnterTaskHandle;
const osThreadAttr_t ChargPageEnterTask_attributes = {
  .name = "ChargPageEnterTask",
  .stack_size = 128 * 10,
  .priority = (osPriority_t) osPriorityLow1,
};

//messagesendtask消息发送任务	
osThreadId_t MessageSendTaskHandle;
const osThreadAttr_t MessageSendTask_attributes = {
  .name = "MessageSendTask",
  .stack_size = 128 * 5,
  .priority = (osPriority_t) osPriorityLow1,
};

//MPUCheckTaskMPU 检查任务	
osThreadId_t MPUCheckTaskHandle;
const osThreadAttr_t MPUCheckTask_attributes = {
  .name = "MPUCheckTask",
  .stack_size = 128 * 3,
  .priority = (osPriority_t) osPriorityLow2,
};

//DataSaveTask数据保存任务	
osThreadId_t DataSaveTaskHandle;
const osThreadAttr_t DataSaveTask_attributes = {
  .name = "DataSaveTask",
  .stack_size = 128 * 5,
  .priority = (osPriority_t) osPriorityLow2,
};

// Idle Enter Task空闲进入任务
osThreadId_t IdleEnterTaskHandle;
const osThreadAttr_t IdleEnterTask_attributes = {
  .name = "IdleEnterTask",
  .stack_size = 128 * 1,
  .priority = (osPriority_t) osPriorityHigh,
};

//Stop Enter Task停止进入任务	
osThreadId_t StopEnterTaskHandle;
const osThreadAttr_t StopEnterTask_attributes = {
  .name = "StopEnterTask",
  .stack_size = 128 * 16,
  .priority = (osPriority_t) osPriorityHigh1,
};

//Key task按键任务	
osThreadId_t KeyTaskHandle;
const osThreadAttr_t KeyTask_attributes = {
  .name = "KeyTask",
  .stack_size = 128 * 1,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Message queues ------------------------------------------------------------*/
// Key message。
// 消息队列/事件标志对象的句柄。它们创建后指向队列控制块或事件标志控制块，用于任务间收发消息、设置/等待事件。
osMessageQueueId_t Key_MessageQueue;
osMessageQueueId_t Idle_MessageQueue;
osMessageQueueId_t Stop_MessageQueue;
osMessageQueueId_t IdleBreak_MessageQueue;
osMessageQueueId_t HomeUpdata_MessageQueue;
osMessageQueueId_t DataSave_MessageQueue;
osEventFlagsId_t HardIntEventHandle;

/* Private function prototypes -----------------------------------------------*/
// 定义不在当前文件中实现的函数原型，告诉编译器这些函数将在其他地方定义，以便在当前文件中调用它们时不会报错。
void LvHandlerTask(void *argument); // LVGL处理任务
void WDOGFeedTask(void *argument); // 看门狗喂狗任务

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void User_Tasks_Init(void)
{
  /* add mutexes, ... */

  /* add semaphores, ... */

  /* start timers, add new ones, ... */

  /* add queues（创建消息队列）, ... */
	Key_MessageQueue  = osMessageQueueNew(1, 1, NULL); // osMessageQueueNew(消息个数, 每条消息字节数, 属性)
	Idle_MessageQueue = osMessageQueueNew(1, 1, NULL);
	Stop_MessageQueue = osMessageQueueNew(1, 1, NULL);
	IdleBreak_MessageQueue = osMessageQueueNew(1, 1, NULL);
	HomeUpdata_MessageQueue = osMessageQueueNew(1, 1, NULL);
	DataSave_MessageQueue = osMessageQueueNew(2, 1, NULL);
  HardIntEventHandle = osEventFlagsNew(NULL);

	/* add threads（创建线程）, ... */
  HardwareInitTaskHandle  = osThreadNew(HardwareInitTask, NULL, &HardwareInitTask_attributes);
  LvHandlerTaskHandle  = osThreadNew(LvHandlerTask, NULL, &LvHandlerTask_attributes);
  WDOGFeedTaskHandle   = osThreadNew(WDOGFeedTask, NULL, &WDOGFeedTask_attributes);
	IdleEnterTaskHandle  = osThreadNew(IdleEnterTask, NULL, &IdleEnterTask_attributes);
	StopEnterTaskHandle  = osThreadNew(StopEnterTask, NULL, &StopEnterTask_attributes);
	KeyTaskHandle 			 = osThreadNew(KeyTask, NULL, &KeyTask_attributes);
	ScrRenewTaskHandle   = osThreadNew(ScrRenewTask, NULL, &ScrRenewTask_attributes);
	SensorDataTaskHandle = osThreadNew(SensorDataUpdateTask, NULL, &SensorDataTask_attributes);
	HRDataTaskHandle		 = osThreadNew(HRDataUpdateTask, NULL, &HRDataTask_attributes);
	ChargPageEnterTaskHandle = osThreadNew(ChargPageEnterTask, NULL, &ChargPageEnterTask_attributes);
  MessageSendTaskHandle = osThreadNew(MessageSendTask, NULL, &MessageSendTask_attributes);
	MPUCheckTaskHandle		= osThreadNew(MPUCheckTask, NULL, &MPUCheckTask_attributes);
	DataSaveTaskHandle		= osThreadNew(DataSaveTask, NULL, &DataSaveTask_attributes);

  /* add events, ... */


	/* add  others ... */
	uint8_t HomeUpdataStr; // 定义一个无符号8位整型变量 HomeUpdataStr，用于存储主界面更新的标志或数据。
	osMessageQueuePut(HomeUpdata_MessageQueue, &HomeUpdataStr, 0, 1); // 将 HomeUpdataStr 放入消息队列 HomeUpdata_MessageQueue 中，优先级为 0，超时时间为 1 个系统节拍。
}


/**
  * @brief  FreeRTOS Tick Hook, to increase the LVGL tick
  * @param  None
  * @retval None
  */
void TaskTickHook(void)
{
	//to increase the LVGL tick
	lv_tick_inc(1); // 每次系统节拍中断时调用 lv_tick_inc(1)，将 LVGL 的内部计数器增加 1 毫秒，以便 LVGL 能够正确处理动画和定时器。
	//to increase the timerpage's timer(put in here is to ensure the Real Time)
	if(ui_TimerPageFlag) // 检查 ui_TimerPageFlag 是否为真，表示计时页面正在显示。
	{
    IdleTimerCount = 0;
    ui_TimerPage_elapsed_ms += 1;
	} // 为真时会将 IdleTimerCount 计数器清零，并将 ui_TimerPage_elapsed_ms 增加 1 毫秒，用于跟踪计时页面的运行时间。
  else
  {
    IdleTimerCount += 1;
  } // 不为真时会增加 IdleTimerCount 计数器，用于跟踪系统空闲时间。
	user_HR_timecount+=1; // 每次系统节拍中断时，user_HR_timecount 计数器增加 1，用于心率数据的采集和处理。
}


/**
  * @brief  LVGL Handler task, to run the lvgl
  * @param  argument: Not used
  * @retval None
  */
void LvHandlerTask(void *argument)
{
	uint8_t IdleBreakstr=0;
  while(1)
  {
		if(lv_disp_get_inactive_time(NULL)<1000) // 检查 LVGL 显示器的非活动时间是否小于 1000 毫秒（1 秒），如果是，则表示用户正在与界面交互。
		{
			//Idle time break, set to 0
			osMessageQueuePut(IdleBreak_MessageQueue, &IdleBreakstr, 0, 0); // 如果用户正在与界面交互，则将 IdleBreakstr 放入 IdleBreak_MessageQueue 中，优先级为 0，超时时间为 0 个系统节拍，以通知其他任务系统处于活动状态。
		}
    uint32_t wait = lv_task_handler();

    if(wait < 1U) wait = 1U; // 确保 wait 的最小值为 1 毫秒，以避免在 lv_task_handler() 返回 0 时导致 osDelay(0) 的调用，从而使任务立即重新调度，可能会导致 CPU 占用过高。
    if(wait > 30U) wait = 30U; // 确保 wait 的最大值为 30 毫秒，以避免在 lv_task_handler() 返回过大值时导致 osDelay() 的调用，从而使任务长时间阻塞，可能会影响界面刷新和用户交互的响应速度。

    osDelay(wait);
	}
}


/**
  * @brief  Watch Dog Feed task
  * @param  argument: Not used
  * @retval None
  */
void WDOGFeedTask(void *argument)
{
	//owdg
	WDOG_Port_Init(); // 这个函数由硬件抽象层（BSP）提供，用于初始化看门狗定时器的端口配置，包括时钟使能、GPIO 配置等，以确保看门狗能够正常工作。
  while(1)
  {
		WDOG_Feed();
		WDOG_Enable();
    osDelay(100); // 这个函数由硬件抽象层（BSP）提供，用于喂狗操作，即向看门狗定时器写入特定的值，以防止看门狗复位系统。WDOG_Enable() 用于启用看门狗定时器。
  }
}


