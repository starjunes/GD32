/********************************************************************************
**
** 文件名:     yx_jt2_tsend.c
** 版权所有:   (c) 2005-2012 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现交通部协议第二服务器TCP数据发送
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
#include "dal_pp_drv.h"
#include "yx_protocol_type.h"
#include "yx_protocol_send.h"
#include "yx_jt2_tlink.h"


/*
********************************************************************************
* 定义配置参数
********************************************************************************
*/
#define COM_TCP              SOCKET_CH_1
#define SIZE_SEND            1600
/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/




/*******************************************************************
** 函数名:     YX_JTT2_InitSend
** 函数描述:   初始化发送模块
** 参数:       无
** 返回:       无
********************************************************************/
void YX_JTT2_InitSend(void)
{
    ;
}

/*******************************************************************
** 函数名:     YX_JTT2_SendHeart
** 函数描述:   发送链路维护帧
** 参数:       无
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN YX_JTT2_SendHeart(void)
{
    INT32U msgattrib;
    TEL_T alarmtel;
    STREAM_T *wstrm;
    PROTOCOL_COM_T attrib;
    
    DAL_PP_ReadParaByID(PP_ID_ALARMTEL, (INT8U *)&alarmtel, sizeof(alarmtel));
    
    msgattrib = 0;
    YX_MEMSET(&attrib, 0, sizeof(attrib));
    attrib.attrib   = SM_ATTR_TCP;                                             /* 发送属性 */
    attrib.channel  = COM_TCP;                                                 /* 发送通道 */
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
** 函数名:     YX_JTT2_SendReqRegist
** 函数描述:   发送注册请求数据包
** 参数:       无
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN YX_JTT2_SendReqRegist(void)
{
    INT32U typelen, msgattrib;
    TEL_T alarmtel;
    VEHICLE_INFO_T vehicleinfo;
    DEVICE_INFO_T devinfo;
    STREAM_T *wstrm;
    PROTOCOL_COM_T attrib;

    DAL_PP_ReadParaByID(PP_ID_ALARMTEL, (INT8U *)&alarmtel, sizeof(alarmtel));
    DAL_PP_ReadParaByID(PP_ID_DEVICE, (INT8U *)&devinfo, sizeof(devinfo));
    DAL_PP_ReadParaByID(PP_ID_VEHICHE, (INT8U *)&vehicleinfo, sizeof(vehicleinfo));

    msgattrib = 0;
    YX_MEMSET(&attrib, 0, sizeof(attrib));
    attrib.attrib   = SM_ATTR_TCP;                                             /* 发送属性 */
    attrib.channel  = COM_TCP;                                                 /* 发送通道 */
    attrib.type     = PTOTOCOL_TYPE_LOG;                                       /* 协议数据类型 */
    attrib.priority = PTOTOCOL_PRIO_HIGH;                                      /* 协议数据优先级 */
    if (alarmtel.tellen < sizeof(attrib.tel)) {                                   /* 手机号码 */
        YX_MEMCPY(attrib.tel, alarmtel.tellen, alarmtel.tel, alarmtel.tellen);
        attrib.tel[alarmtel.tellen] = '\0';
    }
    
    wstrm = YX_PROTOCOL_GetBufferStrm();                                       /* 获取数据流缓存 */
    YX_PROTOCOL_AsmFrameHead(wstrm, UP_CMD_REG, msgattrib, 0);                 /* 组帧数据头 */
            
    YX_WriteDATA_Strm(wstrm, vehicleinfo.province, sizeof(vehicleinfo.province));/* 省域ID */
    YX_WriteDATA_Strm(wstrm, vehicleinfo.city, sizeof(vehicleinfo.city));      /* 市域ID */
    YX_WriteDATA_Strm(wstrm, devinfo.manufacturerid, sizeof(devinfo.manufacturerid));/* 制造商ID */
    
    typelen = YX_STRLEN((char *)devinfo.devtype);                              /* 终端型号 */
    if (typelen < sizeof(devinfo.devtype)) {
        YX_MEMSET(&devinfo.devtype[typelen], 0x00, sizeof(devinfo.devtype) - typelen); 
    }
    YX_WriteDATA_Strm(wstrm, devinfo.devtype, sizeof(devinfo.devtype));
    
    YX_WriteDATA_Strm(wstrm, devinfo.devid, sizeof(devinfo.devid));            /* 终端ID */
    YX_WriteBYTE_Strm(wstrm, vehicleinfo.colour);                              /* 车牌颜色 */
    if (vehicleinfo.colour != 0) {                                             /* 车牌号或车辆VIN号 */
        YX_WriteDATA_Strm(wstrm,  vehicleinfo.vcode, vehicleinfo.l_vcode);
    } else {
        YX_WriteDATA_Strm(wstrm,  vehicleinfo.vin, vehicleinfo.l_vin);
    }
    
    return YX_PROTOCOL_SendData(wstrm, &attrib, 0);
}

/*******************************************************************
** 函数名:     YX_JTT2_SendReqUnregist
** 函数描述:   发送注销请求数据包
** 参数:       无
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN YX_JTT2_SendReqUnregist(void)
{
    INT32U msgattrib;
    TEL_T alarmtel;
    STREAM_T *wstrm;
    PROTOCOL_COM_T attrib;

    DAL_PP_ReadParaByID(PP_ID_ALARMTEL, (INT8U *)&alarmtel, sizeof(alarmtel));
    
    msgattrib = 0;
    YX_MEMSET(&attrib, 0, sizeof(attrib));
    attrib.attrib   = SM_ATTR_TCP;                                             /* 发送属性 */
    attrib.channel  = COM_TCP;                                                 /* 发送通道 */
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
** 函数名:     YX_JTT2_SendReqLogin
** 函数描述:   发送登陆请求数据包
** 参数:       无
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN YX_JTT2_SendReqLogin(void)
{
    INT32U msgattrib;
    AUTH_CODE_T authinfo;
    TEL_T alarmtel;
    STREAM_T *wstrm;
    PROTOCOL_COM_T attrib;

    DAL_PP_ReadParaByID(PP_ID_ALARMTEL, (INT8U *)&alarmtel, sizeof(alarmtel));
    DAL_PP_ReadParaByID(PP_ID_AUTHCODE2, (INT8U *)&authinfo, sizeof(authinfo));
    
    msgattrib = 0;                                                             /* 消息属性 */
    YX_MEMSET(&attrib, 0, sizeof(attrib));
    attrib.attrib   = SM_ATTR_TCP;                                             /* 发送属性 */
    attrib.channel  = COM_TCP;                                                 /* 发送通道 */
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
** 函数名:     YX_JTT2_SendFromGPRS
** 函数描述:   发送数据
** 参数:       [in] attrib:   通道属性和重发属性，见PROTOCOL_COM_T
**             [in] data:     数据指针
**             [in] datalen:  数据长度
** 返回:       成功返回true,失败返回false
********************************************************************/
BOOLEAN YX_JTT2_SendFromGPRS(PROTOCOL_COM_T *attrib, INT8U *data, INT16U datalen)
{
    INT16U templen, memlen;    
    INT8U *memptr;

    if (datalen > SIZE_SEND) {
        return false;
    }
    
    //if (DAL_GPRS_ComIsSuspended()) {
    //    return false;
    //}
    
    if (attrib->type == PTOTOCOL_TYPE_LOG) {                                   /* 登陆帧 */
        if (!YX_JTT2_LinkIsConnect()) {                                        /* TCP链路未建立 */
            return false;
        }
    } else {                                                                   /* 普通数据 */
        if (!YX_JTT2_LinkIsLoged()) {                                          /* 未登陆到GPRS前置机 */
            return false;
        }
    }
    
    memlen = datalen * 2 + 2;
    memptr = YX_DYM_Alloc(memlen);
    if (memptr == 0) {
        #if DEBUG_ERR > 0
        printf_com("<YX_JTT2_SendFromGPRS malloc memory fail>\r\n");
        #endif
        
        return false;
    }
    
    templen = YX_AssembleByRules(memptr, memlen, data, datalen, (ASMRULE_T *)&g_jtt2_rules);
    OS_ASSERT((templen <= memlen), RETURN_FALSE);

    if (AT_SOCKET_SocketSend(COM_TCP, memptr, templen) < 0) {
        YX_DYM_Free(memptr);
        return false;
    } else {
        YX_DYM_Free(memptr);
        return TRUE;
    }
}
    
