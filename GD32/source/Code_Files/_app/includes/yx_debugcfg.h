/********************************************************************************
**
** 文件名:     yx_debugcfg.h
** 版权所有:   (c) 1998-2017 厦门雅迅网络股份有限公司
** 文件描述:   APP层调试打印模块配置头文件
**
*********************************************************************************
**                  修改历史记录
**===============================================================================
**| 日期              | 作者        |  修改记录
**===============================================================================
**| 2017/05/15 | 黄运峰    |  移植、修改、规范化
********************************************************************************/
#ifndef YX_DEBUGCFG_H
#define YX_DEBUGCFG_H       1

#include "port_uart.h"
#include "dal_include.h"
/*
*********************************************************************************
**                  定义调试模块配置参数
*********************************************************************************
*/
#define GLOBALS_DEBUG
#define DEBUG_USE_SYSTEM        0               // 1 用系统TRACE接口; 0 用指定的串口
#define UART_DBG_RBUF_LEN       512             // 调试串口接收缓冲大小
#define UART_DBG_TBUF_LEN       512             // 调试串口发送缓冲大小

// 注：定义成变量的形式，便于在APP层修改调试串口的参数
#ifdef  GLOBALS_DEBUG
//INT32U  DEBUG_UART_BAUD = 115200L;
//INT8U   DEBUG_UART_NO   = UART_IDX1;
#define  DEBUG_UART_BAUD   115200L
#define  DEBUG_UART_NO     UART_IDX3
#else
extern INT32U DEBUG_UART_BAUD;
extern INT8U DEBUG_UART_NO;
#endif

#define EN_CANDEBUG              0              //CAN调试开关
/*
*********************************************************
**              定义APP层调试开关
*********************************************************
*/
#if EN_DEBUG > 0
#define     DEBUG_ERR                   1
#define     DEBUG_INPUT_CMD             0       // 串口命令调试
#define     DEBUG_WIRELESS              0       // 无线下载
#define     DEBUG_COM_REC               0       //串口服务接收
#define     DEBUG_COM_SEND              0       //串口服务发送
#define     DEBUG_COM_LINK              0       //串口连接状态调试
#define     DEBUG_ADC_MAINPWR           1       //调试主电AD值
#define     DEBUG_SIGNAL_STATUS         0       //调试外部输入信号量
#define     DEBUG_CRASH_STATUS          0       //调试碰撞模块
#define     DEBUG_GSEN_STATUS           0       //调试GSENSOR工作状态
#define     DEBUG_PSEN_STATUS           0       //调试大气工作状态
#define     DEBUG_LIGHT_STATUS          0       //调试闪灯控制逻辑
#define     DEBUG_SLEEP_STATUS          0       //ACC休眠测试
#define     DEBUG_IOTEST_STATUS         0       //(yx_signal_man.c)开关量的打印信息开关
#define     DEBUG_DATA_STORE            0       //
#define     DEBUG_CAN                   0
#define     DEBUG_CANSFLFSEND           0
#define     DEBUG_LOCK                  0       //锁车程序调试
#define     DEBUG_RTC_SLEEP             0       //设备休眠RTC唤醒调试
#define     DEBUG_UDS                   0
#define     DEBUG_J1939                 0
#define     DEBUG_ENC                   0       //加密芯片
#define     DEBUG_ENC_TEST              0       //加密芯片过检定制
#define     DEBUG_EXIO                  0       //扩展IO控制
#define     DEBUG_EXUART                0       //扩展串口
#define     DEBUG_CAN_OTA               0
#define     DEBUG_BAT_ADC               0
#else

#define     DEBUG_ERR                   0
#define     DEBUG_INPUT_CMD             0       // 串口调试命令
#define     DEBUG_WIRELESS              0       // 无线下载
#define     DEBUG_COM_REC               0       //串口服务接收
#define     DEBUG_COM_SEND              0       //串口服务发送
#define     DEBUG_COM_LINK              0       //串口连接状态调试
#define     DEBUG_ADC_MAINPWR           0       //调试主电AD值
#define     DEBUG_SIGNAL_STATUS         0       //调试外部输入信号量
#define     DEBUG_CRASH_STATUS          0       //调试碰撞模块
#define     DEBUG_GSEN_STATUS           0       //调试GSENSOR工作状态
#define     DEBUG_PSEN_STATUS           0       //调试大气工作状态
#define     DEBUG_LIGHT_STATUS          0       //调试闪灯控制逻辑
#define     DEBUG_SLEEP_STATUS          0       //ACC休眠测试
#define     DEBUG_IOTEST_STATUS         0       //(yx_signal_man.c)开关量的打印信息开关
#define     DEBUG_DATA_STORE            0       //
#define     DEBUG_CAN                   0
#define     DEBUG_CANSFLFSEND           0
#define     DEBUG_LOCK                  0       //锁车程序调试
#define     DEBUG_RTC_SLEEP             0       //设备休眠RTC唤醒调试
#define     DEBUG_UDS                   0
#define     DEBUG_J1939                 0
#define     DEBUG_ENC                   0       //加密芯片
#define     DEBUG_ENC_TEST              0       //加密芯片过检定制
#define     DEBUG_EXIO                  0       //扩展IO控制
#define     DEBUG_EXUART                0       //扩展串口
#define     DEBUG_CAN_OTA               0
#define     DEBUG_BAT_ADC               0

#endif
/*
*********************************************************
**              定义BAL层调试开关
*********************************************************
*/
//#include "bal_debug_cfg.h"

//-------------------------------------------------------------------------------
#endif /* YX_DEBUGCFG_H */
