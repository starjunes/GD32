/******************************************************************************
**
** Filename:     hal_wwdg_drv.c
** Copyright:    
** Description:  该模块主要实现看门狗的驱动管理
**
*******************************************************************************
**             Revision history
**-----------------------------------------------------------------------------
**| 2017/06/12 | nhong |  创建文件
*******************************************************************************/
#include "yx_include.h"
#include "yx_dym_drv.h"
#include "yx_loopbuf.h"
#include "hal_wdg_drv.h"


/*******************************************************************
** 函数名称: stm32_wdg_init
** 函数描述: stm32看门狗初始化
** 参数:     void
** 返回:     无
********************************************************************/
static BOOLEAN stm32_wdg_init(void)
{
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);                    /* Enable write access to registers */
    IWDG_SetPrescaler(IWDG_Prescaler_32);                            /* overtime is 2s*/
    IWDG_SetReload(0x9c4);
    IWDG_ReloadCounter();                                            /* Reload IWDG counter */
    IWDG_Enable();                                                   /* Enable IWDG */
    return TRUE;
}


/*******************************************************************
** 函数名称: HAL_WDG_InitDrv
** 函数描述: 初始化驱动
** 参数:     无
** 返回:     无
********************************************************************/
void HAL_WDG_InitDrv(void)
{
    stm32_wdg_init();

}

/*******************************************************************
** 函数名称: HAL_WDG_ClearWdg
** 函数描述: 清看门狗
** 参数:     无
** 返回:     无
********************************************************************/
void HAL_WDG_ClearWdg(void)
{
    IWDG_ReloadCounter();
}



