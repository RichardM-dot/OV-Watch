/*
 * ============================================================================
 * 【中文导读】iic_hal.c —— 软件 I2C HAL（B级导读，GBK 编码文件）
 * ----------------------------------------------------------------------------
 * GPIO 模拟 I2C 时序（同 Avem 的 avm_i2c.c 思路），任意引脚皆可 I2C。
 *
 * 为什么不用硬件 I2C：STM32F411 的硬件 I2C 有已知的“总线锁死”问题，而板上
 * MPU6050/AHT21/EM7028/SPL06/LSM303 五颗传感器共用 PB13/PB14 一条总线，
 * 用 GPIO 模拟更容易控制时序、也方便换引脚改板。代价是时序全靠 delay_us()。
 *
 * 一根总线被抽象成 iic_bus_t（端口 + 引脚号），每个驱动自己定义一个实例
 * 再调 IICInit()，因此同一组引脚被多个驱动重复初始化是正常现象（只是重复
 * 配置 GPIO，不冲突）。
 *
 * 阅读顺序：IICInit → IICStart/IICStop → IICSendByte/IICReceiveByte →
 *           IICWaitAck → 四个 IIC_x_Byte 读写封装。
 *
 * 已知的坑：
 *   1) 没有总线恢复：从机在传输中途复位可能把 SDA 一直拉低，此后整条总线
 *      上所有传感器都读不到，只能断电重启（标准做法是补 9 个 SCL 脉冲）。
 *   2) ACK 超时只有 5 次（≈5 us），EEPROM 写周期约 5 ms，写完立刻读会 NACK，
 *      所以 DataSave.c 里用 delay_ms(10) 规避。
 *   3) 延时依赖 delay_us()，而它基于 SysTick，与 FreeRTOS 共用时基。
 * ============================================================================
 */

#include "iic_hal.h"
#include "delay.h"

/**
  * @brief SDA线输入模式配置：把 SDA 从“输出”切成“上拉输入”
  * @note  读 ACK、读数据位之前必须先切输入。否则 MCU 一直强驱动 SDA，
  *        从机根本拉不低，读回来的永远是 1（IICWaitAck/IICReceiveByte
  *        都依赖这个切换）。上拉保证总线空闲时是高电平。
  * @param bus I2C 总线对象（端口 + 引脚号）
  * @retval None
  */
void SDA_Input_Mode(iic_bus_t *bus)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    GPIO_InitStructure.Pin = bus->IIC_SDA_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(bus->IIC_SDA_PORT, &GPIO_InitStructure);
}

/**
  * @brief SDA线输出模式配置：切回“开漏输出”
  * @note  用开漏（OD）而不是推挽（PP）：这样从机拉低 SDA 时不会和 MCU
  *        对推挽“打架”短路，ACK 才读得进来。开漏输出高电平时实际靠
  *        板上的上拉电阻把线拉高，所以这里 Pull 配 NOPULL 也不会影响高电平
  *        （前提是硬件有上拉）。
  * @param bus I2C 总线对象（端口 + 引脚号）
  * @retval None
  */
void SDA_Output_Mode(iic_bus_t *bus)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    GPIO_InitStructure.Pin = bus->IIC_SDA_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(bus->IIC_SDA_PORT, &GPIO_InitStructure);
}

/**
  * @brief SDA线输出一个位
  * @note  直接写 BSRR 寄存器而不是 HAL_GPIO_WritePin()：
  *        - BSRR 低 16 位写 1 置高，高 16 位写 1 置低，单次写是原子的，
  *          不会被中断打断成“读-改-写”的中间态；
  *        - 也省掉一层函数调用，模拟 I2C 的时序才跑得够快。
  * @param bus I2C 总线对象
  * @param val 非 0 输出高电平，0 输出低电平
  * @retval None
  */
void SDA_Output(iic_bus_t *bus, uint16_t val)
{
    if ( val )
    {
        bus->IIC_SDA_PORT->BSRR |= bus->IIC_SDA_PIN;
    }
    else
    {
        bus->IIC_SDA_PORT->BSRR = (uint32_t)bus->IIC_SDA_PIN << 16U;
    }
}

/**
  * @brief SCL线输出一个位
  * @note  与 SDA_Output 同一套 BSRR 写法：高 16 位写 1 = 清0（拉低），
  *        低 16 位写 1 = 置1（拉高）。SCL 只由主机驱动，所以这里推挽/开漏
  *        都不影响从机。
  * @param bus I2C 总线对象
  * @param val 非 0 输出高电平，0 输出低电平
  * @retval None
  */
void SCL_Output(iic_bus_t *bus, uint16_t val)
{
    if ( val )
    {
        bus->IIC_SCL_PORT->BSRR |= bus->IIC_SCL_PIN;
    }
    else
    {
         bus->IIC_SCL_PORT->BSRR = (uint32_t)bus->IIC_SCL_PIN << 16U;
    }
}

/**
  * @brief SDA输入一位
  * @note  HAL_GPIO_ReadPin() 返回的是 GPIO_PinState 枚举（SET/RESET），
  *        这里统一转成 0/1，方便后面的移位与逻辑判断。
  * @param bus I2C 总线对象
  * @retval GPIO读入一位（0 或 1）
  */
uint8_t SDA_Input(iic_bus_t *bus)
{
	if(HAL_GPIO_ReadPin(bus->IIC_SDA_PORT, bus->IIC_SDA_PIN) == GPIO_PIN_SET){
		return 1;
	}else{
		return 0;
	}
}

/**
  * @brief IIC起始信号（Start）
  * @note  起始条件 = SCL 为高电平时，SDA 出现一个下降沿。
  *        序列：SDA=1 → SCL=1 → SDA=0 → SCL=0（最后拉低 SCL 占住总线）。
  *        每步 1~2 us，对应约 100~400 kHz 的软件 I2C 速率；
  *        delay_us 是基于 SysTick 的忙等，FreeRTOS 调度时基会带来抖动，
  *        所以这里只求“够慢能通”，不追求精确频率。
  * @param bus I2C 总线对象
  * @retval None
  */
void IICStart(iic_bus_t *bus)
{
    // 先把 SDA 拉高（总线空闲时两根线都是高）
    SDA_Output(bus,1);
    //delay1(DELAY_TIME);
		delay_us(2);
    // SCL 也拉高，准备在“SCL 高电平期间”制造 SDA 的下降沿
    SCL_Output(bus,1);
		delay_us(1);
    // SDA 由高变低 = 起始条件；此时 SCL 仍是高
    SDA_Output(bus,0);
		delay_us(1);
    // 拉低 SCL，把总线“占住”，接下来开始逐位发数据
    SCL_Output(bus,0);
		delay_us(1);
}

/**
  * @brief IIC结束信号（Stop）
  * @note  停止条件 = SCL 为高电平时，SDA 出现一个上升沿。
  *        序列：SCL=0 → SDA=0 → SCL=1 → SDA=1。
  *        先人为把 SCL/SDA 都压低，是为了不管从机此刻停在哪一位，
  *        都能从确定状态开始收尾，避免多发一个时钟。
  * @param bus I2C 总线对象
  * @retval None
  */
void IICStop(iic_bus_t *bus)
{
    // 先人为把两根线压到低电平，回到确定的初始状态
    SCL_Output(bus,0);
		delay_us(2);
    SDA_Output(bus,0);
		delay_us(1);
    // SCL 拉高后，SDA 由低变高 —— 这个上升沿就是停止条件
    SCL_Output(bus,1);
		delay_us(1);
    // SDA 上升完成，总线回到空闲（两根线都为高）
    SDA_Output(bus,1);
		delay_us(1);

}

/**
  * @brief IIC等待确认信号（读从机 ACK）
  * @note  流程：SDA 切输入（释放总线）→ SCL 拉高 → 读 SDA。
  *        从机应答时会把 SDA 拉低，读到 0 即 ACK 成功；读到 1 就一直等。
  *        超时用 cErrTime 递减实现，只有 5 次（≈5 us），比 I2C 规范短得多，
  *        属于“够用即可”的写法：超时后切回输出、补一个 Stop 释放总线，
  *        返回 ERROR，调用者据此放弃本次传输。
  * @param bus I2C 总线对象
  * @retval SUCCESS 收到 ACK；ERROR 超时（从机不存在或总线被拉死）
  */
unsigned char IICWaitAck(iic_bus_t *bus)
{
    unsigned short cErrTime = 5; // ACK 超时计数：5 次循环约 5 us，够用但不宽松
    // 释放 SDA：切成输入，从机才有机会把线拉低
    SDA_Input_Mode(bus);
    SCL_Output(bus,1);
    while(SDA_Input(bus))
    {
        cErrTime--;
				delay_us(1);
        if (0 == cErrTime)
        {
            // 超时：从机没应答。切回输出并补一个 Stop，避免把总线一直占着
            SDA_Output_Mode(bus);
            IICStop(bus);
            return ERROR;
        }
    }
    // ACK 成功：切回输出，并拉低 SCL，占住总线准备下一个字节
    SDA_Output_Mode(bus);
    SCL_Output(bus,0);
		delay_us(2);
    return SUCCESS;
}

/**
  * @brief IIC发送确认信号（主机给从机 ACK）
  * @note  ACK = SCL 高电平期间 SDA 保持低。主机读完一个字节后用它告诉从机
  *        “这个字节我收到了，继续发下一个”。
  * @param bus I2C 总线对象
  * @retval None
  */
void IICSendAck(iic_bus_t *bus)
{
    SDA_Output(bus,0);
		delay_us(1);
    SCL_Output(bus,1);
		delay_us(1);
    SCL_Output(bus,0);
		delay_us(1);
	
}

/**
  * @brief IIC发送非确认信号（NACK）
  * @note  NACK = SCL 高电平期间 SDA 保持高。读最后一个字节后必须发 NACK，
  *        否则从机会以为主机还要数据，继续占用总线。
  * @param bus I2C 总线对象
  * @retval None
  */
void IICSendNotAck(iic_bus_t *bus)
{
    SDA_Output(bus,1);
		delay_us(1);
    SCL_Output(bus,1);
		delay_us(1);
    SCL_Output(bus,0);
		delay_us(2);

}

/**
  * @brief IIC发送一个字节
  * @note  I2C 规定高位先发（MSB first）：
  *        - `cSendByte & 0x80` 取出 bit7 放到 SDA 上；
  *        - `cSendByte += cSendByte` 等价于左移一位，把次高位顶到 bit7；
  *        - SCL 在数据稳定后再拉高，从机在 SCL 高电平期间采样。
  *        8 位发完后把 SCL 拉低，等主机/从机发 ACK。
  * @param bus I2C 总线对象
  * @param cSendByte 需要发送的字节
  * @retval None
  */
void IICSendByte(iic_bus_t *bus,unsigned char cSendByte)
{
    unsigned char  i = 8;
    while (i--)
    {
        SCL_Output(bus,0);
        delay_us(2);
        SDA_Output(bus, cSendByte & 0x80);
				delay_us(1);
        cSendByte += cSendByte;
				delay_us(1);
        SCL_Output(bus,1);
				delay_us(1);
    }
    SCL_Output(bus,0);
		delay_us(2);
}

/**
  * @brief IIC接收一个字节
  * @note  - 先把 SDA 切输入（释放总线），否则从机拉不低；
  *        - `cR_Byte += cR_Byte` 等价左移一位，给新来的位腾出 bit0；
  *        - 在 SCL 高电平期间采样 SDA（I2C 规定此时数据有效）；
  *        - 读完切回输出，把总线控制权还给主机。
  * @param bus I2C 总线对象
  * @retval 接收到的字节
  */
unsigned char IICReceiveByte(iic_bus_t *bus)
{
    unsigned char i = 8;
    unsigned char cR_Byte = 0;
    SDA_Input_Mode(bus);
    while (i--)
    {
        cR_Byte += cR_Byte;
        SCL_Output(bus,0);
				delay_us(2);
        SCL_Output(bus,1);
				delay_us(1);
        cR_Byte |=  SDA_Input(bus);
    }
    SCL_Output(bus,0);
    SDA_Output_Mode(bus);
    return cR_Byte;
}

/**
  * @brief 向指定从机的单个寄存器写一个字节
  * @note  时序：Start → 设备地址+W → ACK → 寄存器地址 → ACK → 数据 → ACK → Stop。
  *        - `daddr<<1`：daddr 是 7 位地址，左移一位、低位补 0 表示“写”；
  *        - 只检查第一个 ACK（设备地址）：失败说明器件不在总线上/总线被拉死，
  *          立刻 Stop 并返回 1；
  *        - 后两个 ACK 的返回值被忽略，所以“寄存器地址写错”本函数发现不了。
  * @param bus   I2C 总线对象
  * @param daddr 从机 7 位地址（如 EEPROM 0x50）
  * @param reg   寄存器地址
  * @param data  要写入的数据
  * @retval 0 成功；1 设备无应答
  */
uint8_t IIC_Write_One_Byte(iic_bus_t *bus, uint8_t daddr,uint8_t reg,uint8_t data)
{				   	  	    																 
  IICStart(bus);  
	
	IICSendByte(bus,daddr<<1);	    
	if(IICWaitAck(bus))	//等待应答
	{
		IICStop(bus);		 
		return 1;		
	}
	IICSendByte(bus,reg);
	IICWaitAck(bus);	   	 										  		   
	IICSendByte(bus,data);     						   
	IICWaitAck(bus);  		    	   
  IICStop(bus);
	delay_us(1);
	return 0;
}

/**
  * @brief 从指定从机的 reg 开始，连续写 length 个字节
  * @note  依赖器件的“寄存器地址自增”特性（EEPROM、AHT21 等都支持）。
  *        坑：**没有处理页边界**。BL24C02 一页只有 8 字节，一次跨页写会在
  *        页内回卷，把本页开头刚写的数据覆盖掉；所以调用方必须自己保证
  *        length ≤ 8 且不跨页（DataSave.c 每次最多写 3 字节正是为此）。
  *        另外数据阶段的 ACK 也没有检查，写失败不会被告知。
  * @param bus    I2C 总线对象
  * @param daddr  从机 7 位地址
  * @param reg    起始寄存器/存储地址
  * @param length 要写的字节数
  * @param buff   数据缓冲区
  * @retval 0 成功；1 设备无应答
  */
uint8_t IIC_Write_Multi_Byte(iic_bus_t *bus, uint8_t daddr,uint8_t reg,uint8_t length,uint8_t buff[])
{			
	unsigned char i;	
  IICStart(bus);  
	
	IICSendByte(bus,daddr<<1);	    
	if(IICWaitAck(bus))
	{
		IICStop(bus);
		return 1;
	}
	IICSendByte(bus,reg);
	IICWaitAck(bus);	
	for(i=0;i<length;i++)
	{
		IICSendByte(bus,buff[i]);     						   
		IICWaitAck(bus); 
	}		    	   
  IICStop(bus);
	delay_us(1);
	return 0;
} 

/**
  * @brief 读指定从机的一个寄存器
  * @note  先“伪写”寄存器地址，再发重复起始（Repeated Start）切到读方向：
  *        Start → 地址+W → 寄存器 → Start → 地址+R → 读 1 字节 → NACK → Stop。
  *        坑：全程不检查 ACK。器件不在线时读回的是 0xFF（SDA 被上拉），
  *        调用者无法区分“真读到 0xFF”和“总线没人应答”。
  * @param bus   I2C 总线对象
  * @param daddr 从机 7 位地址
  * @param reg   寄存器地址
  * @retval 读到的字节（失败时通常为 0xFF）
  */
unsigned char IIC_Read_One_Byte(iic_bus_t *bus, uint8_t daddr,uint8_t reg)
{
	unsigned char dat;
	IICStart(bus);
	IICSendByte(bus,daddr<<1);
	IICWaitAck(bus);
	IICSendByte(bus,reg);
	IICWaitAck(bus);
	
	IICStart(bus);
	IICSendByte(bus,(daddr<<1)+1);
	IICWaitAck(bus);
	dat = IICReceiveByte(bus);
	IICSendNotAck(bus); // 只读 1 字节，NACK 告诉从机“到此为止”
	IICStop(bus);
	return dat;
}


/**
  * @brief 从指定从机的 reg 开始连续读 length 个字节
  * @note  只有“设备地址”阶段的 ACK 会被检查；读数据阶段对除最后一个字节
  *        外的每个字节回 ACK（提示从机继续送），最后一个回 NACK 再 Stop。
  *        length=0 时循环不执行，但仍会发一次 NACK+Stop（相当于空读，
  *        不会卡死）。
  * @param bus    I2C 总线对象
  * @param daddr  从机 7 位地址
  * @param reg    起始寄存器地址
  * @param length 要读的字节数
  * @param buff   接收缓冲区（调用者保证足够大）
  * @retval 0 成功；1 设备无应答
  */
uint8_t IIC_Read_Multi_Byte(iic_bus_t *bus, uint8_t daddr, uint8_t reg, uint8_t length, uint8_t buff[])
{
	unsigned char i;
	IICStart(bus);
	IICSendByte(bus,daddr<<1);
	if(IICWaitAck(bus))
	{
		IICStop(bus);		 
		return 1;		
	}
	IICSendByte(bus,reg);
	IICWaitAck(bus);
	
	IICStart(bus);
	IICSendByte(bus,(daddr<<1)+1);
	IICWaitAck(bus);
	for(i=0;i<length;i++)
	{
		buff[i] = IICReceiveByte(bus);
		if(i<length-1)
		{IICSendAck(bus);} // 除最后一个字节外都要 ACK，从机才会继续送
	}
	IICSendNotAck(bus);
	IICStop(bus);
	return 0;
}


/**
  * @brief 软件 I2C 引脚初始化
  * @note  - SDA/SCL 都配成**推挽输出 + 上拉**（不是开漏）。所以读数据/读 ACK
  *          之前必须调 SDA_Input_Mode() 切输入，否则会与从机抢总线；
  *        - CLK_ENABLE 那行被注释掉了：GPIO 时钟由各驱动在调用本函数之前
  *          自己 __HAL_RCC_GPIOx_CLK_ENABLE()，这里不重复开；
  *        - 同一组引脚被多个驱动重复初始化是正常的（如 PB13/14 上挂了 5 个
  *          传感器），只是重复写一遍 GPIO 寄存器，不会冲突；
  *        - 小瑕疵：配置成输出的瞬间 ODR 复位值是 0，两根线会先被拉低。
  *          因为每次传输都从 Start 重新建立电平，功能上不受影响。
  * @param bus I2C 总线对象（端口 + 引脚号）
  * @retval None
  */
void IICInit(iic_bus_t *bus)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

		//bus->CLK_ENABLE();
		
    GPIO_InitStructure.Pin = bus->IIC_SDA_PIN ;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP; // 推挽输出：读之前必须切输入
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(bus->IIC_SDA_PORT, &GPIO_InitStructure);
		
		GPIO_InitStructure.Pin = bus->IIC_SCL_PIN ;
    HAL_GPIO_Init(bus->IIC_SCL_PORT, &GPIO_InitStructure);
}
