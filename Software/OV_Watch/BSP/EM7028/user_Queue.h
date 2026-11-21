/**
 * @file    user_Queue.h
 * @brief   简易环形队列（固定容量，元素为 uint32_t）。
 *
 * 供心率算法缓存最近的采样数据，FIFO 使用；容量固定为 QUEUE_SIZE。
 * 注意：本文件没有 include guard，重复包含会报重定义错误。
 */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "main.h"

// 定义队列结构
#define QUEUE_SIZE 7	// 队列容量（可存放 7 个元素）

/** 环形队列控制块：front/rear 为读写下标，size 为当前元素个数 */
typedef struct {
    int8_t front;				// 队头下标（出队位置）
    int8_t rear;				// 队尾下标（入队位置）
    int8_t size;				// 当前队列中元素个数
    uint32_t data[QUEUE_SIZE];	// 数据缓冲区
} Queue;

/** 初始化队列（清空下标与计数） */
void initQueue(Queue *queue);
/** 判断队列是否为空 */
bool isQueueEmpty(Queue *queue);
/** 判断队列是否已满 */
bool isQueueFull(Queue *queue);
/** 入队：把 item 放入队尾 */
void enqueue(Queue *queue, unsigned long item);
/** 出队：返回队头元素并从队列移除 */
uint32_t dequeue(Queue *queue);
