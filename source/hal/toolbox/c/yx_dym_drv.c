/******************************************************************************
**
** Filename:     yx_dynmem.c
** Copyright:    
** Description:  该模块主要实现动态内存管理
**
-------------------------------------------------------------------------------
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#include "hal_include.h"
#include "os_timer.h"
#include "os_errman.h"
#include "yx_dym_drv.h"
#include "yx_dym_cfg.h"

/*
********************************************************************************
* define config parameters
********************************************************************************
*/
#define AREA_FLAG            0xAA
#define FLAG_USED            'U'
#define FLAG_FREE            'F'

#define PERIOD_SCAN          _SECOND, 1
/*
********************************************************************************
* define struct
********************************************************************************
*/



/*
********************************************************************************
* define module variants
********************************************************************************
*/
static INT8U s_scantmr;
#if EN_DYM_STATISTICS > 0
static DYM_STATISTICS_T s_statictis;
#endif



/*-------------------------------------------------------------------
** 函数名:     ScanTmrProc
** 函数描述:   定时器
** 参数:       [in] index:定时器标识
** 返回:       无
-------------------------------------------------------------------*/
static void ScanTmrProc(void *index)
{
    INT8U i, num;
    DYM_POOL_T const *preg;
    DYM_HEAD_T *phead;
    
    index = index;
    
    OS_StartTmr(s_scantmr, PERIOD_SCAN);
    
    num = YX_DYM_GetRegPoolNum();
    for (i = 0; i < num; i++) {
        preg = YX_DYM_GetRegPoolInfo(i);
        
        phead = (DYM_HEAD_T *)YX_LIST_GetListHead(preg->usedlist);
        for (;;) {
            if (phead == 0) {
                break;
            }
            
            if (phead->maxtime > 0) {
                if (++phead->ct_over >= phead->maxtime) {
                    phead->ct_over = 0;
                    //OS_ASSERT(0, RETURN_VOID);
                }
            }
            phead = (DYM_HEAD_T *)YX_LIST_GetNextEle((INT8U *)phead);
        }
    }
}

/*-------------------------------------------------------------------
** 函数名:     DiagnoseProc
** 函数描述:   链表检查函数
** 参数:       无
** 返回:       无
-------------------------------------------------------------------*/
static void DiagnoseProc(void)
{
    INT8U i, num, count;
    DYM_POOL_T const *preg;
    
    OS_ASSERT(OS_TmrIsRun(s_scantmr), RETURN_VOID);
    
    num = YX_DYM_GetRegPoolNum();
    for (i = 0; i < num; i++) {
        preg = YX_DYM_GetRegPoolInfo(i);
        
        count = YX_LIST_GetNodeNum(preg->freelist) + YX_LIST_GetNodeNum(preg->usedlist);
        OS_ASSERT((count == preg->maxnum), RETURN_VOID);
    }
}

/*-------------------------------------------------------------------
** 函数名:     YX_DYM_Init
** 函数描述:   初始化内存池，把各种类型内存池使用单项链表串联起来
** 参数:       无
** 返回:       无
-------------------------------------------------------------------*/
void YX_DYM_Init(void)
{
    INT8U i, type, num, memnum;
    INT8U *pmem;
    DYM_POOL_T const *preg;
    DYM_HEAD_T *phead;

#if EN_DYM_STATISTICS > 0
    YX_MEMSET(&s_statictis, 0, sizeof(s_statictis));
#endif

    num = YX_DYM_GetRegPoolNum();
    for (i = 0; i < num; i++) {
        preg = YX_DYM_GetRegPoolInfo(i);
        
        pmem   = preg->memptr;
        memnum = preg->maxnum;
        type   = preg->type;
        
        YX_LIST_Init(preg->usedlist);
        YX_LIST_CreateList(preg->freelist, (INT8U *)pmem, memnum, preg->totalsize);
        
        phead = (DYM_HEAD_T *)YX_LIST_GetListHead(preg->freelist);
        for (;;) {
            if (phead == 0) {
                break;
            }
            phead->type      = type;
            phead->useflag   = FLAG_FREE;
            phead->startflag = AREA_FLAG;
            *(INT8U *)(((INT32U)phead) + sizeof(DYM_HEAD_T) + preg->datasize) = AREA_FLAG;
            
            phead = (DYM_HEAD_T *)YX_LIST_GetNextEle((INT8U *)phead);
        }
    }
    
    s_scantmr  = OS_CreateTmr(TSK_ID_HAL, (void *)0, ScanTmrProc);
    OS_StartTmr(s_scantmr, PERIOD_SCAN);
    
    OS_RegistDiagnoseProc(DiagnoseProc);
}

/*-------------------------------------------------------------------
** 函数名:     YX_DYM_Alloc
** 函数描述:   申请分配内存接口
** 参数:       [in] datalen: 申请内存的长度
**             [in] overtime:内存使用时间，单位：秒
**             [in] file:    申请内存的文件名
**             [in] line:    申请内存的行号
** 返回:       成功返回内存指针，失败返回0
-------------------------------------------------------------------*/
void *DYM_Alloc(INT32U datalen, INT16U overtime, char *file, INT32U line)
{
    INT8U i, num;
    INT8U *ptr;
    DYM_POOL_T const *preg;
    DYM_HEAD_T *phead;

    ptr = 0;
    
    num = YX_DYM_GetRegPoolNum();
    for (i = 0; i < num; i++) {
        preg = YX_DYM_GetRegPoolInfo(i);
        if (datalen <= preg->datasize) {                                       /* 从最小开始申请 */
            phead = (DYM_HEAD_T *)YX_LIST_DeleListHead(preg->freelist);

            if (phead != 0) {
                phead->useflag = FLAG_USED;
                phead->maxtime = overtime;
                phead->ct_over = 0;
                ptr = (INT8U *)((INT32U)phead) + sizeof(DYM_HEAD_T);
                
                YX_LIST_AppendListEle(preg->usedlist, (INT8U *)phead);
                break;
            }
        }
    }
    
    if (ptr != 0) {
#if EN_DYM_STATISTICS > 0
        s_statictis.usedsize += preg->datasize;
        s_statictis.cur_size += preg->datasize;
        s_statictis.cur_used[phead->type]++;
        s_statictis.total_used[phead->type]++;
        
        if (s_statictis.cur_size > s_statictis.peak_size) {
            s_statictis.peak_size = s_statictis.cur_size;
        }
        
        if (s_statictis.cur_used[phead->type] > s_statictis.peak_used[phead->type]) {
            s_statictis.peak_used[phead->type] = s_statictis.cur_used[phead->type];
        }
#endif
        #if DEBUG_DM > 0
        printf_com("<alloc file(%s),line(%d),addr(0x%x)>\r\n", (file + YX_STRLEN(file) - 15), line, (INT32U)ptr);
        #endif
    }
    return ptr;
}

/*-------------------------------------------------------------------
** 函数名:     YX_DYM_Free
** 函数描述:   释放内存接口
** 参数:       [in] sptr: 释放内存的地址
**             [in] file: 申请内存的文件名
**             [in] line: 申请内存的行号
** 返回:       无
-------------------------------------------------------------------*/
void DYM_Free(void *sptr, char *file, INT32U line)
{
    INT8U num;
    DYM_POOL_T const *preg;
    DYM_HEAD_T *phead;
    
    #if DEBUG_DM > 0
    printf_com("<free file(%s),line(%d),addr(0x%x)>\r\n", (file + YX_STRLEN(file) - 15), line, (INT32U)sptr);
    #endif
    
    OS_ASSERT((((INT32U)sptr & 0x03) == 0), RETURN_VOID);
    
    num = YX_DYM_GetRegPoolNum();
    phead = (DYM_HEAD_T *)(((INT32U)sptr) - sizeof(DYM_HEAD_T));
    
    OS_ASSERT(((phead->useflag == FLAG_USED) && (phead->startflag == AREA_FLAG)), RETURN_VOID);
    OS_ASSERT((phead->type < num), RETURN_VOID);
    
    preg = YX_DYM_GetRegPoolInfo(phead->type);
    
    OS_ASSERT((*(INT8U *)(((INT32U)sptr) + preg->datasize) == AREA_FLAG), RETURN_VOID);
    
    phead->useflag = FLAG_FREE;
    YX_LIST_DeleListEle(preg->usedlist, (INT8U *)phead);
    YX_LIST_AppendListEle(preg->freelist, (INT8U *)phead);
    
#if EN_DYM_STATISTICS > 0
    s_statictis.freesize += preg->datasize;
    s_statictis.cur_size -= preg->datasize;
    s_statictis.cur_used[phead->type]--;
    s_statictis.total_free[phead->type]++;
#endif
}

/*-------------------------------------------------------------------
** 函数名:     YX_DYM_GetStatistics
** 函数描述:   查询内存池当前使用状态接口
** 参数:       无
** 返回:       成功返回返回统计信息，失败返回0
-------------------------------------------------------------------*/
DYM_STATISTICS_T *YX_DYM_GetStatistics(void)
{
#if EN_DYM_STATISTICS > 0
    return &s_statictis;
#else
    return 0;
#endif
}
    
    
    
    




