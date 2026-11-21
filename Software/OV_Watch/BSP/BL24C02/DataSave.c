/*
 * ============================================================================
 * 【中文导读】DataSave.c —— 设置/计步数据的掉电保存（B级导读）
 * ----------------------------------------------------------------------------
 * 在 BL24C02（256 字节 EEPROM）之上做了一层“带校验的键值存储”：
 *   0x00/0x01 存 0x55/0xAA 作为“已初始化”魔数；
 *   0x10 起存用户设置（抬腕开关、UI 开关）；
 *   0x20 起存日期 + 当天步数。
 * 具体地址分配见下面的 EEPROM Data description 表。
 *
 * EEPROM_Check() 是整套机制的核心：读不到魔数就认为“首次上电或数据损坏”，
 * 重新写入魔数并回读确认；SettingSave/SettingGet 每次操作前都先调它，
 * 所以 addr<=1 的地址被保留，业务数据只能从 0x02 往上写（代码里用
 * `addr > 1` 卡住）。
 *
 * 阅读顺序：EEPROM_Init → EEPROM_Check → SettingSave / SettingGet。
 *
 * 坑：
 *   - EEPROM 写周期约 5 ms，函数里到处 delay_ms(10) 就是为它留的；
 *   - 每次 Save/Get 都做一次 EEPROM_Check，等于每次多花 20~30 ms，
 *     高频保存（如每秒存步数）会很浪费；
 *   - BL24C02 一页 8 字节，lenth 超过 8 或跨页会回卷覆盖，本层不检查；
 *   - EEPROM_Check 的返回值是“0=正常”，容易和常见错误码习惯相反。
 * ============================================================================
 */

#include "DataSave.h"
#include "string.h"

/******************************************
EEPROM Data description:
[0x00]:0x55 for check
[0x01]:0xAA for check

[0x10]:user wrist setting, HWInterface.IMU.wrist_is_enabled
[0x11]:user ui_APPSy_EN setting

[0x20]:Last Save Day(0-31)
[0x21]:Day Steps

*******************************************/


/**
  * @brief  初始化 EEPROM（只初始化 I2C，不校验内容）
  * @param  无
  * @retval 无
  */
void EEPROM_Init(void)
{
	BL24C02_Init();
}


//to check the Data is right and the EEPROM is working
/**
  * @brief  检查 EEPROM 是否已初始化、读写是否正常
  * @param  无
  * @retval 0=正常（魔数正确或已成功写入）；1=读写失败
  * @note   先读 0x00/0x01：等于 0x55/0xAA 说明是“老设备”，直接返回 0。
  *         否则认为首次上电/数据损坏，写入魔数、清零缓冲、再回读确认，
  *         确认成功也返回 0——也就是说“刚格式化”和“本来就好”无法区分。
  *         两次 delay_ms(10) 是等 EEPROM 的 5 ms 内部写周期。
  *         注意返回值是 0=OK，和常见“0=错误”的习惯相反。
  */
uint8_t EEPROM_Check(void)
{
	uint8_t check_buff[2];
	delay_ms(10); // 等上一次写周期（约 5ms）结束，否则器件不响应
	BL24C02_Read(0,2,check_buff); // 读 0x00/0x01 的魔数
	if(check_buff[0] == 0x55 && check_buff[1] == 0xAA)
	{
		return 0;//check OK
	}
	else
	{
		check_buff[0] = 0x55; // 首次上电/数据损坏：写入“已初始化”标志
		check_buff[1] = 0xAA;
		delay_ms(10);
		BL24C02_Write(0,2,check_buff);
		memset(check_buff,0,2);
		delay_ms(10);
		BL24C02_Read(0,2,check_buff);
		if(check_buff[0] == 0x55 && check_buff[1] == 0xAA)
			return 0;//check ok
	}
	return 1;//check erro
}


/**
  * @brief  把一段设置写入 EEPROM
  * @param  buf   数据指针
  * @param  addr  起始地址（必须 >1）
  * @param  lenth 字节数（建议 <=8，且不跨页）
  * @retval 0=成功；1=地址非法或 EEPROM 检查失败
  * @note   addr>1 是为了保护 0x00/0x01 的魔数；`!EEPROM_Check()` 即“检查通过”。
  *         写前 delay_ms(10) 等上一次写周期，写后没有再等——连续写多个
  *         地址时，下一次调用自己的写前延时会兜住。
  */
//to Save the settings
uint8_t SettingSave(uint8_t *buf, uint8_t addr, uint8_t lenth) // 把设置数据写入 EEPROM
{
	if(addr > 1 && !EEPROM_Check()) // addr<=1 是魔数区；Check 返回 0 才是正常
	{
		delay_ms(10); // 写入前再等一次，确保上一次写周期已结束
		BL24C02_Write(addr,lenth,buf);
		return 0;
	}
	return 1;
}


/**
  * @brief  从 EEPROM 读回一段设置
  * @param  buf   接收缓冲区
  * @param  addr  起始地址（必须 >1）
  * @param  lenth 字节数
  * @retval 0=成功；1=地址非法或 EEPROM 检查失败（此时 buf 内容未定义）
  * @note   读之前必须确认上一次写已完成（>5ms），否则会读到 0xFF；
  *         这里靠 EEPROM_Check 内部的 delay 和读操作本身拉开时间。
  */
//to Get the settings
uint8_t SettingGet(uint8_t *buf, uint8_t addr, uint8_t lenth) // 从 EEPROM 中读取数据
{
	if(addr > 1 && !EEPROM_Check()) // 同上：保护魔数区 + 先做健康检查
	{
		delay_ms(10);
		BL24C02_Read(addr,lenth,buf);
		return 0;
	}
	return 1;
}
