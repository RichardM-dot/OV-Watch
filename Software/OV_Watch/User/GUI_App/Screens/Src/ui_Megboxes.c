#include "../../ui.h"
#include "../../ui_helpers.h"

/*
 * 【中文导读】ui_Megboxes.c —— 通用确认弹窗（Message Box）
 * ----------------------------------------------------------------------------
 * 注意：本文件不是“页面”，没有 Page_t、也没有独立 screen。它把弹窗直接建在
 * 当前活动屏幕 lv_scr_act() 上（一个 200x130 的白色面板），关闭时 lv_obj_del。
 * 因此它不受 PageManager 管理，也不会随页面切换自动消失——调用方必须保证
 * 在切页前先 ui_mbox_close()。
 * 
 * 结构：面板 → 警告图标 + 文案 Label + OK 行 + Cancel 行（两行都是可点的
 * 面板，PRESSED 态变灰）。当前两个回调都只做“关闭”，没有“确认/取消”的
 * 业务区分——需要回调结果时应在此处扩展，而不是改调用方。
 */
///////////////////// VARIABLES ////////////////////
lv_obj_t * ui_mboxPanel;
lv_obj_t * ui_WarningSymbol;
lv_obj_t * ui_WarningLabel;
lv_obj_t * ui_ApplyPanel;
lv_obj_t * ui_ApplyLabel;
lv_obj_t * ui_CancelPanel;
lv_obj_t * ui_CancelLabel;

/**
 * @brief 关闭弹窗并恢复当前屏幕的滚动能力
 * @return 无
 * @note 建窗时清了 lv_scr_act() 的 SCROLLABLE，防止弹窗期间背景被拖动；关闭时必须加回来，否则后续页面无法滚动。
 */
///////////////////// FUNCTIONS ////////////////////;
void ui_mbox_close(void)
{
    lv_obj_del(ui_mboxPanel);
    lv_obj_add_flag(lv_scr_act(),LV_OBJ_FLAG_SCROLLABLE);
}
/**
 * @brief OK 行点击回调：关闭弹窗
 * @param e LVGL 事件对象
 * @return 无
 * @note 未向调用方回传“确认”结果，仅关闭。
 */

void ui_mbox1_event_cb(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED)
    {
        ui_mbox_close();
    }
}
/**
 * @brief Cancel 行点击回调：同样只关闭
 * @param e LVGL 事件对象
 * @return 无
 * @note 与 OK 行为完全一致，属于未完成的双按钮语义。
 */

void ui_mbox2_event_cb(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED)
    {
        ui_mbox_close();
    }
}

/**
 * @brief 在当前活动屏幕上创建确认弹窗
 * @param text 弹窗正文（以 \0 结尾的字符串）
 * @return 无
 * @note text 指针由 LVGL 拷贝，调用后即可释放；但 ui_mboxPanel 是全局单例，重复调用会覆盖指针并泄漏上一个弹窗。
 */
///////////////////// SCREEN init ////////////////////
void ui_mbox_create(uint8_t * text)
// 先锁住背景滚动，避免弹窗期间底层页面跟着手指动
{
    lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);      /// Flags
// 200x130 居中白色面板，1 px 边框

    ui_mboxPanel = lv_obj_create(lv_scr_act());
    lv_obj_set_width(ui_mboxPanel, 200);
    lv_obj_set_height(ui_mboxPanel, 130);
    lv_obj_set_align(ui_mboxPanel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_mboxPanel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_mboxPanel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_mboxPanel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_mboxPanel, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
// 顶部警告图标（LVGL 内置符号字体）

    ui_WarningSymbol = lv_label_create(ui_mboxPanel);
    lv_obj_set_width(ui_WarningSymbol, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_WarningSymbol, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_WarningSymbol, LV_ALIGN_TOP_MID);
    lv_label_set_text(ui_WarningSymbol, LV_SYMBOL_WARNING);
    lv_obj_set_style_text_color(ui_WarningSymbol, lv_color_hex(0x0080FF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_WarningSymbol, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

// 正文：18 号中文字体，位于图标下方 25 px
    ui_WarningLabel = lv_label_create(ui_mboxPanel);
    lv_obj_set_width(ui_WarningLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_WarningLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_WarningLabel, 0);
    lv_obj_set_y(ui_WarningLabel, 25);
    lv_obj_set_align(ui_WarningLabel, LV_ALIGN_TOP_MID);
    lv_label_set_text(ui_WarningLabel, text);
    lv_obj_set_style_text_color(ui_WarningLabel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_WarningLabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
// OK 行：贴底、上边框分隔、按下变灰
    lv_obj_set_style_text_font(ui_WarningLabel, &ui_font_Cuyuan18, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_ApplyPanel = lv_obj_create(ui_mboxPanel);
    lv_obj_set_width(ui_ApplyPanel, 200);
    lv_obj_set_height(ui_ApplyPanel, 30);
    lv_obj_set_x(ui_ApplyPanel, 0);
    lv_obj_set_y(ui_ApplyPanel, -16);
    lv_obj_set_align(ui_ApplyPanel, LV_ALIGN_BOTTOM_MID);
    lv_obj_clear_flag(ui_ApplyPanel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_ApplyPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_ApplyPanel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_ApplyPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_ApplyPanel, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_ApplyPanel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_ApplyPanel, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui_ApplyPanel, LV_BORDER_SIDE_TOP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_ApplyPanel, lv_color_hex(0x969696), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(ui_ApplyPanel, 255, LV_PART_MAIN | LV_STATE_PRESSED);

    ui_ApplyLabel = lv_label_create(ui_ApplyPanel);
    lv_obj_set_width(ui_ApplyLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_ApplyLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_ApplyLabel, LV_ALIGN_CENTER);
// Cancel 行：紧贴 OK 行下方，样式相同
    lv_label_set_text(ui_ApplyLabel, "OK");
    lv_obj_set_style_text_color(ui_ApplyLabel, lv_color_hex(0x5FB878), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_ApplyLabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_CancelPanel = lv_obj_create(ui_mboxPanel);
    lv_obj_set_width(ui_CancelPanel, 200);
    lv_obj_set_height(ui_CancelPanel, 30);
    lv_obj_set_x(ui_CancelPanel, 0);
    lv_obj_set_y(ui_CancelPanel, 14);
    lv_obj_set_align(ui_CancelPanel, LV_ALIGN_BOTTOM_MID);
    lv_obj_clear_flag(ui_CancelPanel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_CancelPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_CancelPanel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_CancelPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_CancelPanel, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_CancelPanel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_CancelPanel, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui_CancelPanel, LV_BORDER_SIDE_TOP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_CancelPanel, lv_color_hex(0x969696), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(ui_CancelPanel, 255, LV_PART_MAIN | LV_STATE_PRESSED);

    ui_CancelLabel = lv_label_create(ui_CancelPanel);
    lv_obj_set_width(ui_CancelLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_CancelLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_CancelLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_CancelLabel, "Cancel");
    lv_obj_set_style_text_color(ui_CancelLabel, lv_color_hex(0xFF0000), LV_PART_MAIN | LV_STATE_DEFAULT);
// 两个行面板各自绑定回调；注意它们都没有加 CLICKABLE 清除，默认可点
    lv_obj_set_style_text_opa(ui_CancelLabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(ui_ApplyPanel, ui_mbox1_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_CancelPanel, ui_mbox2_event_cb, LV_EVENT_ALL, NULL);
}

/////////////////// SCREEN deinit ////////////////////

