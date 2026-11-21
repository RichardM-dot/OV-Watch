#include "../../ui.h"
#include "../../ui_helpers.h"
#include "../Inc/ui_CompassPage.h"
#include "../../../Func/Inc/HWDataAccess.h"
/*
 * 【中文导读】ui_CompassPage.c —— 指南针页（刻度盘 + 指针图片 + 海拔）
 * ----------------------------------------------------------------------------
 * 画面中心是一个 230x230 的 lv_meter：外圈 0~360 度 61 个刻度（每 6 度一格），
 * 内圈 12 个大刻度（对应表盘习惯的 12 等分）；指针是一张 23x171 的 PNG
 * （ui_img_compass_needle_png），以图片中心为轴旋转。
 * 
 * 读数来源：
 *   方向 → HWInterface.Ecompass.direction（0~359，LSM303 磁力计算出的方位角）
 *   海拔 → HWInterface.Barometer.altitude（SPL06 气压换算，单位 m）
 * 两者都由 SensorDataUpdateTask 周期更新，本页只读缓存。
 * 
 * 换算关键：lv_img_set_angle() 的单位是 0.1 度，所以传 direction*10；
 * 指针图片默认朝上（0 度 = 正北），与 meter 的 0 度起点一致。
 * 
 * 底部两个 100x30 的半透明按钮只是“信息牌”，被清掉 CLICKABLE，纯展示。
 */

///////////////////// Page Manager //////////////////
/**
 * @brief 把指南针页注册进页面管理器
 * @return 无
 * @note 从菜单“指南针”进入。
 */
Page_t Page_Compass = {ui_CompassPage_screen_init, ui_CompassPage_screen_deinit, &ui_CompassPage};

///////////////////// VARIABLES ////////////////////
lv_obj_t * ui_CompassPage;
lv_obj_t * ui_CompassMeter;
lv_obj_t * ui_Compassneedle;

lv_obj_t * ui_CompassDirBtn;
lv_obj_t * ui_CompassDirLabel;
lv_obj_t * ui_CompassDirIcon;

lv_obj_t * ui_EnvAltitudeBtn;
lv_obj_t * ui_EnvAltitudeLabel;
lv_obj_t * ui_EnvAltitudeIcon;

// 500 ms 刷新定时器，与传感器任务同周期
lv_timer_t * ui_EcompassPageTimer;

///////////////////// FUNCTIONS ////////////////////


/////////////////// private Timer ///////////////////
// need to be destroyed when the page is destroyed
/**
 * @brief 指南针定时器：旋转指针并刷新方向/海拔文本
 * @param timer LVGL 定时器句柄（未使用）
 * @return 无
 * @note 每 500 ms 无条件下发旋转；lv_img_set_angle 会触发重绘，但图片小，开销可接受。
 */
static void EcompassPage_timer_cb(lv_timer_t * timer)
{
// 6 字节缓冲：":359" 或 ":1234m" 都在范围内
    uint8_t value_strbuf[6];
    //set text
// direction 单位是度，LVGL 角度单位是 0.1 度，故乘 10
    lv_img_set_angle(ui_Compassneedle, HWInterface.Ecompass.direction * 10);
// 文本带前导冒号，视觉上与左侧图标连成 “图标:数值”
    sprintf(value_strbuf,":%d", HWInterface.Ecompass.direction);
    lv_label_set_text(ui_CompassDirLabel, value_strbuf);

    //set text
// 海拔来自气压计，不是指南针；两个传感器共用这一页展示
    sprintf(value_strbuf,":%dm", HWInterface.Barometer.altitude);
    lv_label_set_text(ui_EnvAltitudeLabel, value_strbuf);
}

///////////////////// SCREEN init ////////////////////
/**
 * @brief 创建指南针页：双层刻度 + 指针图 + 两个信息牌
 * @return 无
 * @note meter 先建、指针后建，保证指针绘制在刻度之上。
 */
void ui_CompassPage_screen_init(void)
{
// 10 字节缓冲，足够 ":359" 与 ":9999m"
    uint8_t value_strbuf[10];

		ui_CompassPage = lv_obj_create(NULL);

// 表盘 230x230，整体上移 20 px 给底部信息牌留位置
    ui_CompassMeter = lv_meter_create(ui_CompassPage);
    lv_obj_set_size(ui_CompassMeter, 230, 230);
    lv_obj_center(ui_CompassMeter);
    lv_obj_set_pos(ui_CompassMeter,0,-20);

// 外圈：61 个刻度、长 1 px、间隔 10 度，0~360 度
    lv_meter_scale_t * scale_360 = lv_meter_add_scale(ui_CompassMeter);
    lv_meter_set_scale_ticks(ui_CompassMeter, scale_360, 61, 1, 10, lv_palette_main(LV_PALETTE_GREY));
    lv_meter_set_scale_range(ui_CompassMeter, scale_360, 0, 360, 360, 0);

// 内圈：12 个大刻度，每 30 度一个，长 15 px 白色
    lv_meter_scale_t * scale_12 = lv_meter_add_scale(ui_CompassMeter);
    lv_meter_set_scale_ticks(ui_CompassMeter, scale_12, 12, 0, 0, lv_palette_main(LV_PALETTE_GREY));
    lv_meter_set_scale_major_ticks(ui_CompassMeter, scale_12, 1, 2, 15, lv_color_white(), 15);
    lv_meter_set_scale_range(ui_CompassMeter, scale_12, 0, 330, 330, 270);


// 指针图片：23x171，居中；ADV_HITTEST 让点击命中按像素透明度计算
    ui_Compassneedle = lv_img_create(ui_CompassMeter);
    lv_img_set_src(ui_Compassneedle, &ui_img_compass_needle_png);
    lv_obj_set_width(ui_Compassneedle, LV_SIZE_CONTENT);   /// 23
    lv_obj_set_height(ui_Compassneedle, LV_SIZE_CONTENT);    /// 171
    lv_obj_set_align(ui_Compassneedle, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_Compassneedle, LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
    lv_obj_clear_flag(ui_Compassneedle, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
// 进页先按当前方向摆一次，避免出现“指北”再跳转
    lv_img_set_angle(ui_Compassneedle, HWInterface.Ecompass.direction * 10);


// 海拔信息牌：对齐 meter 底部中点再右移 60
    ui_EnvAltitudeBtn = lv_btn_create(ui_CompassPage);
    lv_obj_set_size(ui_EnvAltitudeBtn, 100, 30);
    lv_obj_align_to(ui_EnvAltitudeBtn,ui_CompassMeter,LV_ALIGN_BOTTOM_MID,60,50);
    lv_obj_add_flag(ui_EnvAltitudeBtn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_EnvAltitudeBtn, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_EnvAltitudeBtn, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_EnvAltitudeBtn, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_EnvAltitudeBtn, 128, LV_PART_MAIN | LV_STATE_DEFAULT);

// 图标用 UTF-8 转义写死（iconfont 私有码位），避免源文件编码问题
    ui_EnvAltitudeIcon = lv_label_create(ui_EnvAltitudeBtn);
    lv_obj_set_align(ui_EnvAltitudeIcon,LV_ALIGN_LEFT_MID);
    lv_obj_set_pos(ui_EnvAltitudeIcon,-10,0);
    lv_label_set_text(ui_EnvAltitudeIcon, "\xEE\x9A\x9B");
    lv_obj_set_style_text_opa(ui_EnvAltitudeIcon, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_EnvAltitudeIcon, &ui_font_iconfont28, LV_PART_MAIN | LV_STATE_DEFAULT);

// 海拔文本 “:123m”
    ui_EnvAltitudeLabel = lv_label_create(ui_EnvAltitudeBtn);
    lv_obj_set_align(ui_EnvAltitudeLabel,LV_ALIGN_LEFT_MID);
    lv_obj_set_pos(ui_EnvAltitudeLabel,17,0);
		sprintf(value_strbuf,":%dm", HWInterface.Barometer.altitude);
    lv_label_set_text(ui_EnvAltitudeLabel, value_strbuf);
    lv_obj_set_style_text_font(ui_EnvAltitudeLabel, &ui_font_Cuyuan20, LV_PART_MAIN | LV_STATE_DEFAULT);

// 方向信息牌：左移 60，与海拔牌对称
    ui_CompassDirBtn = lv_btn_create(ui_CompassPage);
    lv_obj_set_size(ui_CompassDirBtn, 100, 30);
    lv_obj_align_to(ui_CompassDirBtn,ui_CompassMeter,LV_ALIGN_BOTTOM_MID,-60,50);
    lv_obj_add_flag(ui_CompassDirBtn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_CompassDirBtn, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_CompassDirBtn, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_CompassDirBtn, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_CompassDirBtn, 128, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_CompassDirIcon = lv_label_create(ui_CompassDirBtn);
    lv_obj_set_align(ui_CompassDirIcon,LV_ALIGN_LEFT_MID);
    lv_obj_set_pos(ui_CompassDirIcon,-10,0);
    lv_label_set_text(ui_CompassDirIcon, "\xee\x9e\x88");
    lv_obj_set_style_text_opa(ui_CompassDirIcon, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_CompassDirIcon, &ui_font_iconfont28, LV_PART_MAIN | LV_STATE_DEFAULT);

// 方向文本 “:359”
    ui_CompassDirLabel = lv_label_create(ui_CompassDirBtn);
    lv_obj_set_align(ui_CompassDirLabel,LV_ALIGN_LEFT_MID);
    lv_obj_set_pos(ui_CompassDirLabel,17,0);
		sprintf(value_strbuf,":%d", HWInterface.Ecompass.direction);
    lv_label_set_text(ui_CompassDirLabel, value_strbuf);
    lv_obj_set_style_text_font(ui_CompassDirLabel, &ui_font_Cuyuan20, LV_PART_MAIN | LV_STATE_DEFAULT);

    //timer
// 500 ms 周期
    ui_EcompassPageTimer = lv_timer_create(EcompassPage_timer_cb, 500,  NULL);

}

/////////////////// SCREEN deinit ////////////////////
/**
 * @brief 指南针页销毁：删除定时器
 * @return 无
 * @note 指针图片是常量资源，不需要释放；screen 由 LVGL 回收。
 */
void ui_CompassPage_screen_deinit(void)
{
  lv_timer_del(ui_EcompassPageTimer);
}
