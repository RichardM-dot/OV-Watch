/**
 * @file    user_RunModeTasks.h
 * @brief   运行模式相关任务：进入待机（熄屏）与停止（关机）流程。
 */
#ifndef __USER_RUNMODETASKS_H__
#define __USER_RUNMODETASKS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "user_TasksInit.h"

/** 待机任务：无操作超时后熄屏/进入低功耗，按键或抬腕时唤醒 */
void IdleEnterTask(void *argument);
/** 停止任务：执行关机流程（保存数据、关闭外设、切断电源） */
void StopEnterTask(void *argument);

extern uint32_t IdleTimerCount;	// 无操作计时（单位：Tick），用于判断是否进入待机

#ifdef __cplusplus
}
#endif

#endif
