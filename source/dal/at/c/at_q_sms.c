/********************************************************************************
**
** 文件名:     at_q_sms.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现手机模块短信指令发送队列管理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2014/06/15 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "yx_include.h"
#include "yx_misc.h"
#include "at_drv.h"
#include "at_cmd_common.h"
#include "at_send.h"
#include "at_q_sms.h"

#if EN_AT > 0
/*
********************************************************************************
* define config parameters
********************************************************************************
*/

#define PERIOD_SUSPEND       _SECOND,  3
#define PERIOD_WAIT          _MILTICK, 2

/*
********************************************************************************
* define s_dcb.status
********************************************************************************
*/
#define _FREE                               0
#define _SUSPEND                            1
#define _SENDHEAD                           2
#define _WAIT                               3
#define _SENDATA                           4

/*
********************************************************************************
* define module variants
********************************************************************************
*/
typedef struct {
    BOOLEAN isready;
    BOOLEAN isclog;
    
    INT8U  status;
    INT16U datalen;
    INT8U *memptr;
    void (*informer)(INT8U result);
} DCB_T;

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static DCB_T s_dcb;
static INT8U s_waittmr;


static BOOLEAN CanSendSM(void)
{
    if (s_dcb.status == _FREE && (!s_dcb.isclog)) {
        return true;
    } else {
        return false;
    }
}

static void FreeCurSM(INT8U result)
{
    if (s_dcb.memptr != 0) {
        YX_DYM_Free(s_dcb.memptr);
        s_dcb.memptr = 0;
    }
    
    if (s_dcb.status == _FREE || s_dcb.status == _SUSPEND) {
        return;
    }

    #if DEBUG_AT > 0
    if (result == _SUCCESS) {
        printf_com("<send sm success>\r\n");
    } else {
        printf_com("<send sm error>\r\n");
    }
    #endif
    
    s_dcb.status  = _SUSPEND;
    s_dcb.isready = false;
    OS_StartTmr(s_waittmr, PERIOD_SUSPEND);
    if (s_dcb.informer != 0) {
        s_dcb.informer(result);
        s_dcb.informer = 0;
    }
}

static void InformSendHead(INT8U result)
{
    if (result == AT_SUCCESS) {
        s_dcb.status = _WAIT;
        OS_StartTmr(s_waittmr, PERIOD_WAIT);
    } else {
        FreeCurSM(_FAILURE);
    }
}

static void InformSendPDU(INT8U result)
{
    if (result == AT_SUCCESS) {
        //NotifySendSMSuccess();
        FreeCurSM(_SUCCESS);
    } else {
        //NotifySendSMFailure();
        FreeCurSM(_FAILURE);
    }
}

static void WaitTmrProc(void *pdata)
{
    OS_StopTmr(s_waittmr);
    
    if (s_dcb.status == _WAIT) {
        s_dcb.status  = _SENDATA;
        s_dcb.isready = true;
    } else if (s_dcb.status == _SUSPEND) {
        s_dcb.status  = _FREE;
        s_dcb.isready = false;
    }
}

static void DiagnoseProc(void)
{
    OS_ASSERT((s_dcb.status <= _SENDATA), RETURN_VOID);
    if (s_dcb.status == _WAIT || s_dcb.status == _SUSPEND) {
        OS_ASSERT(OS_TmrIsRun(s_waittmr), RETURN_VOID);
    }
}

/*******************************************************************
** 函数名:     AT_Q_InitSms
** 函数描述:   初始化模块
** 参数:       无
** 返回:       无
********************************************************************/
void AT_Q_InitSms(void)
{
    s_dcb.status    = _FREE;
    s_dcb.isready   = false;
    s_dcb.isclog    = false;
    
    s_waittmr = OS_CreateTmr(TSK_ID_HAL, 0, WaitTmrProc);
    //OS_InstallDiag(DiagnoseProc);
}

/*******************************************************************
** 函数名:     AT_Q_SmsSendEntry
** 函数描述:   发送入口
** 参数:       无
** 返回:       无
********************************************************************/
void AT_Q_SmsSendEntry(void)
{
    INT8U headlen;
    INT8U sendbuf[20];
    
    if (!AT_SEND_CanSendATCmd() || !s_dcb.isready) {
        return;
    }

    if (s_dcb.status == _SENDHEAD) {
        if (s_dcb.isclog) {
            return;
        }
        s_dcb.isready = false;
        
        headlen  = AT_CMD_SendSm(sendbuf, sizeof(sendbuf), (s_dcb.datalen - 2)/2);
        AT_SEND_SendCmd(&g_sms_sendhead_para, sendbuf, headlen, InformSendHead);
    } else if (s_dcb.status == _SENDATA) {
        s_dcb.isready = false;
        AT_SEND_SendCmd(&g_sms_senddata_para, s_dcb.memptr, s_dcb.datalen, InformSendPDU);
    }
}

/*******************************************************************
** 函数名:     AT_Q_ClearSmsQueue
** 函数描述:   清除短信指令队列
** 参数:       无
** 返回:       无
********************************************************************/
void AT_Q_ClearSmsQueue(void)
{
    if (s_dcb.status == _WAIT || s_dcb.status == _SENDHEAD || s_dcb.status == _SENDATA) {
        AT_CORE_EscapeATCmd();
    }
    FreeCurSM(_FAILURE);
   
    s_dcb.status   = _FREE;
    s_dcb.isready  = false;
    s_dcb.isclog   = false;
    OS_StopTmr(s_waittmr);
}

/*******************************************************************
** 函数名:     AT_Q_IsSendingSm
** 函数描述:   是否正常发送短信
** 参数:       无
** 返回:       是则返回TRUE, 否则返回FALSE
********************************************************************/
BOOLEAN AT_Q_IsSendingSm(void)
{
    switch (s_dcb.status)
    {
    case _FREE:
    case _SUSPEND:
        return false;
    case _SENDHEAD:
        if (s_dcb.isready) {
            return false;
        } else {
            return true;
        }
    default:
        return true;
    }
}

void ClogSM(void)
{
    s_dcb.isclog = true;
}

void UnclogSM(void)
{
    if (s_dcb.isclog) {
        s_dcb.isclog = false;
    }
}

BOOLEAN IsClogSM(void)
{
    return s_dcb.isclog;
}

/*******************************************************************
** 函数名:     AT_Q_SendSm
** 函数描述:   发送短信
** 参数:       无
** 返回:       成功则返回TRUE, 失败返回FALSE
********************************************************************/
BOOLEAN AT_Q_SendSm(INT8U dcs, INT8U *dataptr, INT16U datalen, void (*fp)(INT8U result))
{
    DiagnoseProc();
    
    if (datalen == 0) {
        return false;
    }
    
    if (!CanSendSM()) {
        return false;
    }
    
    s_dcb.datalen = datalen;
    s_dcb.memptr = YX_DYM_Alloc(s_dcb.datalen);
    if (s_dcb.memptr == 0) {
        #if DEBUG_ERR > 0
        printf_com("<AT_Q_SendSm malloc memory fail>\r\n");
        #endif
        return false;
    }

    YX_MEMCPY(s_dcb.memptr, s_dcb.datalen, dataptr, datalen);
    
    //s_dcb.headlen  = AT_CMD_SendSm(tmpbuf, sizeof(tmpbuf), (s_dcb.datalen - 2)/2);
    s_dcb.status   = _SENDHEAD;
    s_dcb.isready  = true;
    s_dcb.informer = fp;
    
    return true;
}


#endif
