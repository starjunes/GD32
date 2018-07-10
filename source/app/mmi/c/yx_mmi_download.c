/********************************************************************************
**
** 文件名:     yx_mmi_download.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现固件升级功能
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
#include "hal_flash_drv.h"
#include "dal_gpio_cfg.h"
#include "dal_input_drv.h"
#include "yx_bootmain.h"
#include "yx_debug.h"

#if EN_MMI > 0
#include "yx_mmi_drv.h"

/*
********************************************************************************
* 定义模块配置参数
********************************************************************************
*/

typedef enum {
    DL_IDLE,                 /* 空闲 */
    DL_START,                /* 启动中 */
    DL_LOADING,              /* 下载中 */
    DL_SUCCESS,              /* 下载成功 */
    DL_FAIL,                 /* 下载失败 */
    DL_MAX
} DL_E;



#define MAX_FAIL             5

#define PERIOD_START         _SECOND, 10
#define PERIOD_LOADING       _SECOND, 10
#define PERIOD_RESET         _SECOND, 10

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    INT8U  status;
    INT8U  unlock;
    INT8U  ct_fail;          /* 错误次数 */
    INT32U filesize;         /* 固件长度 */
    INT32U offset;           /* 偏移量 */
} DCB_T;

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static INT8U s_monitortmr;
static DCB_T s_dcb;



/*******************************************************************
** 函数名:     CloseUart
** 函数描述:   关闭所有串口
** 参数:       无
** 返回:       无
********************************************************************/
static void CloseUart(void)
{
    ST_UART_CloseUart(UART_COM_0);
    ST_UART_CloseUart(UART_COM_1);
}

/*******************************************************************
** 函数名:     OpenUart
** 函数描述:   打开所有串口
** 参数:       无
** 返回:       无
********************************************************************/
static void OpenUart(void)
{
    UART_CFG_T cfg;
    
    cfg.com     = UART_COM_0;
    cfg.baud    = 115200;
    cfg.parity  = UART_PARITY_NONE;
    cfg.databit = UART_DATABIT_8;
    cfg.stopbit = UART_STOPBIT_1;
    
    cfg.rx_fcm  = UART_FCM_NULL;
    cfg.tx_fcm  = UART_FCM_NULL;
    
    cfg.rx_len  = 512;
    cfg.tx_len  = 768;
    ST_UART_OpenUart(&cfg);
            
    cfg.com     = UART_COM_1;
    cfg.baud    = 9600;
    cfg.rx_len  = 200;
    cfg.tx_len  = 512;
    ST_UART_OpenUart(&cfg);
}

#if EN_APP == 0
/*******************************************************************
** 函数名:     AbortUpdate
** 函数描述:   中止升级
** 参数:       [in] errcode: 升级异常编码,见DL_RESULT_E
** 返回:       无
********************************************************************/
static void AbortUpdate(INT8U errcode)
{
    HAL_FLASH_Unlock();                                                    /* 解锁FLASH */
    YX_EraseFlagFlashRegion();
    HAL_FLASH_Lock();                                                      /* 锁住FLASH */
    
    s_dcb.status = DL_FAIL;
    s_dcb.unlock = false;
    OS_StopTmr(s_monitortmr);
    YX_MMI_SendFirmwareUpdateResult(errcode);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_ACK_FIRMWARE_UPDATE_REQ
** 函数描述:   从机请求固件升级应答
** 参数:       [in] cmd:    命令编码
**             [in] data:   数据指针
**             [in] datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_ACK_FIRMWARE_UPDATE_REQ(INT8U cmd, INT8U *data, INT16U datalen)
{
    INT8U ctl, acktype;
    STREAM_T rstrm;
    
    #if DEBUG_MMI > 0
    printf_com("<从机请求固件升级应答>\r\n");
    #endif
    
    YX_MMI_ListAck(UP_PE_CMD_FIRMWARE_UPDATE_REQ, _SUCCESS);
    
    YX_InitStrm(&rstrm, data, datalen);
    ctl     = YX_ReadBYTE_Strm(&rstrm);                                        /* 控制命令 */
    acktype = YX_ReadBYTE_Strm(&rstrm);                                        /* 应答类型 */
    if (acktype != 0x01 || ctl != 0x01) {
        AbortUpdate(DL_RESULT_OTHER);
    }
}
#endif

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_FIRMWARE_UPDATE_REQ
** 函数描述:   主机下发固件升级请求
** 参数:       [in] cmd:    命令编码
**             [in] data:   数据指针
**             [in] datalen:数据长度
** 返回:       无
********************************************************************/
static void Callback_UpdateReq(INT8U result)
{
#if EN_APP > 0
    s_dcb.status  = DL_SUCCESS;
    YX_MMI_SendPeResetInform(MMI_RESET_EVENT_UPDATE);                       /* 通知主机外设即将复位 */
    OS_StartTmr(s_monitortmr, PERIOD_RESET);
#else
    s_dcb.status  = DL_LOADING;
    OS_StartTmr(s_monitortmr, PERIOD_LOADING);
    YX_MMI_SendGetFirmwareDataReq(s_dcb.offset, DL_PACKET_SIZE);
#endif
}

static void HdlMsg_DN_PE_CMD_FIRMWARE_UPDATE_REQ(INT8U cmd, INT8U *data, INT16U datalen)
{
#if EN_APP > 0
    INT32U filesize;
    INT8U tempbuf[4];
    STREAM_T rstrm;
    STREAM_T *wstrm;
    
    if (cmd != DN_PE_CMD_FIRMWARE_UPDATE_REQ) {
        return;
    }
    
    YX_InitStrm(&rstrm, data, datalen);
    filesize = YX_ReadLONG_Strm(&rstrm);
    
    #if DEBUG_MMI > 0
    printf_com("<主机下发固件升级请求(%d/0x%x)>\r\n", filesize, filesize);
    #endif
    
    wstrm = YX_STREAM_GetBufferStream();
    if (filesize < 512 || filesize > FLASH_APP_SIZE) {
        YX_WriteBYTE_Strm(wstrm, 0x02);                                        /* 表示固件长度错误，超长 */
    } else {
        YX_WriteBYTE_Strm(wstrm, 0x01);
        
        CloseUart();
        HAL_FLASH_Unlock();                                                    /* 解锁FLASH */
        YX_EraseFlagFlashRegion();
        tempbuf[0] = (INT8U)(CODE_FLAG_UPDATE >> 24);
        tempbuf[1] = (INT8U)(CODE_FLAG_UPDATE >> 16);
        tempbuf[2] = (INT8U)(CODE_FLAG_UPDATE >> 8);
        tempbuf[3] = (INT8U)(CODE_FLAG_UPDATE);
        YX_WriteFlagRegion(0, tempbuf, 4);                                     /* 写入无线升级请求标志 */
        
        tempbuf[0] = filesize >> 24;
        tempbuf[1] = filesize >> 16;
        tempbuf[2] = filesize >> 8;
        tempbuf[3] = filesize;
        YX_WriteFlagRegion(4, tempbuf, 4);
        HAL_FLASH_Lock();                                                      /* 锁住FLASH */
        
        s_dcb.unlock = false;
        OpenUart();
    }
        
    YX_WriteHWORD_Strm(wstrm, DL_PACKET_SIZE);                                 /* 单包数据长度 */
    YX_MMI_ListSend(UP_PE_ACK_FIRMWARE_UPDATE_REQ, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 1, 1, Callback_UpdateReq);
#else
    INT32U filesize;
    STREAM_T rstrm;
    STREAM_T *wstrm;
    
    if (cmd != DN_PE_CMD_FIRMWARE_UPDATE_REQ) {
        return;
    }
    
    YX_InitStrm(&rstrm, data, datalen);
    filesize = YX_ReadLONG_Strm(&rstrm);
    
    #if DEBUG_MMI > 0
    printf_com("<主机下发固件升级请求(%d/0x%x)>\r\n", filesize, filesize);
    #endif
    
    wstrm = YX_STREAM_GetBufferStream();
    if (filesize < 512 || filesize > FLASH_APP_SIZE) {
        YX_WriteBYTE_Strm(wstrm, 0x02);                                        /* 表示固件长度错误，超长 */
    } else {
        YX_WriteBYTE_Strm(wstrm, 0x01);
        
        s_dcb.filesize = filesize;
        s_dcb.offset   = 0;
    }
        
    YX_WriteHWORD_Strm(wstrm, DL_PACKET_SIZE);                                 /* 单包数据长度 */
    YX_MMI_ListSend(UP_PE_ACK_FIRMWARE_UPDATE_REQ, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 1, 1, Callback_UpdateReq);
#endif
}

#if EN_APP == 0
/*******************************************************************
** 函数名:     HdlMsg_DN_PE_ACK_FIRMWARE_DATA_REQ
** 函数描述:   从机请求固件数据应答
** 参数:       [in] cmd:    命令编码
**             [in] data:   数据指针
**             [in] datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_ACK_FIRMWARE_DATA_REQ(INT8U cmd, INT8U *data, INT16U datalen)
{
    INT8U acktype;
    INT32U packetsize, offset, filesize;
    APP_HEAD_T *phead;
    STREAM_T rstrm;
    
    YX_MMI_ListAck(UP_PE_CMD_FIRMWARE_DATA_REQ, _SUCCESS);
    
    YX_InitStrm(&rstrm, data, datalen);
    offset     = YX_ReadLONG_Strm(&rstrm);                                     /* 位置偏移量 */
    packetsize = YX_ReadHWORD_Strm(&rstrm);                                    /* 数据长度 */
    acktype    = YX_ReadBYTE_Strm(&rstrm);                                     /* 应答类型 */
    
    #if DEBUG_MMI > 0
    printf_com("<从机请求固件数据应答(%d)(%d)>\r\n", offset, packetsize);
    #endif
    
    if (offset != s_dcb.offset || offset + packetsize > s_dcb.filesize) {
        return;
    }
    
    OS_StartTmr(s_monitortmr, PERIOD_LOADING);
    if (acktype == 0x01) {                                                     /* 成功 */
        if (offset == 0) {                                                     /* 第一包数据要校验文件头信息 */
            phead = (APP_HEAD_T *)((INT8U *)YX_GetStrmPtr(&rstrm) + FLASH_HEAD_OFFSET);
            YX_MEMCPY(&filesize, sizeof(filesize), phead->filesize, sizeof(phead->filesize));
            if (!YX_CheckFileHead(phead) || filesize != s_dcb.filesize) {
                AbortUpdate(DL_RESULT_VERERR);
                return;
            }
            phead->fileflag = CODE_FLAG_WD;                                    /* 置无线升级标志 */
            
            s_dcb.unlock = true;
            HAL_FLASH_Unlock();                                                /* 解锁FLASH */
            
            CloseUart();
            YX_EraseAppFlashRegion();                                          /* 擦除FLASH */
            OpenUart();
        }
        
        //if ((offset % FLASH_PAGE_SIZE) == 0) {
        //    YX_EraseAppFlashRegionEx(offset);
        //}
        
        if (s_dcb.unlock) {                                                    /* 将数据写入FLASH区 */
            YX_WriteAppCodeRegion(offset, YX_GetStrmPtr(&rstrm), packetsize);
        }
        
        s_dcb.offset += packetsize;
        if (s_dcb.offset < s_dcb.filesize) {                                   /* 未下载完毕 */
            if (s_dcb.filesize - s_dcb.offset > DL_PACKET_SIZE) {
                YX_MMI_SendGetFirmwareDataReq(s_dcb.offset, DL_PACKET_SIZE);
            } else {
                YX_MMI_SendGetFirmwareDataReq(s_dcb.offset, s_dcb.filesize - s_dcb.offset);
            }
        } else {                                                               /* 下载完毕 */
            if (YX_CheckAppIsValid()) {
                s_dcb.status = DL_SUCCESS;
                YX_MMI_SendFirmwareUpdateResult(DL_RESULT_SUCCESS);
            } else {
                if (++s_dcb.ct_fail > MAX_FAIL) {
                    s_dcb.ct_fail = 0;
                    AbortUpdate(DL_RESULT_CHKERR);
                } else {
                    s_dcb.status  = DL_LOADING;
                    s_dcb.offset  = 0;
            
                    YX_MMI_SendFirmwareUpdateReq();
                }
            }
        }
    } else {
        AbortUpdate(DL_RESULT_OTHER);
    }
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_ACK_INFORM_UPDATE_RESULT
** 函数描述:   固件更新结果通知应答
** 参数:       [in] cmd:    命令编码
**             [in] data:   数据指针
**             [in] datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_ACK_INFORM_UPDATE_RESULT(INT8U cmd, INT8U *data, INT16U datalen)
{
    #if DEBUG_MMI > 0
    printf_com("<固件更新结果通知应答(%d)>\r\n", s_dcb.status);
    #endif
    
    YX_MMI_ListAck(UP_PE_CMD_INFORM_UPDATE_RESULT, _SUCCESS);
    if (s_dcb.status == DL_SUCCESS) {
        YX_MMI_SendPeResetInform(MMI_RESET_EVENT_UPDATE);                       /* 通知主机外设即将复位 */
        OS_StartTmr(s_monitortmr, PERIOD_RESET);
    }
}
#endif

static FUNCENTRY_MMI_T s_functionentry[] = {
                  #if EN_APP > 0
                       DN_PE_CMD_FIRMWARE_UPDATE_REQ,              HdlMsg_DN_PE_CMD_FIRMWARE_UPDATE_REQ      /* 主机下发固件升级请求 */
                  #else
                       DN_PE_ACK_FIRMWARE_UPDATE_REQ,              HdlMsg_DN_PE_ACK_FIRMWARE_UPDATE_REQ      /* 从机请求固件升级应答 */
                      ,DN_PE_CMD_FIRMWARE_UPDATE_REQ,              HdlMsg_DN_PE_CMD_FIRMWARE_UPDATE_REQ      /* 主机下发固件升级请求 */
                      ,DN_PE_ACK_FIRMWARE_DATA_REQ,                HdlMsg_DN_PE_ACK_FIRMWARE_DATA_REQ        /* 从机请求固件数据应答 */
                      ,DN_PE_ACK_INFORM_UPDATE_RESULT,             HdlMsg_DN_PE_ACK_INFORM_UPDATE_RESULT     /* 固件更新结果通知应答 */
                  #endif

                                         };


/*******************************************************************
** 函数名:     MonitorTmrProc
** 函数描述:   监控定时器
** 参数:       [in] pdata：定时器特征值
** 返回:       无
********************************************************************/
static void MonitorTmrProc(void *pdata)
{
    //if (YX_CheckAppIsValid()) {                                                /* 无固件升级标志且APP有效 */
        //YX_MMI_StopUpdate();
        //return;
    //}
    
    if (!YX_MMI_IsON()) {
        OS_StartTmr(s_monitortmr, PERIOD_START);
        return;
    }
    
    switch (s_dcb.status) 
    {
    case DL_IDLE:                                                              /* 空闲 */
        break;
    case DL_START:                                                             /* 启动中 */
        YX_MMI_SendFirmwareUpdateReq();
        OS_StartTmr(s_monitortmr, PERIOD_START);
        break;
    case DL_LOADING:                                                           /* 下载中 */
        if (s_dcb.filesize - s_dcb.offset > DL_PACKET_SIZE) {
            YX_MMI_SendGetFirmwareDataReq(s_dcb.offset, DL_PACKET_SIZE);
        } else {
            YX_MMI_SendGetFirmwareDataReq(s_dcb.offset, s_dcb.filesize - s_dcb.offset);
        }
        OS_StartTmr(s_monitortmr, PERIOD_LOADING);
        break;
    case DL_SUCCESS:                                                           /* 下载成功 */
#if EN_APP > 0
        OS_RESET(RESET_EVENT_DIRECT);
#else
        YX_EraseFlagFlashRegion();                                             /* 清除无线升级标志 */
        HAL_FLASH_Lock();                                                      /* 锁住FLASH */
        OS_RESET(RESET_EVENT_DIRECT);
#endif
        break;
    case DL_FAIL:                                                              /* 下载失败 */
        s_dcb.status  = DL_IDLE;
        OS_StopTmr(s_monitortmr);
        break;
    default:
        break;
    }
}

/*******************************************************************
** 函数名:     YX_MMI_InitDownload
** 函数描述:   功能初始化
** 参数:       无
** 返回:       无
********************************************************************/
void YX_MMI_InitDownload(void)
{
    INT8U i;
    
    YX_MEMSET((INT8U *)&s_dcb, 0, sizeof(s_dcb));
    
    for (i = 0; i < sizeof(s_functionentry) / sizeof(s_functionentry[0]); i++) {
        YX_MMI_Register(s_functionentry[i].cmd, s_functionentry[i].entryproc);
    }
    
    s_monitortmr = OS_CreateTmr(TSK_ID_APP, (void *)0, MonitorTmrProc);
#if EN_APP == 0
    YX_MMI_StartUpdate();
#endif
}

/*******************************************************************
** 函数名:     YX_MMI_SendFirmwareUpdateReq
** 函数描述:   从机请求固件升级
** 参数:       无
** 返回:       无
********************************************************************/
void YX_MMI_SendFirmwareUpdateReq(void)
{
    STREAM_T *wstrm;
    
    if (!YX_MMI_IsON()) {
        return;
    }
    
    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteBYTE_Strm(wstrm, 0x01);
    YX_MMI_ListSend(UP_PE_CMD_FIRMWARE_UPDATE_REQ, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 3, 3, 0);
}

/*******************************************************************
** 函数名:     YX_MMI_SendGetFirmwareDataReq
** 函数描述:   从机请求固件升级
** 参数:       [in] offset:    偏移地址
**             [in] packetlen: 请求的数据长度
** 返回:       无
********************************************************************/
void YX_MMI_SendGetFirmwareDataReq(INT32U offset, INT16U packetlen)
{
    STREAM_T *wstrm;
    
    if (!YX_MMI_IsON()) {
        return;
    }
    
    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteLONG_Strm(wstrm, offset);
    YX_WriteHWORD_Strm(wstrm, packetlen);
    YX_MMI_ListSend(UP_PE_CMD_FIRMWARE_DATA_REQ, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 3, 3, 0);
}

/*******************************************************************
** 函数名:     YX_MMI_SendFirmwareUpdateResult
** 函数描述:   固件更新结果通知
** 参数:       [in] result: 固件更新结果,见 DL_RESULT_E
** 返回:       无
********************************************************************/
void YX_MMI_SendFirmwareUpdateResult(INT8U result)
{
    STREAM_T *wstrm;
    
    if (!YX_MMI_IsON()) {
        return;
    }
    
    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteBYTE_Strm(wstrm, result);
    YX_MMI_ListSend(UP_PE_CMD_INFORM_UPDATE_RESULT, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 3, 3, 0);
}

/*******************************************************************
** 函数名:     YX_MMI_StartUpdate
** 函数描述:   启动固件升级
** 参数:       无
** 返回:       无
********************************************************************/
void YX_MMI_StartUpdate(void)
{
    s_dcb.status   = DL_START;
    s_dcb.filesize = 0;
    s_dcb.offset   = 0;
    s_dcb.ct_fail  = 0;
    
    YX_MMI_SendFirmwareUpdateReq();
    OS_StartTmr(s_monitortmr, PERIOD_START);
}

/*******************************************************************
** 函数名:     YX_MMI_StopUpdate
** 函数描述:   停止固件升级
** 参数:       无
** 返回:       无
********************************************************************/
void YX_MMI_StopUpdate(void)
{
    s_dcb.status   = DL_IDLE;
    s_dcb.filesize = 0;
    s_dcb.offset   = 0;
    s_dcb.ct_fail  = 0;
    s_dcb.unlock   = false;
    
    HAL_FLASH_Lock();                                                          /* 锁住FLASH */
    OS_StopTmr(s_monitortmr);
}

#endif
