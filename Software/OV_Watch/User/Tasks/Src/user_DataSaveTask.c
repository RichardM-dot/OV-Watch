/*
 * 【中文导读】user_DataSaveTask.c —— 数据保存任务
 * 收到存数请求时把用户数据写入 BL24C02 EEPROM（断电保留）。队列驱动，不忙。
 */

/* Private includes -----------------------------------------------------------*/
//includes
#include "user_DataSaveTask.h"
//APP SYS setting
#include "ui_DateTimeSetPage.h"
#include "ui_HomePage.h"

#include "main.h"
#include "rtc.h"
#include "DataSave.h"
#include "inv_mpu_dmp_motion_driver.h"

#include "HWDataAccess.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/******************************************
EEPROM Data description:
[0x00]:0x55 for check
[0x01]:0xAA for check

[0x10]:user wrist setting, HWInterface.IMU.wrist_is_enabled
[0x11]:user ui_APPSy_EN setting

[0x20]:Last Save Day(0-31)
[0x21]:Day Steps

*******************************************/


/* Private function prototypes -----------------------------------------------*/

/* Tasks ---------------------------------------------------------------------*/

/**
  * @brief  Data Save in the EEPROM
  * @param  argument: Not used
  * @retval None
  */
void DataSaveTask(void *argument)
{

	while(1)
	{
		uint8_t Datastr=0;
		if(osMessageQueueGet(DataSave_MessageQueue,&Datastr,NULL,1)==osOK)
		{
			/****************
			Setting change
			date change
			Step change
			****************/
			uint8_t dat[3]; 
			dat[0] = HWInterface.IMU.wrist_is_enabled; // 是否需要抬腕检测
			dat[1] = ui_APPSy_EN; // app时间同步开关是否开启
			SettingSave(dat,0x10,2); // 向EEROM写入数据

			RTC_DateTypeDef nowdate; // 定义nowdate结构体用来保存/设置年月日
			HAL_RTC_GetDate(&hrtc,&nowdate,RTC_FORMAT_BIN); // 从 RTC 读取当前日期，以二进制格式存入 nowdate（年/月/日/星期）

			SettingGet(dat,0x20,3); // 从EEROM读取数据到dat，前面dat存放的内容已经被覆盖了
			if(dat[0] != nowdate.Date) // 判断EEROM中存放的数据！=今天的日期
			{
				if(!HWInterface.IMU.ConnectionError)
					HWInterface.IMU.SetSteps(0);

				dat[0] = nowdate.Date;
				dat[2] = 0;
				dat[1] = 0;
				SettingSave(dat,0x20,3);
			}
			else // 还是同一天
			{
				uint16_t temp = HWInterface.IMU.GetSteps();
				dat[0] = nowdate.Date;
				dat[2] = temp & 0xff;
				dat[1] = temp>>8 & 0xff;
				SettingSave(dat,0x20,3);// 把今天日期 + 当前步数存回 EEPROM
			}

		}
		osDelay(100);
	}
}


