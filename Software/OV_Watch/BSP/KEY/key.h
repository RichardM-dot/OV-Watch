/**
 * @file    key.h
 * @brief   按键检测（GPIO 电平读取 + 消抖）。
 *
 * KEY1 为普通按键，KEY2 复用为唤醒键。两个按键均接在 GPIOA 上，
 * 电平通过宏 KEY1/KEY2 直接读取，供 KeyScan 做消抖与短按/长按判定。
 */
#ifndef __KEY_H__
#define __KEY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

//KEY1：普通功能键，接 PA5
#define KEY1_PORT	GPIOA		// KEY1 所在端口
#define KEY1_PIN	GPIO_PIN_5	// KEY1 引脚号
#define KEY1 HAL_GPIO_ReadPin(KEY1_PORT,KEY1_PIN)	// 读取 KEY1 当前电平
//KEY_Wake：唤醒键，接 PA4
#define KEY2_PORT	GPIOA		// KEY2 所在端口
#define KEY2_PIN	GPIO_PIN_4	// KEY2 引脚号
#define KEY2 HAL_GPIO_ReadPin(KEY2_PORT,KEY2_PIN)	// 读取 KEY2 当前电平
	
/** 初始化按键 GPIO（输入模式） */
void Key_Port_Init(void);
/** 按键中断回调：在 EXTI 中断中调用，用于唤醒/标记按键事件 */
void Key_Interrupt_Callback(void);
/** 扫描按键；mode 选择扫描方式，返回按键状态（具体取值见 key.c） */
uint8_t KeyScan(uint8_t mode);




#ifdef __cplusplus
}
#endif
#endif

