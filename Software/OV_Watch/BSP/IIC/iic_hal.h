/**
 * @file    iic_hal.h
 * @brief   软件模拟 I2C（GPIO 位翻转）总线接口。
 *
 * 该模块不依赖 STM32 硬件 I2C 外设，直接用两个 GPIO 模拟 SCL/SDA 时序，
 * 供 AHT21、CST816、BL24C02、EM7028 等传感器/EEPROM 共用。
 * 一条总线用 iic_bus_t 描述，调用时把总线指针传给各函数即可复用同一套时序代码。
 */
#ifndef __IIC_HAL_H
#define __IIC_HAL_H

#include "stm32f4xx_hal.h"

/** I2C 总线描述：记录 SDA/SCL 分别接到哪个 GPIO 端口和引脚 */
typedef struct
{
	GPIO_TypeDef * IIC_SDA_PORT;	// SDA 数据线所在 GPIO 端口（如 GPIOB）
	GPIO_TypeDef * IIC_SCL_PORT;	// SCL 时钟线所在 GPIO 端口（如 GPIOB）
	uint16_t IIC_SDA_PIN;			// SDA 数据线引脚号（GPIO_PIN_x）
	uint16_t IIC_SCL_PIN;			// SCL 时钟线引脚号（GPIO_PIN_x）
	//void (*CLK_ENABLE)(void);
}iic_bus_t;

/** 产生 I2C 起始信号（SCL 高电平期间 SDA 由高变低） */
void IICStart(iic_bus_t *bus);
/** 产生 I2C 停止信号（SCL 高电平期间 SDA 由低变高） */
void IICStop(iic_bus_t *bus);
/** 等待从机应答；返回 0 表示收到 ACK，1 表示超时未应答 */
unsigned char IICWaitAck(iic_bus_t *bus);
/** 主机向从机发送应答 ACK（继续读取下一个字节） */
void IICSendAck(iic_bus_t *bus);
/** 主机发送非应答 NACK（表示读取结束） */
void IICSendNotAck(iic_bus_t *bus);
/** 发送一个字节，高位在先 */
void IICSendByte(iic_bus_t *bus, unsigned char cSendByte);
/** 接收一个字节并返回；调用者需自行发送 ACK/NACK */
unsigned char IICReceiveByte(iic_bus_t *bus);
/** 初始化总线：把 SDA/SCL 配置为开漏输出并置为空闲高电平 */
void IICInit(iic_bus_t *bus);

/** 向从机 daddr 的寄存器 reg 写 1 字节；返回 0 表示成功 */
uint8_t IIC_Write_One_Byte(iic_bus_t *bus, uint8_t daddr,uint8_t reg,uint8_t data);
/** 从寄存器 reg 起连续写 length 字节（EEPROM 页写等）；返回 0 表示成功 */
uint8_t IIC_Write_Multi_Byte(iic_bus_t *bus, uint8_t daddr,uint8_t reg,uint8_t length,uint8_t buff[]);
/** 读从机 daddr 的寄存器 reg，返回读到的字节 */
unsigned char IIC_Read_One_Byte(iic_bus_t *bus, uint8_t daddr,uint8_t reg);
/** 从寄存器 reg 起连续读 length 字节到 buff；返回 0 表示成功 */
uint8_t IIC_Read_Multi_Byte(iic_bus_t *bus, uint8_t daddr, uint8_t reg, uint8_t length, uint8_t buff[]);
#endif
