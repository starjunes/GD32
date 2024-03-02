/**************************************************************************************************
**                                                                                               **
**  文件名称:  Man_Timer.h                                                                       **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2010                                    **
**  创建信息:  LEON -- 2010年12月3日                                                             **
**  文件描述:  系统定时器管理(由systick产生中断)                                                 **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/
#ifndef __MAN_TIMER_H
#define __MAN_TIMER_H

#include "dal_include.h"
#include "man_queue.h"

#ifndef GLOBALS_TIMER
#define _TIMER_EXT                    extern volatile
#else
#define _TIMER_EXT                    volatile
#endif

/*************************************************************************************************/
/*                           系统最多允许的定时器个数                                            */
/*************************************************************************************************/
#define MAX_SYS_TIMER        50

/*************************************************************************************************/
/*                           定义时间属性                                                        */
/*************************************************************************************************/
#define PERTICK              (10)                                    /* 1 tick = 10 ms */
#define TICKS_SECOND         (1000L / PERTICK)
#define TICKS_MINUTE         ((60 * 1000L) / PERTICK)

/*************************************************************************************************/
/*                           定义定时器精度属性                                                  */
/*************************************************************************************************/
typedef enum {
    _TICK   = 1,
    _SECOND = TICKS_SECOND,
    _MINUTE = TICKS_MINUTE
} TIMER_PA_E;

/*
********************************************************************************
* define module variants
********************************************************************************
*/
_TIMER_EXT INT32U g_systicks;

INT32U  CreateTimer(void (*tmrproc)(void));
void    StartTimer(INT32U tmrid, TIMER_PA_E attrib, INT32U interval, BOOLEAN reload);
void    StopTimer(INT32U tmrid);
void    RemoveTimer(INT32U tmrid);
BOOLEAN TimerIsRun(INT32U tmrid);
INT8U   TimerRemain(void);
void    InitTimerMan(void);
void    SysTimerEntryProc(void);
INT32U OS_GetSysTick(INT32U * count);



#endif

