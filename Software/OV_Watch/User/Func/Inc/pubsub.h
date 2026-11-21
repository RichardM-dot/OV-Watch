/**
 * @file    pubsub.h
 * @brief   轻量级发布/订阅（观察者）机制。
 *
 * 发布者维护一条订阅者链表，Publisher_publish 时依次回调所有订阅者。
 * 本工程用它把按键事件（SDL_KeyBoard_Publisher）分发给界面/业务模块，
 * 实现按键产生者与处理者解耦。
 */
#ifndef PUBSUB_H
#define PUBSUB_H

#include <stdint.h>

// Message structure
/** 发布的消息：id 为消息类型，data 为附带数据（最长 255 字节 + 结束符） */
typedef struct {
    int id;				// 消息 ID（各模块自行约定含义）
    char data[256];		// 消息数据缓冲区
} PubSub_Message_t;

// Subscriber function pointer type
/** 订阅者回调：收到消息时被调用 */
typedef void (*Subscriber)(PubSub_Message_t);

// SubscriberNode structure
/** 订阅者链表节点 */
typedef struct SubscriberNode {
    Subscriber subscriber;			// 订阅者回调函数
    struct SubscriberNode *next;	// 下一个订阅者
} SubscriberNode;

// Publisher structure
/** 发布者：持有订阅者链表头 */
typedef struct {
    SubscriberNode *subscribers;
} PubSub_Publisher_t;

// Function declarations
/** 初始化发布者（链表置空） */
void Publisher_init(PubSub_Publisher_t *publisher);
/** 注册订阅者（加入链表尾部） */
void Publisher_subscribe(PubSub_Publisher_t *publisher, Subscriber subscriber);
/** 注销订阅者（从链表中移除） */
void Publisher_unsubscribe(PubSub_Publisher_t *publisher, Subscriber subscriber);
/** 发布消息：依次调用所有订阅者回调 */
void Publisher_publish(PubSub_Publisher_t *publisher, PubSub_Message_t message);

extern PubSub_Publisher_t SDL_KeyBoard_Publisher;	// 全局按键事件发布者

#endif // PUBSUB_H
