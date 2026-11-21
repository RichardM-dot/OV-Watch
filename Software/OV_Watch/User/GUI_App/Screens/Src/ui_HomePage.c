#include "../../ui.h"
#include "../../ui_helpers.h"
#include "../Inc/ui_HomePage.h"
#include "../Inc/ui_MenuPage.h"
#include "../Inc/ui_HRPage.h"
#include "../Inc/ui_SetPage.h"

#include "../../../Func/Inc/HWDataAccess.h"
/*
 * 【中文导读】ui_HomePage.c —— 表盘首页 + 关机页（本项目最重要的 UI 文件）
 * ----------------------------------------------------------------------------
 * 首页是整个系统的“驾驶舱”，一块 240x280 的屏幕同时显示 6 类信息：
 *   时间（时/分两个 Label，中间冒号固定不动）、日期与星期、电量圆弧、
 *   今日步数与进度条、温度圆弧、湿度圆弧、心率圆弧。
 * 下方还挂着一个可下拉的快捷面板（DropDownPanel），内含 NFC / BLE / 关机 /
 * 设置四个按钮和一条背光滑块；关机页 PowerPage 是独立的 lv_obj 屏幕。
 * 
 * 数据来源全部是 HWInterface（见 Func/Inc/HWDataAccess.h）：
 *   RealTimeClock.GetTimeDate → 时间/日期；Power.power_remain → 电量；
 *   IMU.Steps → 步数；AHT21.temperature/humidity → 温湿度；HR_meter.HrRate → 心率。
 * UI 自己不采样，只在 500 ms 定时器里“比对缓存值，变了才刷”，避免无谓重绘。
 * 
 * 页面生命周期：Page_t Page_Home 注册到 PageManager；init 创建控件与定时器，
 * deinit 只删定时器（屏幕对象由 lv_scr_load_anim 的 auto_del 回收）。
 * 
 * 阅读顺序：变量分区 → HomePage_timer_cb（数据刷新）→ 各 ui_event_*（交互）
 *           → ui_HomePage_screen_init（控件树）→ ui_PowerPage_screen_init。
 */


///////////////////// VARIABLES ////////////////////
//home page
lv_obj_t * ui_HomePage;
lv_obj_t * ui_TimeHourLabel;
lv_obj_t * ui_TimeColonLabel;
lv_obj_t * ui_TimeMinuteLabel;
lv_obj_t * ui_BatArc;
lv_obj_t * ui_BaticonLabel;
lv_obj_t * ui_BatNumLabel;
lv_obj_t * ui_DateLabel;
lv_obj_t * ui_DayLabel;
lv_obj_t * ui_StepiconLabel;
lv_obj_t * ui_StepCnLabel;
lv_obj_t * ui_StepNumLabel;
lv_obj_t * ui_StepNumBar;
lv_obj_t * ui_TempArc;
lv_obj_t * ui_TempiconLabel;
lv_obj_t * ui_TempNumLabel;
lv_obj_t * ui_HumiiconLabel;
lv_obj_t * ui_HumiArc;
lv_obj_t * ui_HumiNumLabel;
lv_obj_t * ui_HRArc;
lv_obj_t * ui_HRiconLabel;
lv_obj_t * ui_HRNumLabel;
//drop down
lv_obj_t * ui_DropDownPanel;
lv_obj_t * ui_UpBGPanel;
lv_obj_t * ui_NFCButton;
lv_obj_t * ui_NFCLabel;
lv_obj_t * ui_BLEButton;
lv_obj_t * ui_BLELabel;
lv_obj_t * ui_PowerButton;
lv_obj_t * ui_PowerLabel;
lv_obj_t * ui_SetButton;
lv_obj_t * ui_SetLabel;
lv_obj_t * ui_LightSlider;
lv_obj_t * ui_LightLabel;
lv_obj_t * ui_DownBGPanel;
//power slider
lv_obj_t * ui_PowerPage;
lv_obj_t * ui_PowerSlider;
lv_obj_t * ui_PowerDownLabel;

lv_timer_t * ui_HomePageTimer;

// ── 显示缓存：保存“上一次画到屏幕上的值”，与硬件真值比较后才刷新 ──
// variables
// 时间：初值只是占位，首个 500 ms 定时器周期就会被 RTC 真值覆盖
uint8_t ui_TimeHourValue = 11;
uint8_t ui_TimeMinuteValue = 59;
const char * ui_Days[7] = {"Mon.", "Tue.", "Wed.", "Thu.", "Fri.", "Sat.", "Sun."};
uint8_t ui_DateMonthValue = 11;
uint8_t ui_DateDayValue = 8;
uint8_t ui_DataWeekdayValue = 2;

// 电量/步数/背光：背光滑块的值会回写到 HWInterface.LCD.SetLight
uint8_t ui_BatArcValue = 70;
uint16_t ui_StepNumValue = 0;
uint8_t ui_LightSliderValue = 50;

uint8_t ui_HomePageBLEEN = 0;
uint8_t ui_HomePageNFCEN = 0;

int8_t ui_HomePageTempValue = 25;
int8_t ui_HomePageHumiValue = 67;


///////////////////// Page Manager //////////////////
/**
 * @brief 把首页与关机页注册进页面管理器
 * @return 无
 * @note Page_t 的第三个成员是“屏幕对象指针的地址”，PageManager 用 *page_obj 取 lv_obj_t*。
 */
Page_t Page_Home = {ui_HomePage_screen_init, ui_HomePage_screen_deinit, &ui_HomePage};
Page_t Page_Power = {ui_PowerPage_screen_init, ui_PowerPage_screen_deinit, &ui_PowerPage};

/////////////////// private timer ///////////////////
// need to be destroyed when the page is destroyed
/**
 * @brief 首页私有定时器：每 500 ms 把 HWInterface 里的最新值搬到控件上
 * @param timer LVGL 定时器句柄（未使用）
 * @return 无
 * @note 由 LVGL 的 GUI 任务上下文调用（LvHandlerTask），因此可以直接操作控件；只在首页位于栈顶时工作，后台页面不浪费重绘。
 */
static void HomePage_timer_cb(lv_timer_t * timer)
{
  uint8_t value_strbuf[10];
// 当前页不是首页时直接跳过：Page_Get_NowPage 返回栈顶 Page_t
  if(Page_Get_NowPage()->page_obj  == &ui_HomePage)
	{
// 被注释掉的冒号闪烁实现：osDelay(500) 会阻塞 GUI 任务，已废弃
			/*
			lv_obj_set_style_text_opa(ui_TimeColonLabel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
			osDelay(500);
			lv_obj_set_style_text_opa(ui_TimeColonLabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
			*/

// 一次 RTC 读取拿全 7 个字段，避免多次读寄存器出现跨秒撕裂
			HW_DateTimeTypeDef DateTime;
      HWInterface.RealTimeClock.GetTimeDate(&DateTime);

// 分钟变化：两位补零，视觉上宽度稳定不跳动
			if(ui_TimeMinuteValue != DateTime.Minutes)
			{
				ui_TimeMinuteValue = DateTime.Minutes;
				sprintf(value_strbuf,"%02d",ui_TimeMinuteValue);
				lv_label_set_text(ui_TimeMinuteLabel, value_strbuf);
			}

// 小时用 %2d（空格对齐）而不是 %02d，表盘上不显示前导 0
			if(ui_TimeHourValue != DateTime.Hours)
			{
				ui_TimeHourValue = DateTime.Hours;
				sprintf(value_strbuf,"%2d",ui_TimeHourValue);
				lv_label_set_text(ui_TimeHourLabel, value_strbuf);
			}

// 日期变化时同步刷新星期；WeekDay 是 1~7，数组下标要减 1
			if(ui_DateDayValue != DateTime.Date)
			{
				ui_DateDayValue = DateTime.Date;
				ui_DataWeekdayValue = DateTime.WeekDay;
				sprintf(value_strbuf,"%2d-%02d",ui_DateMonthValue,ui_DateDayValue);
				lv_label_set_text(ui_DateLabel, value_strbuf);
				lv_label_set_text(ui_DayLabel, ui_Days[ui_DataWeekdayValue-1]);

			}
// 月份变化单独判断：跨月时日期与星期必须一起更新
			if(ui_DateMonthValue != DateTime.Month)
			{
				ui_DateMonthValue = DateTime.Month;
				ui_DateDayValue = DateTime.Date;
				ui_DataWeekdayValue = DateTime.WeekDay;
				sprintf(value_strbuf,"%2d-%02d",ui_DateMonthValue,ui_DateDayValue);
				lv_label_set_text(ui_DateLabel, value_strbuf);
				lv_label_set_text(ui_DayLabel, ui_Days[ui_DataWeekdayValue-1]);
			}

// 电量：圆弧 0~100，数值文本带百分号
      if(ui_BatArcValue != HWInterface.Power.power_remain)
      {
        ui_BatArcValue = HWInterface.Power.power_remain;
        lv_arc_set_value(ui_BatArc, ui_BatArcValue);
				sprintf(value_strbuf,"%2d%%",ui_BatArcValue);
				lv_label_set_text(ui_BatNumLabel, value_strbuf);
      }

// 步数：只更新数字，进度条 StepNumBar 在本文件里没有跟着走（固定 40）
      if(ui_StepNumValue != HWInterface.IMU.Steps)
      {
        ui_StepNumValue = HWInterface.IMU.Steps;
        sprintf(value_strbuf,"%d",ui_StepNumValue);
				lv_label_set_text(ui_StepNumLabel, value_strbuf);
      }

// 温度：先转 int8_t 再比较，传感器返回 float，避免 0.1 度级抖动引发重绘
      if( ui_HomePageTempValue != (int8_t)HWInterface.AHT21.temperature )
      {
        ui_HomePageTempValue = (int8_t)HWInterface.AHT21.temperature;
        lv_arc_set_value(ui_TempArc, ui_HomePageTempValue);
        sprintf(value_strbuf,"%d",ui_HomePageTempValue);
				lv_label_set_text(ui_TempNumLabel, value_strbuf);
      }

// 湿度同理；HumiArc 的背景弧被设成 270~620 度，视觉上是“缺口圆环”
      if( ui_HomePageHumiValue != (int8_t)HWInterface.AHT21.humidity )
      {
        ui_HomePageHumiValue = (int8_t)HWInterface.AHT21.humidity;
        lv_arc_set_value(ui_HumiArc, ui_HomePageHumiValue);
				sprintf(value_strbuf,"%d",ui_HomePageHumiValue);
				lv_label_set_text(ui_HumiNumLabel, value_strbuf);
      }

	}
}


///////////////////// FUNCTIONS ////////////////////
/**
 * @brief 首页手势：右滑返回菜单页
 * @param e LVGL 事件对象
 * @return 无
 * @note 只处理 LV_EVENT_GESTURE；target 变量未使用，保留是 SquareLine 的模板痕迹。
 */
void ui_event_HomePage(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);

    if(event_code == LV_EVENT_GESTURE)
    {
			if(lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_RIGHT)
      {
				Page_Load(&Page_Menu);
      }
		}
}

/**
 * @brief 下拉面板 NFC 开关：记录开关状态供页面重建时恢复
 * @param e LVGL 事件对象
 * @return 无
 * @note 真正驱动 NFC 的 ICcard_Select() 已被注释，当前只维护 ui_HomePageNFCEN 标志。
 */
void ui_event_NFCButton(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED))
    {
        //checked
        ui_HomePageNFCEN=1;
				//ICcard_Select(1);
    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED))
    {
        //released
        ui_HomePageNFCEN=0;
				//ICcard_Select(0);
    }
}

/**
 * @brief 下拉面板蓝牙开关：勾选即调 KT6328 的 Enable/Disable
 * @param e LVGL 事件对象
 * @return 无
 * @note BLE 开关是“立即生效”型，不做二次确认；状态存 ui_HomePageBLEEN 供重建恢复。
 */
void ui_event_BLEButton(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_VALUE_CHANGED &&  lv_obj_has_state(target, LV_STATE_CHECKED))
    {
        //checked
        HWInterface.BLE.Enable();
        ui_HomePageBLEEN=1;

    }
    if(event_code == LV_EVENT_VALUE_CHANGED &&  !lv_obj_has_state(target, LV_STATE_CHECKED))
    {
        //released
				HWInterface.BLE.Disable();
        ui_HomePageBLEEN=0;
    }
}

/**
 * @brief 电源按钮：进入滑动关机页
 * @param e LVGL 事件对象
 * @return 无
 * @note 走 Page_Load(&Page_Power) 入栈，返回时 Page_Back 会重建首页。
 */
void ui_event_PowerButton(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED)
    {
			//power slider
      Page_Load(&Page_Power);
			// ui_PowerPage_screen_init();
			// lv_scr_load_anim(ui_PowerPage,LV_SCR_LOAD_ANIM_MOVE_RIGHT,0,0,true);
			// user_Stack_Push(&ScrRenewStack,(long long int)&ui_PowerPage);
    }
}

/**
 * @brief 关机滑块：滑到 >=90 才执行关机，防误触
 * @param e LVGL 事件对象
 * @return 无
 * @note 阈值 90 是硬编码；Power.Shutdown() 之后不会返回（切断电源保持）。
 */
void ui_event_PowerSlider(lv_event_t * e)
{
		lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_VALUE_CHANGED)
    {

    }
		if(event_code == LV_EVENT_CLICKED)
		{
			//power down if slider value >= 90
			if(lv_slider_get_value(ui_PowerSlider) >=90)
			{
        HWInterface.Power.Shutdown();
			}
		}
}

/**
 * @brief 设置按钮：进入设置页
 * @param e LVGL 事件对象
 * @return 无
 */
void ui_event_SetButton(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED)
    {
			Page_Load(&Page_Set);
    }
}

/**
 * @brief 背光滑块：拖动即实时改 PWM 占空比
 * @param e LVGL 事件对象
 * @return 无
 * @note 没有做防抖：LVGL 拖动时每个 VALUE_CHANGED 都写一次 TIM3 比较寄存器，频率约几十 Hz，对本项目无影响。
 */
void ui_event_LightSlider(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_VALUE_CHANGED)
    {
        ui_LightSliderValue = lv_slider_get_value(ui_LightSlider);
        HWInterface.LCD.SetLight(ui_LightSliderValue);
    }
}

///////////////////// SCREEN init ////////////////////
/**
 * @brief 创建首页全部控件、绑定事件并启动 500 ms 刷新定时器
 * @return 无
 * @note 函数无参数无返回值；重复调用会重复创建控件与定时器，必须由 PageManager 保证只调一次。
 */
/*
 * 创建首页控件树（SquareLine 导出后手工改过）
 * 控件层级：ui_HomePage（screen）
 *   ├─ 时间三件套：Hour / Colon / Minute（右对齐，48 号字）
 *   ├─ 顶部信息：BatArc + 电量数字 + 日期 + 星期
 *   ├─ 中部：步数图标/文字/数字 + 步数进度条
 *   ├─ 底部三圆环：Temp / Humi / HR（各带图标与数字）
 *   └─ DropDownPanel（可滚动容器）
 *        ├─ UpBGPanel：NFC / BLE 两个开关
 *        ├─ PowerButton / SetButton / LightSlider
 *        └─ DownBGPanel：底部留白，保证能滚出面板
 * 坐标全部是相对父对象的偏移，配合 LV_ALIGN_* 对齐；改布局时先确认父对象。
 */
void ui_HomePage_screen_init(void)
{
// 清空菜单页记住的滚动偏移，保证每次回首页都从顶部开始
		ui_MenuScrollY = 0;
    uint8_t value_strbuf[10];

// screen 对象：LVGL 的根容器，尺寸自动等于显示屏 240x280
    ui_HomePage = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_HomePage, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

// 时间：右对齐布局，时/分各自独立成 Label，方便只改其中一个
    ui_TimeHourLabel = lv_label_create(ui_HomePage);
    lv_obj_set_width(ui_TimeHourLabel, LV_SIZE_CONTENT);   /// 10
    lv_obj_set_height(ui_TimeHourLabel, LV_SIZE_CONTENT);    /// 10
    lv_obj_set_x(ui_TimeHourLabel, -105);
    lv_obj_set_y(ui_TimeHourLabel, -70);
    lv_obj_set_align(ui_TimeHourLabel, LV_ALIGN_RIGHT_MID);
    sprintf(value_strbuf,"%2d",ui_TimeHourValue);
    lv_label_set_text(ui_TimeHourLabel, value_strbuf);
    lv_obj_set_style_text_font(ui_TimeHourLabel, &ui_font_Cuyuan48, LV_PART_MAIN | LV_STATE_DEFAULT);

// 冒号单独成 Label：颜色更暗（0x808080），起视觉分隔作用
    ui_TimeColonLabel = lv_label_create(ui_HomePage);
    lv_obj_set_width(ui_TimeColonLabel, LV_SIZE_CONTENT);   /// 10
    lv_obj_set_height(ui_TimeColonLabel, LV_SIZE_CONTENT);    /// 10
    lv_obj_set_x(ui_TimeColonLabel, -82);
    lv_obj_set_y(ui_TimeColonLabel, -70);
    lv_obj_set_align(ui_TimeColonLabel, LV_ALIGN_RIGHT_MID);
    lv_label_set_text(ui_TimeColonLabel, ":");
    lv_obj_set_style_text_color(ui_TimeColonLabel, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_TimeColonLabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_TimeColonLabel, &ui_font_Cuyuan48, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_TimeMinuteLabel = lv_label_create(ui_HomePage);
    lv_obj_set_width(ui_TimeMinuteLabel, LV_SIZE_CONTENT);   /// 10
    lv_obj_set_height(ui_TimeMinuteLabel, LV_SIZE_CONTENT);    /// 10
    lv_obj_set_x(ui_TimeMinuteLabel, -20);
    lv_obj_set_y(ui_TimeMinuteLabel, -70);
    lv_obj_set_align(ui_TimeMinuteLabel, LV_ALIGN_RIGHT_MID);
    sprintf(value_strbuf,"%02d",ui_TimeMinuteValue);
    lv_label_set_text(ui_TimeMinuteLabel, value_strbuf);
    lv_obj_set_style_text_font(ui_TimeMinuteLabel, &ui_font_Cuyuan48, LV_PART_MAIN | LV_STATE_DEFAULT);

// 电量圆弧：50x50，绿色；旋钮被 pad=0 撑满，视觉上像圆点
    ui_BatArc = lv_arc_create(ui_HomePage);
    lv_obj_set_width(ui_BatArc, 50);
    lv_obj_set_height(ui_BatArc, 50);
    lv_obj_set_x(ui_BatArc, -75);
    lv_obj_set_y(ui_BatArc, -68);
    lv_obj_set_align(ui_BatArc, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_BatArc, LV_OBJ_FLAG_CLICKABLE);      /// Flags
    lv_arc_set_value(ui_BatArc, ui_BatArcValue);
    lv_obj_set_style_arc_width(ui_BatArc, 5, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_arc_color(ui_BatArc, lv_color_hex(0x19C819), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(ui_BatArc, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(ui_BatArc, 5, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_color(ui_BatArc, lv_color_hex(0x19C819), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_BatArc, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui_BatArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui_BatArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui_BatArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui_BatArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

// 电池图标用 iconfont 的私有码位，字体文件在 GUI_App/Fonts
    ui_BaticonLabel = lv_label_create(ui_HomePage);
    lv_obj_set_width(ui_BaticonLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_BaticonLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_BaticonLabel, -75);
    lv_obj_set_y(ui_BaticonLabel, -50);
    lv_obj_set_align(ui_BaticonLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_BaticonLabel, "");
    lv_obj_set_style_text_color(ui_BaticonLabel, lv_color_hex(0x19C819), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_BaticonLabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_BaticonLabel, &ui_font_iconfont16, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_BatNumLabel = lv_label_create(ui_HomePage);
    lv_obj_set_width(ui_BatNumLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_BatNumLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_BatNumLabel, -75);
    lv_obj_set_y(ui_BatNumLabel, -68);
    lv_obj_set_align(ui_BatNumLabel, LV_ALIGN_CENTER);
    sprintf(value_strbuf,"%2d%%",ui_BatArcValue);
    lv_label_set_text(ui_BatNumLabel, value_strbuf);
    lv_obj_set_style_text_font(ui_BatNumLabel, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

// 日期 "月-日"：两位月 + 两位日，红色强调
    ui_DateLabel = lv_label_create(ui_HomePage);
    lv_obj_set_width(ui_DateLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_DateLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_DateLabel, -75);
    lv_obj_set_y(ui_DateLabel, -110);
    lv_obj_set_align(ui_DateLabel, LV_ALIGN_RIGHT_MID);
    sprintf(value_strbuf,"%2d-%02d",ui_DateMonthValue,ui_DateDayValue);
    lv_label_set_text(ui_DateLabel, value_strbuf);
    lv_obj_set_style_text_color(ui_DateLabel, lv_color_hex(0xFF461E), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_DateLabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_DateLabel, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

// 星期文本直接查 ui_Days 表，避免运行时算蔡勒公式
    ui_DayLabel = lv_label_create(ui_HomePage);
    lv_obj_set_width(ui_DayLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_DayLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_DayLabel, -20);
    lv_obj_set_y(ui_DayLabel, -110);
    lv_obj_set_align(ui_DayLabel, LV_ALIGN_RIGHT_MID);
    lv_label_set_text(ui_DayLabel, ui_Days[ui_DataWeekdayValue-1]);
    lv_obj_set_style_text_color(ui_DayLabel, lv_color_hex(0xFF461E), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_DayLabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_DayLabel, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

// 步数区：图标 + 中文标签 + 数字，颜色统一蓝色 0x3278FF
    ui_StepiconLabel = lv_label_create(ui_HomePage);
    lv_obj_set_width(ui_StepiconLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_StepiconLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_StepiconLabel, -90);
    lv_obj_set_y(ui_StepiconLabel, -5);
    lv_obj_set_align(ui_StepiconLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_StepiconLabel, "");
    lv_obj_set_style_text_color(ui_StepiconLabel, lv_color_hex(0x3278FF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_StepiconLabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_StepiconLabel, &ui_font_iconfont24, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_StepCnLabel = lv_label_create(ui_HomePage);
    lv_obj_set_width(ui_StepCnLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_StepCnLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_StepCnLabel, -40);
    lv_obj_set_y(ui_StepCnLabel, -5);
    lv_obj_set_align(ui_StepCnLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_StepCnLabel, "今日步数");
    lv_obj_set_style_text_color(ui_StepCnLabel, lv_color_hex(0x3278FF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_StepCnLabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_StepCnLabel, &ui_font_Cuyuan18, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_StepNumLabel = lv_label_create(ui_HomePage);
    lv_obj_set_width(ui_StepNumLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_StepNumLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_StepNumLabel, -80);
    lv_obj_set_y(ui_StepNumLabel, 20);
    lv_obj_set_align(ui_StepNumLabel, LV_ALIGN_CENTER);
    sprintf(value_strbuf,"%d",ui_StepNumValue);
    lv_label_set_text(ui_StepNumLabel, value_strbuf);
    lv_obj_set_style_text_font(ui_StepNumLabel, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

// 步数进度条：满量程未设置，当前固定 40%，仅作装饰
    ui_StepNumBar = lv_bar_create(ui_HomePage);
    lv_bar_set_value(ui_StepNumBar, 40, LV_ANIM_OFF);
    lv_obj_set_width(ui_StepNumBar, 200);
    lv_obj_set_height(ui_StepNumBar, 10);
    lv_obj_set_x(ui_StepNumBar, 0);
    lv_obj_set_y(ui_StepNumBar, 40);
    lv_obj_set_align(ui_StepNumBar, LV_ALIGN_CENTER);

    lv_obj_set_style_bg_color(ui_StepNumBar, lv_color_hex(0x3278FF), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_StepNumBar, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

// 温度圆弧量程 -20~60，覆盖手表使用环境；橙色
    ui_TempArc = lv_arc_create(ui_HomePage);
    lv_obj_set_width(ui_TempArc, 50);
    lv_obj_set_height(ui_TempArc, 50);
    lv_obj_set_x(ui_TempArc, -75);
    lv_obj_set_y(ui_TempArc, 90);
    lv_obj_set_align(ui_TempArc, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_TempArc, LV_OBJ_FLAG_CLICKABLE);      /// Flags
    lv_arc_set_range(ui_TempArc, -20, 60);
    lv_arc_set_value(ui_TempArc, ui_HomePageTempValue);
    lv_obj_set_style_arc_width(ui_TempArc, 5, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_arc_color(ui_TempArc, lv_color_hex(0xF5A73A), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(ui_TempArc, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(ui_TempArc, 5, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_color(ui_TempArc, lv_color_hex(0xF5A73A), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_TempArc, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui_TempArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui_TempArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui_TempArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui_TempArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_TempiconLabel = lv_label_create(ui_HomePage);
    lv_obj_set_width(ui_TempiconLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_TempiconLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_TempiconLabel, -75);
    lv_obj_set_y(ui_TempiconLabel, 90);
    lv_obj_set_align(ui_TempiconLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_TempiconLabel, "");
    lv_obj_set_style_text_color(ui_TempiconLabel, lv_color_hex(0xF5A73A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_TempiconLabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_TempiconLabel, &ui_font_iconfont30, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_TempNumLabel = lv_label_create(ui_HomePage);
    lv_obj_set_width(ui_TempNumLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_TempNumLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_TempNumLabel, -73);
    lv_obj_set_y(ui_TempNumLabel, 125);
    lv_obj_set_align(ui_TempNumLabel, LV_ALIGN_CENTER);
    sprintf(value_strbuf,"%d",ui_HomePageTempValue);
    lv_label_set_text(ui_TempNumLabel, value_strbuf);

// 湿度圆弧：图标与数字分开创建，数字在圆环下方
    ui_HumiiconLabel = lv_label_create(ui_HomePage);
    lv_obj_set_width(ui_HumiiconLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_HumiiconLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_HumiiconLabel, 0);
    lv_obj_set_y(ui_HumiiconLabel, 89);
    lv_obj_set_align(ui_HumiiconLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_HumiiconLabel, "");
    lv_obj_set_style_text_color(ui_HumiiconLabel, lv_color_hex(0x14C8E1), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_HumiiconLabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_HumiiconLabel, &ui_font_iconfont28, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_HumiArc = lv_arc_create(ui_HomePage);
    lv_obj_set_width(ui_HumiArc, 50);
    lv_obj_set_height(ui_HumiArc, 50);
    lv_obj_set_x(ui_HumiArc, 0);
    lv_obj_set_y(ui_HumiArc, 90);
    lv_obj_set_align(ui_HumiArc, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_HumiArc, LV_OBJ_FLAG_CLICKABLE);      /// Flags
    lv_arc_set_value(ui_HumiArc, ui_HomePageHumiValue);
    lv_arc_set_bg_angles(ui_HumiArc, 270, 620);
    lv_obj_set_style_arc_width(ui_HumiArc, 5, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_arc_color(ui_HumiArc, lv_color_hex(0x14C8E1), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(ui_HumiArc, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(ui_HumiArc, 5, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_color(ui_HumiArc, lv_color_hex(0x14C8E1), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_HumiArc, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui_HumiArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui_HumiArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui_HumiArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui_HumiArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_HumiNumLabel = lv_label_create(ui_HomePage);
    lv_obj_set_width(ui_HumiNumLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_HumiNumLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_HumiNumLabel, 0);
    lv_obj_set_y(ui_HumiNumLabel, 125);
    lv_obj_set_align(ui_HumiNumLabel, LV_ALIGN_CENTER);
    sprintf(value_strbuf,"%d",ui_HomePageHumiValue);
    lv_label_set_text(ui_HumiNumLabel, value_strbuf);

// 心率圆弧：红色，量程默认 0~100；数值来自 EM7028
		ui_HRArc = lv_arc_create(ui_HomePage);
    lv_obj_set_width(ui_HRArc, 50);
    lv_obj_set_height(ui_HRArc, 50);
    lv_obj_set_x(ui_HRArc, 70);
    lv_obj_set_y(ui_HRArc, 90);
    lv_obj_set_align(ui_HRArc, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_HRArc, LV_OBJ_FLAG_CLICKABLE);      /// Flags
    lv_arc_set_value(ui_HRArc, 100);
    lv_arc_set_bg_angles(ui_HRArc, 270, 620);
    lv_obj_set_style_arc_width(ui_HRArc, 5, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_arc_color(ui_HRArc, lv_color_hex(0xE11432), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(ui_HRArc, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(ui_HRArc, 5, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_color(ui_HRArc, lv_color_hex(0xE11432), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_HRArc, 255, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui_HRArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui_HRArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui_HRArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui_HRArc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_HRiconLabel = lv_label_create(ui_HomePage);
    lv_obj_set_width(ui_HRiconLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_HRiconLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_HRiconLabel, 70);
    lv_obj_set_y(ui_HRiconLabel, 90);
    lv_obj_set_align(ui_HRiconLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_HRiconLabel, "");
    lv_obj_set_style_text_color(ui_HRiconLabel, lv_color_hex(0xE11432), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_HRiconLabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_HRiconLabel, &ui_font_iconfont32, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_HRNumLabel = lv_label_create(ui_HomePage);
    lv_obj_set_width(ui_HRNumLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_HRNumLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_HRNumLabel, 70);
    lv_obj_set_y(ui_HRNumLabel, 125);
    lv_obj_set_align(ui_HRNumLabel, LV_ALIGN_CENTER);
    sprintf(value_strbuf,"%d", HWInterface.HR_meter.HrRate);
    lv_label_set_text(ui_HRNumLabel, value_strbuf);


// 下拉面板：420 高、初始 y=-10，再 scroll_by(-130) 把内容藏到屏幕上方
    ui_DropDownPanel = lv_obj_create(ui_HomePage);
    lv_obj_set_width(ui_DropDownPanel, 240);
    lv_obj_set_height(ui_DropDownPanel, 420);
    lv_obj_set_x(ui_DropDownPanel, 0);
    lv_obj_set_y(ui_DropDownPanel, -10);
    lv_obj_set_align(ui_DropDownPanel, LV_ALIGN_TOP_MID);
    lv_obj_scroll_by(ui_DropDownPanel,0,-130,LV_ANIM_OFF);
    lv_obj_clear_flag(ui_DropDownPanel, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_CHAIN);      /// Flags
    //lv_obj_clear_flag(ui_DropDownPanel, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_CHAIN);      /// Flags
    lv_obj_set_scroll_dir(ui_DropDownPanel, LV_DIR_VER);
    lv_obj_set_style_bg_color(ui_DropDownPanel, lv_color_hex(0x323232), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_DropDownPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_DropDownPanel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_DropDownPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_DropDownPanel, 0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_DropDownPanel, 0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);

// 上半个背景板：半透明深灰，给 NFC/BLE 按钮做底
    ui_UpBGPanel = lv_obj_create(ui_DropDownPanel);
    lv_obj_set_width(ui_UpBGPanel, 240);
    lv_obj_set_height(ui_UpBGPanel, 130);
    lv_obj_set_x(ui_UpBGPanel, 0);
    lv_obj_set_y(ui_UpBGPanel, -10);
    lv_obj_set_align(ui_UpBGPanel, LV_ALIGN_TOP_MID);
    lv_obj_clear_flag(ui_UpBGPanel, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_UpBGPanel, lv_color_hex(0x323232), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_UpBGPanel, 200, LV_PART_MAIN | LV_STATE_DEFAULT);

// NFC 开关按钮：CHECKABLE，勾选态换蓝色；重建时按 ui_HomePageNFCEN 恢复
    ui_NFCButton = lv_btn_create(ui_DropDownPanel);
    lv_obj_set_width(ui_NFCButton, 50);
    lv_obj_set_height(ui_NFCButton, 50);
		lv_obj_set_x(ui_NFCButton, 0);
    lv_obj_set_y(ui_NFCButton, 5);
    lv_obj_add_flag(ui_NFCButton, LV_OBJ_FLAG_CHECKABLE | LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_NFCButton, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_NFCButton, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_NFCButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_NFCButton, lv_color_hex(0x3264C8), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(ui_NFCButton, 255, LV_PART_MAIN | LV_STATE_CHECKED);
    if(ui_HomePageNFCEN)
    {lv_obj_add_state(ui_NFCButton, LV_STATE_CHECKED);}

    ui_NFCLabel = lv_label_create(ui_NFCButton);
    lv_obj_set_width(ui_NFCLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_NFCLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_NFCLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_NFCLabel, "");
    lv_obj_set_style_text_color(ui_NFCLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_NFCLabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_NFCLabel, &ui_font_iconfont34, LV_PART_MAIN | LV_STATE_DEFAULT);

// BLE 开关按钮：同上，但点击会真正调用 KT6328
    ui_BLEButton = lv_btn_create(ui_DropDownPanel);
    lv_obj_set_width(ui_BLEButton, 50);
    lv_obj_set_height(ui_BLEButton, 50);
    lv_obj_set_x(ui_BLEButton, 0);
    lv_obj_set_y(ui_BLEButton, 65);
    lv_obj_add_flag(ui_BLEButton, LV_OBJ_FLAG_CHECKABLE | LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_BLEButton, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_BLEButton, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_BLEButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_BLEButton, lv_color_hex(0x3264C8), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(ui_BLEButton, 255, LV_PART_MAIN | LV_STATE_CHECKED);
    if(ui_HomePageBLEEN)
    {lv_obj_add_state(ui_BLEButton, LV_STATE_CHECKED);}

    ui_BLELabel = lv_label_create(ui_BLEButton);
    lv_obj_set_width(ui_BLELabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_BLELabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_BLELabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_BLELabel, "");
    lv_obj_set_style_text_color(ui_BLELabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_BLELabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_BLELabel, &ui_font_iconfont34, LV_PART_MAIN | LV_STATE_DEFAULT);

// 关机按钮 + 设置按钮：普通按钮，仅触发 Page_Load
    ui_PowerButton = lv_btn_create(ui_DropDownPanel);
    lv_obj_set_width(ui_PowerButton, 50);
    lv_obj_set_height(ui_PowerButton, 50);
	lv_obj_set_x(ui_PowerButton, 0);
    lv_obj_set_y(ui_PowerButton, 5);
    lv_obj_set_align(ui_PowerButton, LV_ALIGN_TOP_MID);
    lv_obj_add_flag(ui_PowerButton, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_PowerButton, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_PowerButton, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_PowerButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_PowerLabel = lv_label_create(ui_PowerButton);
    lv_obj_set_width(ui_PowerLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_PowerLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_PowerLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_PowerLabel, "");
    lv_obj_set_style_text_color(ui_PowerLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_PowerLabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_PowerLabel, &ui_font_iconfont34, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SetButton = lv_btn_create(ui_DropDownPanel);
    lv_obj_set_width(ui_SetButton, 50);
    lv_obj_set_height(ui_SetButton, 50);
    lv_obj_set_x(ui_SetButton, 0);
    lv_obj_set_y(ui_SetButton, 65);
    lv_obj_set_align(ui_SetButton, LV_ALIGN_TOP_MID);
    lv_obj_add_flag(ui_SetButton, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_SetButton, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_SetButton, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_SetButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SetLabel = lv_label_create(ui_SetButton);
    lv_obj_set_width(ui_SetLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_SetLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_SetLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_SetLabel, "");
    lv_obj_set_style_text_color(ui_SetLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_SetLabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_SetLabel, &ui_font_iconfont34, LV_PART_MAIN | LV_STATE_DEFAULT);

// 背光滑块：竖向 110 高，范围 0~100（LVGL 默认 0~100）
    ui_LightSlider = lv_slider_create(ui_DropDownPanel);
    lv_slider_set_value(ui_LightSlider, ui_LightSliderValue, LV_ANIM_OFF);
    if(lv_slider_get_mode(ui_LightSlider) == LV_SLIDER_MODE_RANGE) lv_slider_set_left_value(ui_LightSlider, 0, LV_ANIM_OFF);
    lv_obj_set_width(ui_LightSlider, 50);
    lv_obj_set_height(ui_LightSlider, 110);
	lv_obj_set_x(ui_LightSlider, 0);
    lv_obj_set_y(ui_LightSlider, 5);
    lv_obj_set_align(ui_LightSlider, LV_ALIGN_TOP_RIGHT);
    lv_obj_set_style_radius(ui_LightSlider, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_LightSlider, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_LightSlider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(ui_LightSlider, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_LightSlider, lv_color_hex(0x3264C8), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_LightSlider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_color(ui_LightSlider, lv_color_hex(0x000000), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_LightSlider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_LightLabel = lv_label_create(ui_LightSlider);
    lv_obj_set_width(ui_LightLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_LightLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_LightLabel, 0);
    lv_obj_set_y(ui_LightLabel, 30);
    lv_obj_set_align(ui_LightLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_LightLabel, "");
    lv_obj_set_style_text_color(ui_LightLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_LightLabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_LightLabel, &ui_font_iconfont34, LV_PART_MAIN | LV_STATE_DEFAULT);

// 下半个背景板：透明，仅用于撑出滚动区域，让面板能停在中间
    ui_DownBGPanel = lv_obj_create(ui_DropDownPanel);
    lv_obj_set_width(ui_DownBGPanel, 240);
    lv_obj_set_height(ui_DownBGPanel, 130);
    lv_obj_set_x(ui_DownBGPanel, 0);
    lv_obj_set_y(ui_DownBGPanel, 420);
    lv_obj_set_align(ui_DownBGPanel, LV_ALIGN_TOP_MID);
    lv_obj_clear_flag(ui_DownBGPanel, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_DownBGPanel, lv_color_hex(0x323232), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_DownBGPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_DownBGPanel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_DownBGPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

		//events
// 事件绑定：全部用 LV_EVENT_ALL，由回调内部再按 event_code 过滤
		lv_obj_add_event_cb(ui_HomePage, ui_event_HomePage, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_NFCButton, ui_event_NFCButton, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_BLEButton, ui_event_BLEButton, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_PowerButton, ui_event_PowerButton, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_SetButton, ui_event_SetButton, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_LightSlider,ui_event_LightSlider, LV_EVENT_ALL, NULL);

    //timer
// 500 ms 刷新：比 RTC 秒跳变慢半拍，但足够；周期越短功耗越高
    ui_HomePageTimer = lv_timer_create(HomePage_timer_cb, 500,  NULL);

}


/**
 * @brief 创建关机页：一条横向滑块 + “滑动关机”提示
 * @return 无
 * @note 关机页是独立 screen，由 Page_Load(&Page_Power) 载入；滑块值 >=90 才真正关机。
 */
void ui_PowerPage_screen_init(void)
{
    ui_PowerPage = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_PowerPage, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

// 滑块 220x50，灰色底、深红指示条；旋钮透明（opa=0）
    ui_PowerSlider = lv_slider_create(ui_PowerPage);
    lv_obj_set_width(ui_PowerSlider, 220);
    lv_obj_set_height(ui_PowerSlider, 50);
    lv_obj_set_align(ui_PowerSlider, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_color(ui_PowerSlider, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_PowerSlider, 128, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_color(ui_PowerSlider, lv_color_hex(0x800000), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_PowerSlider, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_color(ui_PowerSlider, lv_color_hex(0x800000), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_PowerSlider, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

// 提示文字压在滑块上，居中，20 号字
    ui_PowerDownLabel = lv_label_create(ui_PowerSlider);
    lv_obj_set_width(ui_PowerDownLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_PowerDownLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_PowerDownLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_PowerDownLabel, "滑动关机");
    lv_obj_set_style_text_color(ui_PowerDownLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_PowerDownLabel, 200, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_PowerDownLabel, &ui_font_Cuyuan20, LV_PART_MAIN | LV_STATE_DEFAULT);

		//events
		lv_obj_add_event_cb(ui_PowerSlider, ui_event_PowerSlider, LV_EVENT_ALL, NULL);

}

/////////////////// SCREEN deinit ////////////////////
/**
 * @brief 首页销毁：删除私有定时器
 * @return 无
 * @note 只删定时器不删控件——screen 由 lv_scr_load_anim(..., auto_del=true) 回收。若定时器不删，回调会在已释放的控件上跑，必崩。
 */
void ui_HomePage_screen_deinit(void)
{
  lv_timer_del(ui_HomePageTimer);
}

/**
 * @brief 关机页销毁：无定时器/无动态资源，空实现
 * @return 无
 * @note 保留空函数是为了满足 Page_t 的函数指针签名，不能删。
 */
void ui_PowerPage_screen_deinit(void)
{}
