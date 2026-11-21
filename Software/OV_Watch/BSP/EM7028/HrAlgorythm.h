/**
 * @file    HrAlgorythm.h
 * @brief   心率计算算法接口。
 *
 * 对 EM7028 采集到的原始 PPG 数据做滑动窗口处理，估算当前心率。
 */
#ifndef __HRALGORYTHM_H__
#define __HRALGORYTHM_H__

#include "main.h"
#include "user_Queue.h"

/** 初始化心率算法内部状态（清空队列与计时） */
void HR_AlgoInit(void);
/** 输入一次采样值 present_dat 及对应时间戳 present_time(ms)，返回当前心率(bpm) */
uint16_t HR_Calculate(uint16_t present_dat,uint32_t present_time);


#endif
