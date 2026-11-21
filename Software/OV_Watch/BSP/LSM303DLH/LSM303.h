/**
 * @file    LSM303.h
 * @brief   LSM303DLH 电子罗盘驱动（三轴加速度 + 三轴磁力计 + 温度）。
 *
 * 器件包含两个 I2C 从机地址：
 *   - 加速度计：0x19
 *   - 磁力计/温度：0x1E
 * 常用于计算指南针方位角（Azimuth）与俯仰/横滚角度。
 */
#ifndef _LSM303_H_
#define _LSM303_H_

#include "iic_hal.h"
 
//LSM303DLH 寄存器定义
//罗盘
#define	LSM303_CRA_REG_M					0x00	//RW 磁力计速率配置
#define	LSM303_CRB_REG_M					0x01	//RW 磁力计增益配置
#define	LSM303_MR_REG_M						0x02	//RW 磁力计工作模式
#define	LSM303_OUT_X_H_M					0x03	//R  磁力计 X 轴高字节
#define	LSM303_OUT_X_L_M					0x04	//R  磁力计 X 轴低字节
#define	LSM303_OUT_Z_H_M					0x05	//R  磁力计 Z 轴高字节
#define	LSM303_OUT_Z_L_M					0x06	//R  磁力计 Z 轴低字节
#define	LSM303_OUT_Y_H_M					0x07	//R  磁力计 Y 轴高字节
#define	LSM303_OUT_Y_L_M					0x08	//R  磁力计 Y 轴低字节
#define	LSM303_SR_REG_M						0x09	//R  磁力计状态寄存器
#define	LSM303_IRA_REG_M					0x0A	//R  器件识别寄存器 A
#define	LSM303_IRB_REG_M					0x0B	//R  器件识别寄存器 B
#define	LSM303_IRC_REG_M					0x0C	//R  器件识别寄存器 C
	
//加速度
#define	LSM303_CTRL_REG1_A					0x20	//RW 加速度计控制寄存器 1（速率/使能）
#define	LSM303_CTRL_REG2_A					0x21	//RW 加速度计控制寄存器 2（滤波）
#define	LSM303_CTRL_REG3_A					0x22	//RW 加速度计控制寄存器 3（中断）
#define	LSM303_CTRL_REG4_A					0x23	//RW 加速度计控制寄存器 4（量程）
#define	LSM303_CTRL_REG5_A					0x24	//RW 加速度计控制寄存器 5
#define	LSM303_HP_FILTER_RESET_A		0x25	//R  高通滤波复位
#define	LSM303_REFERENCE_A					0x26	//RW 参考值
#define	LSM303_STATUS_REG_A					0x27	//R  加速度计状态寄存器
#define	LSM303_OUT_X_L_A						0x28	//R  加速度 X 轴低字节
#define	LSM303_OUT_X_H_A						0x29	//R  加速度 X 轴高字节
#define	LSM303_OUT_Y_L_A						0x2A	//R  加速度 Y 轴低字节
#define	LSM303_OUT_Y_H_A						0x2B	//R  加速度 Y 轴高字节
#define	LSM303_OUT_Z_L_A						0x2C	//R  加速度 Z 轴低字节
#define	LSM303_OUT_Z_H_A						0x2D	//R  加速度 Z 轴高字节
#define	LSM303_INT1_CFG_A						0x30	//RW 中断 1 配置
#define	LSM303_INT1_SOURCE_A				0x31	//R  中断 1 源
#define	LSM303_INT1_THS_A						0x32	//RW 中断 1 阈值
#define	LSM303_INT1_DURATION_A			0x33	//RW 中断 1 持续时间
#define	LSM303_INT2_CFG_A						0x34	//RW 中断 2 配置
#define	LSM303_INT2_SOURCE_A				0x35	//R  中断 2 源
#define	LSM303_INT2_THS_A						0x36	//RW 中断 2 阈值
#define	LSM303_INT2_DURATION_A			0x37	//RW 中断 2 持续时间

//温度
#define TEMP_OUT_H_M								0x31	//R  温度高字节
#define TEMP_OUT_L_M								0x32	//R  温度低字节

#define LSM303_SlaveAddr_A					0x19	//加速度地址
#define LSM303_SlaveAddr_M					0x1E	//磁力计和温度的地址			

//functions
/** 读加速度计/磁力计一个寄存器，返回寄存器值 */
unsigned char LSM303_ReadOneReg(unsigned char RegAddr);
/** 从 RegAddr 起连续读 RegNum 个寄存器到 DataBuff */
void LSM303_ReadMultiReg(unsigned char RegAddr, unsigned char RegNum, unsigned char DataBuff[]);
/** 读温度寄存器（温度挂在磁力计地址下） */
unsigned char LSM303_Temp_ReadOneReg(unsigned char RegAddr);
/** 写一个寄存器 */
void LSM303_WriteOneReg(unsigned char RegAddr, unsigned char dat);

/** 初始化 LSM303DLH（配置量程、速率、连续测量模式）；返回 0 表示成功 */
unsigned char LSM303DLH_Init(void);

/** 让传感器进入休眠 */
void LSM303DLH_Sleep(void);
/** 唤醒传感器 */
void LSM303DLH_Wakeup(void);

/** 读取三轴加速度原始值 */
void LSM303_ReadAcceleration(int16_t *Xa, int16_t *Ya, int16_t *Za);
/** 读取三轴磁力原始值 */
void LSM303_ReadMagnetic(int16_t *Xm, int16_t *Ym, int16_t *Zm);

/** 由加速度计算 Z 轴倾角（单位：度） */
int LSM303DLH_CalculationZAxisAngle(int16_t Xa, int16_t Ya, int16_t Za);
/** 由加速度计算 X 轴倾角（单位：度） */
int LSM303DLH_CalculationXAxisAngle(int16_t Xa, int16_t Ya, int16_t Za);
/** 读取温度原始值 */
void LSM303_ReadTemperature(int16_t *Temp);
/** 由加速度与磁力数据计算方位角（0~360 度） */
float Azimuth_Calculate(int16_t Xa, int16_t Ya, int16_t Za, int16_t Xm, int16_t Ym, int16_t Zm);

#endif /*_LSM303DLH_H_*/
