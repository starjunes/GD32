/******************************************************************************
**
** Filename:     st_uart_drv.h
** Copyright:    
** Description:  该模块主要实现串口的驱动管理
**
*******************************************************************************
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#ifndef ST_UART_DRV_H
#define ST_UART_DRV_H


#include "st_uart_reg.h"
/*
********************************************************************************
* define config parameters
********************************************************************************
*/

/* 流控 */
typedef enum {
    UART_FCM_NULL = 0,         /* 无流控控制 */
    UART_FCM_XONXOFF,          /* 软件流控 Software Flow Control */
    UART_FCM_RTS,              /* 硬件流控 Hardware Flow Control */
    UART_FCM_MAX
} UART_FCM_E;

/* 串口数据位 */
typedef enum {
    UART_DATABIT_NULL = 0,
    UART_DATABIT_5,
    UART_DATABIT_6,
    UART_DATABIT_7,
    UART_DATABIT_8,
    UART_DATABIT_9,
    UART_DATABIT_MAX
} UART_DATABIT_E;

/* 串口停止位 */
typedef enum {
    UART_STOPBIT_NULL = 0,
    UART_STOPBIT_1,
    UART_STOPBIT_2,
    UART_STOPBIT_0_5,
    UART_STOPBIT_1_5,
    UART_STOPBIT_MAX
} UART_STOPBIT_E;

/* 串口校验位 */
typedef enum {
    UART_PARITY_NONE = 0,      /* 无校验 */
    UART_PARITY_EVEN,          /* 偶校验 */
    UART_PARITY_ODD,           /* 奇校验 */
    UART_PARITY_SPACE,         /* 校验位为0 */
    UART_PARITY_MARK,          /* 校验位为1 */
    UART_PARITY_MAX
} UART_PARITY_E;


/*
********************************************************************************
* define struct
********************************************************************************
*/
/* 串口配置参数 */
typedef struct {
    INT32U com;        /* 串口通道编号,见UART_COM_E */
    INT32U baud;       /* 波特率, 1200~115200 */
    INT32U parity;     /* 奇偶校验位, 见UART_PARITY_E */
    INT32U databit;    /* 数据位, 见UART_DATABIT_E */
    INT32U stopbit;    /* 停止位，见UART_STOPBIT_E */
    
    INT32U rx_fcm;     /* 接收流控模式,见UART_FCM_E */
    INT32U tx_fcm;     /* 发送流控模式,见UART_FCM_E */
    
    INT32U rx_len;     /* 配置平台接收缓存长度 */
    INT32U tx_len;     /* 配置平台发送缓存长度 */
} UART_CFG_T;



/*******************************************************************
** 函数名称:   ST_UART_OpenUart
** 函数描述:   打开串口并初始化
** 参数:       [in]  cfg:串口配置参数 
** 返回:       成功返回true,失败返回false
********************************************************************/
BOOLEAN ST_UART_OpenUart(UART_CFG_T *cfg);


/*******************************************************************
** 函数名:     ST_UART_CloseUart
** 函数描述:   关闭串口
** 参数:       [in] com: 通道编号,见UART_COM_E
** 返回:       成功返回true, 失败返回false
********************************************************************/
BOOLEAN ST_UART_CloseUart(INT8U com);

/*******************************************************************
** 函数名称: ST_UART_ReadChar
** 函数描述: 获取一个字节数据
** 参数:     [in] com: 通道编号,见UART_COM_E
** 返回:     成功返回串口数据，失败返回-1
********************************************************************/
INT32S ST_UART_ReadChar(INT8U com);

/*******************************************************************
** 函数名称: ST_UART_WriteChar
** 函数描述: 发送一个字节
** 参数:     [in] com: 通道编号,见UART_COM_E
**           [in] data:字节数据
** 返回:     成功返回true,失败返回false
********************************************************************/
BOOLEAN ST_UART_WriteChar(INT8U com, INT8U data);

/*******************************************************************
** 函数名称: ST_UART_WriteCharWait
** 函数描述: 发送一个字节,等待发送完毕
** 参数:     [in] com: 通道编号,见UART_COM_E
**           [in] data:字节数据
** 返回:     成功返回true,失败返回false
********************************************************************/
BOOLEAN ST_UART_WriteCharWait(INT8U com, INT8U data);

/*******************************************************************
** 函数名称: ST_UART_WriteBlock
** 函数描述: 发送一串数据
** 参数:     [in] com: 通道编号,见UART_COM_E
**           [in] sptr:数据指针
**           [in] slen:数据长度
** 返回:     成功返回true,失败返回false
********************************************************************/
BOOLEAN ST_UART_WriteBlock(INT8U com, INT8U *sptr, INT16U slen);

/*******************************************************************
** 函数名称: ST_UART_GetRecvBytes
** 函数描述: 获取已接收字节数
** 参数:     [in] com: 通道编号,见UART_COM_E
** 返回:     已接收字节数
********************************************************************/
INT32U ST_UART_GetRecvBytes(INT8U com);

/*******************************************************************
** 函数名称: ST_UART_LeftOfSendbuf
** 函数描述: 获取发送缓存剩余空间
** 参数:     [in] com: 通道编号,见UART_COM_E
** 返回:     剩余空间字节数
********************************************************************/
INT32U ST_UART_LeftOfSendbuf(INT8U com);

/*******************************************************************
** 函数名称: ST_UART_InitDrv
** 函数描述: 初始化串口驱动
** 参数:     无
** 返回:     无
********************************************************************/
void ST_UART_InitDrv(void);

/*******************************************************************
** 函数名称: UARTx_TxDMAIrqService
** 函数描述: tx dma irq handle for all uarts
** 参数:     [in] com: 通道编号,见UART_COM_E
** 返回:     无
********************************************************************/
void UARTx_TxDMAIrqService(INT8U com);

/*******************************************************************
** 函数名称: UARTx_RxDMAIrqService
** 函数描述: DMA接收中断处理
** 参数:     [in] com: 通道编号,见UART_COM_E
** 返回:     无
********************************************************************/
void UARTx_RxDMAIrqService(INT8U com);

/*******************************************************************
** 函数名称: Uartx_TxIrqService
** 函数描述: UART发送中断处理
** 参数:     [in] com:                 通道编号,见UART_COM_E
**           [in] uart_base:  uart寄存器基址
** 返回:     无
********************************************************************/
void Uartx_TxIrqService(INT8U com, INT32U uart_base);

/*******************************************************************
** 函数名称: Uartx_RxIrqService
** 函数描述: UART接收中断处理
** 参数:     [in] com:                 通道编号,见UART_COM_E
**           [in] uart_base:  uart寄存器基址
**           [in] data:                字节数据
** 返回:     无
********************************************************************/
void Uartx_RxIrqService(INT8U com, INT32U uart_base, INT16U data);


#endif

