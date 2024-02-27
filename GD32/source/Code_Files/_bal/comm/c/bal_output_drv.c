/********************************************************************************
**
** 文件名:     bal_output_drv.c
** 版权所有:   (c) 1998-2017 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现I/O输出驱动控制
**
*********************************************************************************
**                  修改历史记录
**===============================================================================
**| 日期              | 作者        |  修改记录
**===============================================================================
**| 2017/05/15 | 黄运峰    |  移植、修改、规范化
********************************************************************************/
#include "yx_includes.h"
#include "os_diagnose.h"
#include "bal_tools.h"
#include "bal_output_drv.h"

/*
*********************************************************************************
*                   定义模块数据类型、变量及宏
*********************************************************************************
*/
// 定义配置参数
#define MAX_PORT                    5
#define PERIOD_FLASH                MILTICK, 5  /* flash period = 50 ms */

// 定义管理控制块数据结构
typedef struct {
    INT8U       level;
    INT8U       isctl;
    OUTPUT_IO_T const *preg;
    INT8U       time_high[MAX_MODE];                  /* UNIT: 50 ms */
    INT8U       time_low[MAX_MODE];                   /* UNIT: 50 ms */
    INT8U       ct_high[MAX_MODE];
    INT8U       ct_low[MAX_MODE];
    INT16U      ct_cycle[MAX_MODE];
    void        *pdata[MAX_MODE];
    void        (*informer[MAX_MODE])(INT8U, void *);
} PCB_T;

// 定义本地静态变量
static INT8U s_flashtmr;
static PCB_T s_pcb[MAX_PORT];
        
/*
*********************************************************************************
*                   本地接口实现
*********************************************************************************
*/
/********************************************************************************
** 函数名:     FlashTmrProc
** 函数描述:   定时执行函数，控制输出口的状态
** 参数:       [in]	pdata:	定时器识别号
** 返回:       无
********************************************************************************/
static void FlashTmrProc(void *pdata)
{
    INT8U i, j, nreg;
    
    pdata = pdata;
    OS_StartTmr(s_flashtmr, PERIOD_FLASH);
    
    nreg = bal_output_GetIOMax();
    OS_ASSERT((nreg <= MAX_PORT), RETURN_VOID);
    for (i = 0; i < nreg; i++) { 
        s_pcb[i].isctl = FALSE;
        
        for (j = 0; j < MAX_MODE; j++) {
            if (s_pcb[i].ct_cycle[j] > 0) {                   //周期不为0
                s_pcb[i].isctl = TRUE;
                if (s_pcb[i].ct_high[j] > 0) {                //先进行高电平控制
                    s_pcb[i].ct_high[j]--;
                    if (s_pcb[i].preg->pullup != 0) {
                        s_pcb[i].preg->pullup();
                    }
                    s_pcb[i].level = 1;
                } else {
                    if (s_pcb[i].ct_low[j] > 0) {             //再进行低电平控制
                        s_pcb[i].ct_low[j]--;
                        if (s_pcb[i].preg->pulldown != 0) {
                            s_pcb[i].preg->pulldown();
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
        
        if (s_pcb[i].isctl == FALSE) {//没有针对该输出口的控制则保持原先的状态
            if (s_pcb[i].level) {
                if (s_pcb[i].preg->pullup != 0) {
                    s_pcb[i].preg->pullup();
                }
            } else {
                if (s_pcb[i].preg->pulldown != 0) {
                    s_pcb[i].preg->pulldown();
                }
            }
        }
    }
}

/********************************************************************************
** 函数名:     DiagnoseProc
** 函数描述:   诊断函数入口
** 参数:       无
** 返回:       无
********************************************************************************/
static void DiagnoseProc(void)
{
    OS_ASSERT(OS_TmrIsRun(s_flashtmr), RETURN_VOID);
}

/*
*********************************************************************************
*                   对外接口实现
*********************************************************************************
*/
/********************************************************************************
** 函数名:     bal_output_InitDrv
** 函数描述:   初始化端口驱动
** 参数:       无
** 返回:       无
********************************************************************************/
void bal_output_InitDrv(void)
{
    INT8U i, nreg;
    OUTPUT_IO_T const *preg;
    
    YX_MEMSET(&s_pcb, 0, sizeof(s_pcb));
    nreg = bal_output_GetIOMax();
    OS_ASSERT((nreg <= MAX_PORT), RETURN_VOID);
    for (i = 0; i < nreg; i++) {
        preg = bal_output_GetRegInfo(i);
        s_pcb[i].preg = preg;
        s_pcb[i].level = preg->level;
        if (preg->initport != 0) {
            preg->initport();
        }
        if (preg->level != 0) {
            if (preg->pullup != 0) {
                preg->pullup();
            }
        } else {
            if (preg->pulldown != 0) {
                preg->pulldown();
            }
        }
    }
    s_flashtmr = OS_InstallTmr(PRIO_COMMONTASK, (void *)0, FlashTmrProc);
    OS_StartTmr(s_flashtmr, PERIOD_FLASH);
    OS_InstallDiag(DiagnoseProc);
}

/********************************************************************************
** 函数名:     bal_output_StartFlashPort
** 函数描述:   启动输出控制接口
** 参数:       [in]	port:	    输入口类型,见OUTPUT_IO_E
**             [in]	prior:	    优先级，见OUTPUT_PRIOR_E
**             [in]	cycle:	    次数
**             [in]	time_high:	高电平持续时间，单位：50ms
**             [in]	time_low:	低电平持续时间，单位：50ms
**             [in]	informer:	通知回调函数;
**             [in]	pdata:	    特征值，回调给informer
** 返回:       无
********************************************************************************/
void bal_output_StartFlashPort(INT8U port, INT8U prior, INT16U cycle, INT8U time_high, INT8U time_low, void (*informer)(INT8U, void *), void *pdata)
{
    if (port >= bal_output_GetIOMax()) return;
    if (prior >= MAX_MODE) return;
    
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

/********************************************************************************
** 函数名:     bal_output_StopFlashPort
** 函数描述:   停止输出控制接口
** 参数:       [in]	port:	输入口类型,见OUTPUT_IO_E
**             [in]	prior:	优先级，见OUTPUT_PRIOR_E
** 返回:       无
********************************************************************************/
void bal_output_StopFlashPort(INT8U port, INT8U prior)
{
    if (port >= bal_output_GetIOMax()) return;
    if (prior >= MAX_MODE) return;
    
    s_pcb[port].ct_cycle[prior] = 0;
    //if (!s_pcb[port].isctl) {
        s_pcb[port].level = 0;
        if (s_pcb[port].preg->pulldown != 0) {
            s_pcb[port].preg->pulldown();
        }
    //}
}

/********************************************************************************
** 函数名:     bal_output_InstallPermnentPort
** 函数描述:   启动永久控制输出口接口，常用于闪灯控制
** 参数:       [in]	port:	    输入口类型,见OUTPUT_IO_E
**             [in]	time_high:	高电平持续时间，单位：50ms
**             [in]	time_low:	低电平持续时间，单位：50ms
** 返回:       无
********************************************************************************/
void bal_output_InstallPermnentPort(INT8U port, INT8U time_high, INT8U time_low)
{
    if (port >= bal_output_GetIOMax()) return;

    if ((time_high == 0) && (time_low == 0)) {
        return;
    }
    s_pcb[port].ct_cycle[MAX_MODE-1]  = 0xffff;
    s_pcb[port].ct_high[MAX_MODE-1]   = time_high;
    s_pcb[port].ct_low[MAX_MODE-1]    = time_low;
    s_pcb[port].time_high[MAX_MODE-1] = time_high;
    s_pcb[port].time_low[MAX_MODE-1]  = time_low;
    s_pcb[port].informer[MAX_MODE-1]  = 0;
}

/********************************************************************************
** 函数名:     bal_output_RemovePermnentPort
** 函数描述:   停止永久控制输出口接口，常用于闪灯控制
** 参数:       [in]	port:	输入口类型,见OUTPUT_IO_E
** 返回:       无
********************************************************************************/
void bal_output_RemovePermnentPort(INT8U port)
{
    if (port >= bal_output_GetIOMax()) return;

    s_pcb[port].ct_cycle[MAX_MODE-1] = 0;
    //if (!s_pcb[port].isctl) {
    s_pcb[port].level = 0;
    if (s_pcb[port].preg->pulldown != 0) {
        s_pcb[port].preg->pulldown();
    }
    //}
}

/********************************************************************************
** 函数名:     bal_output_PortIsFlashing
** 函数描述:   判断指定端口是否正在被驱动
** 参数:       [in]	port:	输入口类型,见OUTPUT_IO_E
** 返回:       TRUE：端口已被驱动
**			   FALSE：端口未被驱动
********************************************************************************/
BOOLEAN bal_output_PortIsFlashing(INT8U port)
{
    if (port >= bal_output_GetIOMax()) return FALSE;
    if (s_pcb[port].isctl) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/********************************************************************************
** 函数名:     bal_output_GetPortStatus
** 函数描述:   读取指定端口的当前状态
** 参数:       [in]	port:	输入口类型,见OUTPUT_IO_E
** 返回:       1：高电平状态
**			   0：低点平状态
**            -1: 有错误发生
********************************************************************************/
INT8S bal_output_GetPortStatus(INT8U port)
{
    if (port >= bal_output_GetIOMax()) return -1;
    return s_pcb[port].level;
}

/********************************************************************************
** 函数名:     bal_output_GetPortStatus
** 函数描述:   读取指定端口的当前状态
** 参数:       [in]	port:	输入口类型,见OUTPUT_IO_E
** 返回:       TRUE： 操作成功
**			   FALSE：操作失败
********************************************************************************/
BOOLEAN bal_output_CtlPort(INT8U port, INT8U level)
{
    if(port > bal_output_GetIOMax()) return FALSE;
    s_pcb[port].level = level;
    if (level == 1) {
        if (s_pcb[port].preg->pullup != NULL) {
            s_pcb[port].preg->pullup();
        }
    } else {
        if (s_pcb[port].preg->pulldown != 0) {
            s_pcb[port].preg->pulldown();
        }
    }

    return TRUE;
}

//------------------------------------------------------------------------------
/* End of File */
