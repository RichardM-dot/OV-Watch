/**
 * @file    power.h
 * @brief   电源管理：电池电压检测、充电状态检测、整机电源开关控制。
 *
 * 硬件连接（全部在 GPIOA）：
 *   - PA1：电池电压采样（ADC 分压输入）
 *   - PA2：充电检测输入
 *   - PA3：电源保持/使能输出（高电平维持供电）
 */
#ifndef _POWER_H_
#define _POWER_H_

#include "stm32f4xx_hal.h"

#define BAT_CHECK_PORT	GPIOA			// 电池电压检测端口
#define BAT_CHECK_PIN		GPIO_PIN_1	// 电池电压检测引脚（ADC 通道）

#define CHARGE_PORT			GPIOA			// 充电状态检测端口
#define CHARGE_PIN			GPIO_PIN_2	// 充电状态检测引脚

#define POWER_PORT			GPIOA			// 电源使能端口
#define POWER_PIN				GPIO_PIN_3	// 电源使能引脚（维持整机供电）

/** 初始化电源相关 GPIO（采样脚、检测脚、使能脚） */
void Power_Pins_Init(void);
/** 拉高电源使能脚，保持整机供电（松开按键后不掉电） */
void Power_Enable(void);
/** 拉低电源使能脚，切断整机供电（关机） */
void Power_DisEnable(void);
/** 单次读取电池电压，返回电压值（单位 V） */
float BatCheck(void);
/** 连续采样 8 次取平均，得到更稳定的电池电压（单位 V） */
float BatCheck_8times(void);
/** 检测是否处于充电状态；返回值见 power.c（0/1） */
uint8_t ChargeCheck(void);
/** 根据电池电压换算电量百分比；返回 0~100 */
uint8_t PowerCalculate(void);
/** 电源模块总初始化（引脚 + 首次采样） */
void Power_Init(void);

#endif
