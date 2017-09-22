/********************************************************************************
**
** 文件名:     at_q_set.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现手机模块通用设置指令发送队列管理
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
#include "at_core.h"
#include "at_cmd_common.h"
#include "at_q_sms.h"
#include "at_q_set.h"

#if EN_AT > 0

/*
********************************************************************************
* 定义模块配置参数
********************************************************************************
*/
#define NUM_CMD                             2
#define SIZE_BUF                            64

/*
********************************************************************************
* define CELL_T
********************************************************************************
*/
typedef struct {
    INT8U   len;
    INT8U   memlen;
    INT8U  *memptr;
    void    (*fp)(INT8U result);
    AT_CMD_PARA_T const *cmdpara;
} CELL_T;

/*
********************************************************************************
* define module variants
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
        printf_com("<set AllocCell malloc memory fail>\r\n");
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
    void (*fp)(INT8U result);

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
** 函数名:     AT_Q_InitSet
** 函数描述:   初始化模块
** 参数:       无
** 返回:       无
********************************************************************/
void AT_Q_InitSet(void)
{
    s_curscb = 0;
    YX_LIST_CreateList(&s_freelist, (INT8U *)memblock, sizeof(memblock) / sizeof(memblock[0]), sizeof(memblock[0]));
    YX_LIST_Init(&s_usedlist);
    //OS_InstallDiag(DiagnoseProc_ATSET);
}

/*******************************************************************
** 函数名:     AT_Q_SetSendEntry
** 函数描述:   发送入口
** 参数:       无
** 返回:       无
********************************************************************/
void AT_Q_SetSendEntry(void)
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
** 函数名:     AT_Q_ClearSetQueue
** 函数描述:   清除设置指令队列
** 参数:       无
** 返回:       无
********************************************************************/
void AT_Q_ClearSetQueue(void)
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


BOOLEAN QueryPhoneBook(INT8U index1, INT8U index2, void (*fp)(INT8U result))
{
    CELL_T  *cell;
    
    //if (SIMCardBusy()) return false;
    //if (GetPhoneStatus() != PHONE_FREE) return false;
    if ((cell = AllocCell()) == 0) return false;

    cell->len     = AT_CPBR(cell->memptr, cell->memlen, index1, index2);
    cell->cmdpara = &AT_CPBR_PARA;
    cell->fp      = fp;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;
}

BOOLEAN WritePhoneBook(INT8U index, INT8U *tel, INT8U tellen, INT8U *text, INT8U textlen)
{
    CELL_T  *cell;
    
    //if (SIMCardBusy()) return false;
    //if (GetPhoneStatus() != PHONE_FREE) return false;
    if ((cell = AllocCell()) == 0) return false;

    cell->len     = AT_CPBW(cell->memptr, cell->memlen, index, tel, tellen, text, textlen);
    cell->cmdpara = &AT_CPBW_PARA;
    cell->fp      = 0;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;
}

BOOLEAN ReadRealTimeClock(void (*fp)(INT8U result))
{
    CELL_T  *cell;
    
    //if (GetPhoneStatus() != PHONE_FREE) return false;
    if ((cell = AllocCell()) == 0) return false;

    cell->len     = AT_R_CCLK(cell->memptr, cell->memlen);
    cell->cmdpara = &AT_R_CCLK_PARA;
    cell->fp      = fp;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;
}

BOOLEAN SetRealTimeClock(DATE_T *date, TIME_T *time, void (*fp)(INT8U result))
{
    CELL_T *cell;
    
    //if (GetPhoneStatus() != PHONE_FREE) return false;
    if ((cell = AllocCell()) == 0) return false;

    cell->len     = AT_CCLK(cell->memptr, cell->memlen, date, time);
    cell->cmdpara = &AT_CCLK_PARA;
    cell->fp      = fp;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;    
}

/*******************************************************************
** 函数名:     AT_Q_ListSm
** 函数描述:   列取所有短信
** 参数:       无
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN AT_Q_ListSm(void (*fp)(INT8U result))
{
    CELL_T *cell;

    if ((cell = AllocCell()) == 0) {
        return false;
    }
    cell->len     = AT_CMD_ListSm(cell->memptr, cell->memlen);
    cell->cmdpara = &g_sms_list_para;
    cell->fp      = fp;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;
}

/*******************************************************************
** 函数名:     AT_Q_ReadSmByIndex
** 函数描述:   根据索引号读取短信
** 参数:       [in] index：索引号
**             [in] fp：   回调函数
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN AT_Q_ReadSmByIndex(INT16U index, void (*fp)(INT8U result))
{
    CELL_T *cell;

    if ((cell = AllocCell()) == 0) {
        return false;
    }
    cell->len     = AT_CMD_ReadSm(cell->memptr, cell->memlen, index);
    cell->cmdpara = &g_sms_read_para;
    cell->fp      = fp;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;
}

/*******************************************************************
** 函数名:     AT_Q_DelSmByIndex
** 函数描述:   根根据索引号删除短信
** 参数:       [in] index：索引号
**             [in] fp：   回调函数
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN AT_Q_DelSmByIndex(INT16U index, void (*fp)(INT8U result))
{
    CELL_T *cell;

    if ((cell = AllocCell()) == 0) {
        return false;
    }
    cell->len     = AT_CMD_DeleteSm(cell->memptr, cell->memlen, index);
    cell->cmdpara = &g_sms_delete_para;
    cell->fp      = fp;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;
}

/*******************************************************************
** 函数名:     AT_Q_SetIMEI
** 函数描述:   设置IMEI
** 参数:       [in] imei：   IMEI
**             [in] imeilen：IMEI长度
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN AT_Q_SetIMEI(INT8U *imei, INT8U imeilen)
{
    CELL_T  *cell;

    if ((cell = AllocCell()) == 0) {
        return false;
    }
    cell->len     = AT_IMEI(cell->memptr, cell->memlen, imei, imeilen);
    cell->cmdpara = &AT_IMEI_PARA;
    cell->fp      = 0;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;
}

/*******************************************************************
** 函数名:     AT_Q_SetCfun
** 函数描述:   设置CFUN
** 参数:       [in] fun：  功能号
**             [in] fp：   回调函数
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN AT_Q_SetCfun(INT8U fun, void (*fp)(INT8U result))
{
    CELL_T *cell;

    if ((cell = AllocCell()) == 0) {
        return false;
    }
    cell->len     = AT_CFUN(cell->memptr, cell->memlen, fun);
    cell->cmdpara = &AT_CFUN_PARA;
    cell->fp      = fp;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;
}

/*******************************************************************
** 函数名:     AT_Q_GetSmscTel
** 函数描述:   获取短信服务中心号码
** 参数:       [in] index：索引号
**             [in] fp：   回调函数
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN AT_Q_GetSmscTel(void (*fp)(INT8U result))
{
    CELL_T *cell;

    if ((cell = AllocCell()) == 0) {
        return false;
    }
    cell->len     = AT_CMD_GetSmscTel(cell->memptr, cell->memlen);
    cell->cmdpara = &g_sms_getcsca_para;
    cell->fp      = fp;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;
}

/*******************************************************************
** 函数名:     AT_Q_SetSmscTel
** 函数描述:   设置短信服务中心号码
** 参数:       [in] tel:    号码指针, ASCII码
**             [in] tellen: 号码长度
**             [in] fp：    回调函数
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN AT_Q_SetSmscTel(INT8U *tel, INT8U tellen, void (*fp)(INT8U result))
{
    CELL_T *cell;

    if (tellen > 30 || tellen == 0) {
        return false;
    }
    
    if ((cell = AllocCell()) == 0) {
        return false;
    }
    cell->len     = AT_CMD_SetSmscTel(cell->memptr, cell->memlen, tel, tellen);
    cell->cmdpara = &g_sms_setcsca_para;
    cell->fp      = fp;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;
}

/*******************************************************************
** 函数名:     SetQsclk
** 函数描述:   设置省电工作模式
** 参数:       [in] enable: 1-省电使能，0-正常工作模式
**             [in] fp：    回调函数
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN SetQsclk(INT8U enable, void (*fp)(INT8U result))
{
    CELL_T *cell;

    if ((cell = AllocCell()) == 0) return false;
    cell->len     = AT_QSCLK(cell->memptr, cell->memlen, enable);
    cell->cmdpara = &AT_QSCLK_PARA;
    cell->fp      = fp;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;
}

/*******************************************************************
** 函数名:     SetQeng
** 函数描述:   设置基站信息
** 参数:       [in] enable: 1-开启，0-关闭
**             [in] fp：    回调函数
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN SetQeng(INT8U enable, void (*fp)(INT8U result))
{
    CELL_T *cell;

    if ((cell = AllocCell()) == 0) return false;
    cell->len     = AT_SET_QENG(cell->memptr, cell->memlen, enable);
    cell->cmdpara = &AT_SET_QENG_PARA;
    cell->fp      = fp;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;
}

/*******************************************************************
** 函数名:     QueryQeng
** 函数描述:   查询基站信息
** 参数:       [in] fp：    回调函数
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN QueryQeng(void (*fp)(INT8U result))
{
    CELL_T *cell;

    if ((cell = AllocCell()) == 0) return false;
    cell->len     = AT_Q_QENG(cell->memptr, cell->memlen);
    cell->cmdpara = &AT_Q_QENG_PARA;
    cell->fp      = fp;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;
}

#endif

