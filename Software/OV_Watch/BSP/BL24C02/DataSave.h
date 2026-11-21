/**
 * @file    DataSave.h
 * @brief   设置项持久化：在 EEPROM 上按"参数 ID + 数据"方式存取配置。
 *
 * 上层通过 SettingSave/SettingGet 按地址读写一段数据，
 * EEPROM_Check 用于校验存储区是否已初始化（首次上电写入默认值）。
 */
#ifndef __DATASAVE_H__
#define __DATASAVE_H__

#include "BL24C02.h"

/** 初始化 EEPROM 存储模块（底层总线初始化 + 校验） */
void EEPROM_Init(void);
/** 校验 EEPROM 中的配置是否有效；返回 0 表示有效，非 0 表示需要写默认值 */
uint8_t EEPROM_Check(void);
/** 把 buf 中 lenth 字节保存到 EEPROM 地址 addr；返回 0 表示成功 */
uint8_t SettingSave(uint8_t *buf, uint8_t addr, uint8_t lenth);
/** 从 EEPROM 地址 addr 读取 lenth 字节到 buf；返回 0 表示成功 */
uint8_t SettingGet(uint8_t *buf, uint8_t addr, uint8_t lenth);

#endif
