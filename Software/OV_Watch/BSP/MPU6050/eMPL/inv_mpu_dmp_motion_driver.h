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
 *      @file       inv_mpu_dmp_motion_driver.h
 *      @brief      DMP image and interface functions.
 *      @details    All functions are preceded by the dmp_ prefix to
 *                  differentiate among MPL and general driver function calls.
 */
#ifndef _INV_MPU_DMP_MOTION_DRIVER_H_
#define _INV_MPU_DMP_MOTION_DRIVER_H_

/**
 * @file    inv_mpu_dmp_motion_driver.h
 * @brief   MPU6050 DMP 功能接口（InvenSense 官方驱动）。
 *
 * DMP 可硬件解算四元数、姿态方向、敲击手势、计步等，减轻 MCU 负担。
 * 使用流程一般为：
 *   mpu_init() → dmp_load_motion_driver_firmware() → dmp_enable_feature()
 *   → dmp_set_fifo_rate() → mpu_set_dmp_state(1) → dmp_read_fifo() 周期取数。
 */

#define TAP_X               (0x01)	// 敲击方向：X 轴
#define TAP_Y               (0x02)	// 敲击方向：Y 轴
#define TAP_Z               (0x04)	// 敲击方向：Z 轴
#define TAP_XYZ             (0x07)	// 敲击方向：三轴

#define TAP_X_UP            (0x01)	// X 轴向上敲击
#define TAP_X_DOWN          (0x02)	// X 轴向下敲击
#define TAP_Y_UP            (0x03)	// Y 轴向上敲击
#define TAP_Y_DOWN          (0x04)	// Y 轴向下敲击
#define TAP_Z_UP            (0x05)	// Z 轴向上敲击
#define TAP_Z_DOWN          (0x06)	// Z 轴向下敲击

#define ANDROID_ORIENT_PORTRAIT             (0x00)	// 竖屏
#define ANDROID_ORIENT_LANDSCAPE            (0x01)	// 横屏
#define ANDROID_ORIENT_REVERSE_PORTRAIT     (0x02)	// 反向竖屏
#define ANDROID_ORIENT_REVERSE_LANDSCAPE    (0x03)	// 反向横屏

#define DMP_INT_GESTURE     (0x01)	// 手势触发中断
#define DMP_INT_CONTINUOUS  (0x02)	// 连续（按 FIFO 速率）触发中断

#define DMP_FEATURE_TAP             (0x001)	// 敲击检测
#define DMP_FEATURE_ANDROID_ORIENT  (0x002)	// 安卓屏幕方向识别
#define DMP_FEATURE_LP_QUAT         (0x004)	// 低功耗四元数
#define DMP_FEATURE_PEDOMETER       (0x008)	// 计步器
#define DMP_FEATURE_6X_LP_QUAT      (0x010)	// 6 轴低功耗四元数
#define DMP_FEATURE_GYRO_CAL        (0x020)	// 陀螺仪自动校准
#define DMP_FEATURE_SEND_RAW_ACCEL  (0x040)	// FIFO 中输出原始加速度
#define DMP_FEATURE_SEND_RAW_GYRO   (0x080)	// FIFO 中输出原始陀螺仪
#define DMP_FEATURE_SEND_CAL_GYRO   (0x100)	// FIFO 中输出校准后陀螺仪

#define INV_WXYZ_QUAT       (0x100)	// 四元数格式标志（w,x,y,z 顺序）

/* Set up functions. */
int dmp_load_motion_driver_firmware(void);	// 把 DMP 固件镜像写入 MPU（成功返回 0）
int dmp_set_fifo_rate(unsigned short rate);	// 设置 DMP 输出率（Hz，最高 200）
int dmp_get_fifo_rate(unsigned short *rate);	// 读取 DMP 输出率
int dmp_enable_feature(unsigned short mask);	// 按位掩码使能 DMP 功能（DMP_FEATURE_xxx 组合）
int dmp_get_enabled_features(unsigned short *mask);	// 读取已使能的 DMP 功能掩码
int dmp_set_interrupt_mode(unsigned char mode);	// 设置 DMP 中断模式（手势/连续）
int dmp_set_orientation(unsigned short orient);	// 设置传感器安装方向（由方向矩阵换算）
int dmp_set_gyro_bias(long *bias);		// 设置陀螺仪零偏（消除静态漂移）
int dmp_set_accel_bias(long *bias);		// 设置加速度零偏

/* Tap functions. */
int dmp_register_tap_cb(void (*func)(unsigned char, unsigned char));	// 注册敲击回调（方向, 次数）
int dmp_set_tap_thresh(unsigned char axis, unsigned short thresh);	// 设置某轴敲击阈值
int dmp_set_tap_axes(unsigned char axis);	// 设置参与敲击检测的轴
int dmp_set_tap_count(unsigned char min_taps);	// 设置识别为敲击的最少连续次数
int dmp_set_tap_time(unsigned short time);	// 设置敲击判定时间窗（ms）
int dmp_set_tap_time_multi(unsigned short time);	// 设置多次敲击的时间间隔倍数
int dmp_set_shake_reject_thresh(long sf, unsigned short thresh);	// 设置防抖：抖动阈值
int dmp_set_shake_reject_time(unsigned short time);	// 设置防抖：抖动持续时间
int dmp_set_shake_reject_timeout(unsigned short time);	// 设置防抖：超时时间

/* Android orientation functions. */
int dmp_register_android_orient_cb(void (*func)(unsigned char));	// 注册屏幕方向变化回调

/* LP quaternion functions. */
int dmp_enable_lp_quat(unsigned char enable);	// 使能/关闭低功耗四元数输出
int dmp_enable_6x_lp_quat(unsigned char enable);	// 使能/关闭 6 轴低功耗四元数输出

/* Pedometer functions. */
int dmp_get_pedometer_step_count(unsigned long *count);	// 读取 DMP 累计步数
int dmp_set_pedometer_step_count(unsigned long count);	// 设置 DMP 计步初值
int dmp_get_pedometer_walk_time(unsigned long *time);	// 读取累计行走时间（ms）
int dmp_set_pedometer_walk_time(unsigned long time);	// 设置累计行走时间

/* DMP gyro calibration functions. */
int dmp_enable_gyro_cal(unsigned char enable);	// 使能/关闭 DMP 陀螺仪自动校准

/* Read function. This function should be called whenever the MPU interrupt is
 * detected.
 */
int dmp_read_fifo(short *gyro, short *accel, long *quat,
    unsigned long *timestamp, short *sensors, unsigned char *more);	// 读取 DMP FIFO：陀螺、加速度、四元数、时间戳

#endif  /* #ifndef _INV_MPU_DMP_MOTION_DRIVER_H_ */

