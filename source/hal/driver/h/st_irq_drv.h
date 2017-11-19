/******************************************************************************
**
** Filename:     st_irq_drv.h
** Copyright:    
** Description:  该模块主要实现中断配置和管理
**
*******************************************************************************
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#ifndef  ST_IRQ_DRV_H
#define  ST_IRQ_DRV_H


#include "stm32f10x.h"


/* 定义中断向量个数 */
#define IRQ_ID_MAX           (16 + 68)

/* 中断优先级定义 */
#define IRQ_PRIOTITY_0       0
#define IRQ_PRIOTITY_1       1
#define IRQ_PRIOTITY_2       2
#define IRQ_PRIOTITY_3       3

/* irqid ch priotity in app */
#define  IRQ_PRIOTITY_SYSTICK     0, IRQ_PRIOTITY_3
#define  IRQ_PRIOTITY_TXDMA       0, IRQ_PRIOTITY_2
#define  IRQ_PRIOTITY_RXDMA       0, IRQ_PRIOTITY_2
#define  IRQ_PRIOTITY_UART        0, IRQ_PRIOTITY_2
#define  IRQ_PRIOTITY_EXTI        0, IRQ_PRIOTITY_1
#define  IRQ_PRIOTITY_CAN         0, IRQ_PRIOTITY_3
#define  TIM2_G_PRIOTITY      PREE_PR_LEVEL_0, SUB_PR_LEVEL_3
#define  USB_PRIOTITY         PREE_PR_LEVEL_1, SUB_PR_LEVEL_3

/* Exported macro ------------------------------------------------------------*/
typedef void(* IRQ_SERVICE_FUNC)(void) __irq;

extern IRQ_SERVICE_FUNC g_vector_tbl[IRQ_ID_MAX];


/* Exported functions ------------------------------------------------------- */

/*******************************************************************
** 函数名称: ST_IRQ_InstallIrqHandler
** 函数描述: 安装中断处理函数
** 参数:     [in] irqid :  中断号,见IRQn_Type
**           [in] handle : 中断处理函数
** 返回:     无
********************************************************************/
void ST_IRQ_InstallIrqHandler(INT32S irqid, IRQ_SERVICE_FUNC handle);

/*******************************************************************
** 函数名称: ST_IRQ_ConfigIrqPriority
** 函数描述: 配置中断优先级
** 参数:     [in] irqid :  中断号,见IRQn_Type
**           [in] pree_pr: 主优先级
**           [in] sub_pr:  子优先级
** 返回:     无
********************************************************************/
void ST_IRQ_ConfigIrqPriority(INT32S irqid, INT32U pree_pr, INT32U sub_pr);

/*******************************************************************
** 函数名称: ST_IRQ_ConfigIrqEnable
** 函数描述: 配置中断开启或关闭
** 参数:     [in] irqid :  中断号,见IRQn_Type
**           [in] enable:  TRUE:使能中断，false-关闭中断
** 返回:     无
********************************************************************/
void ST_IRQ_ConfigIrqEnable(INT32S irqid, BOOLEAN enable);


#endif

