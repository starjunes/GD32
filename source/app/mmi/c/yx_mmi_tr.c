/********************************************************************************
**
** 文件名:     yx_mmi_tr.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现行驶记录仪业务管理
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
#include "dal_gpio_cfg.h"
#include "dal_input_drv.h"
#include "dal_ic_drv.h"
#include "yx_debug.h"

#if EN_MMI > 0
#include "yx_mmi_drv.h"

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


/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/


#if EN_ICCARD > 0

/*******************************************************************
** 函数名:     AsmDriverInfo
** 函数描述:   组帧刷卡信息
** 参数:       [in]cmd:    命令编码
**             [in]type:   应答类型
** 返回:       无
********************************************************************/
static void AsmDriverInfo(STREAM_T *wstrm)
{
#if EN_ICCARD_PASER > 0
    INT8U num;
    INT8U *ptr;
    DRIVER_T *driverinfo;
    
    num = 0;
    driverinfo = DAL_IC_GetDriverInfo();
    if (driverinfo->status == IC_STATUS_SUCC) {                                /* 读卡成功 */
        YX_WriteBYTE_Strm(wstrm, 0x01);                                        /* 读卡结果 */
        ptr = YX_GetStrmPtr(wstrm);
        YX_WriteBYTE_Strm(wstrm, 0x00);                                        /* 参数个数 */
        
        if (driverinfo->driveridlen > 0) {                                     /* 司机代码 */
            num++;
            YX_WriteBYTE_Strm(wstrm, 0x01);
            YX_WriteBYTE_Strm(wstrm, driverinfo->driveridlen);
            YX_WriteDATA_Strm(wstrm, driverinfo->driverid, driverinfo->driveridlen);
        }
        
        if (driverinfo->passwordlen > 0) {                                     /* 司机密码 */
            num++;
            YX_WriteBYTE_Strm(wstrm, 0x02);
            YX_WriteBYTE_Strm(wstrm, driverinfo->passwordlen);
            YX_WriteDATA_Strm(wstrm, driverinfo->password, driverinfo->passwordlen);
        }
        
        if (driverinfo->namelen > 0) {                                         /* 司机姓名 */
            num++;
            YX_WriteBYTE_Strm(wstrm, 0x03);
            YX_WriteBYTE_Strm(wstrm, driverinfo->namelen);
            YX_WriteDATA_Strm(wstrm, driverinfo->name, driverinfo->namelen);
        }
        
        if (driverinfo->identitylen > 0) {                                     /* 司机身份证 */
            num++;
            YX_WriteBYTE_Strm(wstrm, 0x04);
            YX_WriteBYTE_Strm(wstrm, driverinfo->identitylen);
            YX_WriteDATA_Strm(wstrm, driverinfo->identity, driverinfo->identitylen);
        }
        
        if (driverinfo->qualificationlen > 0) {                                /* 从业资格证 */
            num++;
            YX_WriteBYTE_Strm(wstrm, 0x05);
            YX_WriteBYTE_Strm(wstrm, driverinfo->qualificationlen);
            YX_WriteDATA_Strm(wstrm, driverinfo->qualification, driverinfo->qualificationlen);
        }
        
        if (driverinfo->institutionlen > 0) {                                  /* 发证机构 */
            num++;
            YX_WriteBYTE_Strm(wstrm, 0x06);
            YX_WriteBYTE_Strm(wstrm, driverinfo->institutionlen);
            YX_WriteDATA_Strm(wstrm, driverinfo->institution, driverinfo->institutionlen);
        }
        
        if (driverinfo->driverlicenselen > 0) {                                /* 驾驶证号 */
            num++;
            YX_WriteBYTE_Strm(wstrm, 0x07);
            YX_WriteBYTE_Strm(wstrm, driverinfo->driverlicenselen);
            YX_WriteDATA_Strm(wstrm, driverinfo->driverlicense, driverinfo->driverlicenselen);
        }
        
        if (driverinfo->date.year != 0) {                                      /* 驾驶证有效期 */
            num++;
            YX_WriteBYTE_Strm(wstrm, 0x08);
            YX_WriteBYTE_Strm(wstrm, 3);
            YX_WriteDATA_Strm(wstrm, (INT8U *)&driverinfo->date, 3);
        }
        
        *ptr = num;
    } else if (driverinfo->status == IC_STATUS_ERROR) {                        /* 读卡失败 */
        YX_WriteBYTE_Strm(wstrm, 0x02);                                        /* 读卡结果 */
        YX_WriteBYTE_Strm(wstrm, 0x00);                                        /* 参数个数 */
    } else {                                                                   /* 卡已拔出 */
        YX_WriteBYTE_Strm(wstrm, 0x03);                                        /* 读卡结果 */
        YX_WriteBYTE_Strm(wstrm, 0x00);                                        /* 参数个数 */
    }
#else
    DRIVER_T *driverinfo;
    
    driverinfo = DAL_IC_GetDriverInfo();
    if (driverinfo->status == IC_STATUS_SUCC) {                                /* 读卡成功 */
        YX_WriteBYTE_Strm(wstrm, 0x01);                                        /* 读卡结果 */
        YX_WriteBYTE_Strm(wstrm, 0x01);                                        /* 参数个数 */
        YX_WriteBYTE_Strm(wstrm, 0x01);                                        /* 参数类型 */
        YX_WriteBYTE_Strm(wstrm, 0x00);                                        /* 参数长度 */
        //YX_WriteSTR_Strm(wstrm, 0x08);                                        /* 参数长度 */
    } else if (driverinfo->status == IC_STATUS_ERROR) {                        /* 读卡失败 */
        YX_WriteBYTE_Strm(wstrm, 0x02);                                        /* 读卡结果 */
        YX_WriteBYTE_Strm(wstrm, 0x00);                                        /* 参数个数 */
    } else {                                                                   /* 卡已拔出 */
        YX_WriteBYTE_Strm(wstrm, 0x03);                                        /* 读卡结果 */
        YX_WriteBYTE_Strm(wstrm, 0x00);                                        /* 参数个数 */
    }
#endif
}


/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_GET_ICCARD_INFO
** 函数描述:   读卡请求
** 参数:       [in] cmd:    命令编码
**             [in] data:   数据指针
**             [in] datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_CMD_GET_ICCARD_INFO(INT8U cmd, INT8U *data, INT16U datalen)
{
#if EN_ICCARD_PASER > 0
    INT8U type;
    STREAM_T *wstrm;
    
    DAL_IC_RereadIccardData();
    type = data[0];
    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteBYTE_Strm(wstrm, type);
    AsmDriverInfo(wstrm);
    
    YX_MMI_ListSend(UP_PE_ACK_GET_ICCARD_INFO, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 0, 0, 0);
#else
    INT8U type;
    INT8U *ptr;
    INT32U len;
    STREAM_T *wstrm;
    DRIVER_T *driverinfo;
    
    driverinfo = DAL_IC_GetDriverInfo();
    type = data[0];
    if (driverinfo->status == IC_STATUS_SUCC) {                                /* 读卡成功 */
        wstrm = YX_STREAM_GetBufferStream();
        YX_WriteBYTE_Strm(wstrm, type);
        AsmDriverInfo(wstrm);
        YX_MMI_ListSend(UP_PE_ACK_GET_ICCARD_INFO, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 0, 0, 0);
        
        ptr = DAL_IC_GetData();
        len  = (ptr[0] << 24);
        len |= (ptr[1] << 16);
        len |= (ptr[2] << 8);
        len |= (ptr[3]);
        YX_MMI_SendIccardData(ptr, len + 4);
    } else {                                                                   /* 读卡失败 */
        DAL_IC_RereadIccardData();
        
        wstrm = YX_STREAM_GetBufferStream();
        YX_WriteBYTE_Strm(wstrm, type);
        AsmDriverInfo(wstrm);
    
        YX_MMI_ListSend(UP_PE_ACK_GET_ICCARD_INFO, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 0, 0, 0);
    }
    
    #if DEBUG_MMI > 0
    printf_com("<收到IC读卡请求(%d)>\r\n", driverinfo->status);
    #endif
#endif
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_ACK_REPORT_ICCARD_INFO
** 函数描述:   主动上报IC卡信息应答
** 参数:       [in] cmd:    命令编码
**             [in] data:   数据指针
**             [in] datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_ACK_REPORT_ICCARD_INFO(INT8U cmd, INT8U *data, INT16U datalen)
{
    #if DEBUG_MMI > 0
    printf_com("<收到主动上报IC卡信息应答>\r\n");
    #endif
    
    YX_MMI_ListAck(UP_PE_CMD_REPORT_ICCARD_INFO, _SUCCESS);
}


static FUNCENTRY_MMI_T s_functionentry[] = {
                       DN_PE_CMD_GET_ICCARD_INFO,                 HdlMsg_DN_PE_CMD_GET_ICCARD_INFO      // 读卡请求
                      ,DN_PE_ACK_REPORT_ICCARD_INFO,              HdlMsg_DN_PE_ACK_REPORT_ICCARD_INFO   // 主动上报IC卡信息应答

                      
                                         };


/*******************************************************************
** 函数名:     Callback_Iccard
** 函数描述:   IC卡事件回调通知
** 参数:       [in] info:  驾驶员信息
** 返回:       无
********************************************************************/
static void Callback_Iccard(DRIVER_T *info, INT8U *data, INT32U datalen)
{
#if EN_ICCARD_PASER > 0
    YX_MMI_SendIccardInfo();
#else
    if (info->status == IC_STATUS_SUCC) {                                      /* 读卡成功 */
        YX_MMI_SendIccardInfo();
        YX_MMI_SendIccardData(data, datalen);
    } else {                                                                   /* 读卡失败 */
        YX_MMI_SendIccardInfo();
    }
#endif
}
#endif

/*******************************************************************
** 函数名:     YX_MMI_InitTr
** 函数描述:   功能初始化
** 参数:       无
** 返回:       无
********************************************************************/
void YX_MMI_InitTr(void)
{
#if EN_ICCARD > 0
    INT8U i;

    for (i = 0; i < sizeof(s_functionentry) / sizeof(s_functionentry[0]); i++) {
        YX_MMI_Register(s_functionentry[i].cmd, s_functionentry[i].entryproc);
    }
    
    DAL_IC_RegistIccardProc(Callback_Iccard);
#endif
}

#if EN_ICCARD > 0
/*******************************************************************
** 函数名:     YX_MMI_SendIccardInfo
** 函数描述:   发送IC卡信息
** 参数:       无
** 返回:       无
********************************************************************/
void YX_MMI_SendIccardInfo(void)
{
    STREAM_T *wstrm;
    
    if (!YX_MMI_IsON()) {
        return;
    }
    wstrm = YX_STREAM_GetBufferStream();
    AsmDriverInfo(wstrm);
    YX_MMI_ListSend(UP_PE_CMD_REPORT_ICCARD_INFO, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 3, 3, 0);
}

/*******************************************************************
** 函数名:     YX_MMI_SendIccardData
** 函数描述:   发送IC卡信息
** 参数:       无
** 返回:       无
********************************************************************/
void YX_MMI_SendIccardData(INT8U *data, INT32U datalen)
{
    if (!YX_MMI_IsON()) {
        return;
    }
    YX_MMI_ListSend(UP_PE_CMD_REPORT_ICCARD_DATA, data, datalen, 3, 3, 0);
}
#endif

#endif
