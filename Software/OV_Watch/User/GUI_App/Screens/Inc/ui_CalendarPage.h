/**
 * @file    ui_CalendarPage.h
 * @brief   日历页面：按月显示日期并高亮当天。
 */
#ifndef _UI_CALENDARPAGE_H
#define _UI_CALENDARPAGE_H

#ifdef __cplusplus
extern "C" {
#endif


extern lv_obj_t * ui_CalendarPage;			// 日历页面的根对象
extern lv_obj_t * ui_CalendarPageCalendar;	// 日历控件（lv_calendar）

extern Page_t Page_Calender;	// 日历页的页面描述（注意变量名拼写为 Calender）

/** 创建日历页控件并绑定事件 */
void ui_CalendarPage_screen_init(void);
/** 销毁日历页控件 */
void ui_CalendarPage_screen_deinit(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
