/**
 * @file    version.h
 * @brief   固件版本号与项目信息。
 *
 * 版本号规则（原作者约定）：
 *   - major：机械结构发生整体变更时递增
 *   - minor：硬件发生变更时递增
 *   - patch：纯软件修改时递增
 * watch_version_xxx() 是对应宏的只读包装，供界面"关于"页调用。
 */
#ifndef VERSION_H
#define VERSION_H

#ifdef __cplusplus
extern "C" {
#endif


/***************************
 * CURRENT VERSION OF OV-Watch
 ***************************/
/**
 * total mechanical structure change will show in major
 * hardware change will show in minor
 * software change will show in patch
 *
 */
#define VERSION_MAJOR 2	// 主版本号：机械结构变更
#define VERSION_MINOR 4	// 次版本号：硬件变更
#define VERSION_PATCH 5	// 修订号：软件修改
#define VERSION_INFO "LCD flush use synchronous DMA, fixed some bugs."	// 版本说明（显示在关于页）

/**
 * No-Chicken的意思是no spring chicken any more...
 *
 */
#define VERSION_AUTHOR_ENG_NAME    "No-Chicken"		// 作者英文名
#define VERSION_AUTHOR_CN_NAME     "不吃油炸鸡"		// 作者中文名
#define VERSION_PROJECT_LINK       "https://github.com/No-Chicken/OV-Watch"	// 项目仓库地址


/**
 * Wrapper functions for VERSION macros
 */

/** 获取主版本号 */
static inline int watch_version_major(void)
{
    return VERSION_MAJOR;
}

/** 获取次版本号 */
static inline int watch_version_minor(void)
{
    return VERSION_MINOR;
}

/** 获取修订号 */
static inline int watch_version_patch(void)
{
    return VERSION_PATCH;
}

/** 获取版本说明字符串 */
static inline const char *watch_version_info(void)
{
    return VERSION_INFO;
}

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
