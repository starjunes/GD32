/******************************************************************************
**
** filename:     os_timer.c
** copyright:    
** description:  该模块主要实现定时器调度和管理
**
-------------------------------------------------------------------------------
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#define GLOBALS_TIMER             1
#include "os_include.h"
#include "os_timer.h"
#include "os_bsp.h"

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
static OS_TMR_T s_tcb[OS_MAX_TIMER];
static INT8U s_tsktmrready[OS_TSK_ID_MAX];
static INT32U s_preticks;



/*******************************************************************
** 函数名称:   OS_InitTimer
** 函数描述:   定时器模块初始化函数
** 参数:       无
** 返回:       无
********************************************************************/
void OS_InitTimer(void)
{
    INT8U i;
    
    for (i = 0; i < OS_MAX_TIMER; i++) {
        s_tcb[i].ready   = false;
        s_tcb[i].tskid   = 0;
        s_tcb[i].tmrproc = 0;
        s_tcb[i].bakproc = 0;
    }
    s_preticks   = 0;
    g_systicks = 0;
    
    OS_SysTickStartup();
}

/*******************************************************************
** 函数名称:   OS_CreateTmr
** 函数描述:   创建定时器
** 参数:       [in] tskid     定时器所属任务ID
**             [in] index:     定时器标识
**             [in] tmrproc:   定时器超时处理函数
** 返回:       定时器ID; 如安装失败, 则返回0xff
********************************************************************/
INT8U OS_CreateTmr(INT8U tskid, void *index, void (*tmrproc)(void *index))
{
    INT8U id;
    
    OS_ASSERT((tmrproc != 0), 0xff);
    OS_ASSERT((tskid < OS_TSK_ID_MAX), 0xff);
    
    for (id = 0; id < OS_MAX_TIMER; id++) {                                    /* 查找一个空定时器 */
        if (s_tcb[id].tmrproc == 0) {
            break;
        }
    }
    OS_ASSERT((id < OS_MAX_TIMER), 0xff);                                      /* 找不到一个空定时器 */

    
    s_tcb[id].ready     = FALSE;
    s_tcb[id].tskid     = tskid;
    s_tcb[id].index     = index;
    s_tcb[id].totaltime = 0x00;
    s_tcb[id].lefttime  = 0x00;
    s_tcb[id].tmrproc   = tmrproc;
    s_tcb[id].bakproc   = tmrproc;
    return id;
}

/*******************************************************************
** 函数名称:   OS_RemoveTmr
** 函数描述:   删除一个已创建的定时器
** 参数:       [in]  id: 定时器ID
** 返回:       无
********************************************************************/
void OS_RemoveTmr(INT8U id)
{
    OS_ASSERT((id < OS_MAX_TIMER), RETURN_VOID);
    
    s_tcb[id].totaltime = 0;
    s_tcb[id].lefttime  = 0;
    s_tcb[id].ready     = FALSE;
    s_tcb[id].tmrproc   = 0;
    s_tcb[id].bakproc   = 0;
}

/*******************************************************************
** 函数名称:   OS_StartTmr
** 函数描述:   启动定时器
** 参数:       [in]  id:        已创建的一个定时器ID
**             [in]  attrib:    定时器超时时间单位
**             [in]  time:      定时器超时时间
** 返回:       无
********************************************************************/
void OS_StartTmr(INT8U id, INT32U attrib, INT32U time)
{
    OS_ASSERT((id < OS_MAX_TIMER && s_tcb[id].tmrproc != 0), RETURN_VOID);

    s_tcb[id].totaltime = (attrib * time);
    s_tcb[id].lefttime  = s_tcb[id].totaltime;
    s_tcb[id].ready     = FALSE;
}

/*******************************************************************
** 函数名称:   OS_StopTmr
** 函数描述:   停止定时器
** 参数:       [in]  id: 定时器ID
** 返回:       无
********************************************************************/
void OS_StopTmr(INT8U id)
{
    OS_ASSERT((id < OS_MAX_TIMER), RETURN_VOID);

    s_tcb[id].totaltime = 0;
    s_tcb[id].lefttime  = 0;
    s_tcb[id].ready     = FALSE;
}

/*******************************************************************
** 函数名称:   OS_GetLeftTime
** 函数描述:   获取定时器还需要等待的超时时间
** 参数:       [in]  id: 定时器ID
** 返回:       剩余的运行时间, 单位: 一个tick
********************************************************************/
INT32U OS_GetLeftTime(INT8U id)
{
    OS_ASSERT((id < OS_MAX_TIMER), 0);
    
    if (s_tcb[id].tmrproc != 0 && s_tcb[id].totaltime > 0) {
        return (s_tcb[id].lefttime);
    } else {
        return 0;
    }
}

/*******************************************************************
** 函数名称:   OS_TmrIsRun
** 函数描述:   判断定时器是否处于运行状态
** 参数:       [in]  id:  定时器ID
** 返回:       true:  运行状态,false: 停止状态
********************************************************************/
INT8U OS_TmrIsRun(INT8U id)
{
    OS_ASSERT((id < OS_MAX_TIMER), RETURN_FALSE);
    
    if (s_tcb[id].tmrproc != 0 && s_tcb[id].totaltime > 0) {
        return true;
    } else {
        return false;
    }
}

/*******************************************************************
** 函数名称:   OS_ExeTmrFunc
** 函数描述:   定时器回调函数执行接口，供各任务调用
** 参数:       [in]     tskid：任务ID号
** 返回:       无
********************************************************************/
void OS_ExeTmrFunc(INT8U tskid) 
{
    INT8U i;
    
    for (i = 0; i < OS_MAX_TIMER; i++) {
        if (s_tcb[i].ready && (tskid == s_tcb[i].tskid) && s_tcb[i].tmrproc != 0) {
            OS_ASSERT(s_tcb[i].tmrproc == s_tcb[i].bakproc, RETURN_VOID);
            
            s_tcb[i].ready = false;
            s_tcb[i].tmrproc(s_tcb[i].index);
        }
    }
}

/*******************************************************************
** 函数名称:   OS_TmrTskEntry
** 函数描述:   系统定时器超时后的处理函数
** 参数:       无
** 返回:       无
********************************************************************/
void OS_TmrTskEntry(void)
{
    INT32U i, time, curticks;

    if (s_preticks == g_systicks) {
        return;
    }
        
    ClearWatchdog();
    curticks = g_systicks;
    if (curticks >= s_preticks) {
        time =  curticks - s_preticks;
    } else {
        time = (0xFFFFFFFF - s_preticks) + curticks;
    }
    s_preticks = curticks;                                    /* 保存当前tick计数 */
    
    for (i = 0; i < OS_TSK_ID_MAX; i++) {
        s_tsktmrready[i]  = FALSE;
    }

    for (i = 0; i < OS_MAX_TIMER; i++) {
        if (s_tcb[i].tmrproc != 0 && s_tcb[i].totaltime > 0) {
            if (s_tcb[i].lefttime >= time) {
                s_tcb[i].lefttime -= time;
            } else {
                s_tcb[i].lefttime  = 0;
            }
            if (s_tcb[i].lefttime == 0) {
                OS_ASSERT((s_tcb[i].tskid < OS_TSK_ID_MAX), RETURN_VOID);
                
                s_tcb[i].lefttime = s_tcb[i].totaltime;
                s_tcb[i].ready = true;
                
                if (!s_tsktmrready[s_tcb[i].tskid]) {
                    s_tsktmrready[s_tcb[i].tskid] = TRUE;
                }
            }
        }
    }
    
    for (i = 0; i < OS_TSK_ID_MAX; i++) {
        if (s_tsktmrready[i]) {
            OS_PostMsgEx(FALSE, i, MSG_ID_NULL, 0, 0);
        }
    }
}

/*************END OF FILE****/

