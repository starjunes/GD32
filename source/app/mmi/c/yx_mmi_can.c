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
#include "dal_pp_drv.h"
#include "yx_debug.h"
#include "yx_sleep.h"
#include "yx_jieyou_drv.h"


#if EN_MMI > 0
#include "yx_mmi_drv.h"

/*
********************************************************************************
*宏定义
********************************************************************************
*/
#define PERIOD_SCAN          _TICK, 1

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
** 函数名:     SetDeviceIdPara
** 函数描述:   设置设备ID
** 参数:       [in] sptr:    数据指针
**             [in] slen:    数据长度
** 返回:       成功返回TRUE；失败返回FALSE
********************************************************************/
static BOOLEAN SetDeviceIdPara(INT8U *sptr, INT32U slen)
{
    DEVICE_ID_T deviceid;
    
    if (slen > sizeof(deviceid.id) || slen == 0) {
        return FALSE;
    }
    
    DAL_PP_ReadParaByID(PP_ID_DEVICEID, (INT8U *)&deviceid, sizeof(deviceid));
    deviceid.idlen = slen;
    YX_MEMCPY(deviceid.id, slen, sptr, slen);
    if (YX_MEMCMP(DAL_PP_GetParaPtr(PP_ID_DEVICEID), (INT8U *)&deviceid, sizeof(deviceid)) != 0) {
        DAL_PP_StoreParaByID(PP_ID_DEVICEID, (INT8U *)&deviceid, sizeof(deviceid));
    }
    return true;
}
/*******************************************************************
** 函数名:     PaserCommonData
** 函数描述:   解析通用参数
** 参数:       [in] wstrm:   数据流组帧
**             [in] sptr:    数据指针
**             [in] slen:    数据长度
** 返回:       成功返回TRUE；失败返回FALSE
********************************************************************/
static void PaserCommonData(STREAM_T *wstrm, INT8U *sptr, INT32U slen)
{
    INT8U i, paranum, paratype, paralen, result;
    STREAM_T rstrm;
    
    YX_InitStrm(&rstrm, sptr, slen);
    paranum = YX_ReadBYTE_Strm(&rstrm);                                        /* 参数个数 */
    YX_WriteBYTE_Strm(wstrm, paranum);
    for (i = 0; i < paranum; i++) {
        paratype = YX_ReadBYTE_Strm(&rstrm);                                   /* 参数类型 */
        paralen  = YX_ReadBYTE_Strm(&rstrm);                                   /* 参数长度 */
        YX_WriteBYTE_Strm(wstrm, paratype);
        if (paralen > YX_GetStrmLeftLen(&rstrm)) {
            YX_WriteBYTE_Strm(wstrm, PE_NAK_MMI);
            break;
        }
        
        switch (paratype)
        {
        case PARA_DEVICEID:                                                    /* 设备ID */
            result = SetDeviceIdPara(YX_GetStrmPtr(&rstrm), paralen);
            break;
        default:
            result = false;
            break;
        }
        
        YX_MovStrmPtr(&rstrm, paralen);
        if (result) {
            YX_WriteBYTE_Strm(wstrm, PE_ACK_MMI);
        } else {
            YX_WriteBYTE_Strm(wstrm, PE_NAK_MMI);
        }
    }
}

/*******************************************************************
** 函数名:     CheckCanAdResult
** 函数描述:   节油CAN AD检测结果
** 参数:       [in] 
** 返回:       无
********************************************************************/
static void CheckCanAdResult(INT16U *ad, INT8U count)
{
    INT8U i;
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();

    YX_WriteBYTE_Strm(wstrm, count);
    for(i = 0; i < count; i++) {
        YX_WriteHWORD_Strm(wstrm, ad[i]);
    }

    YX_MMI_ListSend(UP_PE_ACK_CAN_AD_CHECK, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 0, 0, 0);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_ACK_SLAVE_GET_PARA
** 函数描述:   从机查询通用参数请求应答
** 参数:       [in] cmd:     命令编码
**             [in] data:    数据指针
**             [in] datalen: 数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_ACK_SLAVE_GET_PARA(INT8U cmd, INT8U *data, INT16U datalen)
{
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    PaserCommonData(wstrm, data, datalen);
    YX_MMI_ListAck(UP_PE_CMD_SLAVE_GET_PARA, _SUCCESS);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_HOST_SET_PARA
** 函数描述:   主机设置通用参数
** 参数:       [in] cmd:     命令编码
**             [in] data:    数据指针
**             [in] datalen: 数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_CMD_HOST_SET_PARA(INT8U cmd, INT8U *data, INT16U datalen)
{
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    PaserCommonData(wstrm, data, datalen);
    YX_MMI_ListSend(UP_PE_ACK_HOST_SET_PARA, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 0, 0, 0);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_HOST_GET_PARA
** 函数描述:   主机查询通用参数
** 参数:       [in] cmd:     命令编码
**             [in] data:    数据指针
**             [in] datalen: 数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_CMD_HOST_GET_PARA(INT8U cmd, INT8U *data, INT16U datalen)
{
    INT8U i, paranum, paratype;
    STREAM_T rstrm;
    STREAM_T *wstrm;
    DEVICE_ID_T deviceid;
    
    wstrm = YX_STREAM_GetBufferStream();
    YX_InitStrm(&rstrm, data, datalen);
    paranum = YX_ReadBYTE_Strm(&rstrm);                                        /* 参数个数 */
    YX_WriteBYTE_Strm(wstrm, paranum);
    for (i = 0; i < paranum; i++) {
        paratype = YX_ReadBYTE_Strm(&rstrm);                                   /* 参数类型 */
        YX_WriteBYTE_Strm(wstrm, paratype);
        switch (paratype) 
        {
        case PARA_DEVICEID:                                                    /* 设备ID */
            DAL_PP_ReadParaByID(PP_ID_DEVICEID, (INT8U *)&deviceid, sizeof(deviceid));
            YX_WriteBYTE_Strm(wstrm, deviceid.idlen);
            YX_WriteDATA_Strm(wstrm, deviceid.id, deviceid.idlen);
            break;
        default:
            YX_WriteBYTE_Strm(wstrm, 0);
            break;
        }
    }
    
    YX_MMI_ListSend(UP_PE_ACK_HOST_GET_PARA, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 0, 0, 0);
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
                //DAL_PULSE_OpenClockOutput();                                   /* 打开1HZ脉冲输出 */
            } else {
                //DAL_PULSE_CloseClockOutput();                                  /* 关闭1HZ脉冲输出 */
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
    CAN_CFG_INFO_T pp_cfg;
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

    if(!DAL_PP_ReadParaByID(PP_ID_CAN_CFG, (INT8U *)&pp_cfg, sizeof(pp_cfg))) {
        YX_MEMSET(&pp_cfg, 0, sizeof(pp_cfg));
    }

    if(pp_cfg.isvaild && (pp_cfg.idtype == cfg.idtype) && (pp_cfg.mode == cfg.mode) && (pp_cfg.baud == cfg.baud)) {
        result = PE_ACK_MMI;
    } else {
        if (HAL_CAN_OpenCan(&cfg)) {
            result = PE_ACK_MMI;
        } else {
            result = PE_NAK_MMI;
        }

        pp_cfg.isvaild = TRUE;
        pp_cfg.idtype = cfg.idtype;
        pp_cfg.mode = cfg.mode;
        pp_cfg.baud = cfg.baud;
        DAL_PP_StoreParaByID(PP_ID_CAN_CFG, (INT8U *)&pp_cfg, sizeof(pp_cfg));
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
        #if DEBUG_ERR > 0
        printf_com("<HdlMsg_DN_PE_CMD_CAN_SET_FILTER_ID_LIST malloc memory fail>\r\n");
        #endif
        SendAck(UP_PE_ACK_CAN_RESET, com, PE_NAK_MMI);
        return;
    }
    
    for (i = 0; i < idnum; i++) {
        memptr[i] = YX_ReadLONG_Strm(&rstrm);                                  /* 滤波ID */
    }

    YX_JieYou_SetCanFilterByList(idtype - 1, idnum, memptr);
    /* can通道确认后,才进行滤波配置,未确认，则确认完后节油模块会配置 */
    if(YX_JieYou_IsConfirm()) {
        if (HAL_CAN_SetFilterParaByList(com - 1, idtype - 1, idnum, memptr)) {
            result = PE_ACK_MMI;
        } else {
            result = PE_NAK_MMI;
        }
    } else {
        result = PE_ACK_MMI;
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
        #if DEBUG_ERR > 0
        printf_com("<HdlMsg_DN_PE_CMD_CAN_SET_FILTER_ID_MASK malloc memory fail>\r\n");
        #endif
        SendAck(UP_PE_ACK_CAN_RESET, com, PE_NAK_MMI);
        return;
    }
    
    for (i = 0; i < idnum; i++) {
        memptr[i]         = YX_ReadLONG_Strm(&rstrm);                          /* 滤波ID */
        memptr[idnum + i] = YX_ReadLONG_Strm(&rstrm);                          /* 掩码ID */
    }

    YX_JieYou_SetCanFilterByMask(idtype - 1, idnum, memptr, &memptr[idnum]);
    if(YX_JieYou_IsConfirm()) {
        if (HAL_CAN_SetFilterParaByMask(com - 1, idtype - 1, idnum, memptr, &memptr[idnum])) {
            result = PE_ACK_MMI;
        } else {
            result = PE_NAK_MMI;
        }
    }else {
        result = PE_ACK_MMI;
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

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_ACK_CAN_AD_CHECK
** 函数描述:   CAN总线AD检测
** 参数:       [in] cmd:     命令编码
**             [in] data:    数据指针
**             [in] datalen: 数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_ACK_CAN_AD_CHECK(INT8U cmd, INT8U *data, INT16U datalen)
{
    YX_JieYou_StartADCheck(CheckCanAdResult);
}

#endif


static FUNCENTRY_MMI_T s_functionentry[] = {
     {DN_PE_ACK_SLAVE_GET_PARA,              HdlMsg_DN_PE_ACK_SLAVE_GET_PARA}           /* 从机查询通用参数请求应答 */
    ,{DN_PE_CMD_HOST_SET_PARA,               HdlMsg_DN_PE_CMD_HOST_SET_PARA}            /* 主机设置通用参数 */
    ,{DN_PE_CMD_HOST_GET_PARA,               HdlMsg_DN_PE_CMD_HOST_GET_PARA}            /* 主机查询通用参数 */
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
    ,{DN_PE_CMD_CAN_AD_CHECK,                HdlMsg_DN_PE_ACK_CAN_AD_CHECK}             /* CAN总线AD检测 */
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
        
        if (num > 35) {
            num = 35;
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

                    YX_SLEEP_ConfirmRecCan((INT8U*)&candata);
                    YX_JieYou_Confirm(i, &candata);

                    printf_com("收到can(%d) id:%08x dlc:%d", i, candata.id, candata.dlc);
                    printf_hex(candata.data, candata.dlc);
                    printf_com("\r\n");
                }
            }

            /* 等can通道确认完后,再往主机发送can数据 */
            if(YX_JieYou_IsConfirm()) {
                YX_MMI_DirSend(UP_PE_CMD_CAN_DATA_REPORT, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm));
            }
            
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




