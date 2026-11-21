/**
 * @file    ui_NFCCardPage.h
 * @brief   NFC 卡包页面：列出 3 张可切换的卡片。
 */
#ifndef _UI_NFCCARDPAGE_H
#define _UI_NFCCARDPAGE_H

#ifdef __cplusplus
extern "C" {
#endif

extern lv_obj_t * ui_NFCCardPage;	// NFC 卡包页面的根对象
extern lv_obj_t * ui_Card1Panel;	// 卡片 1 面板
extern lv_obj_t * ui_Card1Btn;		// 卡片 1 按钮
extern lv_obj_t * ui_Card1icon;		// 卡片 1 图标
extern lv_obj_t * ui_Card1Label;	// 卡片 1 文字
extern lv_obj_t * ui_Card2Panel;	// 卡片 2 面板
extern lv_obj_t * ui_Card2Btn;		// 卡片 2 按钮
extern lv_obj_t * ui_Card2icon;		// 卡片 2 图标
extern lv_obj_t * ui_Card2Label;	// 卡片 2 文字
extern lv_obj_t * ui_Card3Panel;	// 卡片 3 面板
extern lv_obj_t * ui_Card3Btn;		// 卡片 3 按钮
extern lv_obj_t * ui_Card3icon;		// 卡片 3 图标
extern lv_obj_t * ui_Card3Label;	// 卡片 3 文字

extern Page_t Page_NFCCard;	// NFC 卡包页的页面描述

/** 创建 NFC 卡包页控件 */
void ui_NFCCardPage_screen_init(void);
/** 销毁 NFC 卡包页控件 */
void ui_NFCCardPage_screen_deinit(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
