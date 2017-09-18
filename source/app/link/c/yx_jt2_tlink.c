/********************************************************************************
**
** 文件名:     yx_jt2_tlink.c
** 版权所有:   (c) 2005-2012 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现交通部协议第二服务器TCP注册、登陆管理
** 
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2013/10/21 | 叶德焰 |  创建第二版本
*********************************************************************************/
#include "yx_include.h"
#include "at_drv.h"
#include "at_gprs_drv.h"
#include "dal_pp_drv.h"
#include "yx_protocol_type.h"
#include "yx_jt_linkman.h"

/*
********************************************************************************
* 定义配置参数
********************************************************************************
*/
#define COM_TCP                     SOCKET_CH_1          /* GPRS通信通道 */

#define MAX_GETIP                   3                      /* 最大域名解析次数 */
#define MAX_ACTIVE                  3                      /* 最大激活次数*/
#define MAX_CONNECT                 3                      /* 最大连接次数 */
#define MAX_REG                     3                      /* 最大注册次数 */
#define MAX_LOG                     3                      /* 最大登入次数 */

/* 状态 */
#define APPLY_                      0x80                   /* 已申请应用 */
#define RECONNECT_                  0x40                   /* 重新连接 */
#define ACTIVE_                     0x01                   /* 激活上下文 */
#define CONNECT_                    0x02                   /* 连接成功 */
#define REGED_                      0x04                   /* 注册成功 */
#define LOGED_                      0x08                   /* 登入成功 */
#define MASK_                       0x3F                   /* 掩码 */

#define PERIOD_WAIT                 _SECOND, 10             /* 参数无效，延时尝试 */
#define PERIOD_ACTIVE               _SECOND, 30             /* 尝试激活的周期 */
#define PERIOD_CONNECT              _SECOND, 30             /* 尝试建链的周期 */
#define PERIOD_REG                  _SECOND, 30             /* 尝试注册的周期 */
#define PERIOD_LOG                  _SECOND, 30             /* 尝试登入的周期 */
#define DELAY_REG                   (10 * 60)

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    INT8U   status;
    INT8U   ct_active;                           /* 激活GPRS上下文次数 */
    INT8U   ct_connect;                          /* socket连接请求次数 */
    INT8U   ct_reg;                              /* 注册请求次数 */
    INT8U   ct_log;                              /* 登入请求次数 */
    INT8U   ct_logfail;
    INT8U   ct_heart;                            /* 心跳请求次数 */
    INT8U   curindex;                            /* 当前使用的ip参数组序号 */

    INT16U  heart_period;                        /* 心跳发送周期 */
    INT16U  heart_waittime;                      /* 心跳应答等待时间 */
    INT16U  heart_max;                           /* 心跳最大次数 */
} LCB_T;

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static INT8U s_monitortmr;
static LCB_T s_lcb;
static GPRS_PARA_T s_gprspara;



/*******************************************************************
** 函数名:     CheckAuthCode
** 函数描述:   检查终端注册/鉴权码是否有效
** 参数:       无
** 返回:       有效返回true，无效返回false
********************************************************************/
static BOOLEAN CheckAuthCode(void)
{
    INT16U len;
    AUTH_CODE_T authinfo;

    DAL_PP_ReadParaByID(PP_ID_AUTHCODE2, (INT8U *)&authinfo, sizeof(authinfo));
    len = YX_STRLEN((char *)authinfo.authcode);
    if (len >= sizeof(authinfo.authcode) || len == 0) {
        return FALSE;
    }
    
    return true;
}

/*******************************************************************
** 函数名:     SendHeartData
** 函数描述:   发送心跳数据
** 参数:       无
** 返回:       有效返回true，无效返回false
********************************************************************/
static BOOLEAN SendHeartData(void)
{
    LINK_PARA_T heart;
    
    if (DAL_PP_ReadParaByID(PP_ID_LINKPARA2, (INT8U *)&heart, sizeof(LINK_PARA_T))) {
        s_lcb.heart_period   = heart.period;
        s_lcb.heart_waittime = heart.tcp_waittime;
        s_lcb.heart_max      = heart.tcp_max;
        
        if (heart.period == 0 || heart.period > 0xffff) {
            s_lcb.heart_period = DEF_HEART_PERIOD;
        }
        if (heart.tcp_waittime == 0 || heart.tcp_waittime > 0xffff) {
            s_lcb.heart_waittime = DEF_HEART_WAITTIME;
        }
        if (heart.tcp_max == 0 || heart.tcp_max > 0xffff) {
            s_lcb.heart_max = DEF_HEART_RSENDCNT;
        }
    } else {
        s_lcb.heart_period   = DEF_HEART_PERIOD;
        s_lcb.heart_waittime = DEF_HEART_WAITTIME;
        s_lcb.heart_max      = DEF_HEART_RSENDCNT;
    }
    
    if (++s_lcb.ct_heart <= s_lcb.heart_max) {                                 /* 链路维护请求超过次数 */
        YX_JTT2_SendHeart();
        OS_StartTmr(s_monitortmr, _SECOND, s_lcb.heart_waittime);
    } else {
        #if DEBUG_TLINK > 0
        printf_com("<第二服务器心跳维护请求超过次数,周期(%d)秒,等待(%d),次数(%d)>\r\n", s_lcb.heart_period, s_lcb.heart_waittime, s_lcb.heart_max);
        #endif
        
        s_lcb.ct_heart = 0;
		YX_JTT2_LinkReconnect(0);
    }
    
    return true;
}

/*******************************************************************
** 函数名:     CheckGprsPara
** 函数描述:   检测GPRS参数是否有效
** 参数:       无
** 返回:       无
********************************************************************/
static BOOLEAN CheckGprsPara(void)
{
    INT8U i;
    char *apn, *username, *password;
    
    if (!DAL_PP_ReadParaByID(PP_ID_GPRSPARA2, (INT8U *)&s_gprspara, sizeof(s_gprspara))) {
        return false;
    }
    
    for (i = 0; i < IP_GROUP_NUM; i++) {                                       /* 寻找一个有效的ip参数 */
        s_lcb.curindex %= IP_GROUP_NUM;
        
        if (!s_gprspara.ippara[s_lcb.curindex].isvalid) {
            s_lcb.curindex++;
            s_lcb.curindex %= IP_GROUP_NUM;
        } else {
            break;
        }
    }
    
    #if DEBUG_TLINK > 0
    printf_com("<第二服务器TCP/IP参数组(%d)>\r\n", s_lcb.curindex);
    #endif
    
    //apn      = s_gprspara.ippara[s_lcb.curindex].apn;
    //username = s_gprspara.ippara[s_lcb.curindex].username;
    //password = s_gprspara.ippara[s_lcb.curindex].password;
    apn      = s_gprspara.apn;
    username = s_gprspara.username;
    password = s_gprspara.password;
    if (!s_gprspara.ippara[s_lcb.curindex].isvalid) {
        return false;
    }
    if ((YX_STRLEN(s_gprspara.ippara[s_lcb.curindex].ip) == 0 ) || s_gprspara.ippara[s_lcb.curindex].port == 0) {
        s_lcb.curindex++;                                                      /* 更换参数 */
        s_lcb.curindex %= IP_GROUP_NUM;
        return false;
    }

    if (ADP_NET_GetOperator() == NETWORK_OP_CT) {                           /* 电信CDMA */
        if (YX_STRLEN(username) == 0 || YX_STRLEN(password) == 0) {
            s_lcb.curindex++;
            s_lcb.curindex %= IP_GROUP_NUM;
            return false;
        }
    } else {
        if (YX_STRLEN(apn) == 0) {
            s_lcb.curindex++;
            s_lcb.curindex %= IP_GROUP_NUM;
            return false;
        }
    }
    
    return true;
}

/*******************************************************************
** 函数名:     Turninto_LOGED
** 函数描述:   中心登入成功
** 参数:       无
** 返回:       无
********************************************************************/
static void Turninto_LOGED(void)
{
    if ((s_lcb.status & (APPLY_ | MASK_)) == (APPLY_ | ACTIVE_ | CONNECT_ | REGED_)) { /* 已注册成功 */
        #if DEBUG_TLINK > 0
        printf_com("<第二服务器TCP登入成功>\r\n");
        #endif
        
        s_lcb.status |= LOGED_;
        s_lcb.ct_log = 0;
        s_lcb.ct_reg = 0;
        s_lcb.ct_connect = 0;
        s_lcb.ct_heart = 0;
        
        OS_StartTmr(s_monitortmr, _SECOND, 1);
    }
}

/*******************************************************************
** 函数名:     Turninto_REGED
** 函数描述:   中心注册成功
** 参数:       无
** 返回:       无
********************************************************************/
static void Turninto_REGED(void)
{
    if ((s_lcb.status & (APPLY_ | MASK_)) == (APPLY_ | ACTIVE_ | CONNECT_)) {
        #if DEBUG_TLINK > 0
        printf_com("<第二服务器TCP注册成功>\r\n");
        #endif
        
        s_lcb.status |= REGED_;
        OS_StartTmr(s_monitortmr, _SECOND, 1);                                 /* 延时几秒开始登入请求 */
    }
}

/*******************************************************************
** 函数名:     Turninto_Activing
** 函数描述:   进入激活GPRS上下文
** 参数:       无
** 返回:       无
********************************************************************/
static void Callback_SetApn(INT8U result, INT32S error_code)
{
    AT_GPRS_ActivePDPContext(PDP_COM_0);
}

static void Turninto_Activing(void)
{
    INT8U rssi, ber, simcard, creg, cgreg, pdpstatus;
    char *apn, *username, *password;
    
    if (!CheckGprsPara()) {                                                    /* 判断参数 */
        #if DEBUG_TLINK > 0
        printf_com("<第二服务器TCP/IP参数无效>\r\n");
        #endif
        
        OS_StartTmr(s_monitortmr, PERIOD_WAIT);
        return;
    }
    
    ADP_NET_GetNetworkState(&simcard, &creg, &cgreg, &rssi, &ber);
    if ((creg != NETWORK_STATE_HOME) && (creg != NETWORK_STATE_ROAMING)) {
        #if DEBUG_TLINK > 0
        printf_com("<未搜索到网络>\r\n");
        #endif
        
        OS_StartTmr(s_monitortmr, PERIOD_WAIT);
        return;
    }
    
    pdpstatus = AT_GPRS_GetPdpStatus();
    //if (pdpstatus == PDP_STATE_ACTIVING) {                                     /* 正在激活中 */
    //    OS_StartTmr(s_monitortmr, PERIOD_WAIT);
    //    return;
    if (pdpstatus == PDP_STATE_ACTIVED) {                               /* 已激活 */
        s_lcb.status = APPLY_ | ACTIVE_;
        s_lcb.ct_active = 0;
        OS_StartTmr(s_monitortmr, _SECOND, 3);
        return;
    }
    
    //apn      = s_gprspara.ippara[s_lcb.curindex].apn;
    //username = s_gprspara.ippara[s_lcb.curindex].username;
    //password = s_gprspara.ippara[s_lcb.curindex].password;
    apn      = s_gprspara.apn;
    username = s_gprspara.username;
    password = s_gprspara.password;
    if (++s_lcb.ct_active <= MAX_ACTIVE) {
        #if DEBUG_TLINK > 0
        printf_com("<请求激活GPRS上下文(%d)(%s)(%s)(%s)>\r\n", s_lcb.ct_active, apn, username, password);
        #endif
        
        AT_GPRS_SetApn(PROFILE_ID_0, apn, username, password, Callback_SetApn);
        OS_StartTmr(s_monitortmr, PERIOD_ACTIVE);
    } else {
        s_lcb.ct_active = 0;
        s_lcb.curindex++;
        CheckGprsPara();
        AT_GPRS_DeactivePDPContext(PDP_COM_0);
        OS_StartTmr(s_monitortmr, PERIOD_ACTIVE);
    }
}

/*******************************************************************
** 函数名:     Turninto_Connecting
** 函数描述:   进入连接状态
** 参数:       无
** 返回:       无
********************************************************************/
static void Callback_GetIpByDomainName(INT8U contxtid, BOOLEAN result, INT32S error, char *domainname, INT8U num, INT32U *ip)
{
    char *ipstr;
    
    if (result && num > 0) {
        #if DEBUG_TLINK > 0
        printf_com("<域名解析成功(%d)(%d)(%s)(0x%x)>\r\n", result, num, domainname, ip[0]);
        #endif
        
        if (YX_ACmpString(false, (INT8U *)domainname, (INT8U *)s_gprspara.ippara[s_lcb.curindex].ip, YX_STRLEN(domainname), YX_STRLEN(domainname)) == STR_EQUAL) {
            ipstr = YX_ConvertIpHexToString(ip[0]);
            AT_SOCKET_SocketConnect(COM_TCP, SOCKET_TYPE_STREAM, ipstr, s_gprspara.ippara[s_lcb.curindex].port);
        }
    } else {
        #if DEBUG_TLINK > 0
        printf_com("<域名解析失败(%d)(%d)>\r\n", result, num);
        #endif
    }
}

static void Turninto_Connecting(void)
{
    INT8U snbits;
    INT32U ip;
    
    if (!CheckGprsPara()) {                                                    /* 判断参数 */
        #if DEBUG_TLINK > 0
        printf_com("<第二服务器TCP/IP参数无效>\r\n");
        #endif
        
        OS_StartTmr(s_monitortmr, PERIOD_WAIT);
        return;
    }
  
    if (AT_SOCKET_SocketIsClosed(COM_TCP)) {                                   /* 关闭状态 */
        if (++s_lcb.ct_connect <= MAX_CONNECT) {
            #if DEBUG_TLINK > 0
            printf_com("<第二服务器TCP连接请求(%d)>\r\n", s_lcb.ct_connect);
            //printf_com("<----APN:%s>\r\n", s_gprspara.ippara[s_lcb.curindex].apn);
            printf_com("<----IP:%s,port(%d)>\r\n", s_gprspara.ippara[s_lcb.curindex].ip, s_gprspara.ippara[s_lcb.curindex].port);
            //printf_com("<----PORT:%d>\r\n", s_gprspara.ippara[s_lcb.curindex].tcp_port);
            #endif
            
            if (YX_ConvertIpStringToHex(&ip, &snbits, s_gprspara.ippara[s_lcb.curindex].ip) != 0) {
                AT_GPRS_GetIpByDomainName(PDP_COM_0, s_gprspara.ippara[s_lcb.curindex].ip, Callback_GetIpByDomainName);
            } else {
                AT_SOCKET_SocketConnect(COM_TCP, SOCKET_TYPE_STREAM, s_gprspara.ippara[s_lcb.curindex].ip, s_gprspara.ippara[s_lcb.curindex].port);
            }
            
            OS_StartTmr(s_monitortmr, PERIOD_CONNECT);
        } else {
            #if DEBUG_TLINK > 0
            printf_com("<第二服务器TCP连接请求超过次数>\r\n");
            #endif
            
            s_lcb.ct_connect = 0;
            s_lcb.curindex++;
            CheckGprsPara();
            if (!YX_JTT1_LinkCanCom()) {                                       /* 第一服务器通信也异常 */
                AT_GPRS_DeactivePDPContext(PDP_COM_0);                         /* 重新激活GPRS */
            }
            OS_StartTmr(s_monitortmr, PERIOD_CONNECT); 
        }
    } else {
        AT_SOCKET_SocketClose(COM_TCP);
        OS_StartTmr(s_monitortmr, PERIOD_CONNECT);
    }
}

/*******************************************************************
** 函数名:     Turninto_Reging
** 函数描述:   请求注册中心
** 参数:       无
** 返回:       无
********************************************************************/
static void Turninto_Reging(void)
{
    if (CheckAuthCode()) {                                                     /* 判断是否已注册 */
        Turninto_REGED();
        return;        
    }

    if (++s_lcb.ct_reg <= MAX_REG) {
        #if DEBUG_TLINK > 0
        printf_com("<第二服务器TCP注册请求(%d)>\r\n", s_lcb.ct_reg);
        #endif
        
        OS_StartTmr(s_monitortmr, PERIOD_REG);
        
        AT_SOCKET_ClearDataBySocket(COM_TCP);
        YX_JTT2_SendReqRegist();
    } else {
        #if DEBUG_TLINK > 0
        printf_com("<第二服务器TCP注册请求超过次数>\r\n");
        #endif
        
        s_lcb.ct_reg = 0;
		YX_JTT2_LinkReconnect(DELAY_REG);
    }
}

/*******************************************************************
** 函数名:     Turninto_Loging
** 函数描述:   请求登入中心
** 参数:       无
** 返回:       无
********************************************************************/
static void Turninto_Loging(void)
{
    if (++s_lcb.ct_log <= MAX_LOG) {
        #if DEBUG_TLINK > 0
        printf_com("<第二服务器TCP登入请求(%d)>\r\n", s_lcb.ct_log);
        #endif
        
        OS_StartTmr(s_monitortmr, PERIOD_LOG);
        
        AT_SOCKET_ClearDataBySocket(COM_TCP);
        YX_JTT2_SendReqLogin();                                                /* 发送登入帧 */
    } else {
        #if DEBUG_TLINK > 0
        printf_com("<第二服务器TCP登入请求超过次数>\r\n");
        #endif
        
        s_lcb.ct_log = 0;
        YX_JTT2_LinkReconnect(0);                                              /* 重新连接 */
    }
}

/*******************************************************************
** 函数名:     MonitorTmrProc
** 函数描述:   定时器处理函数
** 参数:       [in] pdata:定时器特征值
** 返回:       无
********************************************************************/
static void MonitorTmrProc(void *pdata)
{
    if ((s_lcb.status & APPLY_) == 0) {                                        /* 未激活应用 */
        return;
    }
    
    if ((s_lcb.status & RECONNECT_) != 0) {                                    /* 重新连接socket */
        s_lcb.status &= ~(RECONNECT_ | CONNECT_ | REGED_ | LOGED_);
        s_lcb.ct_connect = 0;

        AT_SOCKET_SocketClose(COM_TCP);
    } else if ((s_lcb.status & (APPLY_ | MASK_)) == (APPLY_ | ACTIVE_)) {      /* 已激活,开始连接 */
        Turninto_Connecting();
    } else if ((s_lcb.status & (APPLY_ | MASK_)) == (APPLY_ | ACTIVE_ | CONNECT_)) {/* 已连接,开始注册 */
        Turninto_Reging();
    } else if ((s_lcb.status & (APPLY_ | MASK_)) == (APPLY_ | ACTIVE_ | CONNECT_ | REGED_)) {/* 已注册,开始登入 */
        Turninto_Loging();
    } else if ((s_lcb.status & (APPLY_ | MASK_)) == (APPLY_ | ACTIVE_ | CONNECT_ | REGED_ | LOGED_)) {/* 已登入,开始链路维护 */
        SendHeartData();
    } else {
        Turninto_Activing();
    }
}

/*******************************************************************
** 函数名:     DiagnoseProc
** 函数描述:   诊断函数
** 参数:       无
** 返回:       无
********************************************************************/
static void DiagnoseProc(void)
{
    if (YX_JTT2_LinkIsOpen()) {                                                /* 处于激活状态 */
        OS_ASSERT(OS_TmrIsRun(s_monitortmr), RETURN_VOID);
    }
}

/*******************************************************************
** 函数名:     YX_JTT2_InitLink
** 函数描述:   连接模块初始化
** 参数:       无
** 返回:       无
********************************************************************/
void YX_JTT2_InitLink(void)
{
    YX_MEMSET(&s_lcb, 0, sizeof(s_lcb));
    
    YX_JTT2_InitRecv();
    YX_JTT2_InitSend();
    
    s_monitortmr = OS_CreateTmr(TSK_ID_APP, (void *)0, MonitorTmrProc);
    OS_RegistDiagnoseProc(DiagnoseProc);
}

/*******************************************************************
** 函数名:     YX_JTT2_LinkIsOpen
** 函数描述:   链路流程是否已开启
** 参数:       无
** 返回:       是返回true，否返回false
********************************************************************/
BOOLEAN YX_JTT2_LinkIsOpen(void)
{
    if ((s_lcb.status & APPLY_) != 0) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/*******************************************************************
** 函数名:     YX_JTT2_OpenLink
** 函数描述:   申请应用
** 参数:       无
** 返回:       已登入返回true，为登入返回false
********************************************************************/
BOOLEAN YX_JTT2_OpenLink(void)
{
    if ((s_lcb.status & APPLY_) == 0) {
        s_lcb.status = APPLY_;
        
        s_lcb.curindex   = 0;                                                  /* 缺省情况下采用主GPRS参数 */
        s_lcb.ct_active  = 0;
        s_lcb.ct_connect = 0;
        s_lcb.ct_reg     = 0;
        s_lcb.ct_log     = 0;
            
        OS_StartTmr(s_monitortmr, PERIOD_WAIT);
    }
    
    return YX_JTT2_LinkIsLoged();
}

/*******************************************************************
** 函数名:     YX_JTT2_CloseLink
** 函数描述:   释放应用
** 参数:       无
** 返回:       已登入返回true，为登入返回false
********************************************************************/
BOOLEAN YX_JTT2_CloseLink(void)
{
    INT8U result;
    
    result = YX_JTT2_LinkIsLoged();
    if ((s_lcb.status & APPLY_) != 0) {
        s_lcb.status = 0;
        OS_StopTmr(s_monitortmr);
    }

    return result;
}

/*******************************************************************
** 函数名:     YX_JTT2_LinkInformGprsActived
** 函数描述:   GPRS已激活/已去活回调
** 参数:       [in]  result:  true－表示激活成功，flase表示去活回调
** 返回:       无
********************************************************************/
void YX_JTT2_LinkInformGprsActived(INT8U result)
{
    #if DEBUG_TLINK > 0
    printf_com("<YX_JTT2_LinkInformGprsActived(%d)>\r\n", result);
    #endif
    
    if ((s_lcb.status & APPLY_) != 0) {
        if (result) {                                                          /* 已激活 */
            s_lcb.status = APPLY_ | ACTIVE_;
            
            s_lcb.ct_active = 0;
            OS_StartTmr(s_monitortmr, _SECOND, 3);
        } else {                                                               /* 已去活 */
            s_lcb.status = APPLY_;
            
            s_lcb.ct_connect = 0;
            s_lcb.ct_reg     = 0; 
            s_lcb.ct_log     = 0;
            
            OS_StartTmr(s_monitortmr, PERIOD_CONNECT);
        }
    }
}

/*******************************************************************
** 函数名:     YX_JTT2_LinkInformConnect
** 函数描述:   socket已连接/断开回调处理
** 参数:       [in]  result:  true－表示已连接，flase表示已断开
** 返回:       无
********************************************************************/
void YX_JTT2_LinkInformConnect(INT8U result)
{
    #if DEBUG_TLINK > 0
    printf_com("<YX_JTT2_LinkInformConnect(%d)>\r\n", result);
    #endif
    
    if ((s_lcb.status & APPLY_) != 0) {                                        /* 已激活应用 */
        if (result) {                                                          /* 已连接 */
            s_lcb.status |= CONNECT_;
            s_lcb.status &= ~(REGED_ | LOGED_);
            
            s_lcb.ct_connect = 0;
            s_lcb.ct_reg     = 0;
            s_lcb.ct_log     = 0;
        
            OS_StartTmr(s_monitortmr, _SECOND, 3);
        } else {                                                               /* 已断开 */
            s_lcb.status &= ~(CONNECT_ | REGED_ | LOGED_);
            OS_StartTmr(s_monitortmr, PERIOD_CONNECT);
        }
    }
}

/*******************************************************************
** 函数名:     YX_JTT2_LinkInformReged
** 函数描述:   注册结果通知
** 参数:       [in]  result:  0－注册成功,1-车辆已被注册,2-数据库中无车辆,3-终端已被注册,4-数据库中无终端
**             [in]  sptr:    数据指针
**             [in]  slen:    数据长度
** 返回:       无
********************************************************************/
void YX_JTT2_LinkInformReged(INT8U result, INT8U *sptr, INT8U slen)
{
    AUTH_CODE_T authinfo;
    
    #if DEBUG_TLINK > 0
    printf_com("<YX_JTT2_LinkInformReged(%d)(%d)>\r\n", result, slen);
    #endif
    
    if ((s_lcb.status & (APPLY_ | MASK_)) == (APPLY_ | ACTIVE_ | CONNECT_)) {  /* 已激活应用 */
        if (result == 0 || result == 1 || result == 3) {                       /* 注册成功 */
            s_lcb.ct_reg = 0;
            s_lcb.ct_log = 0;
            
            if (slen > 0 && slen < sizeof(authinfo.authcode)) {
                DAL_PP_ReadParaByID(PP_ID_AUTHCODE2, (INT8U *)&authinfo, sizeof(authinfo));
                YX_MEMCPY(authinfo.authcode, sizeof(authinfo.authcode), sptr, slen);
                authinfo.authcode[slen] = '\0';
                DAL_PP_StoreParaByID(PP_ID_AUTHCODE2, (INT8U *)&authinfo, sizeof(authinfo));
                
                Turninto_REGED();
            } else if (slen == 0) {                                            /* 无注册码,发送注销指令 */
                YX_JTT2_SendReqUnregist();
            }
        } else {                                                               /* 注册失败 */
            s_lcb.status &= ~(REGED_ | LOGED_);
        }
    }
}

/*******************************************************************
** 函数名:     YX_JTT2_LinkInformLoged
** 函数描述:   登入结果通知
** 参数:       [in]  result:  true－成功，flase-失败
** 返回:       无
********************************************************************/
void YX_JTT2_LinkInformLoged(INT8U result)
{
    #if DEBUG_TLINK > 0
    printf_com("<YX_JTT2_LinkInformLoged(%d)>\r\n", result);
    #endif
    
    if ((s_lcb.status & (APPLY_ | MASK_)) == (APPLY_ | ACTIVE_ | CONNECT_ | REGED_)) {/* 已激活应用 */
        if (result) {
            Turninto_LOGED();
        } else {
            if (++s_lcb.ct_logfail >= MAX_LOG) {                               /* 登入失败应答超过次数 */
                s_lcb.ct_logfail = 0;
                s_lcb.status &= ~(REGED_ | LOGED_);            
    
                DAL_PP_ClearParaByID(PP_ID_AUTHCODE2);                         /* 清除鉴权码 */
                AT_SOCKET_ClearDataBySocket(COM_TCP);
            }
        }
    }
}

/*******************************************************************
** 函数名:     YX_JTT2_LinkInformQuery
** 函数描述:   链路维护通知
** 参数:       [in] index:传递参数,根据具体参数
** 返回:       无
********************************************************************/
void YX_JTT2_LinkInformQuery(void *index)
{
    if (YX_JTT2_LinkIsLoged()) {
        #if DEBUG_TLINK > 0
        printf_com("<第二服务器TCP心跳维护(%d)>\r\n", (INT32U)index);
        #endif
        
        s_lcb.ct_heart = 0;
        OS_StartTmr(s_monitortmr, _SECOND, s_lcb.heart_period);
    }
}

/*******************************************************************
** 函数名:     YX_JTT2_LinkReconnect
** 函数描述:   链路socket断开后再重新连接
** 参数:       [in] delay: 重新连接等待时间，单位秒，0－立即断开重连
** 返回:       无
********************************************************************/
void YX_JTT2_LinkReconnect(INT32U delay)
{
    #if DEBUG_TLINK > 0
    printf_com("<YX_JTT2_LinkReconnect (%d) status:0x%x>\r\n", delay, s_lcb.status);
    #endif
    
    if ((s_lcb.status & APPLY_) == 0) {                                        /* 无用户 */
        s_lcb.status = 0;
        OS_StopTmr(s_monitortmr);
    } else {
        s_lcb.status |= RECONNECT_;

        if (delay == 0) {
            OS_StartTmr(s_monitortmr, _SECOND, 1);
        } else {
            OS_StartTmr(s_monitortmr, _SECOND, delay);                         /* 延时一段时间再断开SOCKET连接 */
        }
    }
}

/*******************************************************************
** 函数名:     YX_JTT2_LinkIsLoged
** 函数描述:   是否已登入
** 参数:       无
** 返回:       是返回true，否返回false
********************************************************************/
BOOLEAN YX_JTT2_LinkIsLoged(void)
{
    if ((s_lcb.status & LOGED_) != 0) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/*******************************************************************
** 函数名:     YX_JTT2_LinkIsReged
** 函数描述:   链路是否已注册
** 参数:       无
** 返回:       是返回true，否返回false
********************************************************************/
BOOLEAN YX_JTT2_LinkIsReged(void)
{
    if ((s_lcb.status & REGED_) != 0) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/*******************************************************************
** 函数名:     YX_JTT2_LinkIsConnect
** 函数描述:   链路socket是否已连接
** 参数:       无
** 返回:       是返回true，否返回false
********************************************************************/
BOOLEAN YX_JTT2_LinkIsConnect(void)
{
    if ((s_lcb.status & CONNECT_) != 0) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/*******************************************************************
** 函数名:     YX_JTT2_LinkCanCom
** 函数描述:   通道是否可以通信
** 参数:       无
** 返回:       是返回true，否返回false
********************************************************************/
BOOLEAN YX_JTT2_LinkCanCom(void)
{
    if (!YX_JTT2_LinkIsLoged()) {
        return FALSE;
    } else {
        return TRUE;
    }
}



