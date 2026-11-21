#include "delay.h"
#include "sys.h"
 
/*
 * ============================================================================
 * 【中文导读】delay.c —— 基于 SysTick 的 us/ms 阻塞延时
 * ----------------------------------------------------------------------------
 * 原理：SysTick 是 Cortex-M 内核的 24 位递减计数器，delay_init() 把它配成
 * HCLK(100MHz) 驱动、每 1ms 重装一次。delay_us 不靠任何"计时中断"，而是直接
 * 轮询 SysTick->VAL：两次读数的差值就是走过的时钟数，跨过一个 LOAD 周期就补上
 * reload，凑够 nus*SYS_CLK 个周期就返回。因此精度只取决于 HCLK，被中断打断也
 * 不影响（中断消耗的时间同样计入延时）。
 * 与 FreeRTOS 的关系：本工程 configTICK_RATE_HZ=1000，SysTick 被 FreeRTOS 拿去
 * 做调度节拍，HAL 的 1ms 时基则挪到了 TIM1（见 stm32f4xx_hal_timebase_tim.c）。
 * delay_us 只读 VAL、不依赖 SysTick 中断，所以 RTOS 启动后仍然能用；但它是
 * "忙等"，不会让出 CPU，长延时会把同/低优先级任务全部饿死——超过几毫秒就该
 * 改用 osDelay()。
 * 为什么初始化阶段能用：HardwareInitTask 里先 vTaskSuspendAll() 再 delay_init()，
 * 期间没有任务切换，忙等不会破坏调度；delay_ms(1000) 只是白白烧掉 1 秒 CPU。
 * 坑：SYS_CLK 必须与真实 HCLK 一致（sys.h 里写死 100）；delay_init 会重写 SysTick
 * 的 LOAD/VAL，若 configTICK_RATE_HZ 不是 1000，这里就把 RTOS 节拍改坏了；
 * OS_SUPPORT=0 时下面那套"RTOS 感知、会主动让出 CPU"的代码根本没参与编译。
 * ============================================================================
 */

/* 
//roughly delay
void delay_us(u32 nus)
{		
	u32 i;
	u16 temp;
	temp = nus*SYS_CLK/15;
	for(i=0;i<temp;i++);
}

//延时nms
//nms:要延时的ms数
void delay_ms(u16 nms)
{
	u32 i;
	for(i=0;i<nms;i++) delay_us(1000);
}
*/
// 上面是"空循环数数"的原始写法，已废弃：编译器一开优化就会把它整个删掉，
// 而且循环次数随主频/流水线变化，根本不准。保留只为说明演进过程。

/**
  * @brief  初始化延时基准：把 SysTick 配成 1ms 周期的自由运行计数器。
  * @note   时钟源选 HCLK（不是 HCLK/8）：100MHz 下 1us = 100 个计数，分辨率 10ns。
  *         HAL_SYSTICK_Config(SystemCoreClock/(1000/uwTickFreq)) 中 uwTickFreq=1，
  *         即 SystemCoreClock/1000 = 100000，对应 1ms 重装；该函数内部还会清 VAL、
  *         使能计数器和 SysTick 中断。
  *         注意：FreeRTOS 启动时也按 configCPU_CLOCK_HZ/configTICK_RATE_HZ 配置
  *         SysTick，两者算出来都是 100000，所以这里重复配置不会改变节拍，只会
  *         丢掉当前这不到 1ms 的计数值。
  * @param  None
  * @retval None
  */
void delay_init(void)
{
	HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK); // 时钟源 = HCLK，与 FreeRTOS 的配置保持一致
	/* Configure the SysTick to have interrupt in 1ms time basis*/
	HAL_SYSTICK_Config(SystemCoreClock / (1000U / uwTickFreq)); // 重装值 = 100MHz/1000 = 100000 → 1ms
}

#if OS_SUPPORT 						    								   
// 下面这套是"RTOS 友好版"延时：延时前挂起调度器，长延时直接 vTaskDelay 让出 CPU。
// 但本工程 sys.h 里 OS_SUPPORT=0，而且 delay.h/sys.h 并没有提供 delay_osschedlock/
// delay_osschedunlock/delay_ostimedly/fac_ms 的宏或变量，所以这段代码既没编译、
// 也链接不过；真要启用，得先把正点原子模板里的那组宏补回来。
void delay_us(u32 nus)
{		
	u32 ticks;
	u32 told,tnow,tcnt=0;
	u32 reload=SysTick->LOAD;				   	 
	ticks=nus*SYS_CLK; 						 // 1us 对应 SYS_CLK 个 HCLK 周期
	delay_osschedlock();					 // 挂起调度器，防止延时期间被切换任务（否则计数值会被打乱）
	told=SysTick->VAL;        				
	while(1)
	{
		tnow=SysTick->VAL;	
		if(tnow!=told)
		{	    
			if(tnow<told)tcnt+=told-tnow;//这里注意一下SYSTICK是一个递减的计数器就可以了.	
			else tcnt+=reload-tnow+told;	    
			told=tnow;
			if(tcnt>=ticks)break;			
		}  
	};
	delay_osschedunlock();															    
}  

void delay_ms(u16 nms)
{	
	if(delay_osrunning&&delay_osintnesting==0)    // 只在调度器已运行且不在中断里时，才允许用"让出 CPU"的延时
	{		 
		if(nms>=fac_ms)						// 大于一个 tick 周期的部分交给 vTaskDelay，精度换 CPU
		{ 
   			delay_ostimedly(nms/fac_ms);	
		}
		nms%=fac_ms;						 // 余下的不足一个 tick，仍然用忙等补齐
	}
	delay_us((u32)(nms*1000));			
}
#else  
	 
/**
  * @brief  微秒级阻塞延时，精度取决于 HCLK 与 SYS_CLK 是否一致。
  * @note   实现是"读两次 SysTick->VAL 求差"：VAL 递减，若 tnow<told 说明这期间
  *         没跨过重装点，差值就是走过的周期；若 tnow>told 说明跨过了 0，需要
  *         补上一个 reload。所有差值累加到 tcnt，达到 nus*SYS_CLK 就退出。
  *         因为只读寄存器、不依赖中断，所以在 FreeRTOS 跑起来之后依然有效；
  *         代价是这段时间 CPU 100% 空转，且不会触发任务切换。
  * @param  nus: 要延时的微秒数（u32，nus*100 超过 2^32 会溢出，约 42.9 秒）
  * @retval None
  */
void delay_us(u32 nus)
{		
	u32 ticks;
	u32 told,tnow,tcnt=0;
	u32 reload=SysTick->LOAD;				    	 // 24 位重装值，跨周期补偿时要用
	ticks=nus*SYS_CLK; 						 // SYS_CLK=100(MHz)，所以 1us = 100 个计数
	told=SysTick->VAL;        				 // 记下起点
	while(1)
	{
		tnow=SysTick->VAL;	
		if(tnow!=told)
		{	    
			if(tnow<told)tcnt+=told-tnow;	 // 正常递减：差值就是走过的周期数
			else tcnt+=reload-tnow+told;	 // 跨过 0 点：先到 0 再重装，补上 reload
			told=tnow;
			if(tcnt>=ticks)break;			 // 凑够目标周期数就退出
		}  
	}
}

/**
  * @brief  毫秒级阻塞延时：把 nms 拆成 nms 次 1000us 的忙等。
  * @note   拆开而不是一次算 nms*1000*SYS_CLK，是为了避免 u32 乘法溢出
  *         （一次算的话 nms>42949 就溢出了）。代价是每次循环都有函数调用开销，
  *         长延时的实际时间会略大于标称值。
  * @param  nms: 要延时的毫秒数（u16，最大 65535ms）
  * @retval None
  */
void delay_ms(u16 nms)
{
	u32 i;
	for(i=0;i<nms;i++) delay_us(1000);		 // 1ms 一次，累计 nms 次
}
#endif
