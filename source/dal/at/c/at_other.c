/****************************************************************
**                                                              *
**  FILE         :  AT_SET.C                                    *
**  COPYRIGHT    :  (c) 2001 .Xiamen Yaxon NetWork CO.LTD       *
**                                                              *
**                                                              *
**  By : CCH 2003.11.2                                          *
****************************************************************/
#include "yx_include.h"
#include "yx_misc.h"
#include "yx_list.h"
#include "at_recv.h"
#include "at_send.h"
#include "at_cmd_common.h"
#include "at_q_sms.h"
#include "at_core.h"
#include "at_q_set.h"

#if EN_AT > 0
#if 0
/*
********************************************************************************
* define config parameters
********************************************************************************
*/
#define NUM_CMD                             5
#define SIZE_BUF                            256

/*
********************************************************************************
* define type
********************************************************************************
*/
#define CMD_GPRS                            0
#define CMD_OTHER                           1

/*
********************************************************************************
* define SCB_STRUCT
********************************************************************************
*/
typedef struct {
    INT8U   type;
    INT8U   len;
    INT8U   buffer[SIZE_BUF];
    void    (*fp)(INT8U result);
    AT_CMD_PARA_T const *cmdpara;
} SCB_STRUCT;

/*
********************************************************************************
* define module variants
********************************************************************************
*/
static struct {
    NODE_T        next;
    SCB_STRUCT  command;
} memblock[NUM_CMD];

static LIST_T s_freelist, s_usedlist;
static SCB_STRUCT  *s_curscb;



static SCB_STRUCT *AllocSetCmd(void)
{
    return (SCB_STRUCT *)YX_LIST_DeleListHead(&s_freelist);
}

static void StoreSetCmd(SCB_STRUCT *scb, void (*fp)(INT8U))
{
    scb->fp = fp;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)scb);
    //SendTskMsg(GsmTskID, MSG_ATSET_TSK, 0);
}

static void SendSetCmdInformer_ATSET(INT8U result)
{
    void (*fp)(INT8U);

    if (s_curscb != 0) {
        fp = s_curscb->fp;
        YX_LIST_AppendListEle(&s_freelist, (INT8U *)s_curscb);
        s_curscb = 0;
        //SendTskMsg(GsmTskID, MSG_ATSET_TSK, 0);
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

/*******************************************************************
** 函数名:     AT_OTHER_Init
** 函数描述:   AT指令初始化
** 参数:      无
** 返回:       无
********************************************************************/
void AT_OTHER_Init(void)
{
    s_curscb = 0;
    YX_LIST_CreateList(&s_freelist, (INT8U *)memblock, sizeof(memblock) / sizeof(memblock[0]), sizeof(memblock[0]));
    YX_LIST_Init(&s_usedlist);
    //OS_InstallDiag(DiagnoseProc_ATSET);
}

/*******************************************************************
** 函数名:     AT_OTHER_Entry
** 函数描述:   AT指令执行入口
** 参数:      无
** 返回:       无
********************************************************************/
void AT_OTHER_Entry(void)
{
    if (s_curscb != 0 || YX_LIST_GetNodeNum(&s_usedlist) == 0) {
        return;
    }
    
    if (!AT_SEND_CanSendATCmd()) {
        return;
    }
    
    s_curscb = (SCB_STRUCT  *)YX_LIST_DeleListHead(&s_usedlist);
    AT_SEND_SendCmd(s_curscb->cmdpara, s_curscb->buffer, s_curscb->len, SendSetCmdInformer_ATSET);
}

/*******************************************************************
** 函数名:     AT_OTHER_HaveTsk
** 函数描述:   AT指令执行入口
** 参数:      无
** 返回:       有AT指令返回true，无返回false
********************************************************************/
BOOLEAN AT_OTHER_HaveTsk(void)
{
    if (s_curscb != 0 || YX_LIST_GetNodeNum(&s_usedlist) > 0) {
        return true;
    } else {
        return false;
    }
}

/*******************************************************************
** 函数名:     AT_OTHER_PlayTts
** 函数描述:   启动TTS播放
** 参数:       [in] fp：       回调函数
**             [in] sptr：     数据指针
**             [in] len：      数据长度
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN AT_OTHER_PlayTts(void (*fp)(INT8U), INT8U *sptr, INT16U len)
{
    SCB_STRUCT  *scb;

    if ((scb = AllocSetCmd()) == 0) return false;
    scb->len     = AT_CTTS(scb->buffer, sizeof(scb->buffer), sptr, len);
    scb->cmdpara = &AT_CTTS_PARA;
    scb->type    = CMD_OTHER;
    StoreSetCmd(scb, fp);
    return true;
}

/*******************************************************************
** 函数名:     AT_OTHER_SetTtsPara
** 函数描述:   设置TTS播放参数
** 参数:       [in] fp：       回调函数
**             [in] volume：   音量
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN AT_OTHER_SetTtsPara(void (*fp)(INT8U), INT8U volume)
{
    SCB_STRUCT  *scb;

    if ((scb = AllocSetCmd()) == 0) return false;
    scb->len     = AT_CTTSPARAM(scb->buffer, sizeof(scb->buffer), volume);
    scb->cmdpara = &AT_CTTSPARAM_PARA;
    scb->type    = CMD_OTHER;
    StoreSetCmd(scb, fp);
    return true;
}
#endif

#endif

