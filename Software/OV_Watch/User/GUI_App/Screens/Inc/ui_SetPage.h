/**
 * @file    ui_SetPage.h
 * @brief   设置页面：背光时间、熄屏时间、日期时间、抬腕开关、密码等入口。
 */
#ifndef _UI_SETPAGE_H
#define _UI_SETPAGE_H

#ifdef __cplusplus
extern "C" {
#endif

extern lv_obj_t * ui_SetPage;			// 设置页面的根对象
extern lv_obj_t * ui_LightTimePanel;	// 背光时间设置面板
extern lv_obj_t * ui_LightTimeButton;	// 背光时间设置按钮
extern lv_obj_t * ui_LightTimeicon;		// 背光时间图标
extern lv_obj_t * ui_LightTimeLabel;	// 背光时间文字
extern lv_obj_t * ui_TurnOffTimePanel;	// 熄屏时间设置面板
extern lv_obj_t * ui_TOffTimeButton;	// 熄屏时间设置按钮
extern lv_obj_t * ui_TOffTimeicon;		// 熄屏时间图标
extern lv_obj_t * ui_TOffTimeLabel;		// 熄屏时间文字
extern lv_obj_t * ui_DateTimeSetPanel;	// 日期时间设置面板
extern lv_obj_t * ui_DateTimeSetButton;	// 日期时间设置按钮
extern lv_obj_t * ui_LOffTimeicon;		// 日期时间图标
extern lv_obj_t * ui_DateTimeSetLabel;	// 日期时间文字
extern lv_obj_t * ui_WristPanel;		// 抬腕亮屏设置面板
extern lv_obj_t * ui_WristButton;		// 抬腕亮屏设置按钮
extern lv_obj_t * ui_Wristicon;			// 抬腕图标
extern lv_obj_t * ui_WristLabel;		// 抬腕文字
extern lv_obj_t * ui_WristSwitch;		// 抬腕亮屏开关
extern lv_obj_t * ui_PasswordPanel;		// 密码设置面板
extern lv_obj_t * ui_PasswordButton;	// 密码设置按钮
extern lv_obj_t * ui_Passwordicon;		// 密码图标
extern lv_obj_t * ui_PasswordLabel;		// 密码文字

extern Page_t Page_Set;	// 设置页的页面描述

/** 创建设置页控件并绑定各入口事件 */
void ui_SetPage_screen_init(void);

/** 销毁设置页控件 */
void ui_SetPage_screen_deinit(void);


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
