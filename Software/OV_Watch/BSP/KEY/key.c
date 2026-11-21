/*
 * ============================================================================
 * 【中文导读】key.c —— 按键驱动（B级导读）
 * ----------------------------------------------------------------------------
 * 硬件：KEY1=PA5（内部上拉，**按下为 0**，下降沿中断）、KEY2=PA4（**按下为 1**，
 * 上升沿中断）。两个键极性相反，读代码时最容易在这里绕晕。
 * EXTI 只负责“把系统从 STOP 模式唤醒 + 留下中断标志”，真正的消抖、短按、
 * 长按判定都在 user_KeyTask 里轮询 KeyScan() 完成。
 *
 * KeyScan 是一个极简状态机：
 *   key_up=1 表示“等待按下”，key_down 记录按的是哪个键；
 *   按下沿先 osDelay(3) 消抖，再锁存 key_down、清 key_up；
 *   释放沿再消抖确认，确认松开后才把键值返回上层——所以是**松手触发**，
 *   不是按下触发。长按由上层用系统节拍另外计时。
 *
 * 坑：
 *   - osDelay(3) 是 RTOS 延时，调度器被 vTaskSuspendAll() 挂起时会退化成忙等；
 *   - 两个 if 是独立的，同时按住两个键时 key_down 会被 KEY2 覆盖（=2）；
 *   - 释放条件要求两个键都松开，另一个键按住时不会产生事件；
 *   - KeyScan 内部有 osDelay，不能在中断里调用。
 * ============================================================================
 */

#include "key.h"
#include "delay.h"
#include "user_TasksInit.h"

/**
  * @brief  配置两个按键为 EXTI 中断输入
  * @param  无
  * @retval 无
  * @note   KEY1(PA5)：上拉 + 下降沿（按下接地，电平 1→0）；
  *         KEY2(PA4)：无上下拉 + 上升沿（按下接高，电平 0→1）。
  *         PA5 属于 EXTI9_5 通道，PA4 属于 EXTI4，所以这里要开两个 NVIC。
  *         优先级都设 5：低于系统关键中断，高于普通任务。
  *         中断里只清标志/唤醒，业务处理交给 user_KeyTask。
  */
void Key_Port_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin : PA5 */
  GPIO_InitStruct.Pin = KEY1_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(KEY1_PORT, &GPIO_InitStruct);

	/*Configure GPIO pin : PA4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
	HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

	HAL_NVIC_SetPriority(EXTI4_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);
}

/**
  * @brief  扫描按键，返回“松手”瞬间的键值（带软件消抖的状态机）
  * @param  mode 非 0 时强制复位内部状态（模式切换/唤醒后清掉残留状态）
  * @retval 0=无事件；1=KEY1 松开；2=KEY2 松开
  * @note   状态机分两段：
  *         1) 等待按下：key_up=1 且检测到任意键按下 → osDelay(3) 消抖
  *            → 锁存 key_down、清 key_up（防止按住期间重复触发）；
  *         2) 等待释放：key_down 非 0 且两个键都回到松开电平 → 再消抖
  *            → 返回键值并复位状态。
  *         两个“按下”判断是独立的 if，同时按两个键时 key_down 最终为 2。
  *         osDelay 依赖调度器，不能在中断上下文调用。
  */
uint8_t KeyScan(uint8_t mode)
{
	static uint8_t key_up = 1;   // 1=空闲等待按下（static：状态跨调用保持）
	static uint8_t key_down = 0; // 0=无键，1=KEY1，2=KEY2（已消抖锁存）
	uint8_t keyvalue = 0;

	if(mode)
	{
		key_up = 1;
		key_down = 0;
	}

	if( key_up && ((!KEY1) || KEY2)) // KEY1 为低 或 KEY2 为高 = 有键按下
	{
		osDelay(3);//ensure the key：3 个节拍消抖（调度器挂起时会忙等）
		if(!KEY1)
			key_down = 1;
		if(KEY2)
			key_down = 2;
		if(key_down)
			key_up = 0;
	}

	if ( key_down && (KEY1 && (!KEY2)) ) // 两个键都回到松开电平才认为释放
	{
		osDelay(3);//ensure the key：释放也要消抖，防抖动脉冲
		if(KEY1 && (!KEY2))
		{
			key_up = 1;
			keyvalue = key_down; // 松手才上报，一次按下只产生一个事件
			key_down = 0;
		}
	}

	return keyvalue;
}

