/**
 * @file    user_MessageSendTask.h
 * @brief   消息发送任务：把中断/其他任务产生的事件转发到对应消息队列。
 */
#ifndef __USER_MESSAGESENDTASK_H__
#define __USER_MESSAGESENDTASK_H__

#ifdef __cplusplus
extern "C" {
#endif

/** 消息分发任务：统一处理中断事件标志并投递到业务队列 */
void MessageSendTask(void *argument);


#ifdef __cplusplus
}
#endif

#endif
