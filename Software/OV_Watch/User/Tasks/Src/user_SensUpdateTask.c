/*
 * 【中文导读】user_SensUpdateTask.c —— 传感器轮询任务
 * 周期读 MPU6050 计步/抬腕手势、AHT21 温湿度、EM7028 心率、SPL06 气压，
 * 数据经 pubsub 发布给关心它们的任务（解耦：采集与显示分离）。
 */

/* Private includes -----------------------------------------------------------*/
//includes
#include "user_TasksInit.h"
#include "user_ScrRenewTask.h"
#include "user_SensUpdateTask.h"
#include "ui_HomePage.h"
#include "ui_MenuPage.h"
#include "ui_SetPage.h"
#include "ui_HRPage.h"
#include "ui_SPO2Page.h"
#include "ui_ENVPage.h"
#include "ui_CompassPage.h"
#include "main.h"

#include "AHT21.h"
#include "LSM303.h"
#include "SPL06_001.h"
#include "em70x8.h"
#include "HrAlgorythm.h"

#include "HWDataAccess.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
uint32_t user_HR_timecount=0;

/* Private function prototypes -----------------------------------------------*/
// 这是EM7028官方lib的库函数, 没有lib（库文件）用不了
// 传感器对外接口，不需要记忆
extern uint8_t GET_BP_MAX (void);
extern uint8_t GET_BP_MIN (void);
extern void Blood_Process (void);
extern void Blood_50ms_process (void);
extern void Blood_500ms_process(void);
extern int em70xx_bpm_dynamic(int RECEIVED_BYTE, int g_sensor_x, int g_sensor_y, int g_sensor_z);
extern int em70xx_reset(int ref);


/**
  * @brief  MPU6050 Check the state
  * @param  argument: Not used
  * @retval None
  */
void MPUCheckTask(void *argument)
{
	while(1)
	{
		if(HWInterface.IMU.wrist_is_enabled) // 如果抬腕up
		{
			if(MPU_isHorizontal()) // 如果设备水平
			{
				HWInterface.IMU.wrist_state = WRIST_UP; // 当前手表抬起
			}
			else // 如果设备不水平
			{
				if(WRIST_UP == HWInterface.IMU.wrist_state) // 之前的状态是抬腕up
				{
					HWInterface.IMU.wrist_state = WRIST_DOWN; // 将状态改为down
					if( Page_Get_NowPage()->page_obj == &ui_HomePage ||
						Page_Get_NowPage()->page_obj == &ui_MenuPage ||
						Page_Get_NowPage()->page_obj == &ui_SetPage ) // 如果当前在特定界面
					{
						uint8_t Stopstr;
						osMessageQueuePut(Stop_MessageQueue, &Stopstr, 0, 1);//sleep  发停止/休眠消息
					}
				}
				HWInterface.IMU.wrist_state = WRIST_DOWN; // 在外部，只要不水平就赋值，兜底作用
			}
		}
		osDelay(300);
	}
}

/**
  * @brief  HR data renew task
  * @param  argument: Not used
  * @retval None
  */
void HRDataUpdateTask(void *argument) //更新心率数据函数
{
	uint8_t IdleBreakstr=0;
	uint16_t dat=0;
	uint8_t hr_temp=0;
	while(1)
	{
		if(Page_Get_NowPage()->page_obj == &ui_HRPage) // 如果当前页面是心率的ui界面
		{
			osMessageQueuePut(IdleBreak_MessageQueue, &IdleBreakstr, 0, 1); // 唤醒心率传感器
			//sensor wake up
			EM7028_hrs_Enable(); // 初始化心率传感器
			//receive the sensor wakeup message, sensor wakeup
			if(!HWInterface.HR_meter.ConnectionError) // 如果当前心率/血氧传感器的连接/初始化状态为0（正常）
			{
				//Hr messure
				vTaskSuspendAll(); // 挂起调度器,作用：保证 HR_Calculate(EM7028_Get_HRS1(), user_HR_timecount) 执行期间不会发生任务切换，避免计算过程被其他任务打断导致数据不一致。
				hr_temp = HR_Calculate(EM7028_Get_HRS1(),user_HR_timecount); // hr_temp重新赋值
				xTaskResumeAll(); // 恢复调度器，并处理挂起期间积压的事件（被中断唤醒的任务、错过的 tick），必要时触发一次任务切换。
				if(HWInterface.HR_meter.HrRate != hr_temp && hr_temp>50 && hr_temp<120) // 心率判断
				{
					HWInterface.HR_meter.HrRate = hr_temp;
				}
			}
		}
		osDelay(50);
	}
}


/**
  * @brief  Sensor data update task
  * @param  argument: Not used
  * @retval None
  */
void SensorDataUpdateTask(void *argument)
{
	uint8_t value_strbuf[6]; // 没用
	uint8_t IdleBreakstr=0;
	while(1)
	{
		// Update the sens data showed in Home
		uint8_t HomeUpdataStr;
		if(osMessageQueueGet(HomeUpdata_MessageQueue, &HomeUpdataStr, NULL, 0)==osOK) // 如果函数完成没有发生错误事件
		{
			//bat battery电池
			uint8_t value_strbuf[5]; // 没用

			HWInterface.Power.power_remain = HWInterface.Power.BatCalculate();
			if(HWInterface.Power.power_remain>0 && HWInterface.Power.power_remain<=100)
			{}
			else
			{HWInterface.Power.power_remain = 0;} // 将异常电量设置成0，来提醒用户

			//steps 步数
			if(!(HWInterface.IMU.ConnectionError)) // 如果imu连接正常
			{
				HWInterface.IMU.Steps = HWInterface.IMU.GetSteps();
			}

			//temp and humi
			if(!(HWInterface.AHT21.ConnectionError))
			{
				//temp and humi messure
				float humi,temp;
				HWInterface.AHT21.GetHumiTemp(&humi,&temp); // 浮点数存放温湿度
				//check
				if(temp>-10 && temp<50 && humi>0 && humi<100)
				{
					// 注释并改进下方代码，使得浮点数精度得以保存
					// ui_EnvTempValue = (int8_t)temp;
					// ui_EnvHumiValue = (int8_t)humi;
					HWInterface.AHT21.humidity = humi;
					HWInterface.AHT21.temperature = temp;
				}
			}

			//send data save message queue
			uint8_t Datastr = 3;
			osMessageQueuePut(DataSave_MessageQueue, &Datastr, 0, 1);

		}


		// SPO2 Page 血样饱和度界面
		if(Page_Get_NowPage()->page_obj == &ui_SPO2Page)
		{
			osMessageQueuePut(IdleBreak_MessageQueue, &IdleBreakstr, 0, 1);
			//sensor wake up

			//receive the sensor wakeup message, sensor wakeup
			if(0)
			{
				//SPO2 messure
			}
		}

		// Env Page 环境界面，显示温湿度
		else if(Page_Get_NowPage()->page_obj == &ui_EnvPage)
		{
			osMessageQueuePut(IdleBreak_MessageQueue, &IdleBreakstr, 0, 1);
			//receive the sensor wakeup message, sensor wakeup
			if(!HWInterface.AHT21.ConnectionError)
			{
				//temp and humi messure
				float humi,temp;
				HWInterface.AHT21.GetHumiTemp(&humi,&temp);
				//check
				if(temp>-10 && temp<50 && humi>0 && humi<100)
				{
					HWInterface.AHT21.temperature = (int8_t)temp;
					HWInterface.AHT21.humidity = (int8_t)humi;
				}
			}

		}

		// Compass page LSM303DLH：一颗 加速度计 + 磁力计 传感器；
		else if(Page_Get_NowPage()->page_obj == &ui_CompassPage)
		{
			osMessageQueuePut(IdleBreak_MessageQueue, &IdleBreakstr, 0, 1);
			//receive the sensor wakeup message, sensor wakeup
			LSM303DLH_Wakeup();
			//SPL_Wakeup();
			//if the sensor is no problem
			if(!HWInterface.Ecompass.ConnectionError)
			{
				//messure
				int16_t Xa,Ya,Za,Xm,Ym,Zm;
				LSM303_ReadAcceleration(&Xa,&Ya,&Za);
				LSM303_ReadMagnetic(&Xm,&Ym,&Zm);
				float temp = Azimuth_Calculate(Xa,Ya,Za,Xm,Ym,Zm)+0;//0 offset
				if(temp<0)
				{temp+=360;}
				//check
				if(temp>=0 && temp<=360)
				{
					HWInterface.Ecompass.direction = (uint16_t)temp;
				}
			}
			//if the sensor is no problem
			if(!HWInterface.Barometer.ConnectionError)
			{
				//messure
				float alti = Altitude_Calculate();
				//check
				if(1)
				{
					HWInterface.Barometer.altitude = (int16_t)alti;
				}
			}
		}

		osDelay(500);
	}
}
