/********************************************************************************
**
** 文件名:     yx_win_send.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现通信层协议数据分包发送和可靠传输
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2013/11/09 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "yx_includes.h"
#include "yx_structs.h"
#include "yx_timer.h"
#include "yx_list.h"
#include "yx_stream.h"
#include "yx_dm.h"
#include "yx_tools.h"
#include "dal_systime.h"
#include "dal_pp_drv.h"
#include "dal_gprs_drv.h"
#include "dal_sm_drv.h"
#include "yx_protocol_type.h"
#include "yx_protocol_recv.h"
#include "yx_protocol_send.h"
#include "yx_win_send.h"
#include "yx_win_recv.h"
#include "yx_jt_linkman.h"
#include "yx_debug.h"

/*
********************************************************************************
* 宏定义
********************************************************************************
*/
#define PERIOD_SCAN                      SECOND, 1, LOW_PRECISION_TIMER
                                   
#define MAX_OVER                         3                 /* 连续3次请求应答超时，则认为通讯异常 */   
#define MAX_TIME                         20                /* 请求中心应答超时时间，单位：秒 */
#define SIZE_PACKET                      960               /* 分包传输一次传输最大数据长度 */
#define MAX_WIN                          32                /* 最大链表节点个数 */
#define LMT_WIN                          8                 /* 一次发送数据包数 */
#define WIN_SIZE                         256               /* 最大支持发送的数据包数,要为8的倍数 */

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    INT16U winid;            /* 窗口索引号 */
    INT16U frameseq;         /* 上传包流水号 */
    INT16U wintime;          /* 等待应答时间 */
    INT16U winover;          /* 等待应答次数 */
    INT16U max_wintime;      /* 最大等待应答时间 */
    INT16U max_winover;      /* 最大等待应答次数 */ 
    
    INT8U  period;           /* 发送周期 */
    INT8U  max_period;       /* 发送周期值 */
    INT8U  *ptr;             /* 窗口数据指针 */
    INT32U len;              /* 窗口数据长度 */
    INT16U msgid;            /* 数据类型 */
    
    INT16U maxpacket;        /* 窗口数据最大的分包数 = 包长度 / 包基准大小 + (1 or 0) */
    INT16U packetsize;       /* 包基准大小 */
    
    INT8U  swinflag[WIN_SIZE/8];/* 包发送标志 */
    INT8U  rwinflag[WIN_SIZE/8];/* 包接收标志 */
    
    PROTOCOL_COM_T attrib;    /* 发送属性 */
    void (*fp)(INT16U winid, INT8U result, INT8U *ptr, INT32U len);
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
static LIST_T s_winfreelist, s_winsendlist;                            


/*******************************************************************
** 函数名:     GetWinID
** 函数描述:   获取窗口id
** 参数:       无
** 返回:       返回窗口id
********************************************************************/
static INT16U GetWinID(void)
{
    INT16U winid;
    SYSTIME_T systime;
    
    YX_GetSysTime(&systime);
    
    winid = systime.time.second + (systime.time.minute << 6) + (systime.time.hour << 12);
    
    return winid;
}

/*******************************************************************
** 函数名:     AllSend
** 函数描述:   判断节点数据是否已全部发送
** 参数:       [in] wincell: 窗口节点指针
** 返回:       全部发送返回true，否则返回false
********************************************************************/
static BOOLEAN AllSend(WINCELL_T *wincell)
{
    INT16U i;
    
    for (i = 0; i < wincell->maxpacket; i++) {
        if ((wincell->rwinflag[i / 8] & (1 << (i % 8))) == 0) {
            return FALSE;
        }
    }
    
    return TRUE;
}

/*******************************************************************
** 函数名:     ClearWin
** 函数描述:   清除发送窗口
** 参数:       [in] wincell: 窗口节点指针
** 返回:       无
********************************************************************/
static void ClearWin(WINCELL_T *wincell, INT8U result)
{
    if (wincell->fp != 0) {                                                    /* 回调处理 */
        wincell->fp(wincell->winid, result, wincell->ptr, wincell->len);
        wincell->len = 0;
        wincell->fp = 0;
    }
    
    if (wincell->ptr != 0) {                                                   /* 内存释放 */
        YX_DYM_Free(wincell->ptr);
        wincell->ptr = 0;
    }
    
    YX_AppendListEle(&s_winfreelist, (LISTMEM *)wincell);
}

/*******************************************************************
** 函数名:     SendDataCell
** 函数描述:   发送数据
** 参数:       [in] wincell: 窗口节点指针
**             [in] index:   包序号
** 返回:       发送成功返回true，否则返回false
********************************************************************/
static BOOLEAN SendDataCell(WINCELL_T *cell, INT16U index)                   
{
    INT16U pktlen, comattrib, msgattrib, size_packet, channel;
    STREAM_T *wstrm;
    
    OS_ASSERT((index < cell->maxpacket), RETURN_FALSE);
    OS_ASSERT((cell->len > index * cell->packetsize), RETURN_FALSE);
    
    comattrib = cell->attrib.attrib & SM_ATTR_CHAMASK;                         /* 通信通道类型 */
    channel   = cell->attrib.channel;                                          /* 通信通道编号 */
    if ((comattrib & SM_ATTR_SM) == 0) {
        if ((comattrib & SM_ATTR_TCP) != 0) {                                  /* TCP通道 */
            if (!YX_JTT_LinkCanCom(channel)) {
                return false;
            }
        }
        
        if ((comattrib & SM_ATTR_UDP) != 0) {                                  /* UDP通道 */
            if (!YX_JTU_LinkCanCom(channel)) {
                return false;
            }
        }
    }
        
    wstrm = YX_PROTOCOL_GetBufferStrm();

    size_packet = cell->packetsize;
    if (index != cell->maxpacket - 1) {
        pktlen = size_packet;
    } else {
        pktlen = cell->len - size_packet * index;    
    }
    
    msgattrib = pktlen;                                                        /* 消息属性 */
    if (cell->maxpacket > 1) {                                                 /* 有分包 */
        msgattrib |= PROTOCOL_EXT_HEAD;
    }

	if (cell->frameseq == 0) {                                                 /* 车台默认流水号从1开始，为0表示无效流水号 */
		if (index == 0) {                                                      /* 新生数据 */
		    cell->frameseq = YX_PROTOCOL_AsmFrameHead(wstrm, cell->msgid, msgattrib, cell->frameseq);
		    YX_PROTOCOL_ModifyFrameSeq(cell->maxpacket - 1);
		} else {
            return FALSE;
		}
	} else {
        YX_PROTOCOL_AsmFrameHead(wstrm, cell->msgid, msgattrib, cell->frameseq + index);/* 同一批数据,不同包采用不同的流水号 */
	}
    
    if (cell->maxpacket > 1) {    
        YX_WriteHWORD_Strm(wstrm, cell->maxpacket);                            /* 写总包数*/
        YX_WriteHWORD_Strm(wstrm, index + 1);                                  /* 写包序号: 协议从1开始 */
    }

    YX_WriteDATA_Strm(wstrm, (cell->ptr + index * size_packet), pktlen);       /* 包数据 */
    return YX_PROTOCOL_SendData(wstrm, &(cell->attrib), 0);
}

/*******************************************************************
** 函数名:     ScanTmrProc
** 函数描述:   扫描定时器
** 参数:       [in] index: 定时器特征值
** 返回:       无
********************************************************************/
static void ScanTmrProc(void *index)                        
{                                                           
    BOOLEAN finish;
    INT16U i, j, k, winnum;
    WINCELL_T *wincell, *nextcell;

    YX_StartTmr(s_scantmr, PERIOD_SCAN);
    
    if (YX_ListItem(&s_winsendlist) == 0) {
        YX_StopTmr(s_scantmr);
        return;
    }
    
    winnum = LMT_WIN;
    for (i = PTOTOCOL_PRIO_MAX - 1; i > 0; i--) {
        wincell = (WINCELL_T *)YX_GetListHead(&s_winsendlist);            
        for (;;) {
            if (wincell == 0) {
                break;
            }
            
            if (wincell->attrib.priority != i) {                               /* 高优先级数据先发 */
                wincell = (WINCELL_T *)YX_ListNextEle((LISTMEM *)wincell);
                continue;
            }
            
            if (wincell->period > 0) {
                wincell->period--;
            }

            if (wincell->period == 0) {
                wincell->period = wincell->max_period;
                if (wincell->maxpacket > 0) {                                  /* 发送窗口节点数不为0且已登陆行业中心GPRS前置机 */
                    finish = TRUE;                                             /* 用于标志发送窗口内的数据是否都已发送过 */
                    
                    for (j = 0; j < wincell->maxpacket; j++) {                 /* 扫描发送窗口中未发送数据 */
                        if ((wincell->swinflag[j / 8] & (1 << (j % 8))) == 0) {/* 存在未发送节点, 0: 未发送, 1: 已发送 */
                            finish = FALSE;                                    /* 表示在发送窗口中存在待发送数据 */
                            if (SendDataCell(wincell, j)) {                    /* 发送该节点成功 */
                                wincell->swinflag[j / 8] |= (1 << (j % 8));    /* 将该节点置成已发送标志 */
                                if (--winnum == 0) {
                                    break;
                                }
                            } else {
                                break;
                            }
                        }
                    }
                    
                    if (finish) {                                              /* 发送窗口中数据已发送完毕 */
                        if (wincell->max_wintime == 0 || wincell->max_winover == 0) {/* 无需应答,无需可靠传输 */
                            nextcell = (WINCELL_T *)YX_DelListEle(&s_winsendlist, (LISTMEM *)wincell);
                            ClearWin(wincell, _SUCCESS);
                            wincell = nextcell;
                            continue;
                        } else {
                            if ((++wincell->wintime) * wincell->max_period >= wincell->max_wintime * (wincell->winover + 1)) {/* 判断请求应答时间是否超时 */
                                wincell->wintime = 0;
                                
                                if (++wincell->winover >= wincell->max_winover) {/* 超时次数是否超过规定，则认为GPRS网络存在异常 */
                                    wincell->winover = 0;
                                    nextcell = (WINCELL_T *)YX_DelListEle(&s_winsendlist, (LISTMEM *)wincell);
                                    ClearWin(wincell, _OVERTIME);              /* 重传多次无应答清除指定窗口，释放资源 */
                                    wincell = nextcell;
                                    continue;
                                } else {
                                    wincell->period = 1;
                                    for (k = 0; k < (WIN_SIZE / 8); k++) {
                                        wincell->swinflag[k] = (wincell->swinflag[k] & wincell->rwinflag[k]);
                                    }
                                    //SendDataCell(wincell, wincell->maxpacket - 1);
                                }
                            }
                        }
                    } else {                                                   /* 发送窗口中数据未发送完毕 */
                        wincell->wintime = 0;
                    }
                } else {
                    OS_ASSERT(0, RETURN_VOID);
                }
                
                if (winnum == 0) {                                             /* 通信通道一次性发送的最多数据包数 */
                    return;
                }
            }
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
    OS_ASSERT(YX_CheckList(&s_winsendlist,  (void *)memblock, (void *)&memblock[MAX_WIN], FALSE), RETURN_VOID);
    
    OS_ASSERT(YX_ListItem(&s_winfreelist) + YX_ListItem(&s_winsendlist) == sizeof(memblock) / sizeof(memblock[0]), RETURN_VOID);
    
    if (YX_ListItem(&s_winsendlist) != 0) {
        OS_ASSERT(YX_TmrIsRun(s_scantmr), RETURN_VOID);
    }
}

/*******************************************************************
** 函数名:     YX_WIN_InitSend
** 函数描述:   可靠传输模块初始化
** 参数:       无
** 返回:       无
********************************************************************/
void YX_WIN_InitSend(void)
{
    YX_InitMemList(&s_winfreelist, (LISTMEM *)memblock, sizeof(memblock) / sizeof(memblock[0]), sizeof(memblock[0]));
    YX_InitList(&s_winsendlist);   

    s_scantmr = YX_InstallTmr(PRIO_OPTTASK, 0, ScanTmrProc);

    OS_InstallDiag(DiagnoseProc);
}


/*******************************************************************
** 函数名:     YX_WIN_SendDataByRegInfo
** 函数描述:   请求通过通信层发送协议数据(根据协议通道注册表发送数据接口)
** 参数:       [in] msgid：    协议ID
**             [in] sptr：    数据指针,可以为动态内存或者静态内存,如果是动态内存,将在本模块中释放
**             [in] slen：     数据长度,最大发送数据长度为(WIN_SIZE*SIZE_PACKET)
**             [in] fp：       发送结果回调函数
** 返回:       成功返回索引号，失败返回0, 失败情况下无fp回调
********************************************************************/
INT16U YX_WIN_SendDataByRegInfo(INT32U msgid, INT8U *sptr, INT32U slen, WIN_SEND_CALLBACK fp)
{
    INT8U i;
    INT16U winid, result;
    PROTOCOL_COM_T attrib;
    TEL_T alarmtel;
    PROTOCOL_REG_T const *preg;
    
    preg = YX_PROTOCOL_GetRegInfo(msgid);
    if (preg == 0) {
        return 0;
    }
    
    DAL_PP_ReadParaByID(PP_ID_ALARMTEL, (INT8U *)&alarmtel, sizeof(alarmtel));
    YX_MEMSET(&attrib, 0, sizeof(attrib));
    if (alarmtel.tellen < sizeof(attrib.tel)) {                                   /* 手机号码 */
        YX_MEMCPY(attrib.tel, alarmtel.tellen, alarmtel.tel, alarmtel.tellen);
        attrib.tel[alarmtel.tellen] = '\0';
    }
    
    result = 0;
    for (i = 0; i < TCP_USER_MAX; i++) {
        if ((preg->tcp & (1 << i)) != 0) {
            attrib.attrib   = SM_ATTR_TCP;                                     /* 发送属性 */
            if (preg->sm != 0 && result == 0) {                                /* 需要短信备份,且未调用发送成功 */
                attrib.attrib  |= SM_ATTR_SM;
            }
            
            attrib.channel  = i;                                               /* 发送通道 */
            attrib.type     = PTOTOCOL_TYPE_DATA;                              /* 协议数据类型 */
            attrib.priority = PTOTOCOL_PRIO_LOW;                               /* 协议数据优先级 */
            
            winid = YX_WIN_SendDataEx(preg->msgid, &attrib, 1, MAX_TIME, MAX_OVER, sptr, slen, fp);
            if (winid != 0) {
                if (result == 0) {                                             /* 以主通道为准返回窗口ID */
                    result = winid;
                }
            }
        }
    }
    
    for (i = 0; i < UDP_USER_MAX; i++) {
        if ((preg->udp & (1 << i)) != 0) {
            attrib.attrib   = SM_ATTR_UDP;                                     /* 发送属性 */
            if (preg->sm != 0 && result == 0) {                                /* 需要短信备份,且未调用发送成功 */
                attrib.attrib  |= SM_ATTR_SM;
            }
            
            attrib.channel  = i;                                               /* 发送通道 */
            attrib.type     = PTOTOCOL_TYPE_DATA;                              /* 协议数据类型 */
            attrib.priority = PTOTOCOL_PRIO_LOW;                               /* 协议数据优先级 */
            
            winid = YX_WIN_SendDataEx(preg->msgid, &attrib, 1, MAX_TIME, MAX_OVER, sptr, slen, fp);
            if (winid != 0) {
                if (result == 0) {                                             /* 以主通道为准 */
                    result = winid;
                }
            }
        }
    }
    
    return result;
}

/*******************************************************************
** 函数名:     YX_WIN_SendData
** 函数描述:   请求通过通信层发送协议数据(经典接口)
** 参数:       [in] msgid：    协议ID
** 参数:       [in] attrib:    通道属性和重发属性，见PROTOCOL_COM_T
**             [in] automode： 上传模式,TRUE-主动上报,FALSE-应答模式
**             [in] sptr：    数据指针,可以为动态内存或者静态内存,如果是动态内存,将在本模块中释放
**             [in] slen：     数据长度,最大发送数据长度为(WIN_SIZE*SIZE_PACKET)
**             [in] fp：       发送结果回调函数
** 返回:       成功返回索引号，失败返回0, 失败情况下无fp回调
********************************************************************/
INT16U YX_WIN_SendData(INT32U msgid, PROTOCOL_COM_T *attrib, INT8U automode, INT8U *sptr, INT32U slen, WIN_SEND_CALLBACK fp)
{
    if (automode) {                                                            /* 主动上报的数据帧 */
        return YX_WIN_SendDataEx(msgid, attrib, 1, MAX_TIME, MAX_OVER, sptr, slen, fp);
    } else {                                                                   /* 应答帧 */
        return YX_WIN_SendDataEx(msgid, attrib, 1, 0, 0, sptr, slen, fp);
    }
}

/*******************************************************************
** 函数名:     YX_WIN_SendDataEx
** 函数描述:   请求通过通信层发送协议数据(增强型接口)
** 参数:       [in] msgid：   协议ID
** 参数:       [in] attrib:   通道属性和重发属性,见PROTOCOL_COM_T
**             [in] period：  分包数据的分包间数据发送间隔,单位：秒
**             [in] waittime：数据发送后,等待中心应答时间,单位：秒
**             [in] ct_send： 重发次数,无需等待应答重传的可填为0
**             [in] sptr：    数据指针,可以为动态内存或者静态内存,如果是动态内存,将在本模块中释放
**             [in] slen：    数据长度,最大发送数据长度为(WIN_SIZE*SIZE_PACKET)
**             [in] fp：      发送结果回调函数
** 返回:       成功返回索引号，失败返回0, 失败情况下无fp回调
********************************************************************/
INT16U YX_WIN_SendDataEx(INT32U msgid, PROTOCOL_COM_T *attrib, INT8U period, INT16U waittime, INT16U ct_send, INT8U *sptr, INT32U slen, WIN_SEND_CALLBACK fp)
{
    INT8U channel, comattrib;
    INT16U winid;
    WINCELL_T *wincell;

    if ((sptr == 0) && (slen != 0)) {
        return 0;
    }
    
    comattrib = attrib->attrib;
    channel   = attrib->channel;
    if ((comattrib & SM_ATTR_SM) == 0) {
        if ((comattrib & SM_ATTR_TCP) != 0) {                                  /* TCP通道 */
            if (!YX_JTT_LinkCanCom(channel)) {
                return 0;
            }
        }
        
        if ((comattrib & SM_ATTR_UDP) != 0) {                                  /* UDP通道 */
            if (!YX_JTU_LinkCanCom(channel)) {
                return 0;
            }
        }
    }
    
    if ((wincell = (WINCELL_T *)YX_DelListHead(&s_winfreelist)) == 0) {
        #if DEBUG_GSMSTATUS > 0
        printf_com("<通信层发送链表节点已满>\r\n");
        #endif
        
        return 0;
    }
    YX_MEMSET(wincell, 0, sizeof(WINCELL_T));
    
    if (!YX_DM_IsDynamicMemory(sptr)) {                                        /* 非动态内存则需要申请动态内存 */
        wincell->ptr = YX_DYM_Alloc(slen);
        if (wincell->ptr == 0) {
            #if DEBUG_GSMSTATUS > 0
            printf_com("<通信层分配不到动态内存>\r\n");
            #endif
        
            YX_AppendListEle(&s_winfreelist, (LISTMEM *)wincell);
            return 0;
        } else {
            YX_MEMCPY(wincell->ptr, slen, sptr, slen);
        }
    } else {
        wincell->ptr     = sptr;
    }
    
    winid = GetWinID() + (INT32U)sptr;
    if (winid == 0) {
        winid = 1;
    }
    
    wincell->winid       = winid;
    wincell->msgid       = msgid;
    wincell->period      = 1;
    wincell->max_period  = period;
    wincell->max_wintime = waittime;
    wincell->max_winover = ct_send;
    wincell->len         = slen;
    wincell->fp          = fp;
    wincell->packetsize  = SIZE_PACKET;
    wincell->maxpacket   = (wincell->len + wincell->packetsize - 1) / wincell->packetsize;
    
    YX_MEMCPY(&(wincell->attrib), sizeof(wincell->attrib), attrib, sizeof(wincell->attrib));

    if (wincell->maxpacket == 0) {                                             /* 无消息体 */
        wincell->maxpacket = 1;
    }
    
    if (wincell->maxpacket > WIN_SIZE) {                                       /* 分包数超过最大分包数 */
        #if DEBUG_GSMSTATUS > 0
        printf_com("<通信层发送分包总数超出>\r\n");
        #endif
        
        YX_AppendListEle(&s_winfreelist, (LISTMEM *)wincell);
        return 0;
    }
    
    YX_AppendListEle(&s_winsendlist, (LISTMEM *)wincell);                      /* 将新增节点放入就绪链表 */
    
    if (!YX_TmrIsRun(s_scantmr)) {
        YX_StartTmr(s_scantmr, PERIOD_SCAN);
    }
    
    return winid;
}

/*******************************************************************
** 函数名:     YX_GetSeqMapWinid
** 函数描述:   根据确认帧流水号得到winid
** 参数:       [in]  ackseq：           确认帧流水号
** 返回:       winid
********************************************************************/
INT16U YX_GetSeqMapWinid(INT16U ackseq)
{
    WINCELL_T *wincell;
             
    wincell = (WINCELL_T *)YX_GetListHead(&s_winsendlist);    
    for (;;) {
        if (NULL == wincell) {
            break;
        }

        if (wincell->frameseq == ackseq) {                                          /* 发送窗口节点数不为0且已登陆行业中心GPRS前置机 */
            break;
        }
        wincell = (WINCELL_T *)YX_ListNextEle((LISTMEM * )wincell);
    }
    
    if (wincell == 0) {
        #if DEBUG_GSMSTATUS > 0
        printf_com("<在发送窗口中未找到需要应答的数据帧,frameseq(0x%x), msgid(0x%x)>", ackframeseq, ackmsgid);
        #endif
        
        return 0;
    }
    
    return wincell->winid;
}

/*******************************************************************
** 函数名:     YX_WIN_InformRecvComAck
** 函数描述:   中心的窗口确认应答(平台通用应答)
**             [in]  frameptr:  数据指针
**             [in]  framelen:  数据长度
** 返回:       成功 返回true, 失败返回false
********************************************************************/
BOOLEAN YX_WIN_InformRecvComAck(SYSFRAME_T *frameptr, INT16U framelen)
{
    INT8U result;
    INT16U  datalen, ackframeseq, ackmsgid, msgattrib, totalpkg, pkgseq;
    WINCELL_T *wincell;
    STREAM_T rstrm;

    datalen = framelen - SYSHEAD_LEN - SYSTAIL_LEN;
    if (datalen < 5) {
        return FALSE;
    }
    YX_InitStrm(&rstrm, (INT8U *)frameptr->data, datalen);
    
    msgattrib = (frameptr->msgattrib[0] << 8) + frameptr->msgattrib[1];        /* 消息属性 */
    if ((msgattrib & PROTOCOL_EXT_HEAD) != 0) {
        totalpkg = YX_ReadHWORD_Strm(&rstrm);
        pkgseq   = YX_ReadHWORD_Strm(&rstrm);
    } else {
        totalpkg = 1;
        pkgseq   = 1;
    }
    
    ackframeseq = YX_ReadHWORD_Strm(&rstrm);
    ackmsgid    = YX_ReadHWORD_Strm(&rstrm);
    result      = YX_ReadBYTE_Strm(&rstrm);

    wincell = (WINCELL_T *)YX_GetListHead(&s_winsendlist);
    for (;;) {
        if (wincell == 0) {
            break;
        }
        
        if (wincell->msgid == ackmsgid) {                                      /* 消息ID */
            if (ackframeseq >= wincell->frameseq && ackframeseq < (wincell->frameseq + wincell->maxpacket)) {
                totalpkg = wincell->maxpacket;
                pkgseq   = ackframeseq - wincell->frameseq + 1;                /* 分包包序号从1开始 */
                break;
            }
        }
        wincell = (WINCELL_T *)YX_ListNextEle((LISTMEM *)wincell);
    }
    
    if (wincell == 0) {
        #if DEBUG_GSMSTATUS > 0
        printf_com("<在发送窗口中未找到需要应答的数据帧,frameseq(0x%x), msgid(0x%x)>", ackframeseq, ackmsgid);
        #endif
        
        return FALSE;
    }
    
    if (result == 0x00) {                                                      /* 接收成功 */
        wincell->rwinflag[(pkgseq - 1) / 8] |= (1 << ((pkgseq - 1) % 8));      /* 帧序号从1 开始 */
    } else if (result == 0x01) {                                               /* 部分接收失败 */
        wincell->rwinflag[(pkgseq - 1) / 8] |= (1 << ((pkgseq - 1) % 8));
    } else if (result == 0x02) {                                               /* 全部重传 */
        wincell->rwinflag[(pkgseq - 1) / 8] |= (1 << ((pkgseq - 1) % 8));
    } else {
        YX_DelListEle(&s_winsendlist, (LISTMEM *)wincell);
        ClearWin(wincell, _FAILURE);
        return true;
    }
        
    if (AllSend(wincell)) {                                                    /* 全部发送完毕 */
        YX_DelListEle(&s_winsendlist, (LISTMEM *)wincell);
        ClearWin(wincell, _SUCCESS);
    }
    
    return TRUE;    
}

/*******************************************************************
** 函数名:     YX_WIN_InformRecvAck
** 函数描述:   业务上行收到中心非平台通用应答的应答
** 参数:       [in]  ackmsgid:  应答对应的发送帧
**             [in]  frameptr:  数据指针
**             [in]  framelen:  数据长度
** 返回:       成功 返回true, 失败返回false
********************************************************************/
BOOLEAN YX_WIN_InformRecvAck(INT16U ackmsgid, SYSFRAME_T *frameptr, INT16U framelen)
{
    INT16U  i, rs_num, index, datalen;
    WINCELL_T *wincell; 
    INT32U  s_media_id, r_media_id;
    INT16U  resendseq;
    STREAM_T rstrm;
    
    datalen = framelen - SYSHEAD_LEN - SYSTAIL_LEN;
    if (datalen < 3) {
        return FALSE;
    }
    YX_InitStrm(&rstrm, (INT8U *)frameptr->data, datalen);

    wincell = (WINCELL_T *)YX_GetListHead(&s_winsendlist);
    for (;;) {
        if (wincell == 0) {
            break;
        }
        
        
        if (ackmsgid == DN_CMD_RESEND) {                                       /* 分包补传请求 */
            resendseq = YX_ReadHWORD_Strm(&rstrm);                             /* 需要补传的数据帧的流水号 */
            if (resendseq == wincell->frameseq) {
                break;
            }
        } else if (ackmsgid == wincell->msgid) {
            if (ackmsgid == UP_CMD_PICAUDIO) {                                 /* 上传多媒体数据 */
                r_media_id = YX_ReadLONG_Strm(&rstrm);                         /* 读取多媒体文件索引号 */
                s_media_id = (wincell->ptr[0] << 24) + (wincell->ptr[1] << 16) + (wincell->ptr[2] << 8) + wincell->ptr[3];
                
                if (s_media_id == r_media_id) {                                /* 多媒体文件索引号 */
                    break;
                }
            }
        }
        
        wincell = (WINCELL_T *)YX_ListNextEle((LISTMEM *)wincell);
    }
    
    if (wincell == 0) {
        return FALSE;
    }
    
    if (framelen <= 3) {
        rs_num = 0;
    } else {
        rs_num = YX_ReadBYTE_Strm(&rstrm);
    }

    if (rs_num == 0) {                                                         /* 全部发送成功 */
        YX_DelListEle(&s_winsendlist, (LISTMEM *)wincell);
        ClearWin(wincell, _SUCCESS);
    } else {
        wincell->period = 1;
        for (i = 0; i < rs_num; i++) {
            index = YX_ReadHWORD_Strm(&rstrm);                                 /* 重传分包ID */
            if (index > 0 && index <= wincell->maxpacket) {
                wincell->rwinflag[(index - 1) / 8] &= ~(1 << ((index - 1) % 8));
                wincell->swinflag[(index - 1) / 8] &= ~(1 << ((index - 1) % 8));
            }
        }
    }
    return TRUE;
}



