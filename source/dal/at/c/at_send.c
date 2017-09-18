/********************************************************************************
**
** 文件名:     at_send.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现手机模块数据发送处理
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
#include "at_com.h"
#include "at_recv.h"
#include "at_send.h"
#include "at_cmd_common.h"
#include "at_drv.h"


#if EN_AT > 0
/*
********************************************************************************
* define config parameters
********************************************************************************
*/
#define SIZE_RECVBUF          64
#define MAX_WAIT              1
#define _BASETIME             100                /* 100ms */


#define PERIOD_SCAN          _MILTICK, 1


/*
********************************************************************************
* define TCB_T
********************************************************************************
*/
typedef struct {
    BOOLEAN open;
    INT8U   sendhead;
    BOOLEAN echo;                                  /* 回显状态 */
    INT8U   status;                                /* 命令状态 */
    INT16U  overtime;                              /* 超时时间 */
    INT16U  ct_wait;                               /* 等待发送时间 */
    INT8U   nsEC;                                  /* 发送结束符个数 */
    INT8U   naEC;                                  /* 应答结束符个数 */
    INT8U   nrEC;                                  /* 当前接收结束符个数 */
    INT16U  wlen;                                  /* 当前写长度 */
    INT8U   *wptr;                                 /* 当前写位置 */
    INT16U  sendlen;                               /* 发送长度 */
    INT8U   *memptr;                               /* 动态内存 */
    INT8U   *sendptr;                              /* 发送缓冲区 */
    void   (*informer)(INT8U result);              /* 通知函数 */
    INT8U  (*handler)(INT8U  *ptr, INT16U len);    /* 处理函数 */
} TCB_T;


/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/

static INT8U s_scantmr;
static TCB_T s_tcb;
static INT8U s_recvbuf[SIZE_RECVBUF];



static void DelTCB(INT8U result)
{
    if (s_tcb.memptr != 0) {
        YX_DYM_Free(s_tcb.memptr);
        s_tcb.memptr = 0;
    }
    
    if (s_tcb.status & ATCMD_EXIST) {
        s_tcb.status &= (~ATCMD_EXIST);
        s_tcb.status |= ATCMD_WAIT;
        s_tcb.ct_wait = MAX_WAIT;
        
        if (s_tcb.informer != 0) {
            s_tcb.informer(result);
        }
    }
}

/*******************************************************************
** 函数名:     SendEntry
** 函数描述:   AT指令发送入口
** 参数:       无
** 返回:       无
********************************************************************/
static void SendEntry(void)
{
    INT16U len;
    
    if ((s_tcb.status & ATCMD_WAIT) != 0) {                                    /* 等待状态 */
        return;
    }
    
    if ((s_tcb.status & (ATCMD_EXIST | ATCMD_READY)) != (ATCMD_EXIST | ATCMD_READY)) {
        return;
    }
    
    if ((s_tcb.status & ATCMD_BREAK) != 0 && s_tcb.sendhead == 0) {            /* 分段发送 */
        s_tcb.sendhead = 1;
        len = YX_FindCharPos(s_tcb.sendptr, '\r', 0, s_tcb.sendlen) + 1;
        if (len > s_tcb.sendlen) {
            len = s_tcb.sendlen;
        }
    } else {
        len = s_tcb.sendlen;
    }

    if (AT_COM_SendData(s_tcb.sendptr, len)) {                                 /* 发送数据 */
        #if DEBUG_AT > 0
        printf_com("AT_SEND(%d):", len);
        printf_raw(s_tcb.sendptr, (len > 64) ? 64 : len);
        printf_com(">\r\n");
        if (s_tcb.sendptr[0] < 'A' || s_tcb.sendptr[1] < 'A') {
            printf_hex(s_tcb.sendptr, (len > 64) ? 64 : len);
            printf_com(">\r\n");
        }
        #endif
        
        s_tcb.sendptr += len;
        s_tcb.sendlen -= len;
        if (s_tcb.sendlen == 0) {                                              /* 发送完毕 */
            s_tcb.status &= (~ATCMD_READY);
            if (s_tcb.memptr != 0) {                                           /* 释放内存 */
                YX_DYM_Free(s_tcb.memptr);
                s_tcb.memptr = 0;
            }
            
            if ((!s_tcb.echo && s_tcb.naEC == 0) || s_tcb.nsEC == 0) {         /* 无需应答 */
                DelTCB(AT_SUCCESS);
            }
        } else {
            s_tcb.status |= ATCMD_WAIT;
            s_tcb.ct_wait = MAX_WAIT;
        }
    } else {
        ;
    }
}

/*******************************************************************
** 函数名:     SecheduleATCmd
** 函数描述:   AT指令调度
** 参数:       无
** 返回:       无
********************************************************************/
static void SecheduleATCmd(void)
{
    if (AT_Q_IsSendingSm()) {                                                  /* 正在发短信 */
        AT_Q_SmsSendEntry();
    } else if (AT_CORE_HaveHighTask()) {                                       /* 有初始化任务 */
        AT_CORE_SendEntry();
    //} else if (AT_OTHER_HaveTsk()) {
    //    AT_OTHER_Entry();
    } else if (AT_CORE_HaveLowTask()) {
        AT_CORE_SendEntry();
    } else {
        AT_Q_SetSendEntry();
        AT_Q_SmsSendEntry();
        
        #if EN_AT_PHONE > 0
        AT_Q_PhoneSendEntry();
        #endif
        
        AT_Q_TcpipSendEntry();
        AT_SOCKET_SendEntry();
    }
    
    SendEntry();
}

/*******************************************************************
** 函数名:     ScanTmrProc
** 函数描述:   定时器处理
** 参数:       [in] pdata: 定时器特征值
** 返回:       无
********************************************************************/
static void ScanTmrProc(void *pdata)
{
    OS_StartTmr(s_scantmr, PERIOD_SCAN);
    
    if ((s_tcb.status & (ATCMD_EXIST | ATCMD_READY)) == ATCMD_EXIST) {         /* 发送超时检测 */
        if (s_tcb.sendlen == 0) {                                              /* 发送完毕 */
            if (--s_tcb.overtime == 0) {
                AT_CORE_RecoveryATCmd();
                DelTCB(AT_OVERTIME);
            }
        }
    }
    
    if ((s_tcb.status & ATCMD_WAIT) != 0) {                                    /* 等待状态 */
        if (--s_tcb.ct_wait == 0) {
            s_tcb.status &= (~ATCMD_WAIT);
        }
    }
    
    SecheduleATCmd();
    
    //printf_com("<recve=-----%x,%d,%d>\r\n", s_tcb.status, s_tcb.ct_wait, s_tcb.overtime);
}

/*******************************************************************
** 函数名:     AT_SEND_Init
** 函数描述:   初始化函数
** 参数:       无
** 返回:       无
********************************************************************/
void AT_SEND_Init(void)
{
    YX_MEMSET(&s_tcb, 0, sizeof(s_tcb));
    s_tcb.echo   = 1;
    
    s_scantmr = OS_CreateTmr(TSK_ID_DAL, 0, ScanTmrProc);
}

/*******************************************************************
** 函数名:     AT_SEND_Open
** 函数描述:   打开发送通道
** 参数:       无
** 返回:       无
********************************************************************/
void AT_SEND_Open(void)
{
    if (!s_tcb.open) {
        s_tcb.open = true;
        OS_StartTmr(s_scantmr, PERIOD_SCAN);
    }
}

/*******************************************************************
** 函数名:     AT_SEND_Close
** 函数描述:   关闭发送通道
** 参数:       无
** 返回:       无
********************************************************************/
void AT_SEND_Close(void)
{
    if (s_tcb.open) {
        s_tcb.open = false;
        OS_StopTmr(s_scantmr);
        DelTCB(AT_ABANDON);
    }
}

/*******************************************************************
** 函数名:     AT_SEND_CanSendATCmd
** 函数描述:   是否可以发送指令
** 参数:       无
** 返回:       是则返回TRUE, 否则返回FALSE
********************************************************************/
BOOLEAN AT_SEND_CanSendATCmd(void)
{
    if ((s_tcb.status & (ATCMD_EXIST | ATCMD_WAIT)) != 0 || !s_tcb.open) {
        return false;
    } else {
        return true;
    }
}

/*******************************************************************
** 函数名:     AT_SEND_SendCmd
** 函数描述:   AT指令发送
** 参数:       [in] cmdpara: 命令处理结构体
**             [in] sptr:    数据指针
**             [in] slen:    数据长度
**             [in] fp:      回调函数
** 返回:       成功返回TRUE, 失败返回FALSE
********************************************************************/
BOOLEAN AT_SEND_SendCmd(AT_CMD_PARA_T const *cmdpara, INT8U *sptr, INT16U slen, void (*fp)(INT8U result))
{
    if ((s_tcb.status & ATCMD_EXIST) != 0 || !s_tcb.open) {
        return false;
    }
    
    OS_ASSERT((s_tcb.memptr == 0), RETURN_FALSE);
    
    s_tcb.memptr  = YX_DYM_Alloc(slen);
    if (s_tcb.memptr == 0) {
        #if DEBUG_AT > 0
        printf_com("<AT_SEND_SendCmd alloc memory fail(%d)>\r\n", slen);
        #endif
        
        return false;
    }
    YX_MEMCPY(s_tcb.memptr, slen, sptr, slen);
    
    s_tcb.status = cmdpara->status;
    if ((s_tcb.status & ATCMD_INSANT) != 0) {
        s_tcb.naEC     = 1;
        ATCmdAck.numEC = 0;
    } else {
        s_tcb.naEC     = cmdpara->naEC;
    }
    s_tcb.nsEC     = cmdpara->nsEC;
    s_tcb.overtime = (cmdpara->overtime * 1000) / _BASETIME;
    s_tcb.handler  = cmdpara->handler;
    
    s_tcb.status  |= (ATCMD_EXIST | ATCMD_READY);
    s_tcb.sendhead = 0;
    s_tcb.nrEC     = 0;
    s_tcb.wlen     = 0;
    s_tcb.wptr     = s_recvbuf;
    s_tcb.sendptr  = s_tcb.memptr;
    s_tcb.sendlen  = slen;
    s_tcb.informer = fp;
    
    return true;
}

/*******************************************************************
** 函数名:     YX_SEND_AbandonATCmd
** 函数描述:   放弃当前AT指令发送
** 参数:       无
** 返回:       无
********************************************************************/
void YX_SEND_AbandonATCmd(void)
{
    DelTCB(AT_ABANDON);
}

/*******************************************************************
** 函数名:     YX_SEND_SetEcho
** 函数描述:   设置回显
** 参数:       [in] open: 回显状态,TRUE-打开回显,FALSE-关闭回显
** 返回:       无
********************************************************************/
void YX_SEND_SetEcho(INT8U open)
{
    s_tcb.echo = open;
}

/*******************************************************************
** 函数名:     HdlATCmdAck
** 函数描述:   数据处理
** 参数:       [in] sptr:    数据指针
**             [in] slen:    数据长度
** 返回:       成功返回TRUE, 失败返回FALSE
********************************************************************/
BOOLEAN HdlATCmdAck(INT8U *sptr, INT16U slen)
{
    INT8U nsEC, result;
    
    if (slen <= 3) {
        return false;
    }
    
    if (!((sptr[0] >= 'A' && sptr[0] <= 'Z') || (sptr[0] >= '0' && sptr[0] <= '9') || sptr[0] == '+' || sptr[0] == '\"')) {
        return false;
    }
    
    if ((s_tcb.status & ATCMD_BREAK) == 0) {
        if ((s_tcb.status & (ATCMD_EXIST | ATCMD_READY)) != ATCMD_EXIST) {
            return false;
        }
    }

    s_tcb.nrEC++;    
    if (s_tcb.echo && s_tcb.nrEC <= s_tcb.nsEC) {                              /* 回显处理 */
        if (s_tcb.nrEC == s_tcb.nsEC && s_tcb.naEC == 0) {
            DelTCB(AT_SUCCESS);
        }
    } else {
        if ((s_tcb.status & ATCMD_INSANT) != 0) {
            ATCmdAck.numEC++;
            if (s_tcb.handler == 0) {
                DelTCB(AT_FAILURE);
            } else {
                result = s_tcb.handler(sptr, slen);
                if (result != AT_CONTINUE) {
                    DelTCB(result);
                }
            }
        } else {
            if ((s_tcb.wlen + slen) > sizeof(s_recvbuf)) {
                DelTCB(AT_FAILURE);
                return true;
            }
            
            if (s_tcb.wptr < &s_recvbuf[0] || (s_tcb.wptr + slen) > &s_recvbuf[sizeof(s_recvbuf)]) {
                OS_ASSERT(0, RETURN_FALSE);
            }
            
            YX_MEMCPY(s_tcb.wptr, slen, sptr, slen);
            s_tcb.wptr += slen;
            s_tcb.wlen += slen;
            if (s_tcb.echo) {
                nsEC = s_tcb.nsEC;
            } else {
                nsEC = 0;
            }
            
            if (s_tcb.nrEC >= (s_tcb.naEC + nsEC)) {
                if (s_tcb.handler == 0) {
                    DelTCB(AT_FAILURE);
                } else {
                    result = s_tcb.handler(s_recvbuf, s_tcb.wlen);
                    DelTCB(result);
                }
            }
        }
    }
    return true;
}

#endif

