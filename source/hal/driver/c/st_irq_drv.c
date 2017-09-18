/******************************************************************************
**
** Filename:     st_irq_drv.c
** Copyright:    
** Description:  该模块主要实现中断配置和管理
**
*******************************************************************************
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#include "hal_include.h"
#include "st_irq_drv.h"
#include "st_irq_def.h"

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
__attribute__ ((section ("VECTOR_RAM")))  IRQ_SERVICE_FUNC g_vector_tbl[IRQ_ID_MAX] = {
	                                        (IRQ_SERVICE_FUNC)0,
                                            (IRQ_SERVICE_FUNC)0,
                                            (IRQ_SERVICE_FUNC)NMIException,
                                            (IRQ_SERVICE_FUNC)HardFaultException,
                                            (IRQ_SERVICE_FUNC)MemManageException,
                                            (IRQ_SERVICE_FUNC)BusFaultException,
                                            (IRQ_SERVICE_FUNC)UsageFaultException,
                                            (IRQ_SERVICE_FUNC)0,
                                            (IRQ_SERVICE_FUNC)0,                 
                                            (IRQ_SERVICE_FUNC)0,                
                                            (IRQ_SERVICE_FUNC)0,                 
                                            (IRQ_SERVICE_FUNC)SVCHandler,
                                            (IRQ_SERVICE_FUNC)DebugMonitor,
                                            (IRQ_SERVICE_FUNC)0,                 
                                            (IRQ_SERVICE_FUNC)PendSVC,
                                            (IRQ_SERVICE_FUNC)SysTick_Handler,
                                            
                                            (IRQ_SERVICE_FUNC)WWDG_IRQHandler,
                                            (IRQ_SERVICE_FUNC)PVD_IRQHandler,
                                            (IRQ_SERVICE_FUNC)RTC_IRQHandler,
                                            (IRQ_SERVICE_FUNC)FLASH_IRQHandler,
                                            (IRQ_SERVICE_FUNC)RCC_CRS_IRQHandler,
                                            (IRQ_SERVICE_FUNC)EXTI0_1_IRQHandler,
                                            (IRQ_SERVICE_FUNC)EXTI2_3_IRQHandler,
                                            (IRQ_SERVICE_FUNC)EXTI4_15_IRQHandler,
                                            (IRQ_SERVICE_FUNC)TSC_IRQHandler,
                                            (IRQ_SERVICE_FUNC)DMA1_Channel1_IRQHandler,
                                            (IRQ_SERVICE_FUNC)DMA1_Channel2_3_IRQHandler,
                                            (IRQ_SERVICE_FUNC)DMA1_Channel4_5_6_7_IRQHandler,
                                            (IRQ_SERVICE_FUNC)ADC1_COMP_IRQHandler,
                                            (IRQ_SERVICE_FUNC)TIM1_BRK_UP_TRG_COM_IRQHandler,
                                            (IRQ_SERVICE_FUNC)TIM1_CC_IRQHandler,
                                            (IRQ_SERVICE_FUNC)TIM2_IRQHandler,
                                            (IRQ_SERVICE_FUNC)TIM3_IRQHandler,
                                            (IRQ_SERVICE_FUNC)TIM6_DAC_IRQHandler,
                                            (IRQ_SERVICE_FUNC)TIM7_IRQHandler,
                                            (IRQ_SERVICE_FUNC)TIM14_IRQHandler,
                                            (IRQ_SERVICE_FUNC)TIM15_IRQHandler,
                                            (IRQ_SERVICE_FUNC)TIM16_IRQHandler,
                                            (IRQ_SERVICE_FUNC)TIM17_IRQHandler,
                                            (IRQ_SERVICE_FUNC)I2C1_IRQHandler,
                                            (IRQ_SERVICE_FUNC)I2C2_IRQHandler,
                                            (IRQ_SERVICE_FUNC)SPI1_IRQHandler,
                                            (IRQ_SERVICE_FUNC)SPI2_IRQHandler,
                                            (IRQ_SERVICE_FUNC)USART1_IRQHandler,
                                            (IRQ_SERVICE_FUNC)USART2_IRQHandler,
                                            (IRQ_SERVICE_FUNC)USART3_4_IRQHandler, 
                                            (IRQ_SERVICE_FUNC)CEC_CAN_IRQHandler,
                                            (IRQ_SERVICE_FUNC)USB_IRQHandler

};

/*******************************************************************
** 函数名称: ST_IRQ_InstallIrqHandler
** 函数描述: 安装中断处理函数
** 参数:     [in] irqid :  中断号,见IRQn_Type
**           [in] handle : 中断处理函数
** 返回:     无
********************************************************************/
extern INT32U __headinfo_base;
void ST_IRQ_InstallIrqHandler(INT32S irqid, IRQ_SERVICE_FUNC handle)
{
    INT32U cpu_sr;
    
    *(INT8U *)__headinfo_base = 0;
    OS_ASSERT(((irqid > -16) && (irqid + 16 < IRQ_ID_MAX)), RETURN_VOID);
    OS_ASSERT((handle != 0), RETURN_VOID);
    
    OS_ENTER_CRITICAL();
    g_vector_tbl[irqid + 16] = handle;
    OS_EXIT_CRITICAL();
}

/*******************************************************************
** 函数名称: ST_IRQ_ConfigIrqPriority
** 函数描述: 配置中断优先级
** 参数:     [in] irqid :  中断号,见IRQn_Type
**           [in] pree_pr: 主优先级
**           [in] sub_pr:  子优先级
** 返回:     无
********************************************************************/
void ST_IRQ_ConfigIrqPriority(INT32S irqid, INT32U pree_pr, INT32U sub_pr)
{
    INT32U cpu_sr;
    
    OS_ASSERT(((irqid > -16) && (irqid + 16 < IRQ_ID_MAX)), RETURN_VOID);
    OS_ASSERT((IS_NVIC_PRIORITY(sub_pr)), RETURN_VOID);
    
    OS_ENTER_CRITICAL();
	NVIC_SetPriority((IRQn_Type)irqid, sub_pr);
	OS_EXIT_CRITICAL();
}

/*******************************************************************
** 函数名称: ST_IRQ_ConfigIrqEnable
** 函数描述: 配置中断开启或关闭
** 参数:     [in] irqid :  中断号,见IRQn_Type
**           [in] enable:  TRUE:使能中断，false-关闭中断
** 返回:     无
********************************************************************/
void ST_IRQ_ConfigIrqEnable(INT32S irqid, BOOLEAN enable)
{
    INT32U cpu_sr;
    
    OS_ASSERT(((irqid >= 0) && (irqid + 16 < IRQ_ID_MAX)), RETURN_VOID);
    
    if (enable) {
        OS_ENTER_CRITICAL();
        /* Enable the Selected IRQ Channels --------------------------------------*/
        NVIC_EnableIRQ((IRQn_Type)irqid);
        OS_EXIT_CRITICAL();
    } else {
        OS_ENTER_CRITICAL();
        /* Disable the Selected IRQ Channels -------------------------------------*/
        NVIC_DisableIRQ((IRQn_Type)irqid);
        OS_EXIT_CRITICAL();
    }
}

