/**
 * @file    ui_HomePage.h
 * @brief   主页（表盘）页面。
 *
 * 显示时间、日期、星期、电量、步数、温度、湿度、心率，
 * 以及下拉面板中的 NFC / 蓝牙 / 关机 / 设置入口和亮度滑块。
 * 这是手表最常用、也是默认显示的页面。
 */
#ifndef _UI_HOMEPAGE_H
#define _UI_HOMEPAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../ui.h"

extern lv_obj_t * ui_HomePage;			// 主页的根对象
extern lv_obj_t * ui_TimeHourLabel;		// 时间：小时
extern lv_obj_t * ui_TimeColonLabel;	// 时间分隔冒号
extern lv_obj_t * ui_TimeMinuteLabel;	// 时间：分钟
extern lv_obj_t * ui_BatArc;			// 电量圆弧
extern lv_obj_t * ui_BaticonLabel;		// 电池图标
extern lv_obj_t * ui_BatNumLabel;		// 电量百分比数字
extern lv_obj_t * ui_DateLabel;			// 日期（月/日）
extern lv_obj_t * ui_DayLabel;			// 星期
extern lv_obj_t * ui_StepiconLabel;		// 步数图标
extern lv_obj_t * ui_StepCnLabel;		// "步"字标签
extern lv_obj_t * ui_StepNumLabel;		// 步数数值
extern lv_obj_t * ui_StepNumBar;		// 步数进度条（相对目标步数）
extern lv_obj_t * ui_TempArc;			// 温度圆弧装饰
extern lv_obj_t * ui_TempiconLabel;		// 温度图标
extern lv_obj_t * ui_TempNumLabel;		// 温度数值
extern lv_obj_t * ui_HumiiconLabel;		// 湿度图标
extern lv_obj_t * ui_HumiArc;			// 湿度圆弧装饰
extern lv_obj_t * ui_HumiNumLabel;		// 湿度数值
extern lv_obj_t * ui_HRiconLabel;		// 心率图标
extern lv_obj_t * ui_HRSpinner;			// 心率测量动画（旋转指示器）
extern lv_obj_t * ui_HRNumLabel;		// 心率数值

extern lv_obj_t * ui_DropDownPanel;		// 下拉面板容器（上拉手势展开）
extern lv_obj_t * ui_UpBGPanel;			// 下拉面板上半部分背景
extern lv_obj_t * ui_NFCButton;			// NFC 按钮
extern lv_obj_t * ui_NFCLabel;			// NFC 按钮文字
extern lv_obj_t * ui_BLEButton;			// 蓝牙按钮
extern lv_obj_t * ui_BLELabel;			// 蓝牙按钮文字
extern lv_obj_t * ui_PowerButton;		// 关机按钮
extern lv_obj_t * ui_PowerLabel;		// 关机按钮文字
extern lv_obj_t * ui_SetButton;			// 设置按钮
extern lv_obj_t * ui_SetLabel;			// 设置按钮文字
extern lv_obj_t * ui_LightSlider;		// 背光亮度滑块
extern lv_obj_t * ui_LightLabel;		// 亮度标签
extern lv_obj_t * ui_DownBGPanel;		// 下拉面板下半部分背景

extern uint8_t ui_TimeHourValue;		// 当前小时（用于界面刷新）
extern uint8_t ui_TimeMinuteValue;		// 当前分钟
extern uint8_t ui_DateMonthValue;		// 当前月份
extern uint8_t ui_DateDayValue;			// 当前日
extern uint8_t ui_DataWeekdayValue;		// 当前星期（0~6）
extern const char * ui_Days[7];			// 星期文字表（"MON"~"SUN"）

extern uint8_t ui_BatArcValue;			// 电量圆弧当前值（0~100）
extern uint16_t ui_StepNumValue;		// 当前步数
extern uint8_t ui_LightSliderValue;		// 背光亮度当前值（0~100）

extern Page_t Page_Home;	// 主页的页面描述

/** 下拉面板事件：展开/收起下拉菜单 */
void ui_event_HomePage_DropDownPanel(lv_event_t * e);
/** NFC 按钮事件：进入 NFC 卡页面 */
void ui_event_NFCButton(lv_event_t * e);
/** 蓝牙按钮事件：开/关蓝牙 */
void ui_event_BLEButton(lv_event_t * e);
/** 关机按钮事件：进入关机流程 */
void ui_event_PowerButton(lv_event_t * e);
/** 设置按钮事件：进入设置页 */
void ui_event_SetButton(lv_event_t * e);

/** 创建主页控件 */
void ui_HomePage_screen_init(void);
/** 创建关机确认页控件 */
void ui_PowerPage_screen_init(void);

/** 销毁主页控件 */
void ui_HomePage_screen_deinit(void);
/** 销毁关机确认页控件 */
void ui_PowerPage_screen_deinit(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
