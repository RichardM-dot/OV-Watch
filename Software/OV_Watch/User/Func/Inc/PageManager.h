/**
 * @file    PageManager.h
 * @brief   页面栈管理：以"栈"的方式管理 LVGL 页面（屏幕）的前进/后退。
 *
 * 每个页面由 Page_t 描述（初始化/反初始化回调 + 页面根对象指针），
 * 入栈时调用 init、出栈时调用 deinit，从而在切页时正确创建/销毁 LVGL 对象，
 * 避免屏幕切换过程中内存持续增长。
 */
#ifndef PAGE_STACK_H
#define PAGE_STACK_H

#include "../../GUI_App/ui.h"

// 页面栈深度，栈深度代表了页面堆栈中可以存储的最大页面数量。这个值的选择取决于应用程序的需求和系统资源的限制。较大的栈深度允许更多的页面同时存在，但也会占用更多的内存资源。较小的栈深度则节省内存，但可能限制了页面导航的灵活性。
#define MAX_DEPTH 6 

// 页面结构体
typedef struct {
    void (*init)(void); // 指向一个参数为 void，返回值为 void的函数指针，用于初始化页面
    void (*deinit)(void); // 指向一个参数为 void，返回值为 void的函数指针，用于反初始化页面
    lv_obj_t **page_obj; // 写法为 lv_obj_t **page_obj;，表示 page_obj 是一个指向指针的指针，指向一个 lv_obj_t 类型的对象。这个对象通常是页面的根对象，用于在 LVGL 中管理页面的显示和交互。
} Page_t;


// 页面堆栈结构体
typedef struct { // 页面堆栈结构体
    Page_t* pages[MAX_DEPTH]; // 写法为 Page_t* pages[MAX_DEPTH];，表示 pages 是一个指针数组，数组的每个元素都是一个指向 Page_t 类型的指针。这个数组用于存储页面堆栈中的页面，每个页面都是一个 Page_t 类型的对象。
    uint8_t top; // 写法为 uint8_t top;，表示 top 是一个无符号 8 位整数，用于表示页面堆栈的当前顶部索引。它指示了堆栈中当前有多少个页面，以及下一个页面将被推入堆栈的位置。
} PageStack_t;

extern PageStack_t PageStack; // 声明全局页面堆栈变量


/** 获取当前栈顶页面（即正在显示的页面） */
Page_t* Page_Get_NowPage(void);
/** 返回上一页：出栈并销毁当前页，恢复并重新初始化上一页 */
void Page_Back(void);
/** 直接退回栈底页面（主页），中间页面全部出栈 */
void Page_Back_Bottom(void);
/** 加载新页面：当前页入栈，初始化并显示 newPage */
void Page_Load(Page_t *newPage);
/** 页面系统初始化（清空页面栈） */
void Pages_init(void);


#endif // PAGE_STACK_H
