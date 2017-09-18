/********************************************************************************
**
** 文件名:     dal_pulse_drv.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现里程脉冲采集驱动
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2014/04/09 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "yx_include.h"
#include "yx_loopbuf.h"
#include "st_gpio_drv.h"
#include "st_irq_drv.h"
#include "st_exti_drv.h"
#include "dal_pulse_drv.h"
#include "yx_debug.h"


/*
********************************************************************************
* 定义配置参数
********************************************************************************
*/

#define PERIOD_PULSE         _MILTICK, 2
#define PERIOD_CLOCK         _MILTICK, 5

#define PIN_PULSE            GPIO_PIN_B3         /* 脉冲采集管脚 */
#define MAX_NUM              20                  /* 缓存最大组数 */

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    INT8U  open;
    INT8U  clock;
    INT8U  pulse[MAX_NUM];
    INT16U period;
    INT32U ct_pulse;
    INT32U pre_pulse;
    LOOP_BUF_T  r_round;
} DCB_T;

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/

static INT8U s_pulsetmr, s_clocktmr;
static DCB_T s_dcb;

/*******************************************************************
** 函数名称: PulseCallback
** 函数描述: 检测到脉冲处理
** 参数:     无
** 返回:     无
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void PulseCallback(void)
{
    s_dcb.ct_pulse++;
}

/*******************************************************************
** 函数名:     PulseTmrProc
** 函数描述:   定时器
** 参数:       [in] index:定时器标识
** 返回:       无
********************************************************************/
static void PulseTmrProc(void *index)
{
    INT32U curpulse, pulse;
    volatile INT32U cpu_sr;
    
    OS_StartTmr(s_pulsetmr, _MILTICK, s_dcb.period);
    
    OS_ENTER_CRITICAL();                            /* 关中断 */
    curpulse = s_dcb.ct_pulse;
    OS_EXIT_CRITICAL();                             /* 开中断 */
    
    if (curpulse >= s_dcb.pre_pulse) {
        pulse = curpulse - s_dcb.pre_pulse;
    } else {
        pulse = 0xffffffff - s_dcb.pre_pulse + curpulse + 1;
    }
    s_dcb.pre_pulse = curpulse;
    
    if (YX_UsedOfLoopBuffer(&s_dcb.r_round) >= MAX_NUM) {                      /* 满则删掉最旧 */
        YX_ReadLoopBuffer(&s_dcb.r_round);
    }
    YX_WriteLoopBuffer(&s_dcb.r_round, pulse);
}

/*******************************************************************
** 函数名:     ClockTmrProc
** 函数描述:   定时器
** 参数:       [in] index:定时器标识
** 返回:       无
********************************************************************/
static void ClockTmrProc(void *index)
{
    OS_StartTmr(s_clocktmr, PERIOD_CLOCK);
    
    if (!DAL_PULSE_IsOpen()) {
        if (ST_GPIO_ReadOutputDataBit(GPIO_PIN_B3)) {
            ST_GPIO_WritePin(GPIO_PIN_B3, false);
        } else {
            ST_GPIO_WritePin(GPIO_PIN_B3, true);
        }
    }
}

/*******************************************************************
** 函数名称: DAL_PULSE_InitDrv
** 函数描述: 初始化模块
** 参数:     无
** 返回:     无
********************************************************************/
void DAL_PULSE_InitDrv(void)
{
    YX_MEMSET(&s_dcb, 0, sizeof(s_dcb));
    
    s_dcb.period = 2;                                                          /* 200ms间隔 */
    s_pulsetmr = OS_CreateTmr(TSK_ID_DAL, 0, PulseTmrProc);
    s_clocktmr = OS_CreateTmr(TSK_ID_DAL, 0, ClockTmrProc);
    OS_StartTmr(s_clocktmr, PERIOD_CLOCK);
}

/*******************************************************************
** 函数名称: DAL_PULSE_IsOpen
** 函数描述: 里程脉冲统计功能是否已打开
** 参数:     无
** 返回:     是返回TRUE,否返回FALSE
********************************************************************/
BOOLEAN DAL_PULSE_IsOpen(void)
{
    return s_dcb.open;
}

/*******************************************************************
** 函数名称: DAL_PULSE_OpenPulseCalFunction
** 函数描述: 打开里程脉冲统计功能
** 参数:     [in] period: 采集间隔，单位：ms
** 返回:     成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN DAL_PULSE_OpenPulseCalFunction(INT16U period)
{
    if (!s_dcb.open) {
        s_dcb.open = true;
        if (period != 0) {                                                     /* 等于0,则采用默认间隔 */
            s_dcb.period = (period + 99) / 100;
        }
        YX_InitLoopBuffer(&s_dcb.r_round, s_dcb.pulse, sizeof(s_dcb.pulse));
        ST_EXTI_OpenExtiFunction(PIN_PULSE, EXTI_TRIG_FALL, PulseCallback);
        
        OS_StartTmr(s_pulsetmr, _MILTICK, s_dcb.period);
    } else {
        s_dcb.period = (period + 99) / 100;
    }
    return true;
}

/*******************************************************************
** 函数名称: DAL_PULSE_ClosePulseCalFunction
** 函数描述: 关闭里程脉冲统计功能
** 参数:     [in] pinid:    GPIO统一编号,GPIO_PIN_E
** 返回:     成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN DAL_PULSE_ClosePulseCalFunction(void)
{
    if (s_dcb.open) {
        s_dcb.open = false;
        YX_InitLoopBuffer(&s_dcb.r_round, s_dcb.pulse, sizeof(s_dcb.pulse));
        ST_EXTI_CloseExtiFunction(PIN_PULSE);
        
        OS_StopTmr(s_pulsetmr);
    }
    return true;
}

/*******************************************************************
** 函数名称: DAL_PULSE_OpenClockOutput
** 函数描述: 打开实时时钟1HZ脉冲输出
** 参数:     无
** 返回:     成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN DAL_PULSE_OpenClockOutput(void)
{
    if (!s_dcb.clock) {
        s_dcb.clock = TRUE;
        DAL_PULSE_ClosePulseCalFunction();
        ST_GPIO_SetPin(GPIO_PIN_B3, GPIO_DIR_OUT, GPIO_PULL_NULL, 0);
    }

    return true;
}

/*******************************************************************
** 函数名称: DAL_PULSE_CloseClockOutput
** 函数描述: 关闭实时时钟1HZ脉冲输出
** 参数:     [in] pinid:    GPIO统一编号,GPIO_PIN_E
** 返回:     成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN DAL_PULSE_CloseClockOutput(void)
{
    if (s_dcb.clock) {
        s_dcb.clock = FALSE;
        DAL_PULSE_OpenPulseCalFunction(0);
    }
    
    return true;
}

/*******************************************************************
** 函数名称: DAL_PULSE_GetGroupNum
** 函数描述: 获取脉冲组数
** 参数:     无
** 返回:     成功返回组数
********************************************************************/
INT8U DAL_PULSE_GetGroupNum(void)
{
    if (!s_dcb.open) {
        return 0;
    }
    
    return YX_UsedOfLoopBuffer(&s_dcb.r_round);
}

/*******************************************************************
** 函数名称: DAL_PULSE_GetPulse
** 函数描述: 获取脉冲数，从最旧开始获取
** 参数:     无
** 返回:     成功单组脉冲数,失败返回-1
********************************************************************/
INT32S DAL_PULSE_GetPulse(void)
{
    if (!s_dcb.open) {
        return -1;
    }
    
    return YX_ReadLoopBuffer(&s_dcb.r_round);
}

/*******************************************************************
** 函数名称: DAL_PULSE_GetTotalPulse
** 函数描述: 获取复位重启后总脉冲数
** 参数:     无
** 返回:     脉冲数
********************************************************************/
INT32U DAL_PULSE_GetTotalPulse(void)
{
    return s_dcb.pre_pulse;
}

