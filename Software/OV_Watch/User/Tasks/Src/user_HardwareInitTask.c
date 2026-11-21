/*
 * 【中文导读】user_HardwareInitTask.c —— 硬件初始化任务（一次性）
 * 依次点亮：I2C 总线 → 各传感器(MPU6050/AHT21/EM7028/SPL06) → EEPROM →
 * 蓝牙 → 电源管理。初始化完成后 vTaskDelete 自己——"引导任务"惯用手法。
 */

/* Private includes -----------------------------------------------------------*/

// includes
// sys
#include "usart.h"
#include "tim.h"
#include "stm32f4xx_it.h"
#include "delay.h"

// user
#include "user_TasksInit.h"
#include "HWDataAccess.h"
#include "version.h"

// bsp
#include "key.h"
#include "lcd.h"
#include "lcd_init.h"
#include "CST816.h"
#include "DataSave.h"

// ui
//gui
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "ui.h"

// APP SYS setting
#include "ui_DateTimeSetPage.h" // 此头文件是为了获取 ui_APPSy_EN 变量，判断是否启用 APP 系统设置功能。

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/


/**
  * @brief  hardwares init task
  * @param  argument: Not used
  * @retval None
  */
void HardwareInitTask(void *argument)
{
	while(1)
	{
    vTaskSuspendAll();

    // RTC Wake
    if(HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 2000, RTC_WAKEUPCLOCK_RTCCLK_DIV16) != HAL_OK) // 配置并启动 RTC 的唤醒定时器，使其在计数达到预设值后产生中断
    {
      Error_Handler();
    }

    // usart start
    HAL_UART_Receive_DMA(&huart1,(uint8_t*)HardInt_receive_str,25);
    __HAL_UART_ENABLE_IT(&huart1,UART_IT_IDLE);

    // PWM Start
    HAL_TIM_PWM_Start(&htim3,TIM_CHANNEL_3);

    // sys delay
    delay_init();
    // wait
    // delay_ms(1000);

    // power
    HWInterface.Power.Init();

    // key
    Key_Port_Init();

    // sensors
    uint8_t num = 3;
    while(num && HWInterface.AHT21.ConnectionError)
    {
      num--;
      HWInterface.AHT21.ConnectionError = HWInterface.AHT21.Init();
    }

    num = 3;
    while(num && HWInterface.Ecompass.ConnectionError)
    {
      num--;
      HWInterface.Ecompass.ConnectionError = HWInterface.Ecompass.Init();
    }
    if(!HWInterface.Ecompass.ConnectionError)
      HWInterface.Ecompass.Sleep();

    num = 3;
    while(num && HWInterface.Barometer.ConnectionError)
    {
      num--;
      HWInterface.Barometer.ConnectionError = HWInterface.Barometer.Init();
    }

    num = 3;
    while(num && HWInterface.IMU.ConnectionError)
    {
      num--;
      HWInterface.IMU.ConnectionError = HWInterface.IMU.Init();
      // Sensor_MPU_Erro = MPU_Init();
    }

    num = 3;
    while(num && HWInterface.HR_meter.ConnectionError)
    {
      num--;
      HWInterface.HR_meter.ConnectionError = HWInterface.HR_meter.Init();
    }
    if(!HWInterface.HR_meter.ConnectionError)
      HWInterface.HR_meter.Sleep();


    // EEPROM Electrically Erasable Programmable Read-Only Memory，电可擦除可编程只读存储器
    EEPROM_Init(); // 初始化 EEPROM，准备进行数据读写操作，函数定义在 DataSave.c 中。
    if(!EEPROM_Check()) // 检查 EEPROM 是否正常工作
    {
      uint8_t recbuf[3];
      SettingGet(recbuf,0x10,2);
      if((recbuf[0]!=0 && recbuf[0]!=1) || (recbuf[1]!=0 && recbuf[1]!=1))
      {
        HWInterface.IMU.wrist_is_enabled = 0;
        ui_APPSy_EN = 0;
      }
      else
      {
        HWInterface.IMU.wrist_is_enabled = recbuf[0];
        ui_APPSy_EN = recbuf[1];
      }

      RTC_DateTypeDef nowdate;
      HAL_RTC_GetDate(&hrtc,&nowdate,RTC_FORMAT_BIN);

      SettingGet(recbuf,0x20,3);
      if(recbuf[0] == nowdate.Date)
      {
        uint16_t steps=0;
        steps = recbuf[1]&0x00ff;
        steps = steps<<8 | recbuf[2];
        if(!HWInterface.IMU.ConnectionError)
          dmp_set_pedometer_step_count((unsigned long)steps);
      }
    }


    // BLE
    HWInterface.BLE.Init();
    HWInterface.BLE.Disable();

    //set the KT6328 BautRate 9600
    //default is 115200
    //printf("AT+CT01\r\n");

    // touch 电容触摸控制器芯片
    CST816_GPIO_Init();
    CST816_RESET();

    // lcd
    LCD_Init(); // LCD 初始化
    LCD_Fill(0,0, LCD_W, LCD_H, BLACK); // 清屏，填充为黑色
    delay_ms(10); // 等待 LCD 初始化完成
    LCD_Set_Light(50); // 设置 LCD 背光亮度为 50（范围通常是 0-100）
    LCD_ShowString(72,LCD_H/2,(uint8_t*)"Welcome!", WHITE, BLACK, 24, 0);//12*6,16*8,24*12,32*16
    uint8_t lcd_buf_str[17]; // 缓冲区，用于存储要显示的字符串
    sprintf(lcd_buf_str, "OV-Watch V%d.%d.%d", watch_version_major(), watch_version_minor(), watch_version_patch()); // 格式化字符串，显示版本号
    LCD_ShowString(34, LCD_H/2+48, (uint8_t*)lcd_buf_str, WHITE, BLACK, 24, 0); // 显示版本号字符串
    delay_ms(1000); // 等待 1 秒钟，让用户看到欢迎信息和版本号
    LCD_Fill(0, LCD_H/2-24, LCD_W, LCD_H/2+49, BLACK); // 清除屏幕中间部分，为后续显示做准备


    // ui
    // LVGL init
    lv_init(); // 初始化 LVGL 图形库，准备创建图形界面
    lv_port_disp_init(); // 初始化 LVGL 显示端口，设置显示缓冲区、分辨率等参数
    lv_port_indev_init(); // 初始化 LVGL 输入设备端口，设置触摸屏或按键输入等参数
    ui_init(); // 初始化用户界面，创建界面元素和布局

    xTaskResumeAll(); // 恢复任务调度，允许其他任务继续运行
		vTaskDelete(NULL); // 删除当前任务，即硬件初始化任务已经完成，不再需要继续运行
		osDelay(500); // 延迟 500 毫秒，确保任务删除前有足够的时间完成清理工作
	}
}


