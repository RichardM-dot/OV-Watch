/**
 * @file    user_ScrRenewTask.h
 * @brief   屏幕刷新任务：驱动 LVGL 定时器与页面切换。
 *
 * 任务周期调用 lv_timer_handler() 处理界面事件，
 * 并从队列取出待切换页面，通过 PageManager 完成入栈/出栈。
 */
#ifndef __USER_SCRRENEWTASK_H__
#define __USER_SCRRENEWTASK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "PageManager.h"

/** 屏幕刷新任务（LVGL 主循环） */
void ScrRenewTask(void *argument);


#ifdef __cplusplus
}
#endif

#endif
