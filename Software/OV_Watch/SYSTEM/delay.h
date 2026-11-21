/**
 * @file    delay.h
 * @brief   基于 SysTick 的毫秒/微秒延时接口。
 *
 * delay_init 需在系统时钟配置完成后调用一次，
 * 之后 delay_ms/delay_us 依赖 SysTick 计数实现阻塞延时。
 */
#ifndef _DELAY_H
#define _DELAY_H
#include "sys.h"	  

/** 初始化延时功能（配置 SysTick 计数基准） */
void delay_init(void);
/** 毫秒级阻塞延时，nms 为延时毫秒数 */
void delay_ms(u16 nms);
/** 微秒级阻塞延时，nus 为延时微秒数 */
void delay_us(u32 nus);
#endif
