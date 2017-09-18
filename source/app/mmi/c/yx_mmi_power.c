/********************************************************************************
**
** 文件名:     yx_mmi_power.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现外设电源管理,包括:断电、上电、复位
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2011/01/21 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "yx_include.h"
#include "yx_dym_drv.h"
#include "yx_loopbuf.h"
#include "st_uart_drv.h"
#include "dal_gpio_cfg.h"
#include "yx_debug.h"

#if EN_MMI > 0
#include "yx_mmi_drv.h"


/*
********************************************************************************
* 宏定义
********************************************************************************
*/
#define _POWERON             0x01

#define STEP_POWERDOWN       0
#define STEP_POWERUP         1
#define STEP_ON              2

#define PERIOD_DELAY         _MILTICK,  1
#define PERIOD_POWERDOWN     _SECOND,  3
#define PERIOD_POWERUP       _SECOND,  2
#define PERIOD_ON            _MILTICK,  3

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
static INT8U s_powertmr;
static DCB_T s_dcb;

/*******************************************************************
** 函数名:     PowerTmrProc
** 函数描述:   延时上电定时器
** 参数:       [in] pdata：定时器特征值
** 返回:       无
********************************************************************/
static void PowerTmrProc(void *pdata)
{
    switch (s_dcb.step)
    {
    case STEP_POWERDOWN:                                                       /* 断电 */
        #if DEBUG_MMI > 0
        printf_com("<3515 power down>\r\n");
        #endif
        
        YX_MMI_CloseCom();                                                     /* 关闭串口 */
        DAL_GPIO_PulldownDvrPower();                                           /* 关闭电源 */
        DAL_GPIO_PulldownDvrReset();                                           /* 拉低复位脚 */
        
        s_dcb.step = STEP_POWERUP;
        OS_StartTmr(s_powertmr, PERIOD_POWERDOWN);
        break;
    case STEP_POWERUP:                                                         /* 上电 */
        #if DEBUG_MMI > 0
        printf_com("<3515 power up>\r\n");
        #endif
        
        DAL_GPIO_PullupDvrPower();
        DAL_GPIO_PulldownDvrReset();
        YX_MMI_CfgBaud(115200, UART_DATABIT_8, UART_STOPBIT_1, UART_PARITY_NONE);
        
        s_dcb.step = STEP_ON;
        OS_StartTmr(s_powertmr, PERIOD_ON);
        break;
    case STEP_ON:                                                              /* 开机 */
        #if DEBUG_MMI > 0
        printf_com("<open 3515 module>\r\n");
        #endif
        
        OS_StopTmr(s_powertmr);
        DAL_GPIO_PullupDvrReset();
        
        s_dcb.step++;
        break;
    default:
        OS_StopTmr(s_powertmr);
        break;
    }
}

/*******************************************************************
** 函数名:     YX_MMI_InitPower
** 函数描述:   DVR通信通道初始化
** 参数:       无
** 返回:       无
********************************************************************/
void YX_MMI_InitPower(void)
{
    YX_MEMSET(&s_dcb, 0, sizeof(s_dcb));
    
    DAL_GPIO_InitDvrPower();
    DAL_GPIO_InitDvrReset();
    DAL_GPIO_InitPowerSave();
    
    DAL_GPIO_PulldownDvrPower();
    DAL_GPIO_PulldownDvrReset();
    DAL_GPIO_PulldownPowerSave();
    //DAL_GPIO_PullupPowerSave();
    
    s_powertmr = OS_CreateTmr(TSK_ID_APP, (void *)0, PowerTmrProc);
}

/*******************************************************************
** 函数名:     YX_MMI_PowerOn
** 函数描述:   打开电源
** 参数:       无
** 返回:       无
********************************************************************/
void YX_MMI_PowerOn(void)
{
    if ((s_dcb.status & _POWERON) == 0) {
        s_dcb.status = _POWERON;
        s_dcb.step   = STEP_POWERDOWN;
        
        OS_StartTmr(s_powertmr, PERIOD_DELAY);
    }
}

/*******************************************************************
** 函数名:     YX_MMI_PowerOff
** 函数描述:   关闭电源
** 参数:       无
** 返回:       无
********************************************************************/
void YX_MMI_PowerOff(void)
{
    if ((s_dcb.status & _POWERON) != 0) {
        s_dcb.status = 0;
        s_dcb.step   = STEP_POWERDOWN;
        
        YX_MMI_CloseCom();                                                     /* 关闭串口 */
        DAL_GPIO_PulldownDvrPower();                                           /* 关闭电源 */
        DAL_GPIO_PulldownDvrReset();                                           /* 拉低复位脚 */
        OS_StopTmr(s_powertmr);
    }
}

/*******************************************************************
** 函数名:     YX_MMI_PowerReset
** 函数描述:   复位重启
** 参数:       无
** 返回:       无
********************************************************************/
void YX_MMI_PowerReset(void)
{
    if ((s_dcb.status & _POWERON) != 0) {
        s_dcb.step   = STEP_POWERDOWN;
        
        OS_StartTmr(s_powertmr, PERIOD_DELAY);
    }
}

/*******************************************************************
** 函数名:     YX_MMI_PowerIsOn
** 函数描述:   是否已开启电源
** 参数:       无
** 返回:       是则返回TRUE, 否则返回FALSE
********************************************************************/
BOOLEAN YX_MMI_PowerIsOn(void)
{
    if ((s_dcb.status & _POWERON) != 0) {
        if (s_dcb.step >= STEP_ON) {
            return true;
        }
    }
    return false;
}


#endif


