/*
 * ============================================================================
 * 【中文导读】AHT21.c —— 温湿度传感器 AHT21 驱动（B级导读）
 * ----------------------------------------------------------------------------
 * AHT21 挂在 PB13/PB14 的软件 I2C 上，7 位地址 0x38（写 0x70 / 读 0x71）。
 * 它是“命令式”器件：主机发命令 → 等转换 → 读结果，没有寄存器地址概念，
 * 所以本文件直接调 IICStart/IICSendByte 拼时序，而不是用 IIC_x_Byte 封装。
 *
 * 一次测量的完整流程（看 AHT_Read 就够）：
 *   发 0xAC 0x33 0x00 触发 → 等 80 ms → 轮询状态 bit7（忙）→
 *   发 0x71 读 6 字节 → 湿度 20 位、温度 20 位 → 按公式换算。
 *
 * 阅读顺序：AHT_Read_Status → AHT_Read_Cal_Enable → AHT_Reset → AHT_Init
 *           → AHT_Read。
 *
 * 两个**已知 bug**（不影响基本功能，但读代码时别被绕进去）：
 *   1) `AHT_Read_Status() & 0x80 == 0x80`：== 的优先级高于 &，实际等价于
 *      `status & (0x80==0x80)` = `status & 1`，判断的是 bit0 而不是忙标志
 *      bit7。因为循环还有 cnt 超时，所以不会死等，只是“等了个寂寞”。
 *   2) AHT_Init 里 `AHT_Read_Status() && 0x08!=0x08`：`0x08!=0x08` 恒为 0，
 *      整个条件恒假，0xBE 校准初始化命令**从来不会发出去**。实际靠 AHT21
 *      出厂校准值工作；要严谨应改成 `if(AHT_Read_Status() & 0x08)` 之类。
 * ============================================================================
 */

#include "AHT21.h"

#define AHT_CLK_ENABLE __HAL_RCC_GPIOB_CLK_ENABLE()
iic_bus_t AHT_bus = 
{
	.IIC_SDA_PORT = GPIOB,
	.IIC_SCL_PORT = GPIOB,
	.IIC_SDA_PIN  = GPIO_PIN_13,
	.IIC_SCL_PIN  = GPIO_PIN_14,
};

/**
  * @brief  读 AHT21 的状态字节
  * @param  无
  * @retval 状态字节：bit7=忙，bit3=校准使能，bit6:5=工作模式
  * @note   这是“读”操作：Start → 0x71 → 读 1 字节 → NACK → Stop。
  *         0x71 = 0x38<<1 | 1，已经包含读写位，所以直接用裸时序。
  */
uint8_t AHT_Read_Status(void)
{
	uint8_t Byte_first;	
	IICStart(&AHT_bus);
	IICSendByte(&AHT_bus,0x71);
	IICWaitAck(&AHT_bus);
	Byte_first = IICReceiveByte(&AHT_bus);
	IICSendNotAck(&AHT_bus);
	IICStop(&AHT_bus);	
	return Byte_first;
}

/**
  * @brief  判断校准是否使能、是否处于 NOR 模式
  * @param  无
  * @retval 1=已校准且 NOR 模式；0=未就绪
  * @note   `val & 0x68` 同时看 bit6、bit5（模式）和 bit3（校准使能），
  *         等于 0x08 表示 bit3=1 且 bit6/bit5=0，即 NOR 模式 + 校准完成。
  */
uint8_t AHT_Read_Cal_Enable(void)  //check cal enable bit 
{
	uint8_t val = 0;//ret = 0,
 
  val = AHT_Read_Status();
  if((val & 0x68)==0x08)  //check NOR mode 
		return 1;
  else  
		return 0;
}

/**
  * @brief  软复位 AHT21（发 0xBA）
  * @param  无
  * @retval 无
  * @note   写操作：Start → 0x70 → 0xBA → Stop。复位后要重新等 40 ms 以上
  *         才能再发测量命令。当前 AHT_Init 里这行被注释掉了，没实际调用。
  */
void AHT_Reset(void)//AHT21 send 0xBA reset call
{
	IICStart(&AHT_bus);
	IICSendByte(&AHT_bus,0x70);
	IICWaitAck(&AHT_bus);
	IICSendByte(&AHT_bus,0xBA);
	IICWaitAck(&AHT_bus);
	IICStop(&AHT_bus);
}

/**
  * @brief  初始化 AHT21：开时钟、初始化 I2C、等上电稳定、（理论上）发校准命令
  * @param  无
  * @retval 固定返回 0（没有做真正的在线检测）
  * @note   delay_ms(40) 是上电稳定时间，AHT21 要求 ≥40 ms 才能接受命令。
  *         下面的 if 条件 `AHT_Read_Status() && 0x08!=0x08` 恒为假，
  *         所以 0xBE 0x08 0x00 这段校准初始化**永远不会执行**——这是
  *         本文件最隐蔽的 bug，排查“温湿度不准”时可以先怀疑这里。
  *         另外返回值恒 0，上层无法通过返回值判断器件是否存在。
  */
uint8_t AHT_Init(void)
{
	AHT_CLK_ENABLE;
	IICInit(&AHT_bus);
	
	delay_ms(40);
	
	if(AHT_Read_Status()&&0x08!=0x08)
	{
		IICStart(&AHT_bus);
		IICSendByte(&AHT_bus,0x70);
		IICWaitAck(&AHT_bus);
		IICSendByte(&AHT_bus,0xBE);
		IICWaitAck(&AHT_bus);
		IICSendByte(&AHT_bus,0x08);
		IICWaitAck(&AHT_bus);
		IICSendByte(&AHT_bus,0x00);
		IICWaitAck(&AHT_bus);
		IICStop(&AHT_bus);
		delay_ms(10);
	}
	//AHT_Reset();
	
	return 0;
}	
 
/**
  * @brief  触发一次测量并读出湿度/温度
  * @param  humi 输出：相对湿度（%RH）
  * @param  temp 输出：温度（℃）
  * @retval 0=成功；1=等待转换超时（此时不会写 humi/temp）
  * @note   流程：0xAC 0x33 0x00 触发 → 等 80 ms（转换时间）→ 轮询忙标志
  *         （最多 5 次 x 5 ms）→ 0x71 连续读 6 字节。
  *         6 字节布局：状态 + 湿度[19:12] + 湿度[11:4] + 湿度[3:0]&温度[19:16]
  *         + 温度[15:8] + 温度[7:0]；第 7 字节的 CRC 没有读，也没校验。
  *         换算：湿度 = raw * 1000 / 2^20 / 10（%RH）；
  *              温度 = (raw * 2000 / 2^20 - 500) / 10（℃），raw 是 20 位。
  *         raw*1000 和 raw*2000 都在 uint32 范围内，不会溢出。
  */
uint8_t AHT_Read(float *humi, float *temp)
{
	uint8_t cnt=5;
	uint8_t  Byte_1th=0;
	uint8_t  Byte_2th=0;
	uint8_t  Byte_3th=0;
	uint8_t  Byte_4th=0;
	uint8_t  Byte_5th=0;
	uint8_t  Byte_6th=0;
	uint32_t RetuData = 0;
	
	// 触发测量：0xAC 0x33 0x00（AC=触发，33=温湿度都测，00=参数）
	IICStart(&AHT_bus);
	IICSendByte(&AHT_bus,0x70);
	IICWaitAck(&AHT_bus);
	IICSendByte(&AHT_bus,0xAC);
	IICWaitAck(&AHT_bus);
	IICSendByte(&AHT_bus,0x33);
	IICWaitAck(&AHT_bus);
	IICSendByte(&AHT_bus,0x00);
	IICWaitAck(&AHT_bus);
	IICStop(&AHT_bus);	
	
	delay_ms(80); // 触发命令后至少等 80 ms 让 ADC 完成转换
	// 坑：== 比 & 优先级高，这里实际是 status & (0x80==0x80) = status & 1，
	// 判断的是 bit0 而非忙标志 bit7；靠 cnt 超时兜底，不会死循环。
	while(AHT_Read_Status()&0x80==0x80 && cnt)
	{
		delay_ms(5);
		cnt--;
		AHT_Read_Status();
	}
	if(!cnt)
	{return 1;} // 5 次仍没等到，放弃本次测量（输出参数保持原值）
	
	// 读 6 字节：状态 + 湿度(20bit) + 温度(20bit)，第 7 字节 CRC 没读
	IICStart(&AHT_bus);
	IICSendByte(&AHT_bus,0x71);
	IICWaitAck(&AHT_bus);
	Byte_1th = IICReceiveByte(&AHT_bus);
	IICSendAck(&AHT_bus);
	Byte_2th = IICReceiveByte(&AHT_bus);
	IICSendAck(&AHT_bus);
	Byte_3th = IICReceiveByte(&AHT_bus);
	IICSendAck(&AHT_bus);
	Byte_4th = IICReceiveByte(&AHT_bus);
	IICSendAck(&AHT_bus);
	Byte_5th = IICReceiveByte(&AHT_bus);
	IICSendAck(&AHT_bus);
	Byte_6th = IICReceiveByte(&AHT_bus);
	IICSendNotAck(&AHT_bus);
	IICStop(&AHT_bus);

	RetuData = (RetuData|Byte_2th)<<8;
	RetuData = (RetuData|Byte_3th)<<8;
	RetuData = (RetuData|Byte_4th);
	RetuData =RetuData >>4; // 湿度 20 位：丢掉 Byte4 的低 4 位（那是温度的 bit19:16）
	*humi = (RetuData * 1000 >> 20); // 先乘后移，避免整数除法丢精度
	*humi /= 10;
	
	RetuData = 0;
	RetuData = (RetuData|(Byte_4th&0x0f))<<8; // 温度的 bit19:16 藏在 Byte4 低半字节
	RetuData = (RetuData|Byte_5th)<<8;
	RetuData = (RetuData|Byte_6th);
	RetuData = RetuData&0xfffff; // 取 20 位
	*temp = ((RetuData * 2000 >> 20)- 500); // 公式：T = raw*2000/2^20 - 50
	*temp /= 10;
	
	return 0;
}
