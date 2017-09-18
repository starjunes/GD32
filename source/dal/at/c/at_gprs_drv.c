/********************************************************************************
**
** 文件名:     adp_gprs_drv.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现GPRS的上下文管理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2013/03/15 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "yx_include.h"
#include "yx_misc.h"
#include "at_recv.h"
#include "at_send.h"
#include "at_core.h"
#include "at_cmd_common.h"
#include "at_q_sms.h"
#include "at_q_set.h"
#include "at_q_tcpip.h"
 
#include "at_urc_tcpip.h"
#include "at_gprs_drv.h"
#include "at_socket_drv.h"


#if EN_AT > 0

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

typedef struct {
    INT8U status;
    //char  localip[16];
    void (*callback_setapn)(INT8U result, INT32S error_code);
    void (*callback_getapn)(INT8U id, INT8U result, INT32S error_code, INT8U *apn, INT8U *userid, INT8U *password);
    void (*callback_getipbyname)(INT8U contxtid, BOOLEAN result, INT32S error, char *domainname, INT8U num, INT32U *ip);
} DCB_T;

/*
********************************************************************************
* define module variants
********************************************************************************
*/
static DCB_T s_dcb;
static GPRS_CALLBACK_T s_gprs_callback;




/*******************************************************************
** 函数名:     InformGprsActive
** 函数描述:   通知上下文已激活
** 参数:       [in]  contxtid: GPRS上下文通道编号0－1
** 返回:       成功返回0，失败返回-1
********************************************************************/
static void InformGprsActive(INT8U contexid)
{
    if (s_dcb.status != PDP_STATE_ACTIVING) {
        return;
    }
    
    s_dcb.status = PDP_STATE_ACTIVED;
    if (s_gprs_callback.callback_network_actived != 0) {
        s_gprs_callback.callback_network_actived(contexid);
    }
}

/*******************************************************************
** 函数名:     InformGprsDeactive
** 函数描述:   通知上下文已去活
** 参数:       [in]  contxtid: GPRS上下文通道编号0－1
** 返回:       成功返回0，失败返回-1
********************************************************************/
static void InformGprsDeactive(INT8U contexid, INT32S error_cause, INT32S error)
{
    if (s_dcb.status == PDP_STATE_FREE) {
        return;
    }
    
    s_dcb.status = PDP_STATE_FREE;
    if (s_gprs_callback.callback_network_deactived != 0) {
        s_gprs_callback.callback_network_deactived(contexid);
    }
}

/*******************************************************************
** 函数名:     InformGetIpByDomainName
** 函数描述:   根据域名获取IP地址结果通知
** 参数:       [in]  result: true-解析成功，false-解析失败
**             [in]  domainname: 域名
**             [in]  num:    IP个数
**             [in]  ip:     IP数组
** 返回:       成功返回0，失败返回-1
********************************************************************/
static void InformGetIpByDomainName(INT8U result, char *domainname, INT8U num, INT32U *ip)
{
    if (result && num > 0) {
        if (s_dcb.callback_getipbyname != 0) {
            s_dcb.callback_getipbyname(PDP_COM_0, true, 0, domainname, num, ip);
        }
    } else {
        if (s_dcb.callback_getipbyname != 0) {
            s_dcb.callback_getipbyname(PDP_COM_0, false, 0, domainname, 0, ip);
        }
    }
}

/*******************************************************************
** 函数名:     AT_GPRS_InitDrv
** 函数描述:   初始化GPRS网络，初始化GPRS上下文
** 参数:       无
** 返回:       无
********************************************************************/
void AT_GPRS_InitDrv(void)
{
    AT_GPRS_CALLBACK_T callback;
    
    YX_MEMSET(&s_dcb, 0, sizeof(s_dcb));
    YX_MEMSET(&s_gprs_callback, 0, sizeof(s_gprs_callback));
    
    callback.callback_network_actived   = InformGprsActive;
    callback.callback_network_deactived = InformGprsDeactive;
    callback.callback_getipbyname       = InformGetIpByDomainName;
    
    AT_URC_RegistGprsHandler(&callback);
}

/*******************************************************************
** 函数名:     AT_GPRS_RegistHandler
** 函数描述:   注册GPRS消息处理器
** 参数:       [in]  callback: 注册回调函数参数
** 返回:       成功返回TRUE, 失败返回FALSE
********************************************************************/
BOOLEAN AT_GPRS_RegistHandler(GPRS_CALLBACK_T *callback)
{
    YX_MEMCPY(&s_gprs_callback, sizeof(s_gprs_callback), callback, sizeof(s_gprs_callback));
    return true;
}

/*******************************************************************
** 函数名:     AT_GPRS_GetPdpStatus
** 函数描述:   GPRS上下文状态
** 参数:       无
** 返回:       返回上下文状态
********************************************************************/
INT8U AT_GPRS_GetPdpStatus(void)
{
    return s_dcb.status;
}

/*******************************************************************
** 函数名:     AT_GPRS_ClosePdpContext
** 函数描述:   关闭上下文
** 参数:       无
** 返回:       无
********************************************************************/
void AT_GPRS_ClosePdpContext(void)
{
    s_dcb.status = 0;
}

/*******************************************************************
** 函数名:     AT_GPRS_SetApn
** 函数描述:   设置APN
** 参数:       [in]  profileid: 上下文参数编号0－9,见PROFILE_ID_E
**             [in]  apn:       APN，以'\0'未结束符
**	           [in]  user:      用户名，以'\0'未结束符
**	           [in]  psw:       密码，以'\0'未结束符
**	           [in]  callback:  设置结果回调
** 返回:       0表示已设置成功；
**            -1表示阻塞中，等待callback回调；
**            -2表示参数有误；
**            -3其他错误
********************************************************************/
static void Callback_SetApn(INT8U result)
{
    if (result == _SUCCESS) {
        if (s_dcb.callback_setapn != 0) {
            s_dcb.callback_setapn(true, 0);
        }
    } else {
        if (s_dcb.callback_setapn != 0) {
            s_dcb.callback_setapn(false, -1);
        }
    }
}

static void Callback_SetMux(INT8U result)
{
    if (result == _SUCCESS) {
        ;
    } else {
        ;
    }
}

INT32S AT_GPRS_SetApn(INT8U profileid, char *apn, char *userid, char *password, void (*callback)(INT8U result, INT32S error_code))
{
    s_dcb.callback_setapn = callback;
    
    AT_Q_ClearTcpipQueue();                                                    /* 清除AT_SET模块中的GPRS命令 */
    AT_Q_DeactivePDPContext(0);                                                /* 关闭GPRS */
    AT_Q_SetMultiSocket(Callback_SetMux);                                      /* 设置多路socket支持 */
    
    if (AT_Q_SetAuthentication(Callback_SetApn, profileid, apn, userid, password)) {/* 设置APN */
        return -1;
    } else {
        return -3;
    }
}

/*******************************************************************
** 函数名:     AT_GPRS_GetApn
** 函数描述:   获取APN
** 参数:       [in]  profileid:上下文参数编号0－9，见PROFILE_ID_E
**	           [in]  callback: 获取结果回调
** 返回:       0表示操作成功；
**            -1表示阻塞中，等待callback回调；
**            -2表示参数有误；
**            -3其他错误
********************************************************************/
static void Callback_GetApn(INT8U result)
{
    if (result == _SUCCESS) {
        if (s_dcb.callback_getapn != 0) {
            s_dcb.callback_getapn(PDP_COM_0, true, 0, 0, 0, 0);
        }
    } else {
        if (s_dcb.callback_getapn != 0) {
            s_dcb.callback_getapn(PDP_COM_0, false, 0, 0, 0, 0);
        }
    }
}

INT32S AT_GPRS_GetApn(INT8U profileid, void (*callback)(INT8U id, INT8U result, INT32S error_code, INT8U *apn, INT8U *userid, INT8U *password))
{
    s_dcb.callback_getapn = callback;
    
    if (Callback_GetApn != 0) {
        ;
    }
    return -3;
}

/*******************************************************************
** 函数名:     AT_GPRS_ActivePDPContext
** 函数描述:   激活GPRS上下文
** 参数:       [in]  contxtid: GPRS上下文通道编号0－1, 见PDP_COM_E
** 返回:       0表示操作成功；
**            -1表示阻塞中，等待callback回调；
**            -2表示参数有误；
**            -3表示物理链路已断；
**            -4其他错误
********************************************************************/
static void Callback_GetLocalIp(INT8U result)
{
    //INT8U len;
    
    if (result == _SUCCESS) {
        #if DEBUG_AT > 0
        printf_com("<Callback_GetLocalIp(%s)>\r\n", g_at_ack_info.localip.ip);
        #endif
        
        /*len = YX_STRLEN((char *)g_at_ack_info.localip.ip);
        if (len > sizeof(s_dcb.localip) - 1) {
            YX_STRCPY(s_dcb.localip, "0.0.0.0");
        } else {
            YX_MEMCPY(s_dcb.localip, sizeof(s_dcb.localip), g_at_ack_info.localip.ip, sizeof(s_dcb.localip));
        }*/

        InformGprsActive(PDP_COM_0);
    }
//AT_Q_GetIpByDomainName(0, "www.sohu.com");
}

static void Callback_Active(INT8U result)
{
    if (result == _SUCCESS) {
        AT_Q_GetLocalIp(Callback_GetLocalIp);                                  /* 获得IP地址 */
    }
}

INT32S AT_GPRS_ActivePDPContext(INT8U contxtid)
{
    if (s_dcb.status == PDP_STATE_ACTIVING) {
        return -1;
    }
        
    if (AT_Q_ActivePDPContext(Callback_Active)) {                              /* 激活GPRS */
        s_dcb.status = PDP_STATE_ACTIVING;
        return -1;
    } else {
        return -4;
    }
}

/*******************************************************************
** 函数名:     AT_GPRS_DeactivePDPContext
** 函数描述:   去活GPRS上下文
** 参数:       [in]  contxtid: GPRS上下文通道编号0－1, 见PDP_COM_E
** 返回:       0表示操作成功；
**            -1表示阻塞中，等待callback回调；
**            -2表示参数有误；
**            -4其他错误
********************************************************************/
static void Callback_Deactive(INT8U result)
{
    //if (result == _SUCCESS) {
        InformGprsDeactive(PDP_COM_0, 0, 0);
    //}
}

INT32S AT_GPRS_DeactivePDPContext(INT8U contxtid)
{
    AT_Q_ClearTcpipQueue();                                                    /* 清除AT_SET模块中的所有GPRS命令 */
    AT_SOCKET_CloseAllSocket();
    
    if (AT_Q_DeactivePDPContext(Callback_Deactive)) {                          /* 发送关闭GPRS命令 */
        s_dcb.status = PDP_STATE_DEACTIVING;
        return -1;
    } else {
        return -4;
    }
}

/*******************************************************************
** 函数名:     AT_GPRS_GetLocalIpAddr
** 函数描述:   获得本地IP地址
** 参数:       [in]  contxtid: GPRS上下文通道编号0－1
**             [out] ipaddr:   ip地址,依次从从高位到低位
** 返回:       成功返回0，失败返回-1
********************************************************************/
INT32S AT_GPRS_GetLocalIpAddr(INT8U contxtid, INT32U *ipaddr)
{
    /*INT8U snbits;
    
    if (YX_ConvertIpStringToHex(ipaddr, &snbits, s_dcb.localip) == 0) {
        return 0;
    } else {
        *ipaddr = 0;
        return -1;
    }*/
    
    return -1;
}

/*******************************************************************
** 函数名:     AT_GPRS_GetIpByDomainName
** 函数描述:   根据域名获取IP地址
** 参数:       [in]  contxtid:   GPRS上下文通道编号0－1
**             [in]  domainname: 域名，如“www.yaxon.com”
**             [in]  fp:         异步回调函数
** 返回:       成功返回true，失败返回false
********************************************************************/
static void Callback_GetIpByDomainName(INT8U result)
{
    if (result == _SUCCESS) {
        //InformGetIpByDomainName(true, 0, 0, 0);
    } else {
        InformGetIpByDomainName(false, 0, 0, 0);
    }
}

BOOLEAN AT_GPRS_GetIpByDomainName(INT8U contxtid, char *domainname, void (*fp)(INT8U contxtid, BOOLEAN result, INT32S error, char *domainname, INT8U num, INT32U *ip))
{
    s_dcb.callback_getipbyname = fp;
    
    return AT_Q_GetIpByDomainName(Callback_GetIpByDomainName, domainname);
}

/*******************************************************************
** 函数名:     AT_GPRS_SetDnsAddress
** 函数描述:   设置域名服务器地址
** 参数:       [in]  contxtid:   GPRS上下文通道编号0－1
**             [in]  num:        ip个数,一般为2个
**             [in]  ip:         ip地址,依次从从高位到低位
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN AT_GPRS_SetDnsAddress(INT8U contxtid, INT8U num, INT32U *ip)
{
 #if 0
    INT8U i;
    char *ipstr;
    char ipbuf[2][16];
    
    YX_MEMSET(ipbuf, 0, sizeof(ipbuf));
    
    for (i = 0; i < num && i < 2; i++) {
        ipstr = YX_ConvertIpHexToString(ip[i]);
        YX_MEMCPY(ipbuf[i], ipstr, sizeof(ipbuf[i]));
    }
    
    return AT_Q_SetDNS(0, ipbuf[0], ipbuf[1]);
#endif
    
    return FALSE;
}

/*******************************************************************
** 函数名:     AT_GPRS_GetDnsAddress
** 函数描述:   获取域名服务器地址
** 参数:       [in]  contxtid:   GPRS上下文通道编号0－1
**             [in]  i_num:      ip缓存个数
**             [out] ip:         ip缓存,依次从从高位到低位
**             [out] o_num:      返回ip个数
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN AT_GPRS_GetDnsAddress(INT8U contxtid, INT8U i_num, INT32U *ip, INT8U *o_num)
{
    return false;
}

#endif

