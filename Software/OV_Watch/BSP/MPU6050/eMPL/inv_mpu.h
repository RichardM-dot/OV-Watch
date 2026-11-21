/*
 $License:
    Copyright (C) 2011-2012 InvenSense Corporation, All Rights Reserved.
    See included License.txt for License information.
 $
 */
/**
 *  @addtogroup  DRIVERS Sensor Driver Layer
 *  @brief       Hardware drivers to communicate with sensors via I2C.
 *
 *  @{
 *      @file       inv_mpu.h
 *      @brief      An I2C-based driver for Invensense gyroscopes.
 *      @details    This driver currently works for the following devices:
 *                  MPU6050
 *                  MPU6500
 *                  MPU9150 (or MPU6050 w/ AK8975 on the auxiliary bus)
 *                  MPU9250 (or MPU6500 w/ AK8963 on the auxiliary bus)
 */

#ifndef _INV_MPU_H_
#define _INV_MPU_H_
#include "sys.h"

//定义输出速度
#define DEFAULT_MPU_HZ  (100)		// DMP 默认输出频率 100Hz

#define INV_X_GYRO      (0x40)	// 陀螺仪 X 轴数据位（FIFO/传感器使能掩码）
#define INV_Y_GYRO      (0x20)	// 陀螺仪 Y 轴数据位
#define INV_Z_GYRO      (0x10)	// 陀螺仪 Z 轴数据位
#define INV_XYZ_GYRO    (INV_X_GYRO | INV_Y_GYRO | INV_Z_GYRO)	// 三轴陀螺仪
#define INV_XYZ_ACCEL   (0x08)	// 三轴加速度计
#define INV_XYZ_COMPASS (0x01)	// 三轴磁力计（外挂）

//移植官方MSP430 DMP驱动过来：中断引脚参数
struct int_param_s {
//#if defined EMPL_TARGET_MSP430 || defined MOTION_DRIVER_TARGET_MSP430
    void (*cb)(void);		// 中断回调函数（DMP 数据就绪时触发）
    unsigned short pin;		// 中断引脚编号
    unsigned char lp_exit;	// 是否低功耗退出
    unsigned char active_low;	// 中断极性：1 表示低电平有效
//#elif defined EMPL_TARGET_UC3L0
//    unsigned long pin;
//    void (*cb)(volatile void*);
//    void *arg;
//#endif
};

#define MPU_INT_STATUS_DATA_READY       (0x0001)
#define MPU_INT_STATUS_DMP              (0x0002)
#define MPU_INT_STATUS_PLL_READY        (0x0004)
#define MPU_INT_STATUS_I2C_MST          (0x0008)
#define MPU_INT_STATUS_FIFO_OVERFLOW    (0x0010)
#define MPU_INT_STATUS_ZMOT             (0x0020)
#define MPU_INT_STATUS_MOT              (0x0040)
#define MPU_INT_STATUS_FREE_FALL        (0x0080)
#define MPU_INT_STATUS_DMP_0            (0x0100)
#define MPU_INT_STATUS_DMP_1            (0x0200)
#define MPU_INT_STATUS_DMP_2            (0x0400)
#define MPU_INT_STATUS_DMP_3            (0x0800)
#define MPU_INT_STATUS_DMP_4            (0x1000)
#define MPU_INT_STATUS_DMP_5            (0x2000)

/* Set up APIs */
int mpu_init(void);					// 初始化 MPU6050（复位、读 ID、配置默认量程与采样率）
int mpu_init_slave(void);			// 初始化辅助 I2C 从机（挂接磁力计时使用）
int mpu_set_bypass(unsigned char bypass_on);	// 使能/关闭 I2C 旁路（旁路时可直接访问外挂磁力计）

/* Configuration APIs */
int mpu_lp_accel_mode(unsigned char rate);		// 进入低功耗加速度模式，rate 为采样率分频
int mpu_lp_motion_interrupt(unsigned short thresh, unsigned char time,
    unsigned char lpa_freq);		// 配置低功耗运动中断（阈值、持续时间、频率）
int mpu_set_int_level(unsigned char active_low);	// 设置中断电平极性
int mpu_set_int_latched(unsigned char enable);		// 设置中断是否锁存

int mpu_set_dmp_state(unsigned char enable);	// 启动/停止 DMP
int mpu_get_dmp_state(unsigned char *enabled);	// 查询 DMP 是否已启动

int mpu_get_lpf(unsigned short *lpf);		// 读取数字低通滤波带宽
int mpu_set_lpf(unsigned short lpf);		// 设置数字低通滤波带宽（Hz）

int mpu_get_gyro_fsr(unsigned short *fsr);	// 读取陀螺仪量程
int mpu_set_gyro_fsr(unsigned short fsr);	// 设置陀螺仪量程（±250/500/1000/2000 °/s）

int mpu_get_accel_fsr(unsigned char *fsr);	// 读取加速度量程
int mpu_set_accel_fsr(unsigned char fsr);	// 设置加速度量程（±2/4/8/16 g）

int mpu_get_compass_fsr(unsigned short *fsr);	// 读取磁力计量程

int mpu_get_gyro_sens(float *sens);			// 读取陀螺仪灵敏度（LSB/(°/s)）
int mpu_get_accel_sens(unsigned short *sens);	// 读取加速度灵敏度（LSB/g）

int mpu_get_sample_rate(unsigned short *rate);	// 读取采样率
int mpu_set_sample_rate(unsigned short rate);	// 设置采样率（Hz）
int mpu_get_compass_sample_rate(unsigned short *rate);	// 读取磁力计采样率
int mpu_set_compass_sample_rate(unsigned short rate);	// 设置磁力计采样率

int mpu_get_fifo_config(unsigned char *sensors);	// 读取 FIFO 中启用的传感器通道
int mpu_configure_fifo(unsigned char sensors);		// 配置 FIFO 使能的传感器通道

int mpu_get_power_state(unsigned char *power_on);	// 读取电源状态
int mpu_set_sensors(unsigned char sensors);			// 使能/关闭指定传感器（陀螺/加速度/磁力）

int mpu_set_accel_bias(const long *accel_bias);		// 设置加速度零偏校准值

/* Data getter/setter APIs */
int mpu_get_gyro_reg(short *data, unsigned long *timestamp);	// 读陀螺仪原始寄存器值
int mpu_get_accel_reg(short *data, unsigned long *timestamp);	// 读加速度原始寄存器值
int mpu_get_compass_reg(short *data, unsigned long *timestamp);	// 读磁力计原始值
int mpu_get_temperature(long *data, unsigned long *timestamp);	// 读温度原始值

int mpu_get_int_status(short *status);	// 读取中断状态
int mpu_read_fifo(short *gyro, short *accel, unsigned long *timestamp,
    unsigned char *sensors, unsigned char *more);	// 从 FIFO 取一组数据，more 表示是否还有剩余
int mpu_read_fifo_stream(unsigned short length, unsigned char *data,
    unsigned char *more);				// 按字节流方式读 FIFO
int mpu_reset_fifo(void);				// 复位 FIFO（清空并重新同步）

int mpu_write_mem(unsigned short mem_addr, unsigned short length,
    unsigned char *data);				// 写 DMP 内存
int mpu_read_mem(unsigned short mem_addr, unsigned short length,
    unsigned char *data);				// 读 DMP 内存
int mpu_load_firmware(unsigned short length, const unsigned char *firmware,
    unsigned short start_addr, unsigned short sample_rate);	// 加载 DMP 固件镜像

int mpu_reg_dump(void);					// 打印全部寄存器（调试用）
int mpu_read_reg(unsigned char reg, unsigned char *data);	// 读单个寄存器
int mpu_run_self_test(long *gyro, long *accel);	// 运行自检并返回零偏
int mpu_register_tap_cb(void (*func)(unsigned char, unsigned char));	// 注册敲击回调
//自行添加的一些函数
void mget_ms(unsigned long *time);		// 获取毫秒时间戳（移植时由用户实现）
unsigned short inv_row_2_scale(const signed char *row);	// 把安装方向行向量转换为方向标量
unsigned short inv_orientation_matrix_to_scalar(const signed char *mtx);	// 把安装方向矩阵转换为方向标量
u8 run_self_test(void);					// 执行自检（本工程封装）
u8 mpu_dmp_init(void);					// 初始化 DMP（加载固件、使能功能、设置速率）
u8 mpu_dmp_get_data(float *pitch,float *roll,float *yaw);	// 从 DMP 读取姿态角（俯仰/横滚/航向）

#endif  /* #ifndef _INV_MPU_H_ */

