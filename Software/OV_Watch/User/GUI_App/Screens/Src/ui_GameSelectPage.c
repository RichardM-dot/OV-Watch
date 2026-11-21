#include "../../ui.h"
#include "../../ui_helpers.h"
#include "../Inc/ui_MenuPage.h"
#include "../Inc/ui_GameSelectPage.h"
#include "../Inc/ui_Game2048Page.h"
/*
 * 【中文导读】ui_GameSelectPage.c —— 游戏选择页（3 选 1）
 * ----------------------------------------------------------------------------
 * 三行 240x70 的透明面板，结构与菜单页完全一致（Panel + 40x40 图标 + 名称）：
 *   2048（青绿）→ Page_Game_2048     记忆方块（橙）→ Page_GameMem
 *   反弹球（粉红）→ 未绑定回调，尚未实现
 * 
 * 注意：三个按钮/图标都是函数内的局部变量（lv_obj_t * 在 init 里定义），
 * 不导出到全局，因为本页不需要在别处引用它们；只有 Panel 是全局的，
 * 因为要绑事件回调。
 */
#include "../Inc/ui_GameMemPage.h"

/**
 * @brief 把游戏选择页注册进页面管理器
 * @return 无
 * @note 从菜单“游戏”进入；本页再作为两个游戏的入口。
 */
///////////////////// Page Manager //////////////////
Page_t Page_GameSelect = {ui_GameSelectPage_screen_init, ui_GameSelectPage_screen_deinit, &ui_GameSelectPage};

///////////////////// VARIABLES ////////////////////
lv_obj_t * ui_GameSelectPage;
lv_obj_t * ui_Game2048Panel;
lv_obj_t * ui_GameMemPanel;
lv_obj_t * ui_GameBallPanel;

/**
 * @brief 游戏选择页手势：右滑返回菜单
 * @param e LVGL 事件对象
 * @return 无
 */
void ui_event_GameSelectPage(lv_event_t * e)
{
   lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);

    if(event_code == LV_EVENT_GESTURE)
    {
        if(lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_RIGHT)
        {
			Page_Back();
        }
    }
}

/**
 * @brief 2048 入口：进入 2048 页
 * @param e LVGL 事件对象
 * @return 无
 */
void ui_event_Game2048Panel(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED)
    {
        Page_Load(&Page_Game_2048);
    }
}

void ui_event_GameMemPanel(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
/**
 * @brief 记忆方块入口
 * @param e LVGL 事件对象
 * @return 无
 */
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED)
    {
        Page_Load(&Page_GameMem);
    }
}


///////////////////// SCREEN init ////////////////////
void ui_GameSelectPage_screen_init(void)
{
    ui_GameSelectPage = lv_obj_create(NULL);

    //2048
/**
 * @brief 创建三个游戏条目并绑定事件
 * @return 无
 * @note Button 与 icon 用局部变量即可，Panel 必须全局以便回调识别？实际上回调只用 event_code，Panel 全局只是为了外部一致性。
 */
    ui_Game2048Panel = lv_obj_create(ui_GameSelectPage);
    lv_obj_set_width(ui_Game2048Panel, 240);
    lv_obj_set_height(ui_Game2048Panel, 70);
    lv_obj_set_align(ui_Game2048Panel, LV_ALIGN_TOP_MID);
    lv_obj_clear_flag(ui_Game2048Panel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
// 2048 条目：y=0，青绿色 0x009A8D
    lv_obj_set_style_radius(ui_Game2048Panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Game2048Panel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Game2048Panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Game2048Panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Game2048Panel, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(ui_Game2048Panel, 100, LV_PART_MAIN | LV_STATE_PRESSED);

    lv_obj_t * ui_Game2048Button = lv_btn_create(ui_Game2048Panel);
    lv_obj_set_width(ui_Game2048Button, 40);
    lv_obj_set_height(ui_Game2048Button, 40);
    lv_obj_set_align(ui_Game2048Button, LV_ALIGN_LEFT_MID);
    lv_obj_add_flag(ui_Game2048Button, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_Game2048Button, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_Game2048Button, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Game2048Button, lv_color_hex(0x009A8D), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Game2048Button, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * ui_Game2048icon = lv_label_create(ui_Game2048Button);
    lv_obj_set_width(ui_Game2048icon, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Game2048icon, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_Game2048icon, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Game2048icon, "");
    lv_obj_set_style_text_font(ui_Game2048icon, &ui_font_iconfont30, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * ui_Game2048Label = lv_label_create(ui_Game2048Panel);
    lv_obj_set_width(ui_Game2048Label, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Game2048Label, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Game2048Label, 60);
    lv_obj_set_y(ui_Game2048Label, 0);
    lv_obj_set_align(ui_Game2048Label, LV_ALIGN_LEFT_MID);
    lv_label_set_text(ui_Game2048Label, "2048");
    lv_obj_set_style_text_font(ui_Game2048Label, &ui_font_Cuyuan20, LV_PART_MAIN | LV_STATE_DEFAULT);

// 记忆方块条目：y=70，橙色 0xC07010
    //mem
    ui_GameMemPanel = lv_obj_create(ui_GameSelectPage);
    lv_obj_set_width(ui_GameMemPanel, 240);
    lv_obj_set_height(ui_GameMemPanel, 70);
    lv_obj_set_x(ui_GameMemPanel, 0);
    lv_obj_set_y(ui_GameMemPanel, 70);
    lv_obj_set_align(ui_GameMemPanel, LV_ALIGN_TOP_MID);
    lv_obj_clear_flag(ui_GameMemPanel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_GameMemPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_GameMemPanel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_GameMemPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_GameMemPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_GameMemPanel, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(ui_GameMemPanel, 100, LV_PART_MAIN | LV_STATE_PRESSED);

    lv_obj_t * ui_GameMemButton = lv_btn_create(ui_GameMemPanel);
    lv_obj_set_width(ui_GameMemButton, 40);
    lv_obj_set_height(ui_GameMemButton, 40);
    lv_obj_set_align(ui_GameMemButton, LV_ALIGN_LEFT_MID);
    lv_obj_add_flag(ui_GameMemButton, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_GameMemButton, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_GameMemButton, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_GameMemButton, lv_color_hex(0xC07010), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_GameMemButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * ui_GameMemicon = lv_label_create(ui_GameMemButton);
    lv_obj_set_width(ui_GameMemicon, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_GameMemicon, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_GameMemicon, LV_ALIGN_CENTER);
    lv_label_set_text(ui_GameMemicon, "");
    lv_obj_set_style_text_font(ui_GameMemicon, &ui_font_iconfont34, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * ui_GameMemLabel = lv_label_create(ui_GameMemPanel);
// 反弹球条目：y=140，粉红 0xFF8080
    lv_obj_set_width(ui_GameMemLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_GameMemLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_GameMemLabel, 60);
    lv_obj_set_y(ui_GameMemLabel, 0);
    lv_obj_set_align(ui_GameMemLabel, LV_ALIGN_LEFT_MID);
    lv_label_set_text(ui_GameMemLabel, "记忆方块");
    lv_obj_set_style_text_font(ui_GameMemLabel, &ui_font_Cuyuan20, LV_PART_MAIN | LV_STATE_DEFAULT);

    //ball
    ui_GameBallPanel = lv_obj_create(ui_GameSelectPage);
    lv_obj_set_width(ui_GameBallPanel, 240);
    lv_obj_set_height(ui_GameBallPanel, 70);
    lv_obj_set_x(ui_GameBallPanel, 0);
    lv_obj_set_y(ui_GameBallPanel, 140);
    lv_obj_set_align(ui_GameBallPanel, LV_ALIGN_TOP_MID);
    lv_obj_clear_flag(ui_GameBallPanel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_GameBallPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_GameBallPanel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_GameBallPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_GameBallPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_GameBallPanel, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(ui_GameBallPanel, 100, LV_PART_MAIN | LV_STATE_PRESSED);

    lv_obj_t * ui_GameBallButton = lv_btn_create(ui_GameBallPanel);
    lv_obj_set_width(ui_GameBallButton, 40);
    lv_obj_set_height(ui_GameBallButton, 40);
    lv_obj_set_align(ui_GameBallButton, LV_ALIGN_LEFT_MID);
    lv_obj_add_flag(ui_GameBallButton, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_GameBallButton, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_GameBallButton, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_GameBallButton, lv_color_hex(0xFF8080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_GameBallButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * ui_GameBallicon = lv_label_create(ui_GameBallButton);
    lv_obj_set_width(ui_GameBallicon, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_GameBallicon, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_GameBallicon, LV_ALIGN_CENTER);
    lv_label_set_text(ui_GameBallicon, "");
    lv_obj_set_style_text_font(ui_GameBallicon, &ui_font_iconfont28, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * ui_GameBallLabel = lv_label_create(ui_GameBallPanel);
    lv_obj_set_width(ui_GameBallLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_GameBallLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_GameBallLabel, 60);
    lv_obj_set_y(ui_GameBallLabel, 0);
    lv_obj_set_align(ui_GameBallLabel, LV_ALIGN_LEFT_MID);
    lv_label_set_text(ui_GameBallLabel, "反 弹 球");
// 只绑定了 2048 与记忆方块；反弹球暂无回调
    lv_obj_set_style_text_font(ui_GameBallLabel, &ui_font_Cuyuan20, LV_PART_MAIN | LV_STATE_DEFAULT);


		lv_obj_add_event_cb(ui_GameSelectPage, ui_event_GameSelectPage, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_Game2048Panel, ui_event_Game2048Panel, LV_EVENT_ALL, NULL);
		lv_obj_add_event_cb(ui_GameMemPanel, ui_event_GameMemPanel, LV_EVENT_ALL, NULL);

}

/**
 * @brief 游戏选择页销毁：空实现
 * @return 无
 * @note 无定时器；三个游戏页各自管理自己的资源。
 */
///////////////////// SCREEN deinit ////////////////////
void ui_GameSelectPage_screen_deinit(void)
{}
