/**
 * @file    sys.h
 * @brief   基础类型定义（STM32 标准库风格的别名）。
 *
 * 为兼容正点原子等常见代码风格，这里把 uint8_t/uint16_t/uint32_t 等
 * 定义成 u8/u16/u32，并提供了 volatile、const、只读等组合别名，
 * 供 BSP 和 USER 层代码统一使用。
 */
#ifndef __SYS_H__
#define __SYS_H__

#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"

#define OS_SUPPORT 	0	// 是否启用操作系统支持（0：裸机，1：RTOS）
#define SYS_CLK 		100	// 系统时钟频率（单位 MHz）

typedef int32_t  s32;	// 32 位有符号整型
typedef int16_t s16;	// 16 位有符号整型
typedef int8_t  s8;		// 8 位有符号整型

typedef const int32_t sc32;  // 只读 32 位有符号整型
typedef const int16_t sc16;  // 只读 16 位有符号整型
typedef const int8_t sc8;    // 只读 8 位有符号整型

typedef __IO int32_t  vs32;	// volatile 32 位有符号整型
typedef __IO int16_t  vs16;	// volatile 16 位有符号整型
typedef __IO int8_t   vs8;	// volatile 8 位有符号整型

typedef __I int32_t vsc32;  	// volatile 只读 32 位有符号整型
typedef __I int16_t vsc16;  	// volatile 只读 16 位有符号整型
typedef __I int8_t vsc8;   	// volatile 只读 8 位有符号整型

typedef uint32_t  u32;	// 32 位无符号整型
typedef uint16_t u16;	// 16 位无符号整型
typedef uint8_t  u8;	// 8 位无符号整型

typedef const uint32_t uc32;  // 只读 32 位无符号整型
typedef const uint16_t uc16;  // 只读 16 位无符号整型
typedef const uint8_t uc8;    // 只读 8 位无符号整型

typedef __IO uint32_t  vu32;	// volatile 32 位无符号整型
typedef __IO uint16_t vu16;		// volatile 16 位无符号整型
typedef __IO uint8_t  vu8;		// volatile 8 位无符号整型

typedef __I uint32_t vuc32;  	// volatile 只读 32 位无符号整型
typedef __I uint16_t vuc16;  	// volatile 只读 16 位无符号整型
typedef __I uint8_t vuc8;   	// volatile 只读 8 位无符号整型

#endif
