/********************************************************************************
**
** 文件名:     yx_mmi_drv.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现dvr外设驱动基本功能业务，外设状态管理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2014/03/22 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "yx_include.h"
#include "yx_dym_drv.h"
#include "st_rtc_drv.h"
#include "dal_pp_drv.h"
#include "dal_hit_drv.h"
#include "yx_debug.h"
#include "dal_pp_drv.h"

#if EN_MMI > 0
#include "yx_mmi_drv.h"

/*
********************************************************************************
*宏定义
********************************************************************************
*/

#define _ON                   0x01
#define _SLEEP                0x02

#define MAX_QUERY             20                 /* 链路发送周期 */
#define MAX_OVERTIME          600               /* 监控超时周期 */
#define MAX_WATCHDOG          30                /* 看门狗溢出时间 */
#define MAX_VER               41

#define PERIOD_LINK           _SECOND, 1
#define PERIOD_RESET          _SECOND, 3
#define PERIOD_MEM            _SECOND, 10

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/

typedef struct {
    INT8U     status;                  /* MMI状态 */
    INT16U    ct_query;                /* 链路维护计数器 */
    INT16U    ct_overtime;             /* 链路超时计数器 */
    INT16U    ct_watchdog;             /* 看门狗时间周期 */
    INT8U     powersave;               /* 省电控制 */
    INT8U     verlen;                  /* 版本长度 */
    INT8U     ver[MAX_VER];            /* 版本 */
    INT8U     fun[4];                  /* 功能配置 */
} TCB_T;

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static TCB_T s_tcb;
static INT8U s_linktmr, s_resettmr;
static INT8U s_memtmr;
static LOG_FLAG_T s_logflag = {1};                           /* 为了打印刚上电时的日志信息，默认开启 */
static BOOLEAN s_firstflag = FALSE;

/*******************************************************************
** 函数名:     SendSetupLinkCommand
** 函数描述:   外设发送上电建立连接请求指令
** 参数:       无
** 返回:       无
********************************************************************/
static void SendSetupLinkCommand(void)   
{
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteBYTE_Strm(wstrm, MAX_QUERY);                                       /* 链路维护时间 */
    YX_MMI_ListSend(UP_PE_CMD_LINK_REQ, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 0, 0, 0);
}

/*******************************************************************
** 函数名:     SendBeatCommand
** 函数描述:   外设发送心跳请求指令
** 参数:       无
** 返回:       无
********************************************************************/
static void SendBeatCommand(void)   
{
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteBYTE_Strm(wstrm, MAX_QUERY);                                       /* 链路维护时间 */
    YX_MMI_ListSend(UP_PE_CMD_BEAT_REQ, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 0, 0, 0);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_ACK_LINK_REQ
** 函数描述:   连接注册请求协议应答
** 参数:       [in]cmd:命令编码
**             [in]data:数据指针
**             [in]datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_ACK_LINK_REQ(INT8U cmd, INT8U *data, INT16U datalen)
{
    if (cmd != DN_PE_ACK_LINK_REQ) {
        return;
    }
    
    #if DEBUG_MMI > 0
    printf_com("<收到MMI连接注册请求应答(0x%x)(%d)>\r\n", s_tcb.status, data[0]);
    #endif
    
    if (data[0] == 0x01) {
        s_tcb.ct_query = 0;
        s_tcb.ct_overtime = 0;
        s_tcb.status |= _ON;                                                   /* MMI连接状态 */
    
        YX_MMI_ListAck(UP_PE_CMD_LINK_REQ, _SUCCESS);
        SendBeatCommand();
        YX_MMI_SendRealClock();
        YX_MMI_SendGsensorInfo(DAL_HIT_GetGsensorStatus());                    /* 上报重力传感器状态 */
        //YX_MMI_SendIccardInfo();
    }
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_ACK_BEAT_REQ
** 函数描述:   MMI链路维护请求
** 参数:       [in]cmd:命令编码
**             [in]data:数据指针
**             [in]datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_ACK_BEAT_REQ(INT8U cmd, INT8U *data, INT16U datalen)
{
    if (cmd != DN_PE_ACK_BEAT_REQ) {
        return;
    }
    
    #if DEBUG_MMI > 0
    printf_com("<收到MMI链路维护请求应答>\r\n");
    #endif
    
    if (data[0] == 0x01) {
        s_tcb.ct_query = 0;
        s_tcb.ct_overtime = 0;
        s_tcb.status |= _ON;                                                       /* MMI连接状态 */
        YX_MMI_ListAck(UP_PE_CMD_BEAT_REQ, _SUCCESS);
    }
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_VERSION_REQ
** 函数描述:   版本查询应答
** 参数:       [in]cmd:    命令编码
**             [in]data:   数据指针
**             [in]datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_CMD_VERSION_REQ(INT8U cmd, INT8U *data, INT16U datalen)
{
    STREAM_T *wstrm;
    char *pver;
    
    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteBYTE_Strm(wstrm, COM_VER_MMI);
    YX_WriteBYTE_Strm(wstrm, MAX_QUERY);                                       /* 链路维护时间 */
    pver = YX_GetVersion();                                                    /* 版本号 */
    YX_WriteBYTE_Strm(wstrm, YX_STRLEN(pver));
    YX_WriteSTR_Strm(wstrm, pver);
    
    YX_MMI_ListSend(UP_PE_ACK_VERSION_REQ, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 0, 0, 0);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_RESET_INFORM
** 函数描述:   主机即将复位通知请求
** 参数:       [in]cmd:    命令编码
**             [in]data:   数据指针
**             [in]datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_CMD_RESET_INFORM(INT8U cmd, INT8U *data, INT16U datalen)
{
    STREAM_T *wstrm;
    
    #if DEBUG_MMI > 0
    printf_com("<主机通知外设,主机即将复位(%d)>\r\n", data[0]);
    #endif
    
    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteBYTE_Strm(wstrm, data[0]);
    YX_WriteBYTE_Strm(wstrm, PE_ACK_MMI);
    YX_MMI_ListSend(UP_PE_ACK_RESET_INFORM, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 0, 0, 0);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_ACK_PE_RESET_INFORM
** 函数描述:   外设即将复位通知请求应答
** 参数:       [in]cmd:命令编码
**             [in]data:数据指针
**             [in]datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_ACK_PE_RESET_INFORM(INT8U cmd, INT8U *data, INT16U datalen)
{
    YX_MMI_ListAck(UP_PE_CMD_PE_RESET_INFORM, _SUCCESS);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_ACK_HOST_RESET_INFORM
** 函数描述:   外设通知主机，外设即将关闭或重启主机通知请求应答
** 参数:       [in]cmd:命令编码
**             [in]data:数据指针
**             [in]datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_ACK_HOST_RESET_INFORM(INT8U cmd, INT8U *data, INT16U datalen)
{
    YX_MMI_ListAck(UP_PE_CMD_HOST_RESET_INFORM, _SUCCESS);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_RESTART_REQ
** 函数描述:   复位重启请求
** 参数:       [in]cmd:    命令编码
**             [in]data:   数据指针
**             [in]datalen:数据长度
** 返回:       无
********************************************************************/
static void Reset(INT8U result)
{
    OS_RESET(RESET_EVENT_INITIATE);
}

static void HdlMsg_DN_PE_CMD_RESTART_REQ(INT8U cmd, INT8U *data, INT16U datalen)
{
    STREAM_T *wstrm;
    
    #if DEBUG_MMI > 0
    printf_com("<主机请求复位外设(%d)>\r\n", data[0]);
    #endif
    
    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteBYTE_Strm(wstrm, data[0]);
    YX_WriteBYTE_Strm(wstrm, PE_ACK_MMI);
    YX_MMI_ListSend(UP_PE_ACK_RESTART_REQ, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 1, 5, Reset);
    
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_CLEAR_WATCHDOG
** 函数描述:   看门狗喂狗
** 参数:       [in]cmd:    命令编码
**             [in]data:   数据指针
**             [in]datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_CMD_CLEAR_WATCHDOG(INT8U cmd, INT8U *data, INT16U datalen)
{
    STREAM_T *wstrm;
    
    if (data[0] == 0xAA) {
        s_tcb.ct_watchdog = MAX_WATCHDOG;
        
        wstrm = YX_STREAM_GetBufferStream();
        YX_WriteBYTE_Strm(wstrm, PE_ACK_MMI);
        YX_MMI_ListSend(UP_PE_ACK_CLEAR_WATCHDOG, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 0, 0, 0);
    }
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_GET_RESET_REC
** 函数描述:   最近复位条数
** 参数:       [in]cmd:    命令编码
**             [in]data:   数据指针
**             [in]datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_CMD_GET_RESET_REC(INT8U cmd, INT8U *data, INT16U datalen)
{
#if EN_APP > 0
    INT8U i;
    STREAM_T *wstrm;
    RESET_RECORD_T resetrecord;
    
    DAL_PP_ReadParaByID(PP_RESET_, (INT8U *)&resetrecord, sizeof(RESET_RECORD_T));
    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteHWORD_Strm(wstrm, resetrecord.rst_total);                          /* 总复位次数 */
    YX_WriteHWORD_Strm(wstrm, resetrecord.rst_int);                            /* 主动复位次数 */
    YX_WriteHWORD_Strm(wstrm, resetrecord.rst_err);                            /* 出错复位次数 */
    YX_WriteHWORD_Strm(wstrm, resetrecord.rst_ext);                            /* 外部复位次数 */
    YX_WriteBYTE_Strm(wstrm, MAX_RESET_RECORD);                                /* 最近复位条数 */
    
    for (i = 0; i < MAX_RESET_RECORD; i++) {
        YX_WriteDATA_Strm(wstrm, (INT8U *)&resetrecord.rst_record[i].systime, 6);
        YX_WriteDATA_Strm(wstrm, (INT8U *)&resetrecord.rst_record[i].file, 15);
        YX_WriteHWORD_Strm(wstrm, resetrecord.rst_record[i].line);
    }
    
    YX_MMI_ListSend(UP_PE_ACK_GET_RESET_REC, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 0, 0, 0);
#endif
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_START_OR_STOP_LOG
** 函数描述:   开启或关闭日志
** 参数:       [in]cmd:    命令编码
**             [in]data:   数据指针
**             [in]datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_CMD_START_OR_STOP_LOG(INT8U cmd, INT8U *data, INT16U datalen)
{
    INT8U result = TRUE;
    STREAM_T *wstrm;
    
    if (cmd != DN_PE_CMD_START_OR_STOP_LOG) {
        return;
    }

    switch (data[0]) {
        case 0x01:
            s_logflag.flag = TRUE;
            DAL_PP_StoreParaByID(PP_ID_LOG, (INT8U *)&s_logflag, sizeof(LOG_FLAG_T));
            #if DEBUG_MMI > 0
            printf_com("开启日志\r\n");
            #endif
            break;
        case 0x02:
            #if DEBUG_MMI > 0
            printf_com("关闭日志\r\n");
            #endif
            s_logflag.flag = FALSE;
            DAL_PP_StoreParaByID(PP_ID_LOG, (INT8U *)&s_logflag, sizeof(LOG_FLAG_T));
            break;
        default:
            result = FALSE;
            break;
    }

    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteBYTE_Strm(wstrm, data[0]);
    YX_WriteBYTE_Strm(wstrm, result);

    YX_MMI_ListSend(UP_PE_ACK_START_OR_STOP_LOG, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 0, 0, 0);
}



static FUNCENTRY_MMI_T s_functionentry[] = {
        DN_PE_ACK_LINK_REQ,                   HdlMsg_DN_PE_ACK_LINK_REQ            // 连接注册请求协议
       ,DN_PE_ACK_BEAT_REQ,                   HdlMsg_DN_PE_ACK_BEAT_REQ            // 链路维护请求协议
       ,DN_PE_CMD_VERSION_REQ,	              HdlMsg_DN_PE_CMD_VERSION_REQ         // 版本查询应答
       
       ,DN_PE_CMD_RESET_INFORM,               HdlMsg_DN_PE_CMD_RESET_INFORM        // 主机即将复位通知请求
       ,DN_PE_ACK_PE_RESET_INFORM ,		      HdlMsg_DN_PE_ACK_PE_RESET_INFORM     // 外设通知主机，外设即将复位通知请求
       ,DN_PE_ACK_HOST_RESET_INFORM ,		  HdlMsg_DN_PE_ACK_HOST_RESET_INFORM   // 外设通知主机，外设即将关闭或重启主机通知请求应答
       ,DN_PE_CMD_RESTART_REQ,                HdlMsg_DN_PE_CMD_RESTART_REQ         // 复位重启请求
       
       ,DN_PE_CMD_CLEAR_WATCHDOG,             HdlMsg_DN_PE_CMD_CLEAR_WATCHDOG      // 看门狗喂狗
       ,DN_PE_CMD_GET_RESET_REC,              HdlMsg_DN_PE_CMD_GET_RESET_REC       // 复位记录查询

       ,DN_PE_CMD_START_OR_STOP_LOG,          HdlMsg_DN_PE_CMD_START_OR_STOP_LOG   // 开启或关闭日志
};

static INT8U s_funnum = sizeof(s_functionentry) / sizeof(s_functionentry[0]);


                                             
/*******************************************************************
** 函数名:     ResetTmrProc
** 函数描述:   延时上电定时器
** 参数:       [in] pdata：定时器特征值
** 返回:       无
********************************************************************/
static void ResetTmrProc(void *pdata)
{
    OS_StopTmr(s_resettmr);
    
    YX_MMI_PullUp();
    YX_MMI_Reset();                                                           /* 将通道复位为共享状态 */
    //YX_MMI_CfgBaud(115200, UART_DATABIT_8, UART_STOPBIT_1, UART_PARITY_NONE);
}

/*******************************************************************
** 函数名:     LinkTmrProc
** 函数描述:   链路探寻定时器
** 参数:       [in] pdata：定时器特征值
** 返回:       无
********************************************************************/
static void LinkTmrProc(void *pdata)
{
    HOST_RESET_STATUS_T reset_info;
    
    OS_StartTmr(s_linktmr, PERIOD_LINK);
    if (!YX_MMI_IsON()) {
        #if DEBUG_MMI > 0
        printf_com("上电链接请求\r\n");
        #endif
        SendSetupLinkCommand();
    } else {
        if (++s_tcb.ct_query >= MAX_QUERY) {                                   /* 心跳发送周期 */
            s_tcb.ct_query = 0;
            #if DEBUG_MMI > 0
            printf_com("链路维护请求\r\n");
            #endif
            SendBeatCommand();
            if (s_firstflag == FALSE ) {
                DAL_PP_ReadParaByID(PP_ID_HOST_RESET, (INT8U *)&reset_info, sizeof(HOST_RESET_STATUS_T));
                #if DEBUG_MMI > 0
                printf_com("LinkTmrProc ,status(%d)\r\n", reset_info.status);
                #endif
                YX_MMI_SendHostResetInform_New(reset_info.status);
                s_firstflag = TRUE;
            }
        }
        #if 1
        if (s_tcb.ct_watchdog > 0) {                                           /* 看门狗监控 */
            if (--s_tcb.ct_watchdog == 0) {
                //s_tcb.ct_watchdog = MAX_WATCHDOG;
                s_tcb.ct_watchdog = MAX_OVERTIME;

                #if DEBUG_MMI > 0
                printf_com("看门狗复位\r\n");
                #endif
                
                s_tcb.status &= (~_ON);
                s_tcb.ct_query = 0;
                s_tcb.ct_overtime = 0;
                
                YX_MMI_PullDown();
                OS_StartTmr(s_resettmr, PERIOD_RESET);                         /* 延时上电 */
                return;
            } else if (s_tcb.ct_watchdog == 10) {
                YX_MMI_CfgBaud(115200, UART_DATABIT_8, UART_STOPBIT_1, UART_PARITY_NONE);
                YX_MMI_SendHostResetInform(MMI_RESET_EVENT_WDG);
            }
        }
        #endif
    }
    #if 1
    if (++s_tcb.ct_overtime >= MAX_OVERTIME) {                                 /* 链路维护超时 */
        #if DEBUG_MMI > 0
        printf_com("<MMI链路维护异常>\r\n");
        #endif
        
        s_tcb.status &= (~_ON);
        s_tcb.ct_query = 0;
        s_tcb.ct_overtime = 0;
        s_firstflag = FALSE ;
        YX_MMI_PullDown();
        OS_RESET(RESET_EVENT_INITIATE);                                        /* 主动复位 */
        OS_StartTmr(s_resettmr, PERIOD_RESET);                                 /* 延时上电 */
    } else if (s_tcb.ct_overtime == MAX_OVERTIME - 10) {
        YX_MMI_CfgBaud(115200, UART_DATABIT_8, UART_STOPBIT_1, UART_PARITY_NONE);
        YX_MMI_SendHostResetInform(MMI_RESET_EVENT_NORMAL);
    }
    #endif
}

/*******************************************************************
** 函数名:     MemTmrProc
** 函数描述:   内存定时器
** 参数:       [in] pdata：定时器特征值
** 返回:       无
********************************************************************/
static void MemTmrProc(void *pdata)
{
    #if DEBUG_MMI > 0
    INT8U                *p;
    INT16U               i, j, k;
    static INT32U        cnt = 0;      
    HEAPMEM_STATISTICS_T *mem = YX_HEAPMEM_GetStatistics();

    OS_StartTmr(s_memtmr, PERIOD_MEM);

    printf_com("内存块%d, 使用大小%d\r\n", mem->blocks, mem->occupysize);

    if (s_logflag.flag && ++cnt % 60 == 0) {                             /* 10分钟打印一次整块内存 */
        p = YX_HEAPMEM_GetAddress();
        for (i = 0; i < 4096 / 64; i++) {
            printf_hex(p + 64 * i, 64);
            ClearWatchdog();
            for (j = 350; j > 0; j--) {
                for (k = 5000; k > 0; k--);
            }
        }
        printf_com("\r\n");
    }
    #endif
}

/*******************************************************************
** 函数名:     ResetInform
** 函数描述:   复位回调接口
** 参数:       [in] event：复位类型
**             [in] file： 文件名
**             [in] line： 出错行号
** 返回:       无
********************************************************************/
static void ResetInform(INT8U event, char *file, INT32U line)
{
#if EN_APP > 0
    INT8U weekday;
    INT32U subsecond;
    GPS_DATA_T gpsdata;
    SYSTIME_T systime;
    
    if (event == RESET_EVENT_INITIATE) {
        YX_MMI_SendPeResetInform(MMI_RESET_EVENT_NORMAL);
    } else if (event == RESET_EVENT_ERR) {
        YX_MMI_SendPeResetInform(MMI_RESET_EVENT_ERROR);
    } else if (event == RESET_EVENT_UPDATE) {
        YX_MMI_SendPeResetInform(MMI_RESET_EVENT_UPDATE);
    } else {
        ;
    }
    
    if (ST_RTC_GetSystime(&systime.date, &systime.time, &weekday, &subsecond)) {
        DAL_PP_ReadParaByID(PP_ID_GPSDATA, (INT8U *)&gpsdata, sizeof(gpsdata));
        YX_MEMCPY(&gpsdata.systime, sizeof(gpsdata.systime), &systime, sizeof(systime));
        DAL_PP_StoreParaByID(PP_ID_GPSDATA, (INT8U *)&gpsdata, sizeof(gpsdata));
    }
#else
    if (event == RESET_EVENT_INITIATE) {
        YX_MMI_SendPeResetInform(MMI_RESET_EVENT_NORMAL);
    } else if (event == RESET_EVENT_ERR) {
        YX_MMI_SendPeResetInform(MMI_RESET_EVENT_ERROR);
    } else if (event == RESET_EVENT_UPDATE) {
        YX_MMI_SendPeResetInform(MMI_RESET_EVENT_UPDATE);
    } else {
        ;
    }
#endif
}

/*******************************************************************
** 函数名:     YX_MMI_InitDrv
** 函数描述:   MMI驱动模块初始化
** 参数:       无
** 返回:       无
********************************************************************/
void YX_MMI_InitDrv(void)
{
    INT8U i;
    
    YX_MEMSET(&s_tcb, 0, sizeof(s_tcb));
    //s_tcb.ct_watchdog = MAX_WATCHDOG;
    s_tcb.ct_watchdog = MAX_OVERTIME;
    
    YX_MMI_InitPower();
    YX_MMI_InitCom();
    YX_MMI_InitRecv();
    YX_MMI_InitSend();
    YX_MMI_InitCommon();
    YX_MMI_InitCan();
    //YX_MMI_InitUart();
    YX_MMI_InitGps();
    YX_MMI_InitSensor();
    YX_MMI_InitTr();
    YX_MMI_InitDownload();
#if EN_UARTEXT > 0
    YX_MMI_InitUartext();
#endif

    for (i = 0; i < s_funnum; i++) {
        YX_MMI_Register(s_functionentry[i].cmd, s_functionentry[i].entryproc);
    }
    
    s_linktmr  = OS_CreateTmr(TSK_ID_APP, (void *)0, LinkTmrProc);
    s_resettmr = OS_CreateTmr(TSK_ID_APP, (void *)0, ResetTmrProc);
    s_memtmr = OS_CreateTmr(TSK_ID_APP, (void *)0, MemTmrProc);
    OS_StartTmr(s_memtmr, PERIOD_MEM);
    //OS_StartTmr(s_linktmr,  PERIOD_LINK);
    //YX_MMI_PullUp();
    OS_RegistResetInform(RESET_PRI_0, ResetInform);

    DAL_PP_ReadParaByID(PP_ID_LOG, (INT8U *)&s_logflag, sizeof(LOG_FLAG_T));
}

/*******************************************************************
** 函数名:     YX_MMI_IsON
** 函数描述:   判断MMI是否连接
** 参数:       无
** 返回:       已连接返回true，未连接返回false
********************************************************************/
BOOLEAN YX_MMI_IsON(void)
{
    if ((s_tcb.status & _ON) != 0) {
        return true;
    } else {
        return false;
    }
}

/*******************************************************************
** 函数名:     YX_MMI_IsSleep
** 函数描述:   判断MMI是否处于省电状态
** 参数:       无
** 返回:       是返回true，否返回false
********************************************************************/
BOOLEAN YX_MMI_IsSleep(void)
{
    if ((s_tcb.status & _SLEEP) != 0) {
        return true;
    } else {
        return false;
    }
}

/*******************************************************************
** 函数名:     YX_MMI_Sleep
** 函数描述:   省电
** 参数:       无
** 返回:       已连接返回true，未连接返回false
********************************************************************/
BOOLEAN YX_MMI_Sleep(void)
{
    OS_StopTmr(s_linktmr);
    s_tcb.status = 0;
    s_firstflag = FALSE;
    return true;
}

/*******************************************************************
** 函数名:     YX_MMI_Wakeup
** 函数描述:   唤醒
** 参数:       无
** 返回:       已连接返回true，未连接返回false
********************************************************************/
BOOLEAN YX_MMI_Wakeup(void)
{
    OS_StartTmr(s_linktmr,  PERIOD_LINK);
    //s_tcb.status = 0;
    return true;
}

/*******************************************************************
** 函数名:     YX_MMI_GetVer
** 功能描述:   获取MMI版本号
** 参数:  	   无
** 返回:       版本号指针
********************************************************************/
INT8U *YX_MMI_GetVer(void)
{
    return s_tcb.ver;
}

/***************************************************************
** 函数名:    YX_MMI_ResetReq
** 功能描述:  复位请求
** 参数:  	  [in] type: 复位类型: 0x00升级复位;0x01常规复位;0x02异常复位
** 返回值:    无
***************************************************************/
static void Callback_Reset(INT8U result)
{
    YX_MMI_PullDown();
    OS_StartTmr(s_resettmr, PERIOD_RESET);                                     /* 延时上电 */
}

BOOLEAN YX_MMI_ResetReq(INT8U type)
{
    STREAM_T *wstrm;
    
    if (!YX_MMI_IsON()) {
        return FALSE;
    }
    YX_MMI_SendHostResetInform_New(type);
    
    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteBYTE_Strm(wstrm, type);
    return YX_MMI_ListSend(DN_PE_CMD_RESTART_REQ, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 3, 1, Callback_Reset);
}

/***************************************************************
** 函数名:    YX_MMI_SendPeResetInform
** 功能描述:  外设通知主机，外设即将复位，以便主机做好复位前处理工作
** 参数:  	  [in] type: 复位类型,见 MMI_RESET_EVENT_E
** 返回:      成功返回true，否返回false
***************************************************************/
BOOLEAN YX_MMI_SendPeResetInform(INT8U type)
{
    STREAM_T *wstrm;

    YX_MMI_SendHostResetInform_New(type);
    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteBYTE_Strm(wstrm, type);
    return YX_MMI_DirSend(UP_PE_CMD_PE_RESET_INFORM, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm));
}

/***************************************************************
** 函数名:    YX_MMI_SendPeResetInform
** 功能描述:  外设通知主机，外设即将关闭或重启主机(外设自己不复位)?
** 参数:  	  [in] type: 复位类型,见 MMI_RESET_EVENT_E
** 返回:      成功返回true，否返回false
***************************************************************/
BOOLEAN YX_MMI_SendHostResetInform(INT8U type)
{
    STREAM_T *wstrm;

    YX_MMI_SendHostResetInform_New(type);                                      /* 新协议上报 */
    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteBYTE_Strm(wstrm, type);
    return YX_MMI_DirSend(UP_PE_CMD_HOST_RESET_INFORM, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm));
}

/*******************************************************************
** 函数名:     YX_MMI_GetLogFlag
** 函数描述:   获取日志开启或关闭
** 参数:       无
** 返回:       开启返回true，关闭返回false
********************************************************************/
BOOLEAN YX_MMI_GetLogFlag(void)
{
    return s_logflag.flag;
}


/***************************************************************
** 函数名:    YX_MMI_SendHostResetInform_New
** 功能描述:  外设通知主机，外设即将关闭或重启主机(外设自己不复位)(新 增加休眠唤醒和上电状态上报)
** 参数:      [in] type: 复位类型,见 MMI_RESET_EVENT_NEW_E
** 返回:      成功返回true，否返回false
***************************************************************/
BOOLEAN YX_MMI_SendHostResetInform_New(INT8U type)
{
    STREAM_T *wstrm;
    HOST_RESET_STATUS_T reset_info;

    DAL_PP_ReadParaByID(PP_ID_HOST_RESET, (INT8U *)&reset_info, sizeof(HOST_RESET_STATUS_T));
    reset_info.status = type;
    DAL_PP_StoreParaByID(PP_ID_HOST_RESET, (INT8U *)&reset_info, sizeof(HOST_RESET_STATUS_T));
    
    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteBYTE_Strm(wstrm, type);
    return YX_MMI_DirSend(UP_PE_CMD_HOST_RESET_INFORM_NEW, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm));
}

#endif

