/**
 * @file    StrCalculate.h
 * @brief   四则运算表达式求值（供计算器页面使用）。
 *
 * 采用"数字栈 + 符号栈"双栈结构：
 *   1. NumSymSeparate 把输入字符串拆成数字/符号分别入栈；
 *   2. StrCalculate 按运算符优先级出栈计算，支持 + - * / 与小数。
 * 注意：本文件没有 include guard，请确保只被包含一次，或由包含方保证顺序。
 */
#include <stdint.h>

#define NUMBER_TYPE 0	// 记号类型：数字
#define SYMBOL_TYPE 1	// 记号类型：运算符
#define POINT_TYPE 3	// 记号类型：小数点
#define CAL_DEPTH 5		// 栈深度（最多支持 5 层运算）

/** 字符串栈：存放表达式中的字符，按后进先出处理 */
typedef struct
{
    char strque[10];		// 字符缓冲区（最多 10 个字符）
    uint8_t Top_Point;		// 栈顶下标

}StrStack_t;

/** 字符入栈；返回 0 表示成功，非 0 表示失败（栈满） */
uint8_t strput(StrStack_t * st,char strin);
/** 字符出栈；返回出栈的字符 */
uint8_t strdel(StrStack_t * st);
/** 判断字符串栈是否为空；非 0 表示空 */
uint8_t strstack_isEmpty(StrStack_t* st);
/** 清空字符串栈 */
void strclear(StrStack_t* sq);

/** 计算器记号：类型 + 数值/符号 */
typedef struct
{
    char datatype;	// 记号类型（NUMBER_TYPE / SYMBOL_TYPE / POINT_TYPE）
    float number;	// 数值（datatype 为 NUMBER_TYPE 时有效）
    char symbol;	// 运算符（datatype 为 SYMBOL_TYPE 时有效）
}caldata_t;

/** 数字栈：存放参与运算的数值 */
typedef struct
{
    float data[CAL_DEPTH];	// 数值缓冲区
    uint8_t Top_Point;		// 栈顶下标

}NumStack_t;

/** 符号栈：存放运算符 */
typedef struct
{
    char data[CAL_DEPTH];	// 运算符缓冲区
    uint8_t Top_Point;		// 栈顶下标

}SymStack_t;

/** 清空数字栈 */
void NumStackClear(NumStack_t* st);
/** 清空符号栈 */
void SymStackClear(SymStack_t* st);
/** 把字符串 str 拆分到数字栈与符号栈；返回 0 表示成功 */
uint8_t NumSymSeparate(char * str, uint8_t strlen, NumStack_t * NumStack, SymStack_t * SymStack);
/** 对已入栈的表达式求值；返回 0 表示成功 */
uint8_t StrCalculate(char * str, NumStack_t * NumStack, SymStack_t * SymStack);
/** 判断浮点数是否为整数；非 0 表示是整数 */
uint8_t isIntNumber(float number);

