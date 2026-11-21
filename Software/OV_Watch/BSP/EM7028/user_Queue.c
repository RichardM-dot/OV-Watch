/*
 * ============================================================================
 * 【中文导读】user_Queue.c —— 定长环形队列（B级导读）
 * ----------------------------------------------------------------------------
 * 心率算法的滑动窗口容器：容量固定 QUEUE_SIZE=7，元素是 unsigned long
 * （32 位），用 front/rear/size 三个成员维护。
 *
 *   initQueue  : front=0, rear=-1, size=0
 *   enqueue    : rear=(rear+1)%N 写入，size++
 *   dequeue    : 取 data[front]，front=(front+1)%N，size--
 *
 * 用 size 而不是“front==rear”来判空/满，好处是不用牺牲一个槽位；
 * rear 初值取 -1，是因为 enqueue 里先 ++ 再取模，第一次正好落到 0。
 *
 * 注意：
 *   - **非线程安全**：HR_Calculate 的调用处用 vTaskSuspendAll() 保护，
 *     中断里不要直接调用；
 *   - 队满时 enqueue 只打印警告并**丢弃新数据**（不是覆盖最旧），
 *     调用方（HR_Calculate）自己先 dequeue 腾位置；
 *   - dequeue 空队返回 0，和“元素真的是 0”无法区分；
 *   - 用了 printf，嵌入式里可能阻塞/占用串口，正式产品应改成错误码。
 * ============================================================================
 */

#include "user_Queue.h"


// 初始化队列
/**
  * @brief  初始化队列为空
  * @param  queue 队列指针
  * @retval 无
  * @note   rear 取 -1 是关键：enqueue 先 ++ 再取模，第一次写入正好是 0。
  */
void initQueue(Queue *queue) 
{
    queue->front = 0;
    queue->rear = -1;
    queue->size = 0;
}

// 判断队列是否为空
/**
  * @brief  判断队列是否为空
  * @param  queue 队列指针
  * @retval true=空
  */
bool isQueueEmpty(Queue *queue) 
{
    return queue->size == 0;
}

// 判断队列是否已满
/**
  * @brief  判断队列是否已满
  * @param  queue 队列指针
  * @retval true=已满（size == QUEUE_SIZE）
  */
bool isQueueFull(Queue *queue) 
{
    return queue->size == QUEUE_SIZE;
}

// 入队
/**
  * @brief  入队（队满则丢弃新元素并打印警告）
  * @param  queue 队列指针
  * @param  item  要入队的值（unsigned long）
  * @retval 无
  * @note   队满时**不覆盖最旧数据**，而是直接 return，新数据丢失。
  *         HR_Calculate 因此在入队前先 dequeue 保证有空位。
  */
void enqueue(Queue *queue, unsigned long item) 
{
    if (isQueueFull(queue)) 
    {
        printf("队列已满，无法入队！\n");
        return;
    }
    queue->rear = (queue->rear + 1) % QUEUE_SIZE;
    queue->data[queue->rear] = item;
    queue->size++;
}

// 出队
/**
  * @brief  出队（队空返回 0 并打印警告）
  * @param  queue 队列指针
  * @retval 队首元素；队空时返回 0（与真实值 0 无法区分）
  */
uint32_t dequeue(Queue *queue) 
{
    if (isQueueEmpty(queue)) 
    {
        printf("队列为空，无法出队！\n");
        return 0;
    }
    unsigned long item = queue->data[queue->front];
    queue->front = (queue->front + 1) % QUEUE_SIZE;
    queue->size--;
    return item;
}

