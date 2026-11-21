/**
 * @file    user_SensUpdateTask.h
 * @brief   传感器数据更新任务：周期采集环境、姿态、心率数据并写入 HWInterface。
 */
#ifndef __USER_SENSUPDATETASK_H__
#define __USER_SENSUPDATETASK_H__

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t user_HR_timecount;	// 心率采集计时（单位：Tick）

/** 环境/姿态传感器更新任务（温湿度、气压、罗盘等） */
void SensorDataUpdateTask(void *argument);
/** 心率/血氧数据更新任务 */
void HRDataUpdateTask(void *argument);
/** MPU 状态检查任务（连接检测、抬腕/计步事件处理） */
void MPUCheckTask(void *argument);

#ifdef __cplusplus
}
#endif

#endif
