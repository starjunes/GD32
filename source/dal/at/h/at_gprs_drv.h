/********************************************************************************
**
** 文件名:     adp_gprs_drv.h
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现GPRS驱动管理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2011/01/15 | 叶德焰 |  创建第一版本
*********************************************************************************/
#ifndef AT_GPRS_DRV_H
#define AT_GPRS_DRV_H     1


#if EN_AT > 0

/* GPRS上下文状态 */
typedef enum {
    PDP_STATE_FREE = 0,
    PDP_STATE_ACTIVING,
    PDP_STATE_ACTIVED,
    PDP_STATE_DEACTIVING,
    PDP_STATE_MAX
} PDP_STATE_E;

/* GPRS上下文状态回调 */
typedef struct {
    void(*callback_network_actived)(INT8U contexid);
    void(*callback_network_deactived)(INT8U contexid);
} GPRS_CALLBACK_T;




/*******************************************************************
** 函数名:     AT_GPRS_InitDrv
** 函数描述:   初始化GPRS网络，初始化GPRS上下文
** 参数:       无
** 返回:       无
********************************************************************/
void AT_GPRS_InitDrv(void);

/*******************************************************************
** 函数名:     AT_GPRS_RegistHandler
** 函数描述:   注册GPRS消息处理器
** 参数:       [in]  callback: 注册回调函数参数
** 返回:       成功返回TRUE, 失败返回FALSE
********************************************************************/
BOOLEAN AT_GPRS_RegistHandler(GPRS_CALLBACK_T *callback);

/*******************************************************************
** 函数名:     AT_GPRS_GetPdpStatus
** 函数描述:   GPRS上下文状态
** 参数:       无
** 返回:       返回上下文状态
********************************************************************/
INT8U AT_GPRS_GetPdpStatus(void);

/*******************************************************************
** 函数名:     AT_GPRS_ClosePdpContext
** 函数描述:   关闭上下文
** 参数:       无
** 返回:       无
********************************************************************/
void AT_GPRS_ClosePdpContext(void);

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
INT32S AT_GPRS_SetApn(INT8U profileid, char *apn, char *userid, char *password, void (*callback)(INT8U result, INT32S error_code));

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
INT32S AT_GPRS_GetApn(INT8U profileid, void (*callback)(INT8U id, INT8U result, INT32S error_code, INT8U *apn, INT8U *userid, INT8U *password));

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
INT32S AT_GPRS_ActivePDPContext(INT8U contxtid);

/*******************************************************************
** 函数名:     AT_GPRS_DeactivePDPContext
** 函数描述:   去活GPRS上下文
** 参数:       [in]  contxtid: GPRS上下文通道编号0－1, 见PDP_COM_E
** 返回:       0表示操作成功；
**            -1表示阻塞中，等待callback回调；
**            -2表示参数有误；
**            -4其他错误
********************************************************************/
INT32S AT_GPRS_DeactivePDPContext(INT8U contxtid);

/*******************************************************************
** 函数名:     AT_GPRS_GetLocalIpAddr
** 函数描述:   获得本地IP地址
** 参数:       [in]  contxtid: GPRS上下文通道编号0－1
**             [out] ipaddr:   ip地址,依次从从高位到低位
** 返回:       成功返回0，失败返回-1
********************************************************************/
INT32S AT_GPRS_GetLocalIpAddr(INT8U contxtid, INT32U *ipaddr);

/*******************************************************************
** 函数名:     AT_GPRS_GetIpByDomainName
** 函数描述:   根据域名获取IP地址
** 参数:       [in]  contxtid:   GPRS上下文通道编号0－1
**             [in]  domainname: 域名，如“www.yaxon.com”
**             [in]  fp:         异步回调函数
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN AT_GPRS_GetIpByDomainName(INT8U contxtid, char *domainname, void (*fp)(INT8U contxtid, BOOLEAN result, INT32S error, char *domainname, INT8U num, INT32U *ip));

/*******************************************************************
** 函数名:     AT_GPRS_SetDnsAddress
** 函数描述:   设置域名服务器地址
** 参数:       [in]  contxtid:   GPRS上下文通道编号0－1
**             [in]  num:        ip个数,一般为2个
**             [in]  ip:         ip地址,依次从从高位到低位
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN AT_GPRS_SetDnsAddress(INT8U contxtid, INT8U num, INT32U *ip);

/*******************************************************************
** 函数名:     AT_GPRS_GetDnsAddress
** 函数描述:   获取域名服务器地址
** 参数:       [in]  contxtid:   GPRS上下文通道编号0－1
**             [in]  i_num:      ip缓存个数
**             [out] ip:         ip缓存,依次从从高位到低位
**             [out] o_num:      返回ip个数
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN AT_GPRS_GetDnsAddress(INT8U contxtid, INT8U i_num, INT32U *ip, INT8U *o_num);

/*******************************************************************
** 函数名:     AT_GPRS_InformPDPDeactive
** 函数描述:   通知上下文已去活
** 参数:       [in]  contxtid: GPRS上下文通道编号0－1
** 返回:       成功返回0，失败返回-1
********************************************************************/
void AT_GPRS_InformPDPDeactive(INT8U contxtid);

/*******************************************************************
** 函数名:     AT_GPRS_InformGetIpByDomainName
** 函数描述:   根据域名获取IP地址结果通知
** 参数:       [in]  result: true-解析成功，false-解析失败
**             [in]  domainname: 域名
**             [in]  num:    IP个数
**             [in]  ip:     IP数组
** 返回:       成功返回0，失败返回-1
********************************************************************/
void AT_GPRS_InformGetIpByDomainName(INT8U result, char *domainname, INT8U num, INT32U *ip);

#endif
#endif
