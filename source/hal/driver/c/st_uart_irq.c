/******************************************************************************
**
** Filename:     st_uart_irq.c
** Copyright:    
** Description:  该模块主要实现串口中断服务管理
**
*******************************************************************************
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#include "yx_include.h"
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "st_uart_reg.h"
#include "st_uart_irq.h"
#include "st_uart_drv.h"



/*
********************************************************************************
* define config parameters
********************************************************************************
*/


/*
********************************************************************************
* define struct
********************************************************************************
*/

/*
********************************************************************************
* define module variants
********************************************************************************
*/


/*******************************************************************
** 函数名称: DMA1_CHANNEL1_IrqHandle
** 函数描述: DMA1_CHANNEL1中断处理
** 参数:     无
** 返回:     无
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void DMA1_CHANNEL1_IrqHandle(void)
{
    ;
}

/*******************************************************************
** 函数名称: DMA1_CHANNEL2_IrqHandle
** 函数描述: DMA1_CHANNEL2中断处理(UART1 TX for DMA CH2/ UART1 RX for DMA CH3)
** 参数:     无
** 返回:     无
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void DMA1_CHANNEL2_IrqHandle(void)
{
    /* 发送中断 */
    if (RESET != DMA_GetITStatus(DMA1_IT_TC2)) {
        DMA_ClearITPendingBit(DMA1_IT_TC2);
        UARTx_TxDMAIrqService(UART_COM_3);
    }
    
    /* 接收中断 */
    /*if (RESET != DMA_GetITStatus(DMA1_IT_TC3)) {
        DMA_ClearITPendingBit(DMA1_IT_TC3);
        UARTx_RxDMAIrqService(UART_COM_0);
    }*/
}

/*******************************************************************
** 函数名称: DMA1_CHANNEL3_IrqHandle
** 函数描述: DMA1_CHANNEL3中断处理
** 参数:     无
** 返回:     无
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void DMA1_CHANNEL3_IrqHandle(void)
{
    ;
}


/*******************************************************************
** 函数名称: DMA1_CHANNEL4_IrqHandle
** 函数描述: DMA1_CHANNEL4中断处理(UART2 TX for DMA CH4/ UART2 RX for DMA CH5)
** 参数:     无
** 返回:     无
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void DMA1_CHANNEL4_IrqHandle(void)
{
    /* 发送中断 */
    if (RESET != DMA_GetITStatus(DMA1_IT_TC4)) {
        DMA_ClearITPendingBit(DMA1_IT_TC4);
        UARTx_TxDMAIrqService(UART_COM_0);
    }

    
    /* 接收中断 */
    /*if (RESET != DMA_GetITStatus(DMA1_IT_TC5)) {
        DMA_ClearITPendingBit(DMA1_IT_TC5);
        UARTx_RxDMAIrqService(UART_COM_1);
    }
    if (RESET != DMA_GetITStatus(DMA1_IT_TC6)) {
        DMA_ClearITPendingBit(DMA1_IT_TC6);
        UARTx_RxDMAIrqService(UART_COM_3);
    }
    */
}

/*******************************************************************
** 函数名称: DMA1_CHANNEL5_IrqHandle
** 函数描述: DMA1_CHANNEL5中断处理
** 参数:     无
** 返回:     无
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void DMA1_CHANNEL5_IrqHandle(void)
{
    ;
}

/*******************************************************************
** 函数名称: DMA1_CHANNEL5_IrqHandle
** 函数描述: DMA1_CHANNEL5中断处理
** 参数:     无
** 返回:     无
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void DMA1_CHANNEL6_IrqHandle(void)
{
    ;
}


/*******************************************************************
** 函数名称: DMA1_CHANNEL5_IrqHandle
** 函数描述: DMA1_CHANNEL5中断处理
** 参数:     无
** 返回:     无
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void DMA1_CHANNEL7_IrqHandle(void)
{
     /* 发送中断 */
    if (RESET != DMA_GetITStatus(DMA1_IT_TC7)) {
        DMA_ClearITPendingBit(DMA1_IT_TC7);
        UARTx_TxDMAIrqService(UART_COM_1);
    }
}


/*******************************************************************
** 函数名称: UART1_IrqHandle
** 函数描述: uart1中断服务程序
** 参数:     无
** 返回:     无
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void UART1_IrqHandle(void)
{
    INT16U data;
    
    /* 接收中断 */
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        //data = USART_ReceiveData(USART1);                                      /* 读取数据可以清除中断位 */
        data = USART1->DR & 0x01FF;
        Uartx_RxIrqService(UART_COM_0, (INT32U)USART1_BASE, data);
    }

#if 0
    /* 发送中断 */
	if (USART_GetITStatus(USART1, USART_IT_TXE) != RESET) {
	    USART_ClearITPendingBit(USART1, USART_IT_TXE);	//清除中断位
        Uartx_TxIrqService(UART_COM_0, (INT32U)USART1_BASE);
    }
    
    /* 空闲中断 */
    if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET) {
        USART_ClearFlag(USART1, USART_IT_IDLE);
        //temp = USART1->SR;
        data = USART_ReceiveData(USART1);                                      /* 读取数据可以清除中断位 */
        UARTx_RxDMAIrqService(UART_COM_0);
    }
#endif
}

INT16U testdata;
/*******************************************************************
** 函数名称: UART2_IrqHandle
** 函数描述: uart2中断服务程序
** 参数:     无
** 返回:     无
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void UART2_IrqHandle(void)
{
    INT16U data;
    
    /* 接收中断 */
    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
        //data = USART_ReceiveData(USART2);                                      /* 读取数据可以清除中断位 */
        data = USART2->DR & 0x01FF;
        testdata = data;
        Uartx_RxIrqService(UART_COM_1, (INT32U)USART2_BASE, data);
    }

#if 0
    if (USART_GetITStatus(USART2, USART_IT_ORE) != RESET) {
        USART_ClearFlag(USART2, USART_FLAG_ORE);
        //data = USART_ReceiveData(USART2);                                      /* 读取数据可以清除中断位 */
        data = USART2->RDR & 0x01FF;
        Uartx_RxIrqService(UART_COM_1, (INT32U)USART2_BASE, data);
    }

    /* 发送中断 */
	if (USART_GetITStatus(USART2, USART_IT_TXE) != RESET) {
	    USART_ClearITPendingBit(USART2, USART_IT_TXE);	//清除中断位
        Uartx_TxIrqService(UART_COM_1, (INT32U)USART2_BASE);
    }
    
    /* 空闲中断 */
    if(USART_GetITStatus(USART2, USART_IT_IDLE) != RESET) {
        USART_ClearFlag(USART2, USART_IT_IDLE);
        //temp = USART1->SR;
        data = USART_ReceiveData(USART2);                                      /* 读取数据可以清除中断位 */
        UARTx_RxDMAIrqService(UART_COM_1);
    }
#endif
}

/*******************************************************************
** 函数名称: UART3_IrqHandle
** 函数描述: uart3中断服务程序
** 参数:     无
** 返回:     无
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void UART3_IrqHandle(void)
{
    INT16U data;
    
    /* 接收中断 */
    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET) {
        //data = USART_ReceiveData(USART3);                                      /* 读取数据可以清除中断位 */
        data = USART3->DR & 0x01FF;
        Uartx_RxIrqService(UART_COM_3, (INT32U)USART3_BASE, data);
    }
}





