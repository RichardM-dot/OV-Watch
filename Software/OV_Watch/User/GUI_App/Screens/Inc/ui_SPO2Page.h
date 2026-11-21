/**
 * @file    ui_SPO2Page.h
 * @brief   血氧页面：显示血氧饱和度数值、单位、图标与提示文字。
 */
#ifndef _UI_SPO2PAGE_H
#define _UI_SPO2PAGE_H

#ifdef __cplusplus
extern "C" {
#endif

extern lv_obj_t * ui_SPO2Page;			// 血氧页面的根对象
extern lv_obj_t * ui_SPO2NumLabel;		// 血氧数值
extern lv_obj_t * ui_SPO2UnitLabel;		// 血氧单位（%）
extern lv_obj_t * ui_SPO2NoticeLabel;	// 测量提示/状态文字
extern lv_obj_t * ui_SPO2Icon;			// 血氧图标

extern Page_t Page_SPO2;	// 血氧页的页面描述

/** 创建血氧页控件 */
void ui_SPO2Page_screen_init(void);
/** 销毁血氧页控件 */
void ui_SPO2Page_screen_deinit(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
