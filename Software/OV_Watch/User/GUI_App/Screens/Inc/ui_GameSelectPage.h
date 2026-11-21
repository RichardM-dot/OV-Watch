/**
 * @file    ui_GameSelectPage.h
 * @brief   游戏选择页面：列出可用小游戏并进入。
 */
#ifndef _UI_GAMESELECTPAGE_H
#define _UI_GAMESELECTPAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../ui.h"

extern lv_obj_t * ui_GameSelectPage;	// 游戏选择页面的根对象
extern Page_t Page_GameSelect;			// 游戏选择页的页面描述

/** 创建游戏选择页控件 */
void ui_GameSelectPage_screen_init(void);
/** 销毁游戏选择页控件 */
void ui_GameSelectPage_screen_deinit(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
