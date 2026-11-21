/*
 * ============================================================================
 * 【中文导读】SPL06_001.c —— 气压高度计 SPL06-001 驱动（B级导读）
 * ----------------------------------------------------------------------------
 * 器件挂在 PB13/PB14 的软件 I2C 上，7 位地址 0x76，ID 寄存器 0x0D 应为 0x10。
 * 它输出 24 位有符号的原始温度/气压，但**原始值不能直接用**：每颗芯片出厂
 * 都有一组校准系数（c0/c1/c00/c10/c01/c11/c20/c21/c30），必须按数据手册
 * 的多项式补偿后才能得到真实的温度(℃)、气压(Pa)和高度(m)。
 *
 * 阅读顺序：
 *   SPL_init（配置 + 读 9 个系数）
 *   → Get_Traw/Get_Praw（拼 24 位原始值）
 *   → get_c0...get_c30（拼系数，注意半字节共享和符号扩展）
 *   → Temperature_Calculate / Pressure_Calculate / Altitude_Calculate。
 *
 * 几个关键点：
 *   - 系数不是按字节对齐的：c0/c1 共享寄存器 0x11，c00/c10 共享 0x15，
 *     拼接时要取高/低半字节（见各 get_cX 的注释）；
 *   - 24 位和 12/20 位都是有符号数，最高位为 1 时要手工做符号扩展
 *     （`|= 0xFF000000` / `|= 0xF000` 这些行），漏了会得到巨大的正数；
 *   - KT=524288=2^19、KP=1572864=3*2^19 是手册给定的标度因子，
 *     对应“过采样倍率”带来的移位，不能随便改；
 *   - 校准系数放在全局变量里，只在 SPL_init 读一次，之后计算直接复用；
 *   - 读原始值是 3 次独立的单字节 I2C 读，不是突发读，转换刚好在两次读
 *     之间完成时可能拼出“跨帧”的数据（撕裂），本项目没有做一致性检查。
 * ============================================================================
 */

#include <math.h>
#include "SPL06_001.h"

#define SPL_CLK_ENABLE __HAL_RCC_GPIOB_CLK_ENABLE()
iic_bus_t SPL_bus = 
{
	.IIC_SDA_PORT = GPIOB,
	.IIC_SCL_PORT = GPIOB,
	.IIC_SDA_PIN  = GPIO_PIN_13,
	.IIC_SCL_PIN  = GPIO_PIN_14,
};

int16_t c0,c1,c01,c11,c20,c21,c30;
int32_t c00,c10;

/**
  * @brief  读一个寄存器
  * @param  addr 寄存器地址
  * @retval 寄存器值（器件不应答时底层返回 0xFF）
  */
uint8_t SPL_ReadOneReg(uint8_t addr)
{
	uint8_t dat;
	dat = IIC_Read_One_Byte(&SPL_bus, SPL_CHIP_ADDRESS,addr);
	return dat;
}

/**
  * @brief  写一个寄存器
  * @param  addr 寄存器地址
  * @param  dat  写入值
  * @retval 无（底层写失败不会返回错误）
  */
void SPL_WriteOneReg(uint8_t addr, uint8_t dat)
{
	IIC_Write_One_Byte(&SPL_bus, SPL_CHIP_ADDRESS,addr,dat);
}

/**
  * @brief  读 24 位温度原始值 Traw
  * @param  无
  * @retval Traw（有符号 24 位，已做符号扩展）
  * @note   寄存器 0x03/0x04/0x05 依次是 B0/B1/B2，**小端**：
  *         Traw = B2<<16 | B1<<8 | B0。
  *         bit23 为 1 表示负数，需要 `|= 0xFF000000` 手工补高 8 位，
  *         否则 int32 会把它当成 0x008xxxxx 的大正数。
  */
int32_t Get_Traw()
{
	uint8_t buff[3];
	int32_t Traw;
	buff[0] = SPL_ReadOneReg(SPL_TMP_B0);
	buff[1] = SPL_ReadOneReg(SPL_TMP_B1);
	buff[2] = SPL_ReadOneReg(SPL_TMP_B2);
	Traw = buff[2];
	Traw = Traw << 8 | buff[1];
	Traw = Traw << 8 | buff[0];
	if(Traw & (1<<23))
	{Traw |= 0xFF000000;}//24 bit 2´s complement numbers
	return Traw;
}

/**
  * @brief  读 24 位气压原始值 Praw
  * @param  无
  * @retval Praw（有符号 24 位，已做符号扩展）
  * @note   寄存器 0x00/0x01/0x02 = B0/B1/B2，同样小端、同样要符号扩展。
  */
int32_t Get_Praw()
{
	uint8_t buff[3];
	int32_t Praw;
	buff[0] = SPL_ReadOneReg(SPL_PRS_B0);
	buff[1] = SPL_ReadOneReg(SPL_PRS_B1);
	buff[2] = SPL_ReadOneReg(SPL_PRS_B2);
	Praw = buff[2];
	Praw = Praw << 8 | buff[1];
	Praw = Praw << 8 | buff[0];
	if(Praw & (1<<23))
	{Praw |= 0xFF000000;}//24 bit 2´s complement numbers
	return Praw;
}

/**
  * @brief  读校准系数 c0（12 位有符号）
  * @param  无
  * @retval c0
  * @note   c0 的高 8 位在 COEF_C0(0x10)，低 4 位在 COEF_C0_C1(0x11) 的**高半字节**：
  *         c0 = (buf0 << 4) | (buf1 >> 4)。
  *         bit11 是符号位，`c0 |= 0xF000` 手工做符号扩展。
  */
int16_t get_c0()
{
	uint8_t buff[2];
	int16_t c0;
	buff[0] = SPL_ReadOneReg(COEF_C0);
	buff[1] = SPL_ReadOneReg(COEF_C0_C1);
	c0 = buff[0];
	c0 = (c0 << 4) | (buff[1] >> 4);
	if(c0 & (1<<11))
	{c0 |= 0xF000;}
	return c0;
}

/**
  * @brief  读校准系数 c1（12 位有符号）
  * @param  无
  * @retval c1
  * @note   和 c0 共用 0x11：c1 的高 4 位是 COEF_C0_C1 的**低半字节**，
  *         低 8 位在 COEF_C1(0x12)：
  *         c1 = ((buf0 & 0x0F) << 8) | buf1。同样 12 位符号扩展。
  */
int16_t get_c1()
{
	uint8_t buff[2];
	int16_t c1;
	buff[0] = SPL_ReadOneReg(COEF_C0_C1);
	buff[1] = SPL_ReadOneReg(COEF_C1);
	c1 = buff[0] & 0x0F;
	c1 = (c1 << 8) | buff[1] ;
	if(c1 & (1<<11))
	{c1 |= 0xF000;}
	return c1;
}

/**
  * @brief  读校准系数 c00（20 位有符号）
  * @param  无
  * @retval c00
  * @note   20 位被拆成 8+8+4：COEF_C00_H(0x13) 高 8 位、COEF_C00_L(0x14)
  *         中间 8 位、COEF_C00_C10(0x15) 的高半字节是低 4 位。
  *         所以先拼 16 位再 `<<4`，最后接半字节。bit19 符号 → |= 0xFFF00000。
  */
int32_t get_c00()
{
	uint8_t buff[3];
	int32_t c00;
	buff[0] = SPL_ReadOneReg(COEF_C00_H);
	buff[1] = SPL_ReadOneReg(COEF_C00_L);
	buff[2] = SPL_ReadOneReg(COEF_C00_C10);
	c00 = buff[0];
	c00 = c00<<8 | buff[1];
	c00 = (c00<<4) | (buff[2]>>4);
	if(c00 & (1<<19))
	{c00 |= 0xFFF00000;}
	return c00;
}

/**
  * @brief  读校准系数 c10（20 位有符号）
  * @param  无
  * @retval c10
  * @note   c10 的高 4 位是 COEF_C00_C10(0x15) 的**低半字节**，中间 8 位在
  *         COEF_C10_M(0x16)，低 8 位在 COEF_C10_L(0x17)。
  *         同样 20 位、同样符号扩展。
  */
int32_t get_c10()
{
	uint8_t buff[3];
	int32_t c10;
	buff[0] = SPL_ReadOneReg(COEF_C00_C10);
	buff[1] = SPL_ReadOneReg(COEF_C10_M);
	buff[2] = SPL_ReadOneReg(COEF_C10_L);
	c10 = buff[0] & 0x0F;
	c10 = c10<<8 | buff[1];
	c10 = c10<<8 | buff[2];
	if(c10 & (1<<19))
	{c10 |= 0xFFF00000;}
	return c10;
}

/**
  * @brief  读校准系数 c01（16 位有符号）
  * @param  无
  * @retval c01
  * @note   COEF_C01_H/L(0x18/0x19) 直接拼成 16 位。
  *         这里不需要手工符号扩展：bit15 正好是 int16_t 的符号位，
  *         函数返回时由 int32_t 截断成 int16_t 自动完成。
  */
int16_t get_c01()
{
	uint8_t buff[2];
	int32_t c01;
	buff[0] = SPL_ReadOneReg(COEF_C01_H);
	buff[1] = SPL_ReadOneReg(COEF_C01_L);
	c01 = buff[0];
	c01 = c01<<8 | buff[1];
	return c01;
}

/**
  * @brief  读校准系数 c11（16 位有符号）
  * @param  无
  * @retval c11
  * @note   COEF_C11_H/L(0x1A/0x1B) 直接拼，符号位由 int16_t 截断处理。
  */
int16_t get_c11()
{
	uint8_t buff[2];
	int32_t c11;
	buff[0] = SPL_ReadOneReg(COEF_C11_H);
	buff[1] = SPL_ReadOneReg(COEF_C11_L);
	c11 = buff[0];
	c11 = c11<<8 | buff[1];
	return c11;
}

/**
  * @brief  读校准系数 c20（16 位有符号）
  * @param  无
  * @retval c20
  * @note   COEF_C20_H/L(0x1C/0x1D) 直接拼，用于气压多项式的高阶项。
  */
int16_t get_c20()
{
	uint8_t buff[2];
	int32_t c20;
	buff[0] = SPL_ReadOneReg(COEF_C20_H);
	buff[1] = SPL_ReadOneReg(COEF_C20_L);
	c20 = buff[0];
	c20 = c20<<8 | buff[1];
	return c20;
}

/**
  * @brief  读校准系数 c21（16 位有符号）
  * @param  无
  * @retval c21
  * @note   COEF_C21_H/L(0x1E/0x1F) 直接拼，温度×气压的交叉项系数。
  */
int16_t get_c21()
{
	uint8_t buff[2];
	int32_t c21;
	buff[0] = SPL_ReadOneReg(COEF_C21_H);
	buff[1] = SPL_ReadOneReg(COEF_C21_L);
	c21 = buff[0];
	c21 = c21<<8 | buff[1];
	return c21;
}

/**
  * @brief  读校准系数 c30（16 位有符号）
  * @param  无
  * @retval c30
  * @note   COEF_C30_H/L(0x20/0x21) 直接拼，气压三次项系数。
  */
int16_t get_c30()
{
	uint8_t buff[2];
	int32_t c30;
	buff[0] = SPL_ReadOneReg(COEF_C30_H);
	buff[1] = SPL_ReadOneReg(COEF_C30_L);
	c30 = buff[0];
	c30 = c30<<8 | buff[1];
	return c30;
}

/**
  * @brief  初始化 SPL06-001：配置测量模式并读取出厂校准系数
  * @param  无
  * @retval 0=成功；1=回读配置不一致（器件不在线/写入失败）
  * @note   配置：
  *         - PRS_CFG=0x01：气压 2 倍过采样；
  *         - TMP_CFG=0x80：使用外部温度传感器（SPL06 自身测温），1 倍过采样；
  *         - MEAS_CFG=0x07：气压+温度连续测量；
  *         - CFG_REG=0x00：中断/滤波相关全关。
  *         配置完立刻读 9 个校准系数存到全局变量——系数每颗芯片不同，
  *         必须在 init 时读，不能写死。
  *         最后的校验只回读 PRS_CFG 和 CFG_REG 两个寄存器，**没有验证
  *         系数是否正确**，所以器件半坏时也可能返回 0。
  */
uint8_t SPL_init()
{
	
	SPL_CLK_ENABLE;
	IICInit(&SPL_bus);
	
	SPL_WriteOneReg(SPL_PRS_CFG, 0x01);		// Pressure 2x oversampling

	SPL_WriteOneReg(SPL_TMP_CFG, 0x80);		// External Temperature 1x oversampling

	SPL_WriteOneReg(SPL_MEAS_CFG, 0x07);	// continuous pressure and temperature measurement

	SPL_WriteOneReg(SPL_CFG_REG, 0x00);		//   
	
	c0 = get_c0();
	c1 = get_c1();
	c01 = get_c01();
	c11 = get_c11(); 
	c20 = get_c20();
	c21 = get_c21();
	c30 = get_c30();
	c00 = get_c00();
	c10 = get_c10();
	
	if(SPL_ReadOneReg(SPL_PRS_CFG)!=0x01 || SPL_ReadOneReg(SPL_CFG_REG)!=0x00)
	{return 1;}//ERRO
	else 
	{return 0;}//SUCCESS
}

/**
  * @brief  让传感器进入待机（低功耗）
  * @param  无
  * @retval 无
  * @note   MEAS_CFG=0x00 停止连续测量；校准系数仍保存在全局变量里，
  *         唤醒后不用重新读。下面注释掉的 RESET 是“彻底复位”，
  *         真复位后必须重新 SPL_init()。
  */
void SPL_Sleep()
{
	SPL_WriteOneReg(SPL_MEAS_CFG, 0x00);	// standby
	//SPL_WriteOneReg(SPL_RESET_REG,0x09);//reset
}
	
/**
  * @brief  唤醒：重新进入气压+温度连续测量
  * @param  无
  * @retval 无
  * @note   只恢复 MEAS_CFG，不重读系数（系数还在内存里）。
  *         若传感器断过电，必须先调 SPL_init() 重新读系数，否则算出来的
  *         温压全是错的——这也是下面那行 SPL_init() 被注释掉的原因：
  *         本项目的休眠只断测量、不断传感器电源。
  */
void SPL_Wakeup()
{
	SPL_WriteOneReg(SPL_MEAS_CFG, 0x07);	// continuous pressure and temperature measurement
	//SPL_init();
}

/**
  * @brief  读芯片 ID
  * @param  无
  * @retval ID 寄存器(0x0D) 的值，正常应为 0x10
  * @note   本工程没有在 SPL_init 里校验 ID，相当于少了一道在线检测。
  */
uint8_t SPL_GetID()
{
	return SPL_ReadOneReg(SPL_ID_REG);
}

/**
  * @brief  由原始温度计算真实温度
  * @param  无
  * @retval 温度（℃）
  * @note   Traw_sc = Traw / KT，KT=524288=2^19，对应 1 倍过采样的标度；
  *         再套手册公式 Tcomp = c0*0.5 + c1*Traw_sc。
  *         系数是全局变量，必须先跑过 SPL_init()。
  */
float Temperature_Calculate()
{
	float Traw_sc, Tcomp;
	Traw_sc = Get_Traw();
	Traw_sc /= KT;
	Tcomp = c0 * 0.5 + c1 * Traw_sc;
	return Tcomp;
}

/**
  * @brief  由原始气压计算真实气压
  * @param  无
  * @retval 气压（Pa）
  * @note   Praw_sc = Praw / KP，KP=1572864=3*2^19（对应 2 倍过采样 + 3 的
  *         标度因子）。补偿多项式就是手册里的：
  *         Pcomp = c00 + Praw*(c10 + Praw*(c20 + Praw*c30))
  *                      + Traw*c01 + Traw*Praw*(c11 + Praw*c21)
  *         这里用 Horner 形式展开，减少乘法次数。
  *         注意 KP 与 PRS_CFG 的过采样倍率是绑定的：改了 SPL_init 里的
  *         过采样配置，KP 也必须跟着改，否则气压会差一个固定倍数。
  */
float Pressure_Calculate()
{
	float Traw_sc, Praw_sc, Pcomp;
	Traw_sc = Get_Traw();
	Traw_sc /= KT;
	Praw_sc = Get_Praw();
	Praw_sc /= KP;
	Pcomp = (c00) + Praw_sc * ((c10) + Praw_sc * ((c20) + Praw_sc * (c30))) + Traw_sc * (c01) + Traw_sc * Praw_sc * ((c11) + Praw_sc * (c21)); // 手册补偿多项式（Horner 展开）
	 
	return Pcomp;
}

/**
  * @brief  由气压换算海拔高度
  * @param  无
  * @retval 高度（m）
  * @note   国际标准大气公式：h = 44330 * (1 - (P/P0)^0.1903)，
  *         P0=101325 Pa 是海平面标准气压，44330 m 是对应的大气层基准高度。
  *         用的是绝对气压，所以天气变化会让绝对高度漂移几十米；
  *         要测“相对高度变化”（爬楼）应记录基准气压再做差。
  *         powf 是浮点运算，F411 有 FPU，开销可以接受。
  */
float Altitude_Calculate()
{
	float Altitude;
	Altitude = 44330 * (1 - powf(Pressure_Calculate()/101325, 0.1903)); // 101325Pa=海平面标准气压
	return Altitude;
}
