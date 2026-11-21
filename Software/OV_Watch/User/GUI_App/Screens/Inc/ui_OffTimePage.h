/**
 * @file    ui_OffTimePage.h
 * @brief   熄屏时间设置页面（背光/息屏延时）。
 *
 * 包含两个页面：
 *   - ui_LOffTimeSetPage：背光（Light）自动关闭时间
 *   - ui_TOffTimeSetPage：整机（Time）自动熄屏时间
 * 两个页面结构相同，均为滚轮 + 确定按钮。
 */
#ifndef _UI_OFFTIMEPAGE_H
#define _UI_OFFTIMEPAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"

extern lv_obj_t * ui_LOffTimeSetPage;	// 背光关闭时间设置页根对象
extern lv_obj_t * ui_LTimeSetRoller;	// 背光时间滚轮
extern lv_obj_t * ui_LTimeSetLabel;		// 背光时间说明标签
extern lv_obj_t * ui_LTimeSetOKButton;	// 背光时间确定按钮
extern lv_obj_t * ui_LTimeSetOKLabel;	// 背光时间确定按钮文字
extern lv_obj_t * ui_LTimeSeticon;		// 背光时间确定按钮图标

extern lv_obj_t * ui_TOffTimeSetPage;	// 熄屏时间设置页根对象
extern lv_obj_t * ui_TTimeSetRoller;	// 熄屏时间滚轮
extern lv_obj_t * ui_TTimeSetLabel;		// 熄屏时间说明标签
extern lv_obj_t * ui_TTimeSetOKButton;	// 熄屏时间确定按钮
extern lv_obj_t * ui_TTimeSetOKLabel;	// 熄屏时间确定按钮文字
extern lv_obj_t * ui_TTimeSeticon;		// 熄屏时间确定按钮图标

extern const uint8_t ui_LTimeOptions[6];	// 背光时间可选值（秒/档位）
extern const uint8_t ui_TTimeOptions[6];	// 熄屏时间可选值（秒/档位）
extern uint8_t ui_LTimeValue;				// 当前选中的背光时间
extern uint8_t ui_TTimeValue;				// 当前选中的熄屏时间

extern Page_t Page_LOffTimeSet;	// 背光时间设置页描述
extern Page_t Page_TOffTimeSet;	// 熄屏时间设置页描述

/** 创建背光时间设置页控件 */
void ui_LOffTimeSetPage_screen_init(void);
/** 创建熄屏时间设置页控件 */
void ui_TOffTimeSetPage_screen_init(void);

/** 销毁背光时间设置页控件 */
void ui_LOffTimeSetPage_screen_deinit(void);
/** 销毁熄屏时间设置页控件 */
void ui_TOffTimeSetPage_screen_deinit(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
