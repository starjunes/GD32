/******************************************************************************
**
** Filename:     yx_dynmem.h
** Copyright:    
** Description:  该模块主要实现动态内存管理
**
-------------------------------------------------------------------------------
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#ifndef YX_DYNMEM_H
#define YX_DYNMEM_H


#include "yx_heapmem.h"

/*
********************************************************************************
* define config parameters
********************************************************************************
*/
#define DYM_OT_NULL          0
#define DYM_OT_5S            5

/*
********************************************************************************
* define struct
********************************************************************************
*/
typedef struct {
    INT32U usedsize;         /* 累计申请的总大小 */
    INT32U freesize;         /* 累计释放的总大小 */
    
    INT32U cur_size;         /* 当前已申请总大小 */
    INT32U peak_size;        /* 申请内存的峰值 */
    
    INT32U total_used[10];    /* 每种内存池累计申请的总次数 */
    INT32U total_free[10];    /* 每种内存池累计释放的总次数 */
    
    INT32U cur_used[10];      /* 每种内存池当前已使用的内存数 */
    INT32U peak_used[10];     /* 每种内存池最大使用个数峰值 */
} DYM_STATISTICS_T;




/*-------------------------------------------------------------------
** 函数名:     YX_DYM_Alloc
** 函数描述:   申请分配内存接口
** 参数:       [in] datalen: 申请长度
**             [in] overtime:内存限制时间，单位：秒
**             [in] file:    申请内存函数的文件名
**             [in] line:    申请内存函数的行号
** 返回:       成功返回内存指针，失败返回0
-------------------------------------------------------------------*/
void *DYM_Alloc(INT32U datalen, INT16U overtime, char *file, INT32U line);
//#define YX_DYM_Alloc(SIZE)             DYM_Alloc(SIZE, DYM_OT_NULL, __FILE__, __LINE__)
//#define YX_DYM_AllocEx(SIZE, OVERTIME) DYM_Alloc(SIZE, OVERTIME, __FILE__, __LINE__)

#define YX_DYM_Alloc(SIZE)             HEAPMEM_Alloc(SIZE, DYM_OT_NULL, __FILE__, __LINE__)
#define YX_DYM_AllocEx(SIZE, OVERTIME) HEAPMEM_Alloc(SIZE, OVERTIME, __FILE__, __LINE__)

/*-------------------------------------------------------------------
** 函数名:     YX_DYM_Free
** 函数描述:   释放内存接口
** 参数:       [in] sptr: 释放内存的地址
**             [in] file: 申请内存的文件名
**             [in] line: 申请内存的行号
** 返回:       无
-------------------------------------------------------------------*/
void DYM_Free(void *sptr, char *file, INT32U line);
//#define YX_DYM_Free(PTR)    DYM_Free(PTR, __FILE__, __LINE__)
#define YX_DYM_Free(PTR)    HEAPMEM_Free(PTR, __FILE__, __LINE__)

/*-------------------------------------------------------------------
** 函数名:     YX_DYM_Init
** 函数描述:   初始化内存池，把各种类型内存池使用单项链表串联起来
** 参数:       无
** 返回:       无
-------------------------------------------------------------------*/
void YX_DYM_Init(void);

/*-------------------------------------------------------------------
** 函数名:     YX_DYM_GetStatistics
** 函数描述:   查询内存池当前使用状态接口
** 参数:       无
** 返回:       成功返回返回统计信息，失败返回0
-------------------------------------------------------------------*/
DYM_STATISTICS_T *YX_DYM_GetStatistics(void);

#endif
