/*
 * ============================================================================
 * 【中文导读】PageManager.c —— 页面管理器（本项目最精华的架构设计）
 * ----------------------------------------------------------------------------
 * 手表的每个界面（表盘/菜单/应用页）抽象成 Page_t：{init, initGUI, deinit,
 * show, hide…}。PageStack 维护页面栈：进入子页 push、返回 pop（自动调
 * deinit 释放资源）。切页带动画由 LVGL 实现。
 * 学习点：用 C 语言的"函数指针结构体+栈"实现面向对象的页面路由——
 * 与 Android 的 Activity 栈、Flipper 的 SceneManager 思想完全一致。
 * ============================================================================
 * 页面操作用栈，是因为页面导航天然是“后进先出”：最后打开的页面最先返回，栈的 push/pop 正好对
 * 应“进入/返回”，栈空表示已到根页面。栈适合所有需要回退、撤销、嵌套、最近一次语义的场景，比如
 * 函数调用、括号匹配、DFS、撤销操作；不适合需要随机访问、先进先出、按优先级取元素的场景。
 */

#include "../Inc/PageManager.h"
#include "../../GUI_App/Screens/Inc/ui_HomePage.h"
#include "../../GUI_App/Screens/Inc/ui_MenuPage.h"

PageStack_t PageStack; // 声明全局页面堆栈变量

static void page_stack_init(PageStack_t* stack) { // 初始化页面堆栈，将栈顶索引设置为 0，表示堆栈为空，全局变量 PageStack 在此处被初始化为一个空的页面堆栈，准备存储页面对象。
    stack->top = 0;
}

static uint8_t page_stack_push(PageStack_t* stack, Page_t* page) { // 将页面对象推入页面堆栈的函数。它接受一个指向 PageStack_t 结构体的指针和一个指向 Page_t 结构体的指针作为参数。函数首先检查堆栈是否已满，如果已满则返回 -1 表示错误。否则，将页面对象存储在堆栈的当前顶部索引位置，并将顶部索引加 1，表示堆栈中有一个新的页面被添加。最后返回 0 表示成功。
    if (stack->top >= MAX_DEPTH)
		return -1;
    stack->pages[stack->top++] = page; // 将页面对象存储在堆栈的当前顶部索引位置，并将顶部索引加 1，表示堆栈中有一个新的页面被添加。
	return 0;
}

static uint8_t page_stack_pop(PageStack_t* stack) { // 从页面堆栈中弹出页面对象的函数。它接受一个指向 PageStack_t 结构体的指针作为参数。函数首先检查堆栈是否为空，如果为空则返回 -1 表示错误。否则，将顶部索引减 1，表示堆栈中有一个页面被移除，并调用该页面的 deinit 函数进行反初始化操作。最后返回 0 表示成功。
    if (stack->top <= 0)
		return -1;
    stack->pages[--stack->top]->deinit();
    return 0;
}

static uint8_t page_stack_is_empty(const PageStack_t* stack) { // 这个函数用来判断页面栈是否为空：如果栈顶索引 top == 0，说明栈里没有任何页面，返回 1（真）；否则返回 0（假）。
    return stack->top == 0;
}

static Page_t* get_top_page(PageStack_t* stack) { // 获取页面堆栈顶部页面对象的函数。它接受一个指向 PageStack_t 结构体的指针作为参数。函数首先检查堆栈是否为空，如果为空则返回 NULL 表示没有页面对象可返回。否则，返回堆栈顶部索引减 1 的位置的页面对象指针，即当前显示的页面对象。
    // 检查栈是否为空
    if (stack->top == 0) {
        return NULL; // 如果栈为空，返回NULL
    }

    // 返回栈顶页面的指针
    return stack->pages[stack->top - 1];
}


/**
 * 获取当前页面（栈顶页面）的指针。
 *
 * @param NULL
 * @return 返回当前页面的指针，如果栈为空则返回NULL。
 */
Page_t* Page_Get_NowPage(void) { // 获取当前页面（栈顶页面）的指针的函数。它调用 get_top_page 函数获取页面堆栈顶部页面对象的指针，并返回该指针。如果堆栈为空，则返回 NULL。
    return get_top_page(&PageStack);
}


/**
 * back to previous page
 *
 * @param NULL
 * @return NULL
 */
void Page_Back(void) { // 大部分返回上一页，但是当返回后的栈变空的时候重新压入 Home 和 Menu 并显示 Menu
	if (page_stack_is_empty(&PageStack)) { // 检查页面堆栈是否为空
        // 栈为空时，不应发生
        return;
    }

    // 弹出当前页面
    page_stack_pop(&PageStack);

    if (page_stack_is_empty(&PageStack)) { // 检查页面堆栈是否为空
        // 如果栈为空，入栈并切换到MenuPage
        page_stack_push(&PageStack, &Page_Home);
        page_stack_push(&PageStack, &Page_Menu);
        Page_Menu.init();
        lv_scr_load_anim(*Page_Menu.page_obj, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);
    } else {
        // 切换到上一个页面
        Page_t *previous_page = PageStack.pages[PageStack.top - 1];
        previous_page->init();
        lv_scr_load_anim(*previous_page->page_obj, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true);
    }
}

/**
 * back to bottom page home page
 *
 * @param NULL
 * @return NULL
 */
void Page_Back_Bottom(void) { // 返回到底部页面（主页）的函数。它首先检查页面堆栈是否为空，如果为空则直接返回。然后，它使用一个 while 循环弹出除栈底的所有页面，直到只剩下栈底页面。接着，它调用栈底页面的 init 函数进行初始化，并使用 lv_scr_load_anim 函数加载并应用切换动画，将显示切换到栈底页面。

    if (page_stack_is_empty(&PageStack)) {
        // 栈为空时，不应发生
        return;
    }

    // 弹出除栈底的所有页面
    while(PageStack.top > 1)
        page_stack_pop(&PageStack);
    PageStack.pages[PageStack.top - 1]->init(); // 初始化新页面
    lv_scr_load_anim(*PageStack.pages[PageStack.top - 1]->page_obj, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true); // 加载并应用动画
}

/**
 * Load a new page to stack top
 *
 * @param newPage Page_t a new page
 * @return NULL
 */
void Page_Load(Page_t *newPage) { // 将新页面加载到堆栈顶部的函数。它首先检查页面堆栈是否已满，如果已满则直接返回。然后，它检查堆栈是否非空，如果非空则调用当前页面的 deinit 函数进行反初始化操作。接着，它将新页面推入堆栈，并调用新页面的 init 函数进行初始化。最后，它使用 lv_scr_load_anim 函数加载并应用切换动画，将显示切换到新页面。
	// 检查堆栈是否已满
    if (PageStack.top >= MAX_DEPTH - 1) {
        // 错误处理：堆栈满
        return;
    }

    // 如果堆栈非空，反初始化当前页面
    if (PageStack.top > 0) {
        PageStack.pages[PageStack.top - 1]->deinit();
    }

    // 将新页面推入堆栈
    page_stack_push(&PageStack, newPage);
    newPage->init(); // 初始化新页面
    lv_scr_load_anim(*newPage->page_obj, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, true); // 加载并应用动画
}

/**
 * initialize the page manager & start home page
 *
 * @param NULL
 * @return NULL
 */
void Pages_init(void) { // 初始化页面管理器并启动主页的函数。它首先调用 page_stack_init 函数初始化全局页面堆栈变量 PageStack。然后，它将主页 Page_Home 推入堆栈，并调用主页的 init 函数进行初始化。最后，它使用 lv_disp_load_scr 函数加载主页的显示对象，将显示切换到主页。
    page_stack_init(&PageStack);
    page_stack_push(&PageStack, &Page_Home);
    Page_Home.init();
    lv_disp_load_scr(*Page_Home.page_obj);
}
