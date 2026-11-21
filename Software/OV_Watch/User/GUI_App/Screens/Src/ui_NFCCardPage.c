#include "../../ui.h"
#include "../../ui_helpers.h"
#include "../Inc/ui_Megboxes.h"
#include "../Inc/ui_NFCCardPage.h"

/*
 * 【中文导读】ui_NFCCardPage.c —— 卡包页（3 张 NFC 卡片的占位 UI）
 * ----------------------------------------------------------------------------
 * 三张 200x100 的彩色卡片纵向排列（y=20/140/260，第三张超出屏幕需滚动）：
 * 校园卡（蓝）、门禁卡（绿）、交通卡（蓝）。每张卡含：
 *   ├ 左上：40x40 圆形图标按钮（纯装饰，已清 CLICKABLE）
 *   ├ 卡名：Cuyuan 20 号字
 *   └ 底部：左“改写” / 右“选择”两个 80x30 描边按钮
 * 
 * 功能现状：只有第 1 张卡的“改写/选择”绑定了回调，“改写”弹“开发中”提示框，
 * “选择”是空函数；第 2/3 张卡完全没有绑定事件。NFC 读写驱动尚未接入，
 * 本页目前只是交互原型。
 * 
 * 已知复制粘贴问题（记录不改）：Card2Btn / Card3Btn 的 clear_flag 写的是
 * ui_Card1Btn，导致 2/3 号图标按钮仍保留 CLICKABLE，且 1 号被重复清标志。
 */

///////////////////// Page Manager //////////////////
/**
 * @brief 把卡包页注册进页面管理器
 * @return 无
 * @note 从菜单“卡包”进入。
 */
Page_t Page_NFCCard = {ui_NFCCardPage_screen_init, ui_NFCCardPage_screen_deinit, &ui_NFCCardPage};

///////////////////// VARIABLES ////////////////////
lv_obj_t * ui_NFCCardPage;
lv_obj_t * ui_Card1Panel;
lv_obj_t * ui_Card1Btn;
lv_obj_t * ui_Card1icon;
lv_obj_t * ui_Card1Label;
lv_obj_t * ui_WriteBtn1;
lv_obj_t * ui_WriteLabel1;
lv_obj_t * ui_ChooseBtn1;
lv_obj_t * ui_ChooseLabel1;

lv_obj_t * ui_Card2Panel;
lv_obj_t * ui_Card2Btn;
lv_obj_t * ui_Card2icon;
lv_obj_t * ui_Card2Label;
lv_obj_t * ui_WriteBtn2;
lv_obj_t * ui_WriteLabel2;
lv_obj_t * ui_ChooseBtn2;
lv_obj_t * ui_ChooseLabel2;

lv_obj_t * ui_Card3Panel;
lv_obj_t * ui_Card3Btn;
lv_obj_t * ui_Card3icon;
lv_obj_t * ui_Card3Label;
lv_obj_t * ui_WriteBtn3;
lv_obj_t * ui_WriteLabel3;
lv_obj_t * ui_ChooseBtn3;
lv_obj_t * ui_ChooseLabel3;

///////////////////// FUNCTIONS ////////////////////
/**
 * @brief 第 1 张卡“改写”：弹出“开发中”提示
 * @param e LVGL 事件对象
 * @return 无
 * @note ui_mbox_create 把弹窗建在当前活动屏幕上，不属于本页控件树。
 */
void ui_event_WriteBtn1(lv_event_t * e)
{
	lv_event_code_t event_code = lv_event_get_code(e);
	lv_obj_t * target = lv_event_get_target(e);
	if(event_code == LV_EVENT_CLICKED)
	{
		ui_mbox_create("开发中");
	}
}

/**
 * @brief 第 1 张卡“选择”：空实现
 * @param e LVGL 事件对象
 * @return 无
 * @note 预留给“把该卡设为当前默认卡”的逻辑。
 */
void ui_event_ChooseBtn1(lv_event_t * e)
{
	lv_event_code_t event_code = lv_event_get_code(e);
	lv_obj_t * target = lv_event_get_target(e);
	if(event_code == LV_EVENT_CLICKED)
	{

	}
}

///////////////////// SCREEN init ////////////////////
/**
 * @brief 创建卡包页：3 张卡片，每张 4 个控件
 * @return 无
 * @note 控件数量多但结构重复，2/3 号卡是 1 号卡的复制，注意其中仍有未改全的变量名。
 */
void ui_NFCCardPage_screen_init(void)
{
	ui_NFCCardPage = lv_obj_create(NULL);

// 卡片 1：校园卡，蓝色 0x0F80FF，100 高
    ui_Card1Panel = lv_obj_create(ui_NFCCardPage);
    lv_obj_set_width(ui_Card1Panel, 200);
    lv_obj_set_height(ui_Card1Panel, 100);
    lv_obj_set_x(ui_Card1Panel, 0);
    lv_obj_set_y(ui_Card1Panel, 20);
    lv_obj_set_align(ui_Card1Panel, LV_ALIGN_TOP_MID);
    lv_obj_clear_flag(ui_Card1Panel, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_Card1Panel, lv_color_hex(0x0F80FF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Card1Panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Card1Panel, lv_color_hex(0x0F80FF), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(ui_Card1Panel, 200, LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(ui_Card1Panel, lv_color_hex(0x0F80FF), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(ui_Card1Panel, 128, LV_PART_MAIN | LV_STATE_PRESSED);

// 图标按钮：40x40 白色半透明圆，纯装饰
    ui_Card1Btn = lv_btn_create(ui_Card1Panel);
    lv_obj_set_width(ui_Card1Btn, 40);
    lv_obj_set_height(ui_Card1Btn, 40);
    lv_obj_add_flag(ui_Card1Btn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_Card1Btn, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);      /// Flags
    lv_obj_set_style_radius(ui_Card1Btn, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Card1Btn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Card1Btn, 200, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Card1icon = lv_label_create(ui_Card1Btn);
    lv_obj_set_width(ui_Card1icon, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Card1icon, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_Card1icon, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Card1icon, "");
    lv_obj_set_style_text_color(ui_Card1icon, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Card1icon, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_Card1icon, &ui_font_iconfont34, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Card1Label = lv_label_create(ui_Card1Panel);
    lv_obj_set_width(ui_Card1Label, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Card1Label, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Card1Label, 50);
    lv_obj_set_y(ui_Card1Label, 10);
    lv_label_set_text(ui_Card1Label, "校园卡");
    lv_obj_set_style_text_font(ui_Card1Label, &ui_font_Cuyuan20, LV_PART_MAIN | LV_STATE_DEFAULT);

// “改写”按钮：贴左下，白底 100 透明 + 2 px 灰边
    ui_WriteBtn1 = lv_btn_create(ui_Card1Panel);
    lv_obj_set_width(ui_WriteBtn1, 80);
    lv_obj_set_height(ui_WriteBtn1, 30);
    lv_obj_set_x(ui_WriteBtn1, 0);
    lv_obj_set_y(ui_WriteBtn1, 10);
    lv_obj_set_align(ui_WriteBtn1, LV_ALIGN_BOTTOM_LEFT);
    lv_obj_add_flag(ui_WriteBtn1, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_WriteBtn1, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_WriteBtn1, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_WriteBtn1, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_WriteBtn1, lv_color_hex(0x646464), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_WriteBtn1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_WriteBtn1, 2, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_WriteLabel1 = lv_label_create(ui_WriteBtn1);
    lv_obj_set_width(ui_WriteLabel1, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_WriteLabel1, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_WriteLabel1, LV_ALIGN_CENTER);
    lv_label_set_text(ui_WriteLabel1, "改写");
    lv_obj_set_style_text_font(ui_WriteLabel1, &ui_font_Cuyuan18, LV_PART_MAIN | LV_STATE_DEFAULT);

// “选择”按钮：贴右下，样式与改写对称
    ui_ChooseBtn1 = lv_btn_create(ui_Card1Panel);
    lv_obj_set_width(ui_ChooseBtn1, 80);
    lv_obj_set_height(ui_ChooseBtn1, 30);
    lv_obj_set_x(ui_ChooseBtn1, 0);
    lv_obj_set_y(ui_ChooseBtn1, 10);
    lv_obj_set_align(ui_ChooseBtn1, LV_ALIGN_BOTTOM_RIGHT);
    lv_obj_add_flag(ui_ChooseBtn1, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_ChooseBtn1, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_ChooseBtn1, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_ChooseBtn1, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_ChooseBtn1, lv_color_hex(0x646464), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_ChooseBtn1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_ChooseBtn1, 2, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_ChooseLabel1 = lv_label_create(ui_ChooseBtn1);
    lv_obj_set_width(ui_ChooseLabel1, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_ChooseLabel1, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_ChooseLabel1, LV_ALIGN_CENTER);
    lv_label_set_text(ui_ChooseLabel1, "选择");
    lv_obj_set_style_text_font(ui_ChooseLabel1, &ui_font_Cuyuan18, LV_PART_MAIN | LV_STATE_DEFAULT);

// 卡片 2：门禁卡，绿色 0x0F800F，y=140
    ui_Card2Panel = lv_obj_create(ui_NFCCardPage);
    lv_obj_set_width(ui_Card2Panel, 200);
    lv_obj_set_height(ui_Card2Panel, 100);
    lv_obj_set_x(ui_Card2Panel, 0);
    lv_obj_set_y(ui_Card2Panel, 140);
    lv_obj_set_align(ui_Card2Panel, LV_ALIGN_TOP_MID);
    lv_obj_clear_flag(ui_Card2Panel, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_Card2Panel, lv_color_hex(0x0F800F), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Card2Panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Card2Panel, lv_color_hex(0x0F80FF), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(ui_Card2Panel, 200, LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(ui_Card2Panel, lv_color_hex(0x0F800F), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(ui_Card2Panel, 128, LV_PART_MAIN | LV_STATE_PRESSED);

    ui_Card2Btn = lv_btn_create(ui_Card2Panel);
    lv_obj_set_width(ui_Card2Btn, 40);
    lv_obj_set_height(ui_Card2Btn, 40);
    lv_obj_add_flag(ui_Card2Btn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
// 此处 clear_flag 的目标写成了 ui_Card1Btn（复制粘贴遗留），导致 2 号按钮仍可点击
    lv_obj_clear_flag(ui_Card1Btn, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);      /// Flags
    lv_obj_set_style_radius(ui_Card2Btn, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Card2Btn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Card2Btn, 200, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Card2icon = lv_label_create(ui_Card2Btn);
    lv_obj_set_width(ui_Card2icon, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Card2icon, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_Card2icon, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Card2icon, "");
    lv_obj_set_style_text_color(ui_Card2icon, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Card2icon, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_Card2icon, &ui_font_iconfont34, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Card2Label = lv_label_create(ui_Card2Panel);
    lv_obj_set_width(ui_Card2Label, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Card2Label, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Card2Label, 50);
    lv_obj_set_y(ui_Card2Label, 10);
    lv_label_set_text(ui_Card2Label, "门禁卡");
    lv_obj_set_style_text_font(ui_Card2Label, &ui_font_Cuyuan20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_WriteBtn2 = lv_btn_create(ui_Card2Panel);
    lv_obj_set_width(ui_WriteBtn2, 80);
    lv_obj_set_height(ui_WriteBtn2, 30);
    lv_obj_set_x(ui_WriteBtn2, 0);
    lv_obj_set_y(ui_WriteBtn2, 10);
    lv_obj_set_align(ui_WriteBtn2, LV_ALIGN_BOTTOM_LEFT);
    lv_obj_add_flag(ui_WriteBtn2, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_WriteBtn2, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_WriteBtn2, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_WriteBtn2, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_WriteBtn2, lv_color_hex(0x646464), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_WriteBtn2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_WriteBtn2, 2, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_WriteLabel2 = lv_label_create(ui_WriteBtn2);
    lv_obj_set_width(ui_WriteLabel2, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_WriteLabel2, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_WriteLabel2, LV_ALIGN_CENTER);
    lv_label_set_text(ui_WriteLabel2, "改写");
    lv_obj_set_style_text_font(ui_WriteLabel2, &ui_font_Cuyuan18, LV_PART_MAIN | LV_STATE_DEFAULT);

// “选择”按钮与卡片 1 结构相同
    ui_ChooseBtn2 = lv_btn_create(ui_Card2Panel);
    lv_obj_set_width(ui_ChooseBtn2, 80);
    lv_obj_set_height(ui_ChooseBtn2, 30);
    lv_obj_set_x(ui_ChooseBtn2, 0);
    lv_obj_set_y(ui_ChooseBtn2, 10);
    lv_obj_set_align(ui_ChooseBtn2, LV_ALIGN_BOTTOM_RIGHT);
    lv_obj_add_flag(ui_ChooseBtn2, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_ChooseBtn2, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_ChooseBtn2, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_ChooseBtn2, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_ChooseBtn2, lv_color_hex(0x646464), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_ChooseBtn2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_ChooseBtn2, 2, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_ChooseLabel2 = lv_label_create(ui_ChooseBtn2);
    lv_obj_set_width(ui_ChooseLabel2, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_ChooseLabel2, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_ChooseLabel2, LV_ALIGN_CENTER);
    lv_label_set_text(ui_ChooseLabel2, "选择");
    lv_obj_set_style_text_font(ui_ChooseLabel2, &ui_font_Cuyuan18, LV_PART_MAIN | LV_STATE_DEFAULT);

// 卡片 3：交通卡，y=260，已在屏幕可视区之外，需要上滑
    ui_Card3Panel = lv_obj_create(ui_NFCCardPage);
    lv_obj_set_width(ui_Card3Panel, 200);
    lv_obj_set_height(ui_Card3Panel, 100);
    lv_obj_set_x(ui_Card3Panel, 0);
    lv_obj_set_y(ui_Card3Panel, 260);
    lv_obj_set_align(ui_Card3Panel, LV_ALIGN_TOP_MID);
    lv_obj_clear_flag(ui_Card3Panel, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_Card3Panel, lv_color_hex(0x0F80FF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Card3Panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Card3Panel, lv_color_hex(0x0F80FF), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(ui_Card3Panel, 200, LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(ui_Card3Panel, lv_color_hex(0x0F80FF), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(ui_Card3Panel, 128, LV_PART_MAIN | LV_STATE_PRESSED);

    ui_Card3Btn = lv_btn_create(ui_Card3Panel);
    lv_obj_set_width(ui_Card3Btn, 40);
    lv_obj_set_height(ui_Card3Btn, 40);
    lv_obj_add_flag(ui_Card3Btn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
// 同样误写成 ui_Card1Btn
    lv_obj_clear_flag(ui_Card1Btn, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);      /// Flags
    lv_obj_set_style_radius(ui_Card3Btn, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Card3Btn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Card3Btn, 200, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Card3icon = lv_label_create(ui_Card3Btn);
    lv_obj_set_width(ui_Card3icon, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Card3icon, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_Card3icon, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Card3icon, "");
    lv_obj_set_style_text_color(ui_Card3icon, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Card3icon, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_Card3icon, &ui_font_iconfont30, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Card3Label = lv_label_create(ui_Card3Panel);
    lv_obj_set_width(ui_Card3Label, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Card3Label, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Card3Label, 50);
    lv_obj_set_y(ui_Card3Label, 10);
    lv_label_set_text(ui_Card3Label, "交通卡");
    lv_obj_set_style_text_font(ui_Card3Label, &ui_font_Cuyuan20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_WriteBtn3 = lv_btn_create(ui_Card3Panel);
    lv_obj_set_width(ui_WriteBtn3, 80);
    lv_obj_set_height(ui_WriteBtn3, 30);
    lv_obj_set_x(ui_WriteBtn3, 0);
    lv_obj_set_y(ui_WriteBtn3, 10);
    lv_obj_set_align(ui_WriteBtn3, LV_ALIGN_BOTTOM_LEFT);
    lv_obj_add_flag(ui_WriteBtn3, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_WriteBtn3, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_WriteBtn3, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_WriteBtn3, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_WriteBtn3, lv_color_hex(0x646464), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_WriteBtn3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_WriteBtn3, 2, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_WriteLabel3 = lv_label_create(ui_WriteBtn3);
    lv_obj_set_width(ui_WriteLabel3, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_WriteLabel3, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_WriteLabel3, LV_ALIGN_CENTER);
    lv_label_set_text(ui_WriteLabel3, "改写");
    lv_obj_set_style_text_font(ui_WriteLabel3, &ui_font_Cuyuan18, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_ChooseBtn3 = lv_btn_create(ui_Card3Panel);
    lv_obj_set_width(ui_ChooseBtn3, 80);
    lv_obj_set_height(ui_ChooseBtn3, 30);
    lv_obj_set_x(ui_ChooseBtn3, 0);
    lv_obj_set_y(ui_ChooseBtn3, 10);
    lv_obj_set_align(ui_ChooseBtn3, LV_ALIGN_BOTTOM_RIGHT);
    lv_obj_add_flag(ui_ChooseBtn3, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_ChooseBtn3, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_ChooseBtn3, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_ChooseBtn3, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_ChooseBtn3, lv_color_hex(0x646464), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_ChooseBtn3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_ChooseBtn3, 2, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_ChooseLabel3 = lv_label_create(ui_ChooseBtn3);
    lv_obj_set_width(ui_ChooseLabel3, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_ChooseLabel3, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_ChooseLabel3, LV_ALIGN_CENTER);
    lv_label_set_text(ui_ChooseLabel3, "选择");
    lv_obj_set_style_text_font(ui_ChooseLabel3, &ui_font_Cuyuan18, LV_PART_MAIN | LV_STATE_DEFAULT);

// 只给第 1 张卡绑事件；第 2/3 张卡的两个按钮目前无回调
		lv_obj_add_event_cb(ui_WriteBtn1, ui_event_WriteBtn1, LV_EVENT_ALL, NULL);
		lv_obj_add_event_cb(ui_ChooseBtn1, ui_event_ChooseBtn1, LV_EVENT_ALL, NULL);

}

///////////////////// SCREEN deinit ////////////////////
/**
 * @brief 卡包页销毁：空实现
 * @return 无
 * @note 无定时器；控件随 screen 释放。若弹窗仍开着，需要调用方先 ui_mbox_close。
 */
void ui_NFCCardPage_screen_deinit(void)
{}
