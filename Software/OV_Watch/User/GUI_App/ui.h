/**
 * @file    ui.h
 * @brief   界面总入口：字体声明与 ui_init。
 *
 * 工程使用 LVGL 8.2.0，界面由 SquareLine Studio 生成。
 * 这里声明了"粗源"中文字体（多档字号）与图标字体（iconfont），
 * 各页面文件通过 LV_FONT_DECLARE 引用这些字体对象。
 */
// LVGL VERSION: 8.2.0


#ifndef _UI_H
#define _UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"	// LVGL 图形库总头文件

#include "stdio.h"

#include "../Func/Inc/PageManager.h"	// 页面栈管理

LV_FONT_DECLARE(ui_font_Cuyuan18);	// 中文"粗源"18px 字体
LV_FONT_DECLARE(ui_font_Cuyuan20);	// 中文"粗源"20px 字体
LV_FONT_DECLARE(ui_font_Cuyuan24);	// 中文"粗源"24px 字体
LV_FONT_DECLARE(ui_font_Cuyuan30);	// 中文"粗源"30px 字体
LV_FONT_DECLARE(ui_font_Cuyuan38);	// 中文"粗源"38px 字体
LV_FONT_DECLARE(ui_font_Cuyuan48);	// 中文"粗源"48px 字体
LV_FONT_DECLARE(ui_font_Cuyuan80);	// 中文"粗源"80px 字体
LV_FONT_DECLARE(ui_font_Cuyuan100);	// 中文"粗源"100px 字体（大号数字/时间）
LV_FONT_DECLARE(ui_font_iconfont16);	// 图标字体 16px
LV_FONT_DECLARE(ui_font_iconfont24);	// 图标字体 24px
LV_FONT_DECLARE(ui_font_iconfont28);	// 图标字体 28px
LV_FONT_DECLARE(ui_font_iconfont30);	// 图标字体 30px
LV_FONT_DECLARE(ui_font_iconfont32);	// 图标字体 32px
LV_FONT_DECLARE(ui_font_iconfont34);	// 图标字体 34px
LV_FONT_DECLARE(ui_font_iconfont45);	// 图标字体 45px

/** 初始化界面：创建控件样式并进入主页 */
void ui_init(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
