/*
 * ============================================================================
 * 【中文导读】BL24C02.c —— BL24C02 EEPROM 驱动（B级导读）
 * ----------------------------------------------------------------------------
 * BL24C02 是 2 Kbit（256 字节）的 I2C EEPROM，7 位地址 0x50（A2/A1/A0 接地），
 * 挂在自己的软件 I2C 上：SDA=PA11、SCL=PA12（和其它传感器不是同一组引脚）。
 *
 * 本文件只是三个薄封装，真正的页写/页边界问题在 iic_hal.c 的
 * IIC_Write_Multi_Byte 里，掉电保存的业务逻辑在 DataSave.c 里。
 *
 * 阅读顺序：BL24C02_Init → BL24C02_Write → BL24C02_Read。
 *
 * 坑：
 *   - 一页只有 8 字节，写跨页会在页内回卷覆盖（底层不做分页）；
 *   - 每次写之后芯片有约 5 ms 的内部写周期，期间不响应 I2C，
 *     紧接着读会 NACK，所以 DataSave.c 里到处 delay_ms(10)；
 *   - 写寿命约 100 万次，不要在高频任务里反复写；
 *   - 地址 0x00/0x01 被 DataSave 用作 0x55/0xAA 的“已初始化”标志。
 * ============================================================================
 */

#include "BL24C02.h"

#define BL_CLK_ENABLE __HAL_RCC_GPIOA_CLK_ENABLE()

iic_bus_t BL_bus = 
{
	.IIC_SDA_PORT = GPIOA,
	.IIC_SCL_PORT = GPIOA,
	.IIC_SDA_PIN  = GPIO_PIN_11,
	.IIC_SCL_PIN  = GPIO_PIN_12,
};


/**
  * @brief  向 EEPROM 连续写数据
  * @param  addr   起始存储地址（0~255）
  * @param  length 字节数（**一次不要超过 8，且不要跨页**）
  * @param  buff   数据缓冲区
  * @retval 无（底层返回的 NACK 被丢弃）
  * @note   底层是 IIC_Write_Multi_Byte：Start → 0x50+W → 字地址 → 数据… → Stop。
  *         EEPROM 的页大小是 8 字节，地址在页内自增，写到页尾会回卷到页首，
  *         所以跨页写会覆盖本页开头的数据。本函数不做分页，调用者负责。
  *         写完还要等约 5 ms 的内部写周期才能读（见 DataSave.c 的 delay）。
  */
void BL24C02_Write(uint8_t addr,uint8_t length,uint8_t buff[])
{
	IIC_Write_Multi_Byte(&BL_bus, BL_ADDRESS, addr, length, buff); // 页写：<=8 字节且不跨页
}


/**
  * @brief  从 EEPROM 连续读数据
  * @param  addr   起始存储地址
  * @param  length 字节数（读没有页限制，可以一次读完 256 字节）
  * @param  buff   接收缓冲区
  * @retval 无
  * @note   读之前要确保上一次写已经完成（>5 ms），否则器件不应答，
  *         底层会把 0xFF 填进 buff。
  */
void BL24C02_Read(uint8_t addr, uint8_t length, uint8_t buff[])
{
	IIC_Read_Multi_Byte(&BL_bus, BL_ADDRESS, addr, length, buff);
}


/**
  * @brief  初始化 EEPROM 用的软件 I2C
  * @param  无
  * @retval 无
  * @note   BL_CLK_ENABLE 必须先执行：IICInit 只配引脚、不开时钟。
  *         SDA=PA11、SCL=PA12，和传感器那条 PB13/PB14 总线是分开的。
  */
void BL24C02_Init(void)
{
	BL_CLK_ENABLE;
	IICInit(&BL_bus);
}
