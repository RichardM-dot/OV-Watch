/*
 * ============================================================================
 * 文件名：lcd_init.c  基于 ST7789 的 SPI 驱动
 * ----------------------------------------------------------------------------
 * 说明：本文件实现 ST7789 的 GPIO/SPI 底层驱动，包括：
 *       初始化、写命令/写数据、设置显示窗口、背光 PWM 控制、睡眠唤醒等
 *       支持软件 SPI + 硬件 SPI 切换，支持 Sleep Out / 睡眠唤醒
 *       屏幕分辨率/方向等配置参考 ST7789 datasheet
 *
 * 硬件：240x280 IPS，SPI1，SCLK=PB3，MOSI=PB5，RES=PB7，CS=PB8，
 *       DC=PB9（0=命令 1=数据），背光 TIM3_CH3 PWM
 *
 * 依赖：LCD_GPIO_Init、LCD_Writ_Bus、LCD_WR_REG /
 *       LCD_WR_DATA8 / LCD_WR_DATA、LCD_Address_Set、LCD_Init 等
 *
 * 注意：
 *   1) 初始化前必须先复位，并等待足够时间
 *   2) 0x11 Sleep Out 后需等待 120 ms 再发后续命令
 *   3) 0x21 反显（IPS 屏通常需要）
 *   4) LCD_Writ_Bus 使用 HAL_SPI_Transmit 每次 1 字节，超时 1 ms
 *      若 SPI 时钟较高，需确认传输时间
 *
 * 编码：本文件注释为 GBK 编码，用 GBK 打开可正常显示
 * ============================================================================
 */

#include "lcd_init.h"
#include "delay.h"
#include "spi.h"
#include "tim.h"

/******************************************************************************
      函数名：LCD_GPIO_Init
      @brief   初始化 RES/CS/DC 引脚
      @note    RES/CS/DC 配置为推挽输出，SCLK/MOSI 由 CubeMX 的 MX_SPI1_Init() 配置
               默认电平 RES=1、CS=1、DC=1
               速度设为 VERY_HIGH，保证 SPI 时序
      @param   无
      @retval  无
******************************************************************************/
void LCD_GPIO_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure = {0};
	
 	__HAL_RCC_GPIOB_CLK_ENABLE();
	
	GPIO_InitStructure.Pin = RES_PIN|CS_PIN|DC_PIN;	 
 	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP; 		 //推挽输出
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;//高速 50MHz
 	HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);	  //初始化 GPIOB
 	HAL_GPIO_WritePin(GPIOB, RES_PIN|CS_PIN|DC_PIN, GPIO_PIN_SET); // 默认拉高，RES/CS/DC=高
}


/******************************************************************************
      函数名：LCD_Writ_Bus
      @brief   向 SPI1 写 1 字节（硬件 SPI）
      @note    使用硬件 SPI：HAL_SPI_Transmit(&hspi1, &dat, 1, 1)，超时 1 ms
               软件 SPI 版本被注释掉，可切换
               注意 CS 在 LCD_Init 中拉低，之后保持低，DC 控制命令/数据
      @param   dat  要写入的字节
      @retval  无
******************************************************************************/
void LCD_Writ_Bus(u8 dat) 
{	
	//hard SPI
	HAL_SPI_Transmit(&hspi1,&dat,1,1); // 使用 SPI 发送 1 字节，超时 1 ms
	
	//soft SPI
	/*
	u8 i;
	for(i=0;i<8;i++)
	{			  
		LCD_SCLK_Clr();
		if(dat&0x80)
		{
		   LCD_MOSI_Set();
		}
		else
		{
		   LCD_MOSI_Clr();
		}
		LCD_SCLK_Set();
		dat<<=1;
	}	
	*/
}


/******************************************************************************
      函数名：LCD_WR_DATA8
      @brief   写 1 字节数据（8 位）
      @note    调用 LCD_Writ_Bus 发送
      @param   dat 要写入的数据
      @retval  无
******************************************************************************/
void LCD_WR_DATA8(u8 dat)
{
	LCD_Writ_Bus(dat);
}


/******************************************************************************
      函数名：LCD_WR_DATA
      @brief   写 16 位数据（RGB565 颜色）
      @note    将 16 位拆成 temp[2]，用 HAL_SPI_Transmit 一次发 2 字节
               也可调用两次 LCD_Writ_Bus
      @param   dat 要写入的 16 位数据（高字节在前）
      @retval  无
******************************************************************************/
void LCD_WR_DATA(u16 dat)
{
//	LCD_Writ_Bus(dat>>8);
//	LCD_Writ_Bus(dat);
	uint8_t temp[2];
	temp[0]=(dat>>8)&0xff;
	temp[1]=dat&0xff;
	HAL_SPI_Transmit(&hspi1,temp,2,1);
	
}


/******************************************************************************
      函数名：LCD_WR_REG
      @brief   写 1 字节命令
      @note    DC=0 表示命令，DC=1 表示数据
               写命令后把 DC 拉高，方便后续写数据
      @param   dat 要写入的命令
      @retval  无
******************************************************************************/
void LCD_WR_REG(u8 dat)
{
	LCD_DC_Clr();//写命令
	LCD_Writ_Bus(dat);
	LCD_DC_Set();//写数据
}


/******************************************************************************
      函数名：LCD_Address_Set
      @brief   设置显示窗口坐标
      @note    依次写 0x2A 列地址、0x2B 行地址、0x2C 写显存
               坐标范围 x1/x2、y1/y2
               若显示偏移，在 lcd.c 中调整 OFFSET_Y=20 等
      @param   x1,x2 列起始/结束坐标
      @param   y1,y2 行起始/结束坐标
      @retval  无
******************************************************************************/
void LCD_Address_Set(u16 x1,u16 y1,u16 x2,u16 y2)
{
	LCD_WR_REG(0x2a);//列地址设置
	LCD_WR_DATA(x1);
	LCD_WR_DATA(x2);
	LCD_WR_REG(0x2b);//行地址设置
	LCD_WR_DATA(y1);
	LCD_WR_DATA(y2);
	LCD_WR_REG(0x2c);//写显存
}


/******************************************************************************
      函数名：LCD_Set_Light
      @brief   设置背光亮度（TIM3_CH3 PWM）
      @note    dc 范围 5~100，5% 是最低可见亮度
               占空比 = dc*PWM_PERIOD/100
               注意 PWM_PERIOD 是 ARR 值加 1 或 ARR，需与实际配置一致
      @param   dc 亮度百分比，5%~100%
      @retval  无
******************************************************************************/
void LCD_Set_Light(uint8_t dc)
{
	if(dc>=5 && dc<=100)
		__HAL_TIM_SetCompare(&htim3,TIM_CHANNEL_3,dc*PWM_PERIOD/100); // 把比较值写进定时器的 CCR 寄存器。真正调亮度的是 TIM3_CH3 输出的 PWM 占空比变化。
		                                                              // htim3.Instance->CCR3 = dc * PWM_PERIOD / 100;
}


/******************************************************************************
      函数名：LCD_Close_Light
      @brief   关闭背光
      @note    将 PWM 占空比设为 0，而不是停止 PWM
               停止 PWM 后再次启动需要重新 Start
      @param   无
      @retval  无
******************************************************************************/
void LCD_Close_Light(void)
{
	__HAL_TIM_SetCompare(&htim3,TIM_CHANNEL_3,0);
	//HAL_TIM_PWM_Stop(&htim3,TIM_CHANNEL_3);
}


/******************************************************************************
      函数名：LCD_Open_Light
      @brief   打开背光
      @note    启动 TIM3_CH3 PWM
               先 Start 后才能设置占空比
      @param   无
      @retval  无
******************************************************************************/
void LCD_Open_Light(void)
{
	HAL_TIM_PWM_Start(&htim3,TIM_CHANNEL_3);
}

/******************************************************************************
      函数名：LCD_ST7789_SleepIn
      @brief   ST7789 进入睡眠
      @note    发 0x10 后等待 100 ms
               睡眠后 SPI 仍可通信，需 Sleep Out 唤醒
      @param   无
      @retval  无
******************************************************************************/
void LCD_ST7789_SleepIn(void)
{
	LCD_WR_REG(0x10);
	delay_ms(100);
}


/******************************************************************************
      函数名：LCD_ST7789_SleepOut
      @brief   ST7789 退出睡眠
      @note    0x11 后需等待 120 ms（datasheet 要求）
               实际可等待 100 ms 以上再初始化
      @param   无
      @retval  无
******************************************************************************/
void LCD_ST7789_SleepOut(void)
{
	LCD_WR_REG(0x11);
	delay_ms(100);
}


/******************************************************************************
      函数名：LCD_Init
      @brief   LCD 初始化
      @note    流程：复位 → Sleep Out(0x11) 等 120ms → MADCTL(0x36) 设置方向
               → 0x3A=0x05 16 位 RGB565 → Porch/Gate/VCOM 等参数
               → Gamma → 0x21 反显 → 0x29 开显示
               - 0x36 MADCTL 由 USE_HORIZONTAL 决定 0~3
               - 0x3A=0x05 表示 16 位/像素 RGB565
               - 0xB2~0xD0 为 Porch/Gate/VCOM/Gamma 参数，按屏厂推荐
               - 0x21 反显，IPS 屏通常需要
      @param   无
      @retval  无
******************************************************************************/
void LCD_Init(void)
{
	LCD_GPIO_Init();//初始化 GPIO
	LCD_CS_Clr();		//chip select
	
	LCD_RES_Clr();	//复位
	delay_ms(100);
	LCD_RES_Set();
	delay_ms(100);
	
	// Sleep Out 后需等待 120 ms 再发后续命令
	LCD_WR_REG(0x11); 
	delay_ms(120); 
	// MADCTL，由 USE_HORIZONTAL 选择方向：0x00/0xC0/0x70/0xA0
	LCD_WR_REG(0x36); 
	if(USE_HORIZONTAL==0)LCD_WR_DATA8(0x00);
	else if(USE_HORIZONTAL==1)LCD_WR_DATA8(0xC0);
	else if(USE_HORIZONTAL==2)LCD_WR_DATA8(0x70);
	else LCD_WR_DATA8(0xA0);

	// 0x3A=0x05 = 16 位/像素 RGB565
	LCD_WR_REG(0x3A);
	LCD_WR_DATA8(0x05);

	// Porch 参数：0x0C/0x0C/0x00/0x33/0x33
	LCD_WR_REG(0xB2);
	LCD_WR_DATA8(0x0C);
	LCD_WR_DATA8(0x0C);
	LCD_WR_DATA8(0x00);
	LCD_WR_DATA8(0x33);
	LCD_WR_DATA8(0x33); 

	// Gate 电压 VGH/VGL 设置，0x35 为常用值
	LCD_WR_REG(0xB7); 
	LCD_WR_DATA8(0x35);  

	// VCOM 设置，0x19 为常用值
	LCD_WR_REG(0xBB);
	LCD_WR_DATA8(0x19);

	// LCM 设置，0x2C
	LCD_WR_REG(0xC0);
	LCD_WR_DATA8(0x2C);

	// VDV/VRH 设置，0x01
	LCD_WR_REG(0xC2);
	LCD_WR_DATA8(0x01);

	// VRH 设置
	LCD_WR_REG(0xC3);
	LCD_WR_DATA8(0x12);   

	// VDV 设置
	LCD_WR_REG(0xC4);
	LCD_WR_DATA8(0x20);  

	// 帧率设置，0x0F 约 60 Hz
	LCD_WR_REG(0xC6); 
	LCD_WR_DATA8(0x0F);    

	// 电源设置，0xA4/0xA1
	LCD_WR_REG(0xD0); 
	LCD_WR_DATA8(0xA4);
	LCD_WR_DATA8(0xA1);

	// 正 Gamma，14 字节
	LCD_WR_REG(0xE0);
	LCD_WR_DATA8(0xD0);
	LCD_WR_DATA8(0x04);
	LCD_WR_DATA8(0x0D);
	LCD_WR_DATA8(0x11);
	LCD_WR_DATA8(0x13);
	LCD_WR_DATA8(0x2B);
	LCD_WR_DATA8(0x3F);
	LCD_WR_DATA8(0x54);
	LCD_WR_DATA8(0x4C);
	LCD_WR_DATA8(0x18);
	LCD_WR_DATA8(0x0D);
	LCD_WR_DATA8(0x0B);
	LCD_WR_DATA8(0x1F);
	LCD_WR_DATA8(0x23);

	// 负 Gamma，14 字节，与 0xE0 对应
	LCD_WR_REG(0xE1);
	LCD_WR_DATA8(0xD0);
	LCD_WR_DATA8(0x04);
	LCD_WR_DATA8(0x0C);
	LCD_WR_DATA8(0x11);
	LCD_WR_DATA8(0x13);
	LCD_WR_DATA8(0x2C);
	LCD_WR_DATA8(0x3F);
	LCD_WR_DATA8(0x44);
	LCD_WR_DATA8(0x51);
	LCD_WR_DATA8(0x2F);
	LCD_WR_DATA8(0x1F);
	LCD_WR_DATA8(0x1F);
	LCD_WR_DATA8(0x20);
	LCD_WR_DATA8(0x23);

	// 反显，IPS 屏通常需要
	LCD_WR_REG(0x21); 

	// 开显示
	LCD_WR_REG(0x29); 
}