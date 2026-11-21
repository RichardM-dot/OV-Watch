/**
 * @file    user_ChargCheckTask.h
 * @brief   充电页面/充电检测任务。
 */
#ifndef __USER_CHARGCHECKTASK_H__
#define __USER_CHARGCHECKTASK_H__

#ifdef __cplusplus
extern "C" {
#endif

/** 进入充电页面任务：检测到充电时显示充电界面并刷新电量 */
void ChargPageEnterTask(void *argument);

#ifdef __cplusplus
}
#endif

#endif
