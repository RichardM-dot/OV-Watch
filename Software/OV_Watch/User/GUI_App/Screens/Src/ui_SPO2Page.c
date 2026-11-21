#include "../../ui.h"
#include "../../ui_helpers.h"
#include "../Inc/ui_SPO2Page.h"
#include "../../../Func/Inc/HWDataAccess.h"
/*
 * 【中文导读】ui_SPO2Page.c —— 血氧页（80 号大字 + % + 提示）
 * ----------------------------------------------------------------------------
 * 与心率页同构：中央大数字、右侧单位“%”、上方蓝色血氧图标、下方“正在测量...”。
 * 数据来自 HWInterface.HR_meter.SPO2（EM7028 同一颗芯片算出的血氧饱和度）。
 * 
 * 刷新周期 500 ms，比心率页慢 10 倍——血氧变化本来就慢，没必要高频刷。
 */

///////////////////// Page Manager //////////////////
/**
 * @brief 注册到页面管理器
 * @return 无
 * @note 从菜单“血氧”进入。
 */
Page_t Page_SPO2 = {ui_SPO2Page_screen_init, ui_SPO2Page_screen_deinit, &ui_SPO2Page};

///////////////////// VARIABLES ////////////////////
lv_obj_t * ui_SPO2Page;
lv_obj_t * ui_SPO2NumLabel;
lv_obj_t * ui_SPO2UnitLabel;
lv_obj_t * ui_SPO2NoticeLabel;
lv_obj_t * ui_SPO2Icon;

lv_timer_t * ui_SPO2PageTimer;
// 500 ms 定时器

/////////////////// private Timer ///////////////////
// need to be destroyed when the page is destroyed
static void SPO2Page_timer_cb(lv_timer_t * timer)
{
/**
 * @brief 血氧定时器：刷新中央数值
 * @param timer LVGL 定时器句柄（未使用）
 * @return 无
 * @note SPO2 字段由 HRDataUpdateTask 更新，本回调只读。
 */
    uint8_t value_strbuf[6];
    //set text
    sprintf(value_strbuf, "%d", HWInterface.HR_meter.SPO2);
    lv_label_set_text(ui_SPO2NumLabel, value_strbuf);
}

///////////////////// SCREEN init ////////////////////
void ui_SPO2Page_screen_init(void)
/**
 * @brief 创建血氧页
 * @return 无
 */
{
    uint8_t value_strbuf[5];

    ui_SPO2Page = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_SPO2Page, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

// 数值：80 号字居中
    ui_SPO2NumLabel = lv_label_create(ui_SPO2Page);
    lv_obj_set_width(ui_SPO2NumLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_SPO2NumLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_SPO2NumLabel, LV_ALIGN_CENTER);
    sprintf(value_strbuf, "%d", HWInterface.HR_meter.SPO2);
    lv_label_set_text(ui_SPO2NumLabel, value_strbuf);
    lv_obj_set_style_text_font(ui_SPO2NumLabel, &ui_font_Cuyuan80, LV_PART_MAIN | LV_STATE_DEFAULT);
// 单位“%”：使用 Montserrat 20，因为 Cuyuan 字体没有 % 字形？实际上 Cuyuan 有，此处只是风格选择

    ui_SPO2UnitLabel = lv_label_create(ui_SPO2Page);
    lv_obj_set_width(ui_SPO2UnitLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_SPO2UnitLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_SPO2UnitLabel, 70);
    lv_obj_set_y(ui_SPO2UnitLabel, 18);
    lv_obj_set_align(ui_SPO2UnitLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_SPO2UnitLabel, "%");
    lv_obj_set_style_text_font(ui_SPO2UnitLabel, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
// 提示文本

    ui_SPO2NoticeLabel = lv_label_create(ui_SPO2Page);
    lv_obj_set_width(ui_SPO2NoticeLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_SPO2NoticeLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_SPO2NoticeLabel, 0);
    lv_obj_set_y(ui_SPO2NoticeLabel, 90);
    lv_obj_set_align(ui_SPO2NoticeLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_SPO2NoticeLabel, "正在测量...");
    lv_obj_set_style_text_color(ui_SPO2NoticeLabel, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
// 血氧图标：蓝色
    lv_obj_set_style_text_opa(ui_SPO2NoticeLabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_SPO2NoticeLabel, &ui_font_Cuyuan20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SPO2Icon = lv_label_create(ui_SPO2Page);
    lv_obj_set_width(ui_SPO2Icon, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_SPO2Icon, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_SPO2Icon, 0);
    lv_obj_set_y(ui_SPO2Icon, 30);
    lv_obj_set_align(ui_SPO2Icon, LV_ALIGN_TOP_MID);
    lv_label_set_text(ui_SPO2Icon, "");
    lv_obj_set_style_text_color(ui_SPO2Icon, lv_color_hex(0x0080FF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_SPO2Icon, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_SPO2Icon, &ui_font_iconfont34, LV_PART_MAIN | LV_STATE_DEFAULT);

    //timer
    ui_SPO2PageTimer = lv_timer_create(SPO2Page_timer_cb, 500,  NULL);

}
// 500 ms 周期

/////////////////// SCREEN deinit ////////////////////
void ui_SPO2Page_screen_deinit(void)
{
    lv_timer_del(ui_SPO2PageTimer);
}

/**
 * @brief 血氧页销毁：删除定时器
 * @return 无
 */