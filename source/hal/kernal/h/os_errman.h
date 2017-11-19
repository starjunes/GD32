/******************************************************************************
**
** Filename:     os_errman.h
** Copyright:    
** Description:  该模块主要实现系统运行出错管理
**
-------------------------------------------------------------------------------
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#ifndef OS_ERRMAN_H
#define OS_ERRMAN_H


/*
********************************************************************************
* define config parameters
********************************************************************************
*/
#define RESET_REG_MAX        10
#define OS_MAX_DIAG          10


#define OS_ASSERT(EXPRESSION, retvalue)                       \
do {                                                          \
    if (!(EXPRESSION)) {                                      \
        OS_Reset(RESET_EVENT_ERR, __FILE__, __LINE__);        \
        return retvalue;                                      \
    }                                                         \
} while(0)


#define OS_RESET(EVENT) OS_Reset(EVENT, __FILE__, __LINE__)

/* 复位事件 */
typedef enum {
    RESET_EVENT_ERR,                   /* 出错异常复位 */
    RESET_EVENT_INITIATE,              /* 主动复位 */
    RESET_EVENT_DIRECT,                /* 直接复位，复位前不回调注册进来的复位通知函数 */
    RESET_EVENT_UPDATE,                /* 外设程序升级复位，如启动外设程序升级，外设程序升级完毕 */
    RESET_EVENT_UNREPORT,              /* 主动复位不上报, 用于唤醒是M0主动复位 */
    RESET_EVENT_MAX
} RESET_EVENT_E;

/* 复位回调优先级，0为最高优先级 */
typedef enum {
    RESET_PRI_0,
    RESET_PRI_1,
    RESET_PRI_MAX
} RESET_PRI_E;

/*******************************************************************
** 函数名:     OS_InitErrMan
** 函数描述:   模块初始化函数
** 参数:       无
** 返回:       无
********************************************************************/
void OS_InitErrMan(void);

/*******************************************************************
** 函数名称:   OS_RegistResetInform
** 函数描述:   注册复位通知接口
** 参数:       [in]: prior:待处理事件的优先级,见RESET_PRI_E
**			   [in]: fp   :回调函数
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN OS_RegistResetInform(INT8U prior, void (*fp)(INT8U event, char *filename, INT32U line));

/*******************************************************************
** 函数名:     OS_Reset
** 函数描述:   复位设备
** 参数:       [in] event：    复位事件
**             [in] pfile：    出错文件名
**             [in] line：     出错行号
** 返回:       无
********************************************************************/
void OS_Reset(INT8U event, char *filename, INT32U line);

/*******************************************************************
** 函数名:     OS_ErrTskEntry
** 函数描述:   错误处理任务入口函数
** 参数:       无
** 返回:       无
********************************************************************/
void OS_ErrTskEntry(void);

/*******************************************************************
** 函数名:     OS_RegistDiagnoseProc
** 函数描述:   注册一个诊断函数
** 参数:       [in]  diagproc: 待安装的诊断函数
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN OS_RegistDiagnoseProc(void (*diagproc)(void));


#endif

