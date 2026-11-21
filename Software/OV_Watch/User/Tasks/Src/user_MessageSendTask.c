/*
 * 【中文导读】user_MessageSendTask.c —— 蓝牙消息发送任务
 * 把要发给手机/对端的数据经 KT6328A 蓝牙串口透传发出（队列驱动）。
 */

/* Private includes -----------------------------------------------------------*/
//includes
#include "string.h"
#include "stdio.h"

#include "main.h"
#include "stm32f4xx_it.h"
#include "rtc.h"

#include "user_TasksInit.h"
#include "user_MessageSendTask.h"

#include "ui.h"
#include "ui_EnvPage.h"
#include "ui_HRPage.h"
#include "ui_SPO2Page.h"
#include "ui_HomePage.h"
#include "ui_DateTimeSetPage.h"

#include "HWDataAccess.h"
#include "version.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
struct
{
	RTC_DateTypeDef nowdate; // 日期
	RTC_TimeTypeDef nowtime; // 时间
	int8_t humi;             // 湿度
	int8_t temp;             // 温度
	uint8_t HR;              // 心率
	uint8_t SPO2;            // 血氧饱和度
	uint16_t stepNum;        // 步数
}BLEMessage;

struct
{
	RTC_DateTypeDef nowdate; // 设置日期
	RTC_TimeTypeDef nowtime; // 设置时间
}TimeSetMessage;

/* Private function prototypes -----------------------------------------------*/
// 两个同级别的函数，结合使用

void StrCMD_Get(uint8_t * str,uint8_t * cmd)
{
	uint8_t i=0;
  while(str[i]!='=') // 没检查 '\0'，容易出现野指针越界
  {
      cmd[i] = str[i]; // 语法糖   p[i]  等价于  *(p + i)
      i++;
  }
}

//set time//OV+ST=20230629125555
uint8_t TimeFormat_Get(uint8_t * str)
{
	TimeSetMessage.nowdate.Year = (str[8]-'0')*10+str[9]-'0';
	TimeSetMessage.nowdate.Month = (str[10]-'0')*10+str[11]-'0';
	TimeSetMessage.nowdate.Date = (str[12]-'0')*10+str[13]-'0';
	TimeSetMessage.nowtime.Hours = (str[14]-'0')*10+str[15]-'0';
	TimeSetMessage.nowtime.Minutes = (str[16]-'0')*10+str[17]-'0';
	TimeSetMessage.nowtime.Seconds = (str[18]-'0')*10+str[19]-'0';
	if(TimeSetMessage.nowdate.Year>0 && TimeSetMessage.nowdate.Year<99
		&& TimeSetMessage.nowdate.Month>0 && TimeSetMessage.nowdate.Month<=12
		&& TimeSetMessage.nowdate.Date>0 && TimeSetMessage.nowdate.Date<=31
		&& TimeSetMessage.nowtime.Hours>=0 && TimeSetMessage.nowtime.Hours<=23
		&& TimeSetMessage.nowtime.Minutes>=0 && TimeSetMessage.nowtime.Minutes<=59
		&& TimeSetMessage.nowtime.Seconds>=0 && TimeSetMessage.nowtime.Seconds<=59)
	{
		RTC_SetDate(TimeSetMessage.nowdate.Year, TimeSetMessage.nowdate.Month,TimeSetMessage.nowdate.Date);
		RTC_SetTime(TimeSetMessage.nowtime.Hours,TimeSetMessage.nowtime.Minutes,TimeSetMessage.nowtime.Seconds);
		printf("TIMESETOK\r\n");
	}

}

/**
  * @brief  send the message via BLE, use uart
  * @param  argument: Not used
  * @retval None
  * 等串口事件 → 看收到什么命令 → 按命令回数据或设时间 → 清缓冲区，等下一轮。
  */
void MessageSendTask(void *argument) // 蓝牙/UART 消息处理任务
{
	while(1)
	{
		uint32_t hardint_flags = osEventFlagsWait(HardIntEventHandle, HARDINT_EVENT_UART, osFlagsWaitAny, osWaitForever); // 阻塞等待 UART 事件

		// 确认是 UART 事件
		if((hardint_flags & HARDINT_EVENT_UART) != 0U) // hardint_flags 是 uint32_t 事件位集合；HARDINT_EVENT_UART 在 user_TasksInit.h 定义为第 0 位。按位与检查该位是否置位，确认收到串口事件后再处理。
		{
			uint8_t IdleBreakstr = 0;
			osMessageQueuePut(IdleBreak_MessageQueue,&IdleBreakstr,NULL,1); // 通知空闲管理任务：有串口活动，保持唤醒
			printf("RecStr:%s\r\n",HardInt_receive_str); // 打印收到的字符串

			// 根据命令内容分支处理
			if(!strcmp(HardInt_receive_str,"OV"))
			{
				printf("OK\r\n");
			}
			else if(!strcmp(HardInt_receive_str,"OV+VERSION"))
			{
				printf("VERSION=V%d.%d.%d\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
			}
			else if(!strcmp(HardInt_receive_str,"OV+SEND"))
			{
				HAL_RTC_GetTime(&hrtc,&(BLEMessage.nowtime),RTC_FORMAT_BIN);
				HAL_RTC_GetDate(&hrtc,&BLEMessage.nowdate,RTC_FORMAT_BIN);
				BLEMessage.humi = HWInterface.AHT21.humidity;
				BLEMessage.temp = HWInterface.AHT21.temperature;
				BLEMessage.HR = HWInterface.HR_meter.HrRate;
				BLEMessage.SPO2 = HWInterface.HR_meter.SPO2;
				BLEMessage.stepNum = HWInterface.IMU.Steps;

				printf("data:%2d-%02d\r\n",BLEMessage.nowdate.Month,BLEMessage.nowdate.Date);
				printf("time:%02d:%02d:%02d\r\n",BLEMessage.nowtime.Hours,BLEMessage.nowtime.Minutes,BLEMessage.nowtime.Seconds);
				printf("humidity:%d%%\r\n",BLEMessage.humi);
				printf("temperature:%d\r\n",BLEMessage.temp);
				printf("Heart Rate:%d%%\r\n",BLEMessage.HR);
				printf("SPO2:%d%%\r\n",BLEMessage.SPO2);
				printf("Step today:%d\r\n",BLEMessage.stepNum);
			}
			//set time//OV+ST=20230629125555
			else if(strlen(HardInt_receive_str)==20)
			{
				uint8_t cmd[10];
				memset(cmd,0,sizeof(cmd));
				StrCMD_Get(HardInt_receive_str,cmd);
				if(ui_APPSy_EN && !strcmp(cmd,"OV+ST"))
				{
					TimeFormat_Get(HardInt_receive_str);
				}
			}
			// 清空接收缓冲区
			memset(HardInt_receive_str,0,sizeof(HardInt_receive_str));
		}
	}
}
/*
 *在 usart.c 定义了 fputc()：printf 输出的每个字符都会通过 HAL_UART_Transmit() 发送到 huart1，
 *也就是 USART1。USART1 配置为 115200 波特率，TX 引脚是 PA9。
*/
