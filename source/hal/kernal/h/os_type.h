/******************************************************************************
**
** filename:     os_type.h
** copyright:    
** description:  该模块主要定义数据类型
**
-------------------------------------------------------------------------------
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#ifndef OS_TYPE_H
#define OS_TYPE_H


/*
********************************************************************************
* 定义数据类型
********************************************************************************
*/

typedef unsigned char  BOOLEAN;
typedef unsigned char  INT8U;                   /* Unsigned  8 bit quantity                           */
typedef signed   char  INT8S;                   /* Signed    8 bit quantity                           */
typedef unsigned short INT16U;                  /* Signed   16 bit quantity                           */
typedef signed   short INT16S;                  /* Unsigned 32 bit quantity                           */
typedef unsigned int   INT32U;                  /* Unsigned 32 bit quantity                           */
typedef unsigned long  long INT64U;             /* Unsigned 64 bit quantity                           */
typedef signed   int   INT32S;                  /* Signed   32 bit quantity                           */
typedef float          FP32;                    /* Single precision floating point                    */
typedef double         FP64;                    /* Double precision floating point                    */

/*
********************************************************************************
* 定义关键字
********************************************************************************
*/
#ifndef false
#define false                0
#endif

#ifndef  true
#define  true                1
#endif

#ifndef TRUE
#define TRUE                 1
#endif

#ifndef FALSE
#define FALSE                0
#endif


#ifndef NULL 
#define NULL                 ((void *)0)
#endif

#ifndef	CR
//#define CR                   0x0D
#endif

#ifndef LF
//#define LF                   0x0A
#endif

#ifndef RETURN_VOID
#define RETURN_VOID
#endif

#ifndef RETURN_TRUE
#define RETURN_TRUE          TRUE
#endif

#ifndef RETURN_FALSE
#define RETURN_FALSE         FALSE
#endif

#ifndef	 _SUCCESS
#define  _SUCCESS                0
#endif

#ifndef	 _FAILURE
#define  _FAILURE                1
#endif

#ifndef  _OVERTIME
#define  _OVERTIME               2
#endif



#endif
/*************END OF FILE****/

