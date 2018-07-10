/********************************************************************************
**
** 文件名:     dal_gpio_cfg.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现I/O配置和控制
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
#include "st_adc_drv.h"
#include "dal_input_drv.h"
#include "dal_output_drv.h"
#include "dal_gpio_cfg.h"
#include "dal_gsm_power.h"
#include "st_i2c_reg.h"
#include "st_i2c_simu.h"


/*
********************************************************************************
* 定义GPIO
********************************************************************************
*/
#define PIN_WATCHDOG         GPIO_PIN_MAX        /* 看门狗清狗 */
#define PIN_MMIPOWER         GPIO_PIN_D2         /* MMI电源控制 节油产品指 MU9600C */
#define PIN_MMIRESET         GPIO_PIN_B3         /* MMI复位控制 节油产品值 MU9600C */

#define PIN_CANSLEEP         GPIO_PIN_B7         /* can休眠 */
#define PIN_CAN2SLEEP        GPIO_PIN_B4         /* can休眠 */
#define PIN_POWERSAVE        GPIO_PIN_MAX         /* 外围电路省电功能控制*/
#define PIN_CAP_SLEEP        GPIO_PIN_C6         /* 法拉电容防反灌控制脚 */
#define PIN_CAP_CHARGE       GPIO_PIN_C12         /* 法拉电容充电使能控制 */

#define PIN_GPSPOWER         GPIO_PIN_MAX         /* GPS电源控制 */
#define PIN_GPSBAT           GPIO_PIN_MAX         /* GPS备用电源控制 */
#define PIN_GSMPOWER         GPIO_PIN_MAX        /* GSM电源控制 */
#define PIN_GSMONOFF         GPIO_PIN_A8         /* GSM开关机控制 节油产品指 MU9600C */
#define PIN_PULSE1           GPIO_PIN_MAX        /* 脉冲通道控制 */
#define PIN_PULSE2           GPIO_PIN_MAX        /* 脉冲通道控制 */

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
//static INT32U s_advalue, ct_readvin, s_type;

       
/*
********************************************************************************
*  看门狗
********************************************************************************
*/

void DAL_GPIO_InitWatchdog(void)
{
    ST_GPIO_SetPin(PIN_WATCHDOG, GPIO_DIR_OUT, GPIO_MODE_PP, 0);
}

void DAL_GPIO_ClearWatchdog(void)
{
    if (ST_GPIO_ReadOutputDataBit(PIN_WATCHDOG)) {
        ST_GPIO_WritePin(PIN_WATCHDOG, false);
    } else {
        ST_GPIO_WritePin(PIN_WATCHDOG, true);
    }
}

/*******************************************************************
** 函数名:     DAL_GPIO_InitOutput
** 函数描述:   通用GPIO输出初始化
** 参数:       [in] port: 输出口编号，见OUTPUT_IO_E
** 返回:       无
********************************************************************/
void DAL_GPIO_InitOutput(INT8U port)
{
    const OUTPUT_IO_T *pinfo;
    
    if (port >= DAL_OUTPUT_GetIOMax()) {
        return;
    }
    
    pinfo = DAL_OUTPUT_GetRegInfo(port);
    ST_GPIO_SetPin(pinfo->pin, GPIO_DIR_OUT, GPIO_MODE_PP, pinfo->level);
}

/*******************************************************************
** 函数名:     DAL_GPIO_Pullup
** 函数描述:   控制拉高GPIO
** 参数:       [in] port: 输出口编号，见OUTPUT_IO_E
** 返回:       无
********************************************************************/
void DAL_GPIO_Pullup(INT8U port)
{
    const OUTPUT_IO_T *pinfo;
    
    if (port >= DAL_OUTPUT_GetIOMax()) {
        return;
    }
    
    pinfo = DAL_OUTPUT_GetRegInfo(port);
    ST_GPIO_WritePin(pinfo->pin, true);
}

/*******************************************************************
** 函数名:     DAL_GPIO_Pulldown
** 函数描述:   控制拉低GPIO
** 参数:       [in] port: 输出口编号，见OUTPUT_IO_E
** 返回:       无
********************************************************************/
void DAL_GPIO_Pulldown(INT8U port)
{
    const OUTPUT_IO_T *pinfo;
    
    if (port >= DAL_OUTPUT_GetIOMax()) {
        return;
    }
    
    pinfo = DAL_OUTPUT_GetRegInfo(port);
    ST_GPIO_WritePin(pinfo->pin, false);
}

/*******************************************************************
** 函数名:     DAL_GPIO_PullupMixLed
** 函数描述:   红绿灯混合控制,控制拉高GPIO
** 参数:       [in] port: 输出口编号，见OUTPUT_IO_E
** 返回:       无
********************************************************************/
static INT8U s_flag;
void DAL_GPIO_PullupMixLed(INT8U port)
{
    if (s_flag) {
        s_flag = 0;
        DAL_GPIO_Pulldown(OPT_LEDRED);
        DAL_GPIO_Pullup(OPT_LEDGREEN);
    } else {
        s_flag = 1;
        DAL_GPIO_Pullup(OPT_LEDRED);
        DAL_GPIO_Pulldown(OPT_LEDGREEN);
    }
}

/*******************************************************************
** 函数名:     DAL_GPIO_PulldownMixLed
** 函数描述:   红绿灯混合控制,控制拉低GPIO
** 参数:       [in] port: 输出口编号，见OUTPUT_IO_E
** 返回:       无
********************************************************************/
void DAL_GPIO_PulldownMixLed(INT8U port)
{ 
    DAL_GPIO_Pulldown(OPT_LEDRED);
    DAL_GPIO_Pulldown(OPT_LEDGREEN);
}

/*******************************************************************
** 函数名:     DAL_GPIO_PullupDvrPower
** 函数描述:   控制MMI电源
** 参数:       无
** 返回:       无
********************************************************************/
void DAL_GPIO_InitDvrPower(void)
{
    ST_GPIO_SetPin(PIN_MMIPOWER, GPIO_DIR_OUT, GPIO_MODE_PP, FALSE);
}

void DAL_GPIO_PullupDvrPower(void)
{
    DAL_GPIO_InitDvrPower();
    ST_GPIO_WritePin(PIN_MMIPOWER, TRUE);
}

void DAL_GPIO_PulldownDvrPower(void)
{
    DAL_GPIO_InitDvrPower();
    ST_GPIO_WritePin(PIN_MMIPOWER, FALSE);
}

/*******************************************************************
** 函数名:     DAL_GPIO_PullupDvrReset
** 函数描述:   控制MMI电源
** 参数:       无
** 返回:       无
********************************************************************/
void DAL_GPIO_InitDvrReset(void)
{
    ST_GPIO_SetPin(PIN_MMIRESET, GPIO_DIR_OUT, GPIO_MODE_PP, FALSE);
}

void DAL_GPIO_PullupDvrReset(void)
{
    DAL_GPIO_InitDvrReset();
    ST_GPIO_WritePin(PIN_MMIRESET, TRUE);
}

void DAL_GPIO_PulldownDvrReset(void)
{
    DAL_GPIO_InitDvrReset();
    ST_GPIO_WritePin(PIN_MMIRESET, FALSE);
}

/*******************************************************************
** 函数名:     DAL_GPIO_PullupPowerSave
** 函数描述:   控制外围电路电源
** 参数:       无
** 返回:       无
********************************************************************/
void DAL_GPIO_InitPowerSave(void)
{

    //  #define PIN_CANSLEEP         GPIO_PIN_B3         /* can休眠 */
    //  #define PIN_POWERSAVE        GPIO_PIN_C8         /* 外围电路省电功能控制*/
    //  #define PIN_CAP_SLEEP        GPIO_PIN_C6         /* 法拉电容防反灌控制脚 */

    ST_GPIO_SetPin(PIN_POWERSAVE, GPIO_DIR_OUT, GPIO_MODE_PP, FALSE);
    ST_GPIO_SetPin(PIN_CANSLEEP, GPIO_DIR_OUT, GPIO_MODE_PP, FALSE);
    ST_GPIO_SetPin(PIN_CAN2SLEEP, GPIO_DIR_OUT, GPIO_MODE_PP, FALSE);
    ST_GPIO_SetPin(PIN_CAP_SLEEP, GPIO_DIR_OUT, GPIO_MODE_PP, FALSE);
}

void DAL_GPIO_PullupPowerSave(void)
{
    DAL_GPIO_InitPowerSave();
    ST_GPIO_WritePin(PIN_POWERSAVE, TRUE);
    ST_GPIO_WritePin(PIN_CANSLEEP, TRUE);
    ST_GPIO_WritePin(PIN_CAN2SLEEP, TRUE);
    ST_GPIO_WritePin(PIN_CAP_SLEEP, TRUE);
}

void DAL_GPIO_PulldownPowerSave(void)
{
    DAL_GPIO_InitPowerSave();
    ST_GPIO_WritePin(PIN_POWERSAVE, FALSE);
    ST_GPIO_WritePin(PIN_CANSLEEP, FALSE);
    ST_GPIO_WritePin(PIN_CAN2SLEEP, FALSE);
    ST_GPIO_WritePin(PIN_CAP_SLEEP, FALSE);
}

/*******************************************************************
** 函数名:     DAL_GPIO_InitCapCharge
** 函数描述:   法拉电容充电控制
** 参数:       无
** 返回:       无
********************************************************************/
void DAL_GPIO_InitCapCharge(void)
{
    ST_GPIO_SetPin(PIN_CAP_CHARGE, GPIO_DIR_OUT, GPIO_MODE_PP, FALSE);
}

void DAL_GPIO_PullupCapCharge(void)
{
    DAL_GPIO_InitCapCharge();
    ST_GPIO_WritePin(PIN_CAP_CHARGE, TRUE);
}

void DAL_GPIO_PulldownCapCharge(void)
{
    DAL_GPIO_InitCapCharge();
    ST_GPIO_WritePin(PIN_CAP_CHARGE, FALSE);
}

/*******************************************************************
** 函数名:     DAL_GPIO_PullupGpsPower
** 函数描述:   控制GPS电源
** 参数:       无
** 返回:       无
********************************************************************/
void DAL_GPIO_InitGpsPower(void)
{
    ST_GPIO_SetPin(PIN_GPSPOWER, GPIO_DIR_OUT, GPIO_MODE_PP, FALSE);
}

void DAL_GPIO_PullupGpsPower(void)
{
    DAL_GPIO_InitGpsPower();
    ST_GPIO_WritePin(PIN_GPSPOWER, TRUE);
}

void DAL_GPIO_PulldownGpsPower(void)
{
    DAL_GPIO_InitGpsPower();
    ST_GPIO_WritePin(PIN_GPSPOWER, FALSE);
}

/*******************************************************************
** 函数名:     DAL_GPIO_PullupGpsVbat
** 函数描述:   控制GPS备用电源
** 参数:       无
** 返回:       无
********************************************************************/
void DAL_GPIO_InitGpsVbat(void)
{
    ST_GPIO_SetPin(PIN_GPSBAT, GPIO_DIR_OUT, GPIO_MODE_PP, FALSE);
}

void DAL_GPIO_PullupGpsVbat(void)
{
    DAL_GPIO_InitGpsVbat();
    ST_GPIO_WritePin(PIN_GPSBAT, TRUE);
}

void DAL_GPIO_PulldownGpsVbat(void)
{
    DAL_GPIO_InitGpsVbat();
    ST_GPIO_WritePin(PIN_GPSBAT, FALSE);
}

/*******************************************************************
** 函数名:     DAL_GPIO_PullupGsmPower
** 函数描述:   控制GSM电源
** 参数:       无
** 返回:       无
********************************************************************/
void DAL_GPIO_InitGsmPower(void)
{
    ST_GPIO_SetPin(PIN_GSMPOWER, GPIO_DIR_OUT, GPIO_MODE_PP, FALSE);
}

void DAL_GPIO_PullupGsmPower(void)
{
    DAL_GPIO_InitGsmPower();
    ST_GPIO_WritePin(PIN_GSMPOWER, TRUE);
}

void DAL_GPIO_PulldownGsmPower(void)
{
    DAL_GPIO_InitGsmPower();
    ST_GPIO_WritePin(PIN_GSMPOWER, FALSE);
}


/*******************************************************************
** 函数名:     DAL_GPIO_PullupGsmOnOff
** 函数描述:   控制GSM开关机
** 参数:       无
** 返回:       无
********************************************************************/
void DAL_GPIO_InitGsmOnOff(void)
{
    ST_GPIO_SetPin(PIN_GSMONOFF, GPIO_DIR_OUT, GPIO_MODE_PP, FALSE);
}

void DAL_GPIO_PullupGsmOnOff(void)
{
    DAL_GPIO_InitGsmOnOff();
    ST_GPIO_WritePin(PIN_GSMONOFF, TRUE);
}

void DAL_GPIO_PulldownGsmOnOff(void)
{
    DAL_GPIO_InitGsmOnOff();
    ST_GPIO_WritePin(PIN_GSMONOFF, FALSE);
}

/*******************************************************************
** 函数名:     
** 函数描述:   控制里程脉冲通道
** 参数:       无
** 返回:       无
********************************************************************/
void DAL_GPIO_InitPulseChanne(void)
{
    ST_GPIO_SetPin(PIN_PULSE1, GPIO_DIR_OUT, GPIO_MODE_PP, FALSE);
    ST_GPIO_SetPin(PIN_PULSE2, GPIO_DIR_OUT, GPIO_MODE_PP, FALSE);
}

void DAL_GPIO_PullupPulseCh1(void)
{
    ST_GPIO_WritePin(PIN_PULSE1, TRUE);
}

void DAL_GPIO_PulldownPulseCh1(void)
{
    ST_GPIO_WritePin(PIN_PULSE1, FALSE);
}

void DAL_GPIO_PullupPulseCh2(void)
{
    ST_GPIO_WritePin(PIN_PULSE2, TRUE);
}

void DAL_GPIO_PulldownPulseCh2(void)
{
    ST_GPIO_WritePin(PIN_PULSE2, FALSE);
}

/*
********************************************************************************
*  行驶记录仪里程和脉冲通道选择
PIN_LICHENG_CH  PIN_PULSE_CH           DB9的第7脚的用途
---------------------------------------------------------
    0            X(任意状态)           输入
    1            1                     里程脉冲输出
    1            0                     实时时间输出
********************************************************************************
*/

void DAL_GPIO_InitPulseChannel(void)
{
    ST_GPIO_SetPin(PIN_PULSE1, GPIO_DIR_OUT, GPIO_MODE_PP, FALSE);
    ST_GPIO_SetPin(PIN_PULSE2, GPIO_DIR_OUT, GPIO_MODE_PP, FALSE);
}

/* 选择里程脉冲从外部线束上输入 */
void DAL_GPIO_SelectOdometerPulseFromEXT(void)
{
    DAL_GPIO_InitPulseChannel();
    ST_GPIO_WritePin(PIN_PULSE1, TRUE);
    ST_GPIO_WritePin(PIN_PULSE2, FALSE);
}

/* E1选择里程脉冲从DB9上输入 */
void DAL_GPIO_SelectMeterIntoDB9(void)
{
    DAL_GPIO_InitPulseChannel();
    ST_GPIO_WritePin(PIN_PULSE1, FALSE);
    ST_GPIO_WritePin(PIN_PULSE2, TRUE);
}

/* E2选择脉冲从DB9输出 */
void DAL_GPIO_SelectPulseOutDB9(void)
{
    DAL_GPIO_InitPulseChannel();
    ST_GPIO_WritePin(PIN_PULSE1, TRUE);
    ST_GPIO_WritePin(PIN_PULSE2, TRUE);
}

/* E3选择实时时间从DB9输出 */
void DAL_GPIO_SelectTimeOutDB9(void)
{
    DAL_GPIO_InitPulseChannel();
    ST_GPIO_WritePin(PIN_PULSE1, TRUE);
    ST_GPIO_WritePin(PIN_PULSE2, FALSE);
}


//********************************************************************************-------------------------


/*******************************************************************
** 函数名:     DAL_GPIO_InitInput
** 函数描述:   通用GPIO初始化
** 参数:       [in] port: 输入口编号，见INPUT_IO_E
** 返回:       无
********************************************************************/
void DAL_GPIO_InitInput(INT8U port)
{
    const INPUT_IO_T *pinfo;
    
    if (port >= DAL_INPUT_GetIOMax()) {
        return;
    }
    pinfo = DAL_INPUT_GetCfgTblInfo(port);
    ST_GPIO_SetPin(pinfo->pin, GPIO_DIR_IN, GPIO_MODE_UP, 1);
}

/*******************************************************************
** 函数名:     DAL_GPIO_ReadPort
** 函数描述:   读取通用GPIO状态,高电平表示有效
** 参数:       [in] port: 输入口编号，见INPUT_IO_E
** 返回:       有效返回TRUE,无效返回false
********************************************************************/
BOOLEAN DAL_GPIO_ReadPort(INT8U port)
{
    const INPUT_IO_T *pinfo;
    
    if (port >= DAL_INPUT_GetIOMax()) {
        return false;
    }
    
    pinfo = DAL_INPUT_GetCfgTblInfo(port);
    return ST_GPIO_ReadPin(pinfo->pin);
}

/*******************************************************************
** 函数名:     DAL_GPIO_ReadPortN
** 函数描述:   读取通用GPIO状态,低电平表示有效
** 参数:       [in] port: 输入口编号，见INPUT_IO_E
** 返回:       有效返回TRUE,无效返回false
********************************************************************/
BOOLEAN DAL_GPIO_ReadPortN(INT8U port)
{
    const INPUT_IO_T *pinfo;
    
    if (port >= DAL_INPUT_GetIOMax()) {
        return false;
    }
    
    pinfo = DAL_INPUT_GetCfgTblInfo(port);
    return (!ST_GPIO_ReadPin(pinfo->pin));
}

/*******************************************************************
** 函数名:     DAL_GPIO_ReadPowDect
** 函数描述:   读取断电状态
** 参数:       [in] port: 输入口编号，见INPUT_IO_E
** 返回:       断电返回true，正常返回false
********************************************************************/
void DAL_GPIO_InitPowDect(INT8U port)
{
     ;
}

BOOLEAN DAL_GPIO_ReadPowDect(INT8U port)
{
    INT32S value;

    value = ST_ADC_GetValue(ADC_CH_0);
    if (value < 300) {
        return true;
    } else {
        return false;
    }
}

/*******************************************************************
** 函数名:     DAL_GPIO_ReadLowVol
** 函数描述:   读取欠压状态
** 参数:       [in] port: 输入口编号，见INPUT_IO_E
** 返回:       欠压返回true，正常返回false
********************************************************************/
void DAL_GPIO_InitLowVol(INT8U port) 
{
    ;
}

BOOLEAN DAL_GPIO_ReadLowVol(INT8U port)
{
    INT32S value;

    #if 0
    if (DAL_GPIO_ReadPowDect(port)) {                                          /* 已发生断电 */ 
        return false;
    }
    #endif

#if VERSION_HW == VERSION_HW_OLD
    value = ST_ADC_GetValue(ADC_CH_0);
    if (value > 1513) {                                                        /* 读出的电压值〉18V认为是24V系统 */
        if (value > 1929) {                                                    /* 比较基准电压为22.6V:1929,22.2V:1896,23V:1968 */
            return false;
        }
    } else if (value > 596) {                                                  /* 读出的电压值>8V，认为是12V系统 */
        if (value > 836) {                                                     /* 比较基准电压为10.6V:836,10.2V:799,11V:871 */
            return false;
        }
    }
    return true;
#else
    value = ST_ADC_GetValue(ADC_CH_0);

    if(value > 890) {
        if(value > 1014) {
            return false;
        }
    } else if(value > 480) {
        return false;
    }
    
    return true;
#endif
}

