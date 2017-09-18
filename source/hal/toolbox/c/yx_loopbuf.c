/******************************************************************************
**
** Filename:     yx_loopbuf.c
** Copyright:    
** Description:  该模块主要实现环形缓冲区管理
**
-------------------------------------------------------------------------------
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#include "hal_include.h"
#include "yx_loopbuf.h"



/*
********************************************************************************
* define config parameters
********************************************************************************
*/

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



#pragma O0

/*-------------------------------------------------------------------
** 函数名称:   YX_InitLoopBuffer
** 函数描述:   初始化循环缓冲区
** 参数:       [in]  loop:    循环缓冲区管理结构体
**             [in]  memptr:  缓冲区首地址
**             [in]  memsize: 缓冲区长度
** 返回:       无
-------------------------------------------------------------------*/
void YX_InitLoopBuffer(LOOP_BUF_T *loop, INT8U *memptr, INT32U memsize)
{
    loop->memsize = memsize;
    loop->used    = 0;
    loop->bptr    = memptr;
    loop->eptr    = memptr + memsize;
    loop->wptr    = loop->bptr;
    loop->rptr    = loop->bptr;
}

/*-------------------------------------------------------------------
** 函数名称:   YX_ClearLoopBuffer
** 函数描述:   恢复缓冲区为初始化状态
** 参数:       [in]  loop:    循环缓冲区管理结构体
** 返回:       无
-------------------------------------------------------------------*/
void YX_ClearLoopBuffer(LOOP_BUF_T *loop)
{
    volatile INT32U cpu_sr;
    
    OS_ENTER_CRITICAL();                            /* 关中断 */
    loop->used = 0;
    loop->rptr = loop->bptr;
    loop->wptr = loop->bptr;
    OS_EXIT_CRITICAL();                             /* 开中断 */
}

/*-------------------------------------------------------------------
** 函数名称:   YX_GetLoopBufferStartPtr
** 函数描述:   获取循环缓冲区首地址
** 参数:       [in]  loop:    循环缓冲区管理结构体
** 返回:       循环缓冲区首地址
-------------------------------------------------------------------*/
INT8U *YX_GetLoopBufferStartPtr(LOOP_BUF_T *loop)
{
    return loop->bptr;
}

/*-------------------------------------------------------------------
** 函数名称:   YX_WriteLoopBuffer
** 函数描述:   往循环缓冲区中写入一个字节的数据
** 参数:       [in]  loop:   循环缓冲区管理结构体
**             [in]  indata: 字节数据
** 返回:       成功返回true,失败返回false
-------------------------------------------------------------------*/
BOOLEAN YX_WriteLoopBuffer(LOOP_BUF_T *loop, INT8U indata)
{
    volatile INT32U cpu_sr;
    
    if (loop == 0) {
        return FALSE;
    }
    
    OS_ENTER_CRITICAL();                            /* 关中断 */
    if (loop->used >= loop->memsize) {
        OS_EXIT_CRITICAL();                         /* 开中断 */
        return FALSE;
    } else {
        OS_EXIT_CRITICAL();                         /* 开中断 */
    }
    
    *loop->wptr++ = indata;
    if (loop->wptr >= loop->eptr) {
        loop->wptr = loop->bptr;
    }
    OS_ENTER_CRITICAL();                            /* 关中断 */
    loop->used++;
    OS_EXIT_CRITICAL();                             /* 开中断 */
    return TRUE;
}

/*-------------------------------------------------------------------
** 函数名称:   YX_WriteLoopBuffer_INT
** 函数描述:   往循环缓冲区中写入一个字节的数据,无中断保护
** 参数:       [in]  loop:   循环缓冲区管理结构体
**             [in]  indata: 字节数据
** 返回:       成功返回true,失败返回false
-------------------------------------------------------------------*/
BOOLEAN YX_WriteLoopBuffer_INT(LOOP_BUF_T *loop, INT8U indata)
{
    if (loop == 0) {
        return FALSE;
    }
    
    if (loop->used >= loop->memsize) {
        return FALSE;
    }
    
    *loop->wptr++ = indata;
    if (loop->wptr >= loop->eptr) {
        loop->wptr = loop->bptr;
    }

    loop->used++;
    return TRUE;
}

/*-------------------------------------------------------------------
** 函数名称:   YX_WriteBlockLoopBuffer
** 函数描述:   写入一串数据到环形缓冲区
** 参数:       [in]  loop:    循环缓冲区管理结构体
**             [in]  bptr:    数据块的指针
**             [in] len:      数据块的字节数
** 返回:       成功返回true, 失败返回false
-------------------------------------------------------------------*/
BOOLEAN YX_WriteBlockLoopBuffer(LOOP_BUF_T *loop, INT8U *bptr, INT32U len)
{
    INT32U i;
    volatile INT32U cpu_sr;
    
    if (len > YX_LeftOfLoopBuffer(loop)) {
        return FALSE;
    }
    
    for (i = 0; i < len; i++) {
        *loop->wptr++ = *bptr++;
        if (loop->wptr >= loop->eptr) {
            loop->wptr = loop->bptr;
        }
        OS_ENTER_CRITICAL();                            /* 关中断 */
        loop->used++;
        OS_EXIT_CRITICAL();                             /* 开中断 */
    }    
    return TRUE;
}

/*-------------------------------------------------------------------
** 函数名称:   YX_WriteBlockLoopBuffer_INT
** 函数描述:   写入一串数据到环形缓冲区,无中断保护
** 参数:       [in]  loop:    循环缓冲区管理结构体
**             [in]  bptr:    数据块的指针
**             [in] len:      数据块的字节数
** 返回:       成功返回true, 失败返回false
-------------------------------------------------------------------*/
BOOLEAN YX_WriteBlockLoopBuffer_INT(LOOP_BUF_T *loop, INT8U *bptr, INT32U len)
{
    INT32U i, temp;

    temp = loop->memsize - loop->used;
    if (len > temp) {
        len = temp;
    }
    
    for (i = 0; i < len; i++) {
        *loop->wptr++ = *bptr++;
        if (loop->wptr >= loop->eptr) {
            loop->wptr = loop->bptr;
        }
        loop->used++;
    }    
    return TRUE;
}

/*-------------------------------------------------------------------
** 函数名称:   YX_ReadLoopBuffer
** 函数描述:   从循环缓冲区中读取一个字节的数据
** 参数:       [in]  loop:    循环缓冲区管理结构体
** 返回:       成功返回数据值,失败返回-1
-------------------------------------------------------------------*/
INT32S YX_ReadLoopBuffer(LOOP_BUF_T *loop)
{
    INT32S ret;
    volatile INT32U cpu_sr;
    
    OS_ENTER_CRITICAL();                            /* 关中断 */
    if (loop->used == 0) {
        OS_EXIT_CRITICAL();                         /* 开中断 */
        return -1;
    } else {
        OS_EXIT_CRITICAL();                         /* 开中断 */
    }
    
    ret = *loop->rptr++;
    if (loop->rptr >= loop->eptr) {
        loop->rptr = loop->bptr;
    }
    
    OS_ENTER_CRITICAL();                            /* 关中断 */
    loop->used--;
    OS_EXIT_CRITICAL();                             /* 开中断 */
    return ret;
}

/*-------------------------------------------------------------------
** 函数名称:   YX_ReadLoopBuffer_INT
** 函数描述:   从循环缓冲区中读取一个字节的数据,无中断保护
** 参数:       [in]  loop:    循环缓冲区管理结构体
** 返回:       成功返回数据值,失败返回-1
-------------------------------------------------------------------*/
INT32S YX_ReadLoopBuffer_INT(LOOP_BUF_T *loop)
{
    INT32S ret;
    
    if (loop->used == 0) {
        return -1;
    }
    
    ret = *loop->rptr++;
    if (loop->rptr >= loop->eptr) {
        loop->rptr = loop->bptr;
    }
    
    loop->used--;
    return ret;
}

/*-------------------------------------------------------------------
** 函数名称:   YX_LeftOfLoopBuffer
** 函数描述:   获取循环缓冲区中剩余的空间
** 参数:       [in]  loop:    循环缓冲区管理结构体
** 返回:       剩余空间字节数
-------------------------------------------------------------------*/
INT32U YX_LeftOfLoopBuffer(LOOP_BUF_T *loop)
{
    INT32U temp;
    volatile INT32U cpu_sr;
    
    OS_ENTER_CRITICAL();                            /* 关中断 */
    temp = loop->memsize - loop->used;
    OS_EXIT_CRITICAL();                             /* 开中断 */
    
    return temp;
}

/*-------------------------------------------------------------------
** 函数名称:   YX_UsedOfLoopBuffer
** 函数描述:   获取循环缓冲区中已使用空间
** 参数:       [in]  loop:    循环缓冲区管理结构体
** 返回:       已使用空间字节数
-------------------------------------------------------------------*/
INT32U YX_UsedOfLoopBuffer(LOOP_BUF_T *loop)
{
    INT32U temp;
    volatile INT32U cpu_sr;
    
    OS_ENTER_CRITICAL();                            /* 关中断 */
    temp = loop->used;
    OS_EXIT_CRITICAL();                             /* 开中断 */
    
    return temp;
}




