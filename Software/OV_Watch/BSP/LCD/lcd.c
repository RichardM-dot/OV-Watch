/*
 * ============================================================================
 * 【中文导读】lcd.c —— ST7789 基础绘图层（B级导读）
 * ----------------------------------------------------------------------------
 * 设窗口+写显存的套路是所有 SPI 屏驱动的通用模式：
 *   LCD_Address_Set(x1,y1,x2,y2) 设好矩形窗口，然后连续 LCD_WR_DATA()，
 *   ST7789 自己在窗口内按行递增，不需要每像素都发坐标。
 *
 * 本文件分三层：
 *   1) 刷屏层：LCD_Fill（逐像素，慢）/ LCD_Color_Fill（DMA，LVGL 用）；
 *   2) 图形层：画点/线/矩形/圆（Bresenham、中点画圆）；
 *   3) 字库层：ASCII + 12/16/24/32 点阵汉字（字模在 lcdfont.h，GBK 编码）。
 *
 * 两个必须记住的坐标约定（很容易踩）：
 *   - LCD_Fill 的 xend/yend 是**开区间**（不含），内部会 -1 后设窗口；
 *   - LCD_Color_Fill 的 xend/yend 是**闭区间**（含），宽 = xend-xsta+1，
 *     因为 LVGL 的 area 就是闭区间。
 *   两者混用会出现整体错位一行/一列。
 *
 * 另外：只有 LCD_Fill / LCD_Color_Fill 加了 OFFSET_Y=20（240x280 的屏在
 * 240x320 显存里居中），LCD_DrawPoint 等原始绘图函数**不加**偏移。
 *
 * DMA 刷屏是异步的：LCD_Color_Fill 返回时数据还在发，必须等
 * HAL_SPI_TxCpltCallback 里确认 SPI 空闲后回调 lcd_ready_cb，LVGL 才能
 * 复用这块缓冲，否则下一帧会覆盖正在发送的数据（花屏）。
 *
 * 这些绘图函数是“裸机时代”的遗留，LVGL 接管显示后主要用于开机画面/调试。
 * ============================================================================
 */

#include "lcd.h"
#include "lcd_init.h"
#include "lcdfont.h"
#include "delay.h"
#include "spi.h"
#include "cmsis_os.h"

#define OFFSET_Y 20

static LCD_CallbackFunc_t lcd_ready_cb = NULL;

/******************************************************************************
      函数说明：在指定区域填充颜色
      入口数据：xsta,ysta   起始坐标
                xend,yend   终止坐标
								color       要填充的颜色
      @brief  在矩形区域内填充单色（逐像素写，慢，初始化清屏用）
      @param  xsta,ysta  起始坐标（左上角，包含）
      @param  xend,yend  终止坐标（右下角，**不包含**）
      @param  color      要填充的颜色（RGB565）
      @retval 无
      @note   xend/yend 是开区间，所以设窗口时要 -1；而 LCD_Color_Fill 用的是
              闭区间，两个函数不能混用坐标。本函数加 OFFSET_Y=20。
              慢的原因：每个像素都要走一次 LCD_WR_DATA（2 字节 SPI），
              240x280 全屏就是 13 万次调用，只适合开机清屏。
******************************************************************************/
void LCD_Fill(u16 xsta,u16 ysta,u16 xend,u16 yend,u16 color)
{
	u16 i,j;
	LCD_Address_Set(xsta,ysta+OFFSET_Y,xend-1,yend-1+OFFSET_Y);//设置显示范围（+OFFSET_Y 让可视区在 240x320 显存里居中）
	for(i=ysta;i<yend;i++)
	{
		for(j=xsta;j<xend;j++)
		{
			LCD_WR_DATA(color);
		}
	}
}

/******************************************************************************
      函数说明：在指定区域填充颜色
      入口数据：xsta,ysta   起始坐标
                xend,yend   终止坐标
								color       要填充的颜色
      @brief  用 DMA 把一整块像素推给屏幕（LVGL 的 flush_cb 走这里）
      @param  xsta,ysta  起始坐标（左上角，包含）
      @param  xend,yend  终止坐标（右下角，**包含**）
      @param  color_p    像素数组（u16，RGB565），长度 = 宽*高
      @retval 无
      @note   与 LCD_Fill 的坐标约定不同：这里是闭区间，宽 = xend-xsta+1、
              高 = yend-ysta+1，正好对上 LVGL 的 lv_area_t。
              异步：函数返回时 SPI+DMA 还在发，color_p 必须等
              HAL_SPI_TxCpltCallback 回调后才能被复用，否则会花屏。
              全屏刷新时这里只发一次，比逐像素快两个数量级。
******************************************************************************/
void LCD_Color_Fill(u16 xsta,u16 ysta,u16 xend,u16 yend,u16 *color_p)
{
	u16 i,j,width,height;
	width = xend-xsta+1;
	height = yend-ysta+1;
	uint32_t pixel_count = width * height;
	uint32_t byte_count = pixel_count * 2U;

	LCD_Address_Set(xsta,ysta+OFFSET_Y,xend,yend+OFFSET_Y);// 闭区间：xend/yend 都包含

	HAL_SPI_Transmit_DMA(&hspi1,(uint8_t*)color_p,byte_count); // 异步：返回时还没发完
}

/**
  * @brief  注册“刷屏完成”回调（LVGL 移植层会传 lcd_flush_ready_callback 进来）
  * @param  cb 传输完成时要调用的函数指针
  * @retval 无
  * @note   驱动层不直接依赖 LVGL，用回调解耦：DMA 发完 → 本驱动回调 →
  *         LVGL 调用 lv_disp_flush_ready() 允许复用缓冲。
  *         必须在第一次刷屏前注册，否则 lcd_ready_cb 为 NULL，LVGL 永远
  *         等不到“可以复用”，画面会卡在第一帧。
  */
// 设置通知的回调函数
void LCD_Set_Flush_Complete_Callback(LCD_CallbackFunc_t cb)
{
	lcd_ready_cb = cb;
}

/**
  * @brief  SPI DMA 发送完成中断回调（HAL 的弱函数在这里被重写）
  * @param  hspi 触发中断的 SPI 句柄
  * @retval 无
  * @note   先死等 SPI_FLAG_BSY 清零，再通知上层。原因是 DMA 的“传输完成”
  *         只表示数据从内存搬到了 SPI 移位寄存器，最后一位可能还在线上；
  *         此时就让 LVGL 复用缓冲，下一帧会把最后几个像素覆盖掉（花屏/撕裂）。
  *         只处理 SPI1（屏幕），其它 SPI 直接忽略。
  */
// DMA 传输完成中断回调
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if (hspi->Instance == SPI1)
	{
		// 确保最后一位数据离开 STM32 的移位寄存器
		while (hspi->Instance->SR & SPI_FLAG_BSY);
		if (lcd_ready_cb)
		{
			lcd_ready_cb();
		}
	}
}

/******************************************************************************
      函数说明：在指定位置画点
      入口数据：x,y 画点坐标
                color 点的颜色
      @brief  在 (x,y) 画一个像素
      @param  x,y   画点坐标（原始屏坐标，**不含** OFFSET_Y）
      @param  color 点的颜色（RGB565）
      @retval 无
      @note   为了画一个点要发 0x2A/0x2B/0x2C 三条命令 + 2 字节数据，
              所以画线/画圆这种逐点调用的函数其实很慢；LVGL 不会走这条路。
              注意坐标系：本函数不加 OFFSET_Y，而 LCD_Fill 加，二者混用会差 20 行。
******************************************************************************/
void LCD_DrawPoint(u16 x,u16 y,u16 color)
{
	LCD_Address_Set(x,y,x,y);//设置光标位置
	LCD_WR_DATA(color);
}


/******************************************************************************
      函数说明：画线
      入口数据：x1,y1   起始坐标
                x2,y2   终止坐标
                color   线的颜色
      @brief  画直线（Bresenham 整数算法）
      @param  x1,y1  起始坐标
      @param  x2,y2  终止坐标
      @param  color  线的颜色
      @retval 无
      @note   Bresenham 的核心：xerr/yerr 分别累加两个方向的增量，
              谁超过主轴步数 distance 就沿该方向走一格并减去 distance。
              全程整数运算、无除法无浮点，适合没有 FPU 压力的场合。
              incx/incy 先判断增量为 0 的情况（垂直线/水平线），
              避免出现 -0 之类的方向错误。
******************************************************************************/
void LCD_DrawLine(u16 x1,u16 y1,u16 x2,u16 y2,u16 color)
{
	u16 t;
	int xerr=0,yerr=0,delta_x,delta_y,distance;
	int incx,incy,uRow,uCol;
	delta_x=x2-x1; //计算坐标增量
	delta_y=y2-y1;
	uRow=x1;//画线起点坐标
	uCol=y1;
	if(delta_x>0)incx=1; //设置单步方向
	else if (delta_x==0)incx=0;//垂直线
	else {incx=-1;delta_x=-delta_x;}
	if(delta_y>0)incy=1;
	else if (delta_y==0)incy=0;//水平线
	else {incy=-1;delta_y=-delta_y;}
	if(delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴
	else distance=delta_y;
	for(t=0;t<distance+1;t++)
	{
		LCD_DrawPoint(uRow,uCol,color);//画点
		xerr+=delta_x;
		yerr+=delta_y;
		if(xerr>distance)
		{
			xerr-=distance;
			uRow+=incx;
		}
		if(yerr>distance)
		{
			yerr-=distance;
			uCol+=incy;
		}
	}
}


/******************************************************************************
      函数说明：画矩形
      入口数据：x1,y1   起始坐标
                x2,y2   终止坐标
                color   矩形的颜色
      @brief  画矩形边框（四条边，内部不填充）
      @param  x1,y1  左上角坐标
      @param  x2,y2  右下角坐标
      @param  color  矩形的颜色
      @retval 无
      @note   四条边分别画，四个角会被画两次（颜色相同，无副作用）。
              坐标是闭区间，且不含 OFFSET_Y。
******************************************************************************/
void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2,u16 color)
{
	LCD_DrawLine(x1,y1,x2,y1,color);
	LCD_DrawLine(x1,y1,x1,y2,color);
	LCD_DrawLine(x1,y2,x2,y2,color);
	LCD_DrawLine(x2,y1,x2,y2,color);
}


/******************************************************************************
      函数说明：画圆
      入口数据：x0,y0   圆心坐标
                r       半径
                color   圆的颜色
      @brief  画圆（中点画圆法，只算 1/8 圆弧，靠对称补全）
      @param  x0,y0  圆心坐标
      @param  r      半径
      @param  color  圆的颜色
      @retval 无
      @note   a 从 0 走到 b，每步画 8 个对称点（0~7 注释标的就是八个卦限）。
              `(a*a+b*b) > (r*r)` 时 b--，把半径误差压在一个像素内；
              全部是整数运算，没有开方。
              注意 r 是 u8，半径最大 255，超出会被截断。
******************************************************************************/
void Draw_Circle(u16 x0,u16 y0,u8 r,u16 color)
{
	int a,b;
	a=0;b=r;
	while(a<=b)
	{
		LCD_DrawPoint(x0-b,y0-a,color);             //3
		LCD_DrawPoint(x0+b,y0-a,color);             //0
		LCD_DrawPoint(x0-a,y0+b,color);             //1
		LCD_DrawPoint(x0-a,y0-b,color);             //2
		LCD_DrawPoint(x0+b,y0+a,color);             //4
		LCD_DrawPoint(x0+a,y0-b,color);             //5
		LCD_DrawPoint(x0+a,y0+b,color);             //6
		LCD_DrawPoint(x0-b,y0+a,color);             //7
		a++;
		if((a*a+b*b)>(r*r))//判断要画的点是否过远
		{
			b--;
		}
	}
}

/******************************************************************************
      函数说明：显示汉字串
      入口数据：x,y显示坐标
                *s 要显示的汉字串
                fc 字的颜色
                bc 字的背景色
                sizey 字号 可选 16 24 32
                mode:  0非叠加模式  1叠加模式
      @brief  显示 GBK 汉字串（按字号分派到 12/16/24/32 点阵）
      @param  x,y    显示坐标（左上角）
      @param  s      汉字串，GBK 双字节编码，以 0 结尾
      @param  fc     字的颜色
      @param  bc     字的背景色
      @param  sizey  字号，只支持 12/16/24/32
      @param  mode   0 非叠加（写背景色）1 叠加（背景透出）
      @retval 无
      @note   一个汉字占 2 字节，所以循环里 s += 2、x += sizey。
              sizey 不是 12/16/24/32 时直接 return，后面的字符全部不显示
              （不是跳过，是整个函数退出）。
              不检查 s 是否为空指针，调用者要保证。
******************************************************************************/
void LCD_ShowChinese(u16 x,u16 y,u8 *s,u16 fc,u16 bc,u8 sizey,u8 mode)
{
	while(*s!=0)
	{
		if(sizey==12) LCD_ShowChinese12x12(x,y,s,fc,bc,sizey,mode);
		else if(sizey==16) LCD_ShowChinese16x16(x,y,s,fc,bc,sizey,mode);
		else if(sizey==24) LCD_ShowChinese24x24(x,y,s,fc,bc,sizey,mode);
		else if(sizey==32) LCD_ShowChinese32x32(x,y,s,fc,bc,sizey,mode);
		else return;
		s+=2;
		x+=sizey;
	}
}

/******************************************************************************
      函数说明：显示单个12x12汉字
      入口数据：x,y显示坐标
                *s 要显示的汉字
                fc 字的颜色
                bc 字的背景色
                sizey 字号
                mode:  0非叠加模式  1叠加模式
      @brief  显示单个 12x12 汉字（字模来自 lcdfont.h 的 tfont12）
      @param  x,y    显示坐标（左上角）
      @param  s      指向该汉字 2 字节 GBK 编码
      @param  fc     字的颜色
      @param  bc     字的背景色
      @param  sizey  字号（这里固定按 12 处理）
      @param  mode   0 非叠加 1 叠加
      @retval 无
      @note   取模格式：每个字节的 bit0~bit7 对应从左到右 8 个像素，
              所以内层 `0x01<<j` 是从左往右扫。
              TypefaceNum = (12/8 + 1)*12 = 24 字节，正好 12 行 x 2 字节。
              非叠加模式：每个像素都写（前景 fc / 背景 bc），m%sizey 控制换行；
              叠加模式：只画前景点，背景保留，x/y 会被就地修改。
              坑：循环末尾的 `continue` 与注释“立即退出”不符——它并不会跳出
              字库扫描，只是跳过本轮。若字库里存在重复编码，同一个字会被
              重复绘制到错位的位置；正确写法应是 break。
******************************************************************************/
void LCD_ShowChinese12x12(u16 x,u16 y,u8 *s,u16 fc,u16 bc,u8 sizey,u8 mode)
{
	u8 i,j,m=0;
	u16 k;
	u16 HZnum;//汉字数目
	u16 TypefaceNum;//一个字符所占字节大小
	u16 x0=x;
	TypefaceNum=(sizey/8+((sizey%8)?1:0))*sizey;

	HZnum=sizeof(tfont12)/sizeof(typFNT_GB12);	//统计汉字数目
	for(k=0;k<HZnum;k++)
	{
		if((tfont12[k].Index[0]==*(s))&&(tfont12[k].Index[1]==*(s+1)))
		{
			LCD_Address_Set(x,y,x+sizey-1,y+sizey-1);
			for(i=0;i<TypefaceNum;i++)
			{
				for(j=0;j<8;j++)
				{
					if(!mode)//非叠加方式
					{
						if(tfont12[k].Msk[i]&(0x01<<j))LCD_WR_DATA(fc);
						else LCD_WR_DATA(bc);
						m++;
						if(m%sizey==0)
						{
							m=0;
							break;
						}
					}
					else//叠加方式
					{
						if(tfont12[k].Msk[i]&(0x01<<j))	LCD_DrawPoint(x,y,fc);//画一个点
						x++;
						if((x-x0)==sizey)
						{
							x=x0;
							y++;
							break;
						}
					}
				}
			}
		}
		continue;  //查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
	}
}

/******************************************************************************
      函数说明：显示单个16x16汉字
      入口数据：x,y显示坐标
                *s 要显示的汉字
                fc 字的颜色
                bc 字的背景色
                sizey 字号
                mode:  0非叠加模式  1叠加模式
      @brief  显示单个 16x16 汉字（字模来自 tfont16）
      @param  x,y    显示坐标（左上角）
      @param  s      指向该汉字 2 字节 GBK 编码
      @param  fc     字的颜色
      @param  bc     字的背景色
      @param  sizey  字号（这里固定按 16 处理）
      @param  mode   0 非叠加 1 叠加
      @retval 无
      @note   结构与 12x12 完全一样，只是字库换成 tfont16、TypefaceNum 变成
              (16/8)*16 = 32 字节。末尾同样用 continue 而非 break，
              存在“重复编码重复绘制”的隐患。
******************************************************************************/
void LCD_ShowChinese16x16(u16 x,u16 y,u8 *s,u16 fc,u16 bc,u8 sizey,u8 mode)
{
	u8 i,j,m=0;
	u16 k;
	u16 HZnum;//汉字数目
	u16 TypefaceNum;//一个字符所占字节大小
	u16 x0=x;
  TypefaceNum=(sizey/8+((sizey%8)?1:0))*sizey;
	HZnum=sizeof(tfont16)/sizeof(typFNT_GB16);	//统计汉字数目
	for(k=0;k<HZnum;k++)
	{
		if ((tfont16[k].Index[0]==*(s))&&(tfont16[k].Index[1]==*(s+1)))
		{
			LCD_Address_Set(x,y,x+sizey-1,y+sizey-1);
			for(i=0;i<TypefaceNum;i++)
			{
				for(j=0;j<8;j++)
				{
					if(!mode)//非叠加方式
					{
						if(tfont16[k].Msk[i]&(0x01<<j))LCD_WR_DATA(fc);
						else LCD_WR_DATA(bc);
						m++;
						if(m%sizey==0)
						{
							m=0;
							break;
						}
					}
					else//叠加方式
					{
						if(tfont16[k].Msk[i]&(0x01<<j))	LCD_DrawPoint(x,y,fc);//画一个点
						x++;
						if((x-x0)==sizey)
						{
							x=x0;
							y++;
							break;
						}
					}
				}
			}
		}
		continue;  //查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
	}
}


/******************************************************************************
      函数说明：显示单个24x24汉字
      入口数据：x,y显示坐标
                *s 要显示的汉字
                fc 字的颜色
                bc 字的背景色
                sizey 字号
                mode:  0非叠加模式  1叠加模式
      @brief  显示单个 24x24 汉字（字模来自 tfont24）
      @param  x,y    显示坐标（左上角）
      @param  s      指向该汉字 2 字节 GBK 编码
      @param  fc     字的颜色
      @param  bc     字的背景色
      @param  sizey  字号（这里固定按 24 处理）
      @param  mode   0 非叠加 1 叠加
      @retval 无
      @note   TypefaceNum = (24/8)*24 = 72 字节。24 不是 8 的整数倍也没关系，
              因为每行正好 3 字节。其余逻辑与 12x12 相同（含 continue 的坑）。
******************************************************************************/
void LCD_ShowChinese24x24(u16 x,u16 y,u8 *s,u16 fc,u16 bc,u8 sizey,u8 mode)
{
	u8 i,j,m=0;
	u16 k;
	u16 HZnum;//汉字数目
	u16 TypefaceNum;//一个字符所占字节大小
	u16 x0=x;
	TypefaceNum=(sizey/8+((sizey%8)?1:0))*sizey;
	HZnum=sizeof(tfont24)/sizeof(typFNT_GB24);	//统计汉字数目
	for(k=0;k<HZnum;k++)
	{
		if ((tfont24[k].Index[0]==*(s))&&(tfont24[k].Index[1]==*(s+1)))
		{
			LCD_Address_Set(x,y,x+sizey-1,y+sizey-1);
			for(i=0;i<TypefaceNum;i++)
			{
				for(j=0;j<8;j++)
				{
					if(!mode)//非叠加方式
					{
						if(tfont24[k].Msk[i]&(0x01<<j))LCD_WR_DATA(fc);
						else LCD_WR_DATA(bc);
						m++;
						if(m%sizey==0)
						{
							m=0;
							break;
						}
					}
					else//叠加方式
					{
						if(tfont24[k].Msk[i]&(0x01<<j))	LCD_DrawPoint(x,y,fc);//画一个点
						x++;
						if((x-x0)==sizey)
						{
							x=x0;
							y++;
							break;
						}
					}
				}
			}
		}
		continue;  //查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
	}
}

/******************************************************************************
      函数说明：显示单个32x32汉字
      入口数据：x,y显示坐标
                *s 要显示的汉字
                fc 字的颜色
                bc 字的背景色
                sizey 字号
                mode:  0非叠加模式  1叠加模式
      @brief  显示单个 32x32 汉字（字模来自 tfont32）
      @param  x,y    显示坐标（左上角）
      @param  s      指向该汉字 2 字节 GBK 编码
      @param  fc     字的颜色
      @param  bc     字的背景色
      @param  sizey  字号（这里固定按 32 处理）
      @param  mode   0 非叠加 1 叠加
      @retval 无
      @note   TypefaceNum = (32/8)*32 = 128 字节，是四个函数里最占 flash 的。
              32x32 的非叠加模式一次要写 1024 个像素，逐像素 SPI 会比较慢，
              开机画面里不要大面积使用。
******************************************************************************/
void LCD_ShowChinese32x32(u16 x,u16 y,u8 *s,u16 fc,u16 bc,u8 sizey,u8 mode)
{
	u8 i,j,m=0;
	u16 k;
	u16 HZnum;//汉字数目
	u16 TypefaceNum;//一个字符所占字节大小
	u16 x0=x;
	TypefaceNum=(sizey/8+((sizey%8)?1:0))*sizey;
	HZnum=sizeof(tfont32)/sizeof(typFNT_GB32);	//统计汉字数目
	for(k=0;k<HZnum;k++)
	{
		if ((tfont32[k].Index[0]==*(s))&&(tfont32[k].Index[1]==*(s+1)))
		{
			LCD_Address_Set(x,y,x+sizey-1,y+sizey-1);
			for(i=0;i<TypefaceNum;i++)
			{
				for(j=0;j<8;j++)
				{
					if(!mode)//非叠加方式
					{
						if(tfont32[k].Msk[i]&(0x01<<j))LCD_WR_DATA(fc);
						else LCD_WR_DATA(bc);
						m++;
						if(m%sizey==0)
						{
							m=0;
							break;
						}
					}
					else//叠加方式
					{
						if(tfont32[k].Msk[i]&(0x01<<j))	LCD_DrawPoint(x,y,fc);//画一个点
						x++;
						if((x-x0)==sizey)
						{
							x=x0;
							y++;
							break;
						}
					}
				}
			}
		}
		continue;  //查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
	}
}


/******************************************************************************
      函数说明：显示单个字符
      入口数据：x,y显示坐标
                num 要显示的字符
                fc 字的颜色
                bc 字的背景色
                sizey 字号
                mode:  0非叠加模式  1叠加模式
      @brief  显示单个 ASCII 字符（12/16/24/32 点阵）
      @param  x,y    显示坐标（左上角）
      @param  num    要显示的字符（ASCII）
      @param  fc     字的颜色
      @param  bc     字的背景色
      @param  sizey  字号，只支持 12/16/24/32
      @param  mode   0 非叠加 1 叠加
      @retval 无
      @note   ASCII 字模的宽是高的两倍（sizex = sizey/2），所以字库按
              6x12 / 8x16 / 12x24 / 16x32 组织。
              `num = num - ' '` 把 ASCII 码映射成字库下标：字库第 0 个是空格，
              所以传入控制字符（<0x20）会越界——调用者保证只传可见字符。
              sizey 不合法时直接 return；此时窗口已经设过，但一个像素都没写。
******************************************************************************/
void LCD_ShowChar(u16 x,u16 y,u8 num,u16 fc,u16 bc,u8 sizey,u8 mode)
{
	u8 temp,sizex,t,m=0;
	u16 i,TypefaceNum;//一个字符所占字节大小
	u16 x0=x;
	sizex=sizey/2;
	TypefaceNum=(sizex/8+((sizex%8)?1:0))*sizey;
	num=num-' ';    //得到偏移后的值
	LCD_Address_Set(x,y,x+sizex-1,y+sizey-1);  //设置光标位置
	for(i=0;i<TypefaceNum;i++)
	{
		if(sizey==12)temp=ascii_1206[num][i];		       //调用6x12字体
		else if(sizey==16)temp=ascii_1608[num][i];		 //调用8x16字体
		else if(sizey==24)temp=ascii_2412[num][i];		 //调用12x24字体
		else if(sizey==32)temp=ascii_3216[num][i];		 //调用16x32字体
		else return;
		for(t=0;t<8;t++)
		{
			if(!mode)//非叠加模式
			{
				if(temp&(0x01<<t))LCD_WR_DATA(fc);
				else LCD_WR_DATA(bc);
				m++;
				if(m%sizex==0)
				{
					m=0;
					break;
				}
			}
			else//叠加模式
			{
				if(temp&(0x01<<t))LCD_DrawPoint(x,y,fc);//画一个点
				x++;
				if((x-x0)==sizex)
				{
					x=x0;
					y++;
					break;
				}
			}
		}
	}
}


/******************************************************************************
      函数说明：显示字符串
      入口数据：x,y显示坐标
                *p 要显示的字符串
                fc 字的颜色
                bc 字的背景色
                sizey 字号
                mode:  0非叠加模式  1叠加模式
      @brief  显示 ASCII 字符串
      @param  x,y    显示坐标（左上角）
      @param  p      以 '\0' 结尾的字符串
      @param  fc     字的颜色
      @param  bc     字的背景色
      @param  sizey  字号
      @param  mode   0 非叠加 1 叠加
      @retval 无
      @note   每个字符宽 sizey/2，所以 x 每次加 sizey/2。
              不做屏幕边界检查：字符串太长会超出窗口，ST7789 会按自己的
              地址计数器继续写（表现为折行/错位），需要调用者自己控制长度。
******************************************************************************/
void LCD_ShowString(u16 x,u16 y,const u8 *p,u16 fc,u16 bc,u8 sizey,u8 mode)
{
	while(*p!='\0')
	{
		LCD_ShowChar(x,y,*p,fc,bc,sizey,mode);
		x+=sizey/2;
		p++;
	}
}


/******************************************************************************
      函数说明：显示数字
      入口数据：m底数，n指数
      @brief  计算 m 的 n 次方（整数幂）
      @param  m 底数
      @param  n 指数
      @retval m^n（u32）
      @note   LCD_ShowIntNum/LCD_ShowFloatNum1 用它取每一位的权值
              （10^(len-t-1)）。n=0 时返回 1。
              没有任何溢出保护，n 或 m 太大结果会回绕。
******************************************************************************/
u32 mypow(u8 m,u8 n)
{
	u32 result=1;
	while(n--)result*=m;
	return result;
}


/******************************************************************************
      函数说明：显示整数变量
      入口数据：x,y显示坐标
                num 要显示整数变量
                len 要显示的位数
                fc 字的颜色
                bc 字的背景色
                sizey 字号
      @brief  显示无符号整数（定宽，前导零显示为空格）
      @param  x,y    显示坐标
      @param  num    要显示的整数（u16，最大 65535）
      @param  len    要显示的位数
      @param  fc     字的颜色
      @param  bc     字的背景色
      @param  sizey  字号
      @retval 无
      @note   从最高位往低位取：temp = (num / 10^(len-t-1)) % 10。
              enshow 是“已经遇到第一个非零位”的标志：在此之前 0 都画成空格，
              所以 42 用 len=4 显示是 "  42" 而不是 "0042"。
              最后一位（t == len-1）不再判断，保证 0 本身能显示出一个 0。
              固定用非叠加模式（mode 参数传 0）。
******************************************************************************/
void LCD_ShowIntNum(u16 x,u16 y,u16 num,u8 len,u16 fc,u16 bc,u8 sizey)
{
	u8 t,temp;
	u8 enshow=0;
	u8 sizex=sizey/2;
	for(t=0;t<len;t++)
	{
		temp=(num/mypow(10,len-t-1))%10;
			if(enshow==0&&t<(len-1))
			{
				if(temp==0)
				{
					LCD_ShowChar(x+t*sizex,y,' ',fc,bc,sizey,0); // 前导零 → 空格
					continue;
				}else enshow=1; // 遇到第一个非零位，后面的 0 正常显示

		}
	 	LCD_ShowChar(x+t*sizex,y,temp+48,fc,bc,sizey,0);
	}
}


/******************************************************************************
      函数说明：显示两位小数变量
      入口数据：x,y显示坐标
                num 要显示小数变量
                len 要显示的位数
                fc 字的颜色
                bc 字的背景色
                sizey 字号
      @brief  显示保留两位小数的浮点数
      @param  x,y    显示坐标
      @param  num    要显示的浮点数
      @param  len    总字符数（**含小数点**）
      @param  fc     字的颜色
      @param  bc     字的背景色
      @param  sizey  字号
      @retval 无
      @note   num1 = num*100 把两位小数搬到整数部分，再逐位取数字，
              这样完全避开浮点取模。
              t == len-2 时在倒数第二位前插入小数点，同时 t++、len++，
              保证后面还能继续画完剩余位数。
              两个坑：num*100 是**截断**不是四舍五入（1.239 显示成 1.23）；
              num1 是 u16，num 超过约 655 就会溢出。
******************************************************************************/
void LCD_ShowFloatNum1(u16 x,u16 y,float num,u8 len,u16 fc,u16 bc,u8 sizey)
{
	u8 t,temp,sizex;
	u16 num1;
	sizex=sizey/2;
	num1=num*100;
	for(t=0;t<len;t++)
	{
		temp=(num1/mypow(10,len-t-1))%10;
		if(t==(len-2))
		{
			LCD_ShowChar(x+(len-2)*sizex,y,'.',fc,bc,sizey,0); // 倒数第二位前插小数点
			t++;
			len+=1; // 多出一个字符位置，循环才能画完后面的位
		}
	 	LCD_ShowChar(x+t*sizex,y,temp+48,fc,bc,sizey,0);
	}
}


/******************************************************************************
      函数说明：显示图片
      入口数据：x,y起点坐标
                length 图片长度
                width  图片宽度
                pic[]  图片数组
      @brief  显示一张 RGB565 图片
      @param  x,y     起点坐标（左上角）
      @param  length  图片横向像素数
      @param  width   图片纵向像素数
      @param  pic     图片数组，每像素 2 字节，**高字节在前**
      @retval 无
      @note   数组是“原始字节流”而不是 u16 数组，所以用 k*2 / k*2+1 取
              高/低字节，再分两次 LCD_WR_DATA8 发出去（等价于一次
              LCD_WR_DATA，但省掉拼字）。
              图片数据一般由取模软件生成并放在 pic.h，占的是 Flash；
              240x280 全屏图约 134 KB，F411 的 Flash 放不下几张。
              同样不加 OFFSET_Y。
******************************************************************************/
void LCD_ShowPicture(u16 x,u16 y,u16 length,u16 width,const u8 pic[])
{
	u16 i,j;
	u32 k=0;
	LCD_Address_Set(x,y,x+length-1,y+width-1);
	for(i=0;i<length;i++)
	{
		for(j=0;j<width;j++)
		{
			LCD_WR_DATA8(pic[k*2]);
			LCD_WR_DATA8(pic[k*2+1]);
			k++;
		}
	}
}


