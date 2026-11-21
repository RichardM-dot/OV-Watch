/*
 * ============================================================================
 * 【中文导读】user_RunModeTasks.c —— 运行模式管理（低功耗核心！）
 * ----------------------------------------------------------------------------
 * 手表三模式：
 *   1) 正常运行：100 MHz，屏亮，任务全跑；
 *   2) 睡眠（STOP）：关屏/背光/触摸/串口，SysTick 停，等 RTC/按键/充电唤醒；
 *   3) 关机：Power_DisEnable() 拉低 PA3，TPS63020 断电。
 *
 * 两个任务：
 *   IdleEnterTask  —— 软件计时：到背光时间变暗，到熄屏时间发 Stop；
 *   StopEnterTask  —— 真正的 STOP 进出：配置唤醒源、恢复时钟/外设、
 *                     判断抬腕/按键/充电，决定继续睡还是回正常模式。
 *
 * 读这个文件重点看：
 *   - STOP 前后 SysTick、系统时钟、看门狗、外设为什么要成对处理；
 *   - goto sleep 的跳转路径（每次都会重新 vTaskSuspendAll）；
 *   - 唤醒后没有重新挂 UART DMA（已知隐患，见第 07 篇文档）。
 * ============================================================================
 */

/* Private includes -----------------------------------------------------------*/
//includes
#include "user_TasksInit.h"

#include "ui.h"
#include "ui_HomePage.h"
#include "ui_OffTimePage.h"
#include "ui_TimerPage.h"

#include "main.h"
#include "stm32f4xx_it.h"
#include "usart.h"
#include "lcd_init.h"
#include "power.h"
#include "CST816.h"
#include "MPU6050.h"
#include "key.h"
#include "WDOG.h"

#include "HWDataAccess.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/* 空闲计时（单位 ms），由 TaskTickHook() 在 SysTick 中断里 +1；
 * 这里只读/清零，32 位读写是原子的（要没没发生，要么完整发生，这个操作无法分割）。 */
uint32_t IdleTimerCount = 0;

/* Private function prototypes -----------------------------------------------*/

/* Tasks ---------------------------------------------------------------------*/

/**
 * @brief  空闲/熄屏任务：把“时间”翻译成“背光变暗/进入 STOP”。
 * @param  argument: Not used
 * @retval None
 *
 * 每 10 ms 跑一次：
 *   - 收到 Idle 消息        → 背光降到 5%，记住已经变暗；
 *   - 收到 IdleBreak 消息   → 清零计时、恢复用户亮度；
 *   - 到 ui_LTimeValue 秒   → 给自己发 Idle 消息（只发一次）；
 *   - 到 ui_TTimeValue 秒   → 给 StopEnterTask 发 Stop 消息。
 */
void IdleEnterTask(void *argument)
{
	uint8_t Idlestr=0;
	uint8_t IdleBreakstr=0; 
	uint8_t light_off_sent = 0;   // 1=已经发过“变暗”命令，避免每 10ms 重复发
	while(1)
	{
		//light get dark
		/* 收到“变暗”消息：把背光降到 5%（不是全关，仍能看时间） */
		if(osMessageQueueGet(Idle_MessageQueue,&Idlestr,NULL,1)==osOK)
		{
			LCD_Set_Light(5);
			light_off_sent = 1;
		}
		//resume light if light got dark and idle state breaked by key pressing or screen touching
		/* 收到“打断空闲”消息：按键/触摸/串口/充电都会发，恢复亮度并重新计时
		 * 发送方 osMessageQueuePut  →  队列里有消息  →  接收方 osMessageQueueGet 返回 osOK*/
		if(osMessageQueueGet(IdleBreak_MessageQueue,&IdleBreakstr,NULL,1)==osOK) // 从 IdleBreak_MessageQueue 取一条消息拷贝到 IdleBreakstr，最多等待 1 个 tick，成功取到返回 osOK。
		{
			IdleTimerCount = 0;
			light_off_sent = 0;
			LCD_Set_Light(ui_LightSliderValue);   // 用户设置的亮度（默认 50）
		}

		/* 到“背光时间”：发 Idle 消息；light_off_sent 保证只发一次 */
		if((!light_off_sent) && (IdleTimerCount >= ((uint32_t)ui_LTimeValue * 1000U))) // 还没发过变暗的消息，并且空闲计时已经达到了用户设置的背光时间
		{
			uint8_t IdleMsg = 0;
			osMessageQueuePut(Idle_MessageQueue, &IdleMsg, 0, 0);
		}

		/* 到“熄屏时间”：发 Stop 消息，StopEnterTask 会进入 STOP 模式。
		 * 同时清零计时/变暗标志，保证唤醒后能重新走一遍流程。 */
		if(IdleTimerCount >= ((uint32_t)ui_TTimeValue * 1000U))
		{
			uint8_t Stopstr = 1;
			IdleTimerCount = 0;
			light_off_sent = 0;
			osMessageQueuePut(Stop_MessageQueue, &Stopstr, 0, 0);
		}

		osDelay(10);   // 10 ms 轮询，足够 1 s 精度，也不会占满 CPU
	}
}

/**
 * @brief  进入 STOP 模式并在唤醒后恢复运行。
 * @param  argument: Not used
 * @retval None
 *
 * 这是全项目最需要小心处理的函数：
 *   - STOP 会停掉 SysTick/PLL；
 *   - 外设（UART/LCD/触摸）在睡前被关掉，醒来必须重新初始化；
 *   - 唤醒后要判断“为什么醒”，无事发生就 goto sleep 继续睡。
 */
void StopEnterTask(void *argument)
{
	uint8_t Stopstr;
	uint8_t HomeUpdataStr;
	uint8_t Wrist_Flag=0;   // 1=本次唤醒是抬腕触发的
	while(1)
	{
		/* 阻塞式等 Stop 消息；正常运行时每 100ms 才醒一次，几乎不耗 CPU */
		if(osMessageQueueGet(Stop_MessageQueue,&Stopstr,NULL,0)==osOK)
		{

			/*************************** your operations before sleep***************************/
			/* goto 的落脚点：每次“继续睡”都会回到这里，重新执行睡前动作 */
			sleep: 
			IdleTimerCount = 0;

			//sensors
			/* 注意：这里没有调用 MPU_Sleep()。MPU6050 的 DMP 仍在工作，
			 * 是待机电流偏高的一个可能来源（见第 07 篇）。 */

			//usart
			/* 关 USART1 的时钟/GPIO/DMA：省电，也避免睡眠时串口漏电。
			 * 醒来后只调了 MspInit，没有重新挂 DMA 接收（已知隐患）。 */
			HAL_UART_MspDeInit(&huart1);

			//lcd
			/* 屏复位拉低 + 背光 0：屏进入复位/睡眠，背光彻底关断 */
			LCD_RES_Clr();
			LCD_Close_Light();
			//touch
			/* CST816 写 SleepMode=0x03；它睡眠后不能靠 I2C 唤醒，必须复位 */
			CST816_Sleep();

			/***********************************************************************************/

			/****************************** enter wakeup operations *****************************/

			/* 挂起调度器：睡眠期间不能有任何任务切换；
			 * 注意它与 xTaskResumeAll() 必须成对，goto sleep 会重新挂起。 */
			vTaskSuspendAll();
			//Disnable Watch Dog
			/* 外部看门狗睡眠期间没人喂，必须先禁用，否则会被复位 */
			WDOG_Disnable();
			//systick int
			/* 关 SysTick 中断；STOP 会停掉 SysTick 时钟，醒来再重配 */
			CLEAR_BIT(SysTick->CTRL, SysTick_CTRL_TICKINT_Msk);
			//enter stop mode
			/* WFI = Wait For Interrupt，是一条 ARM 指令，硬件让 CPU 停止取指、停止执行，功耗立刻降下来。当有被允许唤醒的中断到来，CPU 自动醒来，从 WFI 下一条指令继续执行
			 * WFI：等中断唤醒；主稳压器保持，SRAM/寄存器内容保留 
			 */
			HAL_PWR_EnterSTOPMode(PWR_MAINREGULATOR_ON,PWR_STOPENTRY_WFI); // PWR_STOPENTRY_WFI 就是告诉 HAL：用 WFI 指令进入 STOP 模式。
                                                                           // 另一种是 PWR_STOPENTRY_WFE（Wait For Event），靠事件唤醒，不靠中断。大多数场景用 WFI。

			//here is the sleep period

			/***********************************************************************************/

			/****************************** quit wakeup operations *****************************/

			//resume run mode and reset the sysclk
			/* 先恢复 SysTick 中断，再按 1ms 重配重载值 */
			SET_BIT(SysTick->CTRL, SysTick_CTRL_TICKINT_Msk);
			HAL_SYSTICK_Config(SystemCoreClock / (1000U / uwTickFreq));
			/* STOP 唤醒后时钟回到 HSI 16MHz，必须重新升到 100MHz，
			 * 否则串口波特率/SPI 时序/延时全部错误 */
			SystemClock_Config(); // 重新启动 HSI/LSE/PLL，等 PLL 锁定，切 SYSCLK 到 PLL，设总线分频和 Flash latency。执行完 SystemCoreClock 更新为 100MHz。
			WDOG_Feed();          // 醒来立刻喂狗，因为睡眠期间没喂狗，必须在超时前喂一次，否则会被复位。
			xTaskResumeAll();     // 恢复调度（与上面的 SuspendAll 配对）

			/***********************************************************************************/

			/****************************** your wakeup operations *****************************/

			//MPU Check
			/* 抬腕判断：只有“朝下→水平”才算真正抬腕；放下则继续睡 */
			if(HWInterface.IMU.wrist_is_enabled)
			{
				uint8_t hor;
				hor = MPU_isHorizontal();   // roll/pitch 都在 ±0.5rad 内 → 水平
				if(hor && HWInterface.IMU.wrist_state == WRIST_DOWN)
				{
					HWInterface.IMU.wrist_state = WRIST_UP;
					Wrist_Flag = 1;         // 抬腕：允许真正唤醒
					//resume, go on
				}
				else if(!hor && HWInterface.IMU.wrist_state == WRIST_UP)
				{
					HWInterface.IMU.wrist_state = WRIST_DOWN;
					IdleTimerCount  = 0;
					goto sleep;             // 放下了：继续睡
				}
			}

			/* 真正唤醒条件：KEY1 按下（低）、KEY2 按下（高）、正在充电、或抬腕。
			 * 都不满足就继续睡。 */
			if(!KEY1 || KEY2 || ChargeCheck() || Wrist_Flag)
			{
				Wrist_Flag = 0; // 把抬腕事件消费掉
				//resume, go on
			}
			else
			{
				IdleTimerCount  = 0; // 重置空闲计时；
				goto sleep;
			}

			//usart
			/* 重新初始化 USART1 的时钟/GPIO/DMA。
			 * 注意：这里没有 HAL_UART_Receive_DMA()，睡眠后串口接收可能失效，
			 * 需要补上（见第 07 篇 4.6）。 */
			HAL_UART_MspInit(&huart1);
			//lcd
			/* 屏被复位过，必须完整重新初始化；背光恢复到用户亮度 */
			LCD_Init();
			LCD_Set_Light(ui_LightSliderValue);
			//touch
			CST816_Wakeup();          // 复位唤醒触摸
			//check if is Charging
			/* 如果唤醒原因是充电，补发事件，让 ChargPageEnterTask 弹充电页 */
			if(ChargeCheck())
			{osEventFlagsSet(HardIntEventHandle, HARDINT_EVENT_CHARG);}
			//send the Home Updata message
			/* 通知传感器任务做一轮首页数据刷新 */
			osMessageQueuePut(HomeUpdata_MessageQueue, &HomeUpdataStr, 0, 1);

			/**************************************************************************************/

		}
		osDelay(100);
	}
}
