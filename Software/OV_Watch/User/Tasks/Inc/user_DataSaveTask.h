/**
 * @file    user_DataSaveTask.h
 * @brief   参数保存任务：收到保存消息后把设置写入 EEPROM，避免频繁擦写。
 */
#ifndef __USER_DATASAVETASK_H__
#define __USER_DATASAVETASK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "user_TasksInit.h"


/** 数据保存任务：监听 DataSave_MessageQueue，批量写 EEPROM */
void DataSaveTask(void *argument);

	
#ifdef __cplusplus
}
#endif

#endif
