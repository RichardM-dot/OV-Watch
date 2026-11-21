/**
 * @file    ui_Megboxes.h
 * @brief   通用提示框（Message Box）的创建与关闭。
 */
#ifndef _UI_MEGBOXES_H
#define _UI_MEGBOXES_H

#ifdef __cplusplus
extern "C" {
#endif

/** 弹出提示框并显示字符串 str（如"正在保存""电量不足"） */
void ui_mbox_create(uint8_t * str);
/** 关闭当前提示框 */
void ui_mbox_close(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
