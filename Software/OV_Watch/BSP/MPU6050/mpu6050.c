/*
 * 【中文导读】mpu6050.c —— MPU6050 计步/抬腕检测（B级导读+重点）
 * 注意：这里没用官方 DMP 库（功耗高），作者手工配置的低功耗方案；
 * 抬腕亮屏 = 加速度方向突变判定手势。README 作者特别说明此坑。
 */

#include "mpu6050.h"

#define MPU_INT_PORT GPIOB
#define MPU_INT_PIN  GPIO_PIN_12

#define CLK_ENABLE __HAL_RCC_GPIOB_CLK_ENABLE();

iic_bus_t MPU_bus =
{
	.IIC_SDA_PORT = GPIOB,
	.IIC_SCL_PORT = GPIOB,
	.IIC_SDA_PIN  = GPIO_PIN_13,
	.IIC_SCL_PIN  = GPIO_PIN_14,
};


/**************************************************************************/
/*!
    @brief  initialize the iic port connect with MPU6050

    @param  NULL
*/
/**************************************************************************/
/**
 * @brief 配置 MPU6050 中断脚 PB12（下降沿触发）
 * @return 无
 * @note MPU 的 INT 脚接 PB12，配置为 EXTI 下降沿 + 上拉；优先级 5，低于按键但高于 GUI 任务。
 */
void MPU_INT_Pin_Init()
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin : PB12 */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
	HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}


/**************************************************************************/
/*!
    @brief  initialize the motion function of MPU6050

    @param  NULL
*/
/**************************************************************************/
/**
 * @brief 配置“运动检测”中断（抬腕/翻腕的硬件基础）
 * @return 无
 * @note 0x01 阈值=2 mg/LSB 的 1 倍；0x01 持续 1 ms；0x90 让 INT 低有效且 50 us 复位；0x40 打开 MOT_INT。
 */
void MPU_Motion_Init(void)
{
    MPU_Write_Byte(MPU_MOTION_DET_REG,0x01);    //set the acceleration threshold is (LSB*2)mg
    MPU_Write_Byte(MPU_MOTION_DUR_REG,0x01);    //Acceleration detection time is ()ms
    MPU_Write_Byte(MPU_INTBP_CFG_REG,0X90);     //INT Pin active low level, reset until 50us
    MPU_Write_Byte(MPU_INT_EN_REG,0x40);       	//enable INT
}


/**************************************************************************/
/*!
    @brief  initialize the IIC bus

    @param  NULL
*/
/**************************************************************************/
/**
 * @brief 初始化 MPU 专用的软件 I2C 总线
 * @return 无
 * @note SCL=PB14、SDA=PB13；MPU_bus 结构体在文件头定义，所有读写都走这条总线。
 */
void MPU_Bus_Init(void)
{
	CLK_ENABLE;
	IICInit(&MPU_bus);
}


/**************************************************************************/
/*!
    @brief  init the MPU6050

    @param  NULL

    @return 0 if success
*/
/**************************************************************************/
/**
 * @brief 初始化 MPU6050 为“低功耗计步 + 运动中断”模式
 * @return 0 成功 / 1 器件 ID 不匹配
 * @note 不是官方 DMP 方案：先复位、唤醒，陀螺仪 2000dps、加速度 8g、采样 50 Hz，
 *       再关掉 FIFO/主模式，最后进 cycle 模式（0x28）只开加速度计、20 Hz 唤醒。
 */
u8 MPU_Init(void)
{
	u8 res;

	MPU_Bus_Init();

	MPU_Write_Byte(MPU_PWR_MGMT1_REG,0X80);	//复位MPU6050
  delay_ms(100);
	MPU_Write_Byte(MPU_PWR_MGMT1_REG,0X00);	//唤醒MPU6050
	MPU_Set_Gyro_Fsr(3);										//G传感器, 2000dps
	MPU_Set_Accel_Fsr(2);										//A传感器, 8g
	MPU_Set_Rate(50);												//采样率50Hz
	MPU_Write_Byte(MPU_INT_EN_REG,0X00);		//关闭所有中断
	MPU_Write_Byte(MPU_USER_CTRL_REG,0X00);	//IIC主模式关闭
	MPU_Write_Byte(MPU_FIFO_EN_REG,0X00);		//dis FIFO
	MPU_Write_Byte(MPU_INTBP_CFG_REG,0X80);	//INT active low

	res=MPU_Read_Byte(MPU_DEVICE_ID_REG);
	if(res==MPU_ADDR)//ID
	{
		MPU_Write_Byte(MPU_PWR_MGMT1_REG,0X28);	//SET the internal 8MHz,sleep=0,cycle=1,TEMP_DIS=1//low power modes
		MPU_Write_Byte(MPU_PWR_MGMT2_REG,0X87);	//enable accelerometer,disanable gyroscope,set the wake up frequence=20Hz
		MPU_Set_Rate(50);												//采样率50Hz
 	}else return 1;

	MPU_Motion_Init();
	MPU_INT_Pin_Init();


	return 0;
}

/**
 * @brief 进入睡眠：sleep=1、cycle=0、temp_dis=1
 * @return 无
 * @note 0x48 = 0100_1000；睡眠时仍保留内部 8 MHz 时钟，唤醒后可立即采样。
 */
void MPU_Sleep()
{
	MPU_Write_Byte(MPU_PWR_MGMT1_REG,0x48);//sleep=1,cycle=0,temp_dis=1,internal 8MHz
}

/**
 * @brief 唤醒：sleep=0、cycle=1、temp_dis=1
 * @return 无
 * @note 0x28 = 0010_1000；cycle 模式下芯片按 PWR_MGMT2 设定的 20 Hz 间歇工作。
 */
void MPU_Wakeup()
{
	//low power modes
	MPU_Write_Byte(MPU_PWR_MGMT1_REG,0x28);//sleep=0,cycle=1,temp_dis=1,internal 8MHz
}

/**
 * @brief 读中断状态寄存器
 * @return INT_STA 原始值（bit6=MOT_INT，bit0=DATA_RDY）
 * @note SensUpdateTask 用它判断这次中断是“运动”还是“数据就绪”。
 */
uint8_t MPU_Read_Status()
{
	return MPU_Read_Byte(MPU_INT_STA_REG);
}


/**************************************************************************/
/*
    @brief  设置MPU6050陀螺仪传感器满量程范围

    @param  fsr:0,+250dps;1,500dps;2,+1000dps;3,+2000dps

    @return 0 if success
*/
/**************************************************************************/
/**
 * @brief 设置陀螺仪满量程
 * @param fsr 0=±250 / 1=±500 / 2=±1000 / 3=±2000 dps
 * @return 0 成功
 * @note 写入 GYRO_CONFIG 的 bit4:3；本工程用 3（±2000dps），抬腕判定只关心方向突变。
 */
u8 MPU_Set_Gyro_Fsr(u8 fsr)
{
	return MPU_Write_Byte(MPU_GYRO_CFG_REG,fsr<<3);
}


/**************************************************************************/
/*
    @brief  设置MPU6050的数字低通滤波器

    @param  fsr:低通滤波器频率(Hz)

    @return 0 if success
*/
/**************************************************************************/
/**
 * @brief 设置加速度计满量程
 * @param fsr 0=±2g / 1=±4g / 2=±8g / 3=±16g
 * @return 0 成功
 * @note 写入 ACCEL_CONFIG 的 bit4:3；本工程用 2（±8g），留够挥臂时的动态范围。
 */
u8 MPU_Set_Accel_Fsr(u8 fsr)
{
	return MPU_Write_Byte(MPU_ACCEL_CFG_REG,fsr<<3);
}


/**************************************************************************/
/*
    @brief  设置MPU6050的低通滤波器

    @param  lpf: Hz

    @return 0 if success
*/
/**************************************************************************/
/**
 * @brief 设置数字低通滤波器截止频率
 * @param lpf 目标带宽 Hz
 * @return 0 成功
 * @note 按数据手册的档位表折算成 CONFIG 寄存器 DLPF_CFG；带宽越低噪声越小、延迟越大。
 */
u8 MPU_Set_LPF(u16 lpf)
{
	u8 data=0;
	if(lpf>=188)data=1;
	else if(lpf>=98)data=2;
	else if(lpf>=42)data=3;
	else if(lpf>=20)data=4;
	else if(lpf>=10)data=5;
	else data=6;
	return MPU_Write_Byte(MPU_CFG_REG,data);
}


/**************************************************************************/
/*
    @brief  设置MPU6050的采样率

    @param  rate: 4~1000 Hz

    @return 0 if success
*/
/**************************************************************************/
/**
 * @brief 设置采样率（分频器）
 * @param rate 目标采样率 Hz
 * @return 0 成功
 * @note 采样率 = 1 kHz / (1 + SMPLRT_DIV)，rate>1000 时按 1000 处理。
 */
u8 MPU_Set_Rate(u16 rate)
{
	u8 data;
	if(rate>1000)rate=1000;
	if(rate<4)rate=4;
	data=1000/rate-1;
	data=MPU_Write_Byte(MPU_SAMPLE_RATE_REG,data);
 	return MPU_Set_LPF(rate/2);
}

/**************************************************************************/
/*
    @brief  获取MPU6050温度值

    @param  NULL

    @return temperature (short)
*/
/**************************************************************************/
/**
 * @brief 读片上温度
 * @return 温度，单位 0.01 ℃
 * @note 原始值 /340 + 36.53 才是摄氏度；本工程只做粗略监控，未参与 UI 显示。
 */
short MPU_Get_Temperature(void)
{
    u8 buf[2];
    short raw;
		float temp;
		MPU_Read_Len(MPU_ADDR,MPU_TEMP_OUTH_REG,2,buf);
    raw=((u16)buf[0]<<8)|buf[1];
    temp=36.53+((double)raw)/340;
    return temp*100;;
}


/**************************************************************************/
/*
    @brief  获取MPU6050陀螺仪原始值

    @param  NULL

    @return 0 if success
*/
/**************************************************************************/
/**
 * @brief 读三轴角速度原始值
 * @param gx/gy/gz 输出原始 ADC 值
 * @return 0 成功
 * @note 量程 ±2000dps 时 1 LSB = 16.4 LSB/(°/s)；换算成度每秒要除以 16.4。
 */
u8 MPU_Get_Gyroscope(short *gx,short *gy,short *gz)
{
    u8 buf[6],res;
		res=MPU_Read_Len(MPU_ADDR,MPU_GYRO_XOUTH_REG,6,buf);
		if(res==0)
		{
			*gx=((u16)buf[0]<<8)|buf[1];
			*gy=((u16)buf[2]<<8)|buf[3];
			*gz=((u16)buf[4]<<8)|buf[5];
		}
    return res;;
}


/**************************************************************************/
/*
    @brief  获取MPU6050加速度原始值

    @param  NULL

    @return 0 if success
*/
/**************************************************************************/
/**
 * @brief 读三轴加速度原始值
 * @param ax/ay/az 输出原始 ADC 值
 * @return 0 成功
 * @note 量程 ±8g 时 1 LSB = 4096 LSB/g；MPU_Get_Angles 直接拿原始值算角度，比例因子会约掉。
 */
u8 MPU_Get_Accelerometer(short *ax,short *ay,short *az)
{
    u8 buf[6],res;
		res=MPU_Read_Len(MPU_ADDR,MPU_ACCEL_XOUTH_REG,6,buf);
		if(res==0)
		{
			*ax=((u16)buf[0]<<8)|buf[1];
			*ay=((u16)buf[2]<<8)|buf[3];
			*az=((u16)buf[4]<<8)|buf[5];
		}
    return res;;
}

/**************************************************************************/
/*
    @brief  IIC连续写

    @param  addr:器件地址
    @param  reg:寄存器地址
	@param  len:写入长度
	@param  buf:数据区

    @return 0 if success
*/
/**************************************************************************/
/**
 * @brief 连续写多个寄存器（软件 I2C 实现）
 * @param addr 器件地址（7 位）
 * @param reg 起始寄存器
 * @param len 字节数
 * @param buf 数据
 * @return 0 成功
 * @note 起始+地址+寄存器+逐字节写+停止；没有超时重试，总线被占时会一直等。
 */
u8 MPU_Write_Len(u8 addr,u8 reg,u8 len,u8 *buf)
{
	u8 i;
  IICStart(&MPU_bus);
	IICSendByte(&MPU_bus,(addr<<1)|0);
	if(IICWaitAck(&MPU_bus))
	{
		IICStop(&MPU_bus);
		return 1;
	}
    IICSendByte(&MPU_bus,reg);
    IICWaitAck(&MPU_bus);
	for(i=0;i<len;i++)
	{
		IICSendByte(&MPU_bus,buf[i]);
		if(IICWaitAck(&MPU_bus))
		{
			IICStop(&MPU_bus);
			return 1;
		}
	}
    IICStop(&MPU_bus);
	return 0;
}

/**************************************************************************/
/*
    @brief  IIC写单字节

    @param  reg:寄存器地址
	@param  data:数据(uint8_t)

    @return 0 if success
*/
/**************************************************************************/
/**
 * @brief 写单个寄存器
 * @param reg 寄存器地址
 * @param data 写入值
 * @return 0 成功
 */
u8 MPU_Write_Byte(u8 reg,u8 data)
{
  IICStart(&MPU_bus);
	IICSendByte(&MPU_bus, (MPU_ADDR<<1)|0);
	if(IICWaitAck(&MPU_bus))
	{
		IICStop(&MPU_bus);
		return 1;
	}
	IICSendByte(&MPU_bus,reg);
	IICWaitAck(&MPU_bus);
	IICSendByte(&MPU_bus,data);
	if(IICWaitAck(&MPU_bus))
	{
		IICStop(&MPU_bus);
		return 1;
	}
  IICStop(&MPU_bus);
	return 0;
}


/**************************************************************************/
/*
    @brief  IIC读单字节

    @param  reg:寄存器地址

    @return 0 if success
*/
/**************************************************************************/
/**
 * @brief 读单个寄存器
 * @param reg 寄存器地址
 * @return 读到的字节
 * @note 主机发送重启动后读 1 字节并 NACK，是 I2C 读寄存器的标准时序。
 */
u8 MPU_Read_Byte(u8 reg)
{
	u8 res;
  IICStart(&MPU_bus);
	IICSendByte(&MPU_bus,(MPU_ADDR<<1)|0);
	IICWaitAck(&MPU_bus);
  IICSendByte(&MPU_bus,reg);
  IICWaitAck(&MPU_bus);
  IICStart(&MPU_bus);
	IICSendByte(&MPU_bus,(MPU_ADDR<<1)|1);
  IICWaitAck(&MPU_bus);
	res=IICReceiveByte(&MPU_bus);
	IICSendNotAck(&MPU_bus);
  IICStop(&MPU_bus);
	return res;
}


/**************************************************************************/
/*
    @brief  IIC连续读

    @param  addr:器件地址
    @param  reg:寄存器地址
	@param  len:写入长度
	@param  buf:数据区

    @return 0 if success
*/
/**************************************************************************/
/**
 * @brief 连续读多个寄存器
 * @param addr 器件地址
 * @param reg 起始寄存器
 * @param len 字节数
 * @param buf 输出缓冲
 * @return 0 成功
 * @note 除最后一个字节回 NACK 外，其余每字节都回 ACK。
 */
u8 MPU_Read_Len(u8 addr,u8 reg,u8 len,u8 *buf)
{
 	IICStart(&MPU_bus);
	IICSendByte(&MPU_bus,(addr<<1)|0);
	if(IICWaitAck(&MPU_bus))
	{
		IICStop(&MPU_bus);
		return 1;
	}
    IICSendByte(&MPU_bus,reg);
    IICWaitAck(&MPU_bus);
    IICStart(&MPU_bus);
		IICSendByte(&MPU_bus,(addr<<1)|1);
    IICWaitAck(&MPU_bus);
		while(len)
		{
			if(len==1)
			{
				*buf=IICReceiveByte(&MPU_bus);
				IICSendNotAck(&MPU_bus);
			}
			else
			{
				*buf=IICReceiveByte(&MPU_bus);
				IICSendAck(&MPU_bus);
			}
			len--;
			buf++;
		}
    IICStop(&MPU_bus);
		return 0;
}

/**
 * @brief 多字节写（封装 IIC_Write_Multi_Byte）
 * @param addr 寄存器地址
 * @param length 长度
 * @param buff 数据
 * @return 0 成功 / 1 失败
 */
uint8_t MPU_Write_Multi_Byte(uint8_t addr,uint8_t length,uint8_t buff[])
{
	if(IIC_Write_Multi_Byte(&MPU_bus,MPU_ADDR<<1,addr,length,buff))
	{
		return 1;
	}
	return 0;
}

/**
 * @brief 多字节读（封装 IIC_Read_Multi_Byte）
 * @param addr 寄存器地址
 * @param length 长度
 * @param buff 输出
 * @return 0 成功 / 1 失败
 */
uint8_t MPU_Read_Multi_Byte(uint8_t addr, uint8_t length, uint8_t buff[])
{
	if(IIC_Read_Multi_Byte(&MPU_bus, MPU_ADDR<<1, addr, length, buff))
	{
		return 1;
	}
	return 0;
}


/**************************************************************************/
/*
    @brief  get the roll and pitch

    @param  roll:roll(float)
    @param  pitch:pitch(float)

    @return NULL
*/
/**************************************************************************/
/**
 * @brief 由加速度算横滚角与俯仰角
 * @param roll 输出横滚角（弧度）
 * @param pitch 输出俯仰角（弧度）
 * @return 无
 * @note pitch=-atan(ax/sqrt(ay²+az²))、roll=atan(ay/az)；只在静止/准静态下准确，挥臂时会叠加线性加速度噪声。
 */
void MPU_Get_Angles(float * roll,float * pitch)
{
	short ax,ay,az;
	MPU_Get_Accelerometer(&ax,&ay,&az);
	*pitch = -atanf(ax/sqrtf(ay*ay+az*az));
	*roll = atanf((float)ay/(float)az);
}


/**************************************************************************/
/*
    @brief  check the MPU6050 is horizontal or not

    @param  NULL

    @return 1 if is horizontal
*/
/**************************************************************************/
/**
 * @brief 判断手表是否处于水平姿态
 * @return 1 水平 / 0 非水平
 * @note roll 与 pitch 都落在 ±0.50 弧度（约 ±28.6°）内即认为水平；RunModeTasks 用它判断“抬腕”与“放下”。
 */
uint8_t MPU_isHorizontal(void)
{
	float roll,pitch; // 横滚角，俯仰角
	MPU_Get_Angles(&roll,&pitch); // 获取当前角度
	if(roll<=0.50 && roll>=-0.50 && pitch<=0.50 && pitch>=-0.50) // 如果满足roll 和 pitch 都落在 ±0.50 的范围内就认为设备水平，返回1
	{return 1;}
	return 0;
}

