#include "../../ui.h"
#include "../../ui_helpers.h"
#include "../Inc/ui_HomePage.h"
#include "../Inc/ui_MenuPage.h"
#include "../Inc/ui_SetPage.h"
#include "../Inc/ui_OffTimePage.h"
#include "../Inc/ui_DateTimeSetPage.h"

#include "../../../Func/Inc/HWDataAccess.h"
/*
 * 【中文导读】ui_SetPage.c —— 设置页（5 个条目 + 1 个开关）
 * ----------------------------------------------------------------------------
 * 自上而下 5 行，每行 70 高，正好一屏（280）不用滚动：
 *   常亮时间 → Page_LOffTimeSet   熄屏时间 → Page_TOffTimeSet
 *   时间日期 → Page_DateTimeSet   抬腕亮屏 → 开关（直连 IMU）
 *   密码设置 → 回调为空（未实现）
 * 
 * 抬腕开关是本页唯一的“有状态”控件：打开时调 HWInterface.IMU.WristEnable()，
 * 关闭时调 WristDisable()；页面重建时读 HWInterface.IMU.wrist_is_enabled 恢复勾选态。
 * 其余条目都只是 Page_Load 入栈，真正的设置在各自的子页里完成。
 */

///////////////////// Page Manager //////////////////
/**
 * @brief 把设置页注册进页面管理器
 * @return 无
 * @note Page_t 统一由 Page_Load/Page_Back 调度。
 */
Page_t Page_Set = {ui_SetPage_screen_init, ui_SetPage_screen_deinit, &ui_SetPage};

///////////////////// VARIABLES ////////////////////
lv_obj_t * ui_SetPage;
lv_obj_t * ui_LightTimePanel;
lv_obj_t * ui_LightTimeButton;
lv_obj_t * ui_LightTimeicon;
lv_obj_t * ui_LightTimeLabel;
lv_obj_t * ui_TurnOffTimePanel;
lv_obj_t * ui_TOffTimeButton;
lv_obj_t * ui_TOffTimeicon;
lv_obj_t * ui_TOffTimeLabel;
lv_obj_t * ui_DateTimeSetPanel;
lv_obj_t * ui_DateTimeSetButton;
lv_obj_t * ui_LOffTimeicon;
lv_obj_t * ui_DateTimeSetLabel;
lv_obj_t * ui_WristPanel;
lv_obj_t * ui_WristButton;
lv_obj_t * ui_Wristicon;
lv_obj_t * ui_WristLabel;
lv_obj_t * ui_WristSwitch;
lv_obj_t * ui_PasswordPanel;
lv_obj_t * ui_PasswordButton;
lv_obj_t * ui_Passwordicon;
lv_obj_t * ui_PasswordLabel;

///////////////////// ANIMATIONS ////////////////////


///////////////////// FUNCTIONS ////////////////////
/**
 * @brief 设置页全局事件：右滑返回上一页
 * @param e LVGL 事件对象
 * @return 无
 * @note 下方注释块是早期手写页面栈的残留实现，已被 PageManager 取代。
 */
void ui_event_SetPage(lv_event_t * e)
{
   lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);

    if(event_code == LV_EVENT_GESTURE)
    {
			if(lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_RIGHT)
      {
        Page_Back();
				// user_Stack_Pop(&ScrRenewStack);
				// if(ScrRenewStack.Data[ScrRenewStack.Top_Point-1] == (long long int)&ui_HomePage)
				// {
				// 	ui_HomePage_screen_init();
				// 	lv_scr_load_anim(ui_HomePage,LV_SCR_LOAD_ANIM_MOVE_RIGHT,100,0,true);
				// }
				// else if(ScrRenewStack.Data[ScrRenewStack.Top_Point-1] == (long long int)&ui_MenuPage)
				// {
				// 	ui_MenuPage_screen_init();
				// 	lv_scr_load_anim(ui_MenuPage,LV_SCR_LOAD_ANIM_MOVE_RIGHT,100,0,true);
				// }
      }
		}
}

/**
 * @brief 常亮时间入口
 * @param e LVGL 事件对象
 * @return 无
 * @note Page_LOffTimeSet 定义在 ui_OffTimePage.c，与熄屏时间共用一套滚轮 UI。
 */
void ui_event_LightTimePanel(lv_event_t * e)
{
	lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED)
    {
        Page_Load(&Page_LOffTimeSet);
    }
}

/**
 * @brief 熄屏时间入口
 * @param e LVGL 事件对象
 * @return 无
 */
void ui_event_TOffTimePanel(lv_event_t * e)
{
	lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED)
    {
        Page_Load(&Page_TOffTimeSet);
    }
}

/**
 * @brief 抬腕亮屏开关：按勾选态启停 IMU 抬腕检测
 * @param e LVGL 事件对象
 * @return 无
 * @note 只处理 VALUE_CHANGED；开关状态本身由 LVGL 维护，业务侧只发命令。
 */
void ui_event_WristSwitch(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_VALUE_CHANGED)
    {
       if(lv_obj_has_state(ui_WristSwitch,LV_STATE_CHECKED))
       {
            //open
            HWInterface.IMU.WristEnable();
       }
       else
       {
            //close
			HWInterface.IMU.WristDisable();
       }
    }
}

/**
 * @brief 时间日期设置入口
 * @param e LVGL 事件对象
 * @return 无
 */
void ui_event_DateTimeSetPanel(lv_event_t * e)
{
	lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED)
    {
        Page_Load(&Page_DateTimeSet);
    }
}

/**
 * @brief 密码设置入口：空实现
 * @param e LVGL 事件对象
 * @return 无
 * @note 面板可点击但无任何动作，属于占位功能。
 */
void ui_event_PasswordPanel(lv_event_t * e)
{
	lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED)
    {

    }
}


/*
 * 创建设置页控件树
 * 5 个 Panel 依次位于 y=0/70/140/210/280，均为 240x70 透明容器：
 *   Panel（PRESSED 态灰底）→ Button（40x40 圆角纯色）+ Label（中文项名）
 * 第 4 行额外挂一个 lv_switch（55x25，右对齐）用于抬腕开关。
 * 注意 Panel 保留 CLICKABLE、Button 反而清除 CLICKABLE，使整行都能点。
 */
///////////////////// SCREEN init ////////////////////
/**
 * @brief 创建设置页全部控件并绑定事件
 * @return 无
 * @note 无参数；重复调用会重复建控件，必须由 PageManager 保证单次。
 */
void ui_SetPage_screen_init(void)
{
    ui_SetPage = lv_obj_create(NULL);

// 第 1 行：常亮时间（蓝）
    ui_LightTimePanel = lv_obj_create(ui_SetPage);
    lv_obj_set_width(ui_LightTimePanel, 240);
    lv_obj_set_height(ui_LightTimePanel, 70);
    lv_obj_set_align(ui_LightTimePanel, LV_ALIGN_TOP_MID);
    lv_obj_clear_flag(ui_LightTimePanel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_LightTimePanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_LightTimePanel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_LightTimePanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_LightTimePanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_LightTimePanel, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(ui_LightTimePanel, 100, LV_PART_MAIN | LV_STATE_PRESSED);

    ui_LightTimeButton = lv_btn_create(ui_LightTimePanel);
    lv_obj_set_width(ui_LightTimeButton, 40);
    lv_obj_set_height(ui_LightTimeButton, 40);
    lv_obj_set_align(ui_LightTimeButton, LV_ALIGN_LEFT_MID);
    lv_obj_add_flag(ui_LightTimeButton, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_LightTimeButton, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_LightTimeButton, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_LightTimeButton, lv_color_hex(0x0080FF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_LightTimeButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_LightTimeicon = lv_label_create(ui_LightTimeButton);
    lv_obj_set_width(ui_LightTimeicon, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_LightTimeicon, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_LightTimeicon, LV_ALIGN_CENTER);
    lv_label_set_text(ui_LightTimeicon, "");
    lv_obj_set_style_text_font(ui_LightTimeicon, &ui_font_iconfont32, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_LightTimeLabel = lv_label_create(ui_LightTimePanel);
    lv_obj_set_width(ui_LightTimeLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_LightTimeLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_LightTimeLabel, 60);
    lv_obj_set_y(ui_LightTimeLabel, 0);
    lv_obj_set_align(ui_LightTimeLabel, LV_ALIGN_LEFT_MID);
    lv_label_set_text(ui_LightTimeLabel, "常亮时间");
    lv_obj_set_style_text_font(ui_LightTimeLabel, &ui_font_Cuyuan20, LV_PART_MAIN | LV_STATE_DEFAULT);

// 第 2 行：熄屏时间（橙）
    ui_TurnOffTimePanel = lv_obj_create(ui_SetPage);
    lv_obj_set_width(ui_TurnOffTimePanel, 240);
    lv_obj_set_height(ui_TurnOffTimePanel, 70);
    lv_obj_set_x(ui_TurnOffTimePanel, 0);
    lv_obj_set_y(ui_TurnOffTimePanel, 70);
    lv_obj_set_align(ui_TurnOffTimePanel, LV_ALIGN_TOP_MID);
    lv_obj_clear_flag(ui_TurnOffTimePanel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_TurnOffTimePanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_TurnOffTimePanel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_TurnOffTimePanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_TurnOffTimePanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_TurnOffTimePanel, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(ui_TurnOffTimePanel, 100, LV_PART_MAIN | LV_STATE_PRESSED);

    ui_TOffTimeButton = lv_btn_create(ui_TurnOffTimePanel);
    lv_obj_set_width(ui_TOffTimeButton, 40);
    lv_obj_set_height(ui_TOffTimeButton, 40);
    lv_obj_set_align(ui_TOffTimeButton, LV_ALIGN_LEFT_MID);
    lv_obj_add_flag(ui_TOffTimeButton, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_TOffTimeButton, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_TOffTimeButton, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_TOffTimeButton, lv_color_hex(0xE18019), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_TOffTimeButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_TOffTimeicon = lv_label_create(ui_TOffTimeButton);
    lv_obj_set_width(ui_TOffTimeicon, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_TOffTimeicon, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_TOffTimeicon, LV_ALIGN_CENTER);
    lv_label_set_text(ui_TOffTimeicon, "");
    lv_obj_set_style_text_font(ui_TOffTimeicon, &ui_font_iconfont45, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_TOffTimeLabel = lv_label_create(ui_TurnOffTimePanel);
    lv_obj_set_width(ui_TOffTimeLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_TOffTimeLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_TOffTimeLabel, 60);
    lv_obj_set_y(ui_TOffTimeLabel, 0);
    lv_obj_set_align(ui_TOffTimeLabel, LV_ALIGN_LEFT_MID);
    lv_label_set_text(ui_TOffTimeLabel, "熄屏时间");
    lv_obj_set_style_text_font(ui_TOffTimeLabel, &ui_font_Cuyuan20, LV_PART_MAIN | LV_STATE_DEFAULT);

// 第 3 行：时间日期（蓝）
    ui_DateTimeSetPanel = lv_obj_create(ui_SetPage);
    lv_obj_set_width(ui_DateTimeSetPanel, 240);
    lv_obj_set_height(ui_DateTimeSetPanel, 70);
    lv_obj_set_x(ui_DateTimeSetPanel, 0);
    lv_obj_set_y(ui_DateTimeSetPanel, 140);
    lv_obj_set_align(ui_DateTimeSetPanel, LV_ALIGN_TOP_MID);
    lv_obj_clear_flag(ui_DateTimeSetPanel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_DateTimeSetPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_DateTimeSetPanel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_DateTimeSetPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_DateTimeSetPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_DateTimeSetPanel, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(ui_DateTimeSetPanel, 100, LV_PART_MAIN | LV_STATE_PRESSED);

    ui_DateTimeSetButton = lv_btn_create(ui_DateTimeSetPanel);
    lv_obj_set_width(ui_DateTimeSetButton, 40);
    lv_obj_set_height(ui_DateTimeSetButton, 40);
    lv_obj_set_align(ui_DateTimeSetButton, LV_ALIGN_LEFT_MID);
    lv_obj_add_flag(ui_DateTimeSetButton, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_DateTimeSetButton, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_DateTimeSetButton, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_DateTimeSetButton, lv_color_hex(0x0080FF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_DateTimeSetButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_LOffTimeicon = lv_label_create(ui_DateTimeSetButton);
    lv_obj_set_width(ui_LOffTimeicon, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_LOffTimeicon, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_LOffTimeicon, LV_ALIGN_CENTER);
    lv_label_set_text(ui_LOffTimeicon, "");
    lv_obj_set_style_text_font(ui_LOffTimeicon, &ui_font_iconfont28, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_DateTimeSetLabel = lv_label_create(ui_DateTimeSetPanel);
    lv_obj_set_width(ui_DateTimeSetLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_DateTimeSetLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_DateTimeSetLabel, 60);
    lv_obj_set_y(ui_DateTimeSetLabel, 0);
    lv_obj_set_align(ui_DateTimeSetLabel, LV_ALIGN_LEFT_MID);
    lv_label_set_text(ui_DateTimeSetLabel, "时间日期");
    lv_obj_set_style_text_font(ui_DateTimeSetLabel, &ui_font_Cuyuan20, LV_PART_MAIN | LV_STATE_DEFAULT);

// 第 4 行：抬腕亮屏（紫），右侧带开关
    ui_WristPanel = lv_obj_create(ui_SetPage);
    lv_obj_set_width(ui_WristPanel, 240);
    lv_obj_set_height(ui_WristPanel, 70);
    lv_obj_set_x(ui_WristPanel, 0);
    lv_obj_set_y(ui_WristPanel, 210);
    lv_obj_set_align(ui_WristPanel, LV_ALIGN_TOP_MID);
    lv_obj_clear_flag(ui_WristPanel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_WristPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_WristPanel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_WristPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_WristPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_WristButton = lv_btn_create(ui_WristPanel);
    lv_obj_set_width(ui_WristButton, 40);
    lv_obj_set_height(ui_WristButton, 40);
    lv_obj_set_align(ui_WristButton, LV_ALIGN_LEFT_MID);
    lv_obj_add_flag(ui_WristButton, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_WristButton, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_WristButton, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_WristButton, lv_color_hex(0x7D197D), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_WristButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Wristicon = lv_label_create(ui_WristButton);
    lv_obj_set_width(ui_Wristicon, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Wristicon, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_Wristicon, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Wristicon, "");
    lv_obj_set_style_text_font(ui_Wristicon, &ui_font_iconfont45, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_WristLabel = lv_label_create(ui_WristPanel);
    lv_obj_set_width(ui_WristLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_WristLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_WristLabel, 60);
    lv_obj_set_y(ui_WristLabel, 0);
    lv_obj_set_align(ui_WristLabel, LV_ALIGN_LEFT_MID);
    lv_label_set_text(ui_WristLabel, "抬腕亮屏");
    lv_obj_set_style_text_font(ui_WristLabel, &ui_font_Cuyuan20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_WristSwitch = lv_switch_create(ui_WristPanel);
    lv_obj_set_width(ui_WristSwitch, 55);
    lv_obj_set_height(ui_WristSwitch, 25);
    lv_obj_set_align(ui_WristSwitch, LV_ALIGN_RIGHT_MID);

// 按 IMU 当前状态恢复开关外观，避免重建后显示与实际相反
		if(HWInterface.IMU.wrist_is_enabled)
		{lv_obj_add_state(ui_WristSwitch, LV_STATE_CHECKED);}

// 第 5 行：密码设置（橙，占位）
    ui_PasswordPanel = lv_obj_create(ui_SetPage);
    lv_obj_set_width(ui_PasswordPanel, 240);
    lv_obj_set_height(ui_PasswordPanel, 70);
    lv_obj_set_x(ui_PasswordPanel, 0);
    lv_obj_set_y(ui_PasswordPanel, 280);
    lv_obj_set_align(ui_PasswordPanel, LV_ALIGN_TOP_MID);
    lv_obj_clear_flag(ui_PasswordPanel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_PasswordPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_PasswordPanel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_PasswordPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_PasswordPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_PasswordPanel, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(ui_PasswordPanel, 100, LV_PART_MAIN | LV_STATE_PRESSED);

    ui_PasswordButton = lv_btn_create(ui_PasswordPanel);
    lv_obj_set_width(ui_PasswordButton, 40);
    lv_obj_set_height(ui_PasswordButton, 40);
    lv_obj_set_align(ui_PasswordButton, LV_ALIGN_LEFT_MID);
    lv_obj_add_flag(ui_PasswordButton, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_PasswordButton, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_PasswordButton, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_PasswordButton, lv_color_hex(0xE18019), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_PasswordButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Passwordicon = lv_label_create(ui_PasswordButton);
    lv_obj_set_width(ui_Passwordicon, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Passwordicon, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_Passwordicon, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Passwordicon, "");
    lv_obj_set_style_text_font(ui_Passwordicon, &ui_font_iconfont34, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_PasswordLabel = lv_label_create(ui_PasswordPanel);
    lv_obj_set_width(ui_PasswordLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_PasswordLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_PasswordLabel, 60);
    lv_obj_set_y(ui_PasswordLabel, 0);
    lv_obj_set_align(ui_PasswordLabel, LV_ALIGN_LEFT_MID);
    lv_label_set_text(ui_PasswordLabel, "密码设置");
    lv_obj_set_style_text_font(ui_PasswordLabel, &ui_font_Cuyuan20, LV_PART_MAIN | LV_STATE_DEFAULT);

    //events
// 事件绑定：Panel 接点击，Switch 接 VALUE_CHANGED
		lv_obj_add_event_cb(ui_SetPage, ui_event_SetPage, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_LightTimePanel, ui_event_LightTimePanel, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_TurnOffTimePanel, ui_event_TOffTimePanel, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_WristSwitch, ui_event_WristSwitch, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_DateTimeSetPanel, ui_event_DateTimeSetPanel, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_PasswordPanel, ui_event_PasswordPanel, LV_EVENT_ALL, NULL);

}

/////////////////// SCREEN deinit ////////////////////
/**
 * @brief 设置页销毁：空实现
 * @return 无
 * @note 无定时器、无堆资源；控件随 screen 一起释放。
 */
void ui_SetPage_screen_deinit(void)
{}
