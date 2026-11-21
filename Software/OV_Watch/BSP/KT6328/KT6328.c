/*
 * ============================================================================
 * 【中文导读】KT6328.c —— 蓝牙模块电源控制（B级导读）
 * ----------------------------------------------------------------------------
 * KT6328 是串口透传蓝牙模块，数据通路走 USART1（115200，配置在 Core/Src/
 * usart.c 里），本文件只管它的 EN 使能脚：PA8，**高电平使能、低电平关闭**。
 *
 * 三个函数都很薄：
 *   KT6328_GPIO_Init  配 PA8 为推挽输出，并先写 0（默认关闭）；
 *   KT6328_Enable     拉高，给模块上电；
 *   KT6328_Disable    拉低，断电省电。
 *
 * 阅读顺序：GPIO_Init → Enable/Disable。
 *
 * 坑 / 注意：
 *   - 初始化时先写 ODR=0 再配输出，保证“变成输出”的瞬间就是关闭状态，
 *     避免模块在系统还没准备好时上电乱发数据；
 *   - 上电后模块需要几百 ms 启动，Enable 之后不能立刻发 AT/透传数据；
 *   - 本文件没有复位脚、没有状态查询、也不配波特率，这些都在别处；
 *   - 为了省电，开机初始化完成后通常会立刻 Disable，用时再 Enable。
 * ============================================================================
 */

#include "KT6328.h"

/**
  * @brief  配置蓝牙使能脚 PA8
  * @param  无
  * @retval 无
  * @note   先把输出寄存器写 0，再配置成推挽输出——这样引脚生效的瞬间就是
  *         低电平（模块关闭），不会出现上电毛刺把蓝牙模块打开。
  *         速度选 LOW 就够：这只是个静态电平，不参与高速翻转。
  */
void KT6328_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET); // 先置低：配置成输出的瞬间就是“关闭”

  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

}


/**
  * @brief  打开蓝牙模块（PA8 拉高）
  * @param  无
  * @retval 无
  * @note   只给模块上电，模块内部启动还需要几百 ms；这段时间发 AT 命令
  *         或透传数据都会丢。串口本身的配置不在这里。
  */
void KT6328_Enable(void)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
}


/**
  * @brief  关闭蓝牙模块（PA8 拉低），省电
  * @param  无
  * @retval 无
  * @note   模块断电后连接会断开，重新 Enable 需要重新配对/连接。
  *         开机初始化完通常立刻调它，等用户进蓝牙页面再打开。
  */
void KT6328_Disable(void)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
}

