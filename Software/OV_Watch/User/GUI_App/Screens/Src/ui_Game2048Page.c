#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "../../ui.h"
#include "../../ui_helpers.h"
#include "../Inc/ui_MenuPage.h"
#include "../Inc/ui_Game2048Page.h"
#include "../../../Func/Inc/HWDataAccess.h"

// 小栈深度：一行最多 4 个数，留 1 个余量
/*
 * 【中文导读】ui_Game2048Page.c —— 2048 小游戏（纯软件实现，约 580 行）
 * ----------------------------------------------------------------------------
 * 棋盘用 lv_btnmatrix 的 4x4 网格显示，手势（上/下/左/右滑）触发移动。
 * 核心数据：Game_2048.matrix[4][4] 存的是“指数”而不是数值——
 *   0=空格，1=2，2=4，3=8，…，11=2048；显示时用 1<<exp 还原。
 * 这样合并两个相同数字只需 exp+1，比分值快且不会溢出。
 * 
 * 移动算法：每个方向都用同一个套路——把一行/列的非零元素压进一个小栈,
 * 边压边看栈顶两个是否相等，相等就弹出两个、压入 exp+1 并加分；
 * 最后从栈里倒出来写回矩阵。四个 move_* 只是遍历方向不同。
 * 
 * 计分：合并出的新牌面值累加，*score += 1<<(temp-1)。
 * 随机：addRandom 在空格里等概率选一个，填入 2（90%）或 4（10%）。
 * 
 * 已知问题（记录不改）：
 *   1) game_over() 只判断“还有没有空格”，棋盘一满就判负，即使还能合并；
 *   2) Stack_init 的 memset 长度写成 STACK_DEPTH（5 字节），而 Data 是
 *      uint16_t[5]（10 字节），只清了一半；
 *   3) 文字颜色阈值写的是 num > 516（不是 512），512 与 1024 之间判断偏一格。
 */
#define STACK_DEPTH 5
#define MATRIX_SIZE 4

/**
 * @brief 移动时使用的临时栈
 * @return 无
 * @note 只在 move_* 内部使用，生命周期限于一次移动，不需要加锁。
 */
typedef struct
{
    uint16_t Data[STACK_DEPTH];
    uint8_t Top_Point;

}Stack_T;

static void Stack_init(Stack_T* stack);
static uint8_t Stack_Push(Stack_T* stack, uint16_t datain);
static uint8_t Stack_Pop(Stack_T* stack);
static uint8_t Stack_isEmpty(Stack_T* stack);
static void Stack_Clear(Stack_T* stack);

/**
 * @brief 2048 游戏全局状态
 * @return 无
 * @note score 是累计得分；matrix 存指数；game_over 每帧手势前重算。
 */
typedef struct {
    uint16_t score;
    uint16_t matrix[MATRIX_SIZE][MATRIX_SIZE];
    char    * btnm_map[MATRIX_SIZE * MATRIX_SIZE + MATRIX_SIZE];
    uint8_t     game_over;
} ui_Game2048_t;

// 按牌面值区分的背景色，深绿→浅绿渐变
#define GAME_2048_TEXT_BLACK_COLOR      lv_color_hex(0X000000)
#define GAME_2048_TEXT_WHITE_COLOR      lv_color_hex(0Xffffff)

#define GAME_2048_NUMBER_EMPTY_COLOR    lv_color_hex(0x023535)
#define GAME_2048_NUMBER_2_COLOR        lv_color_hex(0x023535)
#define GAME_2048_NUMBER_4_COLOR        lv_color_hex(0x265C4B)
#define GAME_2048_NUMBER_8_COLOR        lv_color_hex(0x146551)
#define GAME_2048_NUMBER_16_COLOR       lv_color_hex(0x146551)
#define GAME_2048_NUMBER_32_COLOR       lv_color_hex(0x007566)
#define GAME_2048_NUMBER_64_COLOR       lv_color_hex(0x007566)
#define GAME_2048_NUMBER_128_COLOR      lv_color_hex(0x009A8D)
#define GAME_2048_NUMBER_256_COLOR      lv_color_hex(0x008F8C)
#define GAME_2048_NUMBER_512_COLOR      lv_color_hex(0x0CABAB)
#define GAME_2048_NUMBER_1024_COLOR     lv_color_hex(0x8FC1B5)
#define GAME_2048_NUMBER_2048_COLOR     lv_color_hex(0x8FD1B5)

// 指数 → 显示文本的映射表，下标 = exp-1
static char *Str2048Nums[]={"2","4","8","16","32","64","128","256","512","1024","2048"};

static void Game_2048_init(void);
static void init_matrix_num(uint16_t matrix[MATRIX_SIZE][MATRIX_SIZE]);
static void update_btnm_map(char * btnm_map[], uint16_t matrix[MATRIX_SIZE][MATRIX_SIZE]);
static void addRandom(uint16_t matrix[MATRIX_SIZE][MATRIX_SIZE]);
static uint8_t move_up(uint16_t * score, uint16_t matrix[MATRIX_SIZE][MATRIX_SIZE]);
static uint8_t move_down(uint16_t * score, uint16_t matrix[MATRIX_SIZE][MATRIX_SIZE]);
static uint8_t move_right(uint16_t * score, uint16_t matrix[MATRIX_SIZE][MATRIX_SIZE]);
static uint8_t move_left(uint16_t * score, uint16_t matrix[MATRIX_SIZE][MATRIX_SIZE]);
static uint8_t game_over(uint16_t matrix[MATRIX_SIZE][MATRIX_SIZE]);
static lv_color_t get_num_color(uint16_t num);


///////////////////// Page Manager //////////////////
// 注册到页面管理器
Page_t Page_Game_2048 = {ui_Game2048Page_screen_init, ui_Game2048Page_screen_deinit, &ui_Game2048Page};

///////////////////// VARIABLES ////////////////////
ui_Game2048_t Game_2048;

lv_obj_t * ui_Game2048Page;
lv_obj_t * ui_Game2048BtnM;
lv_obj_t * ui_Game2048ScLabel;

///////////////////// FUNCTIONS ////////////////////
/**
 * @brief 页面手势回调：按滑动方向移动棋盘
 * @param e LVGL 事件对象
 * @return 无
 * @note 先判 game_over 再处理手势；只有真的发生移动（moved=1）才生成新数并重绘。
 */
static void ui_event_Game2048Page(lv_event_t * e)
{
   lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);

    if(event_code == LV_EVENT_GESTURE)
    {
// 每帧重算一次“是否还有空格”，作为最简单的终局判定
				Game_2048.game_over = game_over(Game_2048.matrix);
        if(!Game_2048.game_over)
        {
					uint8_t moved = 0;
// 四个方向分别调用对应 move_*，返回值表示棋盘是否变化
					if(lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_LEFT)
					{
							moved = move_left(&Game_2048.score,Game_2048.matrix);
					}
					else if(lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_RIGHT)
					{
							moved = move_right(&Game_2048.score,Game_2048.matrix);
					}
					else if(lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_TOP)
					{
							moved = move_up(&Game_2048.score,Game_2048.matrix);
					}
					else if(lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_BOTTOM)
					{
							moved = move_down(&Game_2048.score,Game_2048.matrix);
					}
// 没变化就不加新数、不重绘，避免“滑了但没动”也刷屏
					if(moved)
					{
// 有效移动后随机加一个 2/4
							addRandom(Game_2048.matrix);
							update_btnm_map(Game_2048.btnm_map, Game_2048.matrix);
							lv_btnmatrix_set_map(ui_Game2048BtnM, Game_2048.btnm_map);
// 10 字节缓冲："Score:65535" 刚好放得下
							uint8_t strbuf[10];
							sprintf(strbuf,"Score:%d",Game_2048.score);
							lv_label_set_text(ui_Game2048ScLabel,strbuf);
					}
				}
    }
}

/**
 * @brief 棋盘绘制回调：按格子里的指数取背景色与文字色
 * @param e LVGL 事件对象
 * @return 无
 * @note DRAW_PART_BEGIN 对每个格子触发一次；dsc->id 是格子索引 0~15。
 */
static void ui_event_Game2048BtnM(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_DRAW_PART_BEGIN)
    {
        lv_obj_draw_part_dsc_t * dsc = lv_event_get_param(e);

        /*Change the draw descriptor the button*/
        if((dsc->id >= 0) && (dsc->label_dsc))
        {
            uint16_t x, y, num;

// id → 行列：行优先存储，与 update_btnm_map 的顺序一致
            x = (uint16_t)((dsc->id) / MATRIX_SIZE);
            y = (dsc->id) % MATRIX_SIZE;
// 指数还原成实际数值：1<<exp
            num = (uint16_t)(1 << (Game_2048.matrix[x][y]));

            dsc->rect_dsc->bg_color = get_num_color(num);
// 阈值 516 是历史笔误（应为 512），512 牌仍用白字
            if (num > 516)
                dsc->label_dsc->color = GAME_2048_TEXT_BLACK_COLOR;
            else
                dsc->label_dsc->color = GAME_2048_TEXT_WHITE_COLOR;

        }
    }
    if(code == LV_EVENT_DRAW_PART_END)
    {
        lv_obj_draw_part_dsc_t * dsc = lv_event_get_param(e);
    }
}

/**
 * @brief “重玩”按钮：长按重置棋局
 * @param e LVGL 事件对象
 * @return 无
 * @note 用 LONG_PRESSED 而不是 CLICKED，避免游戏中误触清档。
 */
static void ui_event_new_game_btn(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_LONG_PRESSED)
    {
			Game_2048_init();
      uint8_t strbuf[10];
      sprintf(strbuf,"Score:%d",Game_2048.score);
      lv_label_set_text(ui_Game2048ScLabel,strbuf);
    }
}


///////////////////// SCREEN init ////////////////////
/**
 * @brief 创建 2048 页：棋盘 + 重玩按钮 + 分数 Label
 * @return 无
 * @note Game_2048_init() 在创建 btnmatrix 之后调用，因为它内部要 set_map。
 */
void ui_Game2048Page_screen_init(void)
{

    ui_Game2048Page = lv_obj_create(NULL);

// 整页禁止滚动，手势才能稳定被识别为滑动
    lv_obj_clear_flag(ui_Game2048Page,LV_OBJ_FLAG_SCROLLABLE);
    ui_Game2048BtnM = lv_btnmatrix_create(ui_Game2048Page);
    Game_2048_init();

    lv_obj_set_style_text_font(ui_Game2048BtnM, &ui_font_Cuyuan20, 0);
    lv_btnmatrix_set_one_checked(ui_Game2048BtnM,true);
// 棋盘本身不可点击：所有操作都走页面级手势
    lv_obj_clear_flag(ui_Game2048BtnM, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_border_width(ui_Game2048BtnM,0,0);
    lv_obj_set_style_bg_opa(ui_Game2048BtnM,0,0);
    lv_obj_set_size(ui_Game2048BtnM,240,240);
    lv_obj_set_align(ui_Game2048BtnM,LV_ALIGN_BOTTOM_MID);


// 重玩按钮 85x35，左上角
    lv_obj_t * new_game_btn = lv_btn_create(ui_Game2048Page);
    lv_obj_align(new_game_btn, LV_ALIGN_TOP_LEFT, 12, 8);
    lv_obj_set_size(new_game_btn,85,35);
    lv_obj_t * new_game_btn_label = lv_label_create(new_game_btn);
    lv_label_set_text(new_game_btn_label,"重 玩");
    lv_obj_align(new_game_btn_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(new_game_btn_label, &ui_font_Cuyuan20, 0);

// 分数显示在右上角
    ui_Game2048ScLabel = lv_label_create(ui_Game2048Page);
    uint8_t strbuf[10];
    sprintf(strbuf,"Score:%d",Game_2048.score);
    lv_label_set_text(ui_Game2048ScLabel,strbuf);
    lv_obj_align(ui_Game2048ScLabel, LV_ALIGN_TOP_RIGHT, -10, 15);
    lv_obj_set_style_text_font(ui_Game2048ScLabel, &ui_font_Cuyuan20, 0);


    lv_obj_add_event_cb(ui_Game2048Page, ui_event_Game2048Page, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_Game2048BtnM, ui_event_Game2048BtnM, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(new_game_btn, ui_event_new_game_btn, LV_EVENT_ALL, NULL);

}

/////////////////// SCREEN deinit ////////////////////
/**
 * @brief 2048 页销毁：空实现
 * @return 无
 * @note 棋盘状态在全局 Game_2048 中，下次 init 会重置；无定时器。
 */
void ui_Game2048Page_screen_deinit(void)
{}

///////////////////// FUNCTIONS ////////////////////
/**
 * @brief 重置棋局：清分数、清 map、重建两个初始数字
 * @return 无
 * @note btnm_map 的换行槽位（4/9/14）与结尾空串（19）必须重新写，因为 update_btnm_map 只填数字槽。
 */
static void Game_2048_init(void)
{
    Game_2048.score = 0;
    Game_2048.game_over = 0;

    memset(Game_2048.btnm_map,0,MATRIX_SIZE * MATRIX_SIZE + MATRIX_SIZE);

    Game_2048.btnm_map[4]="\n";
    Game_2048.btnm_map[9]="\n";
    Game_2048.btnm_map[14]="\n";
    Game_2048.btnm_map[19]="";

    init_matrix_num(Game_2048.matrix);
    update_btnm_map(Game_2048.btnm_map, Game_2048.matrix);
    lv_btnmatrix_set_map(ui_Game2048BtnM, Game_2048.btnm_map);

}

/**
 * @brief 初始化矩阵并放入两个随机数
 * @param matrix 4x4 的指数矩阵
 * @return 无
 */
static void init_matrix_num(uint16_t matrix[MATRIX_SIZE][MATRIX_SIZE])
{
    uint8_t x, y;

    for (x = 0; x < MATRIX_SIZE; x++) {
        for (y = 0; y < MATRIX_SIZE; y++) {
            matrix[x][y] = 0;
        }
    }

    /* 初始化两个随机位置的随机数 */
// 标准 2048 开局：两个随机格子
    addRandom(matrix);
    addRandom(matrix);
}

/**
 * @brief 把指数矩阵翻译成 btnmatrix 的字符串表
 * @param btnm_map 20 个槽位：16 个数字 + 3 个换行 + 结尾空串
 * @param matrix   指数矩阵
 * @return 无
 * @note 每 5 个槽位插一个换行（4 个数字 + 1 个 "\n"）。
 */
static void update_btnm_map(char * btnm_map[], uint16_t matrix[MATRIX_SIZE][MATRIX_SIZE])
{
    uint8_t x, y, index;
    index = 0;
    for (x = 0; x < MATRIX_SIZE; x++)
    {
        for (y = 0; y < MATRIX_SIZE; y++)
        {

            if(((index + 1) % 5) == 0)
                index++;

            if(matrix[x][y] != 0)
            {
                btnm_map[index] = Str2048Nums[matrix[x][y]-1];
            }
            else
                btnm_map[index] = " ";

            index++;
        }
    }
}

/**
 * @brief 在随机空位放入一个新数字（2 或 4）
 * @param matrix 指数矩阵
 * @return 无
 * @note 先用 RTC 秒初始化一次 srand；static initialized 保证只播种一次。
 */
static void addRandom(uint16_t matrix[MATRIX_SIZE][MATRIX_SIZE])
{
    static uint8_t initialized = 0;
    uint16_t x, y;
    uint16_t r, len = 0;
    uint16_t n, list[MATRIX_SIZE * MATRIX_SIZE][2];

// 只播种一次：同一局内连续调用不会重置随机序列
    if (!initialized)
		{
			int time=0;
            HW_DateTimeTypeDef nowtime;
            HWInterface.RealTimeClock.GetTimeDate(&nowtime);
			time = nowtime.Seconds;
      srand(time);
      initialized = 1;
    }

// 收集所有空格坐标，len 为空格数
    for (x = 0; x < MATRIX_SIZE; x++) {
        for (y = 0; y < MATRIX_SIZE; y++) {
            if (matrix[x][y] == 0) {
                list[len][0] = x;
                list[len][1] = y;
                len++;
            }
        }
    }

    if (len > 0) {
// 等概率选一个空格
        r = rand() % len;
        x = list[r][0];
        y = list[r][1];
// (rand()%10)/9 得 0 或 1，+1 后为 1 或 2 → 牌面 2 或 4
        n = ((rand() % 10) / 9) + 1;
        matrix[x][y] = n;
    }
}

/**
 * @brief 向上移动：每列独立处理
 * @param score  得分累加（传出）
 * @param matrix 指数矩阵（原地修改）
 * @return 1 表示棋盘有变化，0 表示无变化
 * @note 从下往上压栈，因此栈顶是行号小的元素，弹出顺序正好是从上往下。
 */
static uint8_t move_up(uint16_t * score, uint16_t matrix[MATRIX_SIZE][MATRIX_SIZE])
{
    uint8_t moved = 0;
    uint8_t x,y;
    Stack_T num_stack;
    Stack_init(&num_stack);
    for (y = 0; y < MATRIX_SIZE; y++)
    {
        for(x=0; x<MATRIX_SIZE; x++)
        {
// 倒序压栈：matrix[3] 先入栈，栈顶是 matrix[0] 方向
            if(matrix[MATRIX_SIZE-x-1][y] != 0)
                Stack_Push(&num_stack,matrix[MATRIX_SIZE-x-1][y]);
            else if(x > 0 && matrix[MATRIX_SIZE-x][y] != 0)
                moved = 1;
// 栈顶两个相等就合并：弹出两个、压入 exp+1
            if(num_stack.Top_Point>1)
            {
                if(num_stack.Data[num_stack.Top_Point-1] == num_stack.Data[num_stack.Top_Point-2])
                {
                    uint16_t temp;
                    temp = num_stack.Data[num_stack.Top_Point-1] + 1;
                    Stack_Pop(&num_stack);
                    Stack_Pop(&num_stack);
                    Stack_Push(&num_stack,temp);
// 得分增加合并后的牌面值 2^exp
                    *score += 1<<(temp-1);
                    moved = 1;
                }
            }
        }
// 只有发生过移动/合并才回写，减少无谓写操作
        if(moved)
        {
            for(x=0; x<MATRIX_SIZE; x++)
            {
                if(!Stack_isEmpty(&num_stack))
                {
                    matrix[x][y] = num_stack.Data[num_stack.Top_Point-1];
                    Stack_Pop(&num_stack);
                }
                else
                {
                    matrix[x][y] = 0;
                }

            }
        }
        Stack_Clear(&num_stack);
    }
    return moved;
}

/**
 * @brief 向下移动
 * @param score 得分累加
 * @param matrix 指数矩阵
 * @return 1 有变化 / 0 无变化
 */
static uint8_t move_down(uint16_t * score, uint16_t matrix[MATRIX_SIZE][MATRIX_SIZE])
{
    uint8_t moved = 0;
    uint8_t x,y;
    Stack_T num_stack;
    Stack_init(&num_stack);
    for (y = 0; y < MATRIX_SIZE; y++)
    {
        for(x=0; x<MATRIX_SIZE; x++)
        {
            if(matrix[x][y] != 0)
                Stack_Push(&num_stack,matrix[x][y]);
            else if(x > 0 && matrix[x-1][y] != 0)
                moved = 1;
            if(num_stack.Top_Point>1)
            {
                if(num_stack.Data[num_stack.Top_Point-1] == num_stack.Data[num_stack.Top_Point-2])
                {
                    uint16_t temp;
                    temp = num_stack.Data[num_stack.Top_Point-1] + 1;
                    Stack_Pop(&num_stack);
                    Stack_Pop(&num_stack);
                    Stack_Push(&num_stack,temp);
                    *score += 1<<(temp-1);
                    moved = 1;

                }
            }
        }
        if(moved)
        {
            for(x=0; x<MATRIX_SIZE; x++)
            {
                if(!Stack_isEmpty(&num_stack))
                {
                    matrix[MATRIX_SIZE-x-1][y] = num_stack.Data[num_stack.Top_Point-1];
                    Stack_Pop(&num_stack);
                }
                else
                {
                    matrix[MATRIX_SIZE-x-1][y] = 0;
                }
            }
        }
        Stack_Clear(&num_stack);
    }
    return moved;
}

/**
 * @brief 向右移动
 * @param score 得分累加
 * @param matrix 指数矩阵
 * @return 1 有变化 / 0 无变化
 */
static uint8_t move_right(uint16_t * score, uint16_t matrix[MATRIX_SIZE][MATRIX_SIZE])
{
    uint8_t moved = 0;
    uint8_t x,y;
    Stack_T num_stack;
    Stack_init(&num_stack);
    for (x = 0; x < MATRIX_SIZE; x++)
    {
        for(y=0; y<MATRIX_SIZE; y++)
        {
            if(matrix[x][y] != 0)
                Stack_Push(&num_stack,matrix[x][y]);
            else if(y > 0 && matrix[x][y-1] != 0)
                moved = 1;
            if(num_stack.Top_Point>1)
            {
                if(num_stack.Data[num_stack.Top_Point-1] == num_stack.Data[num_stack.Top_Point-2])
                {
                    uint16_t temp;
                    temp = num_stack.Data[num_stack.Top_Point-1] + 1;
                    Stack_Pop(&num_stack);
                    Stack_Pop(&num_stack);
                    Stack_Push(&num_stack,temp);
                    *score += 1<<(temp-1);
                    moved = 1;

                }
            }
        }
        if(moved)
        {
            for(y=0; y<MATRIX_SIZE; y++)
            {
                if(!Stack_isEmpty(&num_stack))
                {
                    matrix[x][MATRIX_SIZE-y-1] = num_stack.Data[num_stack.Top_Point-1];
                    Stack_Pop(&num_stack);
                }
                else
                {
                    matrix[x][MATRIX_SIZE-y-1] = 0;
                }
            }
        }
        Stack_Clear(&num_stack);
    }
    return moved;
}

/**
 * @brief 向左移动
 * @param score 得分累加
 * @param matrix 指数矩阵
 * @return 1 有变化 / 0 无变化
 */
static uint8_t move_left(uint16_t * score, uint16_t matrix[MATRIX_SIZE][MATRIX_SIZE])
{
    uint8_t moved = 0;
    uint8_t x,y;
    Stack_T num_stack;
    Stack_init(&num_stack);
    for (x = 0; x < MATRIX_SIZE; x++)
    {
        for(y=0; y<MATRIX_SIZE; y++)
        {
            if(matrix[x][MATRIX_SIZE-y-1] != 0)
                Stack_Push(&num_stack,matrix[x][MATRIX_SIZE-y-1]);
            else if(y > 0 && matrix[x][MATRIX_SIZE-y] != 0)
                moved = 1;
            if(num_stack.Top_Point>1)
            {
                if(num_stack.Data[num_stack.Top_Point-1] == num_stack.Data[num_stack.Top_Point-2])
                {
                    uint16_t temp;
                    temp = num_stack.Data[num_stack.Top_Point-1] + 1;
                    Stack_Pop(&num_stack);
                    Stack_Pop(&num_stack);
                    Stack_Push(&num_stack,temp);
                    *score += 1<<(temp-1);
                    moved = 1;
                }
            }
        }
        if(moved)
        {
            for(y=0; y<MATRIX_SIZE; y++)
            {
                if(!Stack_isEmpty(&num_stack))
                {
                    matrix[x][y] = num_stack.Data[num_stack.Top_Point-1];
                    Stack_Pop(&num_stack);
                }
                else
                {
                    matrix[x][y] = 0;
                }

            }
        }
        Stack_Clear(&num_stack);
    }
    return moved;
}

/**
 * @brief 终局判定：棋盘是否已无空格
 * @param matrix 指数矩阵
 * @return 1 无空格（判负）/ 0 仍有空格
 * @note 注意这不是完整的 2048 终局判定——棋盘满但仍可合并时也会返回 1。
 */
static uint8_t game_over(uint16_t matrix[MATRIX_SIZE][MATRIX_SIZE])
{
  uint8_t x, y;
	uint8_t count = 0;

	for (x = 0; x < MATRIX_SIZE; x++) {
		for (y = 0; y < MATRIX_SIZE; y++) {
			if (matrix[x][y] == 0) {
				count++;
			}
		}
	}
	if(count==0)
	{
			return 1;
	}
	return 0;
}

/**
 * @brief 按牌面数值取背景色
 * @param num 实际牌面值（1,2,4,...,2048）
 * @return 对应的 lv_color_t
 * @note 数值来自 1<<exp；switch 覆盖到 2048，超出则用 2048 的颜色。
 */
static lv_color_t get_num_color(uint16_t num)
{
    lv_color_t color;

    switch (num)
    {
        case 0:     color = GAME_2048_NUMBER_EMPTY_COLOR;   break;
        case 1:     color = GAME_2048_NUMBER_EMPTY_COLOR;   break;
        case 2:     color = GAME_2048_NUMBER_2_COLOR;       break;
        case 4:     color = GAME_2048_NUMBER_4_COLOR;       break;
        case 8:     color = GAME_2048_NUMBER_8_COLOR;       break;
        case 16:    color = GAME_2048_NUMBER_16_COLOR;      break;
        case 32:    color = GAME_2048_NUMBER_32_COLOR;      break;
        case 64:    color = GAME_2048_NUMBER_64_COLOR;      break;
        case 128:   color = GAME_2048_NUMBER_128_COLOR;     break;
        case 256:   color = GAME_2048_NUMBER_256_COLOR;     break;
        case 512:   color = GAME_2048_NUMBER_512_COLOR;     break;
        case 1024:  color = GAME_2048_NUMBER_1024_COLOR;    break;
        case 2048:  color = GAME_2048_NUMBER_2048_COLOR;    break;
        default:    color = GAME_2048_NUMBER_2048_COLOR;    break;
    }
    return color;
}

/**
 * @brief 初始化栈：清数据、栈顶指针归零
 * @return 无
 * @note memset 长度误写为 STACK_DEPTH（字节），只清掉一半的 uint16_t 数组。
 */
static void Stack_init(Stack_T* stack)
{
    memset(stack->Data,0,STACK_DEPTH);
    stack->Top_Point = 0;
}

/**
 * @brief 压栈
 * @param stack  目标栈
 * @param datain 要压入的指数
 * @return 0 成功 / 255(-1) 栈满
 * @note 栈满条件是 Top_Point == STACK_DEPTH-1，即最多存 4 个元素，正好一行。
 */
static uint8_t Stack_Push(Stack_T* stack, uint16_t datain)
{
  if(stack->Top_Point == STACK_DEPTH - 1)
    {return -1;}

    stack->Data[stack->Top_Point++] = datain;
    return 0;
}

/**
 * @brief 弹栈
 * @param stack 目标栈
 * @return 0 成功 / 255(-1) 栈空
 * @note 先把被弹位置清 0 再自减，指针始终指向“下一个空位”。
 */
static uint8_t Stack_Pop(Stack_T* stack)
{
  if(stack->Top_Point == 0)
    {return -1;}

    stack->Data[stack->Top_Point--] = NULL;
    return 0;
}

/**
 * @brief 判空
 * @param stack 目标栈
 * @return 1 空 / 0 非空
 */
static uint8_t Stack_isEmpty(Stack_T* stack)
{
    if(stack->Top_Point == 0)
    {return 1;}

    return 0;
}

/**
 * @brief 清空栈：循环弹出直到空
 * @return 无
 * @note move_* 每处理完一行/列都调用一次，复用同一个栈对象。
 */
static void Stack_Clear(Stack_T* stack)
{
    while(!Stack_isEmpty(stack))
    {
        Stack_Pop(stack);
    }
}

