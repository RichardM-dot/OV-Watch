/*
 * ============================================================================
 * 【中文导读】WDOG.c —— 外部硬件看门狗（B级导读）
 * ----------------------------------------------------------------------------
 * 板上外挂一颗看门狗芯片（不是 STM32 内部的 IWDG），两个信号：
 *   WDOG_EN = PB1，**低电平使能**（拉低才看门，拉高关闭）；
 *   WDI     = PB2，翻转它来“喂狗”。
 * WDOG_Feed() 用 HAL_GPIO_TogglePin 翻转 WDI，靠“电平跳变”喂狗而不是固定
 * 电平，所以两次调用之间必须真的产生一次 0→1→0 的变化；喂狗周期由
 * WDOGFeedTask 每 100 ms 保证一次。
 *
 * 系统进入 STOP 前会 WDOG_Disnable()（否则睡眠期间没人喂狗会被复位），
 * 醒来先 WDOG_Feed() 再按需重新使能。
 *
 * 阅读顺序：WDOG_Port_Init → WDOG_Enable / WDOG_Disnable → WDOG_Feed。
 *
 * 坑：
 *   - EN 是低有效，Enable/Disable 的电平和平常直觉相反；
 *   - 函数名 WDOG_Disnable 是笔误（应为 Disable），为了不破坏调用方保留原样；
 *   - Port_Init 没有显式写 ODR，而 ODR 复位值是 0，所以 PB1 配成输出的
 *     瞬间就是“低=已使能”——上电即开始看门，后面的 WDOG_Enable 只是确认。
 * ============================================================================
 */

#include "WDOG.h"
#include "main.h"

//WDOG_EN
#define WDOG_EN_PORT	GPIOB
#define WDOG_EN_PIN		GPIO_PIN_1
//WDI
#define WDI_PORT	GPIOB
#define WDI_PIN		GPIO_PIN_2

/**
  * @brief  配置看门狗的两个引脚
  * @param  无
  * @retval 无
  * @note   EN(PB1) 配成推挽低速：它只是个静态使能电平；
  *         WDI(PB2) 配成推挽高速：需要频繁翻转产生喂狗脉冲。
  *         注意没有先写 ODR：复位值为 0，所以 PB1 一旦变成输出就是低电平
  *         （= 使能看门狗）。上电后必须尽快开始喂狗，否则会被复位。
  */
void WDOG_Port_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStructure = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitStructure.Pin = WDOG_EN_PIN;
  GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructure.Pull = GPIO_PULLUP;
  GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(WDOG_EN_PORT, &GPIO_InitStructure);
  GPIO_InitStructure.Pin = WDI_PIN;
  GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(WDI_PORT, &GPIO_InitStructure);
}

/**
  * @brief  使能看门狗（WDOG_EN 拉低）
  * @param  无
  * @retval 无
  * @note   低电平有效：拉低后芯片开始计时，超时没喂狗就拉复位。
  */
void WDOG_Enable(void)
{
  HAL_GPIO_WritePin(WDOG_EN_PORT,WDOG_EN_PIN,GPIO_PIN_RESET);
}

/**
  * @brief  关闭看门狗（WDOG_EN 拉高）
  * @param  无
  * @retval 无
  * @note   高电平关闭。进 STOP 模式前必须调用，否则睡眠期间没人喂狗，
  *         醒来时已经被复位了。函数名 Disnable 是笔误，保留原样。
  */
void WDOG_Disnable(void)
{
  HAL_GPIO_WritePin(WDOG_EN_PORT,WDOG_EN_PIN,GPIO_PIN_SET);
}

/**
  * @brief  喂狗：翻转 WDI 引脚
  * @param  无
  * @retval 无
  * @note   看门狗芯片检测的是 WDI 的**电平跳变**，所以必须 Toggle 而不是
  *         固定写高/写低。WDOGFeedTask 每 100 ms 调一次，远小于芯片超时。
  *         在临界区里连续调用两次等于没喂（又翻回原电平），调用频率别太高。
  */
void WDOG_Feed(void)
{
  HAL_GPIO_TogglePin(WDI_PORT,WDI_PIN);
}
