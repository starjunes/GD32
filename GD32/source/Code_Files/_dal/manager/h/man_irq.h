/********************************************************************************
**
** 文件名:     man_irq.h
** 版权所有:   (c) 2014-2020 厦门雅迅网络股份有限公司
** 文件描述:   系统中断管理模块
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2010/12/30 | LEON   | 创建本模块
**
*********************************************************************************/
#ifndef __MAN_IRQ_H
#define __MAN_IRQ_H

#include "dal_include.h"

typedef void(* ExecFuncPtr)(void) __irq;

/*************************************************************************************************/
/*   注意:以下两个枚举表格的取值，与NVIC_Configuration中对中断优先级配置的类型相关，请注意关联   */
/*************************************************************************************************/

/*************************************************************************************************/
/*                           中断主优先级枚举配置表                                              */
/*************************************************************************************************/
typedef enum {
    PRE_PRI_FST = 0x00,
    PRE_PRI_SCD = 0x01,
    PRE_PRI_MAX = 0x02
} PRE_PRI_E;

/*************************************************************************************************/
/*                           中断次优先级枚举配置表                                              */
/*************************************************************************************************/
typedef enum {
    SUB_PRI_FST = 0x00,
    SUB_PRI_SCD = 0x01,
    SUB_PRI_THD = 0x02,
    SUB_PRI_FOR = 0x03,
    SUB_PRI_FIV = 0x04,
    SUB_PRI_SIX = 0x05,
    SUB_PRI_SEV = 0x06,
    SUB_PRI_MAX = 0x07
} SUB_PRI_E;

/*************************************************************************************************/
/*                           各个系统中断优先级定义                                              */
/*************************************************************************************************/
#define  EXTI_PRIOTITY        PRE_PRI_FST, SUB_PRI_FST               /* 外部中断（脉冲、红外检测） */
#define  UART_RX_PRIOTITY     PRE_PRI_FST, SUB_PRI_SCD               /* 串口接收 */
#define  UART_TXDMA_PRIOTITY  PRE_PRI_FST, SUB_PRI_THD               /* 串口发送 */
#define  INFRAID_PRIOTITY     PRE_PRI_FST, SUB_PRI_FOR               /* 脉冲、红外定时 */
#define  CAN_PRIOTITY         PRE_PRI_FST, SUB_PRI_THD               /* CAN */

#define  USB_PRIOTITY         PRE_PRI_SCD, SUB_PRI_FST               /* USB */
#define  PRN_PRIOTITY         PRE_PRI_SCD, SUB_PRI_SCD               /* 打印机 */
#define  SYSTICK_PRIOTITY     PRE_PRI_SCD, SUB_PRI_THD               /* 软件中断 */

#define  TIM3_G_PRIOTITY      PRE_PRI_FST, SUB_PRI_SCD               /* mmi_rfid(未用) */
#define  TIM5_G_PRIOTITY      PRE_PRI_FST, SUB_PRI_SCD               /* mmi_rfid(未用) */

/*************************************************************************************************/
/*                           以下定义一个中断号枚举结构                                          */
/*************************************************************************************************/
#ifdef BEGIN_SYS_IRQ_TABLE
#undef BEGIN_SYS_IRQ_TABLE
#endif

#ifdef ENDOF_SYS_IRQ_TABLE
#undef ENDOF_SYS_IRQ_TABLE
#endif

#ifdef DEF_SYS_IRQ
#undef DEF_SYS_IRQ
#endif

#define BEGIN_SYS_IRQ_TABLE
#define DEF_SYS_IRQ(_IRQ_NUMBER_, _IRQ_HANDLE_)            _IRQ_NUMBER_,
#define ENDOF_SYS_IRQ_TABLE

typedef enum {
    #include "man_irq.def"
    MAX_IRQ_NUMABER
} IRQ_NUMBER_E;

/*******************************************************************
** 函数名:     NVIC_IrqHandleInstall
** 函数描述:   向系统注册一个用户自定义中断处理函数
** 参数:       [in] irq: 中断号
**             [in] handle: 中断处理函数
**             [in] ppri: 主优先级
**             [in] spri: 次优先级
**             [in] enable: 是否使能
** 返回:       无
********************************************************************/
void NVIC_IrqHandleInstall(INT8U irq, ExecFuncPtr handle, PRE_PRI_E ppri, SUB_PRI_E spri, BOOLEAN enable);
#endif

