// LVGL VERSION: 8.2.0

/*
 * 【中文导读】ui.c —— LVGL 初始化入口（B级导读）
 * ----------------------------------------------------------------------------
 * 由 main.c 的 LvHandlerTask 在调度器启动后调用一次 ui_init()：
 *   1) 取默认显示器 dispp，套用 lv_theme_default（蓝/红主色、暗色模式）；
 *   2) Pages_init() 建页面栈并把首页压栈；
 *   3) 创建一个 1 s 的空定时器（main_timer 目前是空函数，预留给全局刷新）。
 *
 * 两个 #error 是 SquareLine 的工程约束：颜色深度必须 16 位、字节序必须不交换，
 * 与 ST7789 的 RGB565 配置一致；改 lv_conf.h 时若忘了同步，这里会直接编译失败。
 *
 * 注意：本文件只有 ui_init 一个有效函数，页面创建逻辑全部在各 ui_*Page.c。
 */
#include "ui.h"
#include "./Screens/Inc/ui_HomePage.h"
#include "./Screens/Inc/ui_MenuPage.h"
#include "ui_helpers.h"

///////////////////// TEST LVGL SETTINGS ////////////////////
#if LV_COLOR_DEPTH != 16
    #error "LV_COLOR_DEPTH should be 16bit to match SquareLine Studio's settings"
#endif
#if LV_COLOR_16_SWAP !=1
    #error "LV_COLOR_16_SWAP should be 0 to match SquareLine Studio's settings"
#endif

/////////////////////// Timer //////////////////////
/**
 * Main timer for Refreshing the screens
 */
static void main_timer(lv_timer_t * timer)
{
    // printf("timer\r\n");

}


/////////////////////// ui_initialize //////////////////////
void ui_init(void)
{

    lv_disp_t * dispp = lv_disp_get_default();
    lv_theme_t * theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                                               true, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    Pages_init();
    //timer
    lv_timer_t * ui_MainTimer = lv_timer_create(main_timer, 1000,  NULL);
}
