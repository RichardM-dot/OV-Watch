/**
 * @file    CST816.h
 * @brief   CST816 电容触摸屏驱动（I2C 接口）。
 *
 * 硬件连接：
 *   - 复位脚 RST：PA15（低电平复位）
 *   - 中断脚 INT：PB1（有触摸时输出脉冲）
 * 支持读取触点坐标、手指数量、手势识别结果，并可配置低功耗扫描/自动休眠参数。
 */
#ifndef __CST816_H
#define __CST816_H

#include "stdint.h"
#include "iic_hal.h"
#include "delay.h"

/* CST816 dev pin defination */


#define TOUCH_RST_PIN GPIO_PIN_15	// 触摸复位引脚号
#define TOUCH_INT_PIN GPIO_PIN_1	// 触摸中断引脚号
#define TOUCH_RST_PORT GPIOA		// 触摸复位引脚端口
#define TOUCH_INT_PORT GPIOB		// 触摸中断引脚端口

/* functions define */
#define TOUCH_RST_0 HAL_GPIO_WritePin(TOUCH_RST_PORT, TOUCH_RST_PIN, GPIO_PIN_RESET)	// 拉低复位脚（进入复位）
#define TOUCH_RST_1 HAL_GPIO_WritePin(TOUCH_RST_PORT, TOUCH_RST_PIN, GPIO_PIN_SET)	// 拉高复位脚（释放复位）

/* 设备地址 */
#define Device_Addr 	0x15	// CST816 的 I2C 从机地址

/* 触摸屏寄存器 */
#define GestureID 			0x01	// 手势 ID
#define FingerNum 			0x02	// 当前触点数
#define XposH 					0x03	// 触点 X 坐标高 4 位
#define XposL 					0x04	// 触点 X 坐标低 8 位
#define YposH 					0x05	// 触点 Y 坐标高 4 位
#define YposL 					0x06	// 触点 Y 坐标低 8 位
#define ChipID 					0xA7	// 芯片 ID 寄存器
#define SleepMode				0xE5	// 休眠模式寄存器
#define MotionMask 			0xEC	// 连续动作（手势）使能掩码
#define IrqPluseWidth 	0xED	// 中断脉冲宽度配置
#define NorScanPer 			0xEE	// 正常扫描周期配置
#define MotionSlAngle 	0xEF	// 滑动/手势角度阈值配置
#define LpAutoWakeTime 	0xF4	// 低功耗自动唤醒时间
#define LpScanTH 				0xF5	// 低功耗扫描阈值
#define LpScanWin 			0xF6	// 低功耗扫描窗口
#define LpScanFreq 			0xF7	// 低功耗扫描频率
#define LpScanIdac 			0xF8	// 低功耗扫描驱动电流
#define AutoSleepTime 	0xF9	// 自动休眠时间
#define IrqCtl 					0xFA	// 中断触发方式控制
#define AutoReset 			0xFB	// 无触摸自动复位时间
#define LongPressTime 	0xFC	// 长按判定时间
#define IOCtl 					0xFD	// IO 口控制（中断脚模式等）
#define DisAutoSleep 		0xFE	// 关闭自动休眠

/* 触摸屏坐标结构体 */
typedef struct
{
	unsigned int X_Pos;	// 触点 X 坐标（像素）
	unsigned int Y_Pos;	// 触点 Y 坐标（像素）
} CST816_Info;

/* 手势ID识别选项 */
typedef enum
{
	NOGESTURE = 	0x00,	// 无手势
	DOWNGLIDE = 	0x01,	// 向下滑动
	UPGLIDE = 		0x02,	// 向上滑动
	LEFTGLIDE = 	0x03,	// 向左滑动
	RIGHTGLIDE = 	0x04,	// 向右滑动
	CLICK = 			0x05,	// 单击
	DOUBLECLICK = 0x0B,	// 双击
	LONGPRESS = 	0x0C,	// 长按
} GestureID_TypeDef;

/* 连续动作配置选项 */
typedef enum
{
	M_DISABLE = 	0x00,	// 关闭连续动作检测
	EnConLR = 		0x01,	// 使能左右连续滑动
	EnConUD = 		0x02,	// 使能上下连续滑动
	EnDClick = 		0x03,	// 使能双击
	M_ALLENABLE = 0x07,	// 全部使能
} MotionMask_TypeDef;

/* 中断低脉冲发射方式选项 */
typedef enum
{
	OnceWLP = 		0x00,	// 仅发一次低脉冲
	EnMotion = 		0x10,	// 动作触发时发中断
	EnChange = 		0x20,	// 状态变化时发中断
	EnTouch = 		0x40,	// 触摸时发中断
	EnTest = 			0x80,	// 测试模式中断
} IrqCtl_TypeDef;

/* 触摸屏初始化相关函数 */
/** 初始化触摸屏 GPIO（RST 输出、INT 输入） */
void CST816_GPIO_Init(void);
/** 硬件复位触摸屏（拉低 RST 再释放，等待启动） */
void CST816_RESET(void);
/** 触摸屏完整初始化（GPIO + 复位 + 参数配置） */
void CST816_Init(void);

/* 触摸屏操作函数 */
/** 读取当前触点坐标，结果存入内部 CST816_Info 结构 */
void CST816_Get_XY_AXIS(void);
/** 读取芯片 ID，用于检测触摸屏是否在线 */
uint8_t CST816_Get_ChipID(void);
/** 读取当前触点数 */
uint8_t CST816_Get_FingerNum(void);
/* 触摸屏读写函数 */
/** 向触摸屏寄存器 addr 写 dat */
void CST816_IIC_WriteREG(uint8_t addr, uint8_t dat);
/** 读触摸屏寄存器 addr，返回寄存器值 */
uint8_t CST816_IIC_ReadREG(unsigned char addr);

/* 触摸屏有关参数配置函数 */
/** 配置连续动作/手势使能掩码 */
void CST816_Config_MotionMask(uint8_t mode);
/** 配置无操作后自动休眠的时间 */
void CST816_Config_AutoSleepTime(uint8_t time);
/** 配置手势滑动角度阈值（x_right_y_up_angle，单位度） */
void CST816_Config_MotionSlAngle(uint8_t x_right_y_up_angle);
/** 配置正常模式扫描周期 */
void CST816_Config_NorScanPer(uint8_t Period);
/** 配置中断脉冲宽度 */
void CST816_Config_IrqPluseWidth(uint8_t Width);
/** 配置低功耗扫描阈值 */
void CST816_Config_LpScanTH(uint8_t TH);
/** 唤醒触摸屏（退出休眠） */
void CST816_Wakeup(void);
/** 让触摸屏进入休眠以降低功耗 */
void CST816_Sleep(void);

#endif
