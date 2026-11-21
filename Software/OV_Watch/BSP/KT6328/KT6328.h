/**
 * @file    KT6328.h
 * @brief   KT6328 充电管理芯片控制接口。
 *
 * 通过 GPIO 控制充电芯片的使能/关闭，配合 power 模块完成充电管理。
 */
#ifndef __KT6328_H__
#define __KT6328_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/** 初始化 KT6328 控制引脚（推挽输出） */
void KT6328_GPIO_Init(void);
/** 使能充电（允许 KT6328 给电池充电） */
void KT6328_Enable(void);
/** 关闭充电 */
void KT6328_Disable(void);


#ifdef __cplusplus
}
#endif
#endif

