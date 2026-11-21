/**
 * @file    BL24C02.h
 * @brief   BL24C02 EEPROM 驱动（I2C 接口，容量 2Kbit = 256 字节）。
 *
 * 用于掉电保存用户设置（如抬腕开关、屏幕亮度、闹钟等）。
 * 写操作后 EEPROM 需要约 5ms 的内部写周期，连续写时需留出间隔。
 */
#ifndef __BL24C02_H
#define __BL24C02_H

#include "stm32f4xx_hal.h"
#include "iic_hal.h"
#include "delay.h"

#define BL_ADDRESS	0x50	// EEPROM 7 位 I2C 从机地址（A2/A1/A0 接地）

/** 从内部地址 addr 起写入 length 字节 */
void BL24C02_Write(uint8_t addr,uint8_t length,uint8_t buff[]);
/** 从内部地址 addr 起读出 length 字节到 buff */
void BL24C02_Read(uint8_t addr, uint8_t length, uint8_t buff[]);
/** 初始化 EEPROM 所在 I2C 总线 */
void BL24C02_Init(void);

#endif
