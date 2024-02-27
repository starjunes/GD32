/********************************************************************************
**
** 文件名:     man_queue.c
** 版权所有:   (c) 2014-2020 厦门雅迅网络股份有限公司
** 文件描述:   消息队列管理模块
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2010/12/30 | LEON   | 创建本模块
**
*********************************************************************************/
#include "dal_include.h"
#include "tools.h"
#include "man_error.h"
#include "man_queue.h"
#include "appmain.h"

#define   MAN_QUEUE_G
/*************************************************************************************************/
/*                           模块宏定义                                                          */
/*************************************************************************************************/
#define INVALID_Q_ID         0xFF
#define Q_MAX_NUMS           6                                       /* 注意不能小于2 */

/*************************************************************************************************/
/*                           注册消息结构体                                                      */
/*************************************************************************************************/
typedef struct qreg_t {
    QCB_T      *qptr;
    void      (*qhdl)(void);
} QREG_T;

/*************************************************************************************************/
/*                           模块变量定义                                                        */
/*************************************************************************************************/
static INT8U    s_qmsgid;                                            /*  */
static QCB_T   *s_qmsg_list = PNULL;                                 /*  */
static QCB_T    s_qmsg_tbl[Q_MAX_NUMS];                              /*  */
static QREG_T   s_QRegTbl[Q_MAX_NUMS];                               /*  */
//QCB_T   *g_appQ;

/*******************************************************************
** 函数名:     MsgIsExist
** 函数描述:   消息是否已经存在
** 参数:       无
** 返回:       无
********************************************************************/
static BOOLEAN MsgIsExist(QCB_T *qptr, INT16U msgid)
{
    INT8U  i;
    QMSG_T *pmsg;
    
    pmsg = qptr->qout;
    
    for (i = 0; i < qptr->qentries; i++) {
        if (pmsg->msgid == msgid) {
            return true;
        }
        if (++pmsg == qptr->qend) {
            pmsg = qptr->qstart;
        }
    }
    
    return false;
}

/*******************************************************************
** 函数名:     QCreate
** 函数描述:   创建一个消息队列
** 参数:       无
** 返回:       无
********************************************************************/
QCB_T *QCreate(QMSG_T *new, INT16U size)
{
    QCB_T *qptr;
    
    if ((new == PNULL) || (size == 0)) {
        return PNULL;
    }
    
    ENTER_CRITICAL();
    qptr = s_qmsg_list;
    
    if (s_qmsg_list != PNULL) {
        s_qmsg_list = s_qmsg_list->qptr;    
    }
    EXIT_CRITICAL();
    
    if (qptr != PNULL)  {
        qptr->qstart   = new;
        qptr->qend     = &new[size];
        qptr->qin      = new;
        qptr->qout     = new;
        qptr->qsize    = size;
        qptr->qentries = 0;
    } else {
        return PNULL;
    }
    
    return (qptr);
}

/*******************************************************************
** 函数名:     QRegister
** 函数描述:   注册一个消息队列
** 参数:       无
** 返回:       无
********************************************************************/
BOOLEAN QRegister(INT8U qid, QCB_T *qptr, void (*handle)(void))
{
    INT8U i;
    
    if (qid >= Q_MAX_NUMS) {
        return false;
    }
    if (qptr == PNULL) {
        return false;
    }
    if (handle == PNULL) {
        return false;
    }
    if (s_QRegTbl[qid].qptr != PNULL) {
        return false;
    }
    if (s_QRegTbl[qid].qhdl != PNULL) {
        return false;
    }
    for (i = 0; i < Q_MAX_NUMS; i++) {
        if (s_QRegTbl[i].qptr == qptr) {
            return false;
        }
        if (s_QRegTbl[i].qhdl == handle) {
            return false;
        }
    }
    
    s_QRegTbl[qid].qptr = qptr;
    s_QRegTbl[qid].qhdl = handle;

    return true;
}

/*******************************************************************
** 函数名:     QMsgPost
** 函数描述:   向指定的消息控制块投递一个消息
** 参数:       无
** 返回:       无
********************************************************************/
Q_ERR_E QMsgPost(QCB_T *qptr, INT16U msgid, INT32U lmsgpara, INT32U hmsgpara)
{
    if (qptr == PNULL) {
        return Q_PQ_ERR;
    }
    ENTER_CRITICAL();
    
    if (qptr->qentries >= qptr->qsize) {
        EXIT_CRITICAL();
        return Q_FULL_ERR;
    }
    
    qptr->qin->msgid    = msgid;
    qptr->qin->lmsgpara = lmsgpara;
    qptr->qin->hmsgpara = hmsgpara;
    qptr->qin++;
    qptr->qentries++;
     
    if (qptr->qin == qptr->qend) {
        qptr->qin = qptr->qstart;    
    }

    EXIT_CRITICAL();
    return Q_NO_ERR;
}

/*******************************************************************
** 函数名:     cQMsgPost
** 函数描述:   向指定的消息控制块投递一个消息 - 如果该消息已经存在，则不进行投递
** 参数:       无
** 返回:       无
********************************************************************/
Q_ERR_E cQMsgPost(QCB_T *qptr, INT16U msgid, INT32U lmsgpara, INT32U hmsgpara)
{
    if (qptr == PNULL) {
        return Q_PQ_ERR;
    }
    if (true == MsgIsExist(qptr, msgid)) {
        return Q_OVERLAY_ERR;
    }
    
    return QMsgPost(qptr, msgid, lmsgpara, hmsgpara);
}

/********************************************************************************
** 函数名:     OS_PostMsg
** 函数描述:   发送系统消息
** 参数:       [in] msgid：系统消息ID
**             [in] lpara：低32位消息参数
**             [in] hpara：高32位消息参数
** 返回:       无
********************************************************************************/
void OS_PostMsg(INT8U tskid, INT32U msgid, INT32U lpara, INT32U hpara)
{
    QMsgPost(g_appQ, msgid, lpara, hpara);
}

/********************************************************************************
** 函数名:     OS_APostMsg
** 函数描述:   发送系统消息
** 参数:       [in] overlay：TRUE：如在消息队列中存在相同的消息ID, 则不将新的消息存放在消息队列中
**                           FALSE：新的消息必须存放在消息队列中
**             [in] tskid:   消息所属任务ID
**             [in] msgid:   系统消息ID
**             [in] lpara:   低32位消息参数
**             [in] hpara:   高32位消息参数
** 返回:       无
********************************************************************************/
void OS_APostMsg(BOOLEAN overlay, INT8U tskid, INT32U msgid, INT32U lpara, INT32U hpara)
{
    if (overlay) {
        if (!MsgIsExist(g_appQ, msgid)) {
            OS_PostMsg(tskid, msgid, lpara, hpara);
        }
    } else {
        OS_PostMsg(tskid, msgid, lpara, hpara);
    }
}

/*******************************************************************
** 函数名:     QMsgAccept
** 函数描述:   处理一个消息
** 参数:       无
** 返回:       无
********************************************************************/
Q_ERR_E QMsgAccept(QCB_T *qptr, QMSG_T *msg)
{
    if (qptr == PNULL) {
        return Q_PQ_ERR;
    }
    
    ENTER_CRITICAL();
    if (qptr->qentries == 0) {
        EXIT_CRITICAL();
        return Q_EMPTY_ERR;
    }
    
    *msg = *qptr->qout++;
    qptr->qentries--;
     
    if (qptr->qout == qptr->qend) {
        qptr->qout = qptr->qstart;
    }
    
    EXIT_CRITICAL();
    return Q_NO_ERR;
}

/*******************************************************************
** 函数名:     InitMsgQman
** 函数描述:   消息队列管理模块初始化
** 参数:       无
** 返回:       无
********************************************************************/
void InitMsgQman(void)
{
    INT16U i;
    QCB_T *pq1, *pq2;

    s_qmsgid = INVALID_Q_ID;
    memset(s_QRegTbl, 0, sizeof(s_QRegTbl));
    
    pq1 = &s_qmsg_tbl[0];
    pq2 = &s_qmsg_tbl[1];
    
    for (i = 0; i < (Q_MAX_NUMS - 1); i++) {                         /* Init list of free control blocks */
        pq1->qptr = pq2;
        pq1++;
        pq2++;
    }
    
    pq1->qptr = (QCB_T *)0;
    s_qmsg_list = &s_qmsg_tbl[0];
}

/*******************************************************************
** 函数名:     QMsgDispatch
** 函数描述:   消息调度
** 参数:       无
** 返回:       无
********************************************************************/
void QMsgDispatch(void)
{
    INT8U i; 
    QCB_T *curpq;

    if (s_qmsgid != INVALID_Q_ID) {
        curpq = s_QRegTbl[s_qmsgid].qptr;
        DAL_ASSERT(curpq != PNULL);
        DAL_ASSERT(s_QRegTbl[s_qmsgid].qhdl != PNULL);
        if (curpq->qentries > 0) {
            s_QRegTbl[s_qmsgid].qhdl();
            return;
        } else {
            s_qmsgid = INVALID_Q_ID;
        }
    }
    
    for (i = 0; i < Q_MAX_NUMS; i++) {
        if (s_QRegTbl[i].qptr == PNULL) continue;
        DAL_ASSERT(s_QRegTbl[i].qhdl != PNULL);
        if (s_QRegTbl[i].qptr->qentries > 0) {
            s_qmsgid = i;
            s_QRegTbl[s_qmsgid].qhdl();
            return;
        }
    }
}

