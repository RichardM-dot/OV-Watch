/**
 * @file    ui_DateTimeSetPage.h
 * @brief   日期/时间设置相关页面。
 *
 * 本文件包含三个页面：
 *   - ui_DateTimeSetPage：设置总页（开关 APP 同步、进入日期/时间设置）
 *   - ui_DateSetPage：日期设置（年/月/日滚轮 + 确定）
 *   - ui_TimeSetPage：时间设置（时/分/秒滚轮 + 确定）
 */
#ifndef _DATEDAYSETPAGE_UI_H
#define _DATEDAYSETPAGE_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"
#include "../../ui.h"

extern lv_obj_t * ui_DateTimeSetPage;	// 日期时间设置总页根对象
extern lv_obj_t * ui_APPSyPanel;		// APP 同步开关所在面板
extern lv_obj_t * ui_APPSySwitch;		// APP 同步开关
extern lv_obj_t * ui_APPSyLabel;		// APP 同步文字
extern lv_obj_t * ui_DateSetPanel;		// "日期设置"入口面板
extern lv_obj_t * ui_DateSetLabel;		// 日期设置标题
extern lv_obj_t * ui_DateSetLabel1;		// 日期设置辅助标签 1（年）
extern lv_obj_t * ui_DateSetLabel2;		// 日期设置辅助标签 2（月）
extern lv_obj_t * ui_DateSetLabel3;		// 日期设置辅助标签 3（日）
extern lv_obj_t * ui_TimeSetPanel;		// "时间设置"入口面板
extern lv_obj_t * ui_TimeSetLabel;		// 时间设置标题
extern lv_obj_t * ui_TimeSetLabel1;		// 时间设置辅助标签 1（时）
extern lv_obj_t * ui_TimeSetLabel2;		// 时间设置辅助标签 2（分）
extern lv_obj_t * ui_TimeSetLabel3;		// 时间设置辅助标签 3（秒）
extern lv_obj_t * ui_DateSetPage;		// 日期设置页根对象
extern lv_obj_t * ui_YearSetRoller;		// 年份滚轮（2000~2099）
extern lv_obj_t * ui_MonthSetRoller;	// 月份滚轮（1~12）
extern lv_obj_t * ui_DaySetRoller;		// 日期滚轮（1~31，按月份动态调整）
extern lv_obj_t * ui_DateSetOKButton;	// 日期设置确定按钮
extern lv_obj_t * ui_DateSetOKicon;		// 日期设置确定按钮图标
extern lv_obj_t * ui_TimeSetPage;		// 时间设置页根对象
extern lv_obj_t * ui_HourSetRoller;		// 小时滚轮（0~23）
extern lv_obj_t * ui_TimePoint;			// 时分分隔冒号
extern lv_obj_t * ui_MinSetRoller;		// 分钟滚轮（0~59）
extern lv_obj_t * ui_TimePoint1;		// 分秒分隔冒号
extern lv_obj_t * ui_SecSetRoller;		// 秒滚轮（0~59）
extern lv_obj_t * ui_TimeSetOKButton;	// 时间设置确定按钮
extern lv_obj_t * ui_TimeSetOKicon;		// 时间设置确定按钮图标

extern Page_t Page_DateTimeSet;	// 日期时间设置总页描述
extern Page_t Page_DateSet;		// 日期设置页描述
extern Page_t Page_TimeSet;		// 时间设置页描述

/** 创建日期时间设置总页控件 */
void ui_DateTimeSetPage_screen_init(void);
/** 创建日期设置页控件 */
void ui_DateSetPage_screen_init(void);
/** 创建时间设置页控件 */
void ui_TimeSetPage_screen_init(void);

/** 销毁日期时间设置总页控件 */
void ui_DateTimeSetPage_screen_deinit(void);
/** 销毁日期设置页控件 */
void ui_DateSetPage_screen_deinit(void);
/** 销毁时间设置页控件 */
void ui_TimeSetPage_screen_deinit(void);

extern uint8_t ui_APPSy_EN;	// APP 时间同步开关状态：1 开启，0 关闭

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
