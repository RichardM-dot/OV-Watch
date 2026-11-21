/**
 * @file    ui_EnvPage.h
 * @brief   环境页面：显示温度、湿度（数值 + 进度条 + 图标）。
 */
#ifndef _UI_ENVPAGE_H
#define _UI_ENVPAGE_H

#ifdef __cplusplus
extern "C" {
#endif

extern lv_obj_t * ui_EnvPage;			// 环境页面的根对象
extern lv_obj_t * ui_EnvTempBar;		// 温度进度条
extern lv_obj_t * ui_EnvHumiBar;		// 湿度进度条
extern lv_obj_t * ui_EnvTempNumLabel;	// 温度数值
extern lv_obj_t * ui_EnvHumiNumLabel;	// 湿度数值
extern lv_obj_t * ui_EnvTempUnitLabel;	// 温度单位（℃）
extern lv_obj_t * ui_EnvHumiUnitLabel;	// 湿度单位（%）
extern lv_obj_t * ui_EnvHumiIcon;		// 湿度图标
extern lv_obj_t * ui_EnvTempIcon;		// 温度图标

extern Page_t Page_Env;	// 环境页的页面描述

/** 创建环境页控件 */
void ui_EnvPage_screen_init(void);
/** 销毁环境页控件 */
void ui_EnvPage_screen_deinit(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
