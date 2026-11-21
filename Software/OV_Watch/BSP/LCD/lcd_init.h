/**
 * @file    lcd_init.h
 * @brief   ST7789 LCD 底层初始化（软件 SPI 时序 + 引脚定义）。
 *
 * 引脚全部占用 GPIOB：
 *   - PB3 : SCLK（时钟）
 *   - PB5 : SDA（MOSI 数据）
 *   - PB7 : RES（复位）
 *   - PB9 : DC（数据/命令选择）
 *   - PB8 : CS（片选）
 *   - PB0 : BLK（背光控制）
 * 该屏通过 SPI 接收像素数据，配合 lcd.c 的绘图函数使用。
 */
#ifndef __LCD_INIT_H
#define __LCD_INIT_H

#include "sys.h"

#define USE_HORIZONTAL 0  //设置横屏或者竖屏显示 0或1为竖屏 2或3为横屏


#if USE_HORIZONTAL==0||USE_HORIZONTAL==1
#define LCD_W 240	// 竖屏时的屏幕宽度（像素）
#define LCD_H 280	// 竖屏时的屏幕高度（像素）

#else
#define LCD_W 280	// 横屏时的屏幕宽度（像素）
#define LCD_H 240	// 横屏时的屏幕高度（像素）
#endif


//-----------------LCD端口定义---------------- 
#define SCLK_PORT			GPIOB			// SPI 时钟端口
#define SCLK_PIN			GPIO_PIN_3		// SPI 时钟引脚

#define SDA_PORT			GPIOB			// SPI 数据端口（MOSI）
#define SDA_PIN				GPIO_PIN_5		// SPI 数据引脚

#define RES_PORT			GPIOB			// 屏复位端口
#define RES_PIN				GPIO_PIN_7		// 屏复位引脚

#define DC_PORT				GPIOB			// 数据/命令选择端口
#define DC_PIN				GPIO_PIN_9		// 数据/命令选择引脚

#define CS_PORT				GPIOB			// 片选端口
#define CS_PIN				GPIO_PIN_8		// 片选引脚

#define BLK_PORT			GPIOB			// 背光控制端口
#define BLK_PIN				GPIO_PIN_0		// 背光控制引脚

#define LCD_SCLK_Clr() HAL_GPIO_WritePin(SCLK_PORT,SCLK_PIN,GPIO_PIN_RESET)//SCL=SCLK
#define LCD_SCLK_Set() HAL_GPIO_WritePin(SCLK_PORT,SCLK_PIN,GPIO_PIN_SET)

#define LCD_MOSI_Clr() HAL_GPIO_WritePin(SDA_PORT,SDA_PIN,GPIO_PIN_RESET)//SDA=MOSI
#define LCD_MOSI_Set() HAL_GPIO_WritePin(SDA_PORT,SDA_PIN,GPIO_PIN_SET)

#define LCD_RES_Clr()  HAL_GPIO_WritePin(RES_PORT,RES_PIN,GPIO_PIN_RESET)//RES
#define LCD_RES_Set()  HAL_GPIO_WritePin(RES_PORT,RES_PIN,GPIO_PIN_SET)

#define LCD_DC_Clr()   HAL_GPIO_WritePin(DC_PORT,DC_PIN,GPIO_PIN_RESET)//DC
#define LCD_DC_Set()   HAL_GPIO_WritePin(DC_PORT,DC_PIN,GPIO_PIN_SET)
 		     
#define LCD_CS_Clr()   HAL_GPIO_WritePin(CS_PORT,CS_PIN,GPIO_PIN_RESET)//CS
#define LCD_CS_Set()   HAL_GPIO_WritePin(CS_PORT,CS_PIN,GPIO_PIN_SET)

#define LCD_BLK_Clr()  HAL_GPIO_WritePin(BLK_PORT,BLK_PIN,GPIO_PIN_RESET)//BLK
#define LCD_BLK_Set()  HAL_GPIO_WritePin(BLK_PORT,BLK_PIN,GPIO_PIN_SET)

void LCD_GPIO_Init(void);//初始化GPIO
void LCD_Writ_Bus(u8 dat);//模拟SPI时序
void LCD_WR_DATA8(u8 dat);//写入一个字节
void LCD_WR_DATA(u16 dat);//写入两个字节
void LCD_WR_REG(u8 dat);//写入一个指令
void LCD_Address_Set(u16 x1,u16 y1,u16 x2,u16 y2);//设置坐标函数
void LCD_Init(void);//LCD初始化
void LCD_Set_Light(uint8_t dc);//设置背光亮度（PWM 占空比）
void LCD_Close_Light(void);//关闭背光
void LCD_ST7789_SleepIn(void);//屏进入休眠（低功耗）
void LCD_ST7789_SleepOut(void);//屏退出休眠
void LCD_Open_Light(void);//打开背光
#endif
