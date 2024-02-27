/********************************************************************************
**
** 文件名:     yx_viropttsk.c
** 版权所有:   (c) 1998-2017 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要负责虚拟应用任务初始化及消息处理
**
*********************************************************************************
**                  修改历史记录
**===============================================================================
**| 日期              | 作者        |  修改记录
**===============================================================================
**| 2017/05/15 | 黄运峰    |  移植、修改、规范化
********************************************************************************/
#include "yx_includes.h"
#include "yx_message.h"
#include "bal_stream.h"
#include "bal_tools.h"
#include "yx_viropttsk.h"
//#include "bal_debug.h"
#if EN_WIRELESSDOWN > 0
#include "yx_wd_man.h"
#endif
#include "os_diagnose.h"
#include "bal_input_drv.h"
#include "bal_output_drv.h"
#include "bal_gpio_cfg.h"
#include "port_uart.h"
#include "yx_hit_man.h"
/*
*********************************************************************************
* 定义配置参数
*********************************************************************************
*/
#define PERIOD_MONITOR       SECOND, 1

#define MAXIMSICHANGEHANDLENUM		2

/*
*********************************************************************************
* 定义模块数据结构
*********************************************************************************
*/
typedef struct {
    INT8U  checkimsi;
    INT8U  reset;
    SYSTIME_T pretime;
} MCB_T;

typedef void (*IMSIChangeHandle_F)();

/*
*********************************************************************************
* 定义模块变量
*********************************************************************************
*/
static MCB_T s_mcb;
static INT8U s_monitortmr;

/*
*********************************************************************************
* 定义消息处理函数
*********************************************************************************
*/
/********************************************************************************
**  函数名:     Hdl_MSG_OPT_TMRRUN
**  函数描述:   任务所属定时器入口
**  参数:       [in] msgid:消息编号
**              [in] lpara:参数1
**              [in] hpara:参数2
**  返回:       无
********************************************************************************/
void Hdl_MSG_OPT_TMRRUN(INT16U tskid, INT16U msgid, INT32U lpara, INT32U hpara)
{
    //OS_ExeTmrFunc(PRIO_OPTTASK);
}

#if EN_WIRELESSDOWN > 0
/********************************************************************************
**  函数名:     Hdl_MSG_WDOWNRESULT_ENTRY
**  函数描述:   无线下载消息处理入口
**  参数:       [in] msgid:消息编号
**              [in] lpara:参数1
**              [in] hpara:参数2
**  返回:       无
********************************************************************************/
void Hdl_MSG_WDOWNRESULT_ENTRY(INT16U tskid, INT16U msgid, INT32U lpara, INT32U hpara)
{
    YX_WD_ResultEntry(lpara);
}
#endif

/*
*********************************************************************************
* Handler:  Hdl_MSG_OPT_GSEN_HIT
*********************************************************************************
*/
void Hdl_MSG_OPT_GSEN_HIT(INT16U tskid, INT16U msgid, INT32U lpara, INT32U hpara)
{
    tskid = tskid;
    msgid = msgid;
    lpara = lpara;
    hpara = hpara;
    YX_InformHitAlm();
}
/*
*********************************************************************************
* Handler:  Hdl_MSG_OPT_GSEN_MOTION
*********************************************************************************
*/
void Hdl_MSG_OPT_GSEN_MOTION(INT16U tskid, INT16U msgid, INT32U lpara, INT32U hpara)
{
    tskid = tskid;
    msgid = msgid;
    lpara = lpara;
    hpara = hpara;
    // YX_WakeUpChkHdl();
}
/********************************************************************************
** 函数名:     MonitorTmrProc
** 函数描述:   定时器处理函数
** 参数:       [in] pdata:定时器特征值
** 返回:       无
********************************************************************************/
static void MonitorTmrProc(void *data)
{

    OS_StartTmr(s_monitortmr, PERIOD_MONITOR);
}

/********************************************************************************
**  函数名:     YX_InitVirOptTsk
**  函数描述:   业务任务初始化入口
**  参数:       无
**  返回:       无
********************************************************************************/
void YX_InitVirOptTsk(void)
{
    YX_MEMSET(&s_mcb, 0, sizeof(s_mcb));
    // bal_GetSysTime(&s_mcb.pretime);                                             /* 记录此次复位的时间  */

    s_monitortmr = OS_InstallTmr(PRIO_OPTTASK, (void*)0, MonitorTmrProc);
    //OS_StartTmr(s_monitortmr, PERIOD_MONITOR);
}

//------------------------------------------------------------------------------
/* End of File */

