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
#include "dal_input_drv.h"
#include "dal_gpio_cfg.h"
#include "dal_pp_drv.h"
#include "at_drv.h"
#include "yx_mmi_drv.h"
#include "yx_sleep.h"
#include "yx_jt_linkman.h"

#if EN_AT > 0

/*
********************************************************************************
* 定义模块配置参数
********************************************************************************
*/
#define PERIOD_DELAY         _TICK, 1
#define PERIOD_WAKEUP        _SECOND, 2
#define PERIOD_WAIT          _SECOND, 10

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
    INT32U flag;
} DCB_T;

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static INT8U s_sleeptmr;
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
    AT_DRV_Close();
    YX_JT_CloseLink();
    if (s_dcb.step == STEP_POWERDOWNHOST) {                                    /* 已通知主机复位,则要复位电源 */
        YX_MMI_PowerReset();
    }
    s_dcb.step = STEP_MAX;
    
    YX_MMI_PullUp();
    YX_MMI_Wakeup();
    DAL_GPIO_PullupPowerSave();
}

/*******************************************************************
** 函数名:     GetPara
** 函数描述:   获取参数
** 参数:       无
** 返回:       无
********************************************************************/
static void Callback_GetSleepPara(INT8U result)
{
    OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
    //if (result == _SUCCESS) {
        s_dcb.flag &= ~(1 << PARA_SLEEP);
    //}
}

static void Callback_GetMytel(INT8U result)
{
    OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
    s_dcb.flag &= ~(1 << PARA_MYTEL);
}

static void Callback_GetSmscTel(INT8U result)
{
    OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
    s_dcb.flag &= ~(1 << PARA_SMSC);
}

static void Callback_GetAlarmTel(INT8U result)
{
    OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
    s_dcb.flag &= ~(1 << PARA_ALARMTEL);
}

static void Callback_GetServer1Main(INT8U result)
{
    OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
    s_dcb.flag &= ~(1 << PARA_SERVER1_MAIN);
}

static void Callback_GetServer1Back(INT8U result)
{
    OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
    s_dcb.flag &= ~(1 << PARA_SERVER1_BACK);
}

static void Callback_GetServer1Attrib(INT8U result)
{
    OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
    s_dcb.flag &= ~(1 << PARA_SERVER1_ATTRIB);
}

static void Callback_GetServer1Authcode(INT8U result)
{
    OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
    s_dcb.flag &= ~(1 << PARA_SERVER1_AUTHCODE);
}

static void Callback_GetServer1Link(INT8U result)
{
    OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
    s_dcb.flag &= ~(1 << PARA_SERVER1_LINK);
}

static void Callback_GetServer2Main(INT8U result)
{
    OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
    s_dcb.flag &= ~(1 << PARA_SERVER2_MAIN);
}

static void Callback_GetServer2Back(INT8U result)
{
    OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
    s_dcb.flag &= ~(1 << PARA_SERVER2_BACK);
}

static void Callback_GetServer2Attrib(INT8U result)
{
    OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
    s_dcb.flag &= ~(1 << PARA_SERVER2_ATTRIB);
}

static void Callback_GetServer2Authcode(INT8U result)
{
    OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
    s_dcb.flag &= ~(1 << PARA_SERVER2_AUTHCODE);
}

static void Callback_GetServer2Link(INT8U result)
{
    OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
    s_dcb.flag &= ~(1 << PARA_SERVER2_LINK);
}

static void Callback_GetVehicheProvince(INT8U result)
{
    OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
    s_dcb.flag &= ~(1 << PARA_VEHICHE_PROVINCE);
}

static void Callback_GetVehicheCode(INT8U result)
{
    OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
    s_dcb.flag &= ~(1 << PARA_VEHICHE_CODE);
}

static void Callback_GetVehicheColour(INT8U result)
{
    OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
    s_dcb.flag &= ~(1 << PARA_VEHICHE_COLOUR);
}

static void Callback_GetVehicheBrand(INT8U result)
{
    OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
    s_dcb.flag &= ~(1 << PARA_VEHICHE_BRAND);
}

static void Callback_GetVehicheVin(INT8U result)
{
    OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
    s_dcb.flag &= ~(1 << PARA_VEHICHE_VIN);
}

static void Callback_GetDeviceInfo(INT8U result)
{
    OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
    s_dcb.flag &= ~(1 << PARA_DEVICEINFO);
}

static void Callback_GetAutoReptPara(INT8U result)
{
    OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
    s_dcb.flag &= ~(1 << PARA_AUTOREPT);
    s_dcb.step = STEP_DELAY;
}


static void GetPara(void)
{
    if ((s_dcb.flag & (1 << PARA_SLEEP)) != 0) {                               /* 省电参数 */
        YX_MMI_QueryCommonPara(PARA_SLEEP, Callback_GetSleepPara);
        return;
    }
    
    if ((s_dcb.flag & (1 << PARA_MYTEL)) != 0) {                               /* 本机号码 */
        YX_MMI_QueryCommonPara(PARA_MYTEL, Callback_GetMytel);
        return;
    }
    
    if ((s_dcb.flag & (1 << PARA_SMSC)) != 0) {                                /* 短信中心号码 */
        YX_MMI_QueryCommonPara(PARA_SMSC, Callback_GetSmscTel);
        return;
    }
    
    if ((s_dcb.flag & (1 << PARA_ALARMTEL)) != 0) {                            /* 报警号码 */
        YX_MMI_QueryCommonPara(PARA_ALARMTEL, Callback_GetAlarmTel);
        return;
    }
    
    if ((s_dcb.flag & (1 << PARA_SERVER1_MAIN)) != 0) {                        /* 第一服务器通信参数（主） */
        YX_MMI_QueryCommonPara(PARA_SERVER1_MAIN, Callback_GetServer1Main);
        return;
    }
    
    if ((s_dcb.flag & (1 << PARA_SERVER1_BACK)) != 0) {                        /* 第一服务器通信参数（副） */
        YX_MMI_QueryCommonPara(PARA_SERVER1_BACK, Callback_GetServer1Back);
        return;
    }
    
    if ((s_dcb.flag & (1 << PARA_SERVER1_ATTRIB)) != 0) {                      /* 第一服务器通信属性 */
        YX_MMI_QueryCommonPara(PARA_SERVER1_ATTRIB, Callback_GetServer1Attrib);
        return;
    }
    
    if ((s_dcb.flag & (1 << PARA_SERVER1_AUTHCODE)) != 0) {                    /* 第一服务器鉴权码 */
        YX_MMI_QueryCommonPara(PARA_SERVER1_AUTHCODE, Callback_GetServer1Authcode);
        return;
    }
    
    if ((s_dcb.flag & (1 << PARA_SERVER1_LINK)) != 0) {                        /* 第一服务器链路维护参数 */
        YX_MMI_QueryCommonPara(PARA_SERVER1_LINK, Callback_GetServer1Link);
        return;
    }
    
    if ((s_dcb.flag & (1 << PARA_SERVER2_MAIN)) != 0) {                        /* 第二服务器通信参数（主） */
        YX_MMI_QueryCommonPara(PARA_SERVER2_MAIN, Callback_GetServer2Main);
        return;
    }
    
    if ((s_dcb.flag & (1 << PARA_SERVER2_BACK)) != 0) {                        /* 第二服务器通信参数（副） */
        YX_MMI_QueryCommonPara(PARA_SERVER2_BACK, Callback_GetServer2Back);
        return;
    }
    
    if ((s_dcb.flag & (1 << PARA_SERVER2_ATTRIB)) != 0) {                      /* 第二服务器通信属性 */
        YX_MMI_QueryCommonPara(PARA_SERVER2_ATTRIB, Callback_GetServer2Attrib);
        return;
    }
    
    if ((s_dcb.flag & (1 << PARA_SERVER2_AUTHCODE)) != 0) {                    /* 第二服务器鉴权码 */
        YX_MMI_QueryCommonPara(PARA_SERVER2_AUTHCODE, Callback_GetServer2Authcode);
        return;
    }
    
    if ((s_dcb.flag & (1 << PARA_SERVER2_LINK)) != 0) {                        /* 第二服务器链路维护参数 */
        YX_MMI_QueryCommonPara(PARA_SERVER2_LINK, Callback_GetServer2Link);
        return;
    }
    
    if ((s_dcb.flag & (1 << PARA_VEHICHE_PROVINCE)) != 0) {                    /* 车辆归属地 */
        YX_MMI_QueryCommonPara(PARA_VEHICHE_PROVINCE, Callback_GetVehicheProvince);
        return;
    }
    
    if ((s_dcb.flag & (1 << PARA_VEHICHE_CODE)) != 0) {                        /* 车牌号 */
        YX_MMI_QueryCommonPara(PARA_VEHICHE_CODE, Callback_GetVehicheCode);
        return;
    }
    
    if ((s_dcb.flag & (1 << PARA_VEHICHE_COLOUR)) != 0) {                      /* 车辆颜色 */
        YX_MMI_QueryCommonPara(PARA_VEHICHE_COLOUR, Callback_GetVehicheColour);
        return;
    }
    
    if ((s_dcb.flag & (1 << PARA_VEHICHE_BRAND)) != 0) {                       /* 车辆分类 */
        YX_MMI_QueryCommonPara(PARA_VEHICHE_BRAND, Callback_GetVehicheBrand);
        return;
    }
    
    if ((s_dcb.flag & (1 << PARA_VEHICHE_VIN)) != 0) {                         /* 车辆VIN */
        YX_MMI_QueryCommonPara(PARA_VEHICHE_VIN, Callback_GetVehicheVin);
        return;
    }
    
    if ((s_dcb.flag & (1 << PARA_DEVICEINFO)) != 0) {                          /* 设备信息 */
        YX_MMI_QueryCommonPara(PARA_DEVICEINFO, Callback_GetDeviceInfo);
        return;
    }
    
    if ((s_dcb.flag & (1 << PARA_AUTOREPT)) != 0) {                            /* 主动上报参数 */
        YX_MMI_QueryCommonPara(PARA_DATA_GPS, 0);
        YX_MMI_QueryCommonPara(PARA_AUTOREPT, Callback_GetAutoReptPara);
        return;
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
    BOOLEAN accon, poweroff;
    SLEEP_PARA_T sleeppara;
    
    switch (s_dcb.step)
    {
    case STEP_INIT:                                                            /* 上电初始化 */
        DAL_PP_ReadParaByID(PP_ID_SLEEP, (INT8U *)&sleeppara, sizeof(sleeppara));
        accon = DAL_INPUT_ReadFilterStatus(IPT_ACC);
        poweroff = DAL_INPUT_ReadFilterStatus(IPT_POWDECT);
        
        #if DEBUG_SYS > 0
        printf_com("<省电,初始化,ACC(%d),POWERON(%d),ONOFF(%d)>\r\n", accon, !poweroff, sleeppara.onoff);
        #endif
        
        if ((accon || !sleeppara.onoff) && !poweroff) {
        //if (1) {
            YX_MMI_PullUp();
            YX_MMI_Wakeup();
            DAL_GPIO_PullupPowerSave();
            OS_StopTmr(s_sleeptmr);
        } else {
            s_dcb.step = STEP_POWERDOWNHOST;
            OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
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
            GetPara();
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
            if (s_dcb.waketime > sleeppara.delay) {
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
        OS_StartTmr(s_sleeptmr, PERIOD_WAIT);
        break;
    case STEP_POWERDOWNHOST:                                                   /* 关闭主机 */
        #if DEBUG_SYS > 0
        printf_com("<省电,关闭主机>\r\n");
        #endif
        
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
        
        OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
        break;
    case STEP_POWERONGSM:                                                      /* 打开模块 */
        DAL_PP_ReadParaByID(PP_ID_SLEEP, (INT8U *)&sleeppara, sizeof(sleeppara));
        if (sleeppara.networkmode == SLEEP_NETWORK_GPRS) {                     /* GPRS在线 */
            AT_DRV_Open();
            YX_JT_OpenLink();
        } else if (sleeppara.networkmode == SLEEP_NETWORK_GSM) {               /* GSM在线 */
            AT_DRV_Open();
            YX_JT_CloseLink();
        } else {                                                               /* 关闭网络 */
            AT_DRV_Close();
            YX_JT_CloseLink();
        }
        OS_StopTmr(s_sleeptmr);
        
        #if DEBUG_SYS > 0
        printf_com("<省电,开启手机模块,网络模式(%d)>\r\n", sleeppara.networkmode);
        #endif
        break;
    default:
        OS_StopTmr(s_sleeptmr);
        break;
    }
}

/*******************************************************************
** 函数名:     SignalChangeInformer_ACC
** 函数描述:   信号跳变通知处理
** 参数:       [in] port: 输入口编号，见INPUT_IO_E
**             [in] mode: 信号跳变触发模式,INPUT_TRIG_E
** 返回:       无
********************************************************************/
static void SignalChangeInformer_ACC(INT8U port, INT8U mode)
{
#if 1
    OS_ASSERT((port == IPT_ACC), RETURN_VOID);
    
    if (mode == INPUT_TRIG_POSITIVE) {                                         /* ACC有效 */
        if (!DAL_INPUT_ReadFilterStatus(IPT_POWDECT)) {                        /* 主电源正常 */
            #if DEBUG_SYS > 0
            printf_com("<省电,ACC有效,主电源正常>\r\n");
            #endif
            
            s_dcb.wakeupevent = WAKEUP_EVENT_ACC;
            Wakeup();
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
#if 1
    OS_ASSERT((port == IPT_POWDECT), RETURN_VOID);

    if (mode == INPUT_TRIG_POSITIVE) {                                         /* 主电源断电,直接进入省电 */
        #if DEBUG_SYS > 0
        printf_com("<省电,主电源断电>\r\n");
        #endif
        
        s_dcb.wakeupevent = WAKEUP_EVENT_MAX;
        if (!OS_TmrIsRun(s_sleeptmr) && s_dcb.step != STEP_POWERONGSM) {       /* 未启动省电定时器 */
            s_dcb.step = STEP_GETPARA;
            s_dcb.flag = (1 << PARA_BASE_END) - 1;
            OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
        }
    } else {
        if (DAL_INPUT_ReadFilterStatus(IPT_ACC)) {                             /* 读取ACC状态 */
            #if DEBUG_SYS > 0
            printf_com("<省电,主电源上电,ACC有效>\r\n");
            #endif
            
            s_dcb.wakeupevent = WAKEUP_EVENT_ACC;
            Wakeup();
        } else {
            #if DEBUG_SYS > 0
            printf_com("<省电,主电源上电,ACC无效>\r\n");
            #endif
        }
    }
#endif
}

/*******************************************************************
** 函数名:     SignalChangeInformer_ROB
** 函数描述:   信号跳变通知处理
** 参数:       [in] port: 输入口编号，见INPUT_IO_E
**             [in] mode: 信号跳变触发模式,INPUT_TRIG_E
** 返回:       无
********************************************************************/
static void SignalChangeInformer_ROB(INT8U port, INT8U mode)
{
    OS_ASSERT((port == IPT_ROB), RETURN_VOID);
    
    if (mode == INPUT_TRIG_POSITIVE) {                                         /* 抢劫有效 */
        if (!DAL_INPUT_ReadFilterStatus(IPT_POWDECT)) {                        /* 主电源正常 */
            #if DEBUG_SYS > 0
            printf_com("<省电,抢劫有效,主电源正常>\r\n");
            #endif
            
            YX_SLEEP_Wakeup(20, WAKEUP_EVENT_ROB);
        } else {
            #if DEBUG_SYS > 0
            printf_com("<省电,抢劫有效,主电源断电>\r\n");
            #endif
        }
    } else {
        #if DEBUG_SYS > 0
        printf_com("<省电,抢劫无效(%d)>\r\n", s_dcb.step);
        #endif
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
    OS_StartTmr(s_sleeptmr, PERIOD_DELAY);
    
    DAL_INPUT_InstallTriggerProc(IPT_ACC, INPUT_TRIG_POSITIVE | INPUT_TRIG_NEGATIVE, SignalChangeInformer_ACC);
    DAL_INPUT_InstallTriggerProc(IPT_POWDECT, INPUT_TRIG_POSITIVE | INPUT_TRIG_NEGATIVE, SignalChangeInformer_POWDECT);
    DAL_INPUT_InstallTriggerProc(IPT_ROB, INPUT_TRIG_POSITIVE | INPUT_TRIG_NEGATIVE, SignalChangeInformer_ROB);
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
    if (!DAL_INPUT_ReadFilterStatus(IPT_POWDECT)) {
        s_dcb.wakeupevent = event;
        s_dcb.step = STEP_WAKEUP;
        OS_StartTmr(s_sleeptmr, PERIOD_WAKEUP);
    }
    s_dcb.waketime = waketime;
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

#endif

