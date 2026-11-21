/**
 * @file    user_HardwareInitTask.h
 * @brief   硬件初始化任务：在调度器启动后逐个初始化各外设并检测连接状态。
 *
 * 放到任务中执行（而非 main 里顺序初始化），可避免上电初始化耗时过长，
 * 同时让屏幕能先亮起来显示启动画面。
 */
#ifndef __USER_HARDWAREINITTASK_H__
#define __USER_HARDWAREINITTASK_H__

#ifdef __cplusplus
extern "C" {
#endif

/** 硬件初始化任务：按顺序初始化各传感器并更新 HWInterface 连接状态 */
void HardwareInitTask(void *argument);

#ifdef __cplusplus
}
#endif

#endif
