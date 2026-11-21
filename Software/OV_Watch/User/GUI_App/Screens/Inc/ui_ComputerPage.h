/**
 * @file    ui_ComputerPage.h
 * @brief   计算器页面：文本框 + 按键矩阵 + 返回键。
 */
#ifndef _UI_COMPUTERPAGE_H
#define _UI_COMPUTERPAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../ui.h"

extern lv_obj_t * ui_Computerpage;		// 计算器页面的根对象
extern lv_obj_t * ui_CompageBtnM;		// 按键矩阵（lv_btnmatrix）
extern lv_obj_t * ui_CompageTextarea;	// 表达式/结果显示框
extern lv_obj_t * ui_CompageBackBtn;	// 返回按钮

extern Page_t Page_Computer;	// 计算器页的页面描述

/** 创建计算器页控件并绑定按键事件 */
void ui_ComputerPage_screen_init(void);
/** 销毁计算器页控件 */
void ui_ComputerPage_screen_deinit(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
