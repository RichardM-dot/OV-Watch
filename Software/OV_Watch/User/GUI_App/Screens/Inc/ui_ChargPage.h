/**
 * @file    ui_ChargPage.h
 * @brief   充电页面：显示电池电量圆弧、电池图标、电量数字与当前时间。
 */
#ifndef _UI_CHARGPAGE_H
#define _UI_CHARGPAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"

extern lv_obj_t * ui_ChargPage;			// 充电页面的根对象
extern lv_obj_t * ui_CharPageBatArc;	// 电量圆弧（进度条）
extern lv_obj_t * ui_ChargPageBatIcon;	// 电池图标
extern lv_obj_t * ui_ChargPageBatNum;	// 电量百分比数字
extern lv_obj_t * ui_ChargPagebHourLabel;	// 时间：小时
extern lv_obj_t * ui_ChargPagebMinLabel;	// 时间：分钟
extern lv_obj_t * ui_ChargPageTimePoint;	// 时间分隔冒号

extern Page_t Page_Charg;	// 充电页的页面描述

/** 创建充电页控件 */
void ui_ChargPage_screen_init(void);
/** 销毁充电页控件 */
void ui_ChargPage_screen_deinit(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
