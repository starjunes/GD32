/********************************************************************************
**
** 文件名:     yx_sleep.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现省电管理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2014/06/15 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "yx_include.h"
#include "yx_misc.h"
#include "hal_can_drv.h"
#include "hal_lowpower_drv.h"
#include "st_adc_drv.h"
#include "dal_input_drv.h"
#include "dal_gpio_cfg.h"
#include "dal_gsm_power.h"
#include "dal_pp_drv.h"
#include "yx_mmi_drv.h"
#include "yx_sleep.h"



/*
********************************************************************************
* 定义模块配置参数
********************************************************************************
*/
#define PERIOD_DELAY         _TICK, 1
#define PERIOD_WAKEUP        _SECOND, 2
#define PERIOD_WAIT          _SECOND, 5
#define PERIOD_CAN_TIMEOUT   _SECOND, 1

#define CAN_TIMEOUT 180

typedef enum {
    STEP_INIT,
    STEP_WAKEUP,             /* 唤醒 */
    STEP_GETPARA,            /* 获取参数 */
    STEP_DELAY,              /* 延时时间 */
    STEP_INFORMHOST,         /* 通知主机准备复位 */
    STEP_POWERDOWNHOST,      /* 关闭主机 */
    STEP_POWERONGSM,         /* 打开模块 */
    STEP_MAX
} STEP_E;

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    INT8U step;
    INT8U waketime;          /* 唤醒时长 */
    INT8U wakeupevent;       /* 唤醒事件,见WAKEUP_EVENT_E */
    INT16U canrec_cnt;       /* can接收计数器 */
    INT32U flag;
} DCB_T;

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static INT8U s_sleeptmr;
static INT8U s_cantmr;
static DCB_T s_dcb;

/*******************************************************************
** 函数名:     Wakeup
** 函数描述:   唤醒
** 参数:       无
** 返回:       无
********************************************************************/
static void Wakeup(void)
{
    OS_StopTmr(s_sleeptmr);

    #if DEBUG_SYS > 0
    printf_com("<唤醒,step = %d>\r\n", s_dcb.step);
    #endif

    if (s_dcb.step == STEP_POWERONGSM) {
        OS_RESET(RESET_EVENT_UNREPORT);                                        /* 主动复位，防止运行过久出现问题 */    
    }
    
    if (s_dcb.step == STEP_POWERDOWNHOST) {                                    /* 已通知主机复位,则要复位电源 */
        YX_MMI_PowerReset();
    }
    s_dcb.step = STEP_MAX;
    
    YX_MMI_PullUp();
    YX_MMI_Wakeup();
    DAL_GPIO_PullupPowerSave();
    DAL_GPIO_PullupGpsPower();
    DAL_GSM_PowerOn();
}

/*******************************************************************
** 函数名:     CanTmrProc
** 函数描述:   省电定时器哦
** 参数:       [in] pdata：定时器特征值
** 返回:       无
********************************************************************/
static void CanTmrProc(void *pdata)
{
    //printf_com("canrec_cnt:%d ad:%d ad1:%d ad2:%d\r\n", s_dcb.canrec_cnt,ST_ADC_GetValue(ADC_CH_0), ST_ADC_GetValue(ADC_CH_2), ST_ADC_GetValue(ADC_CH_1));
    
    //printf_com("canrec_cnt:%d ad:%d \r\n", s_dcb.canrec_cnt,ST_ADC_GetValue(ADC_CH_0));
    if(++s_dcb.canrec_cnt > CAN_TIMEOUT) {
        s_dcb.canrec_cnt = 0;

        #if 1
        if (!OS_TmrIsRun(s_sleeptmr) && s_dcb.step != STEP_POWERONGSM) {       /* 未启动省电定时器 */
            s_dcb.step = STEP_INFORMHOST;
            OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
        }
       #endif
    }
}

/*******************************************************************
** 函数名:     SleepTmrProc
** 函数描述:   省电定时器哦
** 参数:       [in] pdata：定时器特征值
** 返回:       无
********************************************************************/
static void SleepTmrProc(void *pdata)
{
    BOOLEAN accon, poweroff, powervin;
    SLEEP_PARA_T sleeppara;
    HOST_RESET_STATUS_T reset_info;
    
    switch (s_dcb.step)
    {
    case STEP_INIT:                                                            /* 上电初始化 */
        DAL_PP_ReadParaByID(PP_ID_SLEEP, (INT8U *)&sleeppara, sizeof(sleeppara));
        poweroff = DAL_INPUT_ReadFilterStatus(IPT_POWDECT);
        powervin = DAL_INPUT_ReadFilterStatus(IPT_VINLOW);
            
        accon = 1;
        poweroff = 0;
        
        #if DEBUG_SYS > 0
        printf_com("<省电,初始化,ACC(%d),POWERON(%d), VINLOW:(%d) ONOFF(%d)>\r\n", accon, !poweroff, !powervin, sleeppara.onoff);
        #endif
        
        if ((accon) && !poweroff && !powervin) {
        //if (1) {
            YX_MMI_PullUp();
            YX_MMI_Wakeup();
            DAL_GPIO_PullupPowerSave();
            DAL_GPIO_PullupGpsPower();
            DAL_GSM_PowerOn();
            OS_StopTmr(s_sleeptmr);
            DAL_PP_ReadParaByID(PP_ID_HOST_RESET, (INT8U *)&reset_info, sizeof(HOST_RESET_STATUS_T));    
            if (reset_info.status == MMI_RESET_EVENT_POWERDOWN) {
                reset_info.status = MMI_RESET_EVENT_POWERUP;
            } else if (reset_info.status == MMI_RESET_EVENT_SLEEP){
                reset_info.status = MMI_RESET_EVENT_WAKEUP;
            } 
            DAL_PP_StoreParaInstantByID(PP_ID_HOST_RESET, (INT8U *)&reset_info, sizeof(HOST_RESET_STATUS_T));
        } else {
            s_dcb.step = STEP_POWERDOWNHOST;
            OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
            if (!poweroff) {
                reset_info.status = MMI_RESET_EVENT_SLEEP;
                DAL_PP_StoreParaInstantByID(PP_ID_HOST_RESET, (INT8U *)&reset_info, sizeof(HOST_RESET_STATUS_T));
            }
            DAL_GPIO_PulldownCapCharge();
        }

        break;
    case STEP_WAKEUP:                                                          /* 唤醒 */
        Wakeup();
        s_dcb.step = STEP_GETPARA;
        OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
        break;
    case STEP_GETPARA:                                                         /* 获取省电参数 */
        #if DEBUG_SYS > 0
        printf_com("<省电,获取省电参数(0x%x)>\r\n", s_dcb.flag);
        #endif
        
        if (YX_MMI_IsON()) {
            OS_StartTmr(s_sleeptmr, PERIOD_WAIT);
        } else {
            OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
            s_dcb.step = STEP_DELAY;
        }
        break;
    case STEP_DELAY:                                                           /* 延时省电 */
        s_dcb.step = STEP_INFORMHOST;
        DAL_PP_ReadParaByID(PP_ID_SLEEP, (INT8U *)&sleeppara, sizeof(sleeppara));
        if (DAL_INPUT_ReadFilterStatus(IPT_POWDECT)) {                         /* 主电源断电 */
            OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
        } else if (sleeppara.onoff) {                                          /* 开启省电功能或者主电源断电 */
            if (s_dcb.waketime != 0) {
                OS_StartTmr(s_sleeptmr, _MINUTE, s_dcb.waketime);
            } else if (sleeppara.delay == 0) {
                OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
            } else {
                OS_StartTmr(s_sleeptmr, _MINUTE, sleeppara.delay);
            }
        } else {
            OS_StopTmr(s_sleeptmr);
        }
        
        #if DEBUG_SYS > 0
        printf_com("<省电,延时中,开关(%d),延时(%d)(%d)分钟>\r\n", sleeppara.onoff, sleeppara.delay, s_dcb.waketime);
        #endif
        break;
    case STEP_INFORMHOST:                                                      /* 通知主机准备复位 */
        #if DEBUG_SYS > 0
        printf_com("<省电,通知主机准备复位>\r\n");
        #endif
        
        s_dcb.waketime = 0;
        s_dcb.step = STEP_POWERDOWNHOST;
        if (DAL_INPUT_ReadFilterStatus(IPT_POWDECT)) {                         /* 主电源断电 */
            YX_MMI_SendHostResetInform(MMI_RESET_EVENT_POWERDOWN);
        } else {
            YX_MMI_SendHostResetInform(MMI_RESET_EVENT_SLEEP);
        }
        OS_StartTmr(s_sleeptmr, _SECOND, 10);
        break;
    case STEP_POWERDOWNHOST:                                                   /* 关闭主机 */
        #if DEBUG_SYS > 0
        printf_com("<省电,关闭主机>\r\n");
        #endif

        #if 0
        s_dcb.step = STEP_POWERONGSM;
        
        YX_MMI_PullDown();                                                     /* 关闭主机 */
        YX_MMI_Sleep();
#if EN_CAN > 0
        HAL_CAN_CloseCan(CAN_COM_0);                                           /* 关闭CAN */
#endif
            
        DAL_GPIO_PulldownGsmPower();                                           /* 关闭GSM */
        DAL_GPIO_PulldownGsmOnOff();
            
        DAL_GPIO_PulldownGpsPower();                                           /* 关闭GPS */
        DAL_GPIO_PulldownGpsVbat();
        DAL_GPIO_PulldownPowerSave();                                          /* 关闭外围电路 */

        s_dcb.step++ ;
        OS_StopTmr(s_sleeptmr);
        #endif
        DAL_GPIO_PulldownPowerSave();
        s_dcb.step++ ;
        OS_StopTmr(s_sleeptmr);
        HAL_LowPower_Enter(LOW_POWER_MODE_STOP, 0);
        
        break;
             
    default:
        OS_StopTmr(s_sleeptmr);
        break;
    }
}

#if 0
/*******************************************************************
** 函数名:     SignalChangeInformer_ACC
** 函数描述:   信号跳变通知处理
** 参数:       [in] port: 输入口编号，见INPUT_IO_E
**             [in] mode: 信号跳变触发模式,INPUT_TRIG_E
** 返回:       无
********************************************************************/
static void SignalChangeInformer_ACC(INT8U port, INT8U mode)
{
    HOST_RESET_STATUS_T reset_info;

    DAL_PP_ReadParaByID(PP_ID_HOST_RESET, (INT8U *)&reset_info, sizeof(HOST_RESET_STATUS_T));
#if 1
    OS_ASSERT((port == IPT_ACC), RETURN_VOID);
    
    if (mode == INPUT_TRIG_POSITIVE) {                                         /* ACC有效 */
        if (!DAL_INPUT_ReadFilterStatus(IPT_POWDECT)) {                        /* 主电源正常 */
            #if DEBUG_SYS > 0
            printf_com("<省电,ACC有效,主电源正常>\r\n");
            #endif
            
            s_dcb.wakeupevent = WAKEUP_EVENT_ACC;
            s_dcb.waketime    = 0;
            Wakeup();
            if(s_dcb.step != STEP_INIT) {
                reset_info.status = MMI_RESET_EVENT_WAKEUP;
                DAL_PP_StoreParaInstantByID(PP_ID_HOST_RESET, (INT8U *)&reset_info, sizeof(HOST_RESET_STATUS_T));
            }
        } else {
            #if DEBUG_SYS > 0
            printf_com("<省电,ACC有效,主电源断电>\r\n");
            #endif
        }
    } else {
        #if DEBUG_SYS > 0
        printf_com("<省电,ACC无效(%d)>\r\n", s_dcb.step);
        #endif
        
        if (!OS_TmrIsRun(s_sleeptmr) && s_dcb.step != STEP_POWERONGSM) {       /* 未启动省电定时器 */
            s_dcb.step = STEP_GETPARA;
            s_dcb.flag = (1 << PARA_BASE_END) - 1;
            OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
        }
    }
#endif
}

/*******************************************************************
** 函数名:     SignalChangeInformer_POWDECT
** 函数描述:   信号跳变通知处理
** 参数:       [in] port: 输入口编号，见INPUT_IO_E
**             [in] mode: 信号跳变触发模式,INPUT_TRIG_E
** 返回:       无
********************************************************************/
static void SignalChangeInformer_POWDECT(INT8U port, INT8U mode)
{
    HOST_RESET_STATUS_T reset_info;

    DAL_PP_ReadParaByID(PP_ID_HOST_RESET, (INT8U *)&reset_info, sizeof(HOST_RESET_STATUS_T));    
#if 1
    OS_ASSERT((port == IPT_POWDECT), RETURN_VOID);

    if (mode == INPUT_TRIG_POSITIVE) {                                         /* 主电源断电,直接进入省电 */
        #if DEBUG_SYS > 0
        printf_com("<省电,主电源断电>\r\n");
        #endif
        
        s_dcb.wakeupevent = WAKEUP_EVENT_MAX;
        if (!OS_TmrIsRun(s_sleeptmr) && s_dcb.step != STEP_POWERONGSM) {       /* 未启动省电定时器 */
            s_dcb.step = STEP_INFORMHOST;
            s_dcb.flag = 0;
            OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
        }
    } else {
        if (DAL_INPUT_ReadFilterStatus(IPT_ACC)) {                             /* 读取ACC状态 */
            #if DEBUG_SYS > 0
            printf_com("<省电,主电源上电,ACC有效>\r\n");
            #endif
            
            s_dcb.wakeupevent = WAKEUP_EVENT_ACC;
            s_dcb.waketime    = 0;
            Wakeup();
            reset_info.status = MMI_RESET_EVENT_POWERUP;
            DAL_PP_StoreParaInstantByID(PP_ID_HOST_RESET, (INT8U *)&reset_info, sizeof(HOST_RESET_STATUS_T));
        } else {
            #if DEBUG_SYS > 0
            printf_com("<省电,主电源上电,ACC无效>\r\n");
            #endif
       }
    }
#endif
}
#endif


/*******************************************************************
** 函数名:     SignalChangeInformer_VINLOW
** 函数描述:   信号跳变通知处理
** 参数:       [in] port: 输入口编号，见INPUT_IO_E
**             [in] mode: 信号跳变触发模式,INPUT_TRIG_E
** 返回:       无
********************************************************************/
static void SignalChangeInformer_VINLOW(INT8U port, INT8U mode)
{
    if (mode == INPUT_TRIG_POSITIVE) {
        DAL_GPIO_PulldownCapCharge();
        s_dcb.step = STEP_INFORMHOST;
        OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
    } else {

        DAL_GPIO_PullupCapCharge();
        Wakeup();
    }
}


/*******************************************************************
** 函数名:     YX_SLEEP_Init
** 函数描述:   初始化函数
** 参数:       无
** 返回:       无
********************************************************************/
void YX_SLEEP_Init(void)
{
    YX_MEMSET(&s_dcb, 0, sizeof(s_dcb));
    
    s_sleeptmr = OS_CreateTmr(TSK_ID_APP, (void *)0, SleepTmrProc);
    s_cantmr = OS_CreateTmr(TSK_ID_APP, (void *)0, CanTmrProc);
    OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
    OS_StartTmr(s_cantmr, PERIOD_CAN_TIMEOUT);

    DAL_INPUT_InstallTriggerProc(IPT_VINLOW, INPUT_TRIG_POSITIVE | INPUT_TRIG_NEGATIVE, SignalChangeInformer_VINLOW);
    //DAL_INPUT_InstallTriggerProc(IPT_ACC, INPUT_TRIG_POSITIVE | INPUT_TRIG_NEGATIVE, SignalChangeInformer_ACC);
    //DAL_INPUT_InstallTriggerProc(IPT_POWDECT, INPUT_TRIG_POSITIVE | INPUT_TRIG_NEGATIVE, SignalChangeInformer_POWDECT);
}

/*******************************************************************
** 函数名:     YX_SLEEP_Wakeup
** 函数描述:   唤醒设备
** 参数:       [in] waketime: 唤醒时长,单位:分钟
** 参数:       [in] event:    唤醒事件,见 WAKEUP_EVENT_E
** 返回:       无
********************************************************************/
void YX_SLEEP_Wakeup(INT8U waketime, INT8U event)
{
    HOST_RESET_STATUS_T reset_info;

    DAL_PP_ReadParaByID(PP_ID_HOST_RESET, (INT8U *)&reset_info, sizeof(HOST_RESET_STATUS_T));   
    if (!DAL_INPUT_ReadFilterStatus(IPT_POWDECT)) {
        s_dcb.wakeupevent = event;
        s_dcb.step = STEP_WAKEUP;
        OS_StartTmr(s_sleeptmr, PERIOD_WAKEUP);
    }
    s_dcb.waketime = waketime;
    reset_info.status = MMI_RESET_EVENT_WAKEUP;
    DAL_PP_StoreParaByID(PP_ID_HOST_RESET, (INT8U *)&reset_info, sizeof(HOST_RESET_STATUS_T));
}

/*******************************************************************
** 函数名:     YX_SLEEP_GetWakeupEvent
** 函数描述:   获取唤醒设备事件
** 参数:       无
** 返回:       见 WAKEUP_EVENT_E
********************************************************************/
INT8U YX_SLEEP_GetWakeupEvent(void)
{
    return s_dcb.wakeupevent;
}

/*******************************************************************
** 函数名:     YX_SLEEP_ConfirmRecCan
** 函数描述:   确认收到can消息
** 参数:       无
** 返回:       见 void
********************************************************************/
void YX_SLEEP_ConfirmRecCan(INT8U *candata)
{
    s_dcb.canrec_cnt = 0;
}


