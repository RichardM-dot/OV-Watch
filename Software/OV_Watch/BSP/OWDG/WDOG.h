/**
 * @file    WDOG.h
 * @brief   独立看门狗（IWDG）驱动接口。
 *
 * 系统正常运行时应周期性调用 WDOG_Feed() 喂狗；
 * 若程序跑飞导致长时间未喂狗，看门狗将复位 MCU。
 */
#ifndef __WDOG_H__
#define __WDOG_H__

#ifdef __cplusplus
extern "C" {
#endif

/** 初始化看门狗相关端口/外设（配置 IWDG 预分频与重装载值） */
void WDOG_Port_Init(void);
/** 启动看门狗 */
void WDOG_Enable(void);
/** 关闭看门狗（注意：IWDG 一旦启动通常无法关闭，仅硬件看门狗使能脚可用） */
void WDOG_Disnable(void);
/** 喂狗：重装载计数器，防止复位 */
void WDOG_Feed(void);


#ifdef __cplusplus
}
#endif
#endif

