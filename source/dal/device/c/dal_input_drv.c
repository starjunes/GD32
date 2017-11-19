/********************************************************************************
**
** 文件名:     dal_input_drv.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现I/O口传感器输入状态检测
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2010/08/09 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "yx_include.h"
#include "st_gpio_drv.h"
#include "dal_input_drv.h"
#include "yx_debug.h"

/*
********************************************************************************
* 定义配置参数
********************************************************************************
*/
#define BASETIME_               40              /* 基准采样时间为100ms,要跟PERIOD_SAMPLE对应 */
#define MAX_INPUT               20               /* IO数目 */
#define MAX_USER                4                /* 一个IO的注册用户数 */


#define PERIOD_SAMPLE           _TICK, 4

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    INT8U  curstatus;                  /* 当前IO状态 */
    INT8U  prestatus;                  /* 前一IO状态 */
    INT8U  filterstatus;               /* 滤波后IO状态 */
    INT8U  ct_user;                    /* 一个IO注册用户数 */
    INT32U ct_filter;                  /* 滤波计数器 */
    INT8U  triggermode[MAX_USER];
    void   (*c_inform[MAX_USER])(INT8U port, INT8U mode);
    void   (*b_inform[MAX_USER])(INT8U port, INT8U mode);
} ICB_T;

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static INT8U s_sampletmr;
static ICB_T s_dcb[MAX_INPUT];



/*******************************************************************
** 函数名:     SignalChangeInform
** 函数描述:   信号变化通知
** 参数:       [in] port:输入IO口
** 返回:       无
********************************************************************/
static void SignalChangeInform(INT8U port)
{
    INT8U i;
    
    if (s_dcb[port].filterstatus != 0) {                                       /* 处理上升沿触发 */
        for (i = 0; i < s_dcb[port].ct_user; i++) {
            if ((s_dcb[port].triggermode[i] & INPUT_TRIG_POSITIVE) != 0) {
                OS_ASSERT((s_dcb[port].c_inform[i] != 0), RETURN_VOID);
                OS_ASSERT((s_dcb[port].c_inform[i] == s_dcb[port].b_inform[i]), RETURN_VOID);
                
                s_dcb[port].c_inform[i](port, INPUT_TRIG_POSITIVE);
            }
        }
    } else {                                                                   /* 处理下降沿触发 */
        for (i = 0; i < s_dcb[port].ct_user; i++) {
            if ((s_dcb[port].triggermode[i] & INPUT_TRIG_NEGATIVE) != 0) {
                OS_ASSERT((s_dcb[port].c_inform[i] != 0), RETURN_VOID);
                OS_ASSERT((s_dcb[port].c_inform[i] == s_dcb[port].b_inform[i]), RETURN_VOID);
                
                s_dcb[port].c_inform[i](port, INPUT_TRIG_NEGATIVE);
            }
        }
    }
}

/*******************************************************************
** 函数名:     SetStatus
** 函数描述:   设置输入端口状态
** 参数:       无
** 返回:       无
********************************************************************/
static void SetStatus(void)
{
    INT8U  i, j, port, fatype, nclass;
    INT32U filtertime;
    INPUT_CLASS_T const *pclass;
    INPUT_IO_T const *pio;
    BOOLEAN (*readproc)(INT8U port);

    nclass = DAL_INPUT_GetRegClassMax();
    
    for (i = 0; i < nclass; i++) {                                             /* 扫描IO类 */
        pclass = DAL_INPUT_GetRegClassInfo(i);
        pio = pclass->pio;
        fatype = pclass->fatype;
        
        if (fatype == PE_TYPE_NULL) {                                          /* 主板的输入IO，需要滤波 */
            for (j = 0; j < pclass->nio; j++) {
                OS_ASSERT((pclass->fatype == pio->fatype), RETURN_VOID);
                
                readproc = pio->readport;
                port = pio->port;
            
                s_dcb[port].curstatus = readproc(port);                        /* 读取IO */
            
                if (s_dcb[port].curstatus != s_dcb[port].prestatus) {
                    s_dcb[port].ct_filter = 0;
                    s_dcb[port].prestatus = s_dcb[port].curstatus;
                } else {
                    if (s_dcb[port].curstatus != 0) {                          /* 获取滤波时间 */
                        filtertime = ((pio->hightime == 0) ? BASETIME_ : pio->hightime);
                    } else {
                        filtertime = ((pio->lowtime == 0) ? BASETIME_ : pio->lowtime);
                    }
                
                    if (s_dcb[port].ct_filter < filtertime) {
                        s_dcb[port].ct_filter += BASETIME_;
                    
                        if (s_dcb[port].ct_filter >= filtertime) {             /* IO口滤波完成 */
                            if (s_dcb[port].curstatus != s_dcb[port].filterstatus) { 
                                s_dcb[port].filterstatus = s_dcb[port].curstatus;
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
            
                s_dcb[port].curstatus = readproc(port);
                
                if (s_dcb[port].curstatus != s_dcb[port].filterstatus) {       /* 状态变化 */
                    s_dcb[port].filterstatus = s_dcb[port].curstatus;
                    SignalChangeInform(port);
                }
                pio++;
            }
        }
    }
}

/*******************************************************************
** 函数名:     SampleTmrProc
** 函数描述:   采样周期定时器
** 参数:       无
** 返回:       无
********************************************************************/
static void SampleTmrProc(void *pdata)
{
    pdata = pdata;
    OS_StartTmr(s_sampletmr, PERIOD_SAMPLE);
    
    SetStatus();
}

/*******************************************************************
** 函数名:     DiagnoseProc
** 函数描述:   诊断接口
** 参数:       无
** 返回:       无
********************************************************************/
static void DiagnoseProc(void)
{
    OS_ASSERT(OS_TmrIsRun(s_sampletmr), RETURN_VOID);
}

/*******************************************************************
** 函数名:     DAL_INPUT_InitDrv
** 函数描述:   初始输入驱动模块
** 参数:       无
** 返回:       无
********************************************************************/
void DAL_INPUT_InitDrv(void)
{
    INT8U  i, j, port, fatype, nclass;
    INPUT_CLASS_T const *pclass;
    INPUT_IO_T const *pio;
    void (*initproc)(INT8U port);
    BOOLEAN (*readproc)(INT8U port);
    
    OS_ASSERT((DAL_INPUT_GetIOMax() <= MAX_INPUT), RETURN_VOID);
    
    YX_MEMSET(&s_dcb, 0, sizeof(s_dcb));
    
    nclass = DAL_INPUT_GetRegClassMax();
    for (i = 0; i < nclass; i++) {                                             /* 扫描IO类 */
        pclass = DAL_INPUT_GetRegClassInfo(i);
        pio = pclass->pio;
        fatype = pclass->fatype;
        
        for (j = 0; j < pclass->nio; j++) {                                    /* 扫描IO类中各个IO */
            OS_ASSERT((pclass->fatype == pio->fatype), RETURN_VOID);
            
            initproc = pio->initport;
            readproc = pio->readport;
            port = pio->port;
            //pin  = pio->pin;
            
            //if (pin < ST_GPIO_GetCfgTblMax()) {                                /* 物理GPIO */
            //    ST_GPIO_SetPin(pin, GPIO_DIR_IN, GPIO_MODE_UP, 1);
            //} else {
                initproc(port);                                                /* 初始化IO */
            //}
            
            if (fatype == PE_TYPE_NULL) {                                      /* 主板的IO输入，读取IO */
                //if (pin < ST_GPIO_GetCfgTblMax()) {                            /* 物理GPIO */
                //    s_dcb[port].curstatus = ST_GPIO_ReadPin(pin);
                //} else {
                    s_dcb[port].curstatus = readproc(port);
                //}
            }
            
            s_dcb[port].prestatus = s_dcb[port].curstatus;
            s_dcb[port].filterstatus = s_dcb[port].curstatus;
            pio++;
        }
    }

    s_sampletmr = OS_CreateTmr(TSK_ID_DAL, (void *)0, SampleTmrProc);
    OS_StartTmr(s_sampletmr, PERIOD_SAMPLE);
    
    OS_RegistDiagnoseProc(DiagnoseProc);
}

/*******************************************************************
** 函数名:     DAL_INPUT_ReadInstantStatus
** 函数描述:   读取当前IO口实时状态(无滤波)
** 参数:       [in] port: 输入口编号，见INPUT_IO_E
** 返回:       有效返回true，无效返回false
********************************************************************/
INT8U DAL_INPUT_ReadInstantStatus(INT8U port)
{
    OS_ASSERT((port < DAL_INPUT_GetIOMax()), LEVEL_NEGATIVE);
    
    if (s_dcb[port].curstatus != 0) {
        return LEVEL_POSITIVE;
    } else {
        return LEVEL_NEGATIVE;
    }
}

/*******************************************************************
** 函数名:     DAL_INPUT_ReadFilterStatus
** 函数描述:   读取滤波后输入口状态
** 参数:       [in] port: 输入口编号，见INPUT_IO_E
** 返回:       有效返回true，无效返回false
********************************************************************/
INT8U DAL_INPUT_ReadFilterStatus(INT8U port)
{
    OS_ASSERT((port < DAL_INPUT_GetIOMax()), LEVEL_NEGATIVE);
    
    if (s_dcb[port].filterstatus != 0) {
        return LEVEL_POSITIVE;
    } else {
        return LEVEL_NEGATIVE;
    }
}


/*******************************************************************
** 函数名:     DAL_INPUT_InstallTriggerProc
** 函数描述:   注册输入口状态变化回调通知
** 参数:       [in] port:     输入口编号,见INPUT_IO_E
**             [in] trigmode: 信号跳变触发模式,INPUT_TRIG_E
**             [in] informer: 信号变化通知函数
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN DAL_INPUT_InstallTriggerProc(INT8U port, INT8U trigmode, void (*informer)(INT8U port, INT8U mode))
{ 
    INT8U i;
	
	OS_ASSERT((port < DAL_INPUT_GetIOMax()), RETURN_FALSE);
	OS_ASSERT((informer != 0), RETURN_FALSE);
    
    if ((trigmode & INPUT_TRIG_MASK) == 0) {
        return FALSE;
    }
    
    for (i = 0; i < MAX_USER; i++) {
        if (s_dcb[port].triggermode[i] == 0) {
            s_dcb[port].triggermode[i] = trigmode;
            s_dcb[port].c_inform[i] = informer;
            s_dcb[port].b_inform[i] = informer;
            s_dcb[port].ct_user++;
            return TRUE;
        }
    }

    OS_ASSERT(false, RETURN_FALSE);
}

/***************************************************************
** 函数名:    DAL_INPUT_SetFilterPara
**  功能描述:  输入口传感器滤波参数配置接口
** 参数:       [in] port:   输入口编号,见INPUT_IO_E
**             [in] ct_low: 低电平滤波时间，单位：毫秒
**             [in] ct_high:高电平滤波时间，单位：毫秒
*   返回值:    无
***************************************************************/
void DAL_INPUT_SetFilterPara(INT8U port, INT16U ct_low, INT16U ct_high)
{
    port = port;
    ct_low = ct_low;
    ct_high = ct_high;
}

