/**
 * @file    ui_AboutPage.h
 * @brief   "关于"页面：显示固件版本、作者与项目信息。
 */
#ifndef _UI_ABOUTPAGE_H
#define _UI_ABOUTPAGE_H

#ifdef __cplusplus
extern "C" {
#endif

extern lv_obj_t * ui_AboutPage;	// 关于页面的根对象（屏幕）

extern Page_t Page_About;		// 关于页的页面描述（供 PageManager 入栈）

/** 创建关于页的控件并绑定事件 */
void ui_AboutPage_screen_init(void);
/** 销毁关于页的控件，释放 LVGL 对象 */
void ui_AboutPage_screen_deinit(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
