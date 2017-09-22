/********************************************************************************
**
** 文件名:     yx_mmi_gps.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现GPS模块业务管理
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
#include "st_uart_drv.h"
#include "dal_gpio_cfg.h"
#include "yx_debug.h"

#if EN_MMI > 0
#include "yx_mmi_drv.h"

/*
********************************************************************************
* 定义模块配置参数
********************************************************************************
*/

#define UART_COM_GPS         UART_COM_1

#define PERIOD_SCAN          _TICK, 2

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
** 函数名:     SendAck
** 函数描述:   发送应答
** 参数:       [in]cmd:    命令编码
**             [in]type:   应答类型
** 返回:       无
********************************************************************/
static void SendAck(INT8U cmd, INT8U type)
{
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteBYTE_Strm(wstrm, type);
    YX_MMI_ListSend(cmd, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 0, 0, 0);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_SET_GPS_UART
** 函数描述:   设置GPS串口通信参数请求
** 参数:       [in] cmd:    命令编码
**             [in] data:   数据指针
**             [in] datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_CMD_SET_GPS_UART(INT8U cmd, INT8U *data, INT16U datalen)
{
    INT8U baud, databit, stopbit, checkbit;
    INT32U baudlevel;
    UART_CFG_T cfg;
    
    if (cmd != DN_PE_CMD_SET_GPS_UART) {
        return;
    }
    
    baud     = data[0];
    databit  = data[1];
    stopbit  = data[2];
    checkbit = data[3];
    
    switch (baud)                                                              /* 波特率 */
    {
    case 0x01:
        baudlevel = 1200;
        break;
    case 0x02:
        baudlevel = 2400;
        break;
    case 0x03:
        baudlevel = 4800;
        break;
    case 0x04:
        baudlevel = 9600;
        break;
    case 0x05:
        baudlevel = 19200;
        break;
    case 0x06:
        baudlevel = 38400;
        break;
    case 0x07:
        baudlevel = 57600;
        break;
    case 0x08:
        baudlevel = 115200;
        break;
    default:
        baudlevel = 9600;
        break;
    }
    
    switch (databit)                                                           /* 数据位 */
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
    
    switch (stopbit)                                                           /* 停止位 */
    {
    case 0x01:
        stopbit = UART_STOPBIT_1;    
        break;
    case 0x02:
        stopbit = UART_STOPBIT_1_5;
        break;
    case 0x03:
        stopbit = UART_STOPBIT_2;
        break;        
    default:
        stopbit = UART_STOPBIT_1;
        break;
    }
    
    switch (checkbit)                                                          /* 校验位 */
    {
    case 0x01:
        checkbit = UART_PARITY_NONE;    
        break;
    case 0x02:
        checkbit = UART_PARITY_ODD;
        break;
    case 0x03:
        checkbit = UART_PARITY_EVEN;
        break;
    case 0x04:
        checkbit = UART_PARITY_MARK;
        break;
    case 0x05:
        checkbit = UART_PARITY_SPACE;
        break;        
    default:
        checkbit = UART_PARITY_NONE;
        break;
    }
    
    cfg.com     = UART_COM_GPS;
    cfg.baud    = baudlevel;
    cfg.parity  = checkbit;
    cfg.databit = databit;
    cfg.stopbit = stopbit;
    
    cfg.rx_fcm = UART_FCM_NULL;
    cfg.tx_fcm = UART_FCM_NULL;
    
    cfg.rx_len = 200;
//#if EN_DEBUG > 0
    cfg.tx_len = 512;
//#else
    //cfg.tx_len = 200;
//#endif
    
    ST_UART_OpenUart(&cfg);
    SendAck(UP_PE_ACK_SET_GPS_UART, PE_ACK_MMI);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_CTL_GPS_POWER
** 函数描述:   控制GPS模块电源请求
** 参数:       [in] cmd:    命令编码
**             [in] data:   数据指针
**             [in] datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_CMD_CTL_GPS_POWER(INT8U cmd, INT8U *data, INT16U datalen)
{
    INT8U ctl;
    
    ctl = data[0];
    
    if ((ctl & 0x01) != 0) {
        DAL_GPIO_PullupGpsPower();
    } else {
        DAL_GPIO_PulldownGpsPower();
    }
    
    if ((ctl & 0x02) != 0) {
        DAL_GPIO_PullupGpsVbat();
    } else {
        DAL_GPIO_PulldownGpsVbat();
    }
        
    SendAck(UP_PE_ACK_CTL_GPS_POWER, PE_ACK_MMI);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_ACK_GPS_DATA_SEND
** 函数描述:   GPS数据上行透传请求应答
** 参数:       [in] cmd:    命令编码
**             [in] data:   数据指针
**             [in] datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_ACK_GPS_DATA_SEND(INT8U cmd, INT8U *data, INT16U datalen)
{
    YX_MMI_ListAck(UP_PE_CMD_GPS_DATA_SEND, _SUCCESS);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_GPS_DATA_SEND
** 函数描述:   GPS数据下行透传请求
** 参数:       [in] cmd:    命令编码
**             [in] data:   数据指针
**             [in] datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_CMD_GPS_DATA_SEND(INT8U cmd, INT8U *data, INT16U datalen)
{
    if (ST_UART_WriteBlock(UART_COM_GPS, data, datalen)) {
        SendAck(UP_PE_ACK_GPS_DATA_SEND, PE_ACK_MMI);
    } else {
        SendAck(UP_PE_ACK_GPS_DATA_SEND, PE_NAK_MMI);
    }
}


static FUNCENTRY_MMI_T s_functionentry[] = {
                       DN_PE_CMD_SET_GPS_UART,                 HdlMsg_DN_PE_CMD_SET_GPS_UART    // 设置GPS串口通信参数请求
                      ,DN_PE_CMD_CTL_GPS_POWER,                HdlMsg_DN_PE_CMD_CTL_GPS_POWER   // 控制GPS模块电源请求
                      ,DN_PE_ACK_GPS_DATA_SEND,                HdlMsg_DN_PE_ACK_GPS_DATA_SEND   // GPS数据上行透传请求应答
                      ,DN_PE_CMD_GPS_DATA_SEND,                HdlMsg_DN_PE_CMD_GPS_DATA_SEND   // GPS数据下行透传请求
                      
                                         };
static INT8U s_funnum = sizeof(s_functionentry)/sizeof(s_functionentry[0]);


/*******************************************************************
** 函数名:     ScanTmrProc
** 函数描述:   定时器
** 参数:       [in] pdata：定时器特征值
** 返回:       无
********************************************************************/
static void ScanTmrProc(void *pdata)
{
    INT32U readlen;
    STREAM_T *wstrm;
    
    OS_StartTmr(s_scantmr, PERIOD_SCAN);
    
    if (!YX_MMI_IsON()) {
        return;
    }
    
    readlen = ST_UART_GetRecvBytes(UART_COM_GPS);
    if (readlen > 100) {
        readlen = 100;
    }
        
    if (readlen > 0) {
        wstrm = YX_STREAM_GetBufferStream();
        for (; readlen > 0; readlen--) {
            YX_WriteBYTE_Strm(wstrm, ST_UART_ReadChar(UART_COM_GPS));
        }
        YX_MMI_DirSend(UP_PE_CMD_GPS_DATA_SEND, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm));
    }
}

/*******************************************************************
** 函数名:     YX_MMI_InitGps
** 函数描述:   功能初始化
** 参数:       无
** 返回:       无
********************************************************************/
void YX_MMI_InitGps(void)
{
    INT8U i;
    
    DAL_GPIO_InitGpsPower();
    DAL_GPIO_InitGpsVbat();
    DAL_GPIO_PulldownGpsPower();
    DAL_GPIO_PulldownGpsVbat();

    for (i = 0; i < s_funnum; i++) {
        YX_MMI_Register(s_functionentry[i].cmd, s_functionentry[i].entryproc);
    }
    
    s_scantmr  = OS_CreateTmr(TSK_ID_APP, (void *)0, ScanTmrProc);
    OS_StartTmr(s_scantmr,  PERIOD_SCAN);
}

#endif
