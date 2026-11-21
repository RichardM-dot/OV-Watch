/*
 * ============================================================================
 * 【中文导读】HrAlgorythm.c —— 心率算法（B级导读）
 * ----------------------------------------------------------------------------
 * 输入是 EM7028 的原始 PPG 采样值 + 采样时刻，输出是 BPM。核心就一个函数
 * HR_Calculate()，用三个全局环形队列（QUEUE_SIZE=7）：
 *   datas   最近 7 个原始采样值；
 *   times   对应的采样时刻；
 *   HR_List 最近 7 个“由心跳周期换算出的瞬时心率”。
 *
 * 找峰值的思路：窗口中心 datas[3] 比左右各 3 个点都高（局部极大），
 * 就认为是一个脉搏波峰；再要求与上一个峰的时间差 > 425（ms），
 * 425ms 对应约 141 BPM，同时能滤掉噪声造成的重复峰值。
 * 用 60000/间隔 得到瞬时 BPM，最后对 7 个瞬时值求平均。
 *
 * 已知 bug（读这段代码时务必知道）：
 *   1) Hr_Ave_Filter() 声明返回 uint8_t 却**没有 return 语句**，而且 ave
 *      没有初始化——未定义行为，实际返回寄存器里的随机值，心率会乱跳。
 *      正确写法：uint32_t ave = 0; ... return ave;
 *   2) 第一次检测到峰值时 peaks_time[0] 还是 0，60000/(times[3]-0) 算的是
 *      “60000/系统运行时间”，不是心率，会先污染一次 HR_List；
 *   3) 用 HR_List.data[6] != 0 判断“已经攒够 7 个”并不可靠：环形队列的
 *      data[6] 与 FIFO 顺序无关，且 60000/间隔 完全可能算出 0。
 * ============================================================================
 */

#include "HrAlgorythm.h"
#include "string.h"

Queue datas;
Queue times;
Queue HR_List;

/**
  * @brief  初始化三个全局队列
  * @param  无
  * @retval 无
  * @note   front=0、rear=-1、size=0。全局变量虽然会被清零，但 rear 必须是
  *         -1（enqueue 先 +1 再取模），所以不调这个函数直接入队会错位。
  */
void HR_AlgoInit(void)
{
	initQueue(&datas);
	initQueue(&times);
	initQueue(&HR_List);
}

/**
  * @brief  求数组平均值（**这是一个有 bug、未写完的函数**）
  * @param  HrList 数据数组
  * @param  lenth  元素个数
  * @retval 名义上是平均值；实际**没有 return**，返回值是未定义的
  * @note   ave 声明后没有初始化，循环里直接 `ave += HrList[i]`，等于在
  *         栈上的垃圾值上累加；算完 `ave /= lenth` 也没有返回。
  *         调用点 `HR = Hr_Ave_Filter(HR_List.data,7)` 拿到的是寄存器里的
  *         随机值，这就是“心率乱跳”的根源之一。
  */
uint8_t Hr_Ave_Filter(uint32_t *HrList, uint8_t lenth)
{
	uint32_t ave; // 坑：没有初始化，初值是栈上的随机数
	uint8_t i;
	for(i = 0;i<lenth;i++)
	{
		ave += HrList[i];
	}
	ave /= lenth;
	// 坑：这里缺少 return ave;，函数返回值未定义
}

/**
  * @brief  喂入一个采样点，检测脉搏峰值并返回心率
  * @param  present_dat  本次原始 PPG 采样值
  * @param  present_time 本次采样时刻（与 425 的阈值同单位，实际是 ms/tick）
  * @retval 心率 BPM；还没测出来时返回 0（或 Hr_Ave_Filter 的随机返回值）
  * @note   三个队列都只保留最近 7 个：满了先 dequeue 再 enqueue。
  *         峰值判定以 datas[3] 为中心，左右各 3 个点都不高于它；内侧比较用
  *         >=、最外侧用 >，是为了避免平顶波形被连续判定成多个峰。
  *         与上一个峰的时间差必须 > 425：425ms ≈ 141 BPM，既限制上限，
  *         也能过滤抖动造成的重复峰。
  *         60000/(t0-t1) 是整数除法，周期越长（心率越低）分辨率越差。
  *         `HR_List.data[6] != 0` 的本意是“队列已攒满 7 个”，但环形队列的
  *         data[6] 只是固定槽位，和有效个数/FIFO 顺序无关，这个判断不可靠。
  *         首次检测到峰值时 peaks_time[0] 还是 0，会算出 60000/运行时间，
  *         是一个无意义的值，会先污染一次 HR_List。
  */
uint16_t HR_Calculate(uint16_t present_dat,uint32_t present_time)
{

	static uint16_t peaks_time[]={0,0}; // [0]=本次峰时刻，[1]=上一次峰时刻
	static uint8_t HR=0; // 心率缓存（uint8 最大 255 BPM，够用）

	if(isQueueFull(&datas))
	{dequeue(&datas);}
	if(isQueueFull(&times))
	{dequeue(&times);}
	if(isQueueFull(&HR_List))
	{dequeue(&HR_List);}

	enqueue(&datas,present_dat);
	enqueue(&times,present_time);

	if((datas.data[3]>=datas.data[2]) && (datas.data[3]>=datas.data[1]) && (datas.data[3]>datas.data[0]) 
		&& (datas.data[3]>=datas.data[4]) && (datas.data[3]>=datas.data[5]) && (datas.data[3]>datas.data[6]))
	{
			if((times.data[3]-peaks_time[0])>425) // 425ms≈141BPM，兼作重复峰去重
			{
					peaks_time[1] = peaks_time[0];
					peaks_time[0] = times.data[3];
					enqueue(&HR_List,60000/(peaks_time[0]-peaks_time[1])); // 60s/心跳周期 = 瞬时BPM
					if(HR_List.data[6]!=0) // 本意是“攒够7个”，但环形队列下不可靠
					{HR = Hr_Ave_Filter(HR_List.data,7);}
			}
	}
	return HR;
}



