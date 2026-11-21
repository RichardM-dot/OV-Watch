/*
 * ============================================================================
 * 【中文导读】StrCalculate.c —— 栈式四则运算表达式求值（计算器页）
 * ----------------------------------------------------------------------------
 * 三个栈各司其职：
 *   StrStack_t 字符栈 —— 不参与求值，是 ui_ComputerPage 记录用户按键输入的缓冲区
 *                        （strput/strdel/strclear），按"="时把 strque 交给本文件；
 *   NumStack_t 数字栈 —— 存操作数（float，支持小数）；
 *   SymStack_t 符号栈 —— 存 + - * /，按优先级决定何时弹出计算。
 * 求值分两步：NumSymSeparate 扫描字符串，把连续数字（含小数）合并后压入数字栈；
 * 遇到运算符时，若"符号栈顶的优先级不低于当前运算符"就先弹栈算一次（这就是
 * Shunting-yard 中缀转后缀的思路）。CalculateOne 弹两个数、一个符号做一次运算，
 * 结果回压。StrCalculate 最后清空符号栈，剩下的唯一数字就是结果，调用方直接读
 * NumStack.data[NumStack.Top_Point-1]。
 * 优先级在 SymisHighPriority 里：* / 永远最高；+ 与 - 同级，设计意图是同级按
 * 左结合先算（实际连续减法存在右结合 bug，见清单第 2b 条）。
 * 已知问题集中列在紧随其后的"已知问题"注释块里，代码逻辑未做任何改动。
 * ============================================================================
 */

/*
 * ----------------------------------------------------------------------------
 * 【已知问题清单】（只做标注，不修改代码）
 *   1) NumStackDel/SymStackDel 写的是 data[Top_Point--]：先把"栈顶之上"的格子清零，
 *      再减指针；真正该弹出的是 data[Top_Point-1]，所以清错了元素。因为取数只认
 *      Top_Point，功能上"碰巧"还能跑，但留下脏数据，且首次调用会清到未使用格子。
 *   2) SymisHighPriority 在 top 不是 + - * / 时（初值 0、将来加括号等）没有 return，
 *      返回值未定义，属于 UB。
 *   2b) 连续减法被当成右结合：top='-'、present='-' 时返回 0（继续压栈），
 *      "8-2-3" 的求值顺序变成 8-(2-3)=9，正确结果应为 3。
 *   3) 不支持一元负号：NumSymSeparate 开头要求 str[0] 必须是数字，"-5+3" 直接返回 1。
 *   4) NumStackPut/SymStackPut 栈满（Top_Point==CAL_DEPTH-1）时返回 -1，调用处全部
 *      忽略，数字被静默丢弃、后续按残缺的栈继续算，结果错乱；而 (Top_Point >
 *      CAL_DEPTH) 这条溢出检查永远不会成立，属于死代码。
 *   5) 除零不检查：浮点除法得到 inf，后续 isIntNumber/printf 都会输出异常值。
 *   6) "1.2.3" 这类重复小数点检测不出来（第二个 '.' 前一位是数字就放行）。
 *   7) 以运算符结尾（"1+"）时，StrCalculate 收尾调用 CalculateOne 会读 NumStack.data[-1]。
 *   8) strput 的容量判断写成 15-1，但 StrStack_t.strque 只有 10 个字节，存在越界。
 * ----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../Inc/StrCalculate.h"

/**
  * @brief  字符入栈（供计算器页记录按键，不是求值用的数字栈）。
  * @param  st: 字符栈；strin: 要压入的字符
  * @note   这里用 Top_Point 做"下一个空位下标"，所以先写 strque[Top_Point] 再自增。
  *         注意容量判断写的是 15-1，而 strque 只有 10 字节——输入超过 10 个字符
  *         会越界写到 Top_Point 自身，属于已知问题，见文件顶部的清单。
  * @retval 0 成功，-1 栈满
  */
uint8_t strput(StrStack_t * st,char strin)
{
    if(st->Top_Point == 15 - 1)
    {return -1;}

    st->strque[st->Top_Point++] = strin;
    return 0;
}

/**
  * @brief  字符出栈（删除最后一个按键）。
  * @param  st: 字符栈
  * @note   先 --Top_Point 再清零，弹出的正是最后一个字符，逻辑正确；
  *         和下面 NumStackDel/SymStackDel 的写法形成对比（那两个写反了）。
  * @retval 0 成功，-1 栈空
  */
uint8_t strdel(StrStack_t * st)
{
    if(st->Top_Point == 0)
    {return -1;}

    st->strque[--st->Top_Point] = NULL;
    return 0;
}

/**
  * @brief  判断字符栈是否为空。
  * @param  st: 字符栈
  * @retval 1 空，0 非空
  */
uint8_t strstack_isEmpty(StrStack_t* st)
{
    if(st->Top_Point == 0)
    {return 1;}

    return 0;
}

/**
  * @brief  清空字符栈：反复出栈直到空。
  * @param  sq: 字符栈
  * @note   逐个 strdel 而不是 Top_Point=0，是为了把缓冲区内容也抹成 0，
  *         这样 strque 直接当 C 字符串用（strlen）时不会读到上次的残留。
  * @retval None
  */
void strclear(StrStack_t* sq)
{
    while(!strstack_isEmpty(sq))
    {
        strdel(sq);
    }
}

/**
  * @brief  数字入栈。
  * @param  st: 数字栈；in: 操作数（float，允许小数）
  * @note   栈满判断是 Top_Point == CAL_DEPTH-1（CAL_DEPTH=5），相当于预留一格，
  *         实际最多只能放 4 个数；返回 -1 表示压不进去，但所有调用处都没检查。
  * @retval 0 成功，-1 栈满
  */
uint8_t NumStackPut(NumStack_t * st, float in)
{
   if(st->Top_Point == CAL_DEPTH - 1)
	{return -1;}

	st->data[st->Top_Point++] = in;
	return 0;
}

/**
  * @brief  数字出栈（弹一个操作数）。
  * @param  st: 数字栈
  * @note   【已知问题】data[st->Top_Point--] 是先取 data[Top_Point] 再自减：真正
  *         的栈顶是 data[Top_Point-1]，所以清零的是"栈顶之上"那格。功能上因为
  *         取数只看 Top_Point，结果仍正确，但会留下未清零的脏数据、并让第一次
  *         出栈清到数组外的下一个元素（NumStack_t 里紧跟 Top_Point）。
  *         正确写法应为 data[--st->Top_Point] = 0。
  * @retval 0 成功，-1 栈空
  */
uint8_t NumStackDel(NumStack_t * st)
{
    if(st->Top_Point == 0)
	{return -1;}
    st->data[st->Top_Point--] = 0;
	return 0;
}

/**
  * @brief  判断数字栈是否为空。
  * @param  st: 数字栈
  * @retval 1 空，0 非空
  */
uint8_t NumStack_isEmpty(NumStack_t* st)
{
	if(st->Top_Point == 0)
	{return 1;}

	return 0;
}

/**
  * @brief  清空数字栈。
  * @param  st: 数字栈
  * @note   同样靠反复出栈，副作用见 NumStackDel 的已知问题。
  * @retval None
  */
void NumStackClear(NumStack_t* st)
{
	while(!NumStack_isEmpty(st))
	{
		NumStackDel(st);
	}
}

/**
  * @brief  符号入栈（+ - * /）。
  * @param  st: 符号栈；in: 运算符字符
  * @note   与数字栈同样预留一格，实际最多 4 个运算符。
  * @retval 0 成功，-1 栈满
  */
uint8_t SymStackPut(SymStack_t * st, char in)
{
	if(st->Top_Point == CAL_DEPTH - 1)
	{return -1;}

	st->data[st->Top_Point++] = in;
	return 0;
}

/**
  * @brief  符号出栈。
  * @param  st: 符号栈
  * @note   【已知问题】与 NumStackDel 相同的下标错位：data[Top_Point--] 清的是
  *         栈顶之上那格，真正弹出的 data[Top_Point-1] 反而没被清零。
  * @retval 0 成功，-1 栈空
  */
uint8_t SymStackDel(SymStack_t * st)
{
  if(st->Top_Point == 0)
	{return -1;}
    st->data[st->Top_Point--] = 0;
	return 0;
}

/**
  * @brief  判断符号栈是否为空。
  * @param  st: 符号栈
  * @retval 1 空，0 非空
  */
uint8_t SymStack_isEmpty(SymStack_t* st)
{
	if(st->Top_Point == 0)
	{return 1;}

	return 0;
}

/**
  * @brief  清空符号栈（求值结束/出错时调用）。
  * @param  st: 符号栈
  * @retval None
  */
void SymStackClear(SymStack_t* st)
{
	while(!SymStack_isEmpty(st))
	{
		SymStackDel(st);
	}
}

/**
  * @brief  判断符号栈顶 top 的优先级是否"不低于"当前运算符 present。
  * @param  top: 符号栈顶运算符；present: 正在扫描到的运算符
  * @note   规则：
  *           top 是 * 或 / → 永远先算（返回 1），因为乘除优先级最高；
  *           top 是 + 且 present 是 - → 返回 1（同级，左结合，先算栈顶）；
  *           top 是 - 且 present 是 + → 返回 1（同理）；
  *           其余情况返回 0，表示当前运算符直接入栈。
  *         【已知问题】top 为 '-'、present 也为 '-' 时返回 0，导致连续减法变成
  *         右结合："8-2-3" 会被算成 8-(2-3)=9 而不是 3；
  *         另外 top 不是 + - * / 时函数没有 return，返回值未定义。
  * @retval 1 栈顶优先（应先弹栈计算），0 当前运算符入栈
  */
uint8_t SymisHighPriority(char top, char present)
{
    //乘除的优先级最大
    if(top == '*' || top == '/')
    {
        return 1;
    }
    else if(top == '+')
    {
        if(present == '-')
        {return 1;}
        else
        {return 0;}
    }
    else if(top == '-')
    {
        if(present == '+')
        {return 1;}
        else
        {return 0;}
    }
}

/**
  * @brief  执行一次二元运算：取数字栈顶两个数、符号栈顶一个符号，结果回压。
  * @param  numstack: 数字栈；symstack: 符号栈
  * @note   顺序很关键：A 是 data[Top-2]（左操作数），B 是 data[Top-1]（右操作数），
  *         减法和除法不能交换。运算完两次 NumStackDel 弹出两数，再 Put 回结果，
  *         净效果是数字栈少一个元素；最后 SymStackDel 弹掉用过的符号。
  *         不做任何合法性检查：若符号不是四则运算符，temp.number 未初始化就被
  *         压栈；若数字不足两个，会读到 data[-1]（见文件顶部已知问题）。
  * @retval None
  */
void CalculateOne(NumStack_t * numstack, SymStack_t * symstack)
{
    caldata_t temp;
    temp.datatype = NUMBER_TYPE;
    temp.symbol = NULL;
    //计算数字栈中的顶部两数,结果存到temp中
    if(symstack->data[symstack->Top_Point-1] == '+')
        temp.number = (numstack->data[numstack->Top_Point-2]) + (numstack->data[numstack->Top_Point-1]);

    else if(symstack->data[symstack->Top_Point-1] == '-')
        temp.number = (numstack->data[numstack->Top_Point-2]) - (numstack->data[numstack->Top_Point-1]);

    else if(symstack->data[symstack->Top_Point-1] == '*')
        temp.number = (numstack->data[numstack->Top_Point-2]) * (numstack->data[numstack->Top_Point-1]);

    else if(symstack->data[symstack->Top_Point-1] == '/')
        temp.number = (numstack->data[numstack->Top_Point-2]) / (numstack->data[numstack->Top_Point-1]); // 除零不检查，浮点会得到 inf

    //运算前两数出栈,运算结果数入栈
    NumStackDel(numstack);
    NumStackDel(numstack);
    NumStackPut(numstack,temp.number);           // 结果回压，数字栈净减少一个元素
    SymStackDel(symstack);                       // 弹掉刚用过的运算符

}

/**
  * @brief  词法扫描：把字符串拆成"数字"和"运算符"，分别压入数字栈/符号栈。
  * @param  str: 表达式字符串（不一定以 '\0' 结尾，长度由 strlen 给出）
  * @param  strlen: 字符串长度（uint8_t，调用方传 size_t 会被截断）
  * @param  NumStack: 数字栈（输出）
  * @param  SymStack: 符号栈（输出）
  * @note   核心是 temp_pre（上一个记号的类型）：它决定本次读到的数字是"新数字"
  *         还是"上一位的进位/小数位"。NumBehindPoint_Flag 记录小数点后已有几位，
  *         用于把 "0.05" 这种逐位拼成 0.05。
  *         遇到运算符时先和符号栈顶比优先级：栈顶不比它低就先算一次再入栈，
  *         这就是中缀转后缀（Shunting-yard）的做法。
  * @retval 0 成功；1 首字符不是数字；2 小数点位置非法；3 数字栈溢出；
  *         4 符号栈溢出；5 连续输入了两个运算符
  */
uint8_t NumSymSeparate(char * str, uint8_t strlen, NumStack_t * NumStack, SymStack_t * SymStack)
{
		// 每次解析前先清空两个栈：调用方可能复用同一组栈结构（计算器页就是复用）
		NumStackClear(NumStack);
		SymStackClear(SymStack);
    caldata_t temp,temp_pre;
    char NumBehindPoint_Flag = 0;//数字是否在小数点后,后多少位
    temp.datatype = NUMBER_TYPE;
    temp.number = 0;
    temp.symbol = NULL;                          // 给 char 赋 NULL 等价于 0，写法不规范但无害
    temp_pre = temp;
    temp_pre.datatype = SYMBOL_TYPE;             // 假装"上一个记号是运算符"，这样第一个数字不会被当成进位
    if(str[0]>'9' || str[0]<'0')
        return 1;//erro
		int i;
    for(i=0;i<strlen;i++)
    {
        if(str[i]=='.')
        {
            // 小数点只能跟在数字后面；前面是运算符或另一个小数点都算非法
            temp.datatype = POINT_TYPE;
            if(temp_pre.datatype == NUMBER_TYPE)
            {}                                   // 合法：整数部分已经入栈，等小数位来合并
            else
            {return 2;}
            temp_pre = temp;                     // 记住"上一个是小数点"，下一位数字走小数分支
        }
        if(str[i]<='9' && str[i]>='0')
        {
            //溢出报错
            // 注意：判断用的是 > 而不是 >=，而且 Put 的 -1 返回值没检查，
            // 表达式过长时仍可能写越界（已知问题）
            if(NumStack->Top_Point>CAL_DEPTH || SymStack->Top_Point>CAL_DEPTH)
            {return 3;}
            //读取当前的字符到temp中
            temp.datatype = NUMBER_TYPE;
            temp.number = (str[i] - '0');
            temp.symbol = NULL;
            //如果为连续数字,需要进行进位,将数字栈顶读出进位，再加上现在位，再入栈
            if(temp_pre.datatype == NUMBER_TYPE)
            {
                if(!NumBehindPoint_Flag)
                {temp.number += NumStack->data[NumStack->Top_Point-1] * 10;} // 整数：旧值×10 + 新位
                else
                {
                    NumBehindPoint_Flag += 1;     // 小数点后又多一位
                    char i = NumBehindPoint_Flag; // 局部变量 i 遮蔽了循环变量 i，但这里不再用外层 i
                    while(i--)
                    {temp.number /= 10;}          // 新位按 10^-n 缩放，例如 0.05 的第二位要除 100
                    temp.number += NumStack->data[NumStack->Top_Point-1]; // 再叠到已入栈的部分上
                }
                NumStackDel(NumStack);            // 先弹旧值
                NumStackPut(NumStack,temp.number); // 再压合并后的值（净元素数不变）
            }
            //当前数字刚好是小数点后一位
            else if(temp_pre.datatype == POINT_TYPE)
            {
                NumBehindPoint_Flag = 1;          // 进入小数状态，后续位按 10^-n 处理
                temp.number /= 10;                // 例如 "0.5"：5/10 = 0.5
                temp.number += NumStack->data[NumStack->Top_Point-1]; // 加上整数部分 0
                NumStackDel(NumStack);
                NumStackPut(NumStack,temp.number);
            }
            //前一位不是数字或小数点,现在读取的这一位是数字，直接入栈
            else
            {
                NumStackPut(NumStack,temp.number); // 新操作数的首位，直接压栈
            }
            temp_pre = temp;
        }
        else if(str[i] == '+' || str[i] == '-' || str[i] == '*' || str[i] == '/')
        {
            //溢出报错
            if(NumStack->Top_Point>CAL_DEPTH || SymStack->Top_Point>CAL_DEPTH)
            {return 4;}
            //读取当前的字符到temp中
            temp.datatype = SYMBOL_TYPE;
            temp.symbol = str[i];
            temp.number = 0;
            NumBehindPoint_Flag = 0;//小数点计算已经结束
            //重复输入了运算符号
            // 这条判断也顺带禁止了一元负号：表达式不能以 '-' 开头，"-5+3" 会在这里被拒
            if(temp_pre.datatype == SYMBOL_TYPE)
            {
                return 5 ;//erro
            }
            else
            {
                // 栈顶优先级不低于当前运算符 → 先把栈顶那一步算掉（保证先乘除后加减）
                if((!SymStack_isEmpty(SymStack)) && SymisHighPriority(SymStack->data[SymStack->Top_Point-1],temp.symbol))
                {
                    CalculateOne(NumStack, SymStack);
                    SymStackPut(SymStack,temp.symbol); // 算完再把当前运算符压栈
                }
                else
                {
                    //符号压入符号栈
                    SymStackPut(SymStack,temp.symbol); // 栈顶优先级更低，当前运算符先等着
                }
                temp_pre = temp;
            }
        }
        // 其它字符（例如 "=" 塞进来的 '\n'）被静默忽略，不影响求值
    }
    return 0;
}

/**
  * @brief  对外接口：解析并求值一个四则运算表达式。
  * @param  str: 表达式字符串；NumStack/SymStack: 调用方提供的两个栈（可复用）
  * @note   先 NumSymSeparate 做词法+部分求值，再把符号栈里剩下的运算符逐个弹出
  *         算完。成功时结果留在 NumStack 栈顶（data[Top_Point-1]），本函数不返回
  *         数值本身。失败时清空两个栈并返回 -1，界面显示 "erro"。
  *         注意 strlen(str) 返回 size_t，传给 uint8_t 形参会截断，超长表达式不可靠。
  * @retval 0 成功；-1 解析失败（已清栈）
  */
uint8_t StrCalculate(char * str,NumStack_t * NumStack, SymStack_t * SymStack)
{
    if(NumSymSeparate(str,strlen(str),NumStack,SymStack))
    {
        //erro, clear all
        NumStackClear(NumStack);                 // 出错就清干净，避免脏数据影响下一次计算
        SymStackClear(SymStack);
        return -1;
    }
    else
    {
        // 符号栈非空说明还有没算完的运算符，从栈顶依次弹出（此时已是后缀顺序）
        while(!SymStack_isEmpty(SymStack))
        {
            CalculateOne(NumStack,SymStack);
        }
    }
    return 0;
}

/**
  * @brief  判断浮点数是不是整数（用于决定显示 "%.0f" 还是 "%.4f"）。
  * @param  number: 待判断的数
  * @note   直接把 float 强转 int 再比较：数值超出 int 范围或为负时行为不理想，
  *         但计算器页的数值范围很小，够用。
  * @retval 1 是整数，0 不是
  */
uint8_t isIntNumber(float number)
{
	if(number == (int)number)
	{return 1;}
	return 0;
}


