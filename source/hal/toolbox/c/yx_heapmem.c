/********************************************************************************
**
** 文件名:     yx_heapmem.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现堆内存管理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2014/06/15 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "hal_include.h"
#include "os_timer.h"
#include "os_errman.h"
#include "yx_heapmem.h"
#include "yx_debug.h"

/*
********************************************************************************
* 定义模块配置参数
********************************************************************************
*/
#define DEBUG_HEAPMEM        0

#define _SIGNATURE           0xAA
#define AREA_FLAG            0xAA

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct blockhead_t {
    struct blockhead_t *prev;                              /* 链表指针 */
    struct blockhead_t *next;                              /* 链表指针 */
    
    INT8U  signature;                                      /* 固定为_SIGNATURE */
    INT8U  allocated;                                      /* 此区域是否已经分配出去：0-N，1-Y */ 
    INT16U size;                                           /* 此区域大小 */
} BLOCKHEAD_T;

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static INT32U s_mempool[(SIZE_HEAP_MEM + 3) / 4];
static HEAPMEM_STATISTICS_T s_statictis;



/*******************************************************************
** 函数名:     YX_HEAPMEM_Init
** 函数描述:   初始化堆内存管理模块
** 参数:       无
** 返回:       无
********************************************************************/
void YX_HEAPMEM_Init(void)
{
    BLOCKHEAD_T *blockptr;

    YX_MEMSET(&s_statictis, 0, sizeof(s_statictis));
    //YX_MEMSET(s_mempool, 0, sizeof(s_mempool));
     
    blockptr            = (BLOCKHEAD_T *)s_mempool;                            /* 获取整块内存池的起始地址 */
	blockptr->next      = 0;
    blockptr->prev      = 0;
    blockptr->allocated = FALSE;
    blockptr->signature = _SIGNATURE;
    blockptr->size      = sizeof(s_mempool) - sizeof(BLOCKHEAD_T);             /* 计算内存池中可用的容量大小 */
    
    s_statictis.blocks++;                                                      /* 初始化内存块数量 */
}

/*******************************************************************
** 函数名:     YX_HEAPMEM_Alloc
** 函数描述:   申请分配内存接口
** 参数:       [in] datalen: 申请内存的长度
**             [in] overtime:内存使用时间，单位：秒
**             [in] file:    申请内存的文件名
**             [in] line:    申请内存的行号
** 返回:       成功返回内存指针，失败返回0
********************************************************************/
void *HEAPMEM_Alloc(INT32U datalen, INT16U overtime, char *file, INT32U line)
{
    BLOCKHEAD_T *blockptr = (BLOCKHEAD_T *)s_mempool;                    /* 指向内存池的起始位置 */
    BLOCKHEAD_T *newblock;                                             /* 表示本次操作完后产生的新块(即剩余的内存块) */
     
    if (datalen == 0 || datalen >= SIZE_HEAP_MEM) {
        #if DEBUG_HEAPMEM > 0
	    printf_com("<申请内存值有误，无法分配内存>\r\n");
        #endif
        
	    return 0;
	}
	
	datalen = ((datalen + 3) / 4) * 4;
     
    while (blockptr != 0) {
	    if (blockptr->allocated == FALSE && blockptr->size >= datalen) {
		    blockptr->allocated = _SIGNATURE;
			if ((blockptr->size - datalen) > sizeof(BLOCKHEAD_T)) {
			    newblock            = (BLOCKHEAD_T *)(((INT8U *)blockptr) + sizeof(BLOCKHEAD_T) + datalen); /* 扣掉帧头偏移量，再扣掉本次需要分配的量 */
                newblock->signature = _SIGNATURE;
                newblock->size      = blockptr->size - datalen - sizeof(BLOCKHEAD_T);/* 剩下的内存块的容量 */
                newblock->allocated = FALSE;
				newblock->prev      = blockptr;                                /* 将新块链接到本次申请的这个块后面 */
                newblock->next      = blockptr->next;                          /* 将本次申请的块插入到剩下的块之前 */

                if (blockptr->next != 0) {
				    blockptr->next->prev = newblock;
				}

                blockptr->next      = newblock;                                /* 将本次申请的块的下一指针指向剩余的内存块 */
                blockptr->size      = datalen;                                 /* 指定本次申请的内存块大小 */
                s_statictis.blocks++;                                          /* 所分配的内存块数累加 */
            }
            
            s_statictis.occupysize += blockptr->size;                          /* 将本次所申请掉的内存容量计入已经使用的范围 */
            break;
        }
        blockptr = blockptr->next;                                             /* 否则，本块不满足分配要求，判断下一块 */
    }

    #if DEBUG_HEAPMEM > 0
    if (blockptr == 0) {
	    printf_com("<分配内存失败:申请值(%d), 剩余内存(%d), 已使用(%d)>\r\n", datalen,  SIZE_HEAP_MEM - s_statictis.occupysize, s_statictis.occupysize);
    } else {
        printf_com("<分配内存成功:分配值(%d), 内存块总数(%d), 已使用(%d)>\r\n", blockptr->size, s_statictis.blocks, s_statictis.occupysize);
	}
    #endif

    return (blockptr != 0) ? (((INT8U *)(blockptr)) + sizeof(BLOCKHEAD_T)) : 0;/* 返回所申请的内存块的有效地址值 */
}

/*******************************************************************
** 函数名:     YX_HEAPMEM_Free
** 函数描述:   释放内存接口
** 参数:       [in] sptr: 释放内存的地址
**             [in] file: 申请内存的文件名
**             [in] line: 申请内存的行号
** 返回:       无
********************************************************************/
void HEAPMEM_Free(void *sptr, char *file, INT32U line)
{
    BLOCKHEAD_T *blockptr = 0;
    BLOCKHEAD_T *prevblock = 0;
    BLOCKHEAD_T *backblock = 0 ;
    BLOCKHEAD_T *nextbackblock = 0;
	
	#if DEBUG_HEAPMEM > 0
	INT16U len;
	#endif
    
    OS_ASSERT((sptr != 0), RETURN_VOID);
    blockptr = (BLOCKHEAD_T *)(((INT8U *)(sptr)) - sizeof(BLOCKHEAD_T));       /* 将指针指向所需要释放的内存块的起始地址 */
    
    OS_ASSERT((blockptr->signature == _SIGNATURE), RETURN_VOID);
    OS_ASSERT((blockptr->allocated == _SIGNATURE), RETURN_VOID);

    blockptr->allocated = FALSE;
    s_statictis.occupysize -= blockptr->size;                                  /* 从已使用的内存值总量中扣去本次释放的值 */

	#if DEBUG_HEAPMEM > 0
	len = blockptr->size;
	#endif
    
	prevblock = blockptr->prev;                                                /* 获取前块指针的起始地址 */
    backblock = blockptr->next;                                                /* 获取后块指针的起始地址 */

    if (blockptr->prev != 0) {                                                 /* 如果前面有其他内存块，进行前项合并 */
		if (prevblock->allocated == FALSE) {
            prevblock->size = prevblock->size + blockptr->size + sizeof(BLOCKHEAD_T);      /* 将本次释放的块大小归并到前块中 */
            prevblock->next = blockptr->next;                                  /* 将后块与前块串接起来 */
            if (backblock != 0) {
                backblock->prev = prevblock;                                   /* 将前块与后块串接起来 */
            }
            s_statictis.blocks--;                                              /* 将所分配的内存块数累减 */

			#if DEBUG_HEAPMEM > 0
            printf_com("<与前项合并成功!>\r\n");
            #endif
        }    
    } 
    
    if (blockptr->next != 0) {                                                 /* 如果合并完后的内存块后面有还有其他内存块，再进行后项合并 */
        if (prevblock != 0) {
            if (prevblock->next == blockptr->next) {
                blockptr = prevblock;                                          /* 获取前面内存块的起始地址 */
            }
        }
        backblock = blockptr->next;                                            /* 获取后面内存块的起始地址 */
        nextbackblock = backblock->next;                                       /* 获取再下一个内存块的起始地址 */
        if (backblock->allocated == FALSE) {
            blockptr->size = blockptr->size + backblock->size + sizeof(BLOCKHEAD_T);       /* 将两个内存块的大小合并 */
            blockptr->next = backblock->next;                                  /* 指定合并后的内存块的下一指针 */
            if (nextbackblock != 0) {
                nextbackblock->prev = blockptr;                                /* 将本块内存与原本的再下一个内存块链接起来 */
            }
            s_statictis.blocks--;                                              /* 将所分配的内存块数累减 */

			#if DEBUG_HEAPMEM > 0
            printf_com("<与后项合并成功!>\r\n");
            #endif
        }    
    }
    
    if (s_statictis.occupysize == 0 && s_statictis.blocks != 1) {
	    OS_ASSERT((0), RETURN_VOID);
	}

    #if DEBUG_HEAPMEM > 0
	printf_com("<释放内存: 释放值(%d), 内存块总数(%d), 已使用大小(%d)>\r\n", len, s_statictis.blocks, s_statictis.occupysize);
    #endif
}

/*******************************************************************
** 函数名:     YX_HEAPMEM_GetStatistics
** 函数描述:   查询内存池当前使用状态接口
** 参数:       无
** 返回:       成功返回返回统计信息，失败返回0
********************************************************************/
HEAPMEM_STATISTICS_T *YX_HEAPMEM_GetStatistics(void)
{
    return &s_statictis;
}
