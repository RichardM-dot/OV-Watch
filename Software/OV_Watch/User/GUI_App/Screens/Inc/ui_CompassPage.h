/**
 * @file    ui_CompassPage.h
 * @brief   指南针页面：显示罗盘指针、方位文字与海拔。
 */
#ifndef _UI_COMPASSPAGE_H
#define _UI_COMPASSPAGE_H

#ifdef __cplusplus
extern "C" {
#endif

extern lv_obj_t * ui_CompassPage;		// 指南针页面的根对象
extern lv_obj_t * ui_Compassneedle;		// 罗盘指针图片对象
extern lv_obj_t * ui_CompassDirLabel;	// 方位文字标签（如 N、NE）
extern lv_obj_t * ui_EnvAltitudeLabel;	// 海拔高度标签

extern Page_t Page_Compass;	// 指南针页的页面描述

/** 创建指南针页控件 */
void ui_CompassPage_screen_init(void);
/** 销毁指南针页控件 */
void ui_CompassPage_screen_deinit(void);

LV_IMG_DECLARE(ui_img_compass_needle_png);	// 罗盘指针图片资源

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
