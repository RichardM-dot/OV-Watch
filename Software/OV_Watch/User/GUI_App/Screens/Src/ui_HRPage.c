#include "../../ui.h"
#include "../../ui_helpers.h"
#include "../Inc/ui_HRPage.h"
#include "../../../Func/Inc/HWDataAccess.h"
/*
 * 【中文导读】ui_HRPage.c —— 心率页（80 号大字 + 单位 + 提示）
 * ----------------------------------------------------------------------------
 * 屏幕中央一个 Cuyuan80 的巨大数字（当前心率），右侧“次/分”，上方一个红色
 * 心跳图标，下方“正在测量...”。
 * 
 * 数据来自 HWInterface.HR_meter.HrRate，由 HRDataUpdateTask 每 50 ms 调
 * EM7028 驱动更新；本页定时器同样是 50 ms，等于把传感器刷新率直接透传到 UI。
 * 注意：50 ms 刷一次 80 号字对 SPI 屏压力不小（每次全字重绘），实际观感会有
 * 轻微闪烁；若在意功耗，可降到 200~500 ms。
 * 
 * “正在测量...”是静态文本，测量完成/失败都不会变——没有状态机，属于简化实现。
 */

///////////////////// Page Manager //////////////////
/**
 * @brief 注册到页面管理器
 * @return 无
 * @note 从菜单“心率”进入。
 */
Page_t Page_HR = {ui_HRPage_screen_init, ui_HRPage_screen_deinit, &ui_HRPage};

///////////////////// VARIABLES ////////////////////
lv_obj_t * ui_HRPage;
lv_obj_t * ui_HRPageNumLabel;
lv_obj_t * ui_HRPageUnitLabel;
lv_obj_t * ui_HRPaggiconLabel;
lv_obj_t * ui_HRPageNoticeLabel;

lv_timer_t * ui_HRPageTimer;
// 50 ms 定时器：与 HRDataUpdateTask 同周期

///////////////////// ANIMATIONS ////////////////////


/////////////////// private timer ///////////////////
/**
 * @brief 心率定时器：把 HR_meter.HrRate 刷到中央大数字
 * @param timer LVGL 定时器句柄（未使用）
 * @return 无
 * @note 无缓存比较，每 50 ms 无条件 set_text；LVGL 内部会判断文本相同则跳过重绘。
 */
// need to be destroyed when the page is destroyed
static void HRPage_timer_cb(lv_timer_t * timer)
{
    uint8_t value_strbuf[6];
    //set text
    sprintf(value_strbuf, "%d", HWInterface.HR_meter.HrRate);
    lv_label_set_text(ui_HRPageNumLabel, value_strbuf);
}

///////////////////// FUNCTIONS ////////////////////

///////////////////// SCREEN init ////////////////////
/**
 * @brief 创建心率页
 * @return 无
 * @note init 时先显示一次当前值，避免进页瞬间是空白。
 */
void ui_HRPage_screen_init(void)
{
    uint8_t value_strbuf[5];

    ui_HRPage = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_HRPage, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
// 数值：80 号字，居中

    ui_HRPageNumLabel = lv_label_create(ui_HRPage);
    lv_obj_set_width(ui_HRPageNumLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_HRPageNumLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_HRPageNumLabel, LV_ALIGN_CENTER);
    sprintf(value_strbuf, "%d", HWInterface.HR_meter.HrRate);
    lv_label_set_text(ui_HRPageNumLabel, value_strbuf);
// 单位“次/分”：20 号灰字，放在数字右下
    lv_obj_set_style_text_font(ui_HRPageNumLabel, &ui_font_Cuyuan80, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_HRPageUnitLabel = lv_label_create(ui_HRPage);
    lv_obj_set_width(ui_HRPageUnitLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_HRPageUnitLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_HRPageUnitLabel, 85);
    lv_obj_set_y(ui_HRPageUnitLabel, 18);
    lv_obj_set_align(ui_HRPageUnitLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_HRPageUnitLabel, "次/分");
// 心跳图标：34 号 iconfont，红色，位于顶部中间偏下
    lv_obj_set_style_text_color(ui_HRPageUnitLabel, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_HRPageUnitLabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_HRPageUnitLabel, &ui_font_Cuyuan20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_HRPaggiconLabel = lv_label_create(ui_HRPage);
    lv_obj_set_width(ui_HRPaggiconLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_HRPaggiconLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_HRPaggiconLabel, 0);
    lv_obj_set_y(ui_HRPaggiconLabel, 30);
    lv_obj_set_align(ui_HRPaggiconLabel, LV_ALIGN_TOP_MID);
    lv_label_set_text(ui_HRPaggiconLabel, "");
// 提示文本：固定在数字下方 90 px
    lv_obj_set_style_text_color(ui_HRPaggiconLabel, lv_color_hex(0xFF1919), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_HRPaggiconLabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_HRPaggiconLabel, &ui_font_iconfont34, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_HRPageNoticeLabel = lv_label_create(ui_HRPage);
    lv_obj_set_width(ui_HRPageNoticeLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_HRPageNoticeLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_HRPageNoticeLabel, 0);
    lv_obj_set_y(ui_HRPageNoticeLabel, 90);
    lv_obj_set_align(ui_HRPageNoticeLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_HRPageNoticeLabel, "正在测量...");
    lv_obj_set_style_text_color(ui_HRPageNoticeLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_HRPageNoticeLabel, 220, LV_PART_MAIN | LV_STATE_DEFAULT);
// 50 ms 周期
    lv_obj_set_style_text_font(ui_HRPageNoticeLabel, &ui_font_Cuyuan20, LV_PART_MAIN | LV_STATE_DEFAULT);

    //timer
    ui_HRPageTimer = lv_timer_create(HRPage_timer_cb, 50,  NULL);

}

/**
 * @brief 心率页销毁：删除定时器
 * @return 无
 * @note 控件随 screen 释放；先删定时器避免回调访问悬空指针。
 */
/////////////////// SCREEN deinit ////////////////////
void ui_HRPage_screen_deinit(void)
{
    lv_timer_del(ui_HRPageTimer);
}
