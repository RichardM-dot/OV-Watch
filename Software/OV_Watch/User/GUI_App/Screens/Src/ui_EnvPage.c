#include "../../ui.h"
#include "../../ui_helpers.h"
#include "../Inc/ui_EnvPage.h"
#include "../../../Func/Inc/HWDataAccess.h"
/*
 * 【中文导读】ui_EnvPage.c —— 环境页（温度 / 湿度双竖条）
 * ----------------------------------------------------------------------------
 * 左条温度（量程 -10~50 ℃，红色系），右条湿度（0~100 %RH，默认蓝色），
 * 条高 150、宽 20，下方是 30 号字数值 + 单位，条顶是 iconfont 图标。
 * 
 * 数据源只有一个：HWInterface.AHT21.temperature / .humidity，由
 * SensorDataUpdateTask 每 500 ms 调 AHT21_GetHumiTemp() 更新（见 Tasks 层）。
 * 本页只负责把这两个字段搬到控件上，不做任何换算。
 * 
 * 刷新策略：500 ms 无条件重写（没有像首页那样先比较缓存），因为两根 bar 的
 * set_value 本身很便宜；代价是即使数值没变也会触发一次重绘。
 * 
 * 坑：HW_AHT21_InterfaceTypeDef.temperature 声明为 uint8_t，负温度会回绕成
 * 200+，所以左条量程虽然写了 -10，实际永远到不了负半轴。
 */

///////////////////// Page Manager //////////////////
/**
 * @brief 把环境页注册进页面管理器
 * @return 无
 * @note 从菜单“环境”进入。
 */
Page_t Page_Env = {ui_EnvPage_screen_init, ui_EnvPage_screen_deinit, &ui_EnvPage};

///////////////////// VARIABLES ////////////////////
lv_obj_t * ui_EnvPage;
lv_obj_t * ui_EnvTempBar;
lv_obj_t * ui_EnvHumiBar;
lv_obj_t * ui_EnvTempNumLabel;
lv_obj_t * ui_EnvHumiNumLabel;
lv_obj_t * ui_EnvTempUnitLabel;
lv_obj_t * ui_EnvHumiUnitLabel;
lv_obj_t * ui_EnvHumiIcon;
lv_obj_t * ui_EnvTempIcon;


// 500 ms 刷新定时器；deinit 必须删除
lv_timer_t * ui_EnvPageTimer;

/////////////////// private Timer ///////////////////
// need to be destroyed when the page is destroyed
/**
 * @brief 环境页定时器：把 AHT21 最新温湿度写到两根 bar 与两个数值 Label
 * @param timer LVGL 定时器句柄（未使用）
 * @return 无
 * @note 没有做页面可见性判断——因为每种页面进入时都会重建且 deinit 会删定时器，同一时刻只会存在一个实例。
 */
static void EnvPage_timer_cb(lv_timer_t * timer)
{
// 6 字节缓冲：温度最多 3 位、湿度最多 3 位，加符号与 \0 足够
    uint8_t value_strbuf[6];
    //set text
// 温度直接当 bar 值用；lv_bar 内部会按 range 夹紧
    lv_bar_set_value(ui_EnvTempBar, HWInterface.AHT21.temperature, LV_ANIM_OFF);
    lv_bar_set_value(ui_EnvHumiBar, HWInterface.AHT21.humidity, LV_ANIM_OFF);
// 文本与 bar 用同一份数据，保证视觉一致
    sprintf(value_strbuf,"%d", HWInterface.AHT21.temperature);
    lv_label_set_text(ui_EnvTempNumLabel, value_strbuf);
    sprintf(value_strbuf,"%d", HWInterface.AHT21.humidity);
    lv_label_set_text(ui_EnvHumiNumLabel, value_strbuf);
}


///////////////////// SCREEN init ////////////////////
/**
 * @brief 创建环境页：两根竖 bar + 数值/单位/图标
 * @return 无
 * @note 控件顺序是先 bar 后文字，后创建的在上层，因此文字不会被 bar 遮住。
 */
void ui_EnvPage_screen_init(void)
{
// 5 字节缓冲：本页只打印 -?99 这类短数字
		uint8_t strbuf[5];
    ui_EnvPage = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_EnvPage, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

// 温度条：量程 -10~50，竖直 150 高；背景半透明红
    ui_EnvTempBar = lv_bar_create(ui_EnvPage);
    lv_bar_set_range(ui_EnvTempBar, -10, 50);
    lv_bar_set_value(ui_EnvTempBar, HWInterface.AHT21.temperature, LV_ANIM_OFF);
    lv_obj_set_width(ui_EnvTempBar, 20);
    lv_obj_set_height(ui_EnvTempBar, 150);
    lv_obj_set_x(ui_EnvTempBar, -50);
    lv_obj_set_y(ui_EnvTempBar, 0);
    lv_obj_set_align(ui_EnvTempBar, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_color(ui_EnvTempBar, lv_color_hex(0xC80000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_EnvTempBar, 50, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_color(ui_EnvTempBar, lv_color_hex(0xC83232), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_EnvTempBar, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

// 湿度条：未设 range，LVGL 默认 0~100，正好对应 %RH
    ui_EnvHumiBar = lv_bar_create(ui_EnvPage);
    lv_bar_set_value(ui_EnvHumiBar, HWInterface.AHT21.humidity, LV_ANIM_OFF);
    lv_obj_set_width(ui_EnvHumiBar, 20);
    lv_obj_set_height(ui_EnvHumiBar, 150);
    lv_obj_set_x(ui_EnvHumiBar, 50);
    lv_obj_set_y(ui_EnvHumiBar, 0);
    lv_obj_set_align(ui_EnvHumiBar, LV_ALIGN_CENTER);

// 温度数值：条下方 100 px，30 号字
    ui_EnvTempNumLabel = lv_label_create(ui_EnvPage);
    lv_obj_set_width(ui_EnvTempNumLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_EnvTempNumLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_EnvTempNumLabel, -50);
    lv_obj_set_y(ui_EnvTempNumLabel, 100);
    lv_obj_set_align(ui_EnvTempNumLabel, LV_ALIGN_CENTER);
		sprintf(strbuf,"%d",HWInterface.AHT21.temperature);
    lv_label_set_text(ui_EnvTempNumLabel, strbuf);
    lv_obj_set_style_text_font(ui_EnvTempNumLabel, &ui_font_Cuyuan30, LV_PART_MAIN | LV_STATE_DEFAULT);

// 湿度数值：右条下方，同样 30 号字
    ui_EnvHumiNumLabel = lv_label_create(ui_EnvPage);
    lv_obj_set_width(ui_EnvHumiNumLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_EnvHumiNumLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_EnvHumiNumLabel, 50);
    lv_obj_set_y(ui_EnvHumiNumLabel, 100);
    lv_obj_set_align(ui_EnvHumiNumLabel, LV_ALIGN_CENTER);
		sprintf(strbuf,"%d", HWInterface.AHT21.humidity);
    lv_label_set_text(ui_EnvHumiNumLabel, strbuf);
    lv_obj_set_style_text_font(ui_EnvHumiNumLabel, &ui_font_Cuyuan30, LV_PART_MAIN | LV_STATE_DEFAULT);

// 单位 “C” 半透明白色，紧贴数值右侧
    ui_EnvTempUnitLabel = lv_label_create(ui_EnvPage);
    lv_obj_set_width(ui_EnvTempUnitLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_EnvTempUnitLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_EnvTempUnitLabel, -20);
    lv_obj_set_y(ui_EnvTempUnitLabel, 100);
    lv_obj_set_align(ui_EnvTempUnitLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_EnvTempUnitLabel, "C");
    lv_obj_set_style_text_color(ui_EnvTempUnitLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_EnvTempUnitLabel, 128, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_EnvTempUnitLabel, &ui_font_Cuyuan30, LV_PART_MAIN | LV_STATE_DEFAULT);

// 单位 “%” 同理
    ui_EnvHumiUnitLabel = lv_label_create(ui_EnvPage);
    lv_obj_set_width(ui_EnvHumiUnitLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_EnvHumiUnitLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_EnvHumiUnitLabel, 80);
    lv_obj_set_y(ui_EnvHumiUnitLabel, 100);
    lv_obj_set_align(ui_EnvHumiUnitLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_EnvHumiUnitLabel, "%");
    lv_obj_set_style_text_color(ui_EnvHumiUnitLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_EnvHumiUnitLabel, 128, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_EnvHumiUnitLabel, &ui_font_Cuyuan30, LV_PART_MAIN | LV_STATE_DEFAULT);

// 湿度图标在条顶上方 95 px
    ui_EnvHumiIcon = lv_label_create(ui_EnvPage);
    lv_obj_set_width(ui_EnvHumiIcon, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_EnvHumiIcon, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_EnvHumiIcon, 50);
    lv_obj_set_y(ui_EnvHumiIcon, -95);
    lv_obj_set_align(ui_EnvHumiIcon, LV_ALIGN_CENTER);
    lv_label_set_text(ui_EnvHumiIcon, "");
    lv_obj_set_style_text_color(ui_EnvHumiIcon, lv_color_hex(0x1496FF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_EnvHumiIcon, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_EnvHumiIcon, &ui_font_iconfont28, LV_PART_MAIN | LV_STATE_DEFAULT);

// 温度图标同理，暗红色
    ui_EnvTempIcon = lv_label_create(ui_EnvPage);
    lv_obj_set_width(ui_EnvTempIcon, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_EnvTempIcon, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_EnvTempIcon, -50);
    lv_obj_set_y(ui_EnvTempIcon, -95);
    lv_obj_set_align(ui_EnvTempIcon, LV_ALIGN_CENTER);
    lv_label_set_text(ui_EnvTempIcon, "");
    lv_obj_set_style_text_color(ui_EnvTempIcon, lv_color_hex(0xA03232), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_EnvTempIcon, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_EnvTempIcon, &ui_font_iconfont34, LV_PART_MAIN | LV_STATE_DEFAULT);

    //timer
// 500 ms 周期与传感器采样周期一致，不会读到半更新的值
    ui_EnvPageTimer = lv_timer_create(EnvPage_timer_cb, 500,  NULL);

}

/////////////////// SCREEN deinit ////////////////////
/**
 * @brief 环境页销毁：删除定时器
 * @return 无
 * @note 控件随 screen 释放；定时器必须先删，否则回调会访问悬空 Label 指针。
 */
void ui_EnvPage_screen_deinit(void)
{
  lv_timer_del(ui_EnvPageTimer);
}
