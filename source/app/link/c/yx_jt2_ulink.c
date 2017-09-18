/********************************************************************************
**
** 文件名:     yx_jt2_ulink.c
** 版权所有:   (c) 2005-2012 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现交通部协议第二服务器UDP注册、登陆管理
** 
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2013/10/21 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "yx_includes.h"
#include "yx_msgman.h"
#include "yx_timer.h"
#include "yx_diagnose.h"
#include "yx_tools.h"
#include "yx_stream.h"
#include "yx_dm.h"
#include "dal_gprs_drv.h"
#include "dal_pp_drv.h"
#include "dal_gsm_drv.h"
#include "yx_protocol_type.h"
#include "yx_jt2_ulink.h"

/*
********************************************************************************
* 定义配置参数
********************************************************************************
*/
#define COM_UDP                     CHA_JT2_ULINK          /* GPRS通信通道 */

#define MAX_GETIP                   3                      /* 最大域名解析次数 */
#define MAX_CONNECT                 3                      /* 最大连接次数 */
#define MAX_REG                     3                      /* 最大注册次数 */
#define MAX_LOG                     3                      /* 最大登入次数 */

#define ACTIVE_                     0x80                   /* 激活应用 */

#define CONNECT_                    0x01                   /* 连接成功 */
#define REGED_                      0x02                   /* 注册成功 */
#define LOGED_                      0x04                   /* 登入成功 */
#define RECONNECT_                  0x08                   /* 重新连接 */
#define MASK_                       0x7F                   /* 掩码 */

#define PERIOD_WAIT                 SECOND, 10, LOW_PRECISION_TIMER             /* 参数无效，延时尝试 */
#define PERIOD_CONNECT              SECOND, 30, LOW_PRECISION_TIMER             /* 尝试建链的周期 */
#define PERIOD_REG                  SECOND, 30, LOW_PRECISION_TIMER             /* 尝试注册的周期 */
#define PERIOD_LOG                  SECOND, 30, LOW_PRECISION_TIMER             /* 尝试登入的周期 */
#define DELAY_REG                   (10 * 60)

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    INT8U   status;
    INT8U   gprsid;
    INT8U   ct_getip;                            /* 域名解析次数 */
    INT8U   ct_connect;                          /* socket连接请求次数 */
    INT8U   ct_reg;                              /* 注册请求次数 */
    INT8U   ct_log;                              /* 登入请求次数 */
    INT8U   ct_logfail;
    INT8U   ct_heart;                            /* 心跳请求次数 */
    INT8U   ct_user;                             /* 申请用户数 */
    INT8U   curindex;                            /* 当前使用的ip参数组序号 */
    INT8U   ip[16];
    INT8U   ipflag;

    INT32U  heart_period;                        /* 心跳发送周期 */
    INT32U  heart_waittime;                      /* 心跳应答等待时间 */
    INT32U  heart_max;                           /* 心跳最大次数 */
} LCB_T;

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static INT8U s_monitortmr, s_delaytmr;
static LCB_T s_lcb;
static GPRSIP_T s_gprspara;



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

    DAL_PP_ReadParaByID(AUTHCODE2_, (INT8U *)&authinfo, sizeof(authinfo));
    len = YX_STRLEN((char *)authinfo.authcode);
    if (len >= sizeof(authinfo.authcode) || len == 0) {
        return FALSE;
    }
    
    if (YX_SearchGBCode(authinfo.authcode, len)) {
        if (len < 2) {
            return FALSE;
        }
    } else {
        ;
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
    
    if (DAL_PP_ReadParaByID(PP_ID_LINKPARA1, (INT8U *)&heart, sizeof(LINK_PARA_T))) {
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
        YX_JTU2_SendHeart();
        YX_StartTmr(s_monitortmr, SECOND, s_lcb.heart_waittime, LOW_PRECISION_TIMER);
    } else {
        #if DEBUG_ULINK > 0
        printf_com("<第二服务器心跳维护请求超过次数,周期(%d)秒,等待(%d),次数(%d)>\r\n", s_lcb.heart_period, s_lcb.heart_waittime, s_lcb.heart_max);
        #endif
        
        s_lcb.ct_heart = 0;
		YX_JTU2_LinkReactivate(0);
    }
}

/*******************************************************************
** 函数名:      Callback_GetIp
** 函数描述:    域名转IP回调
** 参数:        [in] contxtid：上下文ID
**              [in] result：   解析结果，TRUE-表示解析成功，false-表示解析失败
**              [in] num：      解析到IP个数
**              [in] ip：       解析到IP
** 返回:        无
********************************************************************/
static void Callback_GetIp(INT8U contxtid, BOOLEAN result, INT32S error, INT8U num, INT32U *ip)
{
    #if DEBUG_ULINK > 0
    printf_com("<域名转IP: contxtid(%d),result(%d),error(%d),num(%d),ip(0x%x)>\r\n", contxtid, result, error, num, ip[0]);
    #endif
    
    if (!s_lcb.ipflag) {
        if (result && num > 0) {
            s_lcb.ipflag = TRUE;
            s_lcb.ip[0] = '\0';
            YX_ConvertIpHexToString(ip[0], (char *)s_lcb.ip, sizeof(s_lcb.ip));
            YX_StartTmr(s_monitortmr, SECOND, 1, LOW_PRECISION_TIMER);
        } else {
            YX_StartTmr(s_monitortmr, PERIOD_WAIT);
        }
    }
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
    
    if (!DAL_PP_ReadParaByID(GPRSIP2_, (INT8U *)&s_gprspara, sizeof(s_gprspara))) {
        return false;
    }
    
    for (i = 0; i < MAX_IPNUM; i++) {                                          /* 寻找一个有效的ip参数 */
        s_lcb.curindex %= MAX_IPNUM;
        
        if (!s_gprspara.ippara[s_lcb.curindex].isvalid) {
            s_lcb.curindex++;
            s_lcb.curindex %= MAX_IPNUM;
            s_lcb.ipflag = FALSE;  
        } else {
            break;
        }
    }
    
    #if DEBUG_ULINK > 0
    printf_com("<第二服务器UDP/IP参数组(%d)>\r\n", s_lcb.curindex);
    #endif
    
    apn      = s_gprspara.ippara[s_lcb.curindex].apn;
    username = s_gprspara.ippara[s_lcb.curindex].username;
    password = s_gprspara.ippara[s_lcb.curindex].password;
    if (!s_gprspara.ippara[s_lcb.curindex].isvalid) {
        return false;
    }
    if ((YX_STRLEN(s_gprspara.ippara[s_lcb.curindex].udp_ip) == 0 ) || YX_STRLEN(s_gprspara.ippara[s_lcb.curindex].udp_port) == 0) {
        s_lcb.curindex++;                                                      /* 更换参数 */
        s_lcb.curindex %= MAX_IPNUM;
        s_lcb.ipflag = FALSE;
        return false;
    }

    if (DAL_GetNetworkOperator() == NETWORK_OP_CT) {                           /* 电信CDMA */
        if (YX_STRLEN(username) == 0 || YX_STRLEN(password) == 0) {
            s_lcb.curindex++;
            s_lcb.curindex %= MAX_IPNUM;
            s_lcb.ipflag = FALSE;
            return false;
        }
    } else {
        if (YX_STRLEN(apn) == 0) {
            s_lcb.curindex++;
            s_lcb.curindex %= MAX_IPNUM;
            s_lcb.ipflag = FALSE;
            return false;
        }
    }
    
    DAL_GPRS_SetPara(apn, username, password);                                 /* 设置激活参数 */
    
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
    if ((s_lcb.status & (ACTIVE_ | MASK_)) == (ACTIVE_ | CONNECT_ | REGED_)) { /* 已注册成功 */
        #if DEBUG_ULINK > 0
        printf_com("<第二服务器UDP登入成功>\r\n");
        #endif
        
        s_lcb.status |= LOGED_;
        s_lcb.ct_log = 0;
        s_lcb.ct_reg = 0;
        s_lcb.ct_connect = 0;
        
        YX_StartTmr(s_monitortmr, SECOND, 1, LOW_PRECISION_TIMER);
        YX_PostMsg(PRIO_OPTTASK, MSG_UDPLINK_LOGED, COM_UDP, 0);
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
    if ((s_lcb.status & (ACTIVE_ | MASK_)) == (ACTIVE_ | CONNECT_)) {
        #if DEBUG_ULINK > 0
        printf_com("<第二服务器UDP注册成功>\r\n");
        #endif
        
        s_lcb.status |= REGED_;
        YX_StartTmr(s_monitortmr, SECOND, 1, LOW_PRECISION_TIMER);             /* 延时几秒开始登入请求 */
        YX_PostMsg(PRIO_OPTTASK, MSG_UDPLINK_REGED, COM_UDP, 0);
    }
}

/*******************************************************************
** 函数名:     Turninto_LOGOUT
** 函数描述:   中心登出
** 参数:       无
** 返回:       无
********************************************************************/
static void Turninto_LOGOUT(void)
{
    if (YX_JTU2_LinkIsActived()) {                                             /* 应用已启动的条件下 */
        #if DEBUG_ULINK > 0
        printf_com("<第二服务器UDP登出>\r\n");
        #endif
        
        s_lcb.status = 0;
        DAL_UDP_CloseCom(COM_UDP);
        YX_StopTmr(s_monitortmr);
        YX_StopTmr(s_delaytmr);
    
        if (s_lcb.gprsid != 0xFF) {
            DAL_GPRS_Release(s_lcb.gprsid);
            s_lcb.gprsid = 0xFF;
        }
    }
}

/*******************************************************************
** 函数名:     Turninto_Connecting
** 函数描述:   进入连接状态
** 参数:       无
** 返回:       无
********************************************************************/
static void Turninto_Connecting(void)
{
    INT8U snbits;
    INT32U host_ipaddr;
    
    if (!CheckGprsPara()) {
        #if DEBUG_ULINK > 0
        printf_com("<第二服务器UDP/IP参数无效>\r\n");
        #endif
        
        YX_StartTmr(s_monitortmr, PERIOD_WAIT);
        return;
    }
    
    if (DAL_GPRS_ComIsSuspended()) {                                           /* gprs忙 */
        YX_StartTmr(s_monitortmr, PERIOD_WAIT);
        return;
    }
    
    if (!DAL_GPRS_IsActived()) {                                               /* gprs不在线 */
        YX_StartTmr(s_monitortmr, PERIOD_WAIT);
        return;
    }
    
    /* 域名解析部分 */
    if (YX_ConvertIpStringToHex(&host_ipaddr, &snbits, s_gprspara.ippara[s_lcb.curindex].udp_ip) != NULL) {/* 域名地址 */
        if (!s_lcb.ipflag) {
            if (++s_lcb.ct_getip <= MAX_GETIP) {
                PORT_GetIpByDomainName(s_gprspara.ippara[s_lcb.curindex].udp_ip, Callback_GetIp);
                YX_StartTmr(s_monitortmr, PERIOD_CONNECT);
            } else {
                s_lcb.ct_getip = 0;
                s_lcb.ipflag   = FALSE;
                DAL_GPRS_ReactivePdpContext();                                 /* 重新激活GPRS */
                YX_StartTmr(s_monitortmr, PERIOD_CONNECT); 
            }
            return;
        }
    } else {
        YX_STRCPY((char *)s_lcb.ip, s_gprspara.ippara[s_lcb.curindex].udp_ip);
        s_lcb.ipflag   = TRUE;
        s_lcb.ct_getip = 0;
    }
    
    /* socket连接 */
    if (DAL_UDP_ComIsClosed(COM_UDP)) {                                        /* 关闭状态 */
        if (++s_lcb.ct_connect <= MAX_CONNECT) {
            #if DEBUG_ULINK > 0
            printf_com("<第二服务器UDP连接请求(%d)>\r\n", s_lcb.ct_connect);
            printf_com("<----APN:%s>\r\n", s_gprspara.ippara[s_lcb.curindex].apn);
            printf_com("<----IP:%s>\r\n", s_lcb.ip);
            printf_com("<----PORT:%s>\r\n", s_gprspara.ippara[s_lcb.curindex].udp_port);
            #endif
            
            DAL_UDP_OpenCom(COM_UDP, (char *)s_lcb.ip, s_gprspara.ippara[s_lcb.curindex].udp_port);
            YX_StartTmr(s_monitortmr, PERIOD_CONNECT);
        } else {
            #if DEBUG_ULINK > 0
            printf_com("<第二服务器UDP连接请求超过次数>\r\n");
            #endif
            
            s_lcb.ipflag = FALSE;                                              /* 域名解析的IP无效 */
            s_lcb.ct_connect = 0;
            if (YX_JT_LinkCanCom()) {                                          /* 其他链路正常,需要更换参数 */
                s_lcb.curindex++;
                CheckGprsPara();
            } else {
                DAL_GPRS_ReactivePdpContext();                                 /* 重新激活GPRS */
            }
            YX_StartTmr(s_monitortmr, PERIOD_CONNECT); 
        }
    } else {
        DAL_UDP_CloseCom(COM_UDP);
        YX_StartTmr(s_monitortmr, PERIOD_CONNECT);
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
        #if DEBUG_ULINK > 0
        printf_com("<第二服务器UDP注册请求(%d)>\r\n", s_lcb.ct_reg);
        #endif
        
        YX_StartTmr(s_monitortmr, PERIOD_REG);
        
        DAL_UDP_ClearBuf(COM_UDP);
        YX_JTU2_SendReqRegist();
    } else {
        #if DEBUG_ULINK > 0
        printf_com("<第二服务器UDP注册请求超过次数>\r\n");
        #endif
        
        s_lcb.ct_reg = 0;
		YX_JTU2_LinkReactivate(DELAY_REG);
		if (YX_JT_LinkCanCom()) {                                              /* 其他链路正常,需要更换参数 */
		    s_lcb.ipflag = FALSE;                                              /* 域名解析的IP无效 */
            s_lcb.curindex++;
            CheckGprsPara();
        } else {
            DAL_GPRS_ReactivePdpContext();                                     /* 重新激活GPRS */
        }
        YX_PostMsg(PRIO_OPTTASK, MSG_UDPLINK_REGERR, COM_UDP, 0);
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
        #if DEBUG_ULINK > 0
        printf_com("<第二服务器UDP登入请求(%d)>\r\n", s_lcb.ct_log);
        #endif
        
        YX_StartTmr(s_monitortmr, PERIOD_LOG);
        
        DAL_UDP_ClearBuf(COM_UDP);
        YX_JTU2_SendReqLogin();                                                /* 发送登入帧 */
    } else {
        #if DEBUG_ULINK > 0
        printf_com("<第二服务器UDP登入请求超过次数>\r\n");
        #endif
        
        s_lcb.ct_log = 0;
        YX_JTU2_LinkReactivate(0);                                             /* 重新连接 */
        OS_PostMsg(PRIO_OPTTASK, MSG_TCPLINK_LOGERR, COM_UDP, 0);
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
    pdata = pdata;

    if (s_lcb.ct_user == 0) {                                                  /* 无用户 */
        s_lcb.status = 0;
        YX_StopTmr(s_monitortmr);
        return;
    }
    
    if ((s_lcb.status & ACTIVE_) == 0) {                                       /* 未激活应用 */
        return;
    }
    
    if (s_lcb.status & RECONNECT_) {                                           /* 重新连接socket */
        s_lcb.status     = ACTIVE_;
        s_lcb.ct_connect = 0;
        s_lcb.ipflag     = FALSE;
        DAL_UDP_CloseCom(COM_UDP);
    } else if ((s_lcb.status & (ACTIVE_ | MASK_)) == (ACTIVE_ | CONNECT_)) {
        s_lcb.status = ACTIVE_ | CONNECT_;
        Turninto_Reging();        
    } else if ((s_lcb.status & (ACTIVE_ | MASK_)) == (ACTIVE_ | CONNECT_ | REGED_ | LOGED_)) {
        //s_lcb.status = ACTIVE_ | CONNECT_ | REGED_;
        //Turninto_Loging();
        
        SendHeartData();
    } else if ((s_lcb.status & (ACTIVE_ | MASK_)) == (ACTIVE_ | CONNECT_ | REGED_)) {
        Turninto_Loging();
    } else {
        Turninto_Connecting();
    }
}

/*******************************************************************
** 函数名:     DelayTmrProc
** 函数描述:   延时释放连接定时器处理函数
** 参数:       [in] pdata:定时器特征值
** 返回:       无
********************************************************************/
static void DelayTmrProc(void *pdata)
{
    pdata = pdata;

    YX_StopTmr(s_delaytmr);

    if (s_lcb.ct_user == 0) {                                                  /* 无用户 */
        Turninto_LOGOUT();
    } else {
        if (!YX_JTU2_LinkIsLoged()) {
            YX_StartTmr(s_monitortmr, PERIOD_WAIT);
        } else {
            YX_StartTmr(s_monitortmr, SECOND, s_lcb.heart_period, LOW_PRECISION_TIMER);
        }
    }
}

/*******************************************************************
** 函数名:     InformDeviceInfoChange
** 函数描述:   车台注册信息，请求重新注册通知函数
** 参数:       [in]  mode
** 返回:       无
********************************************************************/
static void InformDeviceInfoChange(INT8U mode)
{
    DAL_PP_ClearParaByID(AUTHCODE2_);                                          /* 清除鉴权码 */
    if ((s_lcb.status & (ACTIVE_ | CONNECT_)) ==  (ACTIVE_ | CONNECT_) ) {
        s_lcb.status &= ~(REGED_ | LOGED_);
        s_lcb.ct_reg = 0;
        Turninto_Reging();
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
    if (s_lcb.ct_user > 0) {                                                   /* 有用户，必须处于激活 */
        OS_ASSERT(YX_JTU2_LinkIsActived(), RETURN_VOID);
    }
    
    if (YX_JTU2_LinkIsActived()) {                                             /* 处于激活状态 */
        OS_ASSERT((YX_TmrIsRun(s_monitortmr) || YX_TmrIsRun(s_delaytmr)), RETURN_VOID);
    }
}

/*******************************************************************
** 函数名:     YX_JTU2_InitLink
** 函数描述:   连接模块初始化
** 参数:       无
** 返回:       无
********************************************************************/
void YX_JTU2_InitLink(void)
{
    YX_MEMSET(&s_lcb, 0, sizeof(s_lcb));
    s_lcb.gprsid = 0xff;
    
    s_monitortmr = YX_InstallTmr(PRIO_OPTTASK, (void *)0, MonitorTmrProc);
    s_delaytmr   = YX_InstallTmr(PRIO_OPTTASK, (void *)0, DelayTmrProc);
    
    DAL_PP_RegParaChangeInformer(PP_ID_DEVICE, InformDeviceInfoChange);
    OS_InstallDiag(DiagnoseProc);
}

/*******************************************************************
** 函数名:     YX_JTU2_GetLinkParaIndex
** 函数描述:   获取UDP链路ip参数索引号
** 参数:       无
** 返回:       返回当前参数索引号
********************************************************************/
INT8U YX_JTU2_GetLinkParaIndex(void)
{
    return (s_lcb.curindex % MAX_IPNUM);
}

/*******************************************************************
** 函数名:     YX_JTU2_LinkInformGprsActived
** 函数描述:   GPRS已激活/已去活回调
** 参数:       [in]  result:  true－表示激活成功，flase表示去活回调
** 返回:       无
********************************************************************/
void YX_JTU2_LinkInformGprsActived(INT8U result)
{
    #if DEBUG_ULINK > 0
    printf_com("<YX_JTU2_LinkInformGprsActived(%d)>\r\n", result);
    #endif
    
    if ((s_lcb.status & ACTIVE_) != 0) {
        if (result) {                                                          /* 已激活 */
            YX_StartTmr(s_monitortmr, PERIOD_WAIT);
        } else {                                                               /* 已去活 */
            s_lcb.status = ACTIVE_;
            s_lcb.ct_log = 0;       
            s_lcb.ct_reg = 0;     
            s_lcb.ct_connect = 0;
            s_lcb.ipflag = FALSE;
            s_lcb.curindex++;                                                  /* 更换参数 */
            CheckGprsPara();
            
            YX_StartTmr(s_monitortmr, PERIOD_CONNECT);
        }
    }
}

/*******************************************************************
** 函数名:     YX_JTU2_LinkInformConnect
** 函数描述:   socket已连接/断开回调处理
** 参数:       [in]  result:  true－表示已连接，flase表示已断开
** 返回:       无
********************************************************************/
void YX_JTU2_LinkInformConnect(INT8U result)
{
    if ((s_lcb.status & ACTIVE_) != 0) {                                       /* 已激活应用 */
        if (result) {                                                          /* 已连接 */
            s_lcb.status |= CONNECT_;
            s_lcb.status &= ~REGED_;
            s_lcb.ct_connect = 0; 
            s_lcb.ct_log = 0;
            s_lcb.ct_reg = 0;
        
            YX_StartTmr(s_monitortmr, SECOND, 3, LOW_PRECISION_TIMER);
        } else {                                                               /* 已断开 */
            s_lcb.status = ACTIVE_;
            YX_StartTmr(s_monitortmr, PERIOD_CONNECT);
        }
    }
}

/*******************************************************************
** 函数名:     YX_JTU2_LinkInformReged
** 函数描述:   注册结果通知
** 参数:       [in]  result:  0－注册成功,1-车辆已被注册,2-数据库中无车辆,3-终端已被注册,4-数据库中无终端
**             [in]  sptr:    数据指针
**             [in]  slen:    数据长度
** 返回:       无
********************************************************************/
void YX_JTU2_LinkInformReged(INT8U result, INT8U *sptr, INT8U slen)
{
    AUTH_CODE_T authinfo;
    
    if ((s_lcb.status & (ACTIVE_ | MASK_)) == (ACTIVE_ | CONNECT_)) {          /* 已激活应用 */
        if (result == 0) {                                                     /* 注册成功 */
            s_lcb.ct_connect = 0;
            s_lcb.ct_reg = 0;
            
            if (slen > 0 && slen < sizeof(authinfo.authcode)) {
                DAL_PP_ReadParaByID(AUTHCODE2_, (INT8U *)&authinfo, sizeof(authinfo));
                YX_MEMCPY(authinfo.authcode, sizeof(authinfo.authcode), sptr, slen);
                authinfo.authcode[slen] = '\0';
                DAL_PP_StoreParaByID(AUTHCODE2_, (INT8U *)&authinfo, sizeof(authinfo));
                
                Turninto_REGED();
            }
        } else if (result == 1 || result == 3) {                               /* 已注册 */
            if (slen > 0 && slen < sizeof(authinfo.authcode)) {
                DAL_PP_ReadParaByID(AUTHCODE2_, (INT8U *)&authinfo, sizeof(authinfo));
                YX_MEMCPY(authinfo.authcode, sizeof(authinfo.authcode), sptr, slen);
                authinfo.authcode[slen] = '\0';
                DAL_PP_StoreParaByID(AUTHCODE2_, (INT8U *)&authinfo, sizeof(authinfo));
                
                Turninto_REGED();
            } else {
                YX_JTU2_SendReqUnregist();
            }
        } else {                                                               /* 注册失败 */
            ;
        }
    }
}

/*******************************************************************
** 函数名:     YX_JTU2_LinkInformLoged
** 函数描述:   登入结果通知
** 参数:       [in]  result:  true－成功，flase-失败
** 返回:       无
********************************************************************/
void YX_JTU2_LinkInformLoged(INT8U result)
{
    if ((s_lcb.status & (ACTIVE_ | MASK_)) == (ACTIVE_ | CONNECT_ | REGED_)) { /* 已激活应用 */
        if (result) {
            s_lcb.ct_connect = 0;
            s_lcb.ct_reg = 0;
            s_lcb.ct_log = 0; 
            s_lcb.ct_logfail = 0;

            Turninto_LOGED();
        } else {
            if (++s_lcb.ct_logfail >= MAX_LOG) {                               /* 登入失败应答超过次数 */
                s_lcb.ct_logfail = 0;
                s_lcb.status &= ~(REGED_ | LOGED_);            
    
                DAL_PP_ClearParaByID(AUTHCODE2_);                               /* 清除鉴权码 */
                DAL_UDP_ClearBuf(COM_UDP);
                
                YX_PostMsg(PRIO_OPTTASK, MSG_UDPLINK_LOGERR, COM_UDP, 0);    
            }
        }
    }
}

/*******************************************************************
** 函数名:     YX_JTU2_LinkInformQuery
** 函数描述:   链路维护通知
** 参数:       [in] index:传递参数,根据具体参数
** 返回:       无
********************************************************************/
void YX_JTU2_LinkInformQuery(void *index)
{
    INT32U result;
    
    result = (INT32U)index;
    if (YX_JTU2_LinkIsLoged()) {
        #if DEBUG_ULINK > 0
        printf_com("<第二服务器UDP心跳维护(%d)>\r\n", index);
        #endif
        
        YX_StartTmr(s_monitortmr, SECOND, s_lcb.heart_period, LOW_PRECISION_TIMER);
    }
}

/*******************************************************************
** 函数名:     YX_JTU2_LinkIsLoged
** 函数描述:   是否已登入
** 参数:       无
** 返回:       是返回true，否返回false
********************************************************************/
BOOLEAN YX_JTU2_LinkIsLoged(void)
{
    if ((s_lcb.status & (ACTIVE_ | CONNECT_ | REGED_ | LOGED_)) == (ACTIVE_ | CONNECT_ | REGED_ | LOGED_)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/*******************************************************************
** 函数名:     YX_JTU2_LinkIsReged
** 函数描述:   链路是否已注册
** 参数:       无
** 返回:       是返回true，否返回false
********************************************************************/
BOOLEAN YX_JTU2_LinkIsReged(void)
{
    if ((s_lcb.status & (ACTIVE_ | CONNECT_ | REGED_)) == (ACTIVE_ | CONNECT_ | REGED_)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/*******************************************************************
** 函数名:     YX_JTU2_LinkIsConnect
** 函数描述:   链路socket是否已连接
** 参数:       无
** 返回:       是返回true，否返回false
********************************************************************/
BOOLEAN YX_JTU2_LinkIsConnect(void)
{
    if ((s_lcb.status & (ACTIVE_ | CONNECT_)) == (ACTIVE_ | CONNECT_)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/*******************************************************************
** 函数名:     YX_JTU2_LinkIsActived
** 函数描述:   链路流程是否已启动
** 参数:       无
** 返回:       是返回true，否返回false
********************************************************************/
BOOLEAN YX_JTU2_LinkIsActived(void)
{
    if ((s_lcb.status & ACTIVE_) != 0) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/*******************************************************************
** 函数名:     YX_JTU2_LinkCanCom
** 函数描述:   通道是否可以通信
** 参数:       无
** 返回:       是返回true，否返回false
********************************************************************/
BOOLEAN YX_JTU2_LinkCanCom(void)
{
    if (DAL_GPRS_ComIsSuspended() || !YX_JTU2_LinkIsLoged()) {
        return FALSE;
    } else {
        return TRUE;
    }
}

/*******************************************************************
** 函数名:     YX_JTU2_LinkApply
** 函数描述:   申请链路应用
** 参数:       无
** 返回:       已登入返回true，未登入返回false
********************************************************************/
BOOLEAN YX_JTU2_LinkApply(void)
{
    #if DEBUG_ULINK > 0
    printf_com("<YX_JTU2_LinkApply(%d)>\r\n", s_lcb.ct_user + 1);
    #endif

    s_lcb.ct_user++;
    if ((s_lcb.status & ACTIVE_) == 0) {
        s_lcb.status = ACTIVE_;
        s_lcb.curindex = 0;                                                    /* 缺省情况下采用主GPRS参数 */
        s_lcb.ct_log = 0;
        s_lcb.ct_reg = 0;
        s_lcb.ct_connect = 0;
        s_lcb.ct_logfail = 0;
        
        if (s_lcb.gprsid == 0xff) {                                            /* 未激活GPRS应用 */
            CheckGprsPara();
            DAL_GPRS_Apply(&s_lcb.gprsid);
        }
            
        YX_StartTmr(s_monitortmr, PERIOD_WAIT);
        YX_StopTmr(s_delaytmr);                                                /* 停止延时释放连接 */
    }
    
    return YX_JTU2_LinkIsLoged();
}

/*******************************************************************
** 函数名:     YX_JTU2_LinkRelease
** 函数描述:   释放链路应用
** 参数:       [in] delay:延时释放时间，单位秒，0－立即释放
** 返回:       已登入返回true，为登入返回false
********************************************************************/
BOOLEAN YX_JTU2_LinkRelease(INT32U delay)
{
    #if DEBUG_ULINK > 0
    printf_com("<YX_JTU2_LinkRelease,delay(%d),user(%d)>\r\n", delay, s_lcb.ct_user - 1);
    #endif
    
    if (s_lcb.ct_user > 0) {
        if (--s_lcb.ct_user == 0) {
            if (delay == 0) {
                YX_StartTmr(s_delaytmr, SECOND, 1, LOW_PRECISION_TIMER);
            } else {
                YX_StartTmr(s_delaytmr, SECOND, delay, LOW_PRECISION_TIMER);   /* 延时一段时间再断开GPRS连接 */
            }
            YX_StopTmr(s_monitortmr);
        }
    }

    return YX_JTU2_LinkIsLoged();
}

/*******************************************************************
** 函数名:     YX_JTU2_LinkReactivate
** 函数描述:   重新激活连接，断开socket后，重新连接socket
** 参数:       [in] delay:延时重新激活连接的时间，单位秒，0－立即重新激活链接
** 返回:       无
********************************************************************/
void YX_JTU2_LinkReactivate(INT32U delay)
{
    #if DEBUG_ULINK > 0
    printf_com("<YX_JTU2_LinkReactivate,delay(%d),user(%d)>\r\n", delay, s_lcb.ct_user);
    #endif
    
    if (s_lcb.ct_user == 0) {                                                  /* 无用户 */
        if (!YX_TmrIsRun(s_delaytmr)) {
            s_lcb.status = 0;
            YX_StopTmr(s_monitortmr);
        }
    } else {
        if (YX_JTU2_LinkIsConnect()) {                                         /* 已连接 */
            s_lcb.status |= RECONNECT_;
                
            if (delay == 0) {
                YX_StartTmr(s_monitortmr, SECOND, 1, LOW_PRECISION_TIMER);
            } else {
                YX_StartTmr(s_monitortmr, SECOND, delay, LOW_PRECISION_TIMER); /* 延时一段时间再断开GPRS连接 */
            }
        }
    }
}


