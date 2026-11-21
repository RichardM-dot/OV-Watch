#include "../../ui.h"
#include "../Inc/ui_ChargPage.h"
#include "../Inc/ui_HomePage.h"
#include "../../../Func/Inc/HWDataAccess.h"
/*
 * 【中文导读】ui_ChargPage.c —— 充电页（插入充电器时自动弹出）
 * ----------------------------------------------------------------------------
 * 布局很简洁：左侧一个 60x60 电量圆弧 + 电池图标 + 百分比，右侧两行 100 号大字
 * 显示“时:分”，冒号固定。整页绿色系（0x64C864），与充电状态呼应。
 * 
 * 进入路径：ChargCheckTask 检测到充电插入后调 Page_Load(&Page_Charg)（见
 * Tasks/Src/user_ChargCheckTask.c）；退出由拔出充电器触发 Page_Back_Bottom。
 * 
 * 数据刷新：2 s 定时器，比首页慢 4 倍——充电页只需要看大致趋势，省电。
 * 电量用 HWInterface.Power.BatCalculate() 现场重算 ADC，而不是读缓存字段。
 * 
 * 注意：本页与首页共用 ui_TimeHourValue / ui_TimeMinuteValue 两个全局缓存，
 * 所以两边的时间显示天然同步；但也意味着跨页写同一变量，不能并发调用。
 */

///////////////////// Page Manager //////////////////
/**
 * @brief 把充电页注册进页面管理器
 * @return 无
 * @note 由 ChargCheckTask 触发 Page_Load，不走菜单。
 */
Page_t Page_Charg = {ui_ChargPage_screen_init, ui_ChargPage_screen_deinit, &ui_ChargPage};

///////////////////// VARIABLES ////////////////////
lv_obj_t * ui_ChargPage;
lv_obj_t * ui_CharPageBatArc;
lv_obj_t * ui_ChargPageBatIcon;
lv_obj_t * ui_ChargPageBatNum;
lv_obj_t * ui_ChargPagebHourLabel;
lv_obj_t * ui_ChargPagebMinLabel;
lv_obj_t * ui_ChargPageTimePoint;

// 2 s 周期定时器；页面销毁时必须删，否则回调访问已释放控件
lv_timer_t * ui_ChargPageTimer;

///////////////////// Data Init ////////////////////


/////////////////////// Timer //////////////////////
// need to be destroyed when the page is destroyed
/**
 * @brief 充电页定时器：刷新时间与电量
 * @param timer LVGL 定时器句柄（未使用）
 * @return 无
 * @note 只在充电页位于栈顶时刷新；2 s 周期是为了降低充电时的整机功耗。
 */
static void ChargPage_timer_cb(lv_timer_t * timer)
{
// 页面可见性判断：不在栈顶就整段跳过
    if(Page_Get_NowPage()->page_obj == &ui_ChargPage)
		{
			uint8_t value_strbuf[5];

			HW_DateTimeTypeDef DateTime;
      HWInterface.RealTimeClock.GetTimeDate(&DateTime);

// 时间复用首页的两个全局缓存，变化才刷，减少 LVGL 重绘
			if(ui_TimeMinuteValue != DateTime.Minutes)
			{
				ui_TimeMinuteValue = DateTime.Minutes;
				sprintf(value_strbuf,"%02d",ui_TimeMinuteValue);
				lv_label_set_text(ui_ChargPagebMinLabel, value_strbuf);
			}

			if(ui_TimeHourValue != DateTime.Hours)
			{
				ui_TimeHourValue = DateTime.Hours;
				sprintf(value_strbuf,"%02d",ui_TimeHourValue);
				lv_label_set_text(ui_ChargPagebHourLabel, value_strbuf);
			}

// 每 2 s 重新采样 ADC 算电量；BatCalculate 内部有滤波
			ui_BatArcValue = HWInterface.Power.BatCalculate();
// 只接受 1~100，异常值（0 或 >100）当作 0 处理，避免圆弧越界
			if(ui_BatArcValue>0 && ui_BatArcValue<=100)
			{
				lv_arc_set_value(ui_CharPageBatArc, ui_BatArcValue);
				sprintf(value_strbuf,"%2d%%",ui_BatArcValue);
				lv_label_set_text(ui_ChargPageBatNum, value_strbuf);
			}
			else
			{ui_BatArcValue=0;}

		}
}


/*
 * 创建充电页控件树
 *   ui_ChargPage（screen）
 *    ├─ ui_CharPageBatArc   60x60 圆弧，背景弧 270~629 度（近整圆）
 *    ├─ ui_ChargPageBatIcon 电池图标（iconfont24）
 *    ├─ ui_ChargPageBatNum  百分比文本（Cuyuan24）
 *    ├─ ui_ChargPagebHourLabel 小时（Cuyuan100）
 *    ├─ ui_ChargPagebMinLabel  分钟（Cuyuan100）
 *    └─ ui_ChargPageTimePoint  冒号（透明度 160，弱化）
 * 左半屏是电量、右半屏是时间，中间留白，视觉上互不干扰。
 */
///////////////////// SCREEN init ////////////////////
/**
 * @brief 创建充电页并启动 2 s 刷新定时器
 * @return 无
 * @note 无参数；重复调用会重复建控件与定时器。
 */
void ui_ChargPage_screen_init(void)
{
		uint8_t value_strbuf[5];
// screen 根对象，尺寸随显示屏
    ui_ChargPage = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_ChargPage, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

// 电量圆弧：8 px 线宽，绿色；旋钮 pad=0 撑满弧宽
    ui_CharPageBatArc = lv_arc_create(ui_ChargPage);
    lv_obj_set_width(ui_CharPageBatArc, 60);
    lv_obj_set_height(ui_CharPageBatArc, 60);
    lv_obj_set_x(ui_CharPageBatArc, -70);
    lv_obj_set_y(ui_CharPageBatArc, -70);
    lv_obj_set_align(ui_CharPageBatArc, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_CharPageBatArc, LV_OBJ_FLAG_CLICKABLE);      /// Flags
    lv_arc_set_value(ui_CharPageBatArc, ui_BatArcValue);
    lv_arc_set_bg_angles(ui_CharPageBatArc, 270, 629);
    lv_obj_set_style_arc_width(ui_CharPageBatArc, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_arc_color(ui_CharPageBatArc, lv_color_hex(0x64C864), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(ui_CharPageBatArc, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(ui_CharPageBatArc, 8, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_color(ui_CharPageBatArc, lv_color_hex(0x32C832), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_CharPageBatArc, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui_CharPageBatArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui_CharPageBatArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui_CharPageBatArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui_CharPageBatArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

// 电池图标叠在圆弧中心，与圆弧同心
    ui_ChargPageBatIcon = lv_label_create(ui_ChargPage);
    lv_obj_set_width(ui_ChargPageBatIcon, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_ChargPageBatIcon, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_ChargPageBatIcon, -70);
    lv_obj_set_y(ui_ChargPageBatIcon, -70);
    lv_obj_set_align(ui_ChargPageBatIcon, LV_ALIGN_CENTER);
    lv_label_set_text(ui_ChargPageBatIcon, "");
    lv_obj_set_style_text_color(ui_ChargPageBatIcon, lv_color_hex(0x64C864), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_ChargPageBatIcon, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_ChargPageBatIcon, &ui_font_iconfont24, LV_PART_MAIN | LV_STATE_DEFAULT);

// 百分比数字放在圆弧下方 45 px 处
    ui_ChargPageBatNum = lv_label_create(ui_ChargPage);
    lv_obj_set_width(ui_ChargPageBatNum, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_ChargPageBatNum, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_ChargPageBatNum, -70);
    lv_obj_set_y(ui_ChargPageBatNum, -25);
    lv_obj_set_align(ui_ChargPageBatNum, LV_ALIGN_CENTER);
		sprintf(value_strbuf,"%2d%%",ui_BatArcValue);
    lv_label_set_text(ui_ChargPageBatNum, value_strbuf);
    lv_obj_set_style_text_color(ui_ChargPageBatNum, lv_color_hex(0x64C864), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_ChargPageBatNum, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_ChargPageBatNum, &ui_font_Cuyuan24, LV_PART_MAIN | LV_STATE_DEFAULT);

// 小时用 100 号字，几乎占满右半屏高度
    ui_ChargPagebHourLabel = lv_label_create(ui_ChargPage);
    lv_obj_set_width(ui_ChargPagebHourLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_ChargPagebHourLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_ChargPagebHourLabel, 50);
    lv_obj_set_y(ui_ChargPagebHourLabel, -65);
    lv_obj_set_align(ui_ChargPagebHourLabel, LV_ALIGN_CENTER);
		sprintf(value_strbuf,"%02d",ui_TimeHourValue);
    lv_label_set_text(ui_ChargPagebHourLabel, value_strbuf);
    lv_obj_set_style_text_color(ui_ChargPagebHourLabel, lv_color_hex(0x64C864), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_ChargPagebHourLabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_ChargPagebHourLabel, &ui_font_Cuyuan100, LV_PART_MAIN | LV_STATE_DEFAULT);

// 分钟同理，位于小时下方
    ui_ChargPagebMinLabel = lv_label_create(ui_ChargPage);
    lv_obj_set_width(ui_ChargPagebMinLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_ChargPagebMinLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_ChargPagebMinLabel, 50);
    lv_obj_set_y(ui_ChargPagebMinLabel, 60);
    lv_obj_set_align(ui_ChargPagebMinLabel, LV_ALIGN_CENTER);
		sprintf(value_strbuf,"%02d",ui_TimeMinuteValue);
    lv_label_set_text(ui_ChargPagebMinLabel, value_strbuf);
    lv_obj_set_style_text_color(ui_ChargPagebMinLabel, lv_color_hex(0x64C864), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_ChargPagebMinLabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_ChargPagebMinLabel, &ui_font_Cuyuan100, LV_PART_MAIN | LV_STATE_DEFAULT);

// 冒号单独成 Label，透明度 160 做弱化
    ui_ChargPageTimePoint = lv_label_create(ui_ChargPage);
    lv_obj_set_width(ui_ChargPageTimePoint, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_ChargPageTimePoint, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_ChargPageTimePoint, -35);
    lv_obj_set_y(ui_ChargPageTimePoint, 50);
    lv_obj_set_align(ui_ChargPageTimePoint, LV_ALIGN_CENTER);
    lv_label_set_text(ui_ChargPageTimePoint, ":");
    lv_obj_set_style_text_color(ui_ChargPageTimePoint, lv_color_hex(0x64C864), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_ChargPageTimePoint, 160, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_ChargPageTimePoint, &ui_font_Cuyuan100, LV_PART_MAIN | LV_STATE_DEFAULT);

    //timer
// 2 s 刷新：充电页不需要秒级精度
    ui_ChargPageTimer = lv_timer_create(ChargPage_timer_cb, 2000,  NULL);

}

/////////////////// SCREEN deinit ////////////////////
/**
 * @brief 充电页销毁：删除定时器
 * @return 无
 * @note 与首页同理，只删定时器；screen 由 lv_scr_load_anim 的 auto_del 回收。
 */
void ui_ChargPage_screen_deinit(void)
{
  lv_timer_del(ui_ChargPageTimer);
}
