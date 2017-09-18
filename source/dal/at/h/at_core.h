/********************************************************************************
**
** 文件名:     at_core.h
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现手机模块初始化配置
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2014/06/15 | 叶德焰 |  创建第一版本
*********************************************************************************/
#ifndef AT_CORE_H
#define AT_CORE_H            1


/* 网络状态 */
typedef enum {
    NETWORK_STATE_NOT_REGISTERED=0,  // Not register to network
    NETWORK_STATE_REGISTERED,        /* 已注册 */
    NETWORK_STATE_HOME,              /* 本地网络 */
    NETWORK_STATE_ROAMING,           /* 漫游 */
    NETWORK_STATE_SEARCHING,         /* 搜索网络中 */
    NETWORK_STATE_REG_DENIED,        /* The request to register to network is denied */
    NETWORK_STATE_UNKNOWN,           /* unknow */
    NETWORK_STATE_NOT_ACTIVE,        /* The network is inactive */
    NETWORK_STATE_MAX
} NETWORK_STATE_E;

/* sim卡状态 */
typedef enum {
    SIM_NORMAL = 1,              // The normally working state
    PH_SIM_PIN_REQUIRED,
    PH_FSIM_PIN_REQUIRED,
    PH_FSIM_PUK_REQUIRED,
    PH_SIM_PUK_REQUIRED,
    SIM_NOT_INSERTED,
    SIM_PIN_REQUIRED,
    SIM_PUK_REQUIRED,
    SIM_FAILURE,
    SIM_BUSY,
    SIM_WRONG,
    SIM_PIN2_REQUIRED,
    SIM_PUK2_REQUIRED,
    RMMI_ERR_UNSPECIFIED,  //unspecified parsing error
    RMMI_CME_SIM_POWERED_DOWN,
    SIM_STATE_MAX
} SIM_STATE_E;

/* 电信运营商 */
typedef enum {
    NETWORK_OP_CMCC,         /* 移动 */
    NETWORK_OP_UNICOM,       /* 联通 */
    NETWORK_OP_CT,           /* 电信 */
    NETWORK_OP_MAX = 0xFF
} NETWORK_OP_E;

/* 模块型号定义 */
typedef enum {
    MODULE_TYPE_SIM6320,
    MODULE_TYPE_SIM5360,
    MODULE_TYPE_SIM7100,
    MODULE_TYPE_M12,
    MODULE_TYPE_MAX
} MODULE_TYPE_E;

/* 网络模式 */
typedef enum {
    NETWORK_MODE_FLIGHT,     /* 飞行模式 */
    NETWORK_MODE_WORK,       /* 工作模式 */
    NETWORK_MODE_MAX = 0xFF
} NETWORK_MODE_E;

/*
********************************************************************************
* define network registration status
********************************************************************************
*/

typedef struct {
    INT8U len;
    INT8U imsi[20];
} IMSI_T;

typedef struct {
    INT8U len;
    INT8U imei[20];
} IMEI_T;

typedef struct {
    INT8U len;
    INT8U iccid[30];
} ICCID_T;

typedef struct {
    INT8U   len;
    INT8U   buf[80];
} GSM_VER_T;

/*******************************************************************
** 函数名:     AT_CORE_Init
** 函数描述:   初始化AT CORE
** 参数:       无
** 返回:       无
********************************************************************/
void AT_CORE_Init(void);

/*******************************************************************
** 函数名:     AT_CORE_Open
** 函数描述:   打开模块初始化功能
** 参数:       无
** 返回:       无
********************************************************************/
void AT_CORE_Open(void);

/*******************************************************************
** 函数名:     AT_CORE_Close
** 函数描述:   关闭模块初始化功能
** 参数:       无
** 返回:       无
********************************************************************/
void AT_CORE_Close(void);

/*******************************************************************
** 函数名:     AT_CORE_HaveHighTask
** 函数描述:   是否AT需要发送
** 参数:       无
** 返回:       有返回true，否则返回false
********************************************************************/
BOOLEAN AT_CORE_HaveHighTask(void);

/*******************************************************************
** 函数名:     AT_CORE_HaveLowTask
** 函数描述:   是否AT需要发送
** 参数:       无
** 返回:       有返回true，否则返回false
********************************************************************/
BOOLEAN AT_CORE_HaveLowTask(void);

/*******************************************************************
** 函数名:     AT_CORE_SendEntry
** 函数描述:   AT发送入口
** 参数:       无
** 返回:       无
********************************************************************/
void AT_CORE_SendEntry(void);

void    NotifySendSMSuccess(void);
void    NotifySendSMFailure(void);
void    NotifyVoiceConnect(void);
void    NotifyVoiceDisconnect(void);
void    NotifyDataConnect(void);
void    NotifyDataDisconnect(void);
void    NotifyRecvSM(void);
void    NotifyUnReadSM(INT8U index);
void    AT_CORE_RecoveryATCmd(void);
void    AT_CORE_EscapeATCmd(void);

/*******************************************************************
** 函数名:     ADP_NET_GetIMEI
** 函数描述:   获取IMEI号
** 参数:       [in] ptr：   缓存指针
**             [in] maxlen：缓存最大长度
** 返回:       返回IMEI长度，失败返回0
********************************************************************/
INT8U ADP_NET_GetIMEI(INT8U *ptr, INT16U maxlen);

/*******************************************************************
** 函数名:     ADP_NET_GetIMSI
** 函数描述:   获取IMSI号
** 参数:       [in] ptr：   缓存指针
**             [in] maxlen：缓存最大长度
** 返回:       返回IMSI长度，失败返回0
********************************************************************/
INT8U ADP_NET_GetIMSI(INT8U *ptr, INT16U maxlen);

/*******************************************************************
** 函数名:     ADP_NET_SetNetworkMode
** 函数描述:   设置网络开关功能
** 参数:       [in] mode:网络模式,见NETWORK_MODE_E
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN ADP_NET_SetNetworkMode(INT8U mode);

/*******************************************************************
** 函数名:     ADP_NET_GetNetworkState
** 函数描述:   获取网络状态
** 参数:       [out] simcard: sim卡状态，见SIM_STATE_E
**             [out] creg:    网络注册状态，见NETWORK_STATE_E
**             [out] cgreg：  网络注册状态，见NETWORK_STATE_E
**             [out] rssi：   接收的信号强度指示，单位：dBm.
**             [out] ber：    位错误率Bit error rate，单位：dBm.
** 返回:       0表示操作成功；
**            -1其他错误
********************************************************************/
BOOLEAN ADP_NET_GetNetworkState(INT8U *simcard, INT8U *creg, INT8U *cgreg, INT8U *rssi, INT8U *ber);

/*******************************************************************
** 函数名:     ADP_NET_GetOperator
** 函数描述:   获取网络运营商
** 参数:       无
** 返回:       运营商id,见NETWORK_OP_E
********************************************************************/
INT8U ADP_NET_GetOperator(void);

/*******************************************************************
** 函数名:     ADP_NET_GetModuleType
** 函数描述:   获取模块型号
** 参数:       无
** 返回:       返回模块型号,见 MODULE_TYPE_E
********************************************************************/
INT8U ADP_NET_GetModuleType(void);

/*******************************************************************
** 函数名:     ADP_NET_SetSimPinFunction
** 函数描述:   设置pin码功能
** 参数:       [in] onoff:   true-打开pin码功能，false-关掉pin码功能
**             [in] password:PIN密码，以'\0'为结束符,密码为4位
**             [in] fp:      设置结果回调，result：true-设置成功，false-设置失败
** 返回:       调用成功返回true，失败返回false，调用失败时无fp回调
********************************************************************/
BOOLEAN ADP_NET_SetSimPinFuction(INT8U onoff, char *password, void (*fp)(INT8U result));

/*******************************************************************
** 函数名:     ADP_NET_ModifyPinPassword
** 函数描述:   修改pin密码，必须在已开启pin码功能的情况下
** 参数:       [in] opassword:旧PIN密码，以'\0'为结束符,密码为4位
**             [in] npassword:新PIN密码，以'\0'为结束符,密码为4位
**             [in] fp:      设置结果回调，result：true-设置成功，false-设置失败
** 返回:       调用成功返回true，失败返回false，调用失败时无fp回调
********************************************************************/
BOOLEAN ADP_NET_ModifyPinPassword(char *opassword, char *npassword, void (*fp)(INT8U result));

/*******************************************************************
** 函数名:     ADP_NET_UnlockSimPinCode
** 函数描述:   解SIM卡pin码
** 参数:       [in] password:PIN密码，以'\0'为结束符,密码为4位
**             [in] fp:      解码结果回调，result：true-解码成功，false-解码失败
** 返回:       调用成功返回true，失败返回false，调用失败时无fp回调
********************************************************************/
BOOLEAN ADP_NET_UnlockSimPinCode(char *password, void (*fp)(INT8U result));

/*******************************************************************
** 函数名:     ADP_NET_SimPinFuctionIsOn
** 函数描述:   pin码功能是否已开启
** 参数:       无
** 返回:       开启返回true，未开启返回false
********************************************************************/
BOOLEAN ADP_NET_SimPinFuctionIsOn(void);

/*******************************************************************
** 函数名:     ADP_NET_SimPinCodeIsLock
** 函数描述:   pin码是否未解锁
** 参数:       无
** 返回:       是返回true，否返回false
********************************************************************/
BOOLEAN ADP_NET_SimPinCodeIsLock(void);

/*******************************************************************
** 函数名:     ADP_NET_InformSimPinCodeStatus
** 函数描述:   通知当前PIN码状态
** 参数:       [in] lock:   true-pin码已锁，false-无pin码功能
** 返回:       是返回true，否返回false
********************************************************************/
BOOLEAN ADP_NET_InformSimPinCodeStatus(INT8U lock);


#endif
