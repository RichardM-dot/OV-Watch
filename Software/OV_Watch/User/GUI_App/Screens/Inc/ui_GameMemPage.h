/**
 * @file    ui_GameMemPage.h
 * @brief   记忆翻牌小游戏页面。
 */
#ifndef _UI_GAMEMEMPAGE_H
#define _UI_GAMEMEMPAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../ui.h"

extern lv_obj_t * ui_GameMem_Page;	// 记忆游戏页面的根对象
extern Page_t Page_GameMem;			// 记忆游戏页的页面描述

/** 创建记忆游戏页控件 */
void ui_GameMemPage_screen_init(void);
/** 销毁记忆游戏页控件 */
void ui_GameMemPage_screen_deinit(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
