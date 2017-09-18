/********************************************************************************
**
** 文件名:     yx_win_recv.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现通信层协议数据接收并组成完整的数据块，提供给应用层处理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2013/11/09 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "yx_includes.h"
#include "yx_tools.h"
#include "yx_structs.h"
#include "yx_timer.h"
#include "yx_list.h"
#include "yx_stream.h"
#include "yx_dm.h"
#include "dal_pp_drv.h"
#include "dal_gprs_drv.h"
#include "dal_sm_drv.h"
#include "dal_systime.h"
#include "yx_protocol_type.h"
#include "yx_protocol_recv.h"
#include "yx_protocol_send.h"
#include "yx_win_recv.h"
#include "yx_win_send.h"
#include "yx_apptool.h"
#include "yx_debug.h"



/*
********************************************************************************
* 定义配置参数
********************************************************************************
*/
#define PERIOD_SCAN                      SECOND, 1, LOW_PRECISION_TIMER
                                   
#define MAX_OVER                         10 
#define MAX_TIME                         30
#define MAX_WIN                          2
#define WIN_SIZE                         256               /* 最大支持接收的数据包数,要为8的倍数 */

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    INT16U msgid;                 /* 数据类型 */
    INT16U packetlen;             /* 正常的包长度 */
    INT16U lastlen;               /* 最后一包长度 */    
    INT16U flowseq;               /* 包流水号 */
    INT16U wintime;               /* 等待应答时间 */
    INT16U winover;               /* 等待应答次数 */
    INT8U  *memptr;               /* 窗口数据指针 */
    INT8U  winflag[WIN_SIZE/8];   /* 包接收标志 */
    INT16U maxpacket;             /* 窗口数据最大的分包数 */
} WINCELL_T;

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static struct {
    NODE      res;
    WINCELL_T cell;
} memblock[MAX_WIN];

static INT8U s_scantmr;
static LIST_T s_winfreelist, s_winrecvlist;



/*******************************************************************
** 函数名:     DelCell
** 函数描述:   删除节点
** 参数:       [in] cell:链表节点
** 返回:       无
********************************************************************/
static void DelCell(INT8U result, WINCELL_T *wincell)
{	
    if (wincell->memptr != 0) {
        YX_DYM_Free(wincell->memptr);
        wincell->memptr = 0;
    }
    
    YX_AppendListEle(&s_winfreelist, (LISTMEM *)wincell);
}

/*******************************************************************
** 函数名:     ScanTmrProc
** 函数描述:   扫描定时器
** 参数:       [in] pdata:定时器特征值
** 返回:       无
********************************************************************/
static void ScanTmrProc(void *pdata)
{                                  
    WINCELL_T *wincell, *nextcell;

    pdata = pdata;
    
    if (YX_ListItem(&s_winrecvlist) == 0) {
        YX_StopTmr(s_scantmr);
        return;
    }
    
    YX_StartTmr(s_scantmr, PERIOD_SCAN);
    
    wincell = (WINCELL_T *)YX_GetListHead(&s_winrecvlist);
    for (;;) {
        if (wincell == 0) {
            break;
        }
        
        if (++wincell->wintime >= MAX_TIME) {
            wincell->wintime = 0;
            if (++wincell->winover >= MAX_OVER) {
                wincell->winover = 0;
                nextcell = (WINCELL_T *)YX_DelListEle(&s_winrecvlist, (LISTMEM *)wincell);
                DelCell(_OVERTIME, wincell);                
                wincell = nextcell;
            } else {
                wincell = (WINCELL_T *)YX_ListNextEle((LISTMEM *)wincell);
            }
        } else {
            wincell = (WINCELL_T *)YX_ListNextEle((LISTMEM *)wincell);
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
    OS_ASSERT(YX_CheckList(&s_winfreelist, (void *)memblock, (void *)&memblock[MAX_WIN], FALSE), RETURN_VOID);
    OS_ASSERT(YX_CheckList(&s_winrecvlist,  (void *)memblock, (void *)&memblock[MAX_WIN], FALSE), RETURN_VOID);

    OS_ASSERT((YX_ListItem(&s_winfreelist) + YX_ListItem(&s_winrecvlist)) == sizeof(memblock) / sizeof(memblock[0]), RETURN_VOID);
    
    if (YX_ListItem(&s_winrecvlist) != 0) {
        OS_ASSERT(YX_TmrIsRun(s_scantmr), RETURN_VOID);
    }
}

/*******************************************************************
** 函数名:     YX_WIN_InitRecv
** 函数描述:   可靠下载模块初始化
** 参数:       无
** 返回:       无
********************************************************************/
void YX_WIN_InitRecv(void)
{   
    YX_InitMemList(&s_winfreelist, (LISTMEM *)memblock, sizeof(memblock) / sizeof(memblock[0]), sizeof(memblock[0]));
    YX_InitList(&s_winrecvlist);         

    s_scantmr = YX_InstallTmr(PRIO_OPTTASK, 0, ScanTmrProc);
    YX_StartTmr(s_scantmr, PERIOD_SCAN);

    OS_InstallDiag(DiagnoseProc);
}

/*******************************************************************
** 函数名:     YX_WIN_HdlRecv
** 函数描述:   处理窗口数据
** 参数:       [in]  attrib:    通道属性和重发属性，见PROTOCOL_COM_T
**             [in]  frameptr:  数据指针
**             [in]  framelen:  数据长度
**             [in]  handler:   协议处理函数             
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN YX_WIN_HdlRecv(PROTOCOL_COM_T *attrib, SYSFRAME_T *frameptr, INT16U framelen, void (*handler)(PROTOCOL_COM_T *attrib, SYSFRAME_T *sptr, INT32U slen))
{
    INT8U result, exheadlen;
    INT16U i, num, msgid, msgattrib, flowseq, packetlen, framebodylen, maxpacket, curpacket;
    INT32U templen, offset;
    STREAM_T rstrm, *wstrm;
    WINCELL_T *wincell;

    if (!YX_TmrIsRun(s_scantmr)) {
        YX_StartTmr(s_scantmr, PERIOD_SCAN);
    }

    if (framelen >= (SYSHEAD_LEN + SYSTAIL_LEN)) {
        framebodylen = framelen - SYSHEAD_LEN - SYSTAIL_LEN;
    } else {
        return FALSE;
    }
    
    YX_InitStrm(&rstrm, (INT8U *)frameptr->data, framebodylen);

    msgid     = (frameptr->msgid[0] << 8) + frameptr->msgid[1];                /* 协议ID */
    msgattrib = (frameptr->msgattrib[0] << 8) + frameptr->msgattrib[1];        /* 消息体属性 */
    flowseq   = (frameptr->flowseq[0] << 8) + frameptr->flowseq[1];            /* 流水号 */
    
    packetlen = (msgattrib & 0x03FF);                                          /* 消息体长度 */
    if ((msgattrib & PROTOCOL_EXT_HEAD) != 0) {                                 /* 带分包标志 */
        maxpacket = YX_ReadHWORD_Strm_APP(&rstrm);                             /* 读取总包数 */
        curpacket = YX_ReadHWORD_Strm_APP(&rstrm);                             /* 当前包序号 */
        
        curpacket -= 1;                                                        /* 包序号调准为从0开始 */
        //exheadlen = 4;
        exheadlen = 0;                                                         /* 不向业务层提供扩展头 */
    } else {
        maxpacket = 1;
        curpacket = 0;
        exheadlen = 0;
    }
    
    /* 从接收链表中查找是否已在接收过程中 */
    wincell = (WINCELL_T *)YX_GetListHead(&s_winrecvlist);
    for (;;) {
        if (wincell == 0) {
            break;
        }

        if (wincell->msgid == msgid) {                                         /* 同一批数据流水号不同 */
            break;
        }
        
        wincell = (WINCELL_T *)YX_ListNextEle((LISTMEM *)wincell);
    }
    
    /* 申请一个空闲的接收窗口 */
    if (wincell == 0) {
        wincell = (WINCELL_T *)YX_DelListHead(&s_winfreelist);
        if (wincell == 0) {
            return false;
        }
        
        YX_MEMSET(wincell, 0, sizeof(WINCELL_T));
        wincell->msgid     = msgid;
        wincell->flowseq   = flowseq;
        wincell->maxpacket = maxpacket;
        
        if ((curpacket != (maxpacket - 1)) || (maxpacket == 1)) {              /* 不是最后一包不能作为计算总体长度 */
            wincell->packetlen = packetlen;
            
            if ((maxpacket * packetlen) > SIZE_WIN_RECV) {                     /* 超过最大缓存 */
                #if DEBUG_GSMSTATUS > 0
                printf_com("<通信层接收数据长度超过最大缓存>\r\n");
                #endif
                
                if (wincell->memptr != 0) {
                    YX_DYM_Free(wincell->memptr);
                    wincell->memptr = 0;
                }
            } else {
                templen = SYSHEAD_LEN + exheadlen + (maxpacket * packetlen) + 1;/* last 1 for chksum */
                wincell->memptr = YX_DYM_Alloc(templen);
                OS_ASSERT((wincell->memptr != 0), RETURN_FALSE);
                
                YX_MEMCPY(wincell->memptr, SYSHEAD_LEN + exheadlen, (INT8U *)frameptr, SYSHEAD_LEN + exheadlen);
            }
        } else {
            YX_AppendListEle(&s_winfreelist, (LISTMEM *)wincell);
            return FALSE;
        }
        YX_AppendListEle(&s_winrecvlist, (LISTMEM *)wincell);
    }

    if (wincell->memptr != 0) {
        if (curpacket >= wincell->maxpacket) {                                 /* 包序号出错 */
            #if DEBUG_GSMSTATUS > 0
            printf_com("<通信层接收分包序号出错(%d)(%d)>\r\n", curpacket, wincell->maxpacket);
            #endif
            
            return false;
        } else if (curpacket == maxpacket - 1) {                               /* 最后一包数据长度 */
            wincell->lastlen = packetlen;
        } else {
            if (packetlen != wincell->packetlen) {
                #if DEBUG_GSMSTATUS > 0
                printf_com("<通信层接收分包长度出错(%d)(%d)>\r\n", packetlen, wincell->packetlen);
                #endif
                
                return false;
            }
        }
        
        offset = SYSHEAD_LEN + exheadlen + (curpacket * wincell->packetlen);
        YX_MEMCPY(wincell->memptr + offset, packetlen, (INT8U *)YX_GetStrmPtr(&rstrm), packetlen);
        wincell->winflag[curpacket / 8] |= (1 << (curpacket % 8));
        
        #if DEBUG_GSMSTATUS > 0
        if (wincell->maxpacket > 1) {
            printf_com("<通信层接收,ID(0x%x),总包数(%d),当前包号(%d),winflag[%d]:0x%x>\r\n", 
                          msgid, curpacket, curpacket, (curpacket / 8), wincell->winflag[curpacket / 8]);
        }
        #endif        
    } else {                                                                   /* 之前未能申请到动态内存 */
        wincell->msgid     = msgid;
        wincell->flowseq   = flowseq;
        wincell->maxpacket = maxpacket;
        
        if ((curpacket != maxpacket - 1) || (maxpacket == 1)) {
            wincell->packetlen = packetlen;
            
            if (maxpacket * packetlen > SIZE_WIN_RECV) {                       /* 超过最大缓存 */
                if (wincell->memptr != 0) {
                    YX_DYM_Free(wincell->memptr);
                    wincell->memptr = 0;
                }
            } else {
                templen = SYSHEAD_LEN + exheadlen + maxpacket * packetlen + 1; /* last 1 for chksum */
                wincell->memptr = YX_DYM_Alloc(templen);
                OS_ASSERT((wincell->memptr != 0), RETURN_FALSE);
                
                YX_MEMCPY(wincell->memptr, SYSHEAD_LEN + exheadlen, (INT8U *)frameptr, SYSHEAD_LEN + exheadlen);
                
                offset = SYSHEAD_LEN + exheadlen + (curpacket * wincell->packetlen);
                YX_MEMCPY(wincell->memptr + offset, packetlen, (INT8U *)YX_GetStrmPtr(&rstrm), packetlen);
                wincell->winflag[curpacket / 8] |= (1 << (curpacket % 8));
            }
        }
    }

    /* 若接收完毕则释放通道 */
    num = 0;
    if (wincell->maxpacket > 0) {
        for (i = 0; i < wincell->maxpacket; i++) {
            if ((wincell->winflag[i / 8] & (1 << (i % 8))) == 0) {             /* 未接收到节点 */
                num++;
            }
        }
        
        if (wincell->maxpacket > 1) {                                          /* 接收到多包: 每分包给一个应答 */
            wstrm = YX_PROTOCOL_GetBufferStrm();
            
            msgattrib = 5;
            result    = ACK_SUCCESS;
            YX_PROTOCOL_AsmFrameHead(wstrm, UP_ACK_COMMON, msgattrib, 0);
            
            if (curpacket != wincell->maxpacket - 1) {
                YX_WriteDATA_Strm(wstrm, frameptr->flowseq, sizeof(frameptr->flowseq));/* 对应流水号 */
                YX_WriteDATA_Strm(wstrm, frameptr->msgid, sizeof(frameptr->msgid));    /* 对应消息ID */
                YX_WriteBYTE_Strm(wstrm, result);                                      /* 结果 */
                
                attrib->attrib |= SM_ATTR_SUCCESS;
                YX_PROTOCOL_SendData(wstrm, attrib, 0);
            } else {                                                            /* 最后一包由应用层发送 */
                wincell->flowseq = flowseq;
                YX_MEMCPY(wincell->memptr, SYSHEAD_LEN + exheadlen, (INT8U *)frameptr, SYSHEAD_LEN + exheadlen);                
            }
        }
        
        /* 完整的数据帧接收完毕 */
        if (num == 0) {
            templen = SYSHEAD_LEN + exheadlen + (wincell->maxpacket - 1) * wincell->packetlen + wincell->lastlen + 1; /* last 1 for chksum */
            wincell->memptr[templen - 1] = *((INT8U *)frameptr + framelen - 1);
            if (handler != 0) {
                handler(attrib, (SYSFRAME_T *)wincell->memptr, templen);
            }
            
            YX_DelListEle(&s_winrecvlist, (LISTMEM *)wincell);
            DelCell(_SUCCESS, wincell);
            return TRUE;
        }
    }

    return TRUE;
}

