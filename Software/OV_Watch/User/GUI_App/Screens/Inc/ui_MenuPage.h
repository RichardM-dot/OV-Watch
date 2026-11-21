/**
 * @file    ui_MenuPage.h
 * @brief   功能菜单页面：网格排列各功能入口（日历、计算器、计时器、NFC、
 *          心率、血氧、环境、指南针、游戏、设置、关于）。
 *
 * 每个 xxx_PAGE_EN 宏控制对应入口是否显示/编译，便于按需裁剪功能。
 */
#ifndef _UI_MENUPAGE_H
#define _UI_MENUPAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../ui.h"

#define CALENDAR_PAGE_EN		1	// 日历入口开关
#define COMPUTER_PAGE_EN		1	// 计算器入口开关
#define TIMER_PAGE_EN			1	// 计时器入口开关
#define CARD_PAGE_EN            1	// NFC 卡入口开关
#define SPROTS_PAGE_EN			0	// 运动入口开关（未启用）
#define HR_PAGE_EN				1	// 心率入口开关
#define O2_PAGE_EN				1	// 血氧入口开关
#define ENVIRONMENT_PAGE_EN	    1	// 环境入口开关
#define COMPASS_PAGE_EN			1	// 指南针入口开关
#define GAME_PAGE_EN            1	// 游戏入口开关
#define SET_PAGE_EN				1	// 设置入口开关
#define ABOUT_PAGE_EN			1	// 关于入口开关

extern lv_obj_t * ui_MenuPage;		// 菜单页面的根对象
extern lv_obj_t * ui_MenuCalPanel;	// 日历入口面板
extern lv_obj_t * ui_MenuCalButton;	// 日历入口按钮
extern lv_obj_t * ui_MenuCalicon;	// 日历入口图标
extern lv_obj_t * ui_MenuCalLabel;	// 日历入口文字

extern lv_obj_t * ui_MenuComPanel;	// 计算器入口面板
extern lv_obj_t * ui_MenuComButton;	// 计算器入口按钮
extern lv_obj_t * ui_MenuComicon;	// 计算器入口图标
extern lv_obj_t * ui_MenuComLabel;	// 计算器入口文字

extern lv_obj_t * ui_MenuTimPanel;	// 计时器入口面板
extern lv_obj_t * ui_MenuTimButton;	// 计时器入口按钮
extern lv_obj_t * ui_MenuTimicon;	// 计时器入口图标
extern lv_obj_t * ui_MenuTimLabel;	// 计时器入口文字

extern lv_obj_t * ui_MenuCardPanel;	// NFC 卡入口面板
extern lv_obj_t * ui_MenuCardButton;	// NFC 卡入口按钮
extern lv_obj_t * ui_MenuCardicon;	// NFC 卡入口图标
extern lv_obj_t * ui_MenuCardLabel;	// NFC 卡入口文字

extern lv_obj_t * ui_MenuSprPanel;	// 运动入口面板（未启用）
extern lv_obj_t * ui_MenuSprButton;	// 运动入口按钮
extern lv_obj_t * ui_MenuSpricon;	// 运动入口图标
extern lv_obj_t * ui_MenuSprLabel;	// 运动入口文字

extern lv_obj_t * ui_MenuHRPanel;	// 心率入口面板
extern lv_obj_t * ui_MenuHRButton;	// 心率入口按钮
extern lv_obj_t * ui_MenuHRicon;	// 心率入口图标
extern lv_obj_t * ui_MenuHRLabel;	// 心率入口文字

extern lv_obj_t * ui_MenuO2Panel;	// 血氧入口面板
extern lv_obj_t * ui_MenuO2Button;	// 血氧入口按钮
extern lv_obj_t * ui_MenuO2icon;	// 血氧入口图标
extern lv_obj_t * ui_MenuO2Label;	// 血氧入口文字

extern lv_obj_t * ui_MenuEnvPanel;	// 环境入口面板
extern lv_obj_t * ui_MenuEnvButton;	// 环境入口按钮
extern lv_obj_t * ui_MenuEnvicon;	// 环境入口图标
extern lv_obj_t * ui_MenuEnvLabel;	// 环境入口文字

extern lv_obj_t * ui_MenuCPPanel;	// 指南针入口面板
extern lv_obj_t * ui_MenuCPButton;	// 指南针入口按钮
extern lv_obj_t * ui_MenuCPicon;	// 指南针入口图标
extern lv_obj_t * ui_MenuCPLabel;	// 指南针入口文字

extern lv_obj_t * ui_MenuSetPanel;	// 设置入口面板
extern lv_obj_t * ui_MenuSetButton;	// 设置入口按钮
extern lv_obj_t * ui_MenuSeticon;	// 设置入口图标
extern lv_obj_t * ui_MenuSetLabel;	// 设置入口文字

extern lv_obj_t * ui_MenuAbPanel;	// 关于入口面板
extern lv_obj_t * ui_MenuAbButton;	// 关于入口按钮
extern lv_obj_t * ui_MenuAbicon;	// 关于入口图标
extern lv_obj_t * ui_MenuAbLabel;	// 关于入口文字

extern int16_t ui_MenuScrollY;	// 菜单页滚动偏移（用于手势滑动判定）

extern Page_t Page_Menu;	// 菜单页的页面描述

/** 创建菜单页控件并绑定各入口事件 */
void ui_MenuPage_screen_init(void);
/** 销毁菜单页控件 */
void ui_MenuPage_screen_deinit(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
