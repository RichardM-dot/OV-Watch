/**
 * @file    ui_TimerPage.h
 * @brief   秒表/计时器页面。
 */
#ifndef _UI_TIMERPAGE_H
#define _UI_TIMERPAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../ui.h"

extern lv_obj_t * ui_TimerPage;	// 计时器页面的根对象

extern uint8_t ui_TimerPageFlag;			// 计时器运行标志：1 运行中，0 停止
extern uint32_t ui_TimerPage_elapsed_ms;	// 已计时时长（单位：毫秒）

extern Page_t Page_Timer;	// 计时器页的页面描述

/** 创建计时器页控件并绑定开始/暂停/复位事件 */
void ui_TimerPage_screen_init(void);
/** 销毁计时器页控件 */
void ui_TimerPage_screen_deinit(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
