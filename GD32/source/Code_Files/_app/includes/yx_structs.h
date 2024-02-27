/****************************************************************
**                                                              *
**  FILE         :  YX_Structs.h                                *
**  COPYRIGHT    :  (c) 2001 .Xiamen Yaxon NetWork CO.LTD       *
**                                                              *
**                                                              *
**  By : CCH 2002.1.15                                          *
****************************************************************/

#ifndef DEF_STRUCTS_H
#define DEF_STRUCTS_H

//#include "yx_includes.h"

#if 0
/*
********************************************************************************
*                  DEFINE WORD UNION
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
} HWORD_UNION;
#endif
/*
********************************************************************************
*                  DEFINE LONG UNION
********************************************************************************
*/
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
} LONG_UNION;
#if 0
/*
********************************************************************************
*                  DEFINE DATE STRUCT
********************************************************************************
*/
typedef struct {
    INT8U           year;
    INT8U           month;
    INT8U           day;
} DATE_T;

/*
********************************************************************************
*                  DEFINE TIME STRUCT
********************************************************************************
*/
typedef struct {
    INT8U           hour;
    INT8U           minute;
    INT8U           second;
} TIME_T;

/*
********************************************************************************
*                  DEFINE SYSTIME STRUCT
********************************************************************************
*/
typedef struct {
    DATE_T     date;
    TIME_T     time;
} SYSTIME_T;
#endif
typedef struct
{
   INT8U  LeftLa[4];
   INT8U  LeftLg[4];
   INT8U  RightLa[4];
   INT8U  RightLg[4];
}GPS_REGION_T;
#if 0
/*
********************************************************************************
*                  DEFINE FUNCTION ENTRY STRUCT
********************************************************************************
*/
typedef struct {
    INT16U     index;
    void (* entryproc)(void);
} FUNCENTRY_T;
#endif   // 与m4工程的结构体冲突
/*
********************************************************************************
*                  DEFINE EXTEND FUNCTION ENTRY STRUCT(LCD专用)
********************************************************************************
*/
typedef struct {
    INT16U     index;
    void (* entryproc)(INT8U type, INT8U *data, INT16U datalen);
} EXT_FUNCENTRY_T;

/*
********************************************************************************
*                  DEFINE EXTEND FUNCTION ENTRY STRUCT2
********************************************************************************
*/
typedef struct {
    INT16U     index;
    void (* entryproc)(INT16U type, INT8U *data, INT16U datalen);
} EXT_FUNCENTRY2_T;
#if 0
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
// 脉冲数定义, 用于外接报警器
typedef struct {
    INT8U       ct_low;
    INT8U       ct_high;
} PULS_T;

typedef struct {
    INT8U       ct_puls;
    PULS_T puls;
} WAVE_T;
#if 0
typedef struct
{
     /* HBIT = 01A000      A: VALID BIT,   0 ----- VALID    1 ----- INVALID*/
     INT8U       hbit;
     INT8U       latitude[4];
     INT8U       longitude[4];
     INT8U       vector;
     INT8U       direction;
} GPS_STRUCT;
#endif


#endif
