/**************************************************************************************************
**                                                                                               **
**  文件名称:  Man_Timer.C                                                                       **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2010                                    **
**  创建信息:  LEON -- 2010年12月3日                                                             **
**  文件描述:  系统定时器管理(由systick产生中断)                                                 **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/
#define GLOBALS_TIMER             1

#include "man_irq.h"
#include "man_error.h"
#include "man_timer.h"
#include "debug_print.h"

const INT32U SystemFrequency = SYS_CLK_FREQ;

/*************************************************************************************************/
/*                           定时器控制块                                                        */
/*************************************************************************************************/
typedef struct {
    BOOLEAN  inuse;                                                  /* 定时器是否已被使用 */
    BOOLEAN  isrun;                                                  /* 定时器是否正在运行 */
    BOOLEAN  ready;                                                  /* 定时器是否已经到点 */
    INT32U   ttime;                                                  /* 定时器定时总时间 */
    INT32U   ltime;                                                  /* 定时器剩余时间 */
    void   (*tporc)(void);                                           /* 定时回调函数入口地址 */
    void   (*bproc)(void);                                           /* 备份回调函数入口地址 */
} TIMER_T;

/*************************************************************************************************/
/*                           模块静态变量定义                                                    */
/*************************************************************************************************/
static   TIMER_T  s_systimer[MAX_SYS_TIMER];

/**************************************************************************************************
**  函数名称:  SysTickIrqHandle
**  功能描述:  systick定时器中断处理函数
**  输入参数:
**  返回参数:
**************************************************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) static void SysTickIrqHandle(void)
{
    INT32U i;

    for (i = 0; i < MAX_SYS_TIMER; i++) {

        if (s_systimer[i].inuse == FALSE) continue;
        if (s_systimer[i].isrun == FALSE) continue;

        if (s_systimer[i].ltime > 1) {
            s_systimer[i].ltime--;
        } else {
            s_systimer[i].ready = TRUE;                              /* 此处只是将标志位置为有效 */
        }
    }

    g_systicks++;
}

/**************************************************************************************************
**  函数名称:  SystickStartup
**  功能描述:  启动systick定时器
**  输入参数:
**  返回参数:
**************************************************************************************************/
static void SystickStartup(void)
{
    SysTick_Config(SystemFrequency / 100);                           /* /1000 = 1ms, /100 = 10ms */

    NVIC_IrqHandleInstall(SysTick_IRQ, (ExecFuncPtr)SysTickIrqHandle, SYSTICK_PRIOTITY, TRUE);
}

/**************************************************************************************************
**  函数名称:  CreateTimer
**  功能描述:  创建定时器
**  输入参数:  tmrproc: 定时器回调函数入口地址
**  返回参数:  tmrid: 定时器序号
**************************************************************************************************/
INT32U CreateTimer(void (*tmrproc)(void))
{
    INT32U tmrid;

    DAL_ASSERT(tmrproc != NULL);

    for (tmrid = 0; tmrid < MAX_SYS_TIMER; tmrid++) {
        if (s_systimer[tmrid].inuse == FALSE) break;                 /* 寻找空闲的定时器控制块 */
    }

    DAL_ASSERT(tmrid < MAX_SYS_TIMER);

    s_systimer[tmrid].inuse = TRUE;
    s_systimer[tmrid].tporc = tmrproc;
    s_systimer[tmrid].bproc = tmrproc;

    return tmrid;
}

/**************************************************************************************************
**  函数名称:  StartTimer
**  功能描述:  启动定时器
**  输入参数:  tmrid: 定时器序号，attrib: 定时精度属性，interval: 定时时长，reload: 是否需要自动重载
**  返回参数:
**************************************************************************************************/
void StartTimer(INT32U tmrid, TIMER_PA_E attrib, INT32U interval, BOOLEAN reload)
{
    DAL_ASSERT(tmrid < MAX_SYS_TIMER);
    DAL_ASSERT(s_systimer[tmrid].tporc != NULL);
    DAL_ASSERT(s_systimer[tmrid].tporc == s_systimer[tmrid].bproc);

    s_systimer[tmrid].ltime = attrib * interval;                     /* 计算定时时长 */

    if (reload == TRUE) {
        s_systimer[tmrid].ttime = attrib * interval;                 /* 保存自动重载时长 */
    } else {
        s_systimer[tmrid].ttime = 0;
    }

    s_systimer[tmrid].isrun = TRUE;                                  /* 启动定时器 */
}

/**************************************************************************************************
**  函数名称:  StopTimer
**  功能描述:  停止定时器
**  输入参数:  tmrid: 定时器序号
**  返回参数:
**************************************************************************************************/
void StopTimer(INT32U tmrid)
{
    DAL_ASSERT(tmrid < MAX_SYS_TIMER);

    s_systimer[tmrid].isrun = FALSE;
    s_systimer[tmrid].ttime = 0;
    s_systimer[tmrid].ready = FALSE;
}

/**************************************************************************************************
**  函数名称:  RemoveTimer
**  功能描述:  移除定时器
**  输入参数:  tmrid: 定时器序号
**  返回参数:
**************************************************************************************************/
void RemoveTimer(INT32U tmrid)
{
     DAL_ASSERT(tmrid < MAX_SYS_TIMER);

     memset(&s_systimer[tmrid], 0, sizeof(s_systimer[tmrid]));
}

/**************************************************************************************************
**  函数名称:  TimerIsRun
**  功能描述:  查询定时器是否正在运行中
**  输入参数:  tmrid: 定时器序号
**  返回参数:  TRUE or FALSE
**************************************************************************************************/
BOOLEAN TimerIsRun(INT32U tmrid)
{
    DAL_ASSERT(tmrid < MAX_SYS_TIMER);

    return s_systimer[tmrid].isrun;
}

/**************************************************************************************************
**  函数名称:  InitTimerMan
**  功能描述:  初始化定时器管理模块
**  输入参数:
**  返回参数:
**************************************************************************************************/
void InitTimerMan(void)
{
    memset(s_systimer, 0, sizeof(s_systimer));

    g_systicks = 0;

    SystickStartup();
    #if EN_DEBUG > 1
    Debug_SysPrint("InitTimerMan()\r\n");
    #endif
}

/**************************************************************************************************
**  函数名称:  SysTimerEntryProc
**  功能描述:  定时器管理模块执行函数(由主函数的while循环反复调用)
**  输入参数:
**  返回参数:
**************************************************************************************************/
void SysTimerEntryProc(void)
{
    INT32U i;

    for (i = 0; i < MAX_SYS_TIMER; i++) {

        if (s_systimer[i].inuse == FALSE) continue;
        if (s_systimer[i].isrun == FALSE) continue;

        if (s_systimer[i].ready == TRUE) {

            DAL_ASSERT(s_systimer[i].tporc != NULL);
            DAL_ASSERT(s_systimer[i].tporc == s_systimer[i].bproc);

            s_systimer[i].ready = FALSE;                             /* 本语句位置不能后移 */

            s_systimer[i].tporc();                                   /* 执行回调函数 */
            
            s_systimer[i].isrun = FALSE;                             /* 本语句位置不能后移 */

            if (s_systimer[i].ttime != 0) {                          /* 判断是否需要自动重载 */
                s_systimer[i].ltime = s_systimer[i].ttime;
                s_systimer[i].isrun = TRUE;
            }
        }
    }
}

/*******************************************************************
* 函数名称:  OS_GetSysTick
* 函数描述:  获得系统tick
* 参数:      无
* 返回:      tick
********************************************************************/
INT32U OS_GetSysTick(INT32U * count)
{
    count[0] = g_systicks;
    return g_systicks;
}

