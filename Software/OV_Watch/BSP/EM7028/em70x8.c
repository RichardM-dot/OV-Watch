/*
 * ============================================================================
 * 【中文导读】em70x8.c —— EM7028 心率传感器驱动（B级导读）
 * ----------------------------------------------------------------------------
 * EM7028 挂在 PB13/PB14 的软件 I2C 上，7 位地址 0x24，ID 寄存器 0x00 应为 0x36。
 * 它内部有 LED + 光电二极管，直接输出“光电容积脉搏波”的原始 ADC 值，
 * 心率是靠上层 HrAlgorythm.c 找峰值算出来的，本文件只负责配置和读数。
 *
 * 阅读顺序：ReadOneReg/WriteOneReg → Get_ID（确认在线）
 *           → hrs_init（配置 HRS1 通道）→ hrs_Enable/DisEnable
 *           → Get_HRS1（读原始波形）。
 *
 * 配置要点：
 *   - HRS_CFG=0x00 先关通道，再配 HRS1_CTRL=0x47（增益 1、量程 8、
 *     2.62144MHz/1.5625ms、16 位）和 INT_CTRL=0x00（LED 电流 2.5mA）；
 *   - 使能只需 HRS_CFG=0x08（只开 HRS1，红灯 LED1）；
 *   - 所有函数都先轮询 ID：5 次 x 100ms，最坏会阻塞 500ms，
 *     传感器没插好时上层任务会被卡住。
 *
 * 坑：本文件原来给 return 值标的“成功/失败”是反的——返回 1 表示
 *     “ID 没读到/初始化失败”，返回 0 才是成功（已在本版注释中纠正）。
 * ============================================================================
 */

#include "em70x8.h"

#define CLK_ENABLE __HAL_RCC_GPIOB_CLK_ENABLE();
iic_bus_t EM7028_bus = 
{
	.IIC_SDA_PORT = GPIOB,
	.IIC_SCL_PORT = GPIOB,
	.IIC_SDA_PIN  = GPIO_PIN_13,
	.IIC_SCL_PIN  = GPIO_PIN_14,
};

/**
  * @brief  读一个寄存器
  * @param  RegAddr 寄存器地址
  * @retval 寄存器值（器件不应答时底层返回 0xFF）
  */
uint8_t  EM7028_ReadOneReg(unsigned char RegAddr)
{
	unsigned char dat;
	dat = IIC_Read_One_Byte(&EM7028_bus, EM7028_ADDR, RegAddr);
	return dat;
}

/**
  * @brief  写一个寄存器
  * @param  RegAddr 寄存器地址
  * @param  dat     写入值
  * @retval 无（底层写失败不会返回错误）
  */
void  EM7028_WriteOneReg(unsigned char RegAddr, unsigned char dat)
{
	IIC_Write_One_Byte(&EM7028_bus, EM7028_ADDR, RegAddr, dat);
}

/**
  * @brief  读芯片 ID
  * @param  无
  * @retval ID 寄存器(0x00) 的值，正常应为 0x36
  */
uint8_t EM7028_Get_ID()
{
	return EM7028_ReadOneReg(ID_REG);
}

/**
  * @brief  初始化心率通道 HRS1
  * @param  无
  * @retval 0=成功；1=ID 读不到（传感器不在线）
  * @note   先开 GPIOB 时钟 + 初始化 I2C，再轮询 ID：最多 5 次、每次 100ms，
  *         给传感器上电启动留时间（最坏阻塞 500ms）。
  *         配置顺序：先写 HRS_CFG=0 关通道，再配偏移/增益/量程/中断/LED 电流，
  *         避免在通道开着的时候改参数产生毛刺。
  *         HRS1_CTRL=0x47 的位含义：增益 1、量程 8、2.62144MHz(1.5625ms)、
  *         16 位、HRS1 模式——这些值直接决定原始波形的幅度和噪声。
  */
uint8_t EM7028_hrs_init()
{
	uint8_t i = 5;
	
	CLK_ENABLE;
	IICInit(&EM7028_bus);
	
	while(EM7028_Get_ID() != 0x36 && i)
	{
		HAL_Delay(100);
		i--;
	}
	if(!i)
	{return 1;}
	EM7028_WriteOneReg(HRS_CFG,0x00);				
	//HRS1_EN, HRS2_dis
	//Heart Beat Measurement is enabled with LED1 turned on and only Red Light Sensor and IR sensor enabled. 
	//When LED1 turned on, the result stores to HRS_DATA0;
	EM7028_WriteOneReg(HRS2_DATA_OFFSET, 0x00);
	//0 offset
	EM7028_WriteOneReg(HRS2_GAIN_CTRL, 0x7f);		
	//HRS2 GAIN = 1
	EM7028_WriteOneReg(HRS1_CTRL, 0x47);
	//HRS1 GAIN =1, HRS1 RANGE =8, HRS1 FREQ = 2.62144MHz (1.5625ms), HRS1 RES = 16 bits, HRS1 mode
	EM7028_WriteOneReg(INT_CTRL, 0x00);
	//LED programmed current = 2.5mA
	return 0;
}

/**
  * @brief  使能心率采集（只开 HRS1 通道）
  * @param  无
  * @retval 0=成功；1=ID 读不到
  * @note   HRS_CFG=0x08 打开 HRS1：LED1 点亮，只有红光+红外传感器工作，
  *         结果写入 HRS_DATA0。写之前同样先确认 ID（最多阻塞 500ms）。
  */
uint8_t EM7028_hrs_Enable()
{
	uint8_t i = 5;
	while(EM7028_Get_ID() != 0x36 && i)
	{
		HAL_Delay(100);
		i--;
	}
	if(!i)
	{return 1;} // 成功（原文标反了：这里其实是失败）
	EM7028_WriteOneReg(HRS_CFG,0x08);
	return 0; // 失败（原文标反了：这里才是成功）
}

/**
  * @brief  关闭心率采集
  * @param  无
  * @retval 0=成功；1=ID 读不到
  * @note   只是把 HRS_CFG 清 0，LED 熄灭、通道停止；I2C 和芯片本身没有断电。
  */
uint8_t EM7028_hrs_DisEnable()
{
	uint8_t i = 5;
	while(EM7028_Get_ID() != 0x36 && i)
	{
		HAL_Delay(100);
		i--;
	}
	if(!i)
	{return 1;}
	EM7028_WriteOneReg(HRS_CFG,0x00);
	return 0;
}

/**
  * @brief  读 HRS1 通道的原始光电容积脉搏波数据
  * @param  无
  * @retval 16 位原始值（HRS1_DATA0_H<<8 | HRS1_DATA0_L）
  * @note   先读高字节再读低字节，然后拼成 16 位——两次读之间如果刚好更新，
  *         可能拼出跨帧的数据（没有做一致性检查）。
  *         只用了 DATA0，芯片还有 DATA1~DATA3（FIFO/多相），本项目没用。
  *         这个原始值不能直接当心率，要送 HR_Calculate() 找峰值。
  */
uint16_t EM7028_Get_HRS1(void)
{
	uint16_t dat;
	dat = EM7028_ReadOneReg(HRS1_DATA0_H);
	dat <<= 8;
	dat |= EM7028_ReadOneReg(HRS1_DATA0_L);
	return dat;
}
