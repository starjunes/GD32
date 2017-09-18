/********************************************************************************
**
** 文件名:     yx_jt2_usend.c
** 版权所有:   (c) 2005-2012 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现交通部协议第二服务器UDP数据发送
** 
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2013/10/21 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "yx_includes.h"
#include "yx_dm.h"
#include "dal_pp_drv.h"
#include "dal_sm_drv.h"
#include "yx_jt2_usend.h"
#include "yx_protocol_send.h"
#include "yx_debug.h"
/*
********************************************************************************
* 定义配置参数
********************************************************************************
*/
#define COM_UDP              CHA_JT2_ULINK
#define SIZE_SEND            1600
/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/



/*******************************************************************
** 函数名:     YX_JTU2_SendHeart
** 函数描述:   发送链路维护帧
** 参数:       无
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN YX_JTU2_SendHeart(void)
{
    INT32U msgattrib;
    TEL_T alarmtel;
    STREAM_T *wstrm;
    PROTOCOL_COM_T attrib;
    
    DAL_PP_ReadParaByID(PP_ID_ALARMTEL, (INT8U *)&alarmtel, sizeof(alarmtel));
    
    msgattrib = 0;
    
    YX_MEMSET(&attrib, 0, sizeof(attrib));
    attrib.attrib   = SM_ATTR_UDP;                                             /* 发送属性 */
    attrib.channel  = COM_UDP;                                                 /* 发送通道 */
    attrib.type     = PTOTOCOL_TYPE_LOG;                                       /* 协议数据类型 */
    attrib.priority = PTOTOCOL_PRIO_HIGH;                                      /* 协议数据优先级 */
    if (alarmtel.tellen < sizeof(attrib.tel)) {                                   /* 手机号码 */
        YX_MEMCPY(attrib.tel, alarmtel.tellen, alarmtel.tel, alarmtel.tellen);
        attrib.tel[alarmtel.tellen] = '\0';
    }
    
    wstrm = YX_PROTOCOL_GetBufferStrm();                                       /* 获取数据流缓存 */
    YX_PROTOCOL_AsmFrameHead(wstrm, UP_CMD_HEART, msgattrib, 0);               /* 组帧数据头 */
    return YX_PROTOCOL_SendData(wstrm, &attrib, 0);
}

/*******************************************************************
** 函数名:     YX_JTU2_SendReqRegist
** 函数描述:   发送注册请求数据包
** 参数:       无
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN YX_JTU2_SendReqRegist(void)
{
    INT32U len, typelen, msgattrib;
    TEL_T alarmtel;
    VEHICLE_INFO_T vehicleinfo;
    DEVICE_INFO_T devinfo;
    STREAM_T *wstrm;
    PROTOCOL_COM_T attrib;

    DAL_PP_ReadParaByID(PP_ID_ALARMTEL, (INT8U *)&alarmtel, sizeof(alarmtel));
    DAL_PP_ReadParaByID(PP_ID_DEVICE, (INT8U *)&devinfo, sizeof(devinfo));
    DAL_PP_ReadParaByID(PP_ID_VEHICHE, (INT8U *)&vehicleinfo, sizeof(vehicleinfo));

    if (devinfo.colour != 0) {
        len = sizeof(devinfo) - sizeof(devinfo.vehistring) + YX_STRLEN((char *)devinfo.vehistring);
    } else {
        if (vehicleinfo.l_vin > MAX_VIN_LEN) {
            vehicleinfo.l_vin = MAX_VIN_LEN;
        }
        len = sizeof(devinfo) - sizeof(devinfo.vehistring) + vehicleinfo.l_vin;
    }
    msgattrib = len;
    
    YX_MEMSET(&attrib, 0, sizeof(attrib));
    attrib.attrib   = SM_ATTR_UDP;                                             /* 发送属性 */
    attrib.channel  = COM_UDP;                                                 /* 发送通道 */
    attrib.type     = PTOTOCOL_TYPE_LOG;                                       /* 协议数据类型 */
    attrib.priority = PTOTOCOL_PRIO_HIGH;                                      /* 协议数据优先级 */
    if (alarmtel.tellen < sizeof(attrib.tel)) {                                   /* 手机号码 */
        YX_MEMCPY(attrib.tel, alarmtel.tellen, alarmtel.tel, alarmtel.tellen);
        attrib.tel[alarmtel.tellen] = '\0';
    }
    
    wstrm = YX_PROTOCOL_GetBufferStrm();                                       /* 获取数据流缓存 */
    YX_PROTOCOL_AsmFrameHead(wstrm, UP_CMD_REG, msgattrib, 0);                 /* 组帧数据头 */
            
    YX_WriteDATA_Strm(wstrm, devinfo.province, sizeof(devinfo.province));      /* 省域ID */
    YX_WriteDATA_Strm(wstrm, devinfo.city, sizeof(devinfo.city));              /* 市域ID */
    YX_WriteDATA_Strm(wstrm, devinfo.manufacturerid, sizeof(devinfo.manufacturerid));/* 制造商ID */
    
    typelen = YX_STRLEN((char *)devinfo.devtype);                              /* 终端型号 */
    if (typelen < sizeof(devinfo.devtype)) {
        YX_MEMSET(&devinfo.devtype[typelen], 0x00, sizeof(devinfo.devtype) - typelen); 
    }
    YX_WriteDATA_Strm(wstrm, devinfo.devtype, sizeof(devinfo.devtype));
    
    YX_WriteDATA_Strm(wstrm, devinfo.devid, sizeof(devinfo.devid));            /* 终端ID */
    YX_WriteBYTE_Strm(wstrm, devinfo.colour);                                  /* 车牌颜色 */
    
    if (devinfo.colour != 0) {                                                 /* 车牌号或车辆VIN号 */
        YX_WriteSTR_Strm(wstrm,  (char *)devinfo.vehistring); 
    } else {
        YX_WriteDATA_Strm(wstrm,  vehicleinfo.vin, vehicleinfo.l_vin);
    }
    
    return YX_PROTOCOL_SendData(wstrm, &attrib, 0);
}

/*******************************************************************
** 函数名:     YX_JTU2_SendReqUnregist
** 函数描述:   发送注销请求数据包
** 参数:       无
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN YX_JTU2_SendReqUnregist(void)
{
    INT32U msgattrib;
    TEL_T alarmtel;
    STREAM_T *wstrm;
    PROTOCOL_COM_T attrib;

    DAL_PP_ReadParaByID(PP_ID_ALARMTEL, (INT8U *)&alarmtel, sizeof(alarmtel));
    
    msgattrib = 0;
    
    YX_MEMSET(&attrib, 0, sizeof(attrib));
    attrib.attrib   = SM_ATTR_UDP;                                             /* 发送属性 */
    attrib.channel  = COM_UDP;                                                 /* 发送通道 */
    attrib.type     = PTOTOCOL_TYPE_LOG;                                       /* 协议数据类型 */
    attrib.priority = PTOTOCOL_PRIO_HIGH;                                      /* 协议数据优先级 */
    if (alarmtel.tellen < sizeof(attrib.tel)) {                                   /* 手机号码 */
        YX_MEMCPY(attrib.tel, alarmtel.tellen, alarmtel.tel, alarmtel.tellen);
        attrib.tel[alarmtel.tellen] = '\0';
    }
    
    wstrm = YX_PROTOCOL_GetBufferStrm();                                       /* 获取数据流缓存 */
    YX_PROTOCOL_AsmFrameHead(wstrm, UP_CMD_REG_UNREGIST, msgattrib, 0);        /* 组帧数据头 */
    return YX_PROTOCOL_SendData(wstrm, &attrib, 0);
}

/*******************************************************************
** 函数名:     YX_JTU2_SendReqLogin
** 函数描述:   发送登陆请求数据包
** 参数:       无
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN YX_JTU2_SendReqLogin(void)
{
    INT32U authlen, msgattrib;
    AUTH_CODE_T authinfo;
    TEL_T alarmtel;
    STREAM_T *wstrm;
    PROTOCOL_COM_T attrib;

    DAL_PP_ReadParaByID(PP_ID_ALARMTEL, (INT8U *)&alarmtel, sizeof(alarmtel));
    DAL_PP_ReadParaByID(AUTHCODE2_, (INT8U *)&authinfo, sizeof(authinfo));
    
    authlen = YX_STRLEN((char *)authinfo.authcode);
    
    msgattrib = authlen;                                                       /* 消息属性 */
    
    YX_MEMSET(&attrib, 0, sizeof(attrib));
    attrib.attrib   = SM_ATTR_UDP;                                             /* 发送属性 */
    attrib.channel  = COM_UDP;                                                 /* 发送通道 */
    attrib.type     = PTOTOCOL_TYPE_LOG;                                       /* 协议数据类型 */
    attrib.priority = PTOTOCOL_PRIO_HIGH;                                      /* 协议数据优先级 */
    if (alarmtel.tellen < sizeof(attrib.tel)) {                                   /* 手机号码 */
        YX_MEMCPY(attrib.tel, alarmtel.tellen, alarmtel.tel, alarmtel.tellen);
        attrib.tel[alarmtel.tellen] = '\0';
    }
    
    wstrm = YX_PROTOCOL_GetBufferStrm();                                       /* 获取数据流缓存 */
    YX_PROTOCOL_AsmFrameHead(wstrm, UP_CMD_AULOG, msgattrib, 0);               /* 组帧数据头 */
    YX_WriteSTR_Strm(wstrm, (char *)authinfo.authcode);                        /* 鉴权码 */
    
    return YX_PROTOCOL_SendData(wstrm, &attrib, 0);
}

/*******************************************************************
** 函数名:     YX_JTU2_SendFromGPRS
** 函数描述:   发送数据
** 参数:       [in] attrib:   通道属性和重发属性，见PROTOCOL_COM_T
**             [in] data:     数据指针
**             [in] datalen:  数据长度
** 返回:       成功返回true,失败返回false
********************************************************************/
BOOLEAN YX_JTU2_SendFromGPRS(PROTOCOL_COM_T *attrib, INT8U *data, INT16U datalen)
{
    INT16U templen, memlen;    
    INT8U *memptr;
    
    if (datalen > SIZE_SEND) {
        return false;
    }
    
    if (DAL_GPRS_ComIsSuspended()) {
        return false;
    }
    
    if (attrib->type == PTOTOCOL_TYPE_LOG) {                                   /* 登陆帧 */
        if (!YX_JTU2_LinkIsConnect()) {                                        /* UDP链路未建立 */
            return false;
        }
    } else {                                                                   /* 普通数据 */
        if (!YX_JTU2_LinkIsLoged()) {                                          /* 未登陆到GPRS前置机 */
            return false;
        }
    }
    
    memlen = datalen * 2 + 2;
    memptr = YX_DYM_Alloc(memlen);
    if (memptr == 0) {
        #if DEBUG_ERR > 0
        printf_com("<YX_JTU2_SendFromGPRS malloc memory fail>\r\n");
        #endif
        
        return false;
    }
    
    templen = YX_AssembleByRules(memptr, data, datalen, (ASMRULE_T *)&g_jtt1_rules);
    OS_ASSERT((templen <= memlen), RETURN_FALSE);

    if (!DAL_UDP_SendData(attrib->attrib, COM_UDP, memptr, templen)) {
        YX_DYM_Free(memptr);
        return false;
    }
    return TRUE;
}
    
