/*
 * ============================================================================
 * 【中文导读】power.c —— 电源管理（B级导读）
 * ----------------------------------------------------------------------------
 * 三件事，全部围绕“这块表怎么活、怎么死、还剩多少电”：
 *   1) POWER_EN(PA3) 控制 TPS63020 的使能：高=维持 3V3，低=整机断电。
 *      这就是软关机——MCU 自己把电源关掉，之后只能靠按键/充电重新上电。
 *   2) CHARGE(PA2) 是充电状态输入，1=正在充电；配了双边沿 EXTI 用于
 *      “插拔充电器”唤醒。
 *   3) BAT_ADC(PA1) 采电池分压（1/2 分压，12 位 ADC，参考 3.3 V），
 *      BatCheck_8times 做 8 次平均，再按电压分段映射成 0~100% 电量。
 *
 * 阅读顺序：Power_Pins_Init → Power_Enable/DisEnable → ChargeCheck →
 *           BatCheck/BatCheck_8times → PowerCalculate → Power_Init。
 *
 * 坑：
 *   - PowerCalculate 里 voltage < 3.45 V 时没有任何分支给 power 赋值，
 *     返回的是未初始化的栈变量（电池过放/ADC 异常时会看到乱跳的电量）；
 *   - 充电时电压会被抬高，所以先减去 INTERNAL_RES*CHARGING_CUR=0.128 V
 *     再查表，否则一插充电器就显示 100%；
 *   - 4096 而不是 4095 是“2^12”的写法，满量程会略微偏低，属可接受误差。
 * ============================================================================
 */

#include "power.h"
#include "adc.h"
#include "delay.h"

#define INTERNAL_RES 0.128
#define CHARGING_CUR 1

/**
  * @brief  配置电源使能脚（PA3）和充电检测脚（PA2）
  * @param  无
  * @retval 无
  * @note   先写 ODR=0 再 HAL_GPIO_Init：引脚“变成输出”的瞬间就是低电平，
  *         不会出现上电毛刺把电源误打开；随后由 Power_Enable() 拉高自锁。
  *         CHARGE 配成双边沿中断（EXTI2，优先级 5），用于插拔充电器唤醒；
  *         中断服务里只清标志，具体处理在 user_ChargCheckTask。
  */
void Power_Pins_Init()
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(POWER_PORT, POWER_PIN, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA3 */
  GPIO_InitStruct.Pin = POWER_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(POWER_PORT, &GPIO_InitStruct);

  /*Configure GPIO pin : PA2 */
  GPIO_InitStruct.Pin = CHARGE_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(CHARGE_PORT, &GPIO_InitStruct);

  HAL_NVIC_SetPriority(EXTI2_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

}

/**
  * @brief  拉高 POWER_EN，维持整机供电（开机自锁）
  * @param  无
  * @retval 无
  * @note   按键是“点动”的，松手后 3V3 就靠这根线撑着，所以上电流程里
  *         必须尽早调用；一旦拉低（Power_DisEnable），MCU 自己也会掉电。
  */
void Power_Enable()
{
	HAL_GPIO_WritePin(POWER_PORT,POWER_PIN,GPIO_PIN_SET);
}

/**
  * @brief  拉低 POWER_EN，关断 TPS63020，整机断电（软关机）
  * @param  无
  * @retval 无
  * @note   调用后电源立刻消失，本函数之后的代码不保证能执行完；
  *         再次开机只能靠按键或插充电器。
  */
void Power_DisEnable()
{
	HAL_GPIO_WritePin(POWER_PORT,POWER_PIN,GPIO_PIN_RESET);
}

/**
  * @brief  读充电状态
  * @param  无
  * @retval 1=正在充电，0=未充电（直接返回 GPIO 电平）
  */
uint8_t ChargeCheck()//1:charging
{
	return HAL_GPIO_ReadPin(CHARGE_PORT,CHARGE_PIN);
}

/**
  * @brief  单次采集电池电压
  * @param  无
  * @retval 电池电压（V）
  * @note   12 位 ADC、3.3 V 参考、硬件 1/2 分压：
  *         BatVoltage = dat * 2 * 3.3 / 4096（4096 = 2^12，满量程略偏低）。
  *         单次采样噪声较大，要稳定值请用 BatCheck_8times()。
  *         PollForConversion 只等 5 ms 且不检查返回值，ADC 没就绪时
  *         dat 可能是上一次的旧值。
  */
float BatCheck()
{
	uint16_t dat;
	float BatVoltage;
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1,5);
	dat = HAL_ADC_GetValue(&hadc1);
	HAL_ADC_Stop(&hadc1);
	BatVoltage = dat *2 *3.3 /4096;
	return BatVoltage;
}

/**
  * @brief  连续采样 8 次后取平均，抑制 ADC 噪声
  * @param  无
  * @retval 电池电压（V）
  * @note   dat 用 uint32_t 累加，8 次不会溢出；`dat >> 3` 等价于 /8。
  *         每次采样之间 delay_ms(1)，总共约 8 ms，会占住调用它的任务。
  */
float BatCheck_8times()
{
	uint32_t dat=0;
	uint8_t i;
	float BatVoltage;
	for(i=0;i<8;i++)
	{
		HAL_ADC_Start(&hadc1);
		HAL_ADC_PollForConversion(&hadc1,5);
		dat += HAL_ADC_GetValue(&hadc1);
		HAL_ADC_Stop(&hadc1);
		delay_ms(1);
	}
	dat = dat>>3; // >>3 等价于 /8，求 8 次平均
	BatVoltage = dat *2 *3.3 /4096;
	return BatVoltage;
}

/**
  * @brief  把电池电压映射成 0~100 的电量百分比
  * @param  无
  * @retval 电量百分比（分段值 100/90/80/.../10/5）
  * @note   充电时先减去 INTERNAL_RES*CHARGING_CUR = 0.128 V：充电电流在
  *         电池内阻上产生的压升会让读数虚高，不补这一下，一插充电器就 100%。
  *         分段表是非线性的——锂电池放电曲线中段平缓、两端陡峭，
  *         所以 3.92~3.98 V 只差 10%，而 3.45~3.68 V 也占 5%。
  *         坑：voltage < 3.45 V（过放/ADC 异常）时没有任何分支给 power 赋值，
  *         函数返回的是未初始化的栈变量，表现为电量乱跳。
  */
uint8_t PowerCalculate()
{
	uint8_t power;
	float voltage;
	voltage = BatCheck_8times();

	if(ChargeCheck())
	{voltage -= INTERNAL_RES * CHARGING_CUR;} // 扣除充电电流在内阻上的压升

	if((voltage >= 4.2))
	{power = 100;}
	else if(voltage >= 4.06 && voltage <4.2)
	{power = 90;}
	else if(voltage >= 3.98 && voltage <4.06)
	{power = 80;}
	else if(voltage >= 3.92 && voltage <3.98)
	{power = 70;}
	else if(voltage >= 3.87 && voltage <3.92)
	{power = 60;}
	else if(voltage >= 3.82 && voltage <3.87)
	{power = 50;}
	else if(voltage >= 3.79 && voltage <3.82)
	{power = 40;}
	else if(voltage >= 3.77 && voltage <3.79)
	{power = 30;}
	else if(voltage >= 3.74 && voltage <3.77)
	{power = 20;}
	else if(voltage >= 3.68 && voltage <3.74)
	{power = 10;}
	else if(voltage >= 3.45 && voltage <3.68)
	{power = 5;}
	return power;
}

/**
  * @brief  电源初始化：配脚 + 打开电源自锁
  * @param  无
  * @retval 无
  * @note   必须在系统启动早期调用（HardwareInitTask 里），否则按键松开就掉电。
  */
void Power_Init(void)
{
	Power_Pins_Init();
	Power_Enable();
}


