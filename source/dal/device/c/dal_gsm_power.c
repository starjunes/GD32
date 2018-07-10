/********************************************************************************
**
** 文件名:     dal_gsm_power.h
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要gsm电源控制管理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2017/11/06 | 谢宁鸿 |  创建第一版本
*********************************************************************************/

#include "yx_include.h"
#include "yx_misc.h"
#include "dal_gpio_cfg.h"
#include "dal_gsm_power.h"

/*
********************************************************************************
* 定义模块配置参数
********************************************************************************
*/
#define _POWERON             0x01

#define STEP_POWERDOWN       0
#define STEP_POWERUP         1
#define STEP_ON              2

#define PERIOD_DELAY         _MILTICK,  1
#define PERIOD_POWERDOWN     _SECOND,  3
#define PERIOD_POWERUP       _SECOND,  2
#define PERIOD_ON            _MILTICK,  25

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    INT8U status;
    INT8U step;
} DCB_T;


/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static DCB_T s_dcb;
static INT8U s_powertmr;




/*******************************************************************
** 函数名:     ScanTmrProc
** 函数描述:   定时器处理
** 参数:       [in] pdata: 定时器特征值
** 返回:       无
********************************************************************/
static void ScanTmrProc(void *pdata)
{
    switch (s_dcb.step)
    {
    case STEP_POWERDOWN:                                                       /* 断电 */
        #if DEBUG_AT > 0
        printf_com("<gsm power down>\r\n");
        #endif
        
        OS_StartTmr(s_powertmr, PERIOD_POWERDOWN);
        DAL_GPIO_PulldownGsmOnOff();
        DAL_GPIO_PulldownGsmPower();
        
        s_dcb.step = STEP_POWERUP;
        break;
    case STEP_POWERUP:                                                         /* 上电 */
        #if DEBUG_AT > 0
        printf_com("<gsm power up>\r\n");
        #endif
        
        OS_StartTmr(s_powertmr, PERIOD_POWERUP);
        DAL_GPIO_PullupGsmOnOff();
        DAL_GPIO_PullupGsmPower();
        
        s_dcb.step = STEP_ON;
        break;
    case STEP_ON:                                                              /* 开机 */
        #if DEBUG_AT > 0
        printf_com("<open gsm module>\r\n");
        #endif
        
        OS_StartTmr(s_powertmr, PERIOD_ON);
        DAL_GPIO_PulldownGsmOnOff();
        
        s_dcb.step++;
        break;
    default:
        //DAL_GPIO_PullupGsmOnOff();
        OS_StopTmr(s_powertmr);
        break;
    }
}

/*******************************************************************
** 函数名:     DAL_GSM_Init
** 函数描述:   初始化函数
** 参数:       无
** 返回:       无
********************************************************************/
void DAL_GSM_InitPower(void)
{
    YX_MEMSET(&s_dcb, 0, sizeof(s_dcb));
    
    
    s_powertmr = OS_CreateTmr(TSK_ID_DAL, 0, ScanTmrProc);
}

/*******************************************************************
** 函数名:     DAL_GSM_PowerOn
** 函数描述:   打开模块电源
** 参数:       无
** 返回:       无
********************************************************************/
void DAL_GSM_PowerOn(void)
{
    if ((s_dcb.status & _POWERON) == 0) {
        s_dcb.status = _POWERON;
        s_dcb.step   = STEP_POWERDOWN;
        
        OS_StartTmr(s_powertmr, PERIOD_DELAY);
    }
}

/*******************************************************************
** 函数名:     DAL_GSM_PowerOff
** 函数描述:   关闭模块电源
** 参数:       无
** 返回:       无
********************************************************************/
void DAL_GSM_PowerOff(void)
{
    if ((s_dcb.status & _POWERON) != 0) {
        s_dcb.status = 0;
        s_dcb.step   = STEP_POWERDOWN;
        
        DAL_GPIO_PulldownGsmPower();
        DAL_GPIO_PulldownGsmOnOff();
        OS_StopTmr(s_powertmr);
    }
}

/*******************************************************************
** 函数名:     DAL_GSM_PowerReset
** 函数描述:   复位重启模块
** 参数:       无
** 返回:       无
********************************************************************/
void DAL_GSM_PowerReset(void)
{
    if ((s_dcb.status & _POWERON) != 0) {
        s_dcb.step   = STEP_POWERDOWN;
        
        OS_StartTmr(s_powertmr, PERIOD_DELAY);
    }
}


