/******************************************************************************
**
** filename:     os_bsp.c
** copyright:    
** description:  该模块主要实现板载接口实现
**
-------------------------------------------------------------------------------
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#include "os_include.h"
#include "os_timer.h"
#include "stm32f0xx.h"
#include "stm32f0xx_conf.h"
#include "st_irq_drv.h"

/*
********************************************************************************
* define config parameters
********************************************************************************
*/

/*
********************************************************************************
* define struct
********************************************************************************
*/



/*
********************************************************************************
* define module variants
********************************************************************************
*/


/*******************************************************************
* 函数名称:  SystickIntProc
* 函数描述:  定时器中断处理函数
* 参数:      无
* 返回:      无
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void SysTickHandler(void)
{
	 g_systicks++;
}

/*******************************************************************
* 函数名称:  OS_SysTickStartup
* 函数描述:  启动硬件定时器
* 参数:      无
* 返回:      无
********************************************************************/
void OS_SysTickStartup(void)
{
    SysTick->CTRL &= ~(SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk);    /* 关闭中断和定时器 */
    ST_IRQ_ConfigIrqPriority(SysTick_IRQn, IRQ_PRIOTITY_SYSTICK);               /* 配置中断优先级 */
    ST_IRQ_InstallIrqHandler(SysTick_IRQn, (IRQ_SERVICE_FUNC)SysTickHandler);
    SysTick->LOAD  = ((SystemCoreClock / (8 * 1000)) * PERTICK)- 1;            /* set reload register */
    SysTick->VAL   = 0;                                                        /* Load the SysTick Counter Value */
    SysTick->CTRL |= (SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk);     /* Enable SysTick IRQ and SysTick Timer */
  
#if 0
	 SysTick_CounterCmd(SysTick_Counter_Disable);         /* Disable SysTick Counter    */
	 SysTick_CounterCmd(SysTick_Counter_Clear);           /* Clear SysTick Counter      */
	 SysTick_SetReload(20 * 9000);                        /* 10ms with input clock equal to 9MHz (HCLK/8, default) */
	 ST_IRQ_InstallIrqHandler(IRQ_ID_SYSTICK, (IRQ_SERVICE_FUNC)SystickIntProc, IRQ_PRIOTITY_SYSTICK, TRUE);
	 SysTick_ITConfig(ENABLE);                            /* Enable SysTick interrupt   */
	 SysTick_CounterCmd(SysTick_Counter_Enable);          /* Enable the SysTick Counter */
#endif
}

/*************END OF FILE****/

