/**************************************************************************************************
**                                                                                               **
**  文件名称:  Stream.H                                                                          **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2010                                    **
**  创建信息:  LEON -- 2010年12月1日                                                             **
**  文件描述:  数据流管理模块                                                                    **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/
#ifndef __STREAM_H
#define __STREAM_H

#include "dal_include.h"

/*************************************************************************************************/
/*                           数据流控制块                                                        */
/*************************************************************************************************/
typedef struct {
    INT32U       MaxLen;               /* 数据流所管理内存字节数 */
    INT8U       *StartPtr;             /* 数据流所管理内存地址 */
    INT32U       Len;                  /* 已读/写字节数 */
    INT8U       *CurPtr;               /* 读/写指针 */
} STREAM_T;

BOOLEAN Strm_Init(STREAM_T *Sp, void *Bp, INT32U MaxLen);
INT32U  Strm_GetLeaveLen(STREAM_T *Sp);
INT32U  Strm_GetLen(STREAM_T *Sp);
void   *Strm_GetCurPtr(STREAM_T *Sp);
void   *Strm_GetStartPtr(STREAM_T *Sp);
BOOLEAN Strm_MovPtr(STREAM_T *Sp, INT32U Len);
BOOLEAN Strm_WriteHEX(STREAM_T *Sp, INT8U writebyte);
BOOLEAN Strm_WriteBYTE(STREAM_T *Sp, INT8U writebyte);
BOOLEAN Strm_WriteWORD(STREAM_T *Sp, INT16U writeword);
BOOLEAN Strm_WriteLONG(STREAM_T *Sp, INT32U writelong);
BOOLEAN Strm_WriteLF(STREAM_T *Sp);
BOOLEAN Strm_WriteSTR(STREAM_T *Sp, char *Ptr);
BOOLEAN Strm_WriteDATA(STREAM_T *Sp, INT8U *Ptr, INT16U Len);
INT8U   Strm_ReadBYTE(STREAM_T *Sp);
INT16U  Strm_ReadWORD(STREAM_T *Sp);
void    Strm_ReadDATA(STREAM_T *Sp, INT8U *Ptr, INT16U Len);

#endif

