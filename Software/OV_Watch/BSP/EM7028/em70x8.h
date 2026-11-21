/**
 * @file    em70x8.h
 * @brief   EM7028 心率/血氧传感器驱动（I2C 接口）。
 *
 * 传感器包含两组光电采集通道（HRS1/HRS2）：
 *   - HRS1：心率采集通道，输出 4 组 16 位原始数据
 *   - HRS2：血氧采集通道，输出 4 组 16 位原始数据
 * 本文件定义寄存器地址及底层读写、初始化、数据获取接口。
 */
#ifndef __EM70X8_H__
#define __EM70X8_H__

#include "iic_hal.h"

#define EM7028_ID					0x36	// 芯片 ID 值（用于识别器件）
#define EM7028_ADDR				0x24	// I2C 从机地址

#define ID_REG						0x00	// 芯片 ID 寄存器
#define HRS_CFG						0x01	// 心率采集配置寄存器
#define HRS_INT_CTRL			0x02	// 心率中断控制寄存器
#define HRS_LT_L					0X03	// 心率阈值低字节（低 8 位）
#define HRS_LT_H					0x04	// 心率阈值高字节（高 8 位）
#define HRS_HT_L					0x05	// 心率上限阈值低字节
#define HRS_HT_H					0x06	// 心率上限阈值高字节
#define LED_CRT						0x07	// LED 驱动电流配置寄存器
#define	HRS2_DATA_OFFSET	0x08	// HRS2 数据偏移配置寄存器
#define HRS2_CTRL					0x09	// HRS2 通道控制寄存器
#define HRS2_GAIN_CTRL		0x0A	// HRS2 增益控制寄存器
#define HRS1_CTRL					0x0D	// HRS1 通道控制寄存器
#define INT_CTRL					0x0E	// 中断总控制寄存器
#define SOFT_RESET				0x0F	// 软复位寄存器

#define HRS2_DATA0_L		0x20	// HRS2 通道数据 0 低字节
#define HRS2_DATA0_H		0x21	// HRS2 通道数据 0 高字节
#define HRS2_DATA1_L		0x22	// HRS2 通道数据 1 低字节
#define HRS2_DATA1_H		0x23	// HRS2 通道数据 1 高字节
#define HRS2_DATA2_L		0x24	// HRS2 通道数据 2 低字节
#define HRS2_DATA2_H		0x25	// HRS2 通道数据 2 高字节
#define HRS2_DATA3_L		0x26	// HRS2 通道数据 3 低字节
#define HRS2_DATA3_H		0x27	// HRS2 通道数据 3 高字节

#define HRS1_DATA0_L		0x28	// HRS1 通道数据 0 低字节
#define HRS1_DATA0_H		0x29	// HRS1 通道数据 0 高字节
#define HRS1_DATA1_L		0x2A	// HRS1 通道数据 1 低字节
#define HRS1_DATA1_H		0x2B	// HRS1 通道数据 1 高字节
#define HRS1_DATA2_L		0x2C	// HRS1 通道数据 2 低字节
#define HRS1_DATA2_H		0x2D	// HRS1 通道数据 2 高字节
#define HRS1_DATA3_L		0x2E	// HRS1 通道数据 3 低字节
#define HRS1_DATA3_H		0x2F	// HRS1 通道数据 3 高字节

/** 读取一个寄存器，返回寄存器值 */
uint8_t  EM7028_ReadOneReg(unsigned char RegAddr);
/** 写一个寄存器 */
void  EM7028_WriteOneReg(unsigned char RegAddr, unsigned char dat);

/** 读取芯片 ID，用于检测传感器是否在线（应为 EM7028_ID） */
uint8_t EM7028_Get_ID(void);
/** 心率采集初始化（配置增益、LED 电流、中断等）；返回 0 表示成功 */
uint8_t EM7028_hrs_init(void);
/** 使能心率采集 */
uint8_t EM7028_hrs_Enable(void);
/** 关闭心率采集 */
uint8_t EM7028_hrs_DisEnable(void);
/** 读取 HRS1 通道最新一组数据（16 位原始值） */
uint16_t EM7028_Get_HRS1(void);

#endif
