/**
 * @file    ui_helpers.h
 * @brief   SquareLine Studio 生成的界面辅助函数。
 *
 * 这些函数把"属性 ID + 数值"翻译成对具体 LVGL 控件的操作，
 * 主要供 SquareLine 生成的事件回调使用（设置位置/尺寸/文本/动画等）。
 * 一般不需要修改；若重新从 SquareLine 导出，本文件会被覆盖。
 */
// SquareLine LVGL GENERATED FILE
// EDITOR VERSION: SquareLine Studio 1.1.1
// LVGL VERSION: 8.2.0
// PROJECT: MenuPage

#ifndef _MENUPAGE_UI_HELPERS_H
#define _MENUPAGE_UI_HELPERS_H

#include "ui.h"

#define _UI_TEMPORARY_STRING_BUFFER_SIZE 32	// 临时字符串缓冲区长度
#define _UI_BAR_PROPERTY_VALUE 0			// 进度条属性：当前值
#define _UI_BAR_PROPERTY_VALUE_WITH_ANIM 1	// 进度条属性：当前值（带动画）
void _ui_bar_set_property(lv_obj_t * target, int id, int val);	// 设置进度条属性

#define _UI_BASIC_PROPERTY_POSITION_X 0		// 基础属性：X 坐标
#define _UI_BASIC_PROPERTY_POSITION_Y 1		// 基础属性：Y 坐标
#define _UI_BASIC_PROPERTY_WIDTH 2			// 基础属性：宽度
#define _UI_BASIC_PROPERTY_HEIGHT 3			// 基础属性：高度
void _ui_basic_set_property(lv_obj_t * target, int id, int val);	// 设置控件基础几何属性

#define _UI_DROPDOWN_PROPERTY_SELECTED 0	// 下拉框属性：选中项
void _ui_dropdown_set_property(lv_obj_t * target, int id, int val);	// 设置下拉框属性

#define _UI_IMAGE_PROPERTY_IMAGE 0			// 图片属性：图像源
void _ui_image_set_property(lv_obj_t * target, int id, uint8_t * val);	// 设置图片属性

#define _UI_LABEL_PROPERTY_TEXT 0			// 标签属性：文本
void _ui_label_set_property(lv_obj_t * target, int id, char * val);	// 设置标签文本

#define _UI_ROLLER_PROPERTY_SELECTED 0				// 滚轮属性：选中项
#define _UI_ROLLER_PROPERTY_SELECTED_WITH_ANIM 1	// 滚轮属性：选中项（带动画）
void _ui_roller_set_property(lv_obj_t * target, int id, int val);	// 设置滚轮属性

#define _UI_SLIDER_PROPERTY_VALUE 0				// 滑块属性：当前值
#define _UI_SLIDER_PROPERTY_VALUE_WITH_ANIM 1	// 滑块属性：当前值（带动画）
void _ui_slider_set_property(lv_obj_t * target, int id, int val);	// 设置滑块属性

/** 切换屏幕（target 为目标屏幕，fademode 淡入淡出方式，spd 速度，delay 延时） */
void _ui_screen_change(lv_obj_t * target, lv_scr_load_anim_t fademode, int spd, int delay);

/** 圆弧控件数值增减 */
void _ui_arc_increment(lv_obj_t * target, int val);

/** 进度条数值增减（anm 控制是否动画） */
void _ui_bar_increment(lv_obj_t * target, int val, int anm);

/** 滑块数值增减（anm 控制是否动画） */
void _ui_slider_increment(lv_obj_t * target, int val, int anm);

#define _UI_MODIFY_FLAG_ADD 0		// 标志操作：置位
#define _UI_MODIFY_FLAG_REMOVE 1	// 标志操作：清零
#define _UI_MODIFY_FLAG_TOGGLE 2	// 标志操作：翻转
void _ui_flag_modify(lv_obj_t * target, int32_t flag, int value);	// 修改控件标志位

#define _UI_MODIFY_STATE_ADD 0		// 状态操作：添加
#define _UI_MODIFY_STATE_REMOVE 1	// 状态操作：移除
#define _UI_MODIFY_STATE_TOGGLE 2	// 状态操作：翻转
void _ui_state_modify(lv_obj_t * target, int32_t state, int value);	// 修改控件状态

/** 设置控件不透明度（0~255） */
void _ui_opacity_set(lv_obj_t * target, int val);

void _ui_anim_callback_set_x(lv_anim_t * a, int32_t v);			// 动画回调：设置 X 坐标
void _ui_anim_callback_set_y(lv_anim_t * a, int32_t v);			// 动画回调：设置 Y 坐标
void _ui_anim_callback_set_width(lv_anim_t * a, int32_t v);		// 动画回调：设置宽度
void _ui_anim_callback_set_height(lv_anim_t * a, int32_t v);		// 动画回调：设置高度
void _ui_anim_callback_set_opacity(lv_anim_t * a, int32_t v);		// 动画回调：设置不透明度
void _ui_anim_callback_set_image_zoom(lv_anim_t * a, int32_t v);	// 动画回调：设置图片缩放
void _ui_anim_callback_set_image_angle(lv_anim_t * a, int32_t v);	// 动画回调：设置图片旋转角

int32_t _ui_anim_callback_get_x(lv_anim_t * a);			// 动画回调：读取 X 坐标
int32_t _ui_anim_callback_get_y(lv_anim_t * a);			// 动画回调：读取 Y 坐标
int32_t _ui_anim_callback_get_width(lv_anim_t * a);		// 动画回调：读取宽度
int32_t _ui_anim_callback_get_height(lv_anim_t * a);		// 动画回调：读取高度
int32_t _ui_anim_callback_get_opacity(lv_anim_t * a);		// 动画回调：读取不透明度
int32_t _ui_anim_callback_get_image_zoom(lv_anim_t * a);	// 动画回调：读取图片缩放
int32_t _ui_anim_callback_get_image_angle(lv_anim_t * a);	// 动画回调：读取图片旋转角

/** 用圆弧值刷新文本（prefix/postfix 为前后缀，如 "%"） */
void _ui_arc_set_text_value(lv_obj_t * trg, lv_obj_t * src, char * prefix, char * postfix);

/** 用滑块值刷新文本 */
void _ui_slider_set_text_value(lv_obj_t * trg, lv_obj_t * src, char * prefix, char * postfix);

/** 用复选框状态刷新文本（勾选显示 txt_on，否则显示 txt_off） */
void _ui_checked_set_text_value(lv_obj_t * trg, lv_obj_t * src, char * txt_on, char * txt_off);


#endif
