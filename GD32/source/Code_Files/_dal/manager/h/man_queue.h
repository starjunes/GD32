/********************************************************************************
**
** 文件名:     man_queue.h
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
#ifndef __MAN_QUEUE_H
#define __MAN_QUEUE_H

#ifdef __cplusplus
extern "C"
{
#endif


#include "msg_id.h"

/*************************************************************************************************/
/*                           定义消息队列结构体                                                  */
/*************************************************************************************************/
typedef struct qmsg_t {
    INT16U         msgid;                        /* Message ID */
    INT32U         lmsgpara;                     /* LOW Word  parameter */
    INT32U         hmsgpara;                     /* HIGH Word parameter */
} QMSG_T;

/*************************************************************************************************/
/*                           定义消息管理控制块                                                  */
/*************************************************************************************************/
typedef struct qcb_t {
    struct qcb_t  *qptr;                         /* Link to next block in list of free blocks */
    QMSG_T        *qstart;                       /* Pointer to start of queue data */
    QMSG_T        *qend;                         /* Pointer to end   of queue data */
    QMSG_T        *qin;                          /* Pointer to where next message will be inserted  */
    QMSG_T        *qout;                         /* Pointer to where next message will be extracted */
    INT16U         qsize;                        /* Size of queue (maximum number of entries) */
    INT16U         qentries;                     /* Current number of entries in the queue */
} QCB_T;

/*************************************************************************************************/
/*                           消息处理出错代码                                                    */
/*************************************************************************************************/
typedef enum {
    Q_NO_ERR,                                    /* Queue no error */
    Q_FULL_ERR,                                  /* Queue is full */
    Q_PQ_ERR,                                    /* Queue pointer errror */
    Q_EMPTY_ERR,                                 /* Queue is empty */
    Q_OVERLAY_ERR                                /* Queue message is overlay */
} Q_ERR_E;

#ifdef  MAN_QUEUE_G
#define MAN_QUEUE_EXT     
#else
#define MAN_QUEUE_EXT  extern
#endif

//MAN_QUEUE_EXT  QCB_T   *g_appQ;

QCB_T  *QCreate(QMSG_T *start, INT16U size);
BOOLEAN QRegister(INT8U qid, QCB_T *pq,void (*handle)(void));
Q_ERR_E  QMsgPost(QCB_T *pq,INT16U msgid, INT32U lmsgpara, INT32U hmsgpara);
Q_ERR_E  cQMsgPost(QCB_T *pq,INT16U msgid, INT32U lmsgpara, INT32U hmsgpara);
void OS_PostMsg(INT8U tskid, INT32U msgid, INT32U lpara, INT32U hpara);
void OS_APostMsg(BOOLEAN overlay, INT8U tskid, INT32U msgid, INT32U lpara, INT32U hpara);
Q_ERR_E QMsgAccept(QCB_T *pq,QMSG_T *msg);
void InitMsgQman(void);
void QMsgDispatch(void);

#ifdef __cplusplus
}
#endif

#endif

