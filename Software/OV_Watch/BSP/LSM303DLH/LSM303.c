/*
 * ============================================================================
 * 【中文导读】LSM303.c —— LSM303DLH 电子罗盘（加速度 + 磁力计，B级导读）
 * ----------------------------------------------------------------------------
 * LSM303DLH 是一颗“二合一”器件：3 轴加速度计 + 3 轴磁力计，但**两个功能
 * 挂在两个不同的 I2C 从机地址上**：
 *   - 加速度计：7 位地址 0x19（写 0x32）；
 *   - 磁力计/温度：7 位地址 0x1E（写 0x3C）。
 * 所以本文件的读写函数都靠“寄存器号”判断该用哪个地址：LSM303DLH 的寄存器
 * 布局里 0x00~0x19 属于磁力计，0x20 以上属于加速度计，于是
 * `(RegAddr > 0x19) ? A : M`。这是个巧妙的 hack，但也很脆弱——温度寄存器
 * 0x31 恰好大于 0x19，所以温度必须走专门的 LSM303_Temp_ReadOneReg()。
 *
 * 另一个器件特性：**不支持连续读**（实测），所以 LSM303_ReadMultiReg 只能
 * 一个寄存器一个寄存器地读，不能用 IIC_Read_Multi_Byte。
 *
 * 阅读顺序：ReadOneReg/ReadMultiReg/WriteOneReg（地址选择）
 *           → LSM303DLH_Init（量程/速率/连续转换 + 回读校验）
 *           → ReadAcceleration/ReadMagnetic/ReadTemperature（字节序不同！）
 *           → 两个倾角计算 → Azimuth_Calculate（倾斜补偿罗盘）。
 *
 * 字节序的坑（最容易错）：
 *   - 加速度输出是**小端**：先低字节 OUT_X_L_A，所以 buff[1]<<8 | buff[0]；
 *   - 磁力输出是**大端**：先高字节 OUT_X_H_M，所以 buff[0]<<8 | buff[1]；
 *   - 磁力的数据顺序是 X、**Z**、Y（不是 X、Y、Z），代码里专门换过位置。
 * ============================================================================
 */

#include "LSM303.h"
#include <math.h>

#define PI 3.1415926
#define Delayms(X) HAL_Delay(X)

#define LSM303_CLK_ENABLE __HAL_RCC_GPIOB_CLK_ENABLE()
iic_bus_t LSM303_bus = 
{
	.IIC_SDA_PORT = GPIOB,
	.IIC_SCL_PORT = GPIOB,
	.IIC_SDA_PIN  = GPIO_PIN_13,
	.IIC_SCL_PIN  = GPIO_PIN_14,
};

/************************************************************************************************************************
NOTICE:	the LSM303 can not use mutiple read!

*************************************************************************************************************************/


/*************************************************************************************************************************
*function  	:	LSM303_ReadOneReg()
*paraments	: RegAddr
*return     :	data readed
*detail     :	
*************************************************************************************************************************/
/**
  * @brief  读一个寄存器（自动选择加速度计/磁力计地址）
  * @param  RegAddr 寄存器地址
  * @retval 寄存器值；器件不应答时底层返回 0xFF
  * @note   `RegAddr > 0x19` 是这颗芯片的地址分界：小于等于 0x19 走磁力计
  *         0x1E，大于走加速度计 0x19。温度寄存器 0x31 会被误判成加速度计，
  *         所以温度不能用这个函数。
  */
unsigned char LSM303_ReadOneReg(unsigned char RegAddr)
{
	unsigned char dat;
	unsigned char SlaveAddr = (RegAddr>0x19)?LSM303_SlaveAddr_A:LSM303_SlaveAddr_M;
	dat = IIC_Read_One_Byte(&LSM303_bus, SlaveAddr,RegAddr);
	return dat;
}

/*************************************************************************************************************************
*function  	:	LSM303_ReadMultiReg()
*paraments	: RegAddr, Regnumber, Databuff
*return     :	multiple datas readed
*detail     :	can't use real continue read, we tested the LSM303 is not allowed, so we have to use multiple ReadOneReg
*************************************************************************************************************************/
/**
  * @brief  连续读多个寄存器（逐个读，不是突发读）
  * @param  RegAddr 起始寄存器地址
  * @param  RegNum  要读的个数
  * @param  DataBuff 输出缓冲区
  * @retval 无
  * @note   为什么不用 IIC_Read_Multi_Byte：作者实测 LSM303 不支持连续读
  *         （自动地址递增会读回重复/错位数据），只能每个寄存器单独起一次
  *         传输。代价是 6 个字节要 6 次 Start/Stop，速度慢很多。
  *         地址同样按首寄存器判断 A/M；整段必须落在同一个器件内。
  */
void LSM303_ReadMultiReg(unsigned char RegAddr, unsigned char RegNum, unsigned char DataBuff[])
{
	unsigned char i;
	unsigned char SlaveAddr = (RegAddr>0x19)?LSM303_SlaveAddr_A:LSM303_SlaveAddr_M;
	//IIC_Read_Multi_Byte(SlaveAddr,RegAddr,RegNum,DataBuff);
	for(i=0;i<RegNum;i++)
	{
		DataBuff[i] = IIC_Read_One_Byte(&LSM303_bus, SlaveAddr,RegAddr+i);
	}
}

/*************************************************************************************************************************
*function  	:	LSM303_Temp_ReadOneReg()
*paraments	: RegAddr
*return     : data readed
*detail     :	it's for temperature data read
*************************************************************************************************************************/
/**
  * @brief  读温度寄存器（强制走磁力计地址）
  * @param  RegAddr 寄存器地址（TEMP_OUT_H_M=0x31 / TEMP_OUT_L_M=0x32）
  * @retval 寄存器值
  * @note   温度寄存器地址 > 0x19，用通用函数会被误判成加速度计地址
  *         而读不到，所以单独提供这个“永远用 M 地址”的版本。
  */
unsigned char LSM303_Temp_ReadOneReg(unsigned char RegAddr)
{
	unsigned char dat;
	dat = IIC_Read_One_Byte(&LSM303_bus, LSM303_SlaveAddr_M,RegAddr);
	return dat;
}

/*************************************************************************************************************************
*function  	:	LSM303_WriteOneReg()
*paraments	: RegAddr, data
*return     : None
*detail     :	
*************************************************************************************************************************/
/**
  * @brief  写一个寄存器（自动选择加速度计/磁力计地址）
  * @param  RegAddr 寄存器地址
  * @param  dat     写入值
  * @retval 无
  * @note   地址判断规则同 LSM303_ReadOneReg。底层写失败不会返回错误，
  *         所以 Init 里要靠“写完再回读”来确认配置真的生效。
  */
void LSM303_WriteOneReg(unsigned char RegAddr, unsigned char dat)
{
	unsigned char SlaveAddr = (RegAddr>0x19)?LSM303_SlaveAddr_A:LSM303_SlaveAddr_M;
	IIC_Write_One_Byte(&LSM303_bus, SlaveAddr,RegAddr,dat);
}

/*************************************************************************************************************************
*function  	:	LSM303DLH_Init()
*paraments	: None
*return     : succeed or not
*detail     :	the defalt CTRL_REG4 value is 0x00. FS[5:4], 00: ±2 g, 01: ±4 g, 10: ±8 g, 11: ±16 g
*************************************************************************************************************************/
/**
  * @brief  初始化加速度计和磁力计（量程、速率、连续转换）
  * @param  无
  * @retval 0=成功；1=配置回读不一致（重试 3 次仍失败）
  * @note   加速度计：
  *         - CTRL_REG4_A=0x10：FS[5:4]=01 → ±4g；
  *         - CTRL_REG1_A=0x2F：ODR=0010 → 10Hz，LPen=1 低功耗，XYZ 全开。
  *         磁力计：
  *         - CRA_REG_M=0x10：15Hz 数据输出；
  *         - CRB_REG_M=0x80：±4.0 Gauss（XY 450 LSB/Gauss，Z 400 LSB/Gauss）；
  *         - MR_REG_M=0x00：连续转换模式。
  *         每步都 Delayms(1) 再回读：写配置后器件需要时间生效；不一致就
  *         延时 10ms 重试，最多 3 次。temp 在循环里赋值、循环外判断，
  *         所以“一次就成功”和“三次都失败”两条路径都有确定值。
  */
unsigned char LSM303DLH_Init()
{
	unsigned char temp;
	unsigned char retry = 0;
	
	LSM303_CLK_ENABLE;
	IICInit(&LSM303_bus);
	
	for(retry = 0;retry < 3;retry ++)
	{
		LSM303_WriteOneReg(LSM303_CTRL_REG4_A, 0x10);		//full scale: +-4g
		Delayms(1);
		LSM303_WriteOneReg(LSM303_CTRL_REG1_A, 0x2F);		//low power mode, 10Hz速度
		Delayms(1);															
		temp = LSM303_ReadOneReg(LSM303_CTRL_REG1_A);		//读取0x20寄存器，默认值为0x07
		if(temp != 0x2F)	
		{
			Delayms(10);	
		}
		else break;
	}
	if(temp != 0x2F)
	{
		return 1;
	}
	Delayms(1);									
	for(retry = 0;retry < 3;retry ++)
	{
		LSM303_WriteOneReg(LSM303_CRA_REG_M, 0x10);		//磁场传感器15Hz,正常测量;关闭温度计
		Delayms(1);		
		LSM303_WriteOneReg(LSM303_CRB_REG_M, 0x80);		//磁场传感器gain默认为0x80: +-4.0Gauss, 450LBS/Gauss for XY, 400LBS/Gauss for Z
		Delayms(1);	
		LSM303_WriteOneReg(LSM303_MR_REG_M, 0x00);		//磁场传感器连续转换模式
		Delayms(1);										
		temp = LSM303_ReadOneReg(LSM303_MR_REG_M);		
		if(temp != 0)	
		{
			Delayms(10);	
		}
		else break;
	}
	if(temp != 0)	
	{
		return 1;	
	}
	return 0;
}

/*************************************************************************************************************************
*function  	:	LSM303DLH_Sleep()
*paraments	: None
*return     : None
*detail     :	
*************************************************************************************************************************/
/**
  * @brief  睡眠：磁力计进睡眠、加速度计进掉电模式
  * @param  无
  * @retval 无
  * @note   MR_REG_M=0x03 是磁力计 sleep 模式；
  *         CTRL_REG1_A=0x0F 的高 4 位 ODR=0000 = power-down，低 4 位保留
  *         LPen 和 XYZ 使能位。所以这颗芯片“睡着”时配置还在，唤醒只需
  *         把 ODR 和磁力计模式写回去。
  */
void LSM303DLH_Sleep()
{
	LSM303_WriteOneReg(LSM303_MR_REG_M,0x03);
	LSM303_WriteOneReg(LSM303_CTRL_REG1_A,0x0f);		
}

/*************************************************************************************************************************
*function  	:	LSM303DLH_Wakeup()
*paraments	: None
*return     : None
*detail     :	
*************************************************************************************************************************/
/**
  * @brief  唤醒：恢复磁力计连续转换 + 加速度计 10Hz 低功耗
  * @param  无
  * @retval 无
  * @note   只写两个模式寄存器，量程/增益等配置在睡眠期间没有丢，
  *         所以不需要重新跑 LSM303DLH_Init()。
  */
void LSM303DLH_Wakeup()
{
	LSM303_WriteOneReg(LSM303_MR_REG_M,0x00);//磁场传感器连续转换模式
	LSM303_WriteOneReg(LSM303_CTRL_REG1_A,0x2f);//low power mode, 10Hz速度	
}

/*************************************************************************************************************************
*function  	:	LSM303_ReadAcceleration()
*paraments	: int16_t *Xa, int16_t *Ya, int16_t *Z
*return     : None
*detail     :	to read the Acceleration data, but the real Acceleration should be divided by the unit(n LSB/A) 
*************************************************************************************************************************/
/**
  * @brief  读三轴加速度原始值
  * @param  Xa,Ya,Za 输出：三轴原始码值（有符号 16 位）
  * @retval 无
  * @note   加速度输出是**小端**：OUT_X_L_A 在前、OUT_X_H_A 在后，
  *         所以 temp = buff[1]<<8 | buff[0]。
  *         这里只给原始码值，要换算成 g 需除以灵敏度（±4g 时约 134 LSB/g，
  *         以数据手册为准），调用者自己处理。
  *         读 6 字节实际是 6 次单字节读（器件不支持连续读）。
  */
void LSM303_ReadAcceleration(int16_t *Xa, int16_t *Ya, int16_t *Za)
{
	uint8_t buff[6];
	int16_t temp;
	LSM303_ReadMultiReg(LSM303_OUT_X_L_A,6,buff);
	
	temp = buff[1];
	temp <<= 8;
	temp |= buff[0];
	*Xa = temp;
	
	temp = buff[3];
	temp <<= 8;
	temp |= buff[2];
	*Ya = temp;
	
	temp = buff[5];
	temp <<= 8;
	temp |= buff[4];
	*Za = temp;
}

/*************************************************************************************************************************
*function  	:	LSM303_ReadMagnetic()
*paraments	: int16_t *Xm, int16_t *Ym, int16_t *Zm
*return     : None
*detail     :	to read the Magnetic data, but the real Magnetic should be divided by the unit(n LSB/M) 
*************************************************************************************************************************/
/**
  * @brief  读三轴磁场原始值
  * @param  Xm,Ym,Zm 输出：三轴原始码值（有符号 16 位）
  * @retval 无
  * @note   两个坑叠在一起：
  *         1) 磁力输出是**大端**（OUT_X_H_M 在前），所以 buff[0]<<8 | buff[1]，
  *            和加速度正好相反；
  *         2) 数据顺序是 X、**Z**、Y，不是 X、Y、Z，所以代码里
  *            buff[2..3] 给 Zm、buff[4..5] 给 Ym。
  *         ±4Gauss 时 XY 约 450 LSB/Gauss、Z 约 400 LSB/Gauss。
  */
void LSM303_ReadMagnetic(int16_t *Xm, int16_t *Ym, int16_t *Zm)
{
	uint8_t buff[6];
	int16_t temp;
	LSM303_ReadMultiReg(LSM303_OUT_X_H_M,6, buff);
	
	temp = buff[0];
	temp <<= 8;
	temp |= buff[1];
	*Xm = temp;
	
	// 注意：磁力数据顺序是 X、Z、Y，buff[2..3] 其实是 Z
	temp = buff[2];
	temp <<= 8;
	temp |= buff[3];
	*Zm = temp;
	
	temp = buff[4];
	temp <<= 8;
	temp |= buff[5];
	*Ym = temp;
}

/*************************************************************************************************************************
*function  	:	LSM303_ReadTemperature()
*paraments	: int16_t *Temp
*return     : None
*detail     :	to read the Temperature data, 
							the real Temperature already be divided by 8(8 LSB/deg)(not guaranteed, as the datasheet said) 
*************************************************************************************************************************/
/**
  * @brief  读温度
  * @param  Temp 输出：温度（℃）
  * @retval 无
  * @note   0x31/0x32 是 12 位、左对齐的温度值，代码先拼 16 位再 >>4 取高 12 位。
  *         疑点：注释写的是 8 LSB/℃，但实际做的是 `/3 + 0.5`，与 8 LSB/℃
  *         对不上（按手册应 /8），这个系数像是实测凑出来的，读数是否准确
  *         存疑，只适合看趋势。
  *         另外这是全文件唯一一处用 IIC_Read_Multi_Byte 的地方，和“LSM303
  *         不支持连续读”的结论矛盾——能工作可能只是温度寄存器的特例。
  */
void LSM303_ReadTemperature(int16_t *Temp)
{
	uint8_t buff[2];
	int16_t temp;
	IIC_Read_Multi_Byte(&LSM303_bus, LSM303_SlaveAddr_M, TEMP_OUT_H_M, 2, buff);
	temp = buff[0];
	temp <<= 8;
	temp |= buff[1];
	temp >>= 4;
	*Temp = temp / 3 + 0.5;//8 LSB/deg
}

/*************************************************************************************************************************
*function  	:	LSM303DLH_CalculationZAxisAngle()
*paraments	: int16_t Xa, int16_t Ya, int16_t Za
*return     : ZAxisAngle
*detail     :	
*************************************************************************************************************************/
/**
  * @brief  用重力矢量算“与 Z 轴的夹角”（表盘倾斜角）
  * @param  Xa,Ya,Za 三轴加速度原始值
  * @retval 角度 * 100（定点，单位 0.01°）
  * @note   原理：静止时加速度矢量就是重力方向。先求模长 |A| 做归一化，
  *         再用 atan(sqrt(fx²+fy²)/fz) 得到与 Z 轴的夹角（0~90°）。
  *         边界：fz=0 时除零得 inf，atan(inf)=90°，结果仍合理；
  *         但 A=0（三轴全 0，器件没数据）时 fx/fy/fz 都是 NaN，结果无意义。
  *         返回前 *100 是为了避免上层用浮点。
  */
int LSM303DLH_CalculationZAxisAngle(int16_t Xa, int16_t Ya, int16_t Za)
{
	double A;
	float fx,fy,fz;
	
	A = sqrt((int)Xa*Xa + (int)Ya*Ya + (int)Za*Za);	//计算角加速度的矢量模长 |A|=根号下(X*X+Y*Y+Z*Z)
	fx = Xa/A;
	fy = Ya/A;
	fz = Za/A;
	
	
	//Z方向
	A = fx*fx+fy*fy;
	A = sqrt(A);
	A = (double)A/fz;
	A = atan(A); 
	A = A*180/PI;
	if(A < 0)
	{
		A += 90;
		A = 0-A;
	}
	else
	{
		A = 90-A;
	}
	return A*100;
}
 
 
/*************************************************************************************************************************
*function  	:	LSM303DLH_CalculationZAxisAngle()
*paraments	: int16_t Xa, int16_t Ya, int16_t Za
*return     : XAxisAngle
*detail     :	
*************************************************************************************************************************/
/**
  * @brief  用重力矢量算“与 X 轴的夹角”（表盘前后倾角）
  * @param  Xa,Ya,Za 三轴加速度原始值
  * @retval 角度 * 100（定点，带符号：向上为正、向下为负）
  * @note   和 Z 轴版本同一套推导，只是把分子换成 sqrt(fz²+fy²)、分母换成 fx。
  *         上面英文块里的函数名写成了 CalculationZAxisAngle，是复制粘贴
  *         遗留，实际算的是 X 轴。
  *         符号处理：atan 结果为负时 +90（向上），为正时 90-θ 再取负
  *         （向下），这样 0° 对应水平、正值抬头、负值低头。
  *         同样有 A=0 时 NaN、fx=0 时 90° 的边界行为。
  */
int LSM303DLH_CalculationXAxisAngle(int16_t Xa, int16_t Ya, int16_t Za)
{
	double A;
	float fx,fy,fz;
	
	A = sqrt((int)Xa*Xa + (int)Ya*Ya + (int)Za*Za);	//计算角加速度的矢量模长 |A|=根号下(X*X+Y*Y+Z*Z)
	fx = Xa/A;
	fy = Ya/A;
	fz = Za/A;
	
	
	//X方向
	A = fz*fz+fy*fy;
	A = sqrt(A);
	
	A = (double)A/fx;
	A = atan(A); 
	A = A*180/PI;
	if(A < 0)
	{
		A += 90; //向上为正
	}
	else
	{
		A = 90-A;
		A = 0-A; //向下为负
	}
	return A*100;
}

/*************************************************************************************************************************
*function  	:	Azimuth_Calculate()
*paraments	: int16_t Xa, int16_t Ya, int16_t Za, int16_t Xm, int16_t Ym, int16_t Zm
*return     : Azimuth
*detail     :	to Calculate the Azimuth to find the direction
*************************************************************************************************************************/
/**
  * @brief  计算倾斜补偿后的方位角（罗盘朝向）
  * @param  Xa,Ya,Za 加速度原始值（用于求 pitch/roll）
  * @param  Xm,Ym,Zm 磁力原始值（用于求水平面投影）
  * @retval 方位角（-180~180 度）；调用者通常负值 +360 归一到 0~360
  * @note   步骤：
  *         1) pitch = atan2(Xa, sqrt(Ya²+Za²))、roll = atan2(Ya, sqrt(Xa²+Za²))；
  *         2) 把磁力矢量按 roll/pitch 旋转到水平面，得到 Hy/Hx；
  *         3) Azimuth = atan2(Hy, Hx)，结果在 -180~180。
  *         为什么必须补偿：表盘不是水平时，磁力计的 Z 分量会混进水平分量，
  *         不补偿的话一抬腕方向就乱跳。
  *         本函数没有做磁偏角修正，也没有硬铁/软铁校准，绝对方向会有
  *         几度到十几度的系统误差，只适合做“大致朝向”。
  */
float Azimuth_Calculate(int16_t Xa, int16_t Ya, int16_t Za, int16_t Xm, int16_t Ym, int16_t Zm)
{
	float pitch, roll, Hy, Hx, Azimuth; 
	pitch   = atan2f(Xa, sqrtf(Ya * Ya + Za * Za));
	roll    = atan2f(Ya, sqrtf(Xa * Xa + Za * Za));
	Hy      = Ym * cosf(roll) + Xm * sinf(roll) * sinf(pitch) - Zm * cosf(pitch) * sinf(roll);
	Hx      = Xm * cosf(pitch) + Zm * sinf(pitch);
	Azimuth = atan2f(Hy,Hx)*180.0/PI;
	return Azimuth;
}
