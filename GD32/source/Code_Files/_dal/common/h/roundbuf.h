/********************************************************************************
**
** 文件名:     roundbuf.h
** 版权所有:   (c) 2014-2020 厦门雅迅网络股份有限公司
** 文件描述:   环形缓冲区管理模块
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2014/08/21 | 张鹏   |  创建该文件
**
*********************************************************************************/
#ifndef H_MMI_ROUNDBUF_H
#define H_MMI_ROUNDBUF_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "dal_structs.h"

/*************************************************************************************************/
/*                           环形缓冲区控制块                                                    */
/*************************************************************************************************/
typedef struct {
    INT32U         bufsize;            /* round buffer size */
    INT32U         used;               /* used bytes */
    INT8U          *bptr;              /* begin position */
    INT8U          *eptr;              /* end position */
    INT8U          *wptr;              /* write position */
    INT8U          *rptr;              /* read position */
    ASMRULE_T      *rule;              /* assemble rules */
} ROUNDBUF_T;

void    InitRoundBuf(ROUNDBUF_T *round, INT8U *mem, INT32U memsize, ASMRULE_T *rule);
void    ResetRoundBuf(ROUNDBUF_T *round);
INT8U  *RoundBufStartPos(ROUNDBUF_T *round);
BOOLEAN WriteRoundBuf(ROUNDBUF_T *round, INT8U data);
INT32S  ReadRoundBuf(ROUNDBUF_T *round);
INT32S  ReadRoundBufNoMVPtr(ROUNDBUF_T *round);
INT32U  LeftOfRoundBuf(ROUNDBUF_T *round);
INT32U  UsedOfRoundBuf(ROUNDBUF_T *round);
BOOLEAN WriteBlockRoundBuf(ROUNDBUF_T *round, INT8U *bptr, INT32U blksize);
/**************************************************************************************************
**  函数名称:  UsedOfRoundBuf
**  功能描述:  获取缓冲区已用空间
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
__INLINE INT32U UsedOfRoundBuf(ROUNDBUF_T *round)
{
    return round->used;
}
#ifdef __cplusplus
}
#endif

#endif

