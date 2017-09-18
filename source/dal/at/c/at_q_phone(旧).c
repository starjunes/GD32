/********************************************************************************
**
** 文件名:     at_phone.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现手机模块电话指令发送
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
#include "yx_list.h"
#include "at_send.h"
#include "at_cmd_common.h"
#include "at_q_phone.h"

#if EN_AT > 0
#if EN_AT_PHONE > 0
/*
********************************************************************************
* define config parameters
********************************************************************************
*/
#define SIZE_BUF                                5

/*
********************************************************************************
* define status
********************************************************************************
*/
#define _EXIST                                  0x01
#define _READY                                  0x02

/*
********************************************************************************
* define VCB_STRUCT
********************************************************************************
*/
typedef struct {
    INT8U   status;
    INT8U   len;
    INT8U   buffer[SIZE_BUF];
    void    (*fp)(INT8U result);
    AT_CMD_PARA_T const *cmdpara;    
} VCB_STRUCT;

/*
********************************************************************************
* define module variants
********************************************************************************
*/
static VCB_STRUCT VCB;


void SendVoiceCmdInformer_ATVOICE(INT8U result)
{
/*  if ((VCB.status & (_EXIST | _READY)) == _EXIST) {*/
    if (VCB.status & _EXIST) {
        VCB.status = 0;
        if (VCB.fp != 0) {
            if (result == AT_SUCCESS) {
                result = _SUCCESS;
            } else {
                result = _FAILURE;
            }
            VCB.fp(result);
        }
    }
}

/*******************************************************************
** 函数名:     AT_Q_InitPhone
** 函数描述:   初始化模块
** 参数:       无
** 返回:       无
********************************************************************/
void AT_Q_InitPhone(void)
{
    VCB.status = 0;
}

/*******************************************************************
** 函数名:     AT_Q_PhoneSendEntry
** 函数描述:   发送入口
** 参数:       无
** 返回:       无
********************************************************************/
void AT_Q_PhoneSendEntry(void)
{
    if ((VCB.status & (_EXIST | _READY)) != (_EXIST | _READY)) {
        return;
    }
    
    if (!AT_SEND_CanSendATCmd()) {
        return;
    }
    
    VCB.status &= (~_READY);
    AT_SEND_SendCmd(VCB.cmdpara, VCB.buffer, VCB.len, SendVoiceCmdInformer_ATVOICE);
}

void AT_Q_ClearPhoneQueue(void)
{
    if ((VCB.status & (_EXIST | _READY)) == _EXIST) {
        VCB.status = 0;
        YX_SEND_AbandonATCmd();
    } else {
        VCB.status = 0;
    }
}

static void CommonProc(void (*fp)(INT8U result))
{
    VCB.fp     = fp;
    VCB.status = (_EXIST | _READY);
    //SendTskMsg(GsmTskID, MSG_ATVOICE_TSK, 0);
}

BOOLEAN AT_Q_SendHangupCmd(void (*fp)(INT8U result))
{
    AT_Q_ClearPhoneQueue();
    AT_Q_ClearSmsQueue();
    AT_Q_ClearTcpipQueue();
    
    VCB.len     = AT_CMD_Hangup(VCB.buffer, sizeof(VCB.buffer));
    VCB.cmdpara = &g_phone_hangup_para
    CommonProc(fp);
    return true;
}

BOOLEAN AT_Q_SendPickupCmd(void (*fp)(INT8U result))
{
    if (VCB.status & _EXIST) return false;
    AT_Q_ClearSmsQueue();
    AT_Q_ClearTcpipQueue();
    
    VCB.len     = AT_CMD_Pickup(VCB.buffer, sizeof(VCB.buffer));
    VCB.cmdpara = &g_phone_pickup_para;
    CommonProc(fp);
    return true;
}

BOOLEAN AT_Q_SendRingupCmd(void (*fp)(INT8U result), BOOLEAN DataMode, INT8U *telptr, INT8U tellen)
{
    //if (!SIMCardInserted() || !NetworkRegistered()) return false;
    if (tellen == 0 || tellen > 30) return false;
    if (VCB.status & _EXIST) return false;
    AT_Q_ClearSmsQueue();
    AT_Q_ClearTcpipQueue();

    VCB.len = AT_CMD_Ringup(VCB.buffer, sizeof(VCB.buffer), DataMode, telptr, tellen);
    if (DataMode) {
        VCB.cmdpara = &g_phone_ringup_data_para;
    } else {
        VCB.cmdpara = &g_phone_ringup_para;
    }
    CommonProc(fp);
    return true;
}

void AT_PHONE_Reset(void)
{
    SendVoiceCmdInformer_ATVOICE(AT_FAILURE);
}

#endif
#endif

