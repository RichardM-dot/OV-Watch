/*
 * 【中文导读】user_KeyTask.c —— 按键任务（消抖+事件发布）
 * 轮询按键 GPIO，消抖后把"单击/长按"事件经 pubsub 发给页面管理器。
 */

/* Private includes -----------------------------------------------------------*/
//includes
#include "user_TasksInit.h"
#include "ui_HomePage.h"
#include "main.h"
#include "key.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/


/**
  * @brief  Key press check task
  * @param  argument: Not used
  * @retval None
  */
void KeyTask(void *argument) // 按键任务函数
{
	uint8_t keystr=0; // 用于存储按键扫描结果的变量，初始值为 0，表示没有按键被按下。
	uint8_t Stopstr=0; // 用于存储停止信号的变量，初始值为 0，表示没有停止信号。
	uint8_t IdleBreakstr=0; // 用于存储空闲中断信号的变量，初始值为 0，表示没有空闲中断信号。
	while(1)
	{
		switch(KeyScan(0)) // 根据KeyScan返回的keyvalue值去case需要进行的步骤
		{
			case 1:
				keystr = 1;
				osMessageQueuePut(Key_MessageQueue, &keystr, 0, 1); // 把 keystr 放入 Key_MessageQueue：通知“按键任务”或相关任务，有按键 1 事件 
				osMessageQueuePut(IdleBreak_MessageQueue, &IdleBreakstr, 0, 1); // 把 IdleBreakstr 放入 IdleBreak_MessageQueue：通知“空闲/待机相关任务”，有事件需要退出空闲状态
				break; // 退出switch不再执行case

			case 2:
				if(Page_Get_NowPage()->page_obj == &ui_HomePage) // 获取当前页面判断是否为主页
				{
					osMessageQueuePut(Stop_MessageQueue, &Stopstr, 0, 1); // 发消息到 Stop_MessageQueue，通常表示“按键 2 在主页被按下，要进入停止/关机/退出流程”。
				}
				else
				{
					keystr = 2;
					osMessageQueuePut(Key_MessageQueue, &keystr, 0, 1); // 发按键值给按键业务任务
					osMessageQueuePut(IdleBreak_MessageQueue, &IdleBreakstr, 0, 1); // 发“打断空闲”信号给空闲管理任务
				}
				break;
		}
		osDelay(1);
	}
}
