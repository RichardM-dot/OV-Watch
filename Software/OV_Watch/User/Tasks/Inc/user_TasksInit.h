/**
 * @file    user_TasksInit.h
 * @brief   所有 FreeRTOS 任务与消息队列/事件标志的创建入口。
 *
 * 各业务任务通过消息队列通信（按键、熄屏、计步更新等），
 * 硬件中断通过 HardIntEventHandle 事件标志通知任务层处理，
 * TaskTickHook 在系统节拍钩子中被调用，用于软件定时计数。
 */
#ifndef __USER_TASKSINIT_H__
#define __USER_TASKSINIT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "cmsis_os.h"

#define SCRRENEW_DEPTH	5	// 屏幕刷新消息队列深度
#define HARDINT_EVENT_UART  (1U << 0)	// 事件标志位 0：串口收到数据
#define HARDINT_EVENT_CHARG (1U << 1)	// 事件标志位 1：充电状态变化
#define HARDINT_EVENT_MPU   (1U << 2)	// 事件标志位 2：MPU 中断（抬腕/计步）

extern osMessageQueueId_t Key_MessageQueue;			// 按键事件消息队列
extern osMessageQueueId_t Idle_MessageQueue;		// 熄屏/待机消息队列
extern osMessageQueueId_t Stop_MessageQueue;		// 停止（关机流程）消息队列
extern osMessageQueueId_t IdleBreak_MessageQueue;	// 退出待机消息队列
extern osMessageQueueId_t HomeUpdata_MessageQueue;	// 主页数据刷新消息队列
extern osMessageQueueId_t DataSave_MessageQueue;	// 参数保存消息队列
extern osEventFlagsId_t HardIntEventHandle;			// 中断事件标志组

/** 创建全部任务、队列与事件标志（在 main 中调度器启动前调用） */
void User_Tasks_Init(void);
/** 系统节拍钩子：每个 Tick 调用，用于软件计时 */
void TaskTickHook(void);

#ifdef __cplusplus
}
#endif

#endif
