/********************************************************************************
**
** 文件名:     port_uart.c
** 版权所有:   (c) 1998-2017 厦门雅迅网络股份有限公司
** 文件描述:   实现平台的串口驱动接口
**
*********************************************************************************
**                  修改历史记录
**===============================================================================
**| 日期              | 作者        |  修改记录
**===============================================================================
**| 2017/05/15 | 黄运峰    |  创建该文件
********************************************************************************/
#include "dal_usart.h"
#include "port_uart.h"

#define SIZE_RECVROUNDBUF   2000
#define SIZE_SENDROUNDBUF   2000

static INT8U         s_recvbuf[SIZE_RECVROUNDBUF];                             /*  接收缓冲区  */
static INT8U         s_sendbuf[SIZE_SENDROUNDBUF];                             /*  发送缓冲区  */
static USART_BUF_T  s_uartbuf;  

static INT8U         s_recvbuf1[SIZE_RECVROUNDBUF];                             /*  接收缓冲区  */
static INT8U         s_sendbuf1[SIZE_SENDROUNDBUF];                             /*  发送缓冲区  */
static USART_BUF_T  s_uartbuf1;  
/********************************************************************************
**  函数名:     PORT_InitUart
**  函数描述:   串口初始化
**  参数:       [in] :配置参数
**  返回:       成功返回true，失败返回false
********************************************************************************/
BOOLEAN PORT_InitUart(INT8U uid, INT32U baud, INT16U rlen, INT16U slen)
{
    USART_PARA_T usart;

    if((rlen > SIZE_RECVROUNDBUF) || (slen > SIZE_SENDROUNDBUF)){
        return false;
    }
    if (uid == UART_IDX2) {
        s_uartbuf.rlen = rlen;
        s_uartbuf.slen = slen;
        s_uartbuf.rbuf = s_recvbuf;
        s_uartbuf.sbuf = s_sendbuf;
    } else if (uid == USART_NO3) {
        s_uartbuf1.rlen = rlen;
        s_uartbuf1.slen = slen;
        s_uartbuf1.rbuf = s_recvbuf1;
        s_uartbuf1.sbuf = s_sendbuf1;
    } else {
        s_uartbuf1.rlen = rlen;
        s_uartbuf1.slen = slen;
        s_uartbuf1.rbuf = s_recvbuf1;
        s_uartbuf1.sbuf = s_sendbuf1;
    }
    ClearWatchdog();
    USARTX_IOConfig((USART_INDEX_E)uid);
    usart.baud = (BAUD_VALUE_E)baud;
    usart.databits = DATABITS_8;
    usart.stopbits = STOPBITS_1;
    usart.parity = PARITY_NONE;
    if (uid == UART_IDX2) {
        USARTX_Initiate((USART_INDEX_E)uid, &usart, &s_uartbuf);
    } else if (uid == USART_NO3) {
        USARTX_Initiate((USART_INDEX_E)uid, &usart, &s_uartbuf1);
    }
    USARTX_Enable((USART_INDEX_E)uid);
    return true;
}

/********************************************************************************
**  函数名:     PORT_CloseUart
**  函数描述:   关闭串口
**  参数:       [in] uid：串口编号,见UART_IDX_E
**  返回:       无
********************************************************************************/
void PORT_CloseUart(INT32U uid)
{
    return;
    //hal_uart_Close(uid);
}

/********************************************************************************
** 函数名:   PORT_uart_LeftOfWrbuf
** 函数描述: 查询发送缓冲剩余空间
** 参数:     [in] uid:  串口编号
** 返回:     查询成功，返回发送缓冲剩余空间；查询失败则返回-1.
********************************************************************************/
INT32S PORT_uart_LeftOfWrbuf(INT32U uid)
{
    return USARTX_LeftofBuf((USART_INDEX_E)uid);
}

/********************************************************************************
**  函数名:     PORT_UartWriteBlock
**  函数描述:   向串口发送一串数据
**  参数:       [in] uid: 串口编号，见UART_IDX_E
**              [in] sdata: 待发送的数据指针
**              [in] slen: 待发送的数据长度
**  返回:       发送结果
********************************************************************************/
BOOLEAN PORT_UartWriteBlock(INT32U uid, INT8U *sdata, INT32U slen)
{
    return USARTX_SendData((USART_INDEX_E)uid, sdata, slen);
}

/********************************************************************************
**  函数名:     PORT_UartWriteByte
**  函数描述:   向串口发送一个字节
**  参数:       [in] uid：串口编号，见UART_IDX_E
**              [in] ch：待发送的字节
**  返回:       成功返回TRUE，失败返回FALSE
********************************************************************************/
BOOLEAN PORT_UartWriteByte(INT32U uid, INT8U ch)
{
    return USARTX_SendByte((USART_INDEX_E)uid, ch);
}

/********************************************************************************
** 函数名:   PORT_UartWriteByteDIR
** 函数描述: 强制发送一个字节
** 参数:     [in] uid:  串口编号
**           [in] byte: 所要发送的字节内容
** 返回:     无
********************************************************************************/
void PORT_UartWriteByteDIR(INT32U uid, INT8U ch)
{
    return; // 暂未使用
	//hal_uart_WriteByte_DIR((USART_INDEX_E)uid, ch);
}

/********************************************************************************
** 函数名:   PORT_UartWriteBlockDIR
** 函数描述: 强制发送一串
** 参数:     [in] uid:  串口编号
**           [in] byte: 所要发送的字节内容
** 返回:     无
********************************************************************************/
void PORT_UartWriteBlockDIR(INT32U uid, INT8U *sdata, INT32U slen)
{
    return; // 暂未使用
    
	//INT32U i;
	//for(i=0; i < slen; i++) {
	//	hal_uart_WriteByte_DIR(uid, *(sdata + i));
	//}
}

/********************************************************************************
**  函数名:     PORT_UartRead
**  函数描述:   从串口读取一个字节
**  参数:       [in] uid: 串口编号，见UART_IDX_E
**  返回:       读取到的字节, -1则无效
********************************************************************************/
INT16S PORT_UartRead(INT32U uid)
{
    return USARTX_ReadByte((USART_INDEX_E)uid);
}

//------------------------------------------------------------------------------
/* End of File */
