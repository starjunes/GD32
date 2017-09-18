/********************************************************************************
**
** 文件名:     yx_jt_linkman.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现服务器UDP和TCP链路统一管理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2011/02/09 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "yx_include.h"
#include "yx_stream.h"
#include "at_drv.h"
#include "st_rtc_drv.h"
#include "dal_input_drv.h"
#include "dal_pp_drv.h"
#include "yx_protocol_type.h"
#include "yx_protocol_send.h"
#include "yx_jt1_tlink.h"
#include "yx_jt_linkman.h"
#include "yx_debug.h"

/*
********************************************************************************
* 定义配置参数
********************************************************************************
*/
//#define COM_TCP              SOCKET_CH_0          /* GPRS通信通道 */

/*
********************************************************************************
* 定义配置参数
********************************************************************************
*/

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    INT8U  jtt;
} MCB_T;
/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static MCB_T s_mcb;
static INT8U s_scantmr;


/*******************************************************************
**  函数名:     Callback_Actived
**  函数描述:   GPRS上下文激活回调
**  参数:       [in] contexid: 上下文ID
**  返回:       无
********************************************************************/
static void Callback_Actived(INT8U contexid)
{
    YX_JTT1_LinkInformGprsActived(true);
    YX_JTT2_LinkInformGprsActived(true);
}

/*******************************************************************
**  函数名:     Callback_Actived
**  函数描述:   GPRS上下文去活回调
**  参数:       [in] contexid: 上下文ID
**  返回:       无
********************************************************************/
static void Callback_Deactived(INT8U contexid)
{
    YX_JTT1_LinkInformGprsActived(false);
    YX_JTT2_LinkInformGprsActived(false);
}

/*******************************************************************
** 函数名:     Callback_Connect
** 函数描述:   通知socket已连接
** 参数:       [in] socketid:   socket编号
** 返回:       无
********************************************************************/
static void Callback_Connect(INT8U contexid, INT8S sock, INT8U result, INT32S error_code)
{
    if (sock == SOCKET_CH_0) {
        YX_JTT1_LinkInformConnect(true);
    }
    
    if (sock == SOCKET_CH_1) {
        YX_JTT2_LinkInformConnect(true);
    }
}

/*******************************************************************
** 函数名:     Callback_Close
** 函数描述:   通知socket已断开
** 参数:       [in] socketid:   socket编号
** 返回:       无
********************************************************************/
static void Callback_Close(INT8U contexid, INT8S sock, INT8U result, INT32S error_code)
{
    if (sock == SOCKET_CH_0) {
        YX_JTT1_LinkInformConnect(false);
    }
    
    if (sock == SOCKET_CH_1) {
        YX_JTT2_LinkInformConnect(false);
    }
}

/*******************************************************************
** 函数名:     Callback_Recv
** 函数描述:   处理接收的数据
** 参数:       [in] socketid:  sock编号
**             [in] sptr:      接收数据缓冲区
**             [in] len:       接收数据缓冲区长度
** 返回:       无
********************************************************************/
static void Callback_Recv(INT8U contexid, INT8S sock, INT8U *sptr, INT32U slen)
{
    if (sock == SOCKET_CH_0) {
        YX_JTT1_RecvData(sock, sptr, slen);
    }
    
    if (sock == SOCKET_CH_1) {
        YX_JTT2_RecvData(sock, sptr, slen);
    }
}

/*******************************************************************
** 函数名:     YX_JT_GetPositionInfo
** 函数描述:   获取主动上报位置信息
** 参数:       [in] wstrm: 数据流
** 返回:       无
********************************************************************/
void YX_JT_GetPositionInfo(STREAM_T *wstrm)
{
    INT8U weekday;
    INT32U subsecond, status;
    INT8U simcard, creg, cgreg, rssi, ber;
    GPS_DATA_T gpsdata;
    SYSTIME_T systime;
    
    DAL_PP_ReadParaByID(PP_ID_GPSDATA, (INT8U *)&gpsdata, sizeof(gpsdata));
    /* 基本信息 */
    if (DAL_INPUT_ReadFilterStatus(IPT_POWDECT)) {
        YX_WriteLONG_Strm(wstrm, 0x0100);                                      /* 报警标志 */
    } else {
        YX_WriteLONG_Strm(wstrm, 0);                                           /* 报警标志 */
    }
    
    status = 0;
    if ((gpsdata.flag & 0x08) != 0) {                                          /* 上传定位标志 */
        status |= 0x00000002;
    }
    if ((gpsdata.flag & 0x01) != 0) {                                          /* 经纬度方向 */
        status |= 0x00000008;
    }
    if ((gpsdata.flag & 0x02) != 0) {
        status |= 0x00000004;
    }
    if ((gpsdata.flag & 0x10) != 0) {                                          /* 定位模式 */
        status |= 0x00040000;
    }
    if ((gpsdata.flag & 0x20) != 0) {
        status |= 0x00080000;
    }
    
    YX_WriteLONG_Strm(wstrm, status);                                          /* 状态 */
    YX_WriteLONG_Strm(wstrm, YX_ConvertLatitudeOrLongitude(gpsdata.latitude)); /* 纬度 */
    YX_WriteLONG_Strm(wstrm, YX_ConvertLatitudeOrLongitude(gpsdata.longitude));/* 经度 */
    YX_WriteHWORD_Strm(wstrm, gpsdata.altitude);                               /* 高程 */
    YX_WriteHWORD_Strm(wstrm, 0);                                              /* 速度,1/10km/h */
    YX_WriteHWORD_Strm(wstrm, 0);                                              /* 方向 */
    
    if (ST_RTC_GetSystime(&systime.date, &systime.time, &weekday, &subsecond)) {/* 时间 */
        YX_HexToBcd((INT8U *)&systime, 6, (INT8U *)YX_GetStrmPtr(wstrm), YX_GetStrmLeftLen(wstrm));   
    } else {
        YX_HexToBcd((INT8U *)&gpsdata.systime, 6, (INT8U *)YX_GetStrmPtr(wstrm), YX_GetStrmLeftLen(wstrm));
    }
    YX_MovStrmPtr(wstrm, 6);
    
    /* 附加信息 */
    YX_WriteBYTE_Strm(wstrm, TAG_ALARM_METER);                                 /* 总里程 */
    YX_WriteBYTE_Strm(wstrm, 4);
    YX_WriteLONG_Strm(wstrm, gpsdata.odometer / 100);
    
    YX_WriteBYTE_Strm(wstrm, TAG_IO_STAT);                                     /* 扩展状态,睡眠 */
    YX_WriteBYTE_Strm(wstrm, 2);
    YX_WriteHWORD_Strm(wstrm, 0x0001);
    
    ADP_NET_GetNetworkState(&simcard, &creg, &cgreg, &rssi, &ber);
    YX_WriteBYTE_Strm(wstrm, TAG_GSMSIGNAL);                                   /* 网络信号 */
    YX_WriteBYTE_Strm(wstrm, 1);
    YX_WriteBYTE_Strm(wstrm, rssi);
#if 0
    YX_WriteBYTE_Strm(wstrm, TAG_GNSSNUM);                                     /* 定位卫星数 */
    YX_WriteBYTE_Strm(wstrm, 1);
    YX_WriteBYTE_Strm(wstrm, 12);
#endif
}

/*******************************************************************
** 函数名:     YX_JTT_SendAutoReptData
** 函数描述:   发送主动上报数据
** 参数:       [in] com: 通道号
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN YX_JTT_SendAutoReptData(INT8U com)
{
    INT32U msgattrib;
    STREAM_T *wstrm;
    PROTOCOL_COM_T attrib;
    TEL_T alarmtel;
    
    DAL_PP_ReadParaByID(PP_ID_ALARMTEL, (INT8U *)&alarmtel, sizeof(alarmtel));
    if (alarmtel.tellen < sizeof(attrib.tel)) {                                /* 手机号码 */
        YX_MEMCPY(attrib.tel, alarmtel.tellen, alarmtel.tel, alarmtel.tellen);
        attrib.tel[alarmtel.tellen] = '\0';
    }
    attrib.attrib   = SM_ATTR_TCP;                                             /* 发送属性 */
    attrib.channel  = com;                                                     /* 发送通道 */
    attrib.type     = PTOTOCOL_TYPE_LOG;                                       /* 协议数据类型 */
    attrib.priority = PTOTOCOL_PRIO_LOW;                                       /* 协议数据优先级 */
    
    msgattrib = 0;                                                             /* 消息属性 */
    wstrm = YX_PROTOCOL_GetBufferStrm();                                       /* 获取数据流缓存 */
    YX_PROTOCOL_AsmFrameHead(wstrm, UP_CMD_GPS_INFO, msgattrib, 0);            /* 组帧数据头 */
    YX_JT_GetPositionInfo(wstrm);
    return YX_PROTOCOL_SendData(wstrm, &attrib, 0);
}

/*******************************************************************
** 函数名:     ScanTmrProc
** 函数描述:   定时器
** 参数:       [in] pdata：定时器特征值
** 返回:       无
********************************************************************/
static void ScanTmrProc(void *pdata)
{
    AUTOREPT_PARA_T autopara;
    
    DAL_PP_ReadParaByID(PP_ID_AUTOREPT, (INT8U *)&autopara, sizeof(autopara));
    
    if (YX_JTT1_LinkCanCom()) {
        OS_StartTmr(s_scantmr, _SECOND, autopara.period);
        YX_JTT_SendAutoReptData(SOCKET_CH_0);
    }
    
    if (YX_JTT2_LinkCanCom()) {
        OS_StartTmr(s_scantmr, _SECOND, autopara.period);
        YX_JTT_SendAutoReptData(SOCKET_CH_1);
    }
}

/*******************************************************************
**  函数名:     InformAutoReptParaChange
**  函数描述:   参数变化处理
**  参数:       [in] reason: 参数改变原因
**  返回:       无
********************************************************************/
static void InformAutoReptParaChange(INT8U reason)
{
    AUTOREPT_PARA_T autopara;
    
    DAL_PP_ReadParaByID(PP_ID_AUTOREPT, (INT8U *)&autopara, sizeof(autopara));
    OS_StartTmr(s_scantmr, _SECOND, autopara.period);
}

/*******************************************************************
** 函数名:     YX_JT_InitLinkMan
** 函数描述:   中心前置机链路管理
** 参数:       无
** 返回:       无
********************************************************************/
void YX_JT_InitLinkMan(void)
{
    GPRS_CALLBACK_T callback_gprs;
    SOCKET_CALLBACK_T callback_socket;
    
    YX_MEMSET(&s_mcb, 0, sizeof(s_mcb));
    
    YX_JTT1_InitLink();
    YX_JTT2_InitLink();
    
    s_scantmr  = OS_CreateTmr(TSK_ID_APP, (void *)0, ScanTmrProc);
    OS_StartTmr(s_scantmr, _SECOND, 1);
    
    /* 注册GPRS事件处理器 */
    callback_gprs.callback_network_actived   = Callback_Actived;
    callback_gprs.callback_network_deactived = Callback_Deactived;
    AT_GPRS_RegistHandler(&callback_gprs);
    
    /* 注册SOCKET事件处理器 */
    callback_socket.callback_socket_connect = Callback_Connect;
    callback_socket.callback_socket_close   = Callback_Close;
    callback_socket.callback_socket_recv    = Callback_Recv;
    callback_socket.callback_socket_send    = 0;
    AT_SOCKET_RegistHandler(&callback_socket);
    
    DAL_PP_RegParaChangeInformer(PP_ID_AUTOREPT, InformAutoReptParaChange);
}

/*******************************************************************
** 函数名:     YX_JT_OpenLink
** 函数描述:   申请应用
** 参数:       无
** 返回:       无
********************************************************************/
void YX_JT_OpenLink(void)
{
    YX_JTT1_OpenLink();
    YX_JTT2_OpenLink();
}

/*******************************************************************
** 函数名:     YX_JT_CloseLink
** 函数描述:   释放应用
** 参数:       无
** 返回:       无
********************************************************************/
void YX_JT_CloseLink(void)
{
    YX_JTT1_CloseLink();
    YX_JTT2_CloseLink();
}

/*******************************************************************
** 函数名:     YX_JTT_LinkCanCom
** 函数描述:   TCP通道是否可以通信
** 参数:       [in] com: TCP通道, 见TCP_USER_E
** 返回:       是返回true，否返回false
********************************************************************/
BOOLEAN YX_JTT_LinkCanCom(INT8U com)
{
    switch (com)
    {
    case SOCKET_CH_0:
        return YX_JTT1_LinkCanCom();
    case SOCKET_CH_1:
        return YX_JTT2_LinkCanCom();
    default:
        return false;
    }
}





