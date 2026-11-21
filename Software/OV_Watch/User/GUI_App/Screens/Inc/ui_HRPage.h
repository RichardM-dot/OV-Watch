/**
 * @file    ui_HRPage.h
 * @brief   心率页面：显示心率数值、单位、动画图标与提示文字。
 */
#ifndef _UI_HRPAGE_H
#define _UI_HRPAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"

extern lv_obj_t * ui_HRPage;			// 心率页面的根对象
extern lv_obj_t * ui_HRPageNumLabel;	// 心率数值
extern lv_obj_t * ui_HRPageUnitLabel;	// 心率单位（bpm）
extern lv_obj_t * ui_HRPaggiconLabel;	// 心率动画图标
extern lv_obj_t * ui_HRPageNoticeLabel;	// 测量提示/状态文字

extern Page_t Page_HR;	// 心率页的页面描述

/** 创建心率页控件 */
void ui_HRPage_screen_init(void);
/** 销毁心率页控件 */
void ui_HRPage_screen_deinit(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
