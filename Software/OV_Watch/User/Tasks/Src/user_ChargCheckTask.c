/*
 * 【中文导读】user_ChargCheckTask.c —— 充电检测任务
 * 轮询充电引脚状态，插拔充电器时发布事件（UI 弹充电页、调整功耗策略）。
 */

/* Private includes -----------------------------------------------------------*/
//includes
#include "user_TasksInit.h"
#include "user_ScrRenewTask.h"
#include "user_RunModeTasks.h"
#include "ui_HomePage.h"
#include "ui_ChargPage.h"
#include "main.h"
#include "HWDataAccess.h"
#include "stm32f4xx_it.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/


/**
  * @brief  charg page enter task
  * @param  argument: Not used
  * @retval None
  */
void ChargPageEnterTask(void *argument)
{
    while(1)
    {
        // 阻塞等待"充电事件"标志。osWaitForever 表示一直等，直到事件到来。
        // 返回值 hardint_flags 里包含本次被置位的事件标志集合。
        uint32_t hardint_flags = osEventFlagsWait(HardIntEventHandle,
                                                  HARDINT_EVENT_CHARG,
                                                  osFlagsWaitAny,
                                                  osWaitForever);

        // 检查本次唤醒是否确实包含"充电事件"标志。
        // 因为 osFlagsWaitAny 可能在多个事件中任意一个到来时返回，
        // 这里再按位与确认是充电事件，避免误处理其他事件。
        if((hardint_flags & HARDINT_EVENT_CHARG) != 0U)
        {
            uint8_t IdleBreakstr = 0;

            // 通知空闲/待机管理任务：有充电相关事件，退出空闲/保持唤醒。
            osMessageQueuePut(IdleBreak_MessageQueue, &IdleBreakstr, NULL, 1);

            // 条件1：ChargeCheck() 为真，表示当前正在充电；
            // 条件2：当前页面不是充电页；
            // 两者同时成立：刚插入充电器，且还没显示充电页 → 加载充电页。
            if((ChargeCheck()) && (Page_Get_NowPage()->page_obj != &ui_ChargPage))
            {
                Page_Load(&Page_Charg);   // 切换到充电页面
            }
            // 条件1：ChargeCheck() 为假，表示已不在充电；
            // 条件2：当前页面是充电页；
            // 两者同时成立：刚拔出充电器，且正停在充电页 → 返回上一页。
            else if((!ChargeCheck()) && (Page_Get_NowPage()->page_obj == &ui_ChargPage))
            {
                Page_Back();              // 从充电页返回上一页
            }
        }
    }
}

