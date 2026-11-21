/**
 * @file    SPL06_001.h
 * @brief   SPL06-001 气压计驱动（I2C 接口），用于海拔与气压测量。
 *
 * 传感器内部带出厂校准系数，读取原始气压/温度后需用这些系数做补偿换算。
 * I2C 从机地址为 0x76，芯片 ID 为 0x10。
 */
#ifndef _SPL06_001_H_
#define _SPL06_001_H_

#include "iic_hal.h"

//your own settings
//see the table in the datasheet
#define KT							524288.0		// 温度补偿比例系数（来自数据手册）
#define KP							1572864.0		// 气压补偿比例系数（来自数据手册）

#define SPL_CHIP_ID					0X10		// 芯片 ID 值
#define SPL_CHIP_ADDRESS		0x76		// I2C 从机地址
//register map
//pressure data
#define SPL_PRS_B2					0x00		// 气压原始数据最高字节
#define SPL_PRS_B1					0x01		// 气压原始数据中间字节
#define SPL_PRS_B0					0x02		// 气压原始数据最低字节
//temperature data
#define SPL_TMP_B2					0x03		// 温度原始数据最高字节
#define SPL_TMP_B1					0x04		// 温度原始数据中间字节
#define SPL_TMP_B0					0x05		// 温度原始数据最低字节
//Config
#define SPL_PRS_CFG					0x06		// 气压测量配置（速率/过采样）
#define SPL_TMP_CFG					0x07		// 温度测量配置（内外传感器/速率/过采样）
#define SPL_MEAS_CFG				0x08		// 测量控制（启动单次/连续测量）
#define SPL_CFG_REG					0x09		// 全局配置（中断/FIFO/移位）
#define SPL_INT_STS					0x0A		// 中断状态
#define SPL_FIFO_STS				0x0B		// FIFO 状态
#define SPL_RESET_REG				0x0C		// 软复位
//ID
#define SPL_ID_REG					0x0D		// 芯片 ID 寄存器
//Calibration Coefficients
#define COEF_C0						0x10		// 校准系数 C0
#define COEF_C0_C1				0x11		// 校准系数 C0/C1 拼接字节
#define COEF_C1						0x12		// 校准系数 C1
#define COEF_C00_H				0x13		// 校准系数 C00 高字节
#define COEF_C00_L				0x14		// 校准系数 C00 低字节
#define COEF_C00_C10			0x15		// 校准系数 C00/C10 拼接字节
#define COEF_C10_M				0x16		// 校准系数 C10 中间字节
#define COEF_C10_L				0x17		// 校准系数 C10 低字节
#define COEF_C01_H				0x18		// 校准系数 C01 高字节
#define COEF_C01_L				0x19		// 校准系数 C01 低字节
#define COEF_C11_H				0x1A		// 校准系数 C11 高字节
#define COEF_C11_L				0x1B		// 校准系数 C11 低字节
#define COEF_C20_H				0x1C		// 校准系数 C20 高字节
#define COEF_C20_L				0x1D		// 校准系数 C20 低字节
#define COEF_C21_H				0x1E		// 校准系数 C21 高字节
#define COEF_C21_L				0x1F		// 校准系数 C21 低字节
#define COEF_C30_H				0x20		// 校准系数 C30 高字节
#define COEF_C30_L				0x21		// 校准系数 C30 低字节

//pressure config(6:4)
#define PM_RATE1				0x00		//1   measurements pr sec	每秒测量 1 次
#define PM_RATE2				0x01		//2   measurements pr sec	每秒测量 2 次
#define PM_RATE3				0x02		//4   measurements pr sec	每秒测量 4 次
#define PM_RATE4				0x03		//8   measurements pr sec	每秒测量 8 次
#define PM_RATE5				0x04		//16  measurements pr sec	每秒测量 16 次
#define PM_RATE6				0x05		//32  measurements pr sec	每秒测量 32 次
#define PM_RATE7				0x06		//64  measurements pr sec	每秒测量 64 次
#define PM_RATE8				0x07		//128 measurements pr sec	每秒测量 128 次
//pressure config(3:0)
//oversampling rate
#define	PM_PRC1					0x00		//single	单次采样
#define	PM_PRC2					0x01		//2   times (low power) 	2 倍过采样
#define	PM_PRC3					0x02		//4   times 	4 倍过采样
#define	PM_PRC4					0x03		//8   times (standard)	8 倍过采样（标准）
#define	PM_PRC5					0x04		//16  times	(need a bit shift in CFG_CFG)	16 倍
#define	PM_PRC6					0x05		//32  times	(need a bit shift in CFG_CFG)	32 倍
#define	PM_PRC7					0x06		//64  times	(high precision)(need a bit shift in CFG_CFG)	64 倍（高精度）
#define	PM_PRC8					0x07		//128 times	(need a bit shift in CFG_CFG)	128 倍
//temperature config
//extrenal or internal(7)
#define TMP_EXT 0x01	// 使用外部温度传感器
#define TMP_INT 0x00	// 使用内部温度传感器
//Temperature measurement rate(6:4)
#define TMP_RATE1				0x00		//1   measurements pr sec	每秒测量 1 次
#define TMP_RATE2				0x01		//2   measurements pr sec	每秒测量 2 次
#define TMP_RATE3				0x02		//4   measurements pr sec	每秒测量 4 次
#define TMP_RATE4				0x03		//8   measurements pr sec	每秒测量 8 次
#define TMP_RATE5				0x04		//16  measurements pr sec	每秒测量 16 次
#define TMP_RATE6				0x05		//32  measurements pr sec	每秒测量 32 次
#define TMP_RATE7				0x06		//64  measurements pr sec	每秒测量 64 次
#define TMP_RATE8				0x07		//128 measurements pr sec	每秒测量 128 次
//Temperature oversampling(3:0)
#define	TMP_PRC1				0x00		//single	单次采样
#define	TMP_PRC2				0x01		//2   times	2 倍过采样
#define	TMP_PRC3				0x02		//4   times 	4 倍过采样
#define	TMP_PRC4				0x03		//8   times	8 倍过采样
#define	TMP_PRC5				0x04		//16  times	16 倍过采样
#define	TMP_PRC6				0x05		//32  times	32 倍过采样
#define	TMP_PRC7				0x06		//64  times	64 倍过采样
#define	TMP_PRC8				0x07		//128 times	128 倍过采样

/** 读一个寄存器，返回寄存器值 */
uint8_t SPL_ReadOneReg(uint8_t addr);
/** 写一个寄存器 */
void SPL_WriteOneReg(uint8_t addr, uint8_t dat);

/** 初始化气压计（复位、读校准系数、配置测量参数）；返回 0 表示成功 */
uint8_t SPL_init(void);
/** 让气压计进入休眠 */
void SPL_Sleep(void);
/** 唤醒气压计 */
void SPL_Wakeup(void);
/** 读取芯片 ID（应为 SPL_CHIP_ID），用于检测器件是否在线 */
uint8_t SPL_GetID(void);

/** 计算并返回当前海拔高度（单位：米） */
float Altitude_Calculate(void);
/** 计算并返回当前气压（单位：Pa） */
float Pressure_Calculate(void);
/** 计算并返回当前温度（单位：℃） */
float Temperature_Calculate(void);

#endif
