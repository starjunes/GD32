/********************************************************************************
**
** 文件名:     at_q_phone.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现手机模块电话指令发送队列管理
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
#include "at_recv.h"
#include "at_send.h"
#include "at_cmd_common.h"
#include "at_q_sms.h"
#include "at_q_tcpip.h"
#include "at_q_set.h"
#include "at_q_phone.h"
#include "at_core.h"


#if EN_AT > 0
#if EN_AT_PHONE > 0
/*
********************************************************************************
* 定义模块配置参数
********************************************************************************
*/
#define NUM_CMD                             2
#define SIZE_BUF                            64

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    INT8U  len;
    INT8U  memlen;
    INT8U *memptr;
    void (*fp)(INT8U result);
    AT_CMD_PARA_T const *cmdpara;
} CELL_T;

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
static struct {
    NODE_T next;
    CELL_T command;
} memblock[NUM_CMD];

static LIST_T s_freelist, s_usedlist;
static CELL_T  *s_curscb;


/*******************************************************************
** 函数名:     AllocCell
** 函数描述:   申请链表节点
** 参数:       无
** 返回:       返回链表节点指针
********************************************************************/
static CELL_T *AllocCell(void)
{
    CELL_T *cell;
    
    cell = (CELL_T *)YX_LIST_DeleListHead(&s_freelist);
    if (cell == 0) {
        return 0;
    }
    YX_MEMSET((INT8U *)cell, 0, sizeof(CELL_T));
    
    cell->memlen = SIZE_BUF;
    cell->memptr = YX_DYM_Alloc(cell->memlen);
    if (cell->memptr == 0) {
        #if DEBUG_ERR > 0
        printf_com("<phone AllocCell malloc memory fail>\r\n");
        #endif
        YX_LIST_AppendListEle(&s_freelist, (INT8U *)cell);
        return 0;
    }
    return cell;
}

/*******************************************************************
** 函数名:     Callback_Send
** 函数描述:   发送结果回调
** 参数:       [in] result: 发送结果
** 返回:       无
********************************************************************/
static void Callback_Send(INT8U result)
{
    void (*fp)(INT8U);

    if (s_curscb != 0) {
        fp = s_curscb->fp;
        if (s_curscb->memptr != 0) {
            YX_DYM_Free(s_curscb->memptr);
            s_curscb->memptr = 0;
        }
        
        YX_LIST_AppendListEle(&s_freelist, (INT8U *)s_curscb);
        s_curscb = 0;
        
        if (fp != 0) {
            if (result == AT_SUCCESS) {
                result = _SUCCESS;
            } else {
                result = _FAILURE;
            }
            fp(result);
        }
    }
}

#if 0
void DiagnoseProc_ATSET(void)
{
    INT8U count;
    
    //if (!CheckList(&s_freelist) || !CheckList(&s_usedlist)) 
    //    ErrExit(ERR_ATSET_MEM);

   count = YX_LIST_GetNodeNum(&s_freelist) + YX_LIST_GetNodeNum(&s_usedlist);
   if (s_curscb != 0) count++;
   OS_ASSERT((count == sizeof(memblock) / sizeof(memblock[0])), RETURN_VOID);
}
#endif

/*******************************************************************
** 函数名:     AT_Q_InitPhone
** 函数描述:   初始化模块
** 参数:       无
** 返回:       无
********************************************************************/
void AT_Q_InitPhone(void)
{
    s_curscb = 0;
    YX_LIST_CreateList(&s_freelist, (INT8U *)memblock, sizeof(memblock) / sizeof(memblock[0]), sizeof(memblock[0]));
    YX_LIST_Init(&s_usedlist);
    //OS_InstallDiag(DiagnoseProc_ATSET);
}

/*******************************************************************
** 函数名:     AT_Q_PhoneSendEntry
** 函数描述:   发送入口
** 参数:       无
** 返回:       无
********************************************************************/
void AT_Q_PhoneSendEntry(void)
{
    if (s_curscb != 0 || YX_LIST_GetNodeNum(&s_usedlist) == 0) {
        return;
    }
    
    if (!AT_SEND_CanSendATCmd()) {
        return;
    }
    
    s_curscb = (CELL_T  *)YX_LIST_DeleListHead(&s_usedlist);
    AT_SEND_SendCmd(s_curscb->cmdpara, s_curscb->memptr, s_curscb->len, Callback_Send);
}

/*******************************************************************
** 函数名:     AT_Q_ClearPhoneQueue
** 函数描述:   清除电话指令队列
** 参数:       无
** 返回:       无
********************************************************************/
void AT_Q_ClearPhoneQueue(void)
{
    CELL_T *cell;
    CELL_T *tmp;
    
    if (s_curscb != 0) {
        YX_SEND_AbandonATCmd();                                                /* 放弃当前指令 */
        Callback_Send(_FAILURE);
    }

    cell = (CELL_T  *)YX_LIST_GetListHead(&s_usedlist);
    for (;;) {
        if (cell == 0) {
            break;
        }
        
        tmp = cell;
        if (cell->memptr != 0) {
            YX_DYM_Free(cell->memptr);
            cell->memptr = 0;
        }
            
        cell = (CELL_T  *)YX_LIST_DeleListEle(&s_usedlist, (INT8U *)cell);
        YX_LIST_AppendListEle(&s_freelist, (INT8U *)tmp);
    }
}

/*******************************************************************
** 函数名:     AT_Q_SendHangupCmd
** 函数描述:   发送挂机指令
** 参数:       [in] fp：       回调函数
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN AT_Q_SendHangupCmd(void (*fp)(INT8U result))
{
    CELL_T *cell;

    if ((cell = AllocCell()) == 0) {
        return false;
    }
    
    AT_Q_ClearPhoneQueue();
    AT_Q_ClearSmsQueue();
    AT_Q_ClearTcpipQueue();
    
    cell->len     = AT_CMD_Hangup(cell->memptr, cell->memlen);
    cell->cmdpara = &g_phone_hangup_para;
    cell->fp      = fp;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;
}

/*******************************************************************
** 函数名:     AT_Q_SendPickupCmd
** 函数描述:   发送摘机指令
** 参数:       [in] fp：       回调函数
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN AT_Q_SendPickupCmd(void (*fp)(INT8U result))
{
    CELL_T *cell;

    if ((cell = AllocCell()) == 0) {
        return false;
    }
    
    AT_Q_ClearSmsQueue();
    AT_Q_ClearTcpipQueue();
    
    cell->len     = AT_CMD_Pickup(cell->memptr, cell->memlen);
    cell->cmdpara = &g_phone_pickup_para;
    cell->fp      = fp;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;
}

/*******************************************************************
** 函数名:     AT_Q_SendRingupCmd
** 函数描述:   发送拨打电话指令
** 参数:       [in] fp：       回调函数
**             [in] datamode： 拨号模式,TRUE-数据拨号,FASLE-常规电话
**             [in] tel：      电话号码
**             [in] tellen：   电话号码长度
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN AT_Q_SendRingupCmd(void (*fp)(INT8U result), BOOLEAN datamode, INT8U *tel, INT8U tellen)
{
    CELL_T *cell;
    
    //if (!SIMCardInserted() || !NetworkRegistered()) return false;
    
    if (tel == 0 || tellen == 0 || tellen > 30) {
        return false;
    }
    
    if ((cell = AllocCell()) == 0) {
        return false;
    }
    
    AT_Q_ClearSmsQueue();
    AT_Q_ClearTcpipQueue();
    
    cell->len     = AT_CMD_Ringup(cell->memptr, cell->memlen, datamode, tel, tellen);
    if (datamode) {
        cell->cmdpara = &g_phone_ringup_data_para;
    } else {
        cell->cmdpara = &g_phone_ringup_para;
    }
    cell->fp      = fp;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;
}

/*******************************************************************
** 函数名:     AT_Q_SendDtmf
** 函数描述:   发送拨打电话指令
** 参数:       [in] dtmf：     DTMF字符
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN AT_Q_SendDtmf(char dtmf)
{
    CELL_T  *cell;

    if ((cell = AllocCell()) == 0) {
        return false;
    }
    
    cell->len     = AT_CMD_SendDtmf(cell->memptr, cell->memlen, dtmf);
    cell->cmdpara = &g_phone_dtmf_para;
    cell->fp      = 0;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;
}

/*******************************************************************
** 函数名:     AT_Q_SendDtmf
** 函数描述:   发送拨打电话指令
** 参数:       [in] dtmf：     DTMF字符
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN AT_Q_QueryPhoneStatus(void (*fp)(INT8U result))
{
    CELL_T  *cell;
    
    if ((cell = AllocCell()) == 0) {
        return false;
    }
    cell->len     = AT_CMD_QueryPhoneStatus(cell->memptr, cell->memlen);
    cell->cmdpara = &g_phone_query_para;
    cell->fp      = fp;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;
}

/*******************************************************************
** 函数名:     AT_Q_SelectAudioChannel
** 函数描述:   选择话音通道
** 参数:       [in] ch: 通道号，见AUDIO_CHANNEL_E
** 返回:       成功或失败
********************************************************************/
BOOLEAN AT_Q_SelectAudioChannel(INT8U ch)
{
    CELL_T *cell;

    if ((cell = AllocCell()) == 0) {
        return false;
    }
    cell->len     = AT_CMD_SelectChannel(cell->memptr, cell->memlen, ch);
    cell->cmdpara = &g_phone_setchannel_para;
    cell->fp      = 0;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;
}

/*******************************************************************
** 函数名:     AT_Q_SetSpeakerLevel
** 函数描述:   设置喇叭音量
** 参数:       [in] ch:    通道号，见AUDIO_CHANNEL_E
**             [in] gain:  喇叭音量等级
** 返回:       成功或失败
********************************************************************/
BOOLEAN AT_Q_SetSpeakerLevel(INT8U ch, INT8U gain)
{
    CELL_T *cell;

    if ((cell = AllocCell()) == 0) {
        return false;
    }
    cell->len     = AT_CMD_SetSpkLevel(cell->memptr, cell->memlen, ch, gain);
    cell->cmdpara = &g_phone_setspeaker_para;
    cell->fp      = 0;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;
}

/*******************************************************************
** 函数名:     AT_Q_SetMicLevel
** 函数描述:   设置Mic增益
** 参数:       [in] ch:   通道号，见AUDIO_CHANNEL_E
**             [in] gain: MIC增益
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN AT_Q_SetMicLevel(INT8U ch, INT32U gain)
{
    CELL_T *cell;

    if ((cell = AllocCell()) == 0) {
        return false;
    }
    cell->len     = AT_CMD_SetMicLevel(cell->memptr, cell->memlen, ch, gain);
    cell->cmdpara = &g_phone_setmic_para;
    cell->fp      = 0;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;
}

/*******************************************************************
** 函数名:     AT_Q_SetEchoCancel
** 函数描述:   设置回音抑制参数
** 参数:       [in] ch:   通道号，见AUDIO_CHANNEL_E
**             [in] gain: 回音抑制参数
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN AT_Q_SetEchoCancel(INT8U ch, INT32U echo)
{
    CELL_T *cell;

    if ((cell = AllocCell()) == 0) {
        return false;
    }
    cell->len     = AT_CMD_SetEchoCancel(cell->memptr, cell->memlen, ch, echo);
    cell->cmdpara = &g_phone_setecho_para;
    cell->fp      = 0;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;
}

/*******************************************************************
** 函数名:     AT_Q_SetSideTone
** 函数描述:   设置侧音增益参数
** 参数:       [in] ch:   通道号，见AUDIO_CHANNEL_E
**             [in] gain: 参数
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN AT_Q_SetSideTone(INT8U ch, INT32U gain)
{
    CELL_T *cell;

    if ((cell = AllocCell()) == 0) {
        return false;
    }
    cell->len     = AT_CMD_SetSideTone(cell->memptr, cell->memlen, ch, gain);
    cell->cmdpara = &g_phone_setsidetone_para;
    cell->fp      = 0;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;
}



#endif
#endif

