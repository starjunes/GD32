/********************************************************************************
**
** 文件名:     bal_input_drv.c
** 版权所有:   (c) 1998-2017 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现I/O口传感器输入状态检测
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
#include "bal_input_drv.h"
#include "bal_input_reg.h"
//#include "bal_debug.h"

/*
*********************************************************************************
*                   模块数据类型、变量及宏定义
*********************************************************************************
*/
// 定义配置参数
#define BASETIME                10              /* 基准采样时间为100ms,要跟PERIOD_SAMPLE对应 *///2017.11.01按方工要求，改为10
#define MAX_INPUT               15               /* IO数目 */
#define MAX_USER                8                /* 一个IO的注册用户数 */
#define PERIOD_SAMPLE           MILTICK, 1

// 定义管理控制块数据结构
typedef struct {
    INT8U  curstatus;                  /* 当前IO状态 */
    INT8U  prestatus;                  /* 前一IO状态 */
    INT8U  filterstatus;               /* 滤波后IO状态 */
    INT8U  ct_user;                    /* 一个IO注册用户数 */
    INT32U ct_filter;                  /* 滤波计数器 */
    INT8U  triggermode[MAX_USER];
    void   (*c_inform[MAX_USER])(INT8U type, INT8U mode);
    void   (*b_inform[MAX_USER])(INT8U type, INT8U mode);
} ICB_T;

// 定义本地静态变量
static INT8U s_sampletmr;
static ICB_T s_icb[MAX_INPUT];

/*
*********************************************************************************
*                   本地接口实现
*********************************************************************************
*/
/********************************************************************************
** 函数名:     SignalChangeInform
** 函数描述:   信号变化通知
** 参数:       [in] port:输入IO口
** 返回:       无
********************************************************************************/
static void SignalChangeInform(INT8U port)
{
    INT8U i;
    
    if (s_icb[port].filterstatus != 0) {                                       /* 处理上升沿触发 */
        for (i = 0; i < s_icb[port].ct_user; i++) {
            if ((s_icb[port].triggermode[i] & TRIGGER_POSITIVE) != 0) {
                OS_ASSERT((s_icb[port].c_inform[i] != 0), RETURN_VOID);
                OS_ASSERT((s_icb[port].c_inform[i] == s_icb[port].b_inform[i]), RETURN_VOID);
                
                s_icb[port].c_inform[i](port, TRIGGER_POSITIVE);
            }
        }
    } else {                                                                   /* 处理下降沿触发 */
        for (i = 0; i < s_icb[port].ct_user; i++) {
            if ((s_icb[port].triggermode[i] & TRIGGER_NEGATIVE) != 0) {
                OS_ASSERT((s_icb[port].c_inform[i] != 0), RETURN_VOID);
                OS_ASSERT((s_icb[port].c_inform[i] == s_icb[port].b_inform[i]), RETURN_VOID);
                
                s_icb[port].c_inform[i](port, TRIGGER_NEGATIVE);
            }
        }
    }
}

/********************************************************************************
** 函数名:     SetStatus
** 函数描述:   设置输入端口状态
** 参数:       无
** 返回:       无
********************************************************************************/
static void SetStatus(void)
{
    INT8U  i, j, port, fatype, nclass;
    INT32U filtertime;
    INPUT_CLASS_T const *pclass;
    INPUT_IO_T const *pio;
    BOOLEAN (*readproc)(void);

    nclass = bal_input_GetRegClassMax();
    
    for (i = 0; i < nclass; i++) {                                             /* 扫描IO类 */
        pclass = bal_input_GetRegClassInfo(i);
        pio = pclass->pio;
        fatype = pclass->fatype;
        
        if (fatype == PE_TYPE_NOUSED) {                                        /* 主板的输入IO，需要滤波 */
            for (j = 0; j < pclass->nio; j++) {
                OS_ASSERT((pclass->fatype == pio->fatype), RETURN_VOID);
                
                readproc = pio->readport;
                port = pio->port;
            
                s_icb[port].curstatus = readproc();                            /* 读取IO */
            
                if (s_icb[port].curstatus != s_icb[port].prestatus) {
                    s_icb[port].ct_filter = 0;
                    s_icb[port].prestatus = s_icb[port].curstatus;
                } else {
                    if (s_icb[port].curstatus != 0) {                          /* 获取滤波时间 */
                        filtertime = ((pio->hightime == 0) ? BASETIME : pio->hightime);
                    } else {
                        filtertime = ((pio->lowtime == 0) ? BASETIME : pio->lowtime);
                    }
                
                    if (s_icb[port].ct_filter <= filtertime) {
                        s_icb[port].ct_filter += BASETIME;
                    
                        if (s_icb[port].ct_filter == filtertime) {             /* IO口滤波完成 */
                            if (s_icb[port].curstatus != s_icb[port].filterstatus) { 
                                s_icb[port].filterstatus = s_icb[port].curstatus;
                                SignalChangeInform(port);
                            }
                        }
                    }
                }
                pio++;
            }
        } else {                                                               /* 非主板的输入IO，不需要滤波 */
            for (j = 0; j < pclass->nio; j++) {                                /* 扫描IO类中各个IO */
                OS_ASSERT((pclass->fatype == pio->fatype), RETURN_VOID);
                
                readproc = pio->readport;
                port = pio->port;
            
                s_icb[port].curstatus = readproc();
                
                if (s_icb[port].curstatus != s_icb[port].filterstatus) {       /* 状态变化 */
                    s_icb[port].filterstatus = s_icb[port].curstatus;
                    SignalChangeInform(port);
                }
                pio++;
            }
        }
    }
}

/********************************************************************************
** 函数名:     SampleTmrProc
** 函数描述:   采样周期定时器
** 参数:       无
** 返回:       无
********************************************************************************/
static void SampleTmrProc(void *pdata)
{
    pdata = pdata;
    OS_StartTmr(s_sampletmr, PERIOD_SAMPLE);
    
    SetStatus();
}

/********************************************************************************
** 函数名:     DiagnoseProc
** 函数描述:   诊断接口
** 参数:       无
** 返回:       无
********************************************************************************/
static void DiagnoseProc(void)
{
    OS_ASSERT(OS_TmrIsRun(s_sampletmr), RETURN_VOID);
}
/*
*********************************************************************************
*                   对外接口实现
*********************************************************************************
*/
/********************************************************************************
** 函数名:     bal_input_ReadSensorStatus
** 函数描述:   直接读取当前IO口状态
** 参数:       [in] type:输入口类型;
** 返回:       有效返回true，无效返回false
********************************************************************************/
INT8U bal_input_ReadSensorStatus(INT8U type)
{
    OS_ASSERT((type < bal_input_GetIOMax()), LEVEL_NEGATIVE);

    if (s_icb[type].curstatus != 0) {
        return LEVEL_POSITIVE;
    } else {
        return LEVEL_NEGATIVE;
    }
}

/********************************************************************************
** 函数名:     bal_input_InitDrv
** 函数描述:   初始输入驱动模块
** 参数:       无
** 返回:       无
********************************************************************************/
void bal_input_InitDrv(void)
{
    INT8U  i, j, port, fatype, nclass;
    INPUT_CLASS_T const *pclass;
    INPUT_IO_T const *pio;
    void (*initproc)(void);
    BOOLEAN (*readproc)(void);
    
    OS_ASSERT((bal_input_GetIOMax() <= MAX_INPUT), RETURN_VOID);
    
    YX_MEMSET(&s_icb, 0, sizeof(s_icb));
    
    nclass = bal_input_GetRegClassMax();
    for (i = 0; i < nclass; i++) {                                             /* 扫描IO类 */
        pclass = bal_input_GetRegClassInfo(i);
        pio = pclass->pio;
        fatype = pclass->fatype;
        
        for (j = 0; j < pclass->nio; j++) {                                    /* 扫描IO类中各个IO */
            OS_ASSERT((pclass->fatype == pio->fatype), RETURN_VOID);
            
            initproc = pio->initport;
            readproc = pio->readport;
            port = pio->port;
            
            initproc();                                                        /* 初始化IO */
            if (fatype == PE_TYPE_NOUSED) {                                    /* 主板的IO输入，读取IO */
                s_icb[port].curstatus = readproc();
            }
            s_icb[port].prestatus = s_icb[port].curstatus;
            s_icb[port].filterstatus = s_icb[port].curstatus;
            pio++;
        }
    }

    s_sampletmr = OS_InstallTmr(PRIO_COMMONTASK, (void *)0, SampleTmrProc);
    OS_StartTmr(s_sampletmr, PERIOD_SAMPLE);
    
    OS_InstallDiag(DiagnoseProc);
}

/********************************************************************************
** 函数名:     bal_input_ReadSensorFilterStatus
** 函数描述:   读取滤波后输入口状态
** 参数:       [in] type:输入口类型;
** 返回:       有效返回true，无效返回false
********************************************************************************/
INT8U bal_input_ReadSensorFilterStatus(INT8U type)
{
    OS_ASSERT((type < bal_input_GetIOMax()), LEVEL_NEGATIVE);
    
    if (s_icb[type].filterstatus != 0) {
        return LEVEL_POSITIVE;
    } else {
        return LEVEL_NEGATIVE;
    }
}

/********************************************************************************
** 函数名:     bal_input_InstallTriggerProc
** 函数描述:   输入口状态变化注册接口
** 参数:       [in] type:输入口类型;
**             [in] trigmode:触发模式;
**             [in] informer:信号变化通知函数
** 返回:       成功返回true，失败返回false
********************************************************************************/
BOOLEAN bal_input_InstallTriggerProc(INT8U type, INT8U trigmode, void (*informer)(INT8U type, INT8U mode))
{ 
    INT8U i;
	
	OS_ASSERT((type < bal_input_GetIOMax()), RETURN_FALSE);
	OS_ASSERT((informer != 0), RETURN_FALSE);
    
    if ((trigmode & MASK_TRIGGER) == 0) {
        return FALSE;
    }
    
    for (i = 0; i < MAX_USER; i++) {
        if (s_icb[type].triggermode[i] == 0) {
            s_icb[type].triggermode[i] = trigmode;
            s_icb[type].c_inform[i] = informer;
            s_icb[type].b_inform[i] = informer;
            s_icb[type].ct_user++;
            return TRUE;
        }
    }

    //OS_ASSERT(false, RETURN_FALSE);
    return FALSE;
}

/********************************************************************************
**  函数名:    bal_input_SetFilterPara
**  功能描述:  传感器管脚滤波参数配置接口
**  参数:  	   [in] port:   传感器管脚编号，按照通信协议
**             [in] ct_low: 低电平时间，单位：毫秒
**             [in] ct_high:高电平时间，单位：毫秒
*   返回值:    无
********************************************************************************/
void bal_input_SetFilterPara(INT8U port, INT16U ct_low, INT16U ct_high)
{
    INPUT_CLASS_T const *pclass;
    INPUT_IO_T const *pio;

    pclass = bal_input_GetRegClassInfo(port);
    pio = pclass->pio;
    
    //pio->hightime = (INT32U)ct_high;
    //pio->lowtime  = (INT32U)ct_low;

}

//------------------------------------------------------------------------------
/* End of File */
