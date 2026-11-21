#include "../../ui.h"
#include "../../ui_helpers.h"
#include "../Inc/ui_ComputerPage.h"

#include "../../../Func/Inc/StrCalculate.h"
/*
 * 【中文导读】ui_ComputerPage.c —— 计算器页（UI 只负责按键路由，算法在 StrCalculate.c）
 * ----------------------------------------------------------------------------
 * 上半屏是 200x60 的 textarea（只读，用来显示算式与结果），右上角一个退格按钮；
 * 下半屏是 4x4 的 lv_btnmatrix：
 *   1 2 3 ＋ / 4 5 6 － / 7 8 9 × / . 0 ＝ ÷
 * 运算符在 map 里用 UTF-8 转义写（＋=\xEF\xBC\x8B，×=\xC3\x97，÷=\xC3\xB7）。
 * 
 * 按键路由：VALUE_CHANGED 时把按键文本追加到 textarea，同时把对应 ASCII 字符
 * 压进 CalStr（StrStack_t，字符栈）；按“＝”时调 StrCalculate(CalStr, NumStack,
 * SymStack) 求值，结果按整数/小数分别用 %.0f / %.4f 打印。
 * 
 * 求值后 CalStr 被清空、整个键盘被清 CLICKABLE，必须按退格（清屏 20 个字符）
 * 或长按退格才能继续算——这是防止在结果后面继续拼接的简化做法。
 * 
 * 注意：textarea 的 max_length = TEXT_FULL*2 = 20，而光标位置判断用的是
 * <= TEXT_FULL（10），两者不一致，第 11~20 个字符虽然能输入但会被提前截断。
 */

///////////////////// Page Manager //////////////////
Page_t Page_Computer = {ui_ComputerPage_screen_init, ui_ComputerPage_screen_deinit, &ui_Computerpage};

///////////////////// VARIABLES ////////////////////

// 单条算式长度上限；textarea 容量是它的 2 倍
#define TEXT_FULL 10

StrStack_t CalStr;
NumStack_t NumStack;
SymStack_t SymStack;
lv_obj_t * ui_Computerpage;
lv_obj_t * ui_CompageBtnM;
lv_obj_t * ui_CompageTextarea;
lv_obj_t * ui_CompageBackBtn;

// 按键表：16 个键 + 3 个换行 + 结尾空串；运算符用 UTF-8 字节转义
static const char * ui_ComPageBtnmap[] ={"1", "2", "3", "\xEF\xBC\x8B", "\n",
                                         "4", "5", "6", "\xEF\xBC\x8D", "\n",
                                         "7", "8", "9", "\xC3\x97", "\n",
                                         ".", "0", "\xEF\xBC\x9D", "\xC3\xB7", ""};


///////////////////// FUNCTIONS ////////////////////
/**
 * @brief 键盘事件：按键追加 + 绘制运算符圆角 + “＝”求值
 * @param e LVGL 事件对象
 * @return 无
 * @note DRAW_PART_BEGIN 与 VALUE_CHANGED 共用一个回调；前者决定运算符键的外观，后者处理输入逻辑。
 */
void ui_CompageBtnM_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_DRAW_PART_BEGIN)
    {

        lv_obj_draw_part_dsc_t * dsc = lv_event_get_param(e);
// id 3/7/11/14/15 分别是 + - × ＝ ÷，画成圆形蓝键
        if(dsc->id == 3 || dsc->id == 7 || dsc->id == 11 || dsc->id == 14 || dsc->id == 15)
        {
            dsc->rect_dsc->radius = LV_RADIUS_CIRCLE;
// 选中态用更深一档的蓝色，提供按下反馈
            if (lv_btnmatrix_get_selected_btn(obj) == dsc->id)
            {
                dsc->rect_dsc->bg_color = lv_palette_darken(LV_PALETTE_BLUE,3);
                //lv_btnmatrix_set_selected_btn(ui_CompageBtnM,NULL);
            }
            else
                dsc->rect_dsc->bg_color = lv_palette_main(LV_PALETTE_BLUE);


        }

    }
    if(code == LV_EVENT_DRAW_PART_END)
    {
        lv_obj_draw_part_dsc_t * dsc = lv_event_get_param(e);


    }
// VALUE_CHANGED：真正处理按键
    if(code == LV_EVENT_VALUE_CHANGED)
    {
        uint16_t btn_id = lv_btnmatrix_get_selected_btn(obj); // 获取当前选中的按键的id
        const char * txt = lv_btnmatrix_get_btn_text(obj, btn_id); // 获取当前按键的文本

        if (txt != NULL)
        {
            if (ui_CompageTextarea != NULL)
            {
// 光标位置 <=10 才允许继续输入，超长直接忽略
							if(lv_textarea_get_cursor_pos(ui_CompageTextarea) <= TEXT_FULL)
              {
									lv_textarea_add_text(ui_CompageTextarea, txt); // 文本框追加字符
// switch 把 btn_id 翻译成 ASCII 压入字符栈；UI 显示全角，栈内存半角
									switch(btn_id)
									{
											case 0:
													strput(&CalStr,'1');
													break;
											case 1:
													strput(&CalStr,'2');
													break;
											case 2:
													strput(&CalStr,'3');
													break;
											case 3:
													strput(&CalStr,'+');
													break;
											case 4:
													strput(&CalStr,'4');
													break;
											case 5:
													strput(&CalStr,'5');
													break;
											case 6:
													strput(&CalStr,'6');
													break;
											case 7:
													strput(&CalStr,'-');
													break;
											case 8:
													strput(&CalStr,'7');
													break;
											case 9:
													strput(&CalStr,'8');
													break;
											case 10:
													strput(&CalStr,'9');
													break;
											case 11:
													strput(&CalStr,'*');
													break;
											case 12:
													strput(&CalStr,'.');
													break;
											case 13:
													strput(&CalStr,'0');
													break;
// “＝”：先压 '=' 与换行，再走求值分支
											case 14:
													strput(&CalStr,'=');
													lv_textarea_add_text(ui_CompageTextarea,"\n");
													strput(&CalStr,'\n');
													break;
											case 15:
													strput(&CalStr,'/');
													break;
									}
							}
            }
        }

        if(lv_btnmatrix_get_selected_btn(obj) == 14)
        {
            //calculate
// StrCalculate 返回非 0 表示表达式非法
            if(StrCalculate(CalStr.strque,&NumStack,&SymStack))
						{lv_textarea_add_text(ui_CompageTextarea,"erro");}
						else
						{
// 10 字节输出缓冲：结果最长 9 位 + \0
							char strout[10];
							if(isIntNumber(NumStack.data[NumStack.Top_Point-1]))
							{sprintf(strout,"%.0f",NumStack.data[NumStack.Top_Point-1]);}
							else
							{sprintf(strout,"%.4f",NumStack.data[NumStack.Top_Point-1]);}
							lv_textarea_add_text(ui_CompageTextarea,strout);
						}
            strclear(&CalStr);
// 求值后锁键盘，必须退格清屏才能继续
						lv_obj_clear_flag(ui_CompageBtnM,LV_OBJ_FLAG_CLICKABLE);
        }
    }
}


/**
 * @brief 退格按钮：删一个字符；栈空时清屏并解锁键盘
 * @param e LVGL 事件对象
 * @return 无
 * @note 长按（LONG_PRESSED）直接清空整个字符栈并清屏，是“全部清除”的快捷方式。
 */
void ui_CompageBackBtn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_CLICKED)
    {
        if (ui_CompageTextarea != NULL)
        {
// 栈非空：同步删一个显示字符与一个栈元素
            if(!strstack_isEmpty(&CalStr))
            {
                lv_textarea_del_char(ui_CompageTextarea);
                strdel(&CalStr);
            }
            else
            {
// 栈空：删 TEXT_FULL*2 个字符把 textarea 清空，并恢复键盘可点
                int i = 0;
                for (i = 0; i < (TEXT_FULL*2); i++)
                {
                    lv_textarea_del_char(ui_CompageTextarea);
                }
								lv_obj_add_flag(ui_CompageBtnM,LV_OBJ_FLAG_CLICKABLE);
            }
        }
    }
// 长按：清空栈 + 清屏，但不恢复键盘（下一轮仍走短按逻辑）
		if(code == LV_EVENT_LONG_PRESSED)
		{
			if (ui_CompageTextarea != NULL)
			{
				if(!strstack_isEmpty(&CalStr))
				{
						strclear(&CalStr);
						int i = 0;
						for (i = 0; i < (TEXT_FULL*2); i++)
						{
								lv_textarea_del_char(ui_CompageTextarea);
						}
				}
			}
		}
}


///////////////////// SCREEN init ////////////////////
/**
 * @brief 创建计算器页并清零三个全局栈
 * @return 无
 * @note CalStr/NumStack/SymStack 是文件级全局，进页先清，避免上次残留。
 */
void ui_ComputerPage_screen_init(void)
{
// 三个栈分别存：表达式字符、数字、运算符
		strclear(&CalStr);
		NumStackClear(&NumStack);
		SymStackClear(&SymStack);
    ui_Computerpage = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Computerpage,LV_OBJ_FLAG_SCROLLABLE);
// 4x4 键盘，240x220 贴底
    ui_CompageBtnM = lv_btnmatrix_create(ui_Computerpage);
    lv_btnmatrix_set_map(ui_CompageBtnM, ui_ComPageBtnmap);

    lv_obj_set_style_text_font(ui_CompageBtnM, &ui_font_Cuyuan24, 0);
    lv_btnmatrix_set_one_checked(ui_CompageBtnM,true);
    int i = 0;
// 16 个键全部禁用长按重复，防止一次长按输入一串数字
    for (i = 0; i < 16; i++)
    {
        lv_btnmatrix_set_btn_ctrl(ui_CompageBtnM, i, LV_BTNMATRIX_CTRL_NO_REPEAT); // 长按按钮时禁用重复
    }
    lv_obj_clear_flag(ui_CompageBtnM, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_set_style_border_width(ui_CompageBtnM,0,0);
    lv_obj_set_style_bg_opa(ui_CompageBtnM,0,0);
    lv_obj_set_size(ui_CompageBtnM,240,220);
    lv_obj_set_align(ui_CompageBtnM,LV_ALIGN_BOTTOM_MID);


// 显示区：200x60，只读，右对齐，24 号字
    ui_CompageTextarea = lv_textarea_create(ui_Computerpage);
    lv_textarea_set_one_line(ui_CompageTextarea, false); // 将文本区域配置为一行
    //lv_textarea_set_password_mode(obj_text_area, true); // 将文本区域配置为密码模式
    lv_textarea_set_max_length(ui_CompageTextarea, TEXT_FULL*2); // 设置文本区域可输入的字符长度最大值
    lv_obj_add_state(ui_CompageTextarea, LV_STATE_FOCUSED); // 显示光标
    lv_obj_set_style_radius(ui_CompageTextarea, 0, 0); // 设置样式的圆角弧度
    lv_obj_set_style_border_width(ui_CompageTextarea, 0, 0); //设置边框宽度
    lv_obj_set_style_bg_color(ui_CompageTextarea, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_CompageTextarea, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_size(ui_CompageTextarea, 200, 60); // 设置对象大小
    lv_obj_align(ui_CompageTextarea, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_clear_flag(ui_CompageTextarea,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_text_font(ui_CompageTextarea, &ui_font_Cuyuan24, 0);
    lv_textarea_set_align(ui_CompageTextarea, LV_TEXT_ALIGN_RIGHT);

// 退格按钮：右上角，透明背景，只显示图标
    ui_CompageBackBtn = lv_btn_create(ui_Computerpage);
    lv_obj_align(ui_CompageBackBtn,LV_ALIGN_RIGHT_MID,-10,-110);
    lv_obj_set_width(ui_CompageBackBtn,40);
    lv_obj_set_height(ui_CompageBackBtn,30);
    lv_obj_set_style_bg_opa(ui_CompageBackBtn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * btnlabel = lv_label_create(ui_CompageBackBtn);
    lv_label_set_text(btnlabel, LV_SYMBOL_BACKSPACE);
    lv_obj_set_style_text_font(btnlabel, &lv_font_montserrat_24, 0);
    lv_obj_center(btnlabel);

// 键盘与退格各绑一个回调
    lv_obj_add_event_cb(ui_CompageBtnM, ui_CompageBtnM_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_CompageBackBtn, ui_CompageBackBtn_event_cb, LV_EVENT_ALL, NULL);

}

/////////////////// SCREEN deinit ////////////////////
/**
 * @brief 计算器页销毁：空实现
 * @return 无
 * @note 栈是静态全局，不占堆；控件随 screen 释放。
 */
void ui_ComputerPage_screen_deinit(void)
{}
