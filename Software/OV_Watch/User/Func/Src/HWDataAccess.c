/*
 * 【中文导读】HWDataAccess.c —— 硬件数据访问层（BSP 与业务的桥梁）
 * 封装"读心率/读温度/读电量…"等高层接口，页面代码只调这些函数，
 * 不直接碰传感器驱动——换传感器不改 UI。
 */

/*
 * 【深读】HWDataAccess.c —— 硬件门面（Facade）的 23 个适配函数 + 1 张函数指针表
 * ----------------------------------------------------------------------------
 * 这一层不做任何计算，只做三件事：
 *   1) 把 BSP 的 C 函数包装成函数指针，挂到全局 HWInterface 上；
 *   2) 用 #if HW_USE_xxx 做编译期裁剪，方便把 UI 移植到 PC 仿真；
 *   3) 隔离头文件依赖——上层只 include HWDataAccess.h，不碰 stm32xxxx_hal.h。
 * 
 * 数据流：BSP 采样 → SensUpdateTask/HRDataUpdateTask 写 HWInterface 字段 →
 *         GUI 各页定时器读字段 → lv_label_set_text / lv_arc_set_value。
 * 本文件只负责“函数指针绑定”，字段的周期更新在 Tasks 层（见 KB Tasks_SensRunMode）。
 * 
 * 并发约定：所有字段都是 8/16 位标量，Cortex-M4 上单次读写原子；
 * 但没有锁，也没有“多字段一致性”保证（例如时间的小时与分钟可能跨秒撕裂，
 * 各页面通过“一次 GetTimeDate 取全 7 字段”规避）。
 * 
 * 23 个适配函数按外设分组，函数体基本只有一行；重点看每组的 #if 与返回值约定：
 *   Init 类返回 0 成功、非 0 失败；Sleep/Enable 类无返回值；
 *   Get 类在传感器未就绪时返回 0 或直接跳过（不报错）。
 */
/**
 *  @addtogroup  Hareware Middle Layer
 *  @brief       Hardware Middle Layer, to access data from BSP and STM32 HAL Library
 *
 *  @{
 *      @file       HWDataAccess.c
 *      @brief      middleware, for UI and APP Layer to get the hardware data
 *      @details    you can enable or disable in .h file.
 * 					加这个文件就是为了方便UI移植, 比如你要把工程移植到PC仿真,
 *   				直接把MidFunc中的文件和UI文件都复制过去,
 * 					然后直接把.h文件中的HW_USE_HARDWARE变成0就行了.
 */

// 只依赖门面头文件；BSP 头文件由 HWDataAccess.h 按开关条件包含
#include "../Inc/HWDataAccess.h"


/***************************
 *  RTC Fucntions
 ***************************/

/**************************************************************************/
/*!
    @brief  to get the real time clock from the hardware

    @param  nowdatetime to storge the data&time

	@return None
*/
/**************************************************************************/
/**
 * @brief 读取 RTC 当前日期时间
 * @param nowdatetime 输出结构，调用方负责分配（通常在栈上）
 * @return 无
 * @note HAL 要求先 GetTime 再 GetDate：读日期会把时间影子寄存器解锁，顺序反了会读到旧值。WeekDay 不由 RTC 提供，而是用蔡勒公式现算。
 */
void HW_RTC_Get_TimeDate(HW_DateTimeTypeDef * nowdatetime)
{

	#if HW_USE_RTC
		if (nowdatetime != NULL)
		{
            RTC_DateTypeDef nowdate;
            RTC_TimeTypeDef nowtime;
			HAL_RTC_GetTime(&hrtc, &nowtime, RTC_FORMAT_BIN);
// 先时间后日期是 STM32 RTC 的读时序要求
			HAL_RTC_GetDate(&hrtc, &nowdate, RTC_FORMAT_BIN);
            nowdatetime->Year = nowdate.Year;
            nowdatetime->Month = nowdate.Month;
            nowdatetime->Date = nowdate.Date;
// 星期不存硬件，按年月日现算；世纪固定 20
			nowdatetime->WeekDay = weekday_calculate(nowdatetime->Year, nowdatetime->Month, nowdatetime->Date, 20);
            nowdatetime->Hours = nowtime.Hours;
            nowdatetime->Minutes = nowtime.Minutes;
            nowdatetime->Seconds = nowtime.Seconds;
		}
	#else
// HW_USE_RTC=0 时走桩：固定 2024-06-23 11:59:55，方便 PC 上调试 UI
		nowdatetime->Year = 24;
		nowdatetime->Month = 6;
		nowdatetime->Date = 23;
		nowdatetime->WeekDay = 7;
		nowdatetime->Hours = 11;
		nowdatetime->Minutes = 59;
		nowdatetime->Seconds = 55;
	#endif
}


/**************************************************************************/
/*!
    @brief  to set the RTC Date to hardware

    @param  nowdatetime to storge the data&time

	@return None
*/
/**************************************************************************/
/**
 * @brief 设置 RTC 日期
 * @param year  两位年份（0~99）
 * @param month 1~12
 * @param date  1~31
 * @return 无
 * @note 只写日期不写时间；RTC_SetDate 内部会处理 BCD 转换与备份域解锁。
 */
void HW_RTC_Set_Date(uint8_t year, uint8_t month, uint8_t date)
{
	#if HW_USE_RTC
		RTC_SetDate(year, month, date);
	#endif
}

/**************************************************************************/
/*!
    @brief  to set the RTC Date to hardware

    @param  nowdatetime to storge the data&time

	@return None
*/
/**************************************************************************/
/**
 * @brief 设置 RTC 时间
 * @param hours 0~23
 * @param minutes 0~59
 * @param seconds 0~59
 * @return 无
 * @note 写时间会顺带清掉亚秒计数，下一次秒中断从 0 开始。
 */
void HW_RTC_Set_Time(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
	#if HW_USE_RTC
		RTC_SetTime(hours, minutes, seconds);
	#endif
}

/**************************************************************************/
/*!
    @brief  to calculate the weekday

    @param  nowdatetime to storge the data&time

	@return None
*/
/**************************************************************************/
/**
 * @brief 蔡勒公式计算星期
 * @param setyear  两位年份
 * @param setmonth 1~12
 * @param setday 1~31
 * @param century 世纪，固定传 20
 * @return 1=周一 … 7=周日
 * @note 1/2 月按上一年的 13/14 月处理，所以先做 setyear-- / setmonth+=12。结果 0 映射成 7，正好对上 ui_Days[] 的下标 WeekDay-1。
 */
uint8_t HW_weekday_calculate(uint8_t setyear, uint8_t setmonth, uint8_t setday, uint8_t century)
{
	int w;
// 整数运算，无浮点；26*(m+1)/10 是 Zeller 的月份修正项
	if (setmonth == 1 || setmonth == 2)
	{setyear--, setmonth += 12;}
// w 可能为负（公元早期），先加 7 直到非负
	w = setyear + setyear / 4 + century / 4  + 26*(setmonth + 1)/10 + setday - 1 - 2 * century;
	while(w<0)
		w+=7;
	w%=7;
	w=(w==0)?7:w;
	return w;
}



/***************************
 *  Power Fucntions
 ***************************/
/**************************************************************************/
/*!
    @brief initialize the power

    @param	NULL

	@return NULL
*/
/**************************************************************************/
/**
 * @brief 电源模块初始化（ADC + 分压使能）
 * @return 无
 * @note HW_USE_BAT=0 时整个函数体被预处理器删掉，变成空函数。
 */
void HW_Power_Init(void)
{
	#if HW_USE_BAT
		Power_Init();
	#endif
}

/**************************************************************************/
/*!
    @brief shutdown the power

    @param	NULL

	@return NULL
*/
/**************************************************************************/
/**
 * @brief 关机：拉低电源保持信号，硬件断电
 * @return 无
 * @note 调用后不会返回——电池供电被切断，直到再次插充电器/按键唤醒。
 */
void HW_Power_Shutdown(void)
{
	#if HW_USE_BAT
		Power_DisEnable();
	#endif
}

/**************************************************************************/
/*!
    @brief	calculate the battery power remain

    @param	NULL

	@return bat power remain
*/
/**************************************************************************/
/**
 * @brief 重新采样并换算电池电量百分比
 * @return 0~100；传感器异常时返回 0
 * @note 每次调用都会真的做一次 ADC 采样（PowerCalculate 内部有滤波），所以不要在高频循环里调用。
 */
uint8_t HW_Power_BatCalculate(void)
{
	#if HW_USE_BAT
		return PowerCalculate();
	#endif
		return 0;
}


/***************************
 *  BLE Fucntions
 ***************************/

/**************************************************************************/
/*!
	@brief  initialize the BLE

	@param	NULL

	@return NULL
*/
/**
 * @brief KT6328（蓝牙/充电管理）GPIO 初始化
 * @return 无
 */
void HW_BLE_Init(void)
{
	#if HW_USE_BLE
		KT6328_GPIO_Init();
	#endif
}

/**************************************************************************/
/*!
    @brief  enable BLE

    @param	NULL

	@return NULL
*/
/**************************************************************************/
/**
 * @brief 打开 KT6328（蓝牙广播 + 充电使能）
 * @return 无
 * @note 首页下拉面板的 BLE 开关直接调这个函数。
 */
void HW_BLE_Enable(void)
{
	#if HW_USE_BLE
		KT6328_Enable();
	#endif
}


/**************************************************************************/
/*!
    @brief  disable BLE

    @param	NULL

	@return NULL
*/
/**************************************************************************/
/**
 * @brief 关闭 KT6328
 * @return 无
 */
void HW_BLE_Disable(void)
{
	#if HW_USE_BLE
		KT6328_Disable();
	#endif
}



/***************************
 *  LCD Fucntions
 ***************************/

/**************************************************************************/
/*!
    @brief set the lcd light

    @param	dc the LCD light

	@return NULL
*/
/**************************************************************************/
/**
 * @brief 设置 LCD 背光亮度
 * @param dc PWM 占空比 0~255
 * @return 无
 * @note 最终写到 TIM3_CH3 的比较寄存器；0 是灭屏，255 最亮。
 */
void HW_LCD_Set_Light(uint8_t dc)
{
	#if HW_USE_LCD
		LCD_Set_Light(dc);
	#endif
}


/***************************
 *  IMU Fucntions
 ***************************/

/**************************************************************************/
/*!
    @brief  initialize the MPU6050

    @param	NULL

	@return 0 if successful
*/
/**************************************************************************/
/**
 * @brief 初始化 MPU6050 并加载 DMP 固件
 * @return 0 成功 / 非 0 失败（-1 表示未编译进 IMU 支持）
 * @note mpu_dmp_init 会下载约 3 KB 的 DMP 微码并自检，耗时几十 ms，只能在 HardwareInitTask 里调一次。
 */
uint8_t HW_MPU_Init(void)
{
	#if HW_USE_IMU
		return mpu_dmp_init();
	#endif
	return -1;
}


/**************************************************************************/
/*!
    @brief  set the MPU Wrist wake up to enabled

    @param	NULL

	@return NULL
*/
/**************************************************************************/
/**
 * @brief 打开抬腕检测（只置软件开关）
 * @return 无
 * @note 真正的姿态判断在 SensUpdateTask / RunModeTasks 里读这个标志后才执行。
 */
void HW_MPU_Wrist_Enable(void)
{
	#if HW_USE_IMU
		HWInterface.IMU.wrist_is_enabled = 1;
	#endif
}


/**************************************************************************/
/*!
    @brief  set the MPU Wrist wake up to disabled

    @param	NULL

	@return NULL
*/
/**************************************************************************/
/**
 * @brief 关闭抬腕检测
 * @return 无
 */
void HW_MPU_Wrist_Disable(void)
{
	#if HW_USE_IMU
		HWInterface.IMU.wrist_is_enabled = 0;
	#endif
}


/**************************************************************************/
/*!
    @brief  get the MPU steps

    @param	NULL

	@return the steps
*/
/**************************************************************************/
/**
 * @brief 读取 DMP 计步器步数
 * @return 0~65535（超出回绕）
 * @note DMP 内部是 32 位计数，这里强转 uint16 会截断；日步数一般不会到 6 万，暂无影响。
 */
uint16_t HW_MPU_Get_Steps(void)
{
	#if HW_USE_IMU
		unsigned long STEPS = 0;
		if(!HWInterface.IMU.ConnectionError)
// 连接异常时不读 DMP，直接返回 0，避免 I2C 卡死
			dmp_get_pedometer_step_count(&STEPS);
		return (uint16_t)STEPS;
	#endif
		return 0;
}

/**************************************************************************/
/*!
    @brief	set the MPU steps

    @param	NULL

	@return 0 if successful
*/
/**************************************************************************/
/**
 * @brief 写回 DMP 计步器步数（用于从 Flash 恢复）
 * @param count 要写入的步数
 * @return 0 成功 / -1 失败
 * @note HardwareInitTask 从 EEPROM 恢复步数时调用。
 */
int HW_MPU_Set_Steps(unsigned long count)
{
	#if HW_USE_IMU
		if(!HWInterface.IMU.ConnectionError)
			return dmp_set_pedometer_step_count(count);
	#endif
		return -1;
}

/***************************
 *  temprature & humidity Fucntions
 ***************************/

/**************************************************************************/
/*!
    @brief  initialize the AHT21 - temprature & humidity sensor

    @param	NULL

	@return 0 if successful
*/
/**************************************************************************/
/**
 * @brief 初始化 AHT21 温湿度传感器
 * @return 0 成功 / -1 未启用
 */
uint8_t HW_AHT21_Init(void)
{
	#if HW_USE_AHT21
		return AHT_Init();
	#endif
	return -1;
}

/**************************************************************************/
/*!
    @brief  get the temperature and humidity data for the aht21 sensor

    @param	humi humidity
	@param	temp temperature

	@return NULL
*/
/**************************************************************************/
/**
 * @brief 读取温湿度（浮点）
 * @param humi 输出湿度 %RH
 * @param temp 输出温度 ℃
 * @return 无
 * @note 未连接时直接返回，输出参数保持调用方初值——上层要先判断 ConnectionError。
 */
void HW_AHT21_Get_Humi_Temp(float *humi, float *temp)
{
	#if HW_USE_AHT21
		//temp and humi messure
		if(!HWInterface.AHT21.ConnectionError)
			AHT_Read(humi,temp);
	#endif
}


/***************************
 *  Barometer Fucntions - SPL06-001
 ***************************/

/**************************************************************************/
/*!
    @brief  initialize the SPL06-001 Barometer sensor

    @param	NULL

	@return 0 if successful
*/
/**************************************************************************/
/**
 * @brief 初始化 SPL06-001 气压计
 * @return 0 成功 / -1 未启用
 */
uint8_t HW_Barometer_Init(void)
{
	#if HW_USE_SPL06
		return SPL_init();
	#endif

	return -1;
}


/***************************
 *  E-compass Fucntions - LSM303
 ***************************/

/**************************************************************************/
/*!
    @brief  initialize the lsm303 E-compass sensor

    @param	NULL

	@return 0 if successful
*/
/**************************************************************************/
/**
 * @brief 初始化 LSM303 电子罗盘
 * @return 0 成功 / -1 未启用
 */
uint8_t HW_Ecompass_Init(void)
{
	#if HW_USE_LSM303
		return LSM303DLH_Init();
	#endif

	return -1;
}

/**************************************************************************/
/*!
    @brief  SET the lsm303 E-compass sensor to sleep mode

    @param	NULL

	@return NULL
*/
/**************************************************************************/
/**
 * @brief 让 LSM303 进入休眠（低功耗模式）
 * @return 无
 * @note 罗盘页退出时调用，避免磁力计持续采样耗电。
 */
void HW_Ecompass_Sleep(void)
{
	#if HW_USE_LSM303
		LSM303DLH_Sleep();
	#endif
}

/***************************
 *  heart rate meter Fucntions - em7028
 ***************************/

/**************************************************************************/
/*!
    @brief  initialize the EM7028 heart rate sensor

    @param	NULL

	@return 0 if successful
*/
/**************************************************************************/
/**
 * @brief 初始化 EM7028 心率/血氧传感器
 * @return 0 成功 / -1 未启用
 */
uint8_t HW_HRmeter_Init(void)
{
	#if HW_USE_EM7028
		return EM7028_hrs_init();
	#endif

	return -1;

}

/**************************************************************************/
/*!
    @brief  SET the EM7028 HR sensor to sleep mode

    @param	NULL

	@return NULL
*/
/**************************************************************************/
/**
 * @brief 让 EM7028 进入休眠
 * @return 无
 * @note 心率页退出时调用；再次进页需要重新 Init。
 */
void HW_HRmeter_Sleep(void)
{
	#if HW_USE_EM7028
		EM7028_hrs_DisEnable();
	#endif
}






/***************************
 *  External Variables
 ***************************/
/**
 * @brief 全局硬件接口实例：把 23 个适配函数按外设分组绑定
 * @return 无
 * @note 这是整个工程唯一的 HWInterface 定义处，其它 .c 只能 extern 引用。
 *       ConnectionError 初值全为 1（未初始化），UI 首次显示的是下面写死的占位值。
 */
HW_InterfaceTypeDef HWInterface = {
// RTC 组：4 个函数指针，无数据字段
    .RealTimeClock = {
        .GetTimeDate = HW_RTC_Get_TimeDate,
        .SetDate = HW_RTC_Set_Date,
        .SetTime = HW_RTC_Set_Time,
        .CalculateWeekday = HW_weekday_calculate
    },
// BLE 组：Init/Enable/Disable
    .BLE = {
		.Init = HW_BLE_Init,
        .Enable = HW_BLE_Enable,
        .Disable = HW_BLE_Disable
    },
// 电源组：power_remain 是唯一可写字段，初值 0
    .Power = {
		.power_remain = 0,
		.Init = HW_Power_Init,
        .Shutdown = HW_Power_Shutdown,
		.BatCalculate = HW_Power_BatCalculate
    },
// LCD 组：只有背光
    .LCD = {
        .SetLight = HW_LCD_Set_Light
    },

// IMU 组：ConnectionError=1 表示尚未初始化；Steps=0；抬腕默认关
	.IMU = {
		.ConnectionError = 1,
		.Steps = 0,
		.wrist_is_enabled = 0,
		.wrist_state = WRIST_UP,
		.Init = HW_MPU_Init,
        .WristEnable = HW_MPU_Wrist_Enable,
        .WristDisable = HW_MPU_Wrist_Disable,
        .GetSteps = HW_MPU_Get_Steps,
		.SetSteps = HW_MPU_Set_Steps
    },
// AHT21：初值 26℃/67% 只是让首页不显示 0，首次采样后覆盖
	.AHT21 = {
		.ConnectionError = 1,
		.humidity = 67,
		.temperature = 26,
		.Init = HW_AHT21_Init,
		.GetHumiTemp = HW_AHT21_Get_Humi_Temp
	},
// 气压计：初值 19 m
	.Barometer = {
		.ConnectionError = 1,
		.altitude = 19,
		.Init = HW_Barometer_Init,
	},
// 罗盘：初值 45 度
	.Ecompass = {
		.ConnectionError = 1,
		.direction = 45,
		.Init = HW_Ecompass_Init,
		.Sleep = HW_Ecompass_Sleep
	},
// 心率：HrRate=0，SPO2 占位 99
	.HR_meter = {
		.ConnectionError = 1,
		.HrRate = 0,
		.SPO2 = 99,
		.Init = HW_HRmeter_Init,
		.Sleep = HW_HRmeter_Sleep
	}
};

