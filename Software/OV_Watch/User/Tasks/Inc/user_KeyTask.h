/**
 * @file    user_KeyTask.h
 * @brief   按键处理任务：扫描/接收按键事件并分发给界面。
 */
#ifndef __USER_KEYTASK_H__
#define __USER_KEYTASK_H__

#ifdef __cplusplus
extern "C" {
#endif

/** 按键任务：从 Key_MessageQueue 取事件，做短按/长按判定后发布给订阅者 */
void KeyTask(void *argument);


#ifdef __cplusplus
}
#endif

#endif
