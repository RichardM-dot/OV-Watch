// SquareLine LVGL GENERATED FILE
// EDITOR VERSION: SquareLine Studio 1.1.1
// LVGL VERSION: 8.2.0
// PROJECT: MenuPage

/*
 * 【中文导读】ui_helpers.c —— SquareLine 生成的控件操作辅助函数
 * ----------------------------------------------------------------------------
 * SquareLine Studio 导出的事件回调不直接调 LVGL，而是调这里的 _ui_xxx_xxx()：
 * 回调用一个整数 id 表示“要改哪个属性”，由本文件翻译成对应的 lv_xxx_set_xxx()。
 * 这样做的好处是导出的 ui_event_*.c 与具体控件解耦，重新导出时不会大面积改动。
 *
 * 函数分四类：
 *   1) set_property  —— 按属性 ID 写值（bar/basic/dropdown/image/label/roller/slider）；
 *   2) increment     —— 相对增减（arc/bar/slider），用于 +/- 按钮；
 *   3) flag/state    —— 增删翻转 LVGL 的对象标志与状态位；
 *   4) anim_callback —— 作为 lv_anim_set_exec_cb/get_cb 的执行/取值回调。
 *
 * 注意：本文件由 SquareLine 维护，手改后再次导出会被覆盖；
 *       工程内真正承载业务的事件回调在各 ui_*Page.c 里，不在这里。
 */
#include "ui_helpers.h"

/**
 * @brief 按属性 ID 设置进度条的值
 * @param target 目标进度条对象
 * @param id     _UI_BAR_PROPERTY_VALUE(0) 立即赋值 / _WITH_ANIM(1) 带动画
 * @param val    目标值
 * @return 无
 * @note 两个 if 不是 else-if：调用方只传这两个 id 之一，重复判断无副作用。
 */
void _ui_bar_set_property(lv_obj_t * target, int id, int val)
{
    if(id == _UI_BAR_PROPERTY_VALUE_WITH_ANIM) lv_bar_set_value(target, val, LV_ANIM_ON);
    if(id == _UI_BAR_PROPERTY_VALUE) lv_bar_set_value(target, val, LV_ANIM_OFF);
}

/**
 * @brief 按属性 ID 设置控件的位置与尺寸
 * @param target 目标控件
 * @param id     X/Y/WIDTH/HEIGHT 四个选择子
 * @param val    像素值
 * @return 无
 * @note SquareLine 的"Basic"动画会连续调用本函数改几何，逐帧刷新即可。
 */
void _ui_basic_set_property(lv_obj_t * target, int id, int val)
{
    if(id == _UI_BASIC_PROPERTY_POSITION_X) lv_obj_set_x(target, val);
    if(id == _UI_BASIC_PROPERTY_POSITION_Y) lv_obj_set_y(target, val);
    if(id == _UI_BASIC_PROPERTY_WIDTH) lv_obj_set_width(target, val);
    if(id == _UI_BASIC_PROPERTY_HEIGHT) lv_obj_set_height(target, val);
}


/**
 * @brief 按属性 ID 设置下拉框的选中项
 * @param target 下拉框对象
 * @param id     固定为 _UI_DROPDOWN_PROPERTY_SELECTED
 * @param val    选中项索引（从 0 开始）
 * @return 无
 */
void _ui_dropdown_set_property(lv_obj_t * target, int id, int val)
{
    if(id == _UI_DROPDOWN_PROPERTY_SELECTED) lv_dropdown_set_selected(target, val);
}

/**
 * @brief 按属性 ID 替换图片源
 * @param target 图片对象
 * @param id     固定为 _UI_IMAGE_PROPERTY_IMAGE
 * @param val    图像描述符指针（通常是 lv_img_dsc_t*）
 * @return 无
 * @note 传 uint8_t* 是 SquareLine 的通用签名，内部直接透传给 lv_img_set_src。
 */
void _ui_image_set_property(lv_obj_t * target, int id, uint8_t * val)
{
    if(id == _UI_IMAGE_PROPERTY_IMAGE) lv_img_set_src(target, val);
}

/**
 * @brief 按属性 ID 设置标签文本
 * @param target 标签对象
 * @param id     固定为 _UI_LABEL_PROPERTY_TEXT
 * @param val    以 \0 结尾的字符串
 * @return 无
 * @note LVGL 会拷贝文本，val 指向的栈缓冲区可以随后释放。
 */
void _ui_label_set_property(lv_obj_t * target, int id, char * val)
{
    if(id == _UI_LABEL_PROPERTY_TEXT) lv_label_set_text(target, val);
}


/**
 * @brief 按属性 ID 设置滚轮的选中项
 * @param target 滚轮对象
 * @param id     SELECTED(0) 立即 / _WITH_ANIM(1) 滚动动画
 * @param val    选中行索引
 * @return 无
 * @note 滚轮用于设置页的时/分选择，动画打开时视觉上有滚动惯性。
 */
void _ui_roller_set_property(lv_obj_t * target, int id, int val)
{
    if(id == _UI_ROLLER_PROPERTY_SELECTED_WITH_ANIM) lv_roller_set_selected(target, val, LV_ANIM_ON);
    if(id == _UI_ROLLER_PROPERTY_SELECTED) lv_roller_set_selected(target, val, LV_ANIM_OFF);
}

/**
 * @brief 按属性 ID 设置滑块的值
 * @param target 滑块对象
 * @param id     VALUE(0) 立即 / _WITH_ANIM(1) 带动画
 * @param val    目标值（受 min/max 限制）
 * @return 无
 */
void _ui_slider_set_property(lv_obj_t * target, int id, int val)
{
    if(id == _UI_SLIDER_PROPERTY_VALUE_WITH_ANIM) lv_slider_set_value(target, val, LV_ANIM_ON);
    if(id == _UI_SLIDER_PROPERTY_VALUE) lv_slider_set_value(target, val, LV_ANIM_OFF);
}


/**
 * @brief 切换屏幕并指定过场动画
 * @param target   目标屏幕对象
 * @param fademode 动画类型（LV_SCR_LOAD_ANIM_*）
 * @param spd      动画时长 ms
 * @param delay    开始前延时 ms
 * @return 无
 * @note 最后参数 false = 不自动删除旧屏幕；本项目页面生命周期由 PageManager 管，不经此函数。
 */
void _ui_screen_change(lv_obj_t * target, lv_scr_load_anim_t fademode, int spd, int delay)
{
    lv_scr_load_anim(target, fademode, spd, delay, false);
}

/**
 * @brief 圆弧值相对增减（+/- 按钮用）
 * @param target 圆弧对象
 * @param val    增量，可为负
 * @return 无
 * @note 先读后写，非原子；LVGL 只在 GUI 任务里调用，故无并发问题。
 */
void _ui_arc_increment(lv_obj_t * target, int val)
{
    int old = lv_arc_get_value(target);
    lv_arc_set_value(target, old + val);
}

/**
 * @brief 进度条值相对增减
 * @param target 进度条对象
 * @param val    增量
 * @param anm    动画开关（LV_ANIM_ON/OFF）
 * @return 无
 */
void _ui_bar_increment(lv_obj_t * target, int val, int anm)
{
    int old = lv_bar_get_value(target);
    lv_bar_set_value(target, old + val, anm);
}

/**
 * @brief 滑块值相对增减
 * @param target 滑块对象
 * @param val    增量
 * @param anm    动画开关（LV_ANIM_ON/OFF）
 * @return 无
 */
void _ui_slider_increment(lv_obj_t * target, int val, int anm)
{
    int old = lv_slider_get_value(target);
    lv_slider_set_value(target, old + val, anm);
}

/**
 * @brief 按操作码增删或翻转对象的 LVGL 标志位
 * @param target 目标对象
 * @param flag   要操作的标志（LV_OBJ_FLAG_*）
 * @param value  ADD(0) / REMOVE(1) / TOGGLE(2)
 * @return 无
 * @note TOGGLE 分支用 has_flag 判断当前状态；常用于动态开关可滚动/可点击。
 */
void _ui_flag_modify(lv_obj_t * target, int32_t flag, int value)
{
    if(value == _UI_MODIFY_FLAG_TOGGLE) {
        if(lv_obj_has_flag(target, flag)) lv_obj_clear_flag(target, flag);
        else lv_obj_add_flag(target, flag);
    }
    else if(value == _UI_MODIFY_FLAG_ADD) lv_obj_add_flag(target, flag);
    else lv_obj_clear_flag(target, flag);
}
/**
 * @brief 按操作码增删或翻转对象的状态位
 * @param target 目标对象
 * @param state  要操作的状态（LV_STATE_*）
 * @param value  ADD(0) / REMOVE(1) / TOGGLE(2)
 * @return 无
 * @note 状态位影响样式表匹配，例如 CHECKED 会切换到按下态配色。
 */
void _ui_state_modify(lv_obj_t * target, int32_t state, int value)
{
    if(value == _UI_MODIFY_STATE_TOGGLE) {
        if(lv_obj_has_state(target, state)) lv_obj_clear_state(target, state);
        else lv_obj_add_state(target, state);
    }
    else if(value == _UI_MODIFY_STATE_ADD) lv_obj_add_state(target, state);
    else lv_obj_clear_state(target, state);
}

/**
 * @brief 设置对象主部件的不透明度
 * @param target 目标对象
 * @param val    0（全透明）~255（不透明）
 * @return 无
 */
void _ui_opacity_set(lv_obj_t * target, int val)
{
    lv_obj_set_style_opa(target, val, 0);
}

/**
 * @brief 动画执行回调：把当前插值写到对象 X 坐标
 * @param a 动画句柄，user_data 存目标对象指针
 * @param v 本帧的 X 值
 * @return 无
 * @note user_data 由 lv_anim_set_var 绑定，回调里强转回 lv_obj_t*。
 */
void _ui_anim_callback_set_x(lv_anim_t * a, int32_t v)
{
    lv_obj_set_x((lv_obj_t *)a->user_data, v);
}

/**
 * @brief 动画执行回调：写 Y 坐标
 * @param a 动画句柄
 * @param v 本帧 Y 值
 * @return 无
 */
void _ui_anim_callback_set_y(lv_anim_t * a, int32_t v)
{
    lv_obj_set_y((lv_obj_t *)a->user_data, v);
}

/**
 * @brief 动画执行回调：写宽度
 * @param a 动画句柄
 * @param v 本帧宽度
 * @return 无
 */
void _ui_anim_callback_set_width(lv_anim_t * a, int32_t v)
{
    lv_obj_set_width((lv_obj_t *)a->user_data, v);
}

/**
 * @brief 动画执行回调：写高度
 * @param a 动画句柄
 * @param v 本帧高度
 * @return 无
 */
void _ui_anim_callback_set_height(lv_anim_t * a, int32_t v)
{
    lv_obj_set_height((lv_obj_t *)a->user_data, v);
}

/**
 * @brief 动画执行回调：写不透明度
 * @param a 动画句柄
 * @param v 本帧 0~255
 * @return 无
 */
void _ui_anim_callback_set_opacity(lv_anim_t * a, int32_t v)
{
    lv_obj_set_style_opa((lv_obj_t *)a->user_data, v, 0);
}

/**
 * @brief 动画执行回调：写图片缩放
 * @param a 动画句柄
 * @param v 缩放系数，256 = 原始大小
 * @return 无
 * @note LVGL 8 的 zoom 是 8.8 定点，256 表示 1 倍。
 */
void _ui_anim_callback_set_image_zoom(lv_anim_t * a, int32_t v)
{
    lv_img_set_zoom((lv_obj_t *)a->user_data, v);
}

/**
 * @brief 动画执行回调：写图片旋转角
 * @param a 动画句柄
 * @param v 角度，单位 0.1 度（3600 = 360°）
 * @return 无
 */
void _ui_anim_callback_set_image_angle(lv_anim_t * a, int32_t v)
{
    lv_img_set_angle((lv_obj_t *)a->user_data, v);
}


/**
 * @brief 动画取值回调：读对象当前 X 坐标
 * @return 当前 X（对齐后的值）
 * @note 作为 lv_anim_set_values 的起点，保证从当前视觉位置开始动。
 */
int32_t _ui_anim_callback_get_x(lv_anim_t * a)
{
    return lv_obj_get_x_aligned((lv_obj_t *)a->user_data);
}

/**
 * @brief 动画取值回调：读当前 Y 坐标
 * @return 当前 Y
 */
int32_t _ui_anim_callback_get_y(lv_anim_t * a)
{
    return lv_obj_get_y_aligned((lv_obj_t *)a->user_data);
}

/**
 * @brief 动画取值回调：读当前宽度
 * @return 当前宽度
 */
int32_t _ui_anim_callback_get_width(lv_anim_t * a)
{
    return lv_obj_get_width((lv_obj_t *)a->user_data);
}

/**
 * @brief 动画取值回调：读当前高度
 * @return 当前高度
 */
int32_t _ui_anim_callback_get_height(lv_anim_t * a)
{
    return lv_obj_get_height((lv_obj_t *)a->user_data);
}

/**
 * @brief 动画取值回调：读当前不透明度
 * @return 0~255
 */
int32_t _ui_anim_callback_get_opacity(lv_anim_t * a)
{
    return lv_obj_get_style_opa((lv_obj_t *)a->user_data, 0);
}

/**
 * @brief 动画取值回调：读当前图片缩放
 * @return 8.8 定点缩放值
 */
int32_t _ui_anim_callback_get_image_zoom(lv_anim_t * a)
{
    return lv_img_get_zoom((lv_obj_t *)a->user_data);
}

/**
 * @brief 动画取值回调：读当前图片旋转角
 * @return 0.1 度为单位的角度
 */
int32_t _ui_anim_callback_get_image_angle(lv_anim_t * a)
{
    return lv_img_get_angle((lv_obj_t *)a->user_data);
}

/**
 * @brief 把圆弧当前值格式化成文本写进标签
 * @param trg     目标标签
 * @param src     数值来源圆弧
 * @param prefix  前缀字符串
 * @param postfix 后缀字符串（如 "%"）
 * @return 无
 * @note 缓冲区 32 字节（_UI_TEMPORARY_STRING_BUFFER_SIZE），够放 "100%" 这类短文本。
 */
void _ui_arc_set_text_value(lv_obj_t * trg, lv_obj_t * src, char * prefix, char * postfix)
{
    char buf[_UI_TEMPORARY_STRING_BUFFER_SIZE];
    lv_snprintf(buf, sizeof(buf), "%s%d%s", prefix, (int)lv_arc_get_value(src), postfix);
    lv_label_set_text(trg, buf);
}

/**
 * @brief 把滑块当前值格式化成文本写进标签
 * @param trg 目标标签
 * @param src 数值来源滑块
 * @param prefix 前缀
 * @param postfix 后缀
 * @return 无
 */
void _ui_slider_set_text_value(lv_obj_t * trg, lv_obj_t * src, char * prefix, char * postfix)
{
    char buf[_UI_TEMPORARY_STRING_BUFFER_SIZE];
    lv_snprintf(buf, sizeof(buf), "%s%d%s", prefix, (int)lv_slider_get_value(src), postfix);
    lv_label_set_text(trg, buf);
}
/**
 * @brief 根据复选框状态切换标签文本
 * @param trg     目标标签
 * @param src     复选框/开关对象
 * @param txt_on  勾选时显示
 * @param txt_off 未勾选时显示
 * @return 无
 * @note 只判断 CHECKED 状态位，不读写业务变量。
 */
void _ui_checked_set_text_value(lv_obj_t * trg, lv_obj_t * src, char * txt_on, char * txt_off)
{
    if(lv_obj_has_state(src, LV_STATE_CHECKED)) lv_label_set_text(trg, txt_on);
    else lv_label_set_text(trg, txt_off);
}


