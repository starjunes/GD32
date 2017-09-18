/******************************************************************************
**
** filename:     os_struct.h
** copyright:    
** description:  该模块主要定义通用数据结构
**
-------------------------------------------------------------------------------
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#ifndef OS_STRUCT_H
#define OS_STRUCT_H

#include "os_type.h"

/*
********************************************************************************
*  定义联合体
********************************************************************************
*/
typedef union {
	INT16U     hword;
#ifdef __BIG_ENDIAN
	struct {
		INT8U  high;
		INT8U  low;
	} bytes;
#else
	struct {
		INT8U  low;
		INT8U  high;
	} bytes;
#endif
} HALFWORD_UNION;

typedef union {
    INT32U ulong;
#ifdef  __BIG_ENDIAN   
    struct {
		INT8U  byte1;
		INT8U  byte2;
        INT8U  byte3;
		INT8U  byte4;
	} bytes;
#else
    struct {
		INT8U  byte4;
		INT8U  byte3;
        INT8U  byte2;
		INT8U  byte1;
	} bytes;
#endif
} LONGWORD_UNION;

/*
********************************************************************************
*  定义日期时间
********************************************************************************
*/
typedef struct {
    INT8U year;
    INT8U month;
    INT8U day;
} DATE_T;

typedef struct {
    INT8U hour;
    INT8U minute;
    INT8U second;
} TIME_T;

typedef struct {
    DATE_T date;
    TIME_T time;
} SYSTIME_T;

/*
********************************************************************************
*  定义处理函数结构
********************************************************************************
*/
typedef struct {
    INT32U index;
    void   (*entryproc)(void);
} FUNCTION_T;

typedef struct {
    INT32U index;
    void   (*entryproc)(INT32U type, INT8U *data, INT16U datalen);
} FUNCTION_PARA_T;

/*
********************************************************************************
*                  DEFINE ASMRULE_STRUCT
********************************************************************************
*/
typedef struct {
    INT8U       c_flags;                /* c_convert0 + c_convert1 = c_flags */
    INT8U       c_convert0;             /* c_convert0 + c_convert2 = c_convert0 */
    INT8U       c_convert1;
    INT8U       c_convert2;
} ASMRULE_T;



#endif
/*************END OF FILE****/

