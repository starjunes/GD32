/********************************************************************************
**
** 文件名:     port_gsensor.c
** 版权所有:   (c) 1998-2017 厦门雅迅网络股份有限公司
** 文件描述:   该模块实现g-sensor模块相关功能接口封装和事件处理
**
*********************************************************************************
**                  修改历史记录
**===============================================================================
**| 日期              | 作者        |  修改记录
**===============================================================================
**| 2017/05/12 | 黄运峰    |  创建该文件
********************************************************************************/
//#include "yx_includes.h"
#include "port_gsensor.h"
#include "dal_gsen_drv.h"
#include "port_msghdl.h"

/********************************************************************************
** 函数名:     PORT_Hdl_SHELL_MSG_GSENSOR_EVENT
** 函数描述:   处理gsensor事件:碰撞,振动等
** 参数:       [in] msg: 为事件类型, 具体定义参见GSEN_EVENT_E
** 返回:       成功返回true，失败返回false
********************************************************************************/
#if DEBUG_GSENSOR > 0
static void ReadAndPrintHitData(void)
{
    AXIS_DAT_T hitdata;
    PORT_ReadHitAcclData(&hitdata);
    debug_printf("hit_accl_data: x:%d, y:%d, z:%d \r\n",hitdata.axis_x,
            hitdata.axis_y, hitdata.axis_z);
}
#endif

BOOLEAN PORT_Hdl_SHELL_MSG_GSENSOR_EVENT(void *msg)
{
		return false;
#if 0
    INT32U evnt;

    evnt = (INT32U)msg;
    if (evnt == GSEN_EVT_HIT) {
        OS_APostMsg(TRUE, TSK_ID_OPT, MSG_OPT_GSEN_HIT_EVNT, 0, 0);
#if DEBUG_GSENSOR > 0
        ReadAndPrintHitData();
#endif
    } else if (evnt == GSEN_EVT_MOTION) {
        OS_APostMsg(TRUE, TSK_ID_OPT, MSG_OPT_GSEN_MOTION_EVNT, 0, 0);
    } else {
        return FALSE;
    }

    return TRUE;
#endif
}
/********************************************************************************
** 函数名:     PORT_Hdl_SHELL_MSG_WAKE_EVENT
** 函数描述:   处理gsensor事件:碰撞,振动等
** 参数:       [in] msg: 为事件类型, 具体定义参见SHELL_MCU_WKUP_EVNT_E
** 返回:       成功返回true，失败返回false
********************************************************************************/
BOOLEAN PORT_Hdl_SHELL_MSG_WAKE_EVENT(void *msg)
{
#if 1
    return FALSE;
#else
    INT32U evnt;
    evnt = (INT32U)msg;
#if DEBUG_RTC_SLEEP > 1
    switch ((INT32U)msg) {
        case SHELL_WKUP_EVNT_MOTION:
            debug_printf("中断震动唤醒-消息\r\n");
            break;
        case SHELL_WKUP_EVNT_HIT:
            debug_printf("中断碰撞唤醒-消息\r\n");
            break;
        case SHELL_WKUP_EVNT_RTCALM:
            debug_printf("中断RTC唤醒-消息\r\n");
            break;
        case SHELL_WKUP_EVNT_ACCON:
            debug_printf("中断ACC唤醒-消息\r\n");
            break;
        case SHELL_WKUP_EVNT_CAN0:
            debug_printf("中断CAN0ERR唤醒-消息\r\n");
            break;
        case SHELL_WKUP_EVNT_CHGCAR:
            debug_printf("中断充电信号唤醒-消息\r\n");
            break;
        case SHELL_WKUP_EVNT_MCUWK:
            debug_printf("中断GSM唤醒S32唤醒-消息\r\n");
            break;
        case SHELL_WKUP_EVNT_PWRDECT:
        	debug_printf("主电切换唤醒S32唤醒-消息\r\n");
        	break;
        default:
            break;
    }
#endif
    if ((evnt == SHELL_WKUP_EVNT_MOTION) || (evnt == SHELL_WKUP_EVNT_HIT) || (evnt == SHELL_WKUP_EVNT_ACCON)
            || (evnt == SHELL_WKUP_EVNT_CAN0) || (evnt == SHELL_WKUP_EVNT_CHGCAR) || (evnt == SHELL_WKUP_EVNT_MCUWK)
			|| (evnt == SHELL_WKUP_EVNT_PWRDECT)) {
        OS_APostMsg(TRUE, TSK_ID_OPT, MSG_OPT_OTWAKE_ENENT, 0, 0);
    } else if (evnt == SHELL_WKUP_EVNT_RTCALM) {
        OS_APostMsg(TRUE, TSK_ID_OPT, MSG_OPT_RTCWK_EVENT, 0, 0);
    } else {
        return FALSE;
    }
    return TRUE;
#endif    
}
/********************************************************************************
**  函数名:     PORT_GsenCalibStart
**  函数描述:   启动标定
**  参数:       [in] callback: 回调函数
**  返回:       无
********************************************************************************/
void PORT_GsenCalibStart(GSEN_CALIB_CALBAK calbak)
{
    dal_gsen_CalibStart((GSEN_CALIB_CALBAK_PTR)calbak);
}
/********************************************************************************
**  函数名:     PORT_GsenGetWorkState
**  函数描述:   查询GSensor状态
**  参数:       无
**  返回:       Gsensor状态 0 未运行  1 已运行
********************************************************************************/
INT8U PORT_GsenGetWorkState(void)
{
    return dal_gsen_GetWorkState();
}
/********************************************************************************
** 函数名:     PORT_GsenEnableTemp
** 函数描述:   使能温度数据采集功能，默认初始化完是关闭
** 参数:       无
** 返回:       无
********************************************************************************/
BOOLEAN PORT_GsenEnableTemp(void)
{
    return dal_gsen_EnableTemp();
}

/********************************************************************************
**  函数名:     PORT_CloseGsensor
**  函数描述:   gsensor进入低功耗模式，系统进入休眠模式前调用
**  参数:       无
**  返回:       无
********************************************************************************/
void PORT_SleepGsensor(void)
{
	dal_gsen_EnterLowPower();
}

/********************************************************************************
**  函数名:     PORT_CloseGsensor
**  函数描述:   关闭Gsensor,在运输模式下使用
**  参数:       无
**  返回:       无
********************************************************************************/
void PORT_CloseGsensor(void)
{
    dal_gsen_EnterSleep();
}
/********************************************************************************
**  函数名:     PORT_GsenSetHitPara
**  函数描述:   设置阀值和三轴标定值
**  参数:       [in] ths_val：碰撞加速度阀值
**              [in] calib： X/Y/Z三轴标定值,单位0.001g
**  返回:       TRUE，设置成功；FALSE，设置失败
********************************************************************************/
BOOLEAN PORT_GsenSetHitPara(INT16U ths_val, AXIS_DAT_T *calib)
{
    return dal_gsen_SetHitPara(ths_val, (AXIS_DATA_T *)calib);
}
/********************************************************************************
**  函数名称:  PORT_GsenSetGyroPara
**  功能描述:  设置标定的角速度三轴偏移值
**  输入参数: [in] calib: 标定的角速度三轴偏移值
**
**  返回参数:  TRUE，设置成功；FALSE，设置失败
********************************************************************************/
BOOLEAN PORT_GsenSetGyroPara(AXIS_DAT_T *calib)
{
    return dal_gsen_SetGyroPara((AXIS_DATA_T *)calib);
}

/********************************************************************************
**  函数名:     PORT_ReadAcclData
**  函数描述:  	读取实时三轴的加速度值, 单位: 0.001g
**  参数:       [out] axis_val：X/Y/Z三轴加速度返回结构体指针
**  返回:       是否成功
********************************************************************************/
BOOLEAN PORT_ReadAcclData(AXIS_DAT_T *axis_val)
{
    return dal_gsen_ReadAcclData((AXIS_DATA_T *)axis_val);
}

/********************************************************************************
**  函数名称:  PORT_ReadGyroData
**  功能描述:  获取g-sensor三轴角速度值，单位1/1000 °/s
**  输入参数:  [in] axis_val：用于返回三轴角速度值
**  返回参数:  TRUE，获取成功；FALSE，获取失败
********************************************************************************/
BOOLEAN PORT_ReadGyroData(AXIS_DAT_T *axis_val)
{
    return dal_gsen_ReadGyroData((AXIS_DATA_T *)axis_val);
}

/********************************************************************************
**  函数名:     PORT_ReadHitAcclData
**  函数描述:   读取碰撞时三轴的加速度值, 单位: 0.001g
**  参数:       [out] axis_val： X/Y/Z三轴加速度返回结构体指针
**  返回:       是否成功
********************************************************************************/
BOOLEAN PORT_ReadHitAcclData(AXIS_DAT_T *axis_val)
{
    return dal_gsen_ReadHitAcclData((AXIS_DATA_T *)axis_val);
}

/********************************************************************************
**  函数名称:  PORT_ReadTemperData
**  功能描述:  获取温度传感器值
**  输入参数:  [in] rval：用于返回温度值，单位:1/10℃
**  返回参数:  TRUE，获取成功；FALSE，获取失败
********************************************************************************/
BOOLEAN PORT_ReadTemperData(INT16S *rval)
{
    return dal_gsen_ReadTempData(rval);
}

void PORT_Gsensor_Init(void)
{
	dal_gsen_Init();
}

//------------------------------------------------------------------------------
/* End of File */
