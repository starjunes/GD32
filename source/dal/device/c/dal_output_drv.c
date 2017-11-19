/********************************************************************************
**
** 文件名:     dal_output_drv.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现I/O输出驱动控制
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2012/02/09 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "yx_include.h"
#include "st_gpio_drv.h"
#include "dal_output_drv.h"

/*
********************************************************************************
*                   DEFINE CONFIG PARAMETERS
********************************************************************************
*/
#define MAX_PORT                        3
#define PERIOD_FLASH                    _TICK, 10        /* flash period = 100 ms */

/*
********************************************************************************
*                   DEFINE PORT CONTROL BLOCK STRUCTURE
********************************************************************************
*/
typedef struct {
    INT8U       level;
    INT8U       isctl;
    OUTPUT_IO_T const *preg;
    INT8U       time_high[OUTPUT_PRI_MAX];                  /* UNIT: 100 ms */
    INT8U       time_low[OUTPUT_PRI_MAX];                   /* UNIT: 100 ms */
    INT8U       ct_high[OUTPUT_PRI_MAX];
    INT8U       ct_low[OUTPUT_PRI_MAX];
    INT16U      ct_cycle[OUTPUT_PRI_MAX];
    void        *pdata[OUTPUT_PRI_MAX];
    void        (*informer[OUTPUT_PRI_MAX])(INT8U, void *);
} PCB_T;

/*
********************************************************************************
*                   DEFINE MODULE VARIANT
********************************************************************************
*/
static INT8U s_flashtmr;
static PCB_T s_pcb[MAX_PORT];


        
/*******************************************************************
** 函数名:     FlashTmrProc
** 函数描述:   定时执行函数，控制输出口的状态
** 参数:       [in]	pdata:	定时器识别号
** 返回:       无
********************************************************************/
static void FlashTmrProc(void *pdata)
{
    INT8U i, j, nreg;
    
    pdata = pdata;
    OS_StartTmr(s_flashtmr, PERIOD_FLASH);
    
    nreg = DAL_OUTPUT_GetIOMax();
    OS_ASSERT((nreg <= MAX_PORT), RETURN_VOID);
    for (i = 0; i < nreg; i++) { 
        s_pcb[i].isctl = FALSE;
        
        for (j = 0; j < OUTPUT_PRI_MAX; j++) {
            if (i == OPT_LEDMIX) {                                            /* 混合色控制 */
                if ((s_pcb[OPT_LEDRED].ct_cycle[j] > 0) || (s_pcb[OPT_LEDGREEN].ct_cycle[j] > 0)) {
                    break;
                }
            }
            
            if (s_pcb[i].ct_cycle[j] > 0) {                   //周期不为0
                s_pcb[i].isctl = TRUE;
                if (s_pcb[i].ct_high[j] > 0) {                //先进行高电平控制
                    s_pcb[i].ct_high[j]--;
                    if (s_pcb[i].preg->pullup != 0) {
                        s_pcb[i].preg->pullup(i);
                    }
                    s_pcb[i].level = 1;
                } else {
                    if (s_pcb[i].ct_low[j] > 0) {             //再进行低电平控制
                        s_pcb[i].ct_low[j]--;
                        if (s_pcb[i].preg->pulldown != 0) {
                            s_pcb[i].preg->pulldown(i);
                        }
                        s_pcb[i].level = 0;                        
                    }
                    
                    if (s_pcb[i].ct_low[j] == 0) {            //一个控制周期结束，重新加载控制
                        if (s_pcb[i].ct_cycle[j] != 0xffff) {
                            s_pcb[i].ct_cycle[j]--;
                        }
                        if (s_pcb[i].ct_cycle[j] == 0) {
                            if (s_pcb[i].informer[j] != 0) {
                                s_pcb[i].informer[j](i, s_pcb[i].pdata[j]);
                            }
                        }
                        s_pcb[i].ct_high[j] = s_pcb[i].time_high[j];
                        s_pcb[i].ct_low[j]  = s_pcb[i].time_low[j];
                    }
                }
                break;                                      //高优先级已经进行控制则退出轮询
            }
        }
        
        if (s_pcb[i].isctl == FALSE && (i != OPT_LEDMIX) && (i != OPT_LEDRED) && (i != OPT_LEDGREEN)) {//没有针对该输出口的控制则保持原先的状态
            if (s_pcb[i].level) {
                if (s_pcb[i].preg->pullup != 0) {
                    s_pcb[i].preg->pullup(i);
                }
            } else {
                if (s_pcb[i].preg->pulldown != 0) {
                    s_pcb[i].preg->pulldown(i);
                }
            }
        }
    }
}

/*******************************************************************
** 函数名:     DiagnoseProc
** 函数描述:   诊断函数入口
** 参数:       无
** 返回:       无
********************************************************************/
static void DiagnoseProc(void)
{
    OS_ASSERT(OS_TmrIsRun(s_flashtmr), RETURN_VOID);
}

/*******************************************************************
** 函数名:     DAL_OUTPUT_InitDrv
** 函数描述:   初始化端口驱动
** 参数:       无
** 返回:       无
********************************************************************/
void DAL_OUTPUT_InitDrv(void)
{
    INT8U i, nreg;
    OUTPUT_IO_T const *preg;
    
    YX_MEMSET(&s_pcb, 0, sizeof(s_pcb));
    nreg = DAL_OUTPUT_GetIOMax();
    OS_ASSERT((nreg <= MAX_PORT), RETURN_VOID);
    for (i = 0; i < nreg; i++) {
        preg = DAL_OUTPUT_GetRegInfo(i);
        s_pcb[i].preg = preg;
        s_pcb[i].level = preg->level;
        if (preg->initport != 0) {
            preg->initport(i);
        }
        if (preg->level != 0) {
            if (preg->pullup != 0) {
                preg->pullup(i);
            }
        } else {
            if (preg->pulldown != 0) {
                preg->pulldown(i);
            }
        }
    }
    s_flashtmr = OS_CreateTmr(TSK_ID_DAL, (void *)0, FlashTmrProc);
    OS_StartTmr(s_flashtmr, PERIOD_FLASH);
    OS_RegistDiagnoseProc(DiagnoseProc);
}

/*******************************************************************
** 函数名:     DAL_OUTPUT_StartFlash
** 函数描述:   启动输出控制接口
** 参数:       [in]	port:	    输入口类型,见OUTPUT_IO_E
**             [in]	prior:	    优先级，见OUTPUT_PRI_E
**             [in]	cycle:	    次数,0xFFFF表示永久闪烁
**             [in]	time_high:	高电平持续时间，单位：100ms
**             [in]	time_low:	低电平持续时间，单位：100ms
**             [in]	informer:	通知回调函数;
**             [in]	pdata:	    特征值，回调给informer
** 返回:       无
********************************************************************/
void DAL_OUTPUT_StartFlash(INT8U port, INT8U prior, INT16U cycle, INT8U time_high, INT8U time_low, void (*informer)(INT8U, void *), void *pdata)
{
    OS_ASSERT((port < DAL_OUTPUT_GetIOMax()), RETURN_VOID);
    OS_ASSERT((prior < OUTPUT_PRI_MAX), RETURN_VOID);
    
    if ((cycle != 0) && ((time_high == 0) && (time_low == 0))) {
        return;
    }
    if (cycle == 0) {
        return;
    }
    s_pcb[port].ct_cycle[prior]  = cycle;
    s_pcb[port].ct_high[prior]   = time_high;
    s_pcb[port].ct_low[prior]    = time_low;
    s_pcb[port].time_high[prior] = time_high;
    s_pcb[port].time_low[prior]  = time_low;
    s_pcb[port].informer[prior]  = informer;
    s_pcb[port].pdata[prior]     = pdata;
}

/*******************************************************************
** 函数名:     DAL_OUTPUT_StopFlash
** 函数描述:   停止输出控制接口
** 参数:       [in]	port:	输入口类型,见OUTPUT_IO_E
**             [in]	prior:	优先级，见OUTPUT_PRI_E
** 返回:       无
********************************************************************/
void DAL_OUTPUT_StopFlash(INT8U port, INT8U prior)
{
    OS_ASSERT((port < DAL_OUTPUT_GetIOMax()), RETURN_VOID);
    OS_ASSERT((prior < OUTPUT_PRI_MAX), RETURN_VOID);
    
    s_pcb[port].ct_cycle[prior] = 0;
    //if (!s_pcb[port].isctl) {
        s_pcb[port].level = 0;
        if (s_pcb[port].preg->pulldown != 0) {
            s_pcb[port].preg->pulldown(port);
        }
    //}
}

/*******************************************************************
** 函数名:     DAL_OUTPUT_StartPermentFlash
** 函数描述:   启动永久控制输出口接口，常用于闪灯控制
** 参数:       [in]	port:	    输入口类型,见OUTPUT_IO_E
**             [in]	time_high:	高电平持续时间，单位：100ms
**             [in]	time_low:	低电平持续时间，单位：100ms
** 返回:       无
********************************************************************/
void DAL_OUTPUT_StartPermentFlash(INT8U port, INT8U time_high, INT8U time_low)
{
    OS_ASSERT((port < DAL_OUTPUT_GetIOMax()), RETURN_VOID);
    
    if ((time_high == 0) && (time_low == 0)) {
        return;
    }
    s_pcb[port].ct_cycle[OUTPUT_PRI_MAX-1]  = 0xffff;
    s_pcb[port].ct_high[OUTPUT_PRI_MAX-1]   = time_high;
    s_pcb[port].ct_low[OUTPUT_PRI_MAX-1]    = time_low;
    s_pcb[port].time_high[OUTPUT_PRI_MAX-1] = time_high;
    s_pcb[port].time_low[OUTPUT_PRI_MAX-1]  = time_low;
    s_pcb[port].informer[OUTPUT_PRI_MAX-1]  = 0;
}

/*******************************************************************
** 函数名:     DAL_OUTPUT_StopPermentFlash
** 函数描述:   停止永久控制输出口接口，常用于闪灯控制
** 参数:       [in]	port:	输入口类型,见OUTPUT_IO_E
** 返回:       无
********************************************************************/
void DAL_OUTPUT_StopPermentFlash(INT8U port)
{
    OS_ASSERT((port < DAL_OUTPUT_GetIOMax()), RETURN_VOID);

    s_pcb[port].ct_cycle[OUTPUT_PRI_MAX - 1] = 0;
    //if (!s_pcb[port].isctl) {
        s_pcb[port].level = 0;
        if (s_pcb[port].preg->pulldown != 0) {
            s_pcb[port].preg->pulldown(port);
        }
    //}
}

/*******************************************************************
** 函数名:     DAL_OUTPUT_PortIsFlashing
** 函数描述:   判断指定端口是否正在被驱动
** 参数:       [in]	port:	输入口类型,见OUTPUT_IO_E
** 返回:       TRUE：端口已被驱动
**			   FALSE：端口未被驱动
********************************************************************/
BOOLEAN DAL_OUTPUT_PortIsFlashing(INT8U port)
{
    OS_ASSERT((port < DAL_OUTPUT_GetIOMax()), RETURN_FALSE);
    
    if (s_pcb[port].isctl) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/*******************************************************************
** 函数名:     DAL_OUTPUT_GetPortStatus
** 函数描述:   读取指定端口的当前状态
** 参数:       [in]	port:	输入口类型,见OUTPUT_IO_E
** 返回:       1：高电平状态
**			   0：低点平状态
********************************************************************/
INT8U DAL_OUTPUT_GetPortStatus(INT8U port)
{
    OS_ASSERT((port < DAL_OUTPUT_GetIOMax()), RETURN_FALSE);
    
    return s_pcb[port].level;
}
