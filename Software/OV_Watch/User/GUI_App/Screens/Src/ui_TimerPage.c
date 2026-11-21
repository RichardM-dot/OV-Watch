#include "../../ui.h"
#include "../../ui_helpers.h"
#include "../Inc/ui_TimerPage.h"
/*
 * 【中文导读】ui_TimerPage.c —— 秒表页（lv_meter + 10 ms 软件计时）
 * ----------------------------------------------------------------------------
 * 界面：一个 240x240 的 lv_meter 画 60 格刻度（每秒一格），中央叠“分:秒.厘秒”
 * 四个 Label，下方两个圆形按钮：左复位（LV_SYMBOL_REFRESH）、右启停（PLAY/PAUSE）。
 * 
 * 计时模型：ui_TimerPage_elapsed_ms 是唯一的时间基准，10 ms 定时器回调把它
 * 换算成 min/sec/10ms 后刷 3 个 Label。启停用 lv_timer_pause/resume，
 * 而不是在回调里判断标志位——暂停期间回调根本不跑，省 CPU。
 * 
 * 注意（代码现状）：LabelRefresh_timer 只读 elapsed_ms、从未累加，也没有用
 * lv_tick_get() 做差值，所以当前秒表显示恒为 00:00.00。要在不改变现有结构的
 * 前提下修复，应在回调开头累加 10（或改用 tick 差值）。详见 KB Tasks_SensRunMode。
 * 
 * 另一个坑：strbuf[2] 装 "%02d" 需要 3 字节（2 字符 + \0），栈上越界 1 字节；
 * 当前靠编译对齐侥幸未崩，属于必须记录的问题。
 */

///////////////////// Page Manager //////////////////
/**
 * @brief 把秒表页注册进页面管理器
 * @return 无
 * @note 从菜单“秒表”进入，右滑 Page_Back 返回。
 */
Page_t Page_Timer = {ui_TimerPage_screen_init, ui_TimerPage_screen_deinit, &ui_TimerPage};

///////////////////// VARIABLES ////////////////////
lv_obj_t * ui_TimerPage;
lv_obj_t * ui_TimerMeter;
lv_obj_t * ui_TimerStartBtn;
lv_obj_t * ui_TimerReBtn;
lv_obj_t * ui_TimerStartLabel;
lv_obj_t * ui_TimerReLabel;
lv_obj_t * ui_TimerMinLabel;
lv_obj_t * ui_TimerSecColonLabel;
lv_obj_t * ui_TimerSecLabel;
lv_obj_t * ui_TimerMilSecColonLabel;
lv_obj_t * ui_TimerMilSecLabel;

// 10 ms 周期定时器；创建后立即 pause，等用户按开始
lv_timer_t * ui_TimerPageTimer;

// 运行时状态：Flag=1 表示计时中；其余是拆分后的显示值
uint8_t ui_TimerPageFlag=0;
uint8_t ui_TimerPage_min=0;
uint8_t ui_TimerPage_sec=0;
uint8_t ui_TimerPage_10ms=0;
uint8_t ui_TimerPage_ms=0;
uint32_t ui_TimerPage_elapsed_ms=0;

///////////////////// ANIMATIONS ////////////////////


///////////////////// Data Init ////////////////////
/**
 * @brief 复位所有秒表状态变量
 * @return 无
 * @note 只在页面 init 时调用一次；不复位 LVGL 控件，控件文本由 init/复位按钮负责。
 */
static void ui_TimerPage_Data_init(void)
{
    ui_TimerPageFlag=0;
    ui_TimerPage_min=0;
    ui_TimerPage_sec=0;
	ui_TimerPage_10ms=0;
    ui_TimerPage_ms=0;
    ui_TimerPage_elapsed_ms=0;
}

/////////////////////// Timer //////////////////////
/**
 * @brief 10 ms 定时器：把 elapsed_ms 拆成 分/秒/厘秒 并刷新 Label
 * @param timer LVGL 定时器句柄（未使用）
 * @return 无
 * @note 回调运行在 GUI 任务上下文；暂停时 LVGL 不再调度本回调。
 */
static void LabelRefresh_timer(lv_timer_t * timer)
{
// 快照一次，避免下面多次读取时被其它上下文改动（当前只有本回调写）
    uint32_t elapsed = ui_TimerPage_elapsed_ms;
// strbuf[2] 偏小："%02d" 最多写 3 字节，越界 1 字节（见文件头说明）
    uint8_t strbuf[2];

// 整除取模拆时分秒；10ms 单位即厘秒，不是毫秒
    ui_TimerPage_min = (elapsed / 60000U) % 60U;
    ui_TimerPage_sec = (elapsed / 1000U) % 60U;
    ui_TimerPage_10ms = (elapsed / 10U) % 100U;
    ui_TimerPage_ms = elapsed % 10U;

    sprintf(strbuf,"%02d",ui_TimerPage_min);
    lv_label_set_text(ui_TimerMinLabel, strbuf);
    sprintf(strbuf,"%02d",ui_TimerPage_sec);
    lv_label_set_text(ui_TimerSecLabel, strbuf);
    sprintf(strbuf,"%02d",ui_TimerPage_10ms);
    lv_label_set_text(ui_TimerMilSecLabel, strbuf);


}

///////////////////// FUNCTIONS ////////////////////
/**
 * @brief 启停按钮：勾选=开始，取消=暂停
 * @param e LVGL 事件对象
 * @return 无
 * @note 按钮是 CHECKABLE，LVGL 自己维护勾选态；这里只负责切换图标与定时器。
 */
static void ui_event_TimerStartBtn(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED))
    {
        //checked
// 开始：标志置 1、图标换暂停、恢复 10 ms 定时器
        ui_TimerPageFlag = 1;
        lv_label_set_text(ui_TimerStartLabel, LV_SYMBOL_PAUSE);
        lv_timer_resume(ui_TimerPageTimer);

    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED))
    {
        //released
// 暂停：标志清 0、图标换播放、挂起定时器（保留已计时间）
        ui_TimerPageFlag = 0;
        lv_label_set_text(ui_TimerStartLabel, LV_SYMBOL_PLAY);
        lv_timer_pause(ui_TimerPageTimer);

    }
}

/**
 * @brief 复位按钮：仅在未计时时可用
 * @param e LVGL 事件对象
 * @return 无
 * @note 条件里的 !CHECKED 保证计时中按复位无效，防止误清零。
 */
static void ui_event_TimerReBtn(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED && !lv_obj_has_state(ui_TimerStartBtn, LV_STATE_CHECKED))
    {
        //clear
// 先暂停定时器再清显示，避免清零过程中回调又写回旧值
        lv_timer_pause(ui_TimerPageTimer);
        lv_label_set_text(ui_TimerMinLabel, "00");
        lv_label_set_text(ui_TimerSecLabel, "00");
        lv_label_set_text(ui_TimerMilSecLabel, "00");
        ui_TimerPage_elapsed_ms = 0;
        ui_TimerPage_min = 0;
        ui_TimerPage_sec = 0;
		ui_TimerPage_10ms = 0;
        ui_TimerPage_ms = 0;

    }

}

///////////////////// SCREEN init ////////////////////
/**
 * @brief 创建秒表页：meter 刻度 + 四个数字 Label + 两个按钮
 * @return 无
 * @note init 里先调 Data_init() 清零状态，保证每次进页都从 00:00.00 开始。
 */
void ui_TimerPage_screen_init(void)
{

// 先复位软件状态，再建控件，避免残留上次的计时值
    ui_TimerPage_Data_init();
    ui_TimerPage = lv_obj_create(NULL);

// meter 240x240 居中，作为整个表盘的背景
    ui_TimerMeter = lv_meter_create(ui_TimerPage);
    lv_obj_set_size(ui_TimerMeter, 240, 240);
    lv_obj_center(ui_TimerMeter);

// 去掉默认指示针：本项目只用刻度盘做装饰
    lv_obj_remove_style(ui_TimerMeter, NULL, LV_PART_INDICATOR);
// 61 个刻度、每 10 度一格、总跨度 360 度、从 270 度开始
    lv_meter_scale_t * scale_min = lv_meter_add_scale(ui_TimerMeter);
    lv_meter_set_scale_ticks(ui_TimerMeter, scale_min, 61, 1, 10, lv_palette_main(LV_PALETTE_BLUE));
    lv_meter_set_scale_range(ui_TimerMeter, scale_min, 0, 60, 360, 270);

// 开始/暂停按钮：60x60 圆形，半透明，位于表盘右下
    ui_TimerStartBtn = lv_btn_create(ui_TimerMeter);
    lv_obj_set_width(ui_TimerStartBtn, 60);
    lv_obj_set_height(ui_TimerStartBtn, 60);
    lv_obj_set_style_radius(ui_TimerStartBtn, 50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_TimerStartBtn, 200, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_align(ui_TimerStartBtn, LV_ALIGN_CENTER);
    lv_obj_set_pos(ui_TimerStartBtn,40,50);
    lv_obj_add_flag(ui_TimerStartBtn, LV_OBJ_FLAG_CHECKABLE | LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_TimerStartBtn, LV_OBJ_FLAG_SCROLLABLE);      /// Flags


// 复位按钮：同样 60x60，位于左下
    ui_TimerReBtn = lv_btn_create(ui_TimerMeter);
    lv_obj_set_width(ui_TimerReBtn, 60);
    lv_obj_set_height(ui_TimerReBtn, 60);
    lv_obj_set_style_radius(ui_TimerReBtn, 50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_TimerReBtn, 200, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_align(ui_TimerReBtn, LV_ALIGN_CENTER);
    lv_obj_set_pos(ui_TimerReBtn,-40,50);

    ui_TimerStartLabel = lv_label_create(ui_TimerStartBtn);
    lv_label_set_text(ui_TimerStartLabel, LV_SYMBOL_PLAY);//LV_SYMBOL_PAUSE
    lv_obj_center(ui_TimerStartLabel);
    lv_obj_set_style_text_font(ui_TimerStartLabel, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_TimerReLabel = lv_label_create(ui_TimerReBtn);
    lv_label_set_text(ui_TimerReLabel, LV_SYMBOL_REFRESH);
    lv_obj_center(ui_TimerReLabel);
    lv_obj_set_style_text_font(ui_TimerReLabel, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);

// 时间文本：分、冒号、秒、点、厘秒依次排开，整体左移
    ui_TimerMinLabel = lv_label_create(ui_TimerMeter);
    lv_obj_set_align(ui_TimerMinLabel, LV_ALIGN_CENTER);
    lv_obj_set_pos(ui_TimerMinLabel,-60,-20);
    lv_label_set_text(ui_TimerMinLabel, "00");
    lv_obj_set_style_text_font(ui_TimerMinLabel, &ui_font_Cuyuan38, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_TimerSecColonLabel = lv_label_create(ui_TimerMeter);
    lv_obj_set_align(ui_TimerSecColonLabel, LV_ALIGN_CENTER);
    lv_obj_set_pos(ui_TimerSecColonLabel,-30,-20);
    lv_label_set_text(ui_TimerSecColonLabel, ":");
    lv_obj_set_style_text_font(ui_TimerSecColonLabel, &ui_font_Cuyuan38, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_TimerSecLabel = lv_label_create(ui_TimerMeter);
    lv_obj_set_align(ui_TimerSecLabel, LV_ALIGN_CENTER);
    lv_obj_set_pos(ui_TimerSecLabel,0,-20);
    lv_label_set_text(ui_TimerSecLabel, "00");
    lv_obj_set_style_text_font(ui_TimerSecLabel, &ui_font_Cuyuan38, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_TimerMilSecColonLabel = lv_label_create(ui_TimerMeter);
    lv_obj_set_align(ui_TimerMilSecColonLabel, LV_ALIGN_CENTER);
    lv_obj_set_pos(ui_TimerMilSecColonLabel,30,-20);
    lv_label_set_text(ui_TimerMilSecColonLabel, ".");
    lv_obj_set_style_text_font(ui_TimerMilSecColonLabel, &ui_font_Cuyuan38, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_TimerMilSecLabel = lv_label_create(ui_TimerMeter);
    lv_obj_set_align(ui_TimerMilSecLabel, LV_ALIGN_CENTER);
    lv_obj_set_pos(ui_TimerMilSecLabel,60,-20);
    lv_label_set_text(ui_TimerMilSecLabel, "00");
    lv_obj_set_style_text_font(ui_TimerMilSecLabel, &ui_font_Cuyuan38, LV_PART_MAIN | LV_STATE_DEFAULT);

    //events
// 事件绑定
    lv_obj_add_event_cb(ui_TimerStartBtn, ui_event_TimerStartBtn, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_TimerReBtn, ui_event_TimerReBtn, LV_EVENT_ALL, NULL);

    //timer
// 10 ms 定时器：厘秒级刷新；先暂停等用户点开始
    ui_TimerPageTimer = lv_timer_create(LabelRefresh_timer, 10,  NULL);
    lv_timer_pause(ui_TimerPageTimer);

}

//////////////////// SCREEN deinit ////////////////////
/**
 * @brief 秒表页销毁：空实现（有隐患）
 * @return 无
 * @note ui_TimerPageTimer 没有在这里删除，离开页面后定时器仍在跑并访问已释放的 Label。
 *       正确做法是 lv_timer_del(ui_TimerPageTimer)，但按“零逻辑改动”要求本次不修。
 */
void ui_TimerPage_screen_deinit(void)
{}
