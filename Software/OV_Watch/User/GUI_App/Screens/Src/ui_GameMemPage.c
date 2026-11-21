#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "../../ui.h"
#include "../../ui_helpers.h"
#include "../Inc/ui_MenuPage.h"
#include "../Inc/ui_GameMemPage.h"

#include "../../../Func/Inc/HWDataAccess.h"
/*
 * 【中文导读】ui_GameMemPage.c —— 记忆翻牌游戏（4x4 数字配对）
 * ----------------------------------------------------------------------------
 * 玩法：16 个格子藏着 8 对数字（1~8 各两张）。点第一张翻开，点第二张：
 *   · 数字相同 → 两张一起隐藏（HIDDEN），matrix 对应项清 0；
 *   · 数字不同 → 新的一张显示 0.4 透明度，上一张盖回去。
 * 全部翻完（matrix 全 0）后显示“重玩”按钮。
 * 
 * 实现要点：用 lv_btnmatrix 而不是 16 个独立按钮——一个控件管 16 格，
 * 通过 btnm_map 字符串数组描述每格的文本（"\n" 表示换行），
 * 再用 DRAW_PART_BEGIN 事件统一改每格的底色与文字透明度。
 * 
 * 已记录的隐患（不修改）：previous_id/present_id 声明为 uint8_t，
 * 初始化时赋 -1 实际是 255；首次点击时 matrix[255] 越界读 1 字节。
 * 另外 list_rand_number 的洗牌不是 Fisher-Yates（w 的下界随 i 变化），
 * 分布有偏，但对“看起来随机”够用。
 */

// 棋盘边长：4x4
#define MATRIX_SIZE 4

/**
 * @brief 记忆游戏运行时状态
 * @return 无
 * @note matrix 多留 1 个元素（17）作为哨兵；btnm_map 需要 16+4 个槽位放 4 个换行符。
 */
typedef struct {
    uint8_t matrix[MATRIX_SIZE * MATRIX_SIZE + 1];
    char    * btnm_map[MATRIX_SIZE * MATRIX_SIZE + MATRIX_SIZE];
    uint8_t label_opa_map[MATRIX_SIZE * MATRIX_SIZE];
    uint8_t previous_id;
    uint8_t present_id;
} memory_game_t;


// 0~8 的文本表：0 表示“已配对/空”，1~8 是牌面数字
static char * num_list[] = {"0","1","2","3","4","5","6","7","8"};
void Game_Mem_Init(void);
void list_rand_number(uint8_t arry[], uint8_t max_count, uint8_t count);
void updata_btnm(char * btnm_map[], uint8_t matrix[MATRIX_SIZE * MATRIX_SIZE + 1]);
uint8_t Game_Mem_Finish(void);

///////////////////// Page Manager //////////////////
// 把记忆游戏页注册进页面管理器
Page_t Page_GameMem = {ui_GameMemPage_screen_init, ui_GameMemPage_screen_deinit, &ui_GameMem_Page};

///////////////////// VARIABLES ////////////////////
// 全局唯一游戏实例：所有回调共享这一份状态
memory_game_t Game_Mem;

lv_obj_t * ui_GameMem_Page;
lv_obj_t * ui_GameMem_BtnM;
lv_obj_t * ui_new_game_btn;

///////////////////// FUNCTIONS ////////////////////
/**
 * @brief 棋盘事件：处理翻牌配对与逐格绘制
 * @param e LVGL 事件对象
 * @return 无
 * @note 一个回调同时处理 VALUE_CHANGED（逻辑）与 DRAW_PART_BEGIN（渲染），因为 btnmatrix 没有“每格单独样式”的接口。
 */
void ui_event_GameMemBtnM_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_VALUE_CHANGED)
    {
// selected_btn 返回被点格子的索引 0~15
        uint32_t id = lv_btnmatrix_get_selected_btn(obj);
        //const char * txt = lv_btnmatrix_get_btn_text(obj, id);
        Game_Mem.present_id = (uint8_t)id;
// 同一格重复点击直接忽略，避免自己和自己配对
        if(Game_Mem.present_id != Game_Mem.previous_id)
        {

// 两张数字相同：一起隐藏并把 matrix 清 0
            if(Game_Mem.matrix[Game_Mem.present_id] == Game_Mem.matrix[Game_Mem.previous_id])
            {

                lv_btnmatrix_set_btn_ctrl(ui_GameMem_BtnM,Game_Mem.present_id,LV_BTNMATRIX_CTRL_HIDDEN);
                lv_btnmatrix_set_btn_ctrl(ui_GameMem_BtnM,Game_Mem.previous_id,LV_BTNMATRIX_CTRL_HIDDEN);
                Game_Mem.matrix[Game_Mem.present_id] = 0;
                Game_Mem.matrix[Game_Mem.previous_id] = 0;
// 全部清空则显示“重玩”按钮
                if(Game_Mem_Finish())
                {
                    lv_obj_clear_flag(ui_new_game_btn,LV_OBJ_FLAG_HIDDEN);
                }
// 把当前格记为“上一张”，供下一次比较
                Game_Mem.previous_id = Game_Mem.present_id;

            }
            else
            {
// 不同：先盖回上一张（越界保护 0<=id<17）
								if(Game_Mem.previous_id >= 0 && Game_Mem.previous_id < 17 )
									Game_Mem.label_opa_map[Game_Mem.previous_id] = LV_OPA_0;
// 新翻开的格子用 0.4 透明度显示数字
                Game_Mem.label_opa_map[Game_Mem.present_id] = LV_OPA_100;
                Game_Mem.previous_id = Game_Mem.present_id;
            }

        }
    }

// 绘制阶段：每格统一棕底白字，透明度取 label_opa_map
    if(code == LV_EVENT_DRAW_PART_BEGIN)
    {
        lv_obj_draw_part_dsc_t * dsc = lv_event_get_param(e);


        if((dsc->id >= 0) && (dsc->label_dsc))
        {
            dsc->rect_dsc->bg_color = lv_color_hex(0xB07010);
            dsc->label_dsc->color = lv_color_white();
            dsc->label_dsc->opa = Game_Mem.label_opa_map[dsc->id];
        }
    }
}


/**
 * @brief “重玩”按钮：重置状态并重新发牌
 * @param e LVGL 事件对象
 * @return 无
 * @note 重置后必须 set_map + clear_btn_ctrl_all，否则上一局的 HIDDEN 状态会残留。
 */
void ui_event_NewGameBtn_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED)
    {
        Game_Mem_Init();
        lv_btnmatrix_set_map(ui_GameMem_BtnM, Game_Mem.btnm_map);
        lv_btnmatrix_clear_btn_ctrl_all(ui_GameMem_BtnM,LV_BTNMATRIX_CTRL_HIDDEN);
        lv_obj_add_flag(ui_new_game_btn,LV_OBJ_FLAG_HIDDEN);
    }
}

///////////////////// SCREEN init ////////////////////
/**
 * @brief 创建记忆游戏页：一个 240x240 棋盘 + 隐藏的重玩按钮
 * @return 无
 * @note init 里先 Game_Mem_Init() 发牌，再建控件，保证 btnm_map 已有内容。
 */
void ui_GameMemPage_screen_init(void)
{

    Game_Mem_Init();
// 先发牌再建棋盘；重复的分号是历史遗留，不影响语义
    ui_GameMem_Page = lv_obj_create(NULL);;
    ui_GameMem_BtnM = lv_btnmatrix_create(ui_GameMem_Page);
    lv_btnmatrix_set_map(ui_GameMem_BtnM, Game_Mem.btnm_map);
// 24 号中文字体，0 表示 LV_PART_MAIN
    lv_obj_set_style_text_font(ui_GameMem_BtnM, &ui_font_Cuyuan24, 0);
// one_checked：同一时刻只允许一个格子处于选中态
    lv_btnmatrix_set_one_checked(ui_GameMem_BtnM,true);
    lv_obj_set_style_border_width(ui_GameMem_BtnM,0,0);
    lv_obj_set_style_bg_opa(ui_GameMem_BtnM,0,0);
    lv_obj_set_size(ui_GameMem_BtnM,240,240);
    lv_obj_set_align(ui_GameMem_BtnM,LV_ALIGN_CENTER);


// 重玩按钮先隐藏，通关后才显示
    ui_new_game_btn = lv_btn_create(ui_GameMem_Page);
    lv_obj_align(ui_new_game_btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(ui_new_game_btn,90,45);
    lv_obj_t * new_game_btn_label = lv_label_create(ui_new_game_btn);
    lv_label_set_text(new_game_btn_label,"重 玩");
    lv_obj_align(new_game_btn_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(new_game_btn_label, &ui_font_Cuyuan24, 0);
    lv_obj_add_flag(ui_new_game_btn,LV_OBJ_FLAG_HIDDEN);

    //lv_btnmatrix_set_btn_ctrl(ui_GameMem_BtnM,1,LV_BTNMATRIX_CTRL_HIDDEN);
// 棋盘接 ALL 事件（逻辑+绘制），重玩按钮接点击
    lv_obj_add_event_cb(ui_GameMem_BtnM, ui_event_GameMemBtnM_handler, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_new_game_btn, ui_event_NewGameBtn_handler, LV_EVENT_ALL, NULL);
}

///////////////////// SCREEN deinit ////////////////////
/**
 * @brief 记忆游戏页销毁：空实现
 * @return 无
 * @note 状态在全局 Game_Mem 里，下次 init 会重新初始化，不需要额外清理。
 */
void ui_GameMemPage_screen_deinit(void)
{}

///////////////////// FUNCTIONS ////////////////////
/**
 * @brief 重置棋盘状态并重新发牌
 * @return 无
 * @note matrix 清 0、透明度清 0、上一张/当前张置 -1（uint8_t 下即 255）。
 */
void Game_Mem_Init(void)
{
    uint8_t x;

    for (x = 0; x < MATRIX_SIZE * MATRIX_SIZE + 1; x++) {
            Game_Mem.matrix[x] = 0;
    }

    for (x = 0; x < MATRIX_SIZE * MATRIX_SIZE; x++) {
        Game_Mem.label_opa_map[x] = LV_OPA_0;
    }
// -1 赋给 uint8_t 变成 255，这是后文越界读的根因
    Game_Mem.previous_id = -1;
    Game_Mem.present_id = -1;
// 发牌：16 格、每格 1~8、每个数字出现两次
    list_rand_number(Game_Mem.matrix,MATRIX_SIZE * MATRIX_SIZE,MATRIX_SIZE*MATRIX_SIZE/2);
    updata_btnm(Game_Mem.btnm_map,Game_Mem.matrix);
    //Game_Mem.label_opa_map[6] = LV_OPA_100;

}

/**
 * @brief 生成打乱后的数字序列
 * @param arry      输出数组（长度 max_count）
 * @param max_count 格子总数 16
 * @param count     不同数字个数 8
 * @return 无
 * @note 用 RTC 的秒做随机种子——手表没有独立 RNG，同一秒内重开一局牌序相同。
 */
void list_rand_number(uint8_t arry[], uint8_t max_count, uint8_t count)
{
    int w, t;
	int time=0;
	HW_DateTimeTypeDef nowtime;
	HWInterface.RealTimeClock.GetTimeDate(&nowtime);
	time = nowtime.Seconds;
// 种子只取秒（0~59），粒度很粗
    srand(time);

// 先按 i%8+1 填成 1..8 各两遍
    for (int i = 0; i < max_count; i++)
        arry[i] = (i % count) + 1;
    for (int i = 0; i < max_count; i++)
    {
// 洗牌：w 的范围是 [i, ...]，只与后面的元素交换，分布有偏
        w = rand() % (count - (i % count)) + i;
        if (w > max_count)  w = max_count - 1;

        t = arry[i];
        arry[i] = arry[w];
        arry[w] = t;
    }
}

/**
 * @brief 把数字矩阵翻译成 btnmatrix 的字符串表
 * @param btnm_map 输出：16 个数字 + 4 个换行 + 结尾空串
 * @param matrix   输入：每格的牌面值（0~8）
 * @return 无
 * @note 每 4 格插一个 "\n"，末尾必须是空字符串，否则 LVGL 会越界读。
 */
void updata_btnm(char * btnm_map[], uint8_t matrix[MATRIX_SIZE * MATRIX_SIZE + 1])
{
    uint8_t x,index;
    index = 0;
    for(x=0;x<MATRIX_SIZE * MATRIX_SIZE;x++)
    {
        if((index + 1) % (MATRIX_SIZE + 1) == 0)
        {
            btnm_map[index] = "\n";
            index++;
        }
        btnm_map[index] = num_list[matrix[x]];
        index++;
    }
    btnm_map[MATRIX_SIZE * MATRIX_SIZE + MATRIX_SIZE - 1] = "";
}

/**
 * @brief 判断是否全部配对完成
 * @return 1 完成 / 0 未完成
 * @note matrix 全 0 即通关；空实现会返回 1，所以初值必须清零。
 */
uint8_t Game_Mem_Finish(void)
{
    for(uint16_t x = 0; x < MATRIX_SIZE*MATRIX_SIZE; x++)
    {
        if(Game_Mem.matrix[x]!=0)
            return 0;
    }
    return 1;
}

