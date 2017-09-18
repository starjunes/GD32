/********************************************************************************
**
** 文件名:     at_urc_tcpip.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现手机模块TCPIP数据接收解析处理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2014/06/15 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "yx_include.h"
#include "yx_misc.h"
#include "yx_dym_drv.h"
#include "at_com.h"
#include "at_recv.h"
#include "at_send.h"
 
#include "at_urc_tcpip.h"



#if EN_AT > 0

/*
********************************************************************************
* 定义模块配置参数
********************************************************************************
*/


/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    SOCKET_CALLBACK_T callback_socket;
    AT_GPRS_CALLBACK_T callback_gprs;
} RCB_T;

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static RCB_T s_rcb;





//#if GSM_TYPE == GSM_SIM6320
#if 1
/*
********************************************************************************
* 去活GPRS上下文
********************************************************************************
*/
static INT8U Handler_DeactiveGprs(INT8U ct_recv, INT8U event, INT8U *sptr, INT16U slen)
{
    if (s_rcb.callback_gprs.callback_network_deactived != 0) {
        s_rcb.callback_gprs.callback_network_deactived(PDP_COM_0, 0, 0);
    }

    return AT_SUCCESS;
}
/*
********************************************************************************
* 激活GPRS上下文
********************************************************************************
*/
static INT8U Handler_ActiveGprs(INT8U ct_recv, INT8U event, INT8U *sptr, INT16U slen)
{
    INT8U result;
    
    result = YX_SearchDigitalString(sptr, 30, '\r', 1);
    if (result == 0) {
        if (s_rcb.callback_gprs.callback_network_actived != 0) {
            s_rcb.callback_gprs.callback_network_actived(PDP_COM_0);
        }
    }

    return AT_SUCCESS;
}

/*
********************************************************************************
* HANDLER: socket连接成功
********************************************************************************
*/
static INT8U Handler_SocketConnect(INT8U ct_recv, INT8U event, INT8U *sptr, INT16U slen)
{
    INT8U socket;
    
    socket = YX_SearchDigitalString(sptr, 30, ',', 1);
    if (socket >= SOCKET_CH_MAX) {
        return AT_FAILURE;
    }
    
    if (s_rcb.callback_socket.callback_socket_connect != 0) {
        s_rcb.callback_socket.callback_socket_connect(PDP_COM_0, socket, true, 0);
    }
    
    return AT_SUCCESS;
}

/*
********************************************************************************
* HANDLER: socke已断开或者连接失败
********************************************************************************
*/
static INT8U Handler_SocketClose(INT8U ct_recv, INT8U event, INT8U *sptr, INT16U slen)
{
    INT8U socket;
    
    socket = YX_SearchDigitalString(sptr, 30, ',', 1);
    if (socket >= SOCKET_CH_MAX) {
        return AT_FAILURE;
    }
    
    if (s_rcb.callback_socket.callback_socket_close != 0) {
        s_rcb.callback_socket.callback_socket_close(PDP_COM_0, socket, true, 0);
    }
    return AT_SUCCESS;
}

/*
********************************************************************************
* HANDLER: QUERY CONNECT
********************************************************************************
*/
static INT8U Handler_QuerySocket(INT8U ct_recv, INT8U event, INT8U *sptr, INT16U slen)
{
    INT8U i, result;
    
    if (slen < 20) {
        return AT_SUCCESS;
    }
    
    for (i = 0; i < SOCKET_CH_MAX; i++) {
        result = YX_SearchDigitalString(sptr, 30, ',', i + 1);
        if (result) {
            if (s_rcb.callback_socket.callback_socket_connect != 0) {
                s_rcb.callback_socket.callback_socket_connect(PDP_COM_0, i, true, 0);
            }
        } else {
            if (s_rcb.callback_socket.callback_socket_close != 0) {
                s_rcb.callback_socket.callback_socket_close(PDP_COM_0, i, true, 0);
            }
        }
    }
    return AT_SUCCESS;
}

/*
********************************************************************************
* HANDLER: socket数据接收
********************************************************************************
*/
static INT8U Handler_SocketRecv(INT8U ct_recv, INT8U event, INT8U *sptr, INT16U slen)
{
    INT16U iplen, socket;
    
    iplen = YX_SearchDigitalString(sptr, 30, ':', 1);
    if (iplen >= slen) {
        iplen = YX_SearchDigitalString(sptr, 30, '\r', 1);
        if (iplen >= slen) {
            return AT_SUCCESS;
        }
    }
    
    socket = YX_SearchDigitalString(sptr, 30, ',', 2);
    if (socket >= SOCKET_CH_MAX) {
        return AT_SUCCESS;
    }
    sptr += (slen - iplen);
    
    if (s_rcb.callback_socket.callback_socket_recv != 0) {
        s_rcb.callback_socket.callback_socket_recv(PDP_COM_0, socket, sptr, iplen);
    }
    
    return AT_SUCCESS;
}
/*
********************************************************************************
* HANDLER: socket数据发送
********************************************************************************
*/
static INT8U Handler_SocketSend(INT8U ct_recv, INT8U event, INT8U *sptr, INT16U slen)
{
    INT16U iplen, socket;
    
    socket = YX_SearchDigitalString(sptr, 30, ',', 1);
    //socket = SOCKET_CH_0;
    if (socket >= SOCKET_CH_MAX) {
        return AT_FAILURE;
    }
    
    iplen = YX_SearchDigitalString(sptr, 30, '\r', 1);
    
    if (s_rcb.callback_socket.callback_socket_send != 0) {
        s_rcb.callback_socket.callback_socket_send(PDP_COM_0, socket, true, 0, iplen);
    }
    
    return AT_SUCCESS;
}
/*
********************************************************************************
* HANDLER: SEND FAIL
********************************************************************************
*/
static INT8U Handler_SocketSendFail(INT8U ct_recv, INT8U event, INT8U *sptr, INT16U slen)
{
    INT16U socket;
    
    socket = YX_SearchDigitalString(sptr, 30, ',', 1);
    if (socket >= SOCKET_CH_MAX) {
        return AT_FAILURE;
    }
    
    if (s_rcb.callback_socket.callback_socket_send != 0) {
        s_rcb.callback_socket.callback_socket_send(PDP_COM_0, socket, false, 0, 0);
    }
    
    return AT_SUCCESS;
}

/*
********************************************************************************
* HANDLER: +IPD
********************************************************************************
*/
static INT8U Handler_IPD(INT8U ct_recv, INT8U event, INT8U *sptr, INT16U slen)
{
    INT16U iplen;
    
    event = event;

    if ((iplen = YX_SearchDigitalString(sptr, slen, ':', 1)) >= slen) {
        return AT_SUCCESS;
    }
    sptr += (slen - iplen);
    //HdlMsg_GPRS_DATA(sptr, iplen);
    return AT_SUCCESS;
}

/*
********************************************************************************
* HANDLER: +CDNSGIP:根据域名获取IP地址
********************************************************************************
*/
static INT8U Handler_GetIpByDomainName(INT8U ct_recv, INT8U event, INT8U *sptr, INT16U slen)
{
    INT8U i, result, pos, snbits;
    INT32U ip[2];
    char temp[16];
    char domainname[50];
    
    domainname[0] = '\0'; 
    result = YX_SearchDigitalString(sptr, slen, ',', 1);
    if (result == 1) {
        pos = YX_SearchString((INT8U *)domainname, sizeof(domainname) - 1, sptr, slen, '"', 1);/* 查找域名 */
        if (pos == 0) {
            if (s_rcb.callback_gprs.callback_getipbyname != 0) {
                s_rcb.callback_gprs.callback_getipbyname(false, domainname, 0, ip);
            }
            return AT_SUCCESS;
        }
        domainname[pos] = '\0';

        for (i = 0; i < sizeof(ip) / sizeof(ip[0]); i++) {                     /* 解析IP */
            pos = YX_SearchString((INT8U *)temp, sizeof(temp) - 1, sptr, slen, '"', i + 2);
            if (pos == 0) {
                break;
            }
            
            temp[pos] = '\0';
            if (YX_ConvertIpStringToHex(&ip[i], &snbits, temp) != 0) {
                break;
            }
        }
        
        if (s_rcb.callback_gprs.callback_getipbyname != 0) {
            s_rcb.callback_gprs.callback_getipbyname(true, domainname, i, ip);
        }
    } else {
        if (s_rcb.callback_gprs.callback_getipbyname != 0) {
            s_rcb.callback_gprs.callback_getipbyname(false, domainname, 0, ip);
        }
    }

    return AT_SUCCESS;
}


/*
********************************************************************************
* define receive control block
********************************************************************************
*/
static URC_HDL_TBL_T const s_hdl_tbl[] = {
                   {"+PDP: DEACT",          2,   true,   Handler_DeactiveGprs}
                  ,{"+NETCLOSE",            2,   true,   Handler_DeactiveGprs}
                  ,{"+NETOPEN",             2,   true,   Handler_ActiveGprs}
                  ,{"+CDNSGIP:",            2,   true,   Handler_GetIpByDomainName}
                  
                  ,{"ALREADY CONNECT",      2,   false,  Handler_SocketConnect}
                  ,{"CONNECT OK",           2,   false,  Handler_SocketConnect}
                  //,{"+CIPOPEN",             2,   false,  Handler_SocketConnect}
                  ,{"CONNECT FAIL",         2,   false,  Handler_SocketClose}
                  ,{"CLOSED",               2,   false,  Handler_SocketClose}
                  ,{"+CIPCLOSE:",           2,   true,   Handler_QuerySocket}
                  ,{"+IPCLOSE:",            2,   true,   Handler_SocketClose}
                  
                  ,{"RECV FROM",            2,   true,   0}
                  ,{"+RECEIVE",             2,   true,   Handler_SocketRecv}
                  ,{"DATA ACCEPT:",         2,   true,   Handler_SocketSend}
                  ,{"+CIPSEND:",            2,   true,   Handler_SocketSend}
                  ,{"SEND FAIL",            2,   false,  Handler_SocketSendFail}
                                        
                  ,{"+IPD",                 2,   true,   Handler_IPD}
                                        
                                     };
#endif


#if GSM_TYPE == GSM_SIM800

/*
********************************************************************************
* HANDLER: +PDP: DEACT
********************************************************************************
*/
static INT8U Handler_DeactiveGprs(INT8U ct_recv, INT8U event, INT8U *sptr, INT16U slen)
{
    AT_GPRS_InformPDPDeactive(0);
    return AT_SUCCESS;
}

/*
********************************************************************************
* HANDLER: +CDNSGIP:根据域名获取IP地址
********************************************************************************
*/
static INT8U Handler_GetIpByDomainName(INT8U ct_recv, INT8U event, INT8U *sptr, INT16U slen)
{
    INT8U i, result, pos, snbits;
    INT32U ip[2];
    char temp[16];
    char domainname[50];
    
    domainname[0] = '\0'; 
    result = YX_SearchDigitalString(sptr, slen, ',', 1);
    if (result == 1) {
        pos = YX_SearchString((INT8U *)domainname, sizeof(domainname) - 1, sptr, slen, '"', 1);/* 查找域名 */
        if (pos == 0) {
            AT_GPRS_InformGetIpByDomainName(false, domainname, 0, ip);
            return AT_SUCCESS;
        }
        domainname[pos] = '\0';

        for (i = 0; i < sizeof(ip) / sizeof(ip[0]); i++) {                     /* 解析IP */
            pos = YX_SearchString((INT8U *)temp, sizeof(temp) - 1, sptr, slen, '"', i + 2);
            if (pos == 0) {
                break;
            }
            
            temp[pos] = '\0';
            if (YX_ConvertIpStringToHex(&ip[i], &snbits, temp) != 0) {
                break;
            }
        }
        AT_GPRS_InformGetIpByDomainName(true, domainname, i, ip);
    } else {
        AT_GPRS_InformGetIpByDomainName(false, domainname, 0, ip);
    }

    return AT_SUCCESS;
}

/*
********************************************************************************
* HANDLER: CONNECT OK
********************************************************************************
*/
static INT8U Handler_SocketConnect(INT8U ct_recv, INT8U event, INT8U *sptr, INT16U slen)
{
    INT8U socket;
    
    socket = YX_SearchDigitalString(sptr, 30, ',', 1);
    if (socket >= SOCKET_CH_MAX) {
        return AT_SUCCESS;
    }
    
    AT_SOCKET_InformSocketConnect(socket);
    return AT_SUCCESS;
}

/*
********************************************************************************
* HANDLER: CONNECT FAIL
********************************************************************************
*/
static INT8U Handler_SocketClose(INT8U ct_recv, INT8U event, INT8U *sptr, INT16U slen)
{
    INT8U socket;
    
    socket = YX_SearchDigitalString(sptr, 30, ',', 1);
    if (socket >= SOCKET_CH_MAX) {
        return AT_SUCCESS;
    }
    
    AT_SOCKET_InformSocketDisconnect(socket);
    return AT_SUCCESS;
}

/*
********************************************************************************
* HANDLER: QUERY CONNECT
********************************************************************************
*/
static INT8U Handler_QuerySocket(INT8U ct_recv, INT8U event, INT8U *sptr, INT16U slen)
{
    INT8U i, result;
    
    for (i = 0; i < SOCKET_CH_MAX; i++) {
        result = YX_SearchDigitalString(sptr, 30, ',', i + 1);
        
    if (socket >= SOCKET_CH_MAX) {
        return AT_SUCCESS;
    }
    
    AT_SOCKET_InformSocketDisconnect(socket);
    return AT_SUCCESS;
}

/*
********************************************************************************
* HANDLER: +RECEIVE
********************************************************************************
*/
static INT8U Handler_SocketRecv(INT8U ct_recv, INT8U event, INT8U *sptr, INT16U slen)
{
    INT16U iplen, ch;
    
    iplen = YX_SearchDigitalString(sptr, 30, ':', 1);
    if (iplen >= slen) {
        iplen = YX_SearchDigitalString(sptr, 30, '\r', 1);
        if (iplen >= slen) {
            return AT_SUCCESS;
        }
    }
    
    ch = YX_SearchDigitalString(sptr, 30, ',', 2);
    if (ch >= SOCKET_CH_MAX) {
        return AT_SUCCESS;
    }
    
    sptr += (slen - iplen);
    AT_SOCKET_HdlRecvData(ch, sptr, iplen);
    return AT_SUCCESS;
}
/*
********************************************************************************
* HANDLER: DATA ACCEPT:
********************************************************************************
*/
static INT8U Handler_SocketSend(INT8U ct_recv, INT8U event, INT8U *sptr, INT16U slen)
{
    INT16U iplen, ch;
    
    ch = YX_SearchDigitalString(sptr, 30, ',', 1);
    if (ch >= SOCKET_CH_MAX) {
        return AT_SUCCESS;
    }
    
    iplen = YX_SearchDigitalString(sptr, 30, '\r', 1);
    AT_SOCKET_HdlSendDataAck(ch, iplen);
    
    return AT_SUCCESS;
}
/*
********************************************************************************
* HANDLER: SEND FAIL
********************************************************************************
*/
static INT8U Handler_SocketSendFail(INT8U ct_recv, INT8U event, INT8U *sptr, INT16U slen)
{
    INT16U ch;
    
    ch = YX_SearchDigitalString(sptr, 30, ',', 1);
    if (ch >= SOCKET_CH_MAX) {
        return AT_SUCCESS;
    }
    AT_SOCKET_HdlSendDataAck(ch, 0);
    
    return AT_SUCCESS;
}

/*
********************************************************************************
* HANDLER: +IPD
********************************************************************************
*/
static INT8U Handler_IPD(INT8U ct_recv, INT8U event, INT8U *sptr, INT16U slen)
{
    INT16U iplen;
    
    event = event;

    if ((iplen = YX_SearchDigitalString(sptr, slen, ':', 1)) >= slen) {
        return AT_SUCCESS;
    }
    sptr += (slen - iplen);
    //HdlMsg_GPRS_DATA(sptr, iplen);
    return AT_SUCCESS;
}


/*
********************************************************************************
* define receive control block
********************************************************************************
*/
static URC_HDL_TBL_T const s_hdl_tbl[] = {
                   {"+PDP: DEACT",          2,   true,   Handler_DeactiveGprs}
                  ,{"+CDNSGIP:",            2,   true,   Handler_GetIpByDomainName}
                  
                  ,{"ALREADY CONNECT",      2,   false,  Handler_SocketConnect}
                  ,{"CONNECT OK",           2,   false,  Handler_SocketConnect}
                  ,{"CONNECT FAIL",         2,   false,  Handler_SocketClose}
                  ,{"CLOSED",               2,   false,  Handler_SocketClose}
                  ,{"+CIPCLOSE:",           2,   true,   Handler_QuerySocket}
                  ,{"+RECEIVE",             2,   true,   Handler_SocketRecv}
                  ,{"DATA ACCEPT:",         2,   true,   Handler_SocketSend}
                  ,{"SEND FAIL",            2,   false,  Handler_SocketSendFail}
                                        
                  ,{"+IPD",                 2,   true,   Handler_IPD}
                                        
                                     };
#endif

/*******************************************************************
** 函数名:     AT_URC_InitTcpip
** 函数描述:   初始化函数
** 参数:       无
** 返回:       无
********************************************************************/
void AT_URC_InitTcpip(void)
{
    INT8U i;
    
    YX_MEMSET(&s_rcb, 0, sizeof(s_rcb));
    
    for (i = 0; i < sizeof(s_hdl_tbl) / sizeof(s_hdl_tbl[0]); i++) {
        AT_RECV_RegistUrcHandler((URC_HDL_TBL_T *)&s_hdl_tbl[i]);
    }
}

/*******************************************************************
** 函数名:     AT_URC_RegistGprsHandler
** 函数描述:   注册GPRS消息接收处理器
** 参数:       [in] callback:   消息处理器
** 返回:       注册成功返回true，注册失败返回false
********************************************************************/
BOOLEAN AT_URC_RegistGprsHandler(AT_GPRS_CALLBACK_T *callback)
{
    YX_MEMCPY(&s_rcb.callback_gprs, sizeof(s_rcb.callback_gprs), callback, sizeof(AT_GPRS_CALLBACK_T));
    return true;
}

/*******************************************************************
** 函数名:     AT_URC_RegistSocketHandler
** 函数描述:   注册SOCKET消息接收处理器
** 参数:       [in] callback:   消息处理器
** 返回:       注册成功返回true，注册失败返回false
********************************************************************/
BOOLEAN AT_URC_RegistSocketHandler(SOCKET_CALLBACK_T *callback)
{
    YX_MEMCPY(&s_rcb.callback_socket, sizeof(s_rcb.callback_socket), callback, sizeof(SOCKET_CALLBACK_T));
    return true;
}


#endif
