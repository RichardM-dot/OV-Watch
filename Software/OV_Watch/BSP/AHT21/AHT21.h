/**
 * @file    AHT21.h
 * @brief   AHT21 温湿度传感器驱动（I2C 接口）。
 *
 * 上电后需等待传感器内部校准完成（约 40ms 以上）再发起测量；
 * 每次测量前发送触发命令，等待转换完成后读取 6 字节原始数据并换算。
 */
#ifndef __AHT21_H
#define __AHT21_H

#include "stm32f4xx_hal.h"
#include "iic_hal.h"
#include "delay.h"

/** 读取传感器状态字节（bit3 为校准使能位，bit7 为忙标志） */
uint8_t AHT_Read_Status(void);
/** 软复位传感器（发送 0xBA 命令），用于异常恢复 */
void AHT_Reset(void);
/** 初始化 AHT21；返回 0 表示成功，非 0 表示失败（未检测到器件） */
uint8_t AHT_Init(void);
/** 触发一次测量并读取结果；humi 返回相对湿度(%RH)，temp 返回温度(℃) */
uint8_t AHT_Read(float *humi, float *temp);

#endif
