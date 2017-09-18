/********************************************************************************
**
** 文件名:     at_urc_tcpip.h
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
#ifndef AT_URC_TCPIP_H
#define AT_URC_TCPIP_H       1


#if EN_AT > 0


/* 上下文参数ID */
typedef enum {
    PROFILE_ID_0,
    PROFILE_ID_1,
    PROFILE_ID_2,
    PROFILE_ID_3,
    PROFILE_ID_4,
    PROFILE_ID_5,
    PROFILE_ID_6,
    PROFILE_ID_7,
    PROFILE_ID_8,
    PROFILE_ID_9,
    PROFILE_ID_MAX
} PROFILE_ID_E;

/* 上下文通道编号 */
typedef enum {
    PDP_COM_0,               /* 通道0 */
    PDP_COM_1,               /* 通道1 */
    PDP_COM_MAX
} PDP_COM_E;

/* socket通道定义 */
typedef enum {
    SOCKET_CH_0 = 0,
    SOCKET_CH_1,
    SOCKET_CH_MAX
} SOCKET_CH_E;

/* GPRS消息回调通知 */
typedef struct {
    void (*callback_network_actived)(INT8U contexid);
    void (*callback_network_deactived)(INT8U contexid, INT32S error_cause, INT32S error);
    void (*callback_getipbyname)(INT8U result, char *domainname, INT8U num, INT32U *ip);
} AT_GPRS_CALLBACK_T;

/* SOCKET消息回调通知 */
typedef struct {
    void (*callback_socket_connect)(INT8U contexid, INT8S sock, INT8U result, INT32S error_code);
    void (*callback_socket_close)(INT8U contexid, INT8S sock, INT8U result, INT32S error_code);
    void (*callback_socket_recv)(INT8U contexid, INT8S sock, INT8U *sptr, INT32U slen);
    void (*callback_socket_send)(INT8U contexid, INT8S sock, INT8U result, INT32S error_code, INT16U len);
} SOCKET_CALLBACK_T;



/*******************************************************************
** 函数名:     AT_URC_InitTcpip
** 函数描述:   初始化函数
** 参数:       无
** 返回:       无
********************************************************************/
void AT_URC_InitTcpip(void);

/*******************************************************************
** 函数名:     AT_URC_RegistGprsHandler
** 函数描述:   注册GPRS消息接收处理器
** 参数:       [in] callback:   消息处理器
** 返回:       注册成功返回true，注册失败返回false
********************************************************************/
BOOLEAN AT_URC_RegistGprsHandler(AT_GPRS_CALLBACK_T *callback);

/*******************************************************************
** 函数名:     AT_URC_RegistSocketHandler
** 函数描述:   注册SOCKET消息接收处理器
** 参数:       [in] callback:   消息处理器
** 返回:       注册成功返回true，注册失败返回false
********************************************************************/
BOOLEAN AT_URC_RegistSocketHandler(SOCKET_CALLBACK_T *callback);


#endif
#endif

