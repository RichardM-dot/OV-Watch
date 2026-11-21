#include "../../ui.h"
#include "../../ui_helpers.h"
#include "../Inc/ui_CalendarPage.h"
#include "../../../Func/Inc/HWDataAccess.h"
/*
 * 【中文导读】ui_CalendarPage.c —— 日历页（LVGL 内置 lv_calendar）
 * ----------------------------------------------------------------------------
 * 全工程最简单的一页：整屏只放一个 lv_calendar，240x280 铺满，带左右箭头
 * 表头用于翻月。数据只在 init 时从 RTC 读一次，之后不再刷新——日历不需要秒级
 * 同步，用户翻月时 LVGL 自己维护显示月份。
 * 
 * lv_calendar 默认把“今天”高亮；today 与 showed 都设为 RTC 的 年/月/日。
 * 注意 RTC 年份是 0~99，LVGL 需要完整年份，所以 2000 + DateTime.Year。
 * 
 * 本页没有定时器，deinit 因此是空实现；控件随 screen 一起销毁。
 */

///////////////////// Page Manager //////////////////
/**
 * @brief 把日历页注册进页面管理器
 * @return 无
 * @note 变量名 Page_Calender（拼写少一个 a）是历史遗留，菜单回调里也用的这个名字。
 */
Page_t Page_Calender = {ui_CalendarPage_screen_init, ui_CalendarPage_screen_deinit, &ui_CalendarPage};

///////////////////// VARIABLES ////////////////////
lv_obj_t * ui_CalendarPage;
lv_obj_t * ui_CalendarPageCalendar;

///////////////////// FUNCTIONS ////////////////////


///////////////////// SCREEN init ////////////////////
/**
 * @brief 创建日历页：读取 RTC 并初始化 lv_calendar
 * @return 无
 * @note 无参数；只在进入页面时读一次时间，页面停留期间不跟随 RTC 跳变。
 */
void ui_CalendarPage_screen_init(void)
{
// 栈上取一份 RTC 快照；lv_calendar 需要年/月/日三个字段
    HW_DateTimeTypeDef DateTime;
    HWInterface.RealTimeClock.GetTimeDate(&DateTime);

    ui_CalendarPage = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_CalendarPage, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
// today 决定哪个日期格子高亮，showed 决定当前显示哪个月

    ui_CalendarPageCalendar = lv_calendar_create(ui_CalendarPage);
// 箭头表头：左右箭头翻月，不需要额外回调
    lv_calendar_set_today_date(ui_CalendarPageCalendar, 2000+DateTime.Year, DateTime.Month, DateTime.Date);
// 日历铺满 240x280，去掉圆角与边框，视觉上与屏幕融合
    lv_calendar_set_showed_date(ui_CalendarPageCalendar, 2000+DateTime.Year, DateTime.Month);
    lv_obj_t * ui_CalendarPageCalendar_header = lv_calendar_header_arrow_create(ui_CalendarPageCalendar);
    lv_obj_set_width(ui_CalendarPageCalendar, 240);
    lv_obj_set_height(ui_CalendarPageCalendar, 280);
    lv_obj_set_align(ui_CalendarPageCalendar, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(ui_CalendarPageCalendar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_CalendarPageCalendar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

}

/**
 * @brief 日历页销毁：空实现
 * @return 无
 * @note 没有定时器与堆资源；lv_calendar 随 screen 释放。
 */
///////////////////// SCREEN deinit ////////////////////
void ui_CalendarPage_screen_deinit(void)
{}
