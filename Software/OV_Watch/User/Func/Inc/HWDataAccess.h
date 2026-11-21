/**
 *  @addtogroup  Hareware Middle Layer
 *  @brief       Hardware Middle Layer, to access data from BSP and STM32 HAL Library
 *
 *  @{
 *      @file       HWDataAccess.h
 *      @brief      middleware, for UI and APP Layer to get the hardware data
 *      @details    you can enable or disable in .h file.
 * 					        加这个文件就是为了方便UI移植, 比如你要把工程移植到PC仿真,
 *   				        直接把MidFunc中的文件和UI文件都复制过去,
 * 					        然后直接把.h文件中的HW_USE_HARDWARE变成0就行了.
 *
 * 中文说明：
 *   这是 UI/应用层与硬件层之间的"中间层"门面（Facade）。
 *   上层只依赖 HWInterface 这一个全局结构体，通过里面的函数指针访问硬件，
 *   不直接调用 BSP 驱动。这样在 PC 上做 UI 仿真时，把 HW_USE_HARDWARE 改为 0、
 *   换一套桩实现即可，UI 代码一行都不用改。
 */


#ifndef HWDATAACCESS_H
#define HWDATAACCESS_H

#ifdef __cplusplus
extern "C" {
#endif


/***************************
 *  Hardware Define
 ***************************/
/**
 *  if not use, just set 0
 *
 *
 *  if just test ui, no hardware, just set HW_USE_HARDWARE 0
 *
 *  中文：各开关为 1 表示使用真实硬件，为 0 表示使用桩实现（便于 PC 仿真）。
 */

#define HW_USE_HARDWARE 1	// 总开关：1 使用真实硬件，0 全部走仿真桩

#if HW_USE_HARDWARE
  #define HW_USE_RTC    1	// 使用真实 RTC
  #define HW_USE_BLE    1	// 使用 KT6328 蓝牙/充电控制
  #define HW_USE_BAT    1	// 使用电池检测
  #define HW_USE_LCD    1	// 使用 LCD 背光控制
  #define HW_USE_IMU    1	// 使用 MPU6050 六轴传感器
  #define HW_USE_AHT21  1	// 使用 AHT21 温湿度传感器
  #define HW_USE_SPL06  1	// 使用 SPL06-001 气压计
  #define HW_USE_LSM303 1	// 使用 LSM303 电子罗盘
  #define HW_USE_EM7028 1	// 使用 EM7028 心率/血氧传感器
#endif

/***************************
 *  Includes
 ***************************/

#include "stdint.h"
#include <stdbool.h>

#if HW_USE_RTC
  #include "rtc.h"
#endif

#if HW_USE_BLE
  #include "kt6328.h"
#endif

#if HW_USE_BAT
  #include "power.h"
#endif

#if HW_USE_LCD
  #include "lcd_init.h"
#endif

#if HW_USE_IMU
  #include "MPU6050.h"
  #include "inv_mpu.h"
  #include "inv_mpu_dmp_motion_driver.h"
#endif

#if HW_USE_AHT21
  #include "AHT21.h"
#endif

#if HW_USE_SPL06
  #include "SPL06_001.h"
#endif

#if HW_USE_LSM303
  #include "LSM303.h"
#endif

#if HW_USE_EM7028
  #include "em70x8.h"
#endif

/***************************
 *  TypeDefs
 ***************************/
/**
  * @brief  HW RTC DateTime structure definition
  *
  * 中文：RTC 日期时间结构（字段含义沿用 HAL 的 RTC_DateTypeDef/RTC_TimeTypeDef）。
  */
typedef struct
{
    uint8_t WeekDay;    /*!< Specifies the RTC Date WeekDay. 星期（1~7）
                            This parameter can be a value of @ref RTC_WeekDay_Definitions */

    uint8_t Month;      /*!< Specifies the RTC Date Month (in BCD format).
                            This parameter can be a value of @ref RTC_Month_Date_Definitions */

    uint8_t Date;       /*!< Specifies the RTC Date.
                            This parameter must be a number between Min_Data = 1 and Max_Data = 31 */

    uint8_t Year;       /*!< Specifies the RTC Date Year.
                            This parameter must be a number between Min_Data = 0 and Max_Data = 99 */

    uint8_t Hours;      /*!< Specifies the RTC Time Hour.
                            This parameter must be a number between Min_Data = 0 and Max_Data = 12 if the RTC_HourFormat_12 is selected
                            This parameter must be a number between Min_Data = 0 and Max_Data = 23 if the RTC_HourFormat_24 is selected */

    uint8_t Minutes;    /*!< Specifies the RTC Time Minutes.
                            This parameter must be a number between Min_Data = 0 and Max_Data = 59 */

    uint8_t Seconds;    /*!< Specifies the RTC Time Seconds.
                            This parameter must be a number between Min_Data = 0 and Max_Data = 59 */

} HW_DateTimeTypeDef;


/**
  * @brief  HW RTC Interface definition
  *
  * 中文：RTC 操作接口（由 hw_rtc.c 赋值到 HWInterface.RealTimeClock）。
  */
typedef struct
{
    void (*GetTimeDate)(HW_DateTimeTypeDef *nowdatetime);	// 读取当前日期时间
    void (*SetDate)(uint8_t year, uint8_t month, uint8_t date);	// 设置日期
    void (*SetTime)(uint8_t hours, uint8_t minutes, uint8_t seconds);	// 设置时间
    uint8_t (*CalculateWeekday)(uint8_t setyear, uint8_t setmonth, uint8_t setday, uint8_t century);	// 计算星期
} HW_RTC_InterfaceTypeDef;


/**
  * @brief  HW BLE Interface definition
  *
  * 中文：KT6328 蓝牙/充电芯片控制接口。
  */
typedef struct
{
    void (*Init)(void);		// 初始化芯片
    void (*Enable)(void);	// 使能（开启充电/蓝牙）
    void (*Disable)(void);	// 关闭
} HW_BLE_InterfaceTypeDef;


/**
  * @brief  HW Power Interface definition
  *
  * 中文：电源管理接口（电池电量、关机、电量换算）。
  */
typedef struct
{
    uint8_t power_remain;	// 剩余电量百分比（0~100）

    void (*Init)(void);		// 初始化电源模块
    void (*Shutdown)(void);	// 关机（切断电源保持）
    uint8_t (*BatCalculate)(void);	// 重新计算电量百分比
} HW_Power_InterfaceTypeDef;


/**
  * @brief  HW LCD Interface definition
  *
  * 中文：LCD 背光控制接口。
  */
typedef struct
{
    void (*SetLight)(uint8_t dc);	// 设置背光亮度（PWM 占空比 0~255）
} HW_LCD_InterfaceTypeDef;


/**
  * @brief  HW IMU wrist state defines
  *
  * 中文：抬腕状态取值。
  */
#define WRIST_UP 1		// 手腕抬起
#define WRIST_DOWN 0	// 手腕放下

/**
  * @brief  HW IMU Interface definition
  */
typedef struct // 硬件接口定义结构体，用来存放MU 的运行时状态数据（几个变量）和 操作 IMU 的函数指针（Init、WristEnable、GetSteps 等）
{
  // 数据状态
    uint8_t ConnectionError;       // IMU 连接/初始化状态：0 表示正常，非 0 表示异常
    uint16_t Steps;                // 最近一次读取到的步数
    uint8_t wrist_state;           // 当前手腕状态：WRIST_UP（抬起）或 WRIST_DOWN（放下）
    uint8_t wrist_is_enabled;      // 抬腕检测开关：1 表示启用，0 表示禁用

  // 指针函数
    uint8_t (*Init)(void);                         // 初始化 IMU；返回 0 表示成功，非 0 表示失败
    void (*WristEnable)(void);                     // 启用抬腕检测
    void (*WristDisable)(void);                    // 禁用抬腕检测
    uint16_t (*GetSteps)(void);                    // 读取 IMU 记录的步数
    int (*SetSteps)(unsigned long count);          // 设置 IMU 步数；返回 0 表示成功
} HW_IMU_InterfaceTypeDef;


/**
  * @brief  HW AHT21 Interface definition
  *
  * 中文：AHT21 温湿度传感器接口。
  */
typedef struct
{
  uint8_t ConnectionError;	// 传感器连接状态：0 正常，非 0 异常
  uint8_t temperature;		// 当前温度（整数，单位 ℃）
  uint8_t humidity;			// 当前相对湿度（整数，单位 %RH）
  uint8_t (*Init)(void);	// 初始化传感器；返回 0 表示成功
  void (*GetHumiTemp)(float *humi, float *temp);	// 读取湿度与温度（浮点）
} HW_AHT21_InterfaceTypeDef;


/**
  * @brief  HW SPL06-001 Barometer Interface definition
  *
  * 中文：SPL06-001 气压计接口。
  */
typedef struct
{
  uint8_t ConnectionError;	// 传感器连接状态：0 正常，非 0 异常
  uint16_t altitude;		// 当前海拔高度（单位 m）
  uint8_t (*Init)(void);	// 初始化传感器；返回 0 表示成功

} HW_Barometer_InterfaceTypeDef;

/**
  * @brief  HW LSM_303 E-compass Interface definition
  *
  * 中文：LSM303 电子罗盘接口。
  */
typedef struct
{
  uint8_t ConnectionError;	// 传感器连接状态：0 正常，非 0 异常
  uint16_t direction;		// 当前方位角（0~359 度）
  uint8_t (*Init)(void);	// 初始化传感器；返回 0 表示成功
  void (*Sleep)(void);		// 让传感器进入休眠

} HW_Ecompass_InterfaceTypeDef;

/**
  * @brief  HW EM7028 heart rate meter Interface definition
  *
  * 中文：EM7028 心率/血氧传感器接口。
  */
typedef struct
{
  uint8_t ConnectionError;       // 心率/血氧传感器连接或初始化状态：0 表示正常，非 0 表示异常
  uint8_t HrRate;                // 当前心率，单位为次/分钟
  uint8_t SPO2;                  // 当前血氧饱和度，单位为百分比
  uint8_t (*Init)(void);         // 初始化 EM7028 心率/血氧传感器；返回 0 表示成功，非 0 表示失败
  void (*Sleep)(void);           // 使心率/血氧传感器进入休眠状态

} HW_HRmeter_InterfaceTypeDef;

/**
  * @brief  Hardware Interface structure definition
  *
  * 中文：硬件总接口。上层通过 HWInterface.xxx 访问各外设，
  *       所有成员在 hw_xxx.c 的初始化函数中完成函数指针绑定。
  */
typedef struct
{
    HW_RTC_InterfaceTypeDef RealTimeClock;	// 实时时钟
    HW_BLE_InterfaceTypeDef BLE;			// 蓝牙/充电芯片
    HW_Power_InterfaceTypeDef Power;		// 电源管理
    HW_LCD_InterfaceTypeDef LCD;			// LCD 背光
    HW_IMU_InterfaceTypeDef IMU;			// 六轴传感器（姿态/计步/抬腕）
    HW_AHT21_InterfaceTypeDef AHT21;		// 温湿度
    HW_Barometer_InterfaceTypeDef Barometer;	// 气压/海拔
    HW_Ecompass_InterfaceTypeDef Ecompass;	// 电子罗盘
    HW_HRmeter_InterfaceTypeDef HR_meter;	// 心率/血氧
} HW_InterfaceTypeDef;



/***************************
 *  PROTOTYPES
 ***************************/


/***************************
 *  External Variables
 ***************************/
extern HW_InterfaceTypeDef HWInterface;	// 全局硬件接口实例（唯一入口）



#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
