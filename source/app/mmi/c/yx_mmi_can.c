/********************************************************************************
**
** 文件名:     yx_mmi_can.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现CAN通信管理功能
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
#include "st_rtc_drv.h"
#include "hal_can_drv.h"
#include "dal_gpio_cfg.h"
#include "dal_pulse_drv.h"
#include "dal_pp_drv.h"
#include "dal_ic_drv.h"
#include "yx_debug.h"

#if EN_MMI > 0
#include "yx_mmi_drv.h"

/*
********************************************************************************
*宏定义
********************************************************************************
*/
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
#if EN_CAN > 0
static INT8U s_count;
static CAN_STATUS_T s_canstatus[CAN_COM_MAX];

/*******************************************************************
** 函数名:     SendAck
** 函数描述:   发送应答
** 参数:       [in]cmd:    命令编码
**             [in]type:   应答类型
** 返回:       无
********************************************************************/
static void SendAck(INT8U cmd, INT8U com, INT8U type)
{
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteBYTE_Strm(wstrm, com);
    YX_WriteBYTE_Strm(wstrm, type);
    YX_MMI_ListSend(cmd, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 0, 0, 0);
}
#endif

/*******************************************************************
** 函数名:     SetMyTelPara
** 函数描述:   设置本机号码
** 参数:       [in] sptr:  数据指针
**             [in] slen:  数据长度
** 返回:       成功返回TRUE；失败返回FALSE
********************************************************************/
static BOOLEAN SetMyTelPara(INT8U *sptr, INT32U slen)
{
    TEL_T mytel;
    
    DAL_PP_ReadParaByID(PP_ID_MYTEL, (INT8U *)&mytel, sizeof(mytel));
    if (slen > sizeof(mytel.tel)) {
        return false;
    }
    
    mytel.tellen = slen;
    YX_MEMCPY(mytel.tel, sizeof(mytel.tel), sptr, slen);
    
    if (YX_MEMCMP(DAL_PP_GetParaPtr(PP_ID_MYTEL), (INT8U *)&mytel, sizeof(mytel)) != 0) {
        DAL_PP_StoreParaByID(PP_ID_MYTEL, (INT8U *)&mytel, sizeof(mytel));
    }
    return true;
}

/*******************************************************************
** 函数名:     SetAlarmTelPara
** 函数描述:   设置报警号码
** 参数:       [in] sptr:  数据指针
**             [in] slen:  数据长度
** 返回:       成功返回TRUE；失败返回FALSE
********************************************************************/
static BOOLEAN SetAlarmTelPara(INT8U *sptr, INT32U slen)
{
    TEL_T alarmtel;
    
    DAL_PP_ReadParaByID(PP_ID_ALARMTEL, (INT8U *)&alarmtel, sizeof(alarmtel));
    if (slen > sizeof(alarmtel.tel)) {
        return false;
    }
    
    alarmtel.tellen = slen;
    YX_MEMCPY(alarmtel.tel, sizeof(alarmtel.tel), sptr, slen);
    if (YX_MEMCMP(DAL_PP_GetParaPtr(PP_ID_ALARMTEL), (INT8U *)&alarmtel, sizeof(alarmtel)) != 0) {
        DAL_PP_StoreParaByID(PP_ID_ALARMTEL, (INT8U *)&alarmtel, sizeof(alarmtel));
    }
    return true;
}

/*******************************************************************
** 函数名:     SetSmscTelPara
** 函数描述:   设置短信服务中心号码
** 参数:       [in] sptr:  数据指针
**             [in] slen:  数据长度
** 返回:       成功返回TRUE；失败返回FALSE
********************************************************************/
static BOOLEAN SetSmscTelPara(INT8U *sptr, INT32U slen)
{
    TEL_T smsctel;
    
    DAL_PP_ReadParaByID(PP_ID_SMSCTEL, (INT8U *)&smsctel, sizeof(smsctel));
    if (slen > sizeof(smsctel.tel)) {
        return false;
    }
    
    smsctel.tellen = slen;
    YX_MEMCPY(smsctel.tel, sizeof(smsctel.tel), sptr, slen);
    if (YX_MEMCMP(DAL_PP_GetParaPtr(PP_ID_SMSCTEL), (INT8U *)&smsctel, sizeof(smsctel)) != 0) {
        DAL_PP_StoreParaByID(PP_ID_SMSCTEL, (INT8U *)&smsctel, sizeof(smsctel));
    }
    return true;
}
        
/*******************************************************************
** 函数名:     SetGprsPara
** 函数描述:   设置服务器通信参数
** 参数:       [in] sptr:    数据指针
**             [in] slen:    数据长度
**             [in] severno: 服务器编号, 0-第一服务器,1-第二服务器
**             [in] id:      主备服务器编号,0-主服务器,1-副服务器
** 返回:       成功返回TRUE；失败返回FALSE
********************************************************************/
static BOOLEAN SetGprsPara(INT8U *sptr, INT32U slen, INT8U severno, INT8U id)
{
    INT8U apnlen, usernamelen, passwordlen, iplen;
    STREAM_T rstrm;
    GPRS_PARA_T gprspara;
    
    if (id >= IP_GROUP_NUM) {
        return false;
    }
    
    YX_InitStrm(&rstrm, sptr, slen);
    
    if (severno == 0) {
        DAL_PP_ReadParaByID(PP_ID_GPRSPARA1, (INT8U *)&gprspara, sizeof(gprspara));
    } else {
        DAL_PP_ReadParaByID(PP_ID_GPRSPARA2, (INT8U *)&gprspara, sizeof(gprspara));
    }
    
    gprspara.ippara[id].isvalid = YX_ReadBYTE_Strm(&rstrm);                    /* 参数有效性 */
    if (id == 0) {
        apnlen = YX_ReadBYTE_Strm(&rstrm);                                     /* APN长度 */
        if (apnlen >= sizeof(gprspara.apn)) {
            goto SET_EXIT;
        }
        YX_ReadDATA_Strm(&rstrm, (INT8U *)gprspara.apn, apnlen);
        gprspara.apn[apnlen] = '\0';
        
        usernamelen = YX_ReadBYTE_Strm(&rstrm);                                /* 用户名长度 */
        if (usernamelen >= sizeof(gprspara.username)) {
            goto SET_EXIT;
        }
        YX_ReadDATA_Strm(&rstrm, (INT8U *)gprspara.username, usernamelen);
        gprspara.username[usernamelen] = '\0';
        
        passwordlen = YX_ReadBYTE_Strm(&rstrm);                                /* 密码长度 */
        if (passwordlen >= sizeof(gprspara.password)) {
            goto SET_EXIT;
        }
        YX_ReadDATA_Strm(&rstrm, (INT8U *)gprspara.password, passwordlen);
        gprspara.password[passwordlen] = '\0';
    } else {
        apnlen = YX_ReadBYTE_Strm(&rstrm);                                     /* APN长度 */
        YX_MovStrmPtr(&rstrm, apnlen);
        usernamelen = YX_ReadBYTE_Strm(&rstrm);                                /* 用户名长度 */
        YX_MovStrmPtr(&rstrm, usernamelen);
        passwordlen = YX_ReadBYTE_Strm(&rstrm);                                /* 密码长度 */
        YX_MovStrmPtr(&rstrm, passwordlen);
    }
    
    iplen = YX_ReadBYTE_Strm(&rstrm);                                          /* IP长度 */
    if (iplen >= sizeof(gprspara.ippara[id].ip)) {
        goto SET_EXIT;
    }
    YX_ReadDATA_Strm(&rstrm, (INT8U *)gprspara.ippara[id].ip, iplen);
    gprspara.ippara[id].ip[iplen] = '\0';
    gprspara.ippara[id].port = YX_ReadHWORD_Strm(&rstrm);                      /* 端口号 */
    
    if (severno == 0) {
        DAL_PP_StoreParaByID(PP_ID_GPRSPARA1, (INT8U *)&gprspara, sizeof(gprspara));
    } else {
        DAL_PP_StoreParaByID(PP_ID_GPRSPARA2, (INT8U *)&gprspara, sizeof(gprspara));
    }
    return true;

SET_EXIT:
    gprspara.ippara[id].isvalid = false;
    if (severno == 0) {
        if (YX_MEMCMP(DAL_PP_GetParaPtr(PP_ID_GPRSPARA1), (INT8U *)&gprspara, sizeof(gprspara)) != 0) {
            DAL_PP_StoreParaByID(PP_ID_GPRSPARA1, (INT8U *)&gprspara, sizeof(gprspara));
        }
    } else {
        if (YX_MEMCMP(DAL_PP_GetParaPtr(PP_ID_GPRSPARA2), (INT8U *)&gprspara, sizeof(gprspara)) != 0) {
            DAL_PP_StoreParaByID(PP_ID_GPRSPARA2, (INT8U *)&gprspara, sizeof(gprspara));
        }
    }
    return false;
}

/*******************************************************************
** 函数名:     SetGprsAttribPara
** 函数描述:   设置服务器通信属性
** 参数:       [in] sptr:    数据指针
**             [in] slen:    数据长度
**             [in] severno: 服务器编号, 0-第一服务器,1-第二服务器
**             [in] id:      主备服务器编号,0-主服务器,1-副服务器
** 返回:       成功返回TRUE；失败返回FALSE
********************************************************************/
static BOOLEAN SetGprsAttribPara(INT8U *sptr, INT32U slen, INT8U severno, INT8U id)
{
    INT8U type;
    STREAM_T rstrm;
    GPRS_PARA_T gprspara;
    
    if (id >= IP_GROUP_NUM) {
        return false;
    }
    
    YX_InitStrm(&rstrm, sptr, slen);
    
    if (severno == 0) {
        DAL_PP_ReadParaByID(PP_ID_GPRSPARA1, (INT8U *)&gprspara, sizeof(gprspara));
    } else {
        DAL_PP_ReadParaByID(PP_ID_GPRSPARA2, (INT8U *)&gprspara, sizeof(gprspara));
    }
    
    type = YX_ReadBYTE_Strm(&rstrm);                                           /* TCP/UDP */
    if (type == 0x01) {
        gprspara.istcp = true;
    } else {
        gprspara.istcp = false;
    }
    gprspara.protocol = YX_ReadBYTE_Strm(&rstrm);                              /* 通信协议类型 */
    if (severno == 0) {
        if (YX_MEMCMP(DAL_PP_GetParaPtr(PP_ID_GPRSPARA1), (INT8U *)&gprspara, sizeof(gprspara)) != 0) {
            DAL_PP_StoreParaByID(PP_ID_GPRSPARA1, (INT8U *)&gprspara, sizeof(gprspara));
        }
    } else {
        if (YX_MEMCMP(DAL_PP_GetParaPtr(PP_ID_GPRSPARA2), (INT8U *)&gprspara, sizeof(gprspara)) != 0) {
            DAL_PP_StoreParaByID(PP_ID_GPRSPARA2, (INT8U *)&gprspara, sizeof(gprspara));
        }
    }
    return TRUE;
}

/*******************************************************************
** 函数名:     SetAuthcodePara
** 函数描述:   设置服务器鉴权码
** 参数:       [in] sptr:    数据指针
**             [in] slen:    数据长度
**             [in] severno: 服务器编号, 0-第一服务器,1-第二服务器
**             [in] id:      主备服务器编号,0-主服务器,1-副服务器
** 返回:       成功返回TRUE；失败返回FALSE
********************************************************************/
static BOOLEAN SetAuthcodePara(INT8U *sptr, INT32U slen, INT8U severno, INT8U id)
{
    AUTH_CODE_T authcode;
    
    if (severno == 0) {
        DAL_PP_ReadParaByID(PP_ID_AUTHCODE1, (INT8U *)&authcode, sizeof(authcode));
        if (slen > sizeof(authcode.authcode)) {
            authcode.len = 0;
            DAL_PP_StoreParaByID(PP_ID_AUTHCODE1, (INT8U *)&authcode, sizeof(authcode));
            return false;
        } else {
            authcode.len = slen;
            YX_MEMCPY(authcode.authcode, sizeof(authcode.authcode), sptr, slen);
            if (YX_MEMCMP(DAL_PP_GetParaPtr(PP_ID_AUTHCODE1), (INT8U *)&authcode, sizeof(authcode)) != 0) {
                DAL_PP_StoreParaByID(PP_ID_AUTHCODE1, (INT8U *)&authcode, sizeof(authcode));
            }
            return true;
        }
    } else {
        DAL_PP_ReadParaByID(PP_ID_AUTHCODE2, (INT8U *)&authcode, sizeof(authcode));
        if (slen > sizeof(authcode.authcode)) {
            authcode.len = 0;
            DAL_PP_StoreParaByID(PP_ID_AUTHCODE2, (INT8U *)&authcode, sizeof(authcode));
            return false;
        } else {
            authcode.len = slen;
            YX_MEMCPY(authcode.authcode, sizeof(authcode.authcode), sptr, slen);
            if (YX_MEMCMP(DAL_PP_GetParaPtr(PP_ID_AUTHCODE2), (INT8U *)&authcode, sizeof(authcode)) != 0) {
                DAL_PP_StoreParaByID(PP_ID_AUTHCODE2, (INT8U *)&authcode, sizeof(authcode));
            }
            return true;
        }
    }
}

/*******************************************************************
** 函数名:     SetLinkPara
** 函数描述:   设置服务器链路维护
** 参数:       [in] sptr:    数据指针
**             [in] slen:    数据长度
**             [in] severno: 服务器编号, 0-第一服务器,1-第二服务器
**             [in] id:      主备服务器编号,0-主服务器,1-副服务器
** 返回:       成功返回TRUE；失败返回FALSE
********************************************************************/
static BOOLEAN SetLinkPara(INT8U *sptr, INT32U slen, INT8U severno, INT8U id)
{
    LINK_PARA_T linkpara;
    STREAM_T rstrm;
    
    YX_InitStrm(&rstrm, sptr, slen);
    
    if (severno == 0) {
        DAL_PP_ReadParaByID(PP_ID_LINKPARA1, (INT8U *)&linkpara, sizeof(linkpara));
    } else {
        DAL_PP_ReadParaByID(PP_ID_LINKPARA2, (INT8U *)&linkpara, sizeof(linkpara));
    }
     
    linkpara.period       = YX_ReadHWORD_Strm(&rstrm);                         /* 终端心跳发送间隔，秒 */
    linkpara.tcp_waittime = YX_ReadHWORD_Strm(&rstrm);                         /* TCP消息应答超时时间，秒 */
    linkpara.tcp_max      = YX_ReadHWORD_Strm(&rstrm);                         /* 消息重传次数 */
    
    if (severno == 0) {
        if (YX_MEMCMP(DAL_PP_GetParaPtr(PP_ID_LINKPARA1), (INT8U *)&linkpara, sizeof(linkpara)) != 0) {
            DAL_PP_StoreParaByID(PP_ID_LINKPARA1, (INT8U *)&linkpara, sizeof(linkpara));
        }
    } else {
        if (YX_MEMCMP(DAL_PP_GetParaPtr(PP_ID_LINKPARA2), (INT8U *)&linkpara, sizeof(linkpara)) != 0) {
            DAL_PP_StoreParaByID(PP_ID_LINKPARA2, (INT8U *)&linkpara, sizeof(linkpara));
        }
    }
    
    return TRUE;
}

/*******************************************************************
** 函数名:     SetVehicheProvincePara
** 函数描述:   设置车辆归属地
** 参数:       [in] sptr:    数据指针
**             [in] slen:    数据长度
** 返回:       成功返回TRUE；失败返回FALSE
********************************************************************/
static BOOLEAN SetVehicheProvincePara(INT8U *sptr, INT32U slen)
{
    VEHICLE_INFO_T vehiche;
    STREAM_T rstrm;
    
    YX_InitStrm(&rstrm, sptr, slen);
    
    DAL_PP_ReadParaByID(PP_ID_VEHICHE, (INT8U *)&vehiche, sizeof(vehiche));
    
    YX_ReadDATA_Strm(&rstrm, vehiche.province, 2);                             /* 省域ID */
    YX_ReadDATA_Strm(&rstrm, vehiche.city, 2);                                 /* 市域ID */
    
    if (YX_MEMCMP(DAL_PP_GetParaPtr(PP_ID_VEHICHE), (INT8U *)&vehiche, sizeof(vehiche)) != 0) {
        DAL_PP_StoreParaByID(PP_ID_VEHICHE, (INT8U *)&vehiche, sizeof(vehiche));
    }
    return TRUE;
}

/*******************************************************************
** 函数名:     SetVehicheCodePara
** 函数描述:   设置车牌号
** 参数:       [in] sptr:    数据指针
**             [in] slen:    数据长度
** 返回:       成功返回TRUE；失败返回FALSE
********************************************************************/
static BOOLEAN SetVehicheCodePara(INT8U *sptr, INT32U slen)
{
    VEHICLE_INFO_T vehiche;
    
    if (slen > sizeof(vehiche.vcode)) {
        return false;
    }
    
    DAL_PP_ReadParaByID(PP_ID_VEHICHE, (INT8U *)&vehiche, sizeof(vehiche));
    vehiche.l_vcode = slen;
    YX_MEMCPY(vehiche.vcode, sizeof(vehiche.vcode), sptr, slen);
    if (YX_MEMCMP(DAL_PP_GetParaPtr(PP_ID_VEHICHE), (INT8U *)&vehiche, sizeof(vehiche)) != 0) {
        DAL_PP_StoreParaByID(PP_ID_VEHICHE, (INT8U *)&vehiche, sizeof(vehiche));
    }
    return TRUE;
}

/*******************************************************************
** 函数名:     SetVehicheColourPara
** 函数描述:   设置车牌颜色
** 参数:       [in] sptr:    数据指针
**             [in] slen:    数据长度
** 返回:       成功返回TRUE；失败返回FALSE
********************************************************************/
static BOOLEAN SetVehicheColourPara(INT8U *sptr, INT32U slen)
{
    VEHICLE_INFO_T vehiche;
    
    DAL_PP_ReadParaByID(PP_ID_VEHICHE, (INT8U *)&vehiche, sizeof(vehiche));
    vehiche.colour = sptr[0];
    if (YX_MEMCMP(DAL_PP_GetParaPtr(PP_ID_VEHICHE), (INT8U *)&vehiche, sizeof(vehiche)) != 0) {
        DAL_PP_StoreParaByID(PP_ID_VEHICHE, (INT8U *)&vehiche, sizeof(vehiche));
    }
    return TRUE;
}

/*******************************************************************
** 函数名:     SetVehicheVinPara
** 函数描述:   设置车辆识别码
** 参数:       [in] sptr:    数据指针
**             [in] slen:    数据长度
** 返回:       成功返回TRUE；失败返回FALSE
********************************************************************/
static BOOLEAN SetVehicheVinPara(INT8U *sptr, INT32U slen)
{
    VEHICLE_INFO_T vehiche;
    
    if (slen > sizeof(vehiche.vin)) {
        return false;
    }
    
    DAL_PP_ReadParaByID(PP_ID_VEHICHE, (INT8U *)&vehiche, sizeof(vehiche));
    vehiche.l_vin = slen;
    YX_MEMCPY(vehiche.vin, sizeof(vehiche.vin), sptr, slen);
    if (YX_MEMCMP(DAL_PP_GetParaPtr(PP_ID_VEHICHE), (INT8U *)&vehiche, sizeof(vehiche)) != 0) {
        DAL_PP_StoreParaByID(PP_ID_VEHICHE, (INT8U *)&vehiche, sizeof(vehiche));
    }
    return TRUE;
}

/*******************************************************************
** 函数名:     SetDeviceInfoPara
** 函数描述:   设置设备信息
** 参数:       [in] sptr:    数据指针
**             [in] slen:    数据长度
** 返回:       成功返回TRUE；失败返回FALSE
********************************************************************/
static BOOLEAN SetDeviceInfoPara(INT8U *sptr, INT32U slen)
{
    INT8U idlen;
    DEVICE_INFO_T device;
    STREAM_T rstrm;
    
    YX_InitStrm(&rstrm, sptr, slen);
    
    DAL_PP_ReadParaByID(PP_ID_DEVICE, (INT8U *)&device, sizeof(device));
    
    idlen = YX_ReadBYTE_Strm(&rstrm);                         /* 终端ID长度 */
    if (idlen > sizeof(device.manufacturerid)) {
        YX_MEMSET(device.manufacturerid, 0, sizeof(device.manufacturerid));
        goto SET_EXIT;
    }
    YX_MEMSET(device.manufacturerid, 0, sizeof(device.manufacturerid));
    YX_ReadDATA_Strm(&rstrm, device.manufacturerid, idlen);
    
    idlen = YX_ReadBYTE_Strm(&rstrm);                         /* 设备型号长度 */
    if (idlen > sizeof(device.devtype)) {
        YX_MEMSET(device.devtype, 0, sizeof(device.devtype));
        goto SET_EXIT;
    }
    YX_MEMSET(device.devtype, 0, sizeof(device.devtype));
    YX_ReadDATA_Strm(&rstrm, device.devtype, idlen);
    
    idlen = YX_ReadBYTE_Strm(&rstrm);                         /* 设备ID长度 */
    if (idlen > sizeof(device.devid)) {
        YX_MEMSET(device.devid, 0, sizeof(device.devid));
        goto SET_EXIT;
    }
    YX_MEMSET(device.devid, 0, sizeof(device.devid));
    YX_ReadDATA_Strm(&rstrm, device.devid, idlen);
    if (YX_MEMCMP(DAL_PP_GetParaPtr(PP_ID_DEVICE), (INT8U *)&device, sizeof(device)) != 0) {
        DAL_PP_StoreParaByID(PP_ID_DEVICE, (INT8U *)&device, sizeof(device));
    }
    return true;
    
SET_EXIT:
    DAL_PP_StoreParaByID(PP_ID_DEVICE, (INT8U *)&device, sizeof(device));
    return false;
}

/*******************************************************************
** 函数名:     SetSleepPara
** 函数描述:   设置省电参数
** 参数:       [in] sptr:    数据指针
**             [in] slen:    数据长度
** 返回:       成功返回TRUE；失败返回FALSE
********************************************************************/
static BOOLEAN SetSleepPara(INT8U *sptr, INT32U slen)
{
    SLEEP_PARA_T sleeppara;
    STREAM_T rstrm;
    
    YX_InitStrm(&rstrm, sptr, slen);
    
    DAL_PP_ReadParaByID(PP_ID_SLEEP, (INT8U *)&sleeppara, sizeof(sleeppara));
    sleeppara.onoff       = YX_ReadBYTE_Strm(&rstrm);                          /* 省电开关, TRUE-打开省电功能,FALSE-关闭省电功能 */
    sleeppara.networkmode = YX_ReadBYTE_Strm(&rstrm);                          /* 省电网络模式, 见 SLEEP_NETWORK_E */
    sleeppara.delay       = YX_ReadBYTE_Strm(&rstrm);                          /* ACC OFF后进入省电延时时间,单位:分钟,默认为20分钟,0表示立即 */
    
    if (YX_MEMCMP(DAL_PP_GetParaPtr(PP_ID_SLEEP), (INT8U *)&sleeppara, sizeof(sleeppara)) != 0) {
        DAL_PP_StoreParaByID(PP_ID_SLEEP, (INT8U *)&sleeppara, sizeof(sleeppara));
    }
    return true;
}

/*******************************************************************
** 函数名:     SetAutoReptPara
** 函数描述:   设置主动上报参数
** 参数:       [in] sptr:    数据指针
**             [in] slen:    数据长度
** 返回:       成功返回TRUE；失败返回FALSE
********************************************************************/
static BOOLEAN SetAutoReptPara(INT8U *sptr, INT32U slen)
{
    AUTOREPT_PARA_T autopara;
    
    DAL_PP_ReadParaByID(PP_ID_AUTOREPT, (INT8U *)&autopara, sizeof(autopara));
    autopara.period = (sptr[0] << 8) + sptr[1];                                /* 周期 */
    if (YX_MEMCMP(DAL_PP_GetParaPtr(PP_ID_AUTOREPT), (INT8U *)&autopara, sizeof(autopara)) != 0) {
        DAL_PP_StoreParaByID(PP_ID_AUTOREPT, (INT8U *)&autopara, sizeof(autopara));
    }
    return true;
}

/*******************************************************************
** 函数名:     SetGpsData
** 函数描述:   设置GPS数据
** 参数:       [in] sptr:    数据指针
**             [in] slen:    数据长度
** 返回:       成功返回TRUE；失败返回FALSE
********************************************************************/
static BOOLEAN SetGpsData(INT8U *sptr, INT32U slen)
{
    INT8U result, weekday;
    INT32U subsecond;
    GPS_DATA_T gpsdata;
    SYSTIME_T systime;
    STREAM_T rstrm;
    
    YX_InitStrm(&rstrm, sptr, slen);
    
    DAL_PP_ReadParaByID(PP_ID_GPSDATA, (INT8U *)&gpsdata, sizeof(gpsdata));
    YX_ReadDATA_Strm(&rstrm, (INT8U *)&gpsdata.systime, 6);                    /* 时间 */
    gpsdata.flag = YX_ReadBYTE_Strm(&rstrm);                                   /* 状态字 */
    YX_ReadDATA_Strm(&rstrm, (INT8U *)&gpsdata.longitude, 4);                  /* 经度 */
    YX_ReadDATA_Strm(&rstrm, (INT8U *)&gpsdata.latitude, 4);                   /* 纬度 */
    YX_ReadHWORD_Strm(&rstrm);                                                 /* 速度 */
    YX_ReadHWORD_Strm(&rstrm);                                                 /* 方向 */
    gpsdata.altitude = YX_ReadHWORD_Strm(&rstrm);                              /* 高程 */
    gpsdata.odometer = YX_ReadLONG_Strm(&rstrm);                               /* 里程 */
    DAL_PP_StoreParaByID(PP_ID_GPSDATA, (INT8U *)&gpsdata, sizeof(gpsdata));
    /* 设置实时时钟 */
    if (!ST_RTC_GetSystime(&systime.date, &systime.time, &weekday, &subsecond)) {/* 实时时钟无效,则将当前时间设置到实时时钟 */
        result = ST_RTC_OpenRtcFunction(RTC_CLOCK_LSE);
        if (result) {
            weekday = 1;
            result = ST_RTC_SetSystime(&gpsdata.systime.date, &gpsdata.systime.time, weekday);
        }
    }
    return true;
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_ACK_GET_PARA
** 函数描述:   从机查询通用参数请求应答
** 参数:       [in] cmd:     命令编码
**             [in] data:    数据指针
**             [in] datalen: 数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_ACK_GET_PARA(INT8U cmd, INT8U *data, INT16U datalen)
{
    INT8U i, paranum, paratype, paralen;
    STREAM_T rstrm;
    
    YX_InitStrm(&rstrm, data, datalen);
    
    paranum = YX_ReadBYTE_Strm(&rstrm);                                        /* 参数个数 */
    for (i = 0; i < paranum; i++) {
        paratype = YX_ReadBYTE_Strm(&rstrm);                                   /* 参数类型 */
        paralen  = YX_ReadBYTE_Strm(&rstrm);                                   /* 参数长度 */
        if (paralen > YX_GetStrmLeftLen(&rstrm)) {
            break;
        }
        if (paralen == 0) {
            continue;
        }
        
        switch (paratype)
        {
        case PARA_MYTEL:                                                       /* 本机号码 */
            SetMyTelPara(YX_GetStrmPtr(&rstrm), paralen);
            YX_MovStrmPtr(&rstrm, paralen);
            break;
        case PARA_SMSC:                                                        /* 短信服务中心号码 */
            SetSmscTelPara(YX_GetStrmPtr(&rstrm), paralen);
            YX_MovStrmPtr(&rstrm, paralen);
            break;
        case PARA_ALARMTEL:                                                    /* 报警号码 */
            SetAlarmTelPara(YX_GetStrmPtr(&rstrm), paralen);
            YX_MovStrmPtr(&rstrm, paralen);
            break;
        case PARA_SERVER1_MAIN:                                                /* 第一服务器通信参数（主） */
            SetGprsPara(YX_GetStrmPtr(&rstrm), paralen, 0, 0);
            YX_MovStrmPtr(&rstrm, paralen);
            break;
        case PARA_SERVER1_BACK:                                                /* 第一服务器通信参数（副） */
            SetGprsPara(YX_GetStrmPtr(&rstrm), paralen, 0, 1);
            YX_MovStrmPtr(&rstrm, paralen);
            break;
        case PARA_SERVER1_ATTRIB:                                              /* 第一服务器通信属性 */
            SetGprsAttribPara(YX_GetStrmPtr(&rstrm), paralen, 0, 0);
            YX_MovStrmPtr(&rstrm, paralen);
            break;
        case PARA_SERVER1_AUTHCODE:                                            /* 第一服务器鉴权码 */
            SetAuthcodePara(YX_GetStrmPtr(&rstrm), paralen, 0, 0);
            YX_MovStrmPtr(&rstrm, paralen);
            break;
        case PARA_SERVER1_LINK:                                                /* 第一服务器链路维护参数 */
            SetLinkPara(YX_GetStrmPtr(&rstrm), paralen, 0, 0);
            YX_MovStrmPtr(&rstrm, paralen);
            break;
        case PARA_SERVER2_MAIN:                                                /* 第二服务器通信参数（主） */
            SetGprsPara(YX_GetStrmPtr(&rstrm), paralen, 1, 0);
            YX_MovStrmPtr(&rstrm, paralen);
            break;
        case PARA_SERVER2_BACK:                                                /* 第二服务器通信参数（副） */
            SetGprsPara(YX_GetStrmPtr(&rstrm), paralen, 1, 1);
            YX_MovStrmPtr(&rstrm, paralen);
            break;
        case PARA_SERVER2_ATTRIB:                                              /* 第二服务器通信属性 */
            SetGprsAttribPara(YX_GetStrmPtr(&rstrm), paralen, 1, 0);
            YX_MovStrmPtr(&rstrm, paralen);
            break;
        case PARA_SERVER2_AUTHCODE:                                            /* 第二服务器鉴权码 */
            SetAuthcodePara(YX_GetStrmPtr(&rstrm), paralen, 1, 0);
            YX_MovStrmPtr(&rstrm, paralen);
            break;
        case PARA_SERVER2_LINK:                                                /* 第二服务器链路维护参数 */
            SetLinkPara(YX_GetStrmPtr(&rstrm), paralen, 1, 0);
            YX_MovStrmPtr(&rstrm, paralen);
            break;
        case PARA_VEHICHE_PROVINCE:                                            /* 车辆归属地 */
            SetVehicheProvincePara(YX_GetStrmPtr(&rstrm), paralen);
            YX_MovStrmPtr(&rstrm, paralen);
            break;
        case PARA_VEHICHE_CODE:                                                /* 车牌号 */
            SetVehicheCodePara(YX_GetStrmPtr(&rstrm), paralen);
            YX_MovStrmPtr(&rstrm, paralen);
            break;
        case PARA_VEHICHE_COLOUR:                                              /* 车辆颜色 */
            SetVehicheColourPara(YX_GetStrmPtr(&rstrm), paralen);
            YX_MovStrmPtr(&rstrm, paralen);
            break;
        case PARA_VEHICHE_BRAND:                                               /* 车辆分类 */
            YX_MovStrmPtr(&rstrm, paralen);
            break;
        case PARA_VEHICHE_VIN:                                                 /* 车辆VIN */
            SetVehicheVinPara(YX_GetStrmPtr(&rstrm), paralen);
            YX_MovStrmPtr(&rstrm, paralen);
            break;
        case PARA_DEVICEINFO:                                                  /* 设备信息 */
            SetDeviceInfoPara(YX_GetStrmPtr(&rstrm), paralen);
            YX_MovStrmPtr(&rstrm, paralen);
            break;
        case PARA_SLEEP:                                                       /* 省电参数 */
            SetSleepPara(YX_GetStrmPtr(&rstrm), paralen);
            YX_MovStrmPtr(&rstrm, paralen);
            break;
        case PARA_AUTOREPT:                                                    /* 主动上报参数 */
            SetAutoReptPara(YX_GetStrmPtr(&rstrm), paralen);
            YX_MovStrmPtr(&rstrm, paralen);
            break;
        case PARA_DATA_GPS:                                                    /* GPS数据 */
            SetGpsData(YX_GetStrmPtr(&rstrm), paralen);
            YX_MovStrmPtr(&rstrm, paralen);
            break;
        default:
            YX_MovStrmPtr(&rstrm, paralen);
            break;
        }
    }
    
    YX_MMI_ListAck(UP_PE_CMD_GET_PARA, _SUCCESS);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_CTL_FUNCTION
** 函数描述:   功能控制
** 参数:       [in] cmd:     命令编码
**             [in] data:    数据指针
**             [in] datalen: 数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_CMD_CTL_FUNCTION(INT8U cmd, INT8U *data, INT16U datalen)
{
    INT8U i, num, control, onoff;
    STREAM_T rstrm;
    STREAM_T *wstrm;
    
    YX_InitStrm(&rstrm, data, datalen);
    wstrm = YX_STREAM_GetBufferStream();
    num = YX_ReadBYTE_Strm(&rstrm);                                            /* 控制个数 */
    for (i = 0; i < num; i++) {
        control = YX_ReadBYTE_Strm(&rstrm);                                    /* 控制类型 */
        onoff   = YX_ReadBYTE_Strm(&rstrm);                                    /* 控制开关 */
        switch (control)
        {
        case 0x01:                                                             /* DB9时钟脉冲(1HZ)*/
            if (onoff == 0x01) {
                DAL_PULSE_OpenClockOutput();                                   /* 打开1HZ脉冲输出 */
            } else {
                DAL_PULSE_CloseClockOutput();                                  /* 关闭1HZ脉冲输出 */
            }
            break;
        default:
            break;
        }
    }
    YX_WriteBYTE_Strm(wstrm, 0x01);
    YX_MMI_ListSend(UP_PE_ACK_CTL_FUNCTION, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 0, 0, 0);
}

#if EN_ICCARD > 0
/*******************************************************************
** 函数名:     HdlMsg_DN_PE_ACK_REPORT_ICCARD_DATA
** 函数描述:   主动上报IC卡原始数据应答
** 参数:       [in] cmd:     命令编码
**             [in] data:    数据指针
**             [in] datalen: 数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_ACK_REPORT_ICCARD_DATA(INT8U cmd, INT8U *data, INT16U datalen)
{
    YX_MMI_ListAck(UP_PE_CMD_REPORT_ICCARD_DATA, _SUCCESS);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_WRITE_ICCARD
** 函数描述:   写IC卡数据
** 参数:       [in] cmd:     命令编码
**             [in] data:    数据指针
**             [in] datalen: 数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_CMD_WRITE_ICCARD(INT8U cmd, INT8U *data, INT16U datalen)
{
    INT32U offset, len;
    STREAM_T rstrm;
    STREAM_T *wstrm;
    
    YX_InitStrm(&rstrm, data, datalen);
    offset = YX_ReadLONG_Strm(&rstrm);                                         /* 偏移地址 */
    len    = YX_ReadBYTE_Strm(&rstrm);                                         /* 数据长度 */
    
    wstrm = YX_STREAM_GetBufferStream();
    if (DAL_IC_WriteData(offset, YX_GetStrmPtr(&rstrm), len) == len) {
        YX_WriteBYTE_Strm(wstrm, 0x01);
    } else {
        YX_WriteBYTE_Strm(wstrm, 0x02);
    }
    YX_MMI_ListSend(UP_PE_ACK_WRITE_ICCARD, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 0, 0, 0);
}
#endif

#if EN_CAN > 0
/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_CAN_SET_PARA
** 函数描述:   CAN通信参数设置请求
** 参数:       [in] cmd:     命令编码
**             [in] data:    数据指针
**             [in] datalen: 数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_CMD_CAN_SET_PARA(INT8U cmd, INT8U *data, INT16U datalen)
{
    INT8U result, com, idtype;
    CAN_CFG_T cfg;
    STREAM_T rstrm;
    
    YX_InitStrm(&rstrm, data, datalen);
    
    com     = YX_ReadBYTE_Strm(&rstrm);                                        /* 通道号 */
    
    cfg.com    = com - 1;                                                      /* 通道编号 */
    cfg.baud   = YX_ReadLONG_Strm(&rstrm);                                     /* 总线波特率 */
    idtype     = YX_ReadBYTE_Strm(&rstrm);                                     /* 帧ID类型 */
    if (idtype == 0x01) {
        cfg.idtype = CAN_ID_TYPE_STD;
    } else {
        cfg.idtype = CAN_ID_TYPE_EXT;
    }
    cfg.mode   = YX_ReadBYTE_Strm(&rstrm);                                     /* 工作模式 */
    
    #if DEBUG_MMI > 0
    printf_com("<CAN通信参数设置(%d)(%d)(%d)(%d)>\r\n", cfg.com, cfg.baud, cfg.idtype, cfg.mode);
    #endif
    
    if (HAL_CAN_OpenCan(&cfg)) {
        result = PE_ACK_MMI;
    } else {
        result = PE_NAK_MMI;
    }
    
    SendAck(UP_PE_ACK_CAN_SET_PARA, com, result);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_CAN_CLOSE
** 函数描述:   CAN通信关闭请求
** 参数:       [in] cmd:     命令编码
**             [in] data:    数据指针
**             [in] datalen: 数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_CMD_CAN_CLOSE(INT8U cmd, INT8U *data, INT16U datalen)
{
    INT8U com;
    STREAM_T rstrm;
    
    #if DEBUG_MMI > 0
    printf_com("<CAN总线关闭请求>\r\n");
    #endif
    
    YX_InitStrm(&rstrm, data, datalen);
    
    com     = YX_ReadBYTE_Strm(&rstrm);                                        /* 通道号 */
    HAL_CAN_CloseCan(com - 1);
    
    SendAck(UP_PE_ACK_CAN_CLOSE, com, PE_ACK_MMI);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_CAN_RESET
** 函数描述:   CAN总线复位请求
** 参数:       [in] cmd:     命令编码
**             [in] data:    数据指针
**             [in] datalen: 数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_CMD_CAN_RESET(INT8U cmd, INT8U *data, INT16U datalen)
{
    INT8U com;
    STREAM_T rstrm;
    
    #if DEBUG_MMI > 0
    printf_com("<CAN总线复位请求>\r\n");
    #endif
    
    YX_InitStrm(&rstrm, data, datalen);
    
    com     = YX_ReadBYTE_Strm(&rstrm);                                        /* 通道号 */
    //ctl     = YX_ReadBYTE_Strm(&rstrm);
    
    SendAck(UP_PE_ACK_CAN_RESET, com, PE_ACK_MMI);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_CAN_SET_FILTER_ID_LIST
** 函数描述:   CAN滤波ID参数设置,列表式
** 参数:       [in] cmd:     命令编码
**             [in] data:    数据指针
**             [in] datalen: 数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_CMD_CAN_SET_FILTER_ID_LIST(INT8U cmd, INT8U *data, INT16U datalen)
{
    INT8U i, result, com, idtype, idnum;
    INT32U *memptr;
    STREAM_T rstrm;
    
    YX_InitStrm(&rstrm, data, datalen);
    
    com    = YX_ReadBYTE_Strm(&rstrm);                                         /* 通道 */
    idtype = YX_ReadBYTE_Strm(&rstrm);                                         /* 帧ID类型 */
    idnum  = YX_ReadBYTE_Strm(&rstrm);                                         /* ID个数 */
    
    #if DEBUG_MMI > 0
    printf_com("<CAN滤波ID参数设置,列表式(%d)(%d)(%d)>\r\n", com, idtype, idnum);
    #endif
    
    memptr = (INT32U *)YX_DYM_Alloc(idnum * 4 + 1);
    if (memptr == 0) {
        SendAck(UP_PE_ACK_CAN_RESET, com - 1, PE_NAK_MMI);
        return;
    }
    
    for (i = 0; i < idnum; i++) {
        memptr[i] = YX_ReadLONG_Strm(&rstrm);                                  /* 滤波ID */
    }
    
    if (HAL_CAN_SetFilterParaByList(com - 1, idtype - 1, idnum, memptr)) {
        result = PE_ACK_MMI;
    } else {
        result = PE_NAK_MMI;
    }
    
    YX_DYM_Free(memptr);
        
    SendAck(UP_PE_ACK_CAN_SET_FILTER_ID_LIST, com, result);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_CAN_SET_FILTER_ID_MASK
** 函数描述:   CAN滤波ID参数设置,掩码式
** 参数:       [in] cmd:     命令编码
**             [in] data:    数据指针
**             [in] datalen: 数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_CMD_CAN_SET_FILTER_ID_MASK(INT8U cmd, INT8U *data, INT16U datalen)
{
    INT8U i, result, com, idtype, idnum;
    INT32U *memptr;
    STREAM_T rstrm;
    
    YX_InitStrm(&rstrm, data, datalen);
    
    com     = YX_ReadBYTE_Strm(&rstrm);                                        /* 通道 */
    idtype = YX_ReadBYTE_Strm(&rstrm);                                         /* 帧ID类型 */
    idnum  = YX_ReadBYTE_Strm(&rstrm);                                         /* ID个数 */
    
    #if DEBUG_MMI > 0
    printf_com("<CAN滤波ID参数设置,掩码式(%d)(%d)(%d)>\r\n", com - 1, idtype, idnum);
    #endif
    
    memptr = (INT32U *)YX_DYM_Alloc(idnum * 8 + 1);
    if (memptr == 0) {
        SendAck(UP_PE_ACK_CAN_RESET, com - 1, PE_NAK_MMI);
        return;
    }
    
    for (i = 0; i < idnum; i++) {
        memptr[i]         = YX_ReadLONG_Strm(&rstrm);                          /* 滤波ID */
        memptr[idnum + i] = YX_ReadLONG_Strm(&rstrm);                          /* 掩码ID */
    }
    
    if (HAL_CAN_SetFilterParaByMask(com - 1, idtype - 1, idnum, memptr, &memptr[idnum])) {
        result = PE_ACK_MMI;
    } else {
        result = PE_NAK_MMI;
    }
    
    YX_DYM_Free(memptr);
        
    SendAck(UP_PE_ACK_CAN_SET_FILTER_ID_MASK, com, result);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_ACK_CAN_DATA_REPORT
** 函数描述:   主动上报CAN数据请求的应答
** 参数:       [in] cmd:     命令编码
**             [in] data:    数据指针
**             [in] datalen: 数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_ACK_CAN_DATA_REPORT(INT8U cmd, INT8U *data, INT16U datalen)
{
    INT16U seq;
    STREAM_T rstrm;
    
    YX_InitStrm(&rstrm, data, datalen);
    
    YX_ReadBYTE_Strm(&rstrm);                                        /* 通道 */
    seq = YX_ReadHWORD_Strm(&rstrm);                                           /* 流水号 */
    
    #if DEBUG_MMI > 0
    printf_com("<主动上报CAN数据请求的应答(%d)>\r\n", seq);
    #endif
    
    YX_MMI_ListSeqAck(UP_PE_CMD_CAN_DATA_REPORT, seq, _SUCCESS);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_CAN_SEND_DATA
** 函数描述:   发送CAN数据请求
** 参数:       [in] cmd:     命令编码
**             [in] data:    数据指针
**             [in] datalen: 数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_CMD_CAN_SEND_DATA(INT8U cmd, INT8U *data, INT16U datalen)
{
    INT8U com, result;
    CAN_DATA_T candata;
    STREAM_T rstrm;
    
    YX_InitStrm(&rstrm, data, datalen);
    
    com = YX_ReadBYTE_Strm(&rstrm);                                            /* 通道 */
    candata.id       = YX_ReadLONG_Strm(&rstrm);                               /* 帧ID,标准帧则取值0~0x7FF,扩展帧则取值0~0x1FFFFFFF. */
    candata.idtype   = YX_ReadBYTE_Strm(&rstrm) - 1;                           /* 帧ID类型,见 CAN_ID_TYPE_E */
    candata.datatype = YX_ReadBYTE_Strm(&rstrm) - 1;                           /* 帧数据类型,见 CAN_RTR_TYPE_E */
    candata.dlc      = YX_ReadBYTE_Strm(&rstrm);                               /* 数据长度,取值0~8 */
    
    YX_ReadDATA_Strm(&rstrm, candata.data, candata.dlc > 8 ? 8 : candata.dlc);
    
    if (HAL_CAN_SendData(com - 1, &candata)) {
        result = PE_ACK_MMI;
    } else {
        result = PE_NAK_MMI;
    }
    
    SendAck(UP_PE_ACK_CAN_SEND_DATA, com, result);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_ACK_CAN_BUS_STATUS_REPORT
** 函数描述:   主动上报CAN总线状态的应答
** 参数:       [in] cmd:     命令编码
**             [in] data:    数据指针
**             [in] datalen: 数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_ACK_CAN_BUS_STATUS_REPORT(INT8U cmd, INT8U *data, INT16U datalen)
{
    YX_MMI_ListAck(UP_PE_CMD_CAN_BUS_STATUS_REPORT, _SUCCESS);
}
#endif


static FUNCENTRY_MMI_T s_functionentry[] = {
     {DN_PE_ACK_GET_PARA,                    HdlMsg_DN_PE_ACK_GET_PARA}                 /* 从机查询通用参数请求应答 */
    ,{DN_PE_CMD_CTL_FUNCTION,                HdlMsg_DN_PE_CMD_CTL_FUNCTION}             /* 功能控制 */
#if EN_ICCARD > 0
    ,{DN_PE_ACK_REPORT_ICCARD_DATA,          HdlMsg_DN_PE_ACK_REPORT_ICCARD_DATA}       /* 主动上报IC卡原始数据应答 */
    ,{DN_PE_CMD_WRITE_ICCARD,                HdlMsg_DN_PE_CMD_WRITE_ICCARD}             /* 写IC卡数据 */
#endif

#if EN_CAN > 0
    ,{DN_PE_CMD_CAN_SET_PARA,                HdlMsg_DN_PE_CMD_CAN_SET_PARA}             // CAN通信参数设置请求
    ,{DN_PE_CMD_CAN_CLOSE,                   HdlMsg_DN_PE_CMD_CAN_CLOSE}                // CAN通信关闭请求
    ,{DN_PE_CMD_CAN_RESET,                   HdlMsg_DN_PE_CMD_CAN_RESET}                // CAN通信总线复位请求
       
    ,{DN_PE_CMD_CAN_SET_FILTER_ID_LIST,      HdlMsg_DN_PE_CMD_CAN_SET_FILTER_ID_LIST}   // CAN滤波ID设置,列表式
    ,{DN_PE_CMD_CAN_SET_FILTER_ID_MASK,      HdlMsg_DN_PE_CMD_CAN_SET_FILTER_ID_MASK}   // CAN滤波ID设置,掩码式
       
    ,{DN_PE_ACK_CAN_DATA_REPORT,             HdlMsg_DN_PE_ACK_CAN_DATA_REPORT}          // 主动上报CAN数据请求的应答
    ,{DN_PE_CMD_CAN_SEND_DATA,               HdlMsg_DN_PE_CMD_CAN_SEND_DATA}            // 发送CAN数据请求
    ,{DN_PE_ACK_CAN_BUS_STATUS_REPORT,       HdlMsg_DN_PE_ACK_CAN_BUS_STATUS_REPORT}    /* 主动上报CAN总线状态的应答(DOWN) */
#endif
};


static void MsgHandler(INT8U cmd, INT8U *data, INT16U datalen)
{
    INT8U i;
    
    for (i = 0; i < sizeof(s_functionentry) / sizeof(s_functionentry[0]); i++) {
        if (s_functionentry[i].cmd == data[0]) {
            s_functionentry[i].entryproc(data[0], &data[1], datalen - 1);
            break;
        }
    }
}

/*******************************************************************
** 函数名:     ScanTmrProc
** 函数描述:   定时器
** 参数:       [in] pdata：定时器特征值
** 返回:       无
********************************************************************/
static void ScanTmrProc(void *pdata)
{
#if EN_CAN > 0
    INT8U i, num;
    CAN_DATA_T candata;
    CAN_STATUS_T canstatus;
    STREAM_T *wstrm;
    
    OS_StartTmr(s_scantmr, PERIOD_SCAN);
#if 0
    if (DAL_INPUT_ReadInstantStatus(IPT_ACC)) {
        DAL_GPIO_PullupPowerSave();
        if (!HAL_CAN_IsOpened(CAN_COM_0)) {
            CAN_CFG_T cfg;
    
            cfg.com     = CAN_COM_0;
            cfg.baud    = 250000;
            cfg.mode    = CAN_WORK_MODE_NORMAL;
            HAL_CAN_OpenCan(&cfg);
            
            candata.id       = 0x01cf17fd;
            candata.idtype   = CAN_ID_TYPE_EXT;
            candata.datatype = CAN_RTR_TYPE_DATA;
            candata.dlc      = 8;
            YX_MEMSET(candata.data, 0xFF, sizeof(candata.data));
            HAL_CAN_SendData(CAN_COM_0, &candata);
        } else {
            if (!YX_MMI_IsON()) {
                candata.id       = 0x01cf17fd;
                candata.idtype   = CAN_ID_TYPE_EXT;
                candata.datatype = CAN_RTR_TYPE_DATA;
                candata.dlc      = 8;
                YX_MEMSET(candata.data, 0xFF, sizeof(candata.data));
                HAL_CAN_SendData(CAN_COM_0, &candata);
            }
        }
    }
#endif
    for (i = 0; i < CAN_COM_MAX; i++) {
        num = HAL_CAN_UsedOfRecvbuf(i);
        
        if (num > 5) {
            num = 5;
        }
        
        if (num > 0) {
            //printf_com("HAL_CAN_UsedOfRecvbuf(%d)\r\n", num);
            wstrm = YX_STREAM_GetBufferStream();
            
            YX_WriteBYTE_Strm(wstrm, i + 1);                                   /* 通道号 */
            YX_WriteHWORD_Strm(wstrm, YX_MMI_GetSendSeq());                    /* 流水号 */
            YX_WriteBYTE_Strm(wstrm, num);                                     /* 数据个数 */
            for (; num > 0; num--) {
                if (HAL_CAN_ReadData(i, &candata)) {
                    YX_WriteLONG_Strm(wstrm, candata.id);                      /* 帧ID */
                    if (candata.idtype == CAN_ID_TYPE_STD) {                   /* 帧ID类型 */
                        YX_WriteBYTE_Strm(wstrm, 0x01);
                    } else {
                        YX_WriteBYTE_Strm(wstrm, 0x02);
                    }
                    
                    if (candata.datatype == CAN_RTR_TYPE_DATA) {               /* 数据类型 */
                        YX_WriteBYTE_Strm(wstrm, 0x01);
                    } else {
                        YX_WriteBYTE_Strm(wstrm, 0x02);
                    }
                    
                    YX_WriteBYTE_Strm(wstrm, candata.dlc);                     /* 数据长度 */
                    YX_WriteDATA_Strm(wstrm, candata.data, candata.dlc);       /* 数据 */
                }
            }
            YX_MMI_DirSend(UP_PE_CMD_CAN_DATA_REPORT, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm));
        }
    }
    
    /* 查询CAN总线状态,并上报 */
    if (++s_count >= 50) {
        s_count = 0;
        for (i = 0; i < CAN_COM_MAX; i++) {
            if (HAL_CAN_GetStatus(i, &canstatus)) {
                if (YX_ACmpString(CASESENSITIVE, (INT8U *)&canstatus, (INT8U *)&s_canstatus[i], sizeof(canstatus), sizeof(canstatus)) != STR_EQUAL) {
                    YX_MEMCPY((INT8U *)&s_canstatus[i], sizeof(canstatus), (INT8U *)&canstatus, sizeof(canstatus));
                    wstrm = YX_STREAM_GetBufferStream();
                    YX_WriteBYTE_Strm(wstrm, i + 1);                           /* 通道号 */
                    YX_WriteBYTE_Strm(wstrm, canstatus.status);
                    YX_WriteBYTE_Strm(wstrm, canstatus.lec);
                    YX_WriteBYTE_Strm(wstrm, canstatus.errorstep);
                    YX_WriteBYTE_Strm(wstrm, canstatus.tec);
                    YX_WriteBYTE_Strm(wstrm, canstatus.rec);
                    YX_MMI_DirSend(UP_PE_CMD_CAN_BUS_STATUS_REPORT, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm));
                }
            }
        }
    }
#endif
}

/*******************************************************************
** 函数名:     YX_MMI_InitCan
** 函数描述:   初始化模块
** 参数:       无
** 返回:       无
********************************************************************/
void YX_MMI_InitCan(void)
{
    YX_MMI_Register(DN_PE_CMD_CAN_TRANS_DATA, MsgHandler);
    
    s_scantmr  = OS_CreateTmr(TSK_ID_APP, (void *)0, ScanTmrProc);
    OS_StartTmr(s_scantmr,  PERIOD_SCAN);
}

#endif




