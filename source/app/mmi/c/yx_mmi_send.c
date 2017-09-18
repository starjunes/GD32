/********************************************************************************
**
** 文件名:     yx_mmi_send.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现MMI外设协议封装发送，发送链表管理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2011/01/22 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "yx_include.h"
#include "yx_dym_drv.h"
#include "yx_list.h"
#include "yx_debug.h"

#if EN_MMI > 0
#include "yx_mmi_drv.h"



/*
********************************************************************************
* 宏定义
********************************************************************************
*/
#define NUM_MEM              8
#define MAX_SIZE             512       /* 数据长度限制 */
#define PE_TYPE_DEF          PE_TYPE_YXMMI

#define PERIOD_SEND          _TICK,  1

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    INT16U type;                       /* 命令类型 */
    INT16U seq;                        /* 命令流水号 */
    INT8U  ct_send;                    /* 重发次数 */
    INT16U ct_time;                    /* 重发等待时间计数 */
    INT16U flowtime;                   /* 重发等待时间 */
    INT16U slen;                       /* 数据长度 */
    INT8U  *sptr;                      /* 数据指针 */
    void   (*fp)(INT8U result);        /* 发送结果通知回调 */
} CELL_T;
/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static struct {
    NODE_T reserve;
    CELL_T cell;
} s_memory[NUM_MEM];

static INT8U s_sendtmr;
static LIST_T s_waitlist, s_readylist, s_freelist;
static INT8U s_seq;


/*******************************************************************
** 函数名:     DelCell
** 函数描述:   删除节点
** 参数:       [in] cell:链表节点
** 参数:       [in] result:结果
** 返回:       无
********************************************************************/

static void DelCell(CELL_T *cell, INT8U result)
{
    void (*fp)(INT8U result);
    
    fp = cell->fp;
    
    if (cell->sptr != 0) {
        YX_DYM_Free(cell->sptr);
        cell->sptr = 0;
    }
    YX_LIST_AppendListEle(&s_freelist, (INT8U *)cell);            
    if (fp != 0) { 
        fp(result);
    }
}

/*******************************************************************
** 函数名:     ScanTmrProc
** 函数描述:   扫描定时器
** 参数:       [in] pdata:定时器特征值
** 返回:       无
********************************************************************/
static void SendTmrProc(void *pdata)
{
    INT8U *memptr;
    INT16U len, memlen;
    CELL_T *cell, *next;
    
    pdata = pdata;
    
    if (YX_LIST_GetNodeNum(&s_waitlist) + YX_LIST_GetNodeNum(&s_readylist) == 0) {
        OS_StopTmr(s_sendtmr);
        return;
    }
    
    OS_StartTmr(s_sendtmr, PERIOD_SEND);
       
    cell = (CELL_T *)YX_LIST_GetListHead(&s_waitlist);                         /* 扫描等待链表，检测重传时间是否超时 */
    for (;;) {
        if (cell == 0) break;
        if (++cell->ct_time > cell->flowtime) {                                /* 重传时间超时 */
            cell->ct_time = 0;
            next = (CELL_T *)YX_LIST_DeleListEle(&s_waitlist,(INT8U *)cell);
            if (--cell->ct_send == 0) {                                        /* 超过最大重传次数 */
                DelCell(cell, _OVERTIME);
            } else {
                YX_LIST_AppendListEle(&s_readylist, (INT8U *)cell);
            }
            cell = next;
        } else { 
            cell = (CELL_T *)YX_LIST_GetNextEle((INT8U *)cell);
        }
    }
    
    cell = (CELL_T *)YX_LIST_GetListHead(&s_readylist);                        /* 扫描准备就绪链表，检查等待时间是否超时 */
    for (;;) {
        if (cell == 0) break;
        if (++cell->ct_time > cell->flowtime) {                                /* 等待时间超时 */
            cell->ct_time = 0;
            if (cell->ct_send == 0) {
                next = (CELL_T *)YX_LIST_DeleListEle(&s_readylist, (INT8U *)cell); /* 从就绪链表中删除节点 */
                DelCell(cell, _OVERTIME);
                cell = next;
                continue;
            } else {
                cell->ct_send--;
            }
        }
        cell = (CELL_T *)YX_LIST_GetNextEle((INT8U *)cell);
    }
   
    if (YX_LIST_GetNodeNum(&s_readylist) > 0) {                                /* 就绪链表中存在待发送节点 */
        cell = (CELL_T *)YX_LIST_GetListHead(&s_readylist);
        for (;;) {
           if (cell == 0) break;  
           
           memlen = cell->slen * 2 + 2;
           memptr = YX_DYM_Alloc(memlen);
           if (memptr == 0) {
               #if DEBUG_ERR > 0 
               printf_com("SendTmrProc malloc memfail>\r\n");
               #endif
               
               break;
           }
           
           if ((len = YX_AssembleByRules(memptr, memlen, cell->sptr, cell->slen, &g_dvr_rules)) == 0) {
                cell->ct_send = 0;
            } else {
                if (len <= YX_MMI_UartLeftOfSendbuf()) {
                    YX_MMI_SendData(memptr, len);
                } else {
                    ;
                }
            }
            YX_DYM_Free(memptr);

            next = (CELL_T *)YX_LIST_DeleListEle(&s_readylist, (INT8U *)cell);
            if (cell->ct_send > 0) {                                           /* 需要重传的数据帧 */
                cell->ct_time = 0;
                YX_LIST_AppendListEle(&s_waitlist,(INT8U *)cell);
            } else {                                                           /* 无需继续重传的数据帧 */
                DelCell(cell, _SUCCESS);
            }
            cell = next;
        }
    }
}
/*******************************************************************
** 函数名:     DiagnoseProc
** 函数描述:   诊断函数
** 参数:       无
** 返回:       无
********************************************************************/
static void DiagnoseProc(void)
{
    INT16U count;

    //OS_ASSERT(YX_CheckList(&s_readylist, (void *)s_memory, (void *)&s_memory[NUM_MEM], FALSE), ERR_MMI_SEND);
    //OS_ASSERT(YX_CheckList(&s_freelist, (void *)s_memory, (void *)&s_memory[NUM_MEM], FALSE), ERR_MMI_SEND);
    //OS_ASSERT(YX_CheckList(&s_waitlist, (void *)s_memory, (void *)&s_memory[NUM_MEM], FALSE), ERR_MMI_SEND);
    
    count = YX_LIST_GetNodeNum(&s_waitlist) + YX_LIST_GetNodeNum(&s_readylist) + YX_LIST_GetNodeNum(&s_freelist);
    OS_ASSERT(count == sizeof(s_memory)/sizeof(s_memory[0]), RETURN_VOID);
    
     if (YX_LIST_GetNodeNum(&s_waitlist) + YX_LIST_GetNodeNum(&s_readylist) > 0) {
        OS_ASSERT(OS_TmrIsRun(s_sendtmr), RETURN_VOID);
    }
}

/*******************************************************************
** 函数名:     YX_MMI_InitSend
** 函数描述:   MMI调度屏发送模块初始化
** 参数:       无
** 返回:       无
********************************************************************/
void YX_MMI_InitSend(void)
{
    s_seq = 0;
    YX_LIST_Init(&s_waitlist);
    YX_LIST_Init(&s_readylist);
    YX_LIST_CreateList(&s_freelist, (INT8U *)s_memory, sizeof(s_memory)/sizeof(s_memory[0]), sizeof(s_memory[0]));
    
    s_sendtmr = OS_CreateTmr(TSK_ID_APP, (void *)0, SendTmrProc);
  
    OS_RegistDiagnoseProc(DiagnoseProc);
}

/*******************************************************************
** 函数名:     YX_MMI_ListSend
** 函数描述:   MMI调度屏数据链表发送
** 参数:       [in] type: 协议类型
**             [in] ptr:  数据指针
**             [in] len:  数据长度
**             [in] ct_send:  发送次数
**             [in] ct_time:  重发等待时间，单位：秒
**             [in] fp:   发送结果通知
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN YX_MMI_ListSend(INT16U type, INT8U *ptr, INT16U len, INT8U ct_send, INT16U ct_time, void(*fp)(INT8U))
{
    CELL_T *cell;         
   
	if (len > MAX_SIZE) {                                                      /* 数据帧长度过长 */
	    return false;
	}
	
    if ((cell = (CELL_T *)YX_LIST_DeleListHead(&s_freelist)) != 0) {           /* 申请链表节点 */
        cell->slen = len + 4;
        cell->sptr = YX_DYM_Alloc(cell->slen + 1);
        if (cell->sptr == 0) {
            #if DEBUG_ERR > 0 
            printf_com("YX_MMI_ListSend malloc memfail>\r\n");   
            #endif
            
            YX_LIST_AppendListEle(&s_freelist, (INT8U *)cell);                 /* 重新释放回已获取的空闲链表 */
            return false;
        }
        cell->type     = type;
        cell->seq      = s_seq++;
        cell->ct_send  = ct_send;
        cell->flowtime = ct_time * 50 + 1;                                     /* 等待重传时间 */
        cell->ct_time  = 0;
        cell->fp       = fp;

        cell->sptr[1] = 0x01;                                                  /* 厂商编号 */
        cell->sptr[2] = PE_TYPE_DEF;                                           /* 外设类型 */
        
        if ((type > UP_PE_CMD_CAN_TRANS_DATA && type < UP_PE_CMD_CAN_TRANS_DATA + 0x10) ||
            type == UP_PE_CMD_SLAVE_GET_PARA || type == UP_PE_ACK_HOST_SET_PARA || type == UP_PE_ACK_HOST_GET_PARA ||
            type == UP_PE_CMD_REPORT_ICCARD_DATA || type == UP_PE_ACK_WRITE_ICCARD || 
            type == UP_PE_ACK_CTL_FUNCTION) {
            cell->slen++;
            cell->sptr[3] = UP_PE_CMD_CAN_TRANS_DATA;
            cell->sptr[4] = type;
        
            YX_MEMCPY(cell->sptr + 5, len, ptr, len);
            cell->sptr[0] = YX_GetChkSum(cell->sptr + 1, len + 4);
        } else {
            cell->sptr[3] = type;
        
            YX_MEMCPY(cell->sptr + 4, len, ptr, len);
            cell->sptr[0] = YX_GetChkSum(cell->sptr + 1, len + 3);
        }
        
        YX_LIST_AppendListEle(&s_readylist, (INT8U *)cell);                    /* 将新增节点放入就绪链表 */
        
        if (!OS_TmrIsRun(s_sendtmr)) {
            OS_StartTmr(s_sendtmr, PERIOD_SEND);
        }
        
        return true;
    } else {
        return false;
    }
}

/*******************************************************************
** 函数名:     YX_MMI_DirSend
** 函数描述:   直接发送，不挂到链表发送
** 参数:       [in] cmd: 协议类型
**             [in] ptr: 数据指针
**             [in] len: 数据长度
** 返回:       发送成功返回true，失败返回false
********************************************************************/
BOOLEAN YX_MMI_DirSend(INT16U type, INT8U *ptr, INT16U len)
{
    INT8U *memptr;
    INT16U dlen, memlen;
    BOOLEAN result;
    
    memlen = len * 2 + 10;
    memptr = YX_DYM_Alloc(len * 2 + 10);
    if (memptr == 0) {
        return false;
    }
    s_seq++;
    memptr[len + 2] = 0x01;                                                      /* 厂商编号 */
    memptr[len + 3] = PE_TYPE_DEF;                                               /* 外设类型 */
    
    if ((type > UP_PE_CMD_CAN_TRANS_DATA && type < UP_PE_CMD_CAN_TRANS_DATA + 0x10) ||
         type == UP_PE_CMD_SLAVE_GET_PARA || type == UP_PE_ACK_HOST_SET_PARA || type == UP_PE_ACK_HOST_GET_PARA ||
         type == UP_PE_CMD_REPORT_ICCARD_DATA || type == UP_PE_ACK_WRITE_ICCARD || 
         type == UP_PE_ACK_CTL_FUNCTION) {
        memptr[len + 4] = UP_PE_CMD_CAN_TRANS_DATA;
        memptr[len + 5] = type;
        
        YX_MEMCPY(&memptr[len + 6], len, ptr, len);   
        memptr[len + 1] = YX_GetChkSum(&memptr[len + 2], len + 4);
        if ((dlen = YX_AssembleByRules(memptr, memlen, &memptr[len + 1], len + 5, &g_dvr_rules)) == 0) {
            result = false;
        } else {
            result = YX_MMI_SendData(memptr, dlen);
        }
    } else {
        memptr[len + 4] = type;                                                   
        
        YX_MEMCPY(&memptr[len + 5], len, ptr, len);   
        memptr[len + 1] = YX_GetChkSum(&memptr[len + 2], len + 3);
        if ((dlen = YX_AssembleByRules(memptr, memlen, &memptr[len + 1], len + 4, &g_dvr_rules)) == 0) {
            result = false;
        } else {
            result = YX_MMI_SendData(memptr, dlen);
        }
    }

    YX_DYM_Free(memptr);
    
    return result;
}

/*******************************************************************
** 函数名:     YX_MMI_ListAck
** 函数描述:   MMI调度屏发送链表确认
** 参数:       [in] type: 协议类型
**             [in] result: 结果
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN YX_MMI_ListAck(INT16U type, INT8U result)
{
    CELL_T  *cell;
    
    cell = (CELL_T *)YX_LIST_GetListHead(&s_waitlist);
    for (;;) {                                                                 /* 查找等待链表 */
        if (cell == 0) break;
        if (cell->type == type) {                                              /* 查找到匹配的节点 */
            YX_LIST_DeleListEle(&s_waitlist, (INT8U *)cell);
            DelCell(cell, result);
            return true;
        } else {
            cell = (CELL_T *)YX_LIST_GetNextEle((INT8U *)cell);                /* 扫描下个节点 */
        }
    }

    cell = (CELL_T *)YX_LIST_GetListHead(&s_readylist);                        /* 查找就绪链表 */
    for (;;) {
        if (cell == 0) break;
        if (cell->type == type) {                                              /* 查找到匹配节点 */
            YX_LIST_DeleListEle(&s_readylist, (INT8U *)cell);
            DelCell(cell, result);
            return true;
        } else {
            cell = (CELL_T *)YX_LIST_GetNextEle((INT8U *)cell);                /* 扫描下个节点 */
        }
    }
    return false;        
}

/*******************************************************************
** 函数名:     YX_MMI_ListSeqAck
** 函数描述:   MMI调度屏发送链表确认(带流水号)
** 参数:       [in] type: 协议类型
**             [in] seq: 流水号
**             [in] result: 结果
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN YX_MMI_ListSeqAck(INT16U type, INT16U seq, INT8U result)
{
    CELL_T  *cell;
    
    cell = (CELL_T *)YX_LIST_GetListHead(&s_waitlist);
    for (;;) {                                                                 /* 查找等待链表 */
        if (cell == 0) break;
        if ((cell->type == type) && (cell->seq == seq)) {                      /* 查找到匹配的节点 */
            YX_LIST_DeleListEle(&s_waitlist, (INT8U *)cell);
            DelCell(cell, result);
            return true;
        } else {
            cell = (CELL_T *)YX_LIST_GetNextEle((INT8U *)cell);                /* 扫描下个节点 */
        }
    }

    cell = (CELL_T *)YX_LIST_GetListHead(&s_readylist);                        /* 查找就绪链表 */
    for (;;) {
        if (cell == 0) break;
        if ((cell->type == type) && (cell->seq == seq)) {                      /* 查找到匹配节点 */
            YX_LIST_DeleListEle(&s_readylist, (INT8U *)cell);
            DelCell(cell, result);
            return true;
        } else {
            cell = (CELL_T *)YX_LIST_GetNextEle((INT8U *)cell);                /* 扫描下个节点 */
        }
    }
    return false;        
}

/*******************************************************************
**  函数名  :  YX_MMI_GetSendSeq
** 函数描述:  获取发送流水号
** 参数:      无
**  返回参数:  无
********************************************************************/
INT16U YX_MMI_GetSendSeq(void)
{
    return s_seq;
}

#endif


