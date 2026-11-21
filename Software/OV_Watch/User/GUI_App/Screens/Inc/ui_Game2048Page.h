/**
 * @file    ui_Game2048Page.h
 * @brief   2048 小游戏页面。
 */
#ifndef _UI_GAME2048PAGE_H
#define _UI_GAME2048PAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../ui.h"

extern lv_obj_t * ui_Game2048Page;	// 2048 游戏页面的根对象

extern Page_t Page_Game_2048;	// 2048 游戏页的页面描述

/** 创建 2048 游戏页控件并初始化棋盘 */
void ui_Game2048Page_screen_init(void);
/** 销毁 2048 游戏页控件 */
void ui_Game2048Page_screen_deinit(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
