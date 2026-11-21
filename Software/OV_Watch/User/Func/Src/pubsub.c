/*
 * ============================================================================
 * 【中文导读】pubsub.c —— 发布/订阅消息总线（46 行的极简观察者实现），模拟器使用不需要掌握
 * ----------------------------------------------------------------------------
 * 结构：发布者持有一条“订阅者回调”单向链表。
 *   Publisher_subscribe()  → 头插一个节点（malloc）
 *   Publisher_unsubscribe()→ 找到节点、摘链、free
 *   Publisher_publish()    → 遍历链表，同步调用每个回调
 *
 * 本工程里的实际用途：**PC 模拟器**把 SDL 键盘事件分发给页面逻辑
 * （lv_sim_vscode_win/user_test/GUI_App/ui.c 里 init/subscribe）。
 * 固件侧只定义了 SDL_KeyBoard_Publisher，没有调用点；手表按键走的是
 * KeyTask → Key_MessageQueue → ScrRenewTask 这条队列路径。
 *
 * 注意：
 *   1) 链表用 malloc/free，没有锁，**不是线程安全的**；
 *   2) publish 是同步回调，回调里不能做耗时/阻塞操作；
 *   3) 发布者销毁时不会自动释放链表（本工程是全局变量，生命周期到进程结束）。
 * ============================================================================
 */

#include "../Inc/pubsub.h"
#include <stdlib.h>
#include <stdio.h>

/* 全局发布者：模拟器用它把 SDL 键盘事件分发给界面（固件未使用） */
PubSub_Publisher_t SDL_KeyBoard_Publisher;


/**
 * @brief  初始化发布者：把订阅者链表头置空。
 * @param  publisher 发布者对象（调用者负责分配）
 * @retval None
 *
 * 注意：这里只清链表头，不释放旧节点；对同一个 publisher 重复 init
 * 会泄漏之前订阅的节点。本工程只在启动时调用一次。
 */
// Initialize the publisher
void Publisher_init(PubSub_Publisher_t *publisher) {
    publisher->subscribers = NULL;
}

/**
 * @brief  订阅：头插一个订阅者节点。
 * @param  publisher  发布者
 * @param  subscriber 订阅者回调（函数指针）
 * @retval None
 *
 * 设计细节：
 *   - 头插 O(1)，不保证回调顺序（后订阅的先被调用）；
 *   - malloc 失败时静默返回，调用者无法感知订阅失败；
 *   - 同一个回调可以重复订阅（会产生多个节点、被调用多次）。
 */
// Subscribe a new subscriber
void Publisher_subscribe(PubSub_Publisher_t *publisher, Subscriber subscriber) {
    /* 为链表节点申请内存：节点本身很小（两个指针） */
    SubscriberNode *node = (SubscriberNode *)malloc(sizeof(SubscriberNode));
    if (node == NULL) {
        // Handle memory allocation failure
        return;                     /* 分配失败：静默放弃订阅 */
    }
    node->subscriber = subscriber;
    node->next = publisher->subscribers;   /* 新节点指向原链表头 */
    publisher->subscribers = node;         /* 更新链表头（头插） */
}

/**
 * @brief  取消订阅：从链表中摘除第一个匹配的节点并释放。
 * @param  publisher  发布者
 * @param  subscriber 要取消的回调
 * @retval None
 *
 * 技巧：用“二级指针”遍历，`current` 指向“指向当前节点的指针”。
 * 这样删除节点时不需要单独处理“删的是头节点”的情况：
 *   *current = (*current)->next;  // 让前一个节点的 next 跳过当前节点
 * 如果只订阅了一次，找到的就是它；重复订阅只会删第一个。
 */
// Unsubscribe an existing subscriber
void Publisher_unsubscribe(PubSub_Publisher_t *publisher, Subscriber subscriber) {
    SubscriberNode **current = &publisher->subscribers;  /* 指向链表头的指针 */
    while (*current != NULL) {
        if ((*current)->subscriber == subscriber) {       /* 找到目标回调 */
            SubscriberNode *to_delete = *current;
            *current = (*current)->next;                  /* 摘链：前驱指向后继 */
            free(to_delete);                              /* 释放节点 */
            return;
        }
        current = &((*current)->next);                    /* 移动到下一个节点 */
    }
}

/**
 * @brief  发布消息：同步调用所有订阅者。
 * @param  publisher 发布者
 * @param  message   消息（按值传递，结构体 260 字节，栈开销较大）
 * @retval None
 *
 * 注意：
 *   - 回调在调用者上下文同步执行，任何一个回调阻塞都会拖住整个发布；
 *   - 遍历期间如果回调里 subscribe/unsubscribe，链表可能被改坏；
 *   - 固件里没有使用，只有模拟器按键分发使用，单线程下安全。
 */
// Publish a message to all subscribers
void Publisher_publish(PubSub_Publisher_t *publisher, PubSub_Message_t message) {
    SubscriberNode *node = publisher->subscribers;
    while (node != NULL) {
        node->subscriber(message);  /* 同步回调：发布者不关心订阅者做什么 */
        node = node->next;
    }
}
