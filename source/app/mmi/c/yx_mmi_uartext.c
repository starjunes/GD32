/********************************************************************************
**
** 文件名:     yx_mmi_uartext.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现串口扩展功能
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2014/05/22 | 叶德焰 |  创建第一版本
********************************************************************************/
#include "yx_include.h"
#include "yx_dym_drv.h"
#include "hal_flash_drv.h"
#include "dal_gpio_cfg.h"
#include "yx_debug.h"

#if EN_MMI > 0
#if EN_UARTEXT > 0
#include "yx_mmi_drv.h"

/*
********************************************************************************
* 定义模块配置参数
********************************************************************************
*/
#define PERIOD_LOW          _TICK, 4
#define PERIOD_HIGH         _TICK, 2

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/


/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static INT8U s_scantmr;



/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_SET_UART_PARA
** 函数描述:   设置扩展串口参数
** 参数:       [in] cmd:    命令编码
**             [in] data:   数据指针
**             [in] datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_CMD_SET_UART_PARA(INT8U cmd, INT8U *data, INT16U datalen)
{
    INT8U port, databit, stopbit, checkbit;
    INT32U baud;
    UART_CFG_T cfg;
    STREAM_T rstrm;
    STREAM_T *wstrm;
    
    YX_InitStrm(&rstrm, data, datalen);
    port     = YX_ReadBYTE_Strm(&rstrm);
    baud     = YX_ReadLONG_Strm(&rstrm);
    databit  = YX_ReadBYTE_Strm(&rstrm);
    stopbit  = YX_ReadBYTE_Strm(&rstrm);
    checkbit = YX_ReadBYTE_Strm(&rstrm);
    
    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteBYTE_Strm(wstrm, port);
    if (port - 1 >= UART_EXT_MAX) {
        YX_WriteBYTE_Strm(wstrm, PE_NAK_MMI);
        YX_MMI_DirSend(UP_PE_ACK_SET_UART_PARA, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm));
        return;
    }
    
    switch (databit)
    {
    case 0x01:
        databit = UART_DATABIT_5;
        break;
    case 0x02:
        databit = UART_DATABIT_6;
        break; 
    case 0x03:
        databit = UART_DATABIT_7;
        break;   
     case 0x04:
        databit = UART_DATABIT_8;
        break;         
     case 0x05:
        databit = UART_DATABIT_9;
        break;
    default:
        databit = UART_DATABIT_8;
        break;
    }
    
    switch (stopbit)
    {
    case 0x01:
        stopbit = UART_STOPBIT_1;    
        break;
    case 0x02:
        stopbit = UART_STOPBIT_2;
        break;
    case 0x03:
        stopbit = UART_STOPBIT_0_5;
        break;
    case 0x04:
        stopbit = UART_STOPBIT_1_5;
        break;
    default:
        stopbit = UART_STOPBIT_1;
        break;
    }
    
    switch (checkbit)
    {
    case 0x01:
        checkbit = UART_PARITY_NONE;    
        break;
    case 0x02:
        checkbit = UART_PARITY_EVEN;
        break;
    case 0x03:
        checkbit = UART_PARITY_ODD;
        break;
    case 0x04:
        checkbit = UART_PARITY_SPACE;
        break;
    case 0x05:
        checkbit = UART_PARITY_MARK;
        break;        
    default:
        checkbit = UART_PARITY_NONE;
        break;
    }
    
    cfg.com     = UART_COM_3 + port - 1;
    cfg.baud    = baud;
    cfg.parity  = checkbit;
    cfg.databit = databit;
    cfg.stopbit = stopbit;
    
    cfg.rx_fcm = UART_FCM_NULL;
    cfg.tx_fcm = UART_FCM_NULL;
    
    cfg.rx_len = 200;
    cfg.tx_len = 512;
    
    if (ST_UART_OpenUart(&cfg)) {
        YX_WriteBYTE_Strm(wstrm, PE_ACK_MMI);
    } else {
        YX_WriteBYTE_Strm(wstrm, PE_NAK_MMI);
    }
    YX_MMI_DirSend(UP_PE_ACK_SET_UART_PARA, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm));
    
    if (baud <= 19200) {
        OS_StartTmr(s_scantmr,  PERIOD_LOW);
    } else {
        OS_StartTmr(s_scantmr,  PERIOD_HIGH);
    }
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_CTL_UART_POWER
** 函数描述:   控制扩展串口电源
** 参数:       [in] cmd:    命令编码
**             [in] data:   数据指针
**             [in] datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_CMD_CTL_UART_POWER(INT8U cmd, INT8U *data, INT16U datalen)
{
    INT8U port, ctl;
    STREAM_T rstrm;
    STREAM_T *wstrm;
    
    YX_InitStrm(&rstrm, data, datalen);
    port = YX_ReadBYTE_Strm(&rstrm);
    ctl  = YX_ReadBYTE_Strm(&rstrm);
    
    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteBYTE_Strm(wstrm, port);
    YX_WriteBYTE_Strm(wstrm, ctl);
    YX_WriteBYTE_Strm(wstrm, PE_NAK_MMI);
    
    YX_MMI_ListSend(UP_PE_ACK_CTL_UART_POWER, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 0, 0, 0);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_ACK_UART_DATA_SEND
** 函数描述:   UART数据上行透传请求应答
** 参数:       [in] cmd:    命令编码
**             [in] data:   数据指针
**             [in] datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_ACK_UART_DATA_SEND(INT8U cmd, INT8U *data, INT16U datalen)
{
    YX_MMI_ListAck(UP_PE_CMD_UART_DATA_SEND, _SUCCESS);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_UART_DATA_SEND
** 函数描述:   UART数据下行透传请求
** 参数:       [in] cmd:    命令编码
**             [in] data:   数据指针
**             [in] datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_CMD_UART_DATA_SEND(INT8U cmd, INT8U *data, INT16U datalen)
{
    INT8U port;
    INT16U packetlen;
    STREAM_T rstrm;
    STREAM_T *wstrm;
    
    YX_InitStrm(&rstrm, data, datalen);
    port      = YX_ReadBYTE_Strm(&rstrm);
    packetlen = YX_ReadHWORD_Strm(&rstrm);
    
    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteBYTE_Strm(wstrm, port);
    if (port - 1 >= UART_EXT_MAX) {
        YX_WriteBYTE_Strm(wstrm, PE_NAK_MMI);
    } else {
        if (ST_UART_WriteBlock(UART_COM_3 + port - 1, YX_GetStrmPtr(&rstrm), packetlen)) {
            YX_WriteBYTE_Strm(wstrm, PE_ACK_MMI);
        } else {
            YX_WriteBYTE_Strm(wstrm, PE_NAK_MMI);
        }
    }
    YX_MMI_DirSend(UP_PE_ACK_UART_DATA_SEND, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm));
}

static FUNCENTRY_MMI_T s_functionentry[] = {
                       DN_PE_CMD_SET_UART_PARA,              HdlMsg_DN_PE_CMD_SET_UART_PARA      /* 设置扩展串口参数 */
                      ,DN_PE_CMD_CTL_UART_POWER,             HdlMsg_DN_PE_CMD_CTL_UART_POWER     /* 控制扩展串口电源 */
                      ,DN_PE_ACK_UART_DATA_SEND,             HdlMsg_DN_PE_ACK_UART_DATA_SEND     /* UART数据上行透传请求应答 */
                      ,DN_PE_CMD_UART_DATA_SEND,             HdlMsg_DN_PE_CMD_UART_DATA_SEND     /* UART数据下行透传请求 */

                                         };


/*******************************************************************
** 函数名:     ScanTmrProc
** 函数描述:   定时器
** 参数:       [in] pdata：定时器特征值
** 返回:       无
********************************************************************/
static void ScanTmrProc(void *pdata)
{
    INT32U i, readlen;
    STREAM_T *wstrm;
    
    if (!YX_MMI_IsON()) {
        return;
    }
    
    for (i = 0; i < UART_EXT_MAX; i++) {
        readlen = ST_UART_GetRecvBytes(UART_COM_3 + i);
        if (readlen > 80) {
            readlen = 80;
        }
        
        if (readlen > 0) {
            wstrm = YX_STREAM_GetBufferStream();
            YX_WriteBYTE_Strm(wstrm, i + 1);
            YX_WriteHWORD_Strm(wstrm, readlen);
            for (; readlen > 0; readlen--) {
                YX_WriteBYTE_Strm(wstrm, ST_UART_ReadChar(UART_COM_3 + i));
            }
            YX_MMI_DirSend(UP_PE_CMD_UART_DATA_SEND, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm));
        }
    }
}

/*******************************************************************
** 函数名:     YX_MMI_InitUartext
** 函数描述:   功能初始化
** 参数:       无
** 返回:       无
********************************************************************/
void YX_MMI_InitUartext(void)
{
    INT8U i;
    
    for (i = 0; i < sizeof(s_functionentry) / sizeof(s_functionentry[0]); i++) {
        YX_MMI_Register(s_functionentry[i].cmd, s_functionentry[i].entryproc);
    }
    
    s_scantmr  = OS_CreateTmr(TSK_ID_APP, (void *)0, ScanTmrProc);
    OS_StartTmr(s_scantmr,  PERIOD_LOW);
}

#endif
#endif
