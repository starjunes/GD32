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
                                            (IRQ_SERVICE_FUNC)TAMPER_IRQHandler,
                                            (IRQ_SERVICE_FUNC)RTC_IRQHandler,
                                            (IRQ_SERVICE_FUNC)FLASH_IRQHandler,
                                            (IRQ_SERVICE_FUNC)RCC_IRQHandler,
                                            (IRQ_SERVICE_FUNC)EXTI0_IRQHandler,
                                            (IRQ_SERVICE_FUNC)EXTI1_IRQHandler,
                                            (IRQ_SERVICE_FUNC)EXTI2_IRQHandler,
                                            (IRQ_SERVICE_FUNC)EXTI3_IRQHandler,
                                            (IRQ_SERVICE_FUNC)EXTI4_IRQHandler,
                                            (IRQ_SERVICE_FUNC)DMA1_Channel1_IRQHandler,
                                            (IRQ_SERVICE_FUNC)DMA1_Channel2_IRQHandler,
                                            (IRQ_SERVICE_FUNC)DMA1_Channel3_IRQHandler,
                                            (IRQ_SERVICE_FUNC)DMA1_Channel4_IRQHandler,
                                            (IRQ_SERVICE_FUNC)DMA1_Channel5_IRQHandler,
                                            (IRQ_SERVICE_FUNC)DMA1_Channel6_IRQHandler,
                                            (IRQ_SERVICE_FUNC)DMA1_Channel7_IRQHandler,
                                            (IRQ_SERVICE_FUNC)ADC1_2_IRQHandler,
                                            (IRQ_SERVICE_FUNC)CAN1_TX_IRQHandler,
                                            (IRQ_SERVICE_FUNC)CAN1_RX0_IRQHandler,
                                            (IRQ_SERVICE_FUNC)CAN1_RX1_IRQHandler,
                                            (IRQ_SERVICE_FUNC)CAN1_SCE_IRQHandler,
                                            (IRQ_SERVICE_FUNC)EXTI9_5_IRQHandler,
                                            (IRQ_SERVICE_FUNC)TIM1_BRK_IRQHandler,
                                            (IRQ_SERVICE_FUNC)TIM1_UP_IRQHandler,
                                            (IRQ_SERVICE_FUNC)TIM1_TRG_COM_IRQHandler,
                                            (IRQ_SERVICE_FUNC)TIM1_CC_IRQHandler,
                                            (IRQ_SERVICE_FUNC)TIM2_IRQHandler,
                                            (IRQ_SERVICE_FUNC)TIM3_IRQHandler,
                                            (IRQ_SERVICE_FUNC)TIM4_IRQHandler,
                                            (IRQ_SERVICE_FUNC)I2C1_EV_IRQHandler,
                                            (IRQ_SERVICE_FUNC)I2C1_ER_IRQHandler,
                                            (IRQ_SERVICE_FUNC)I2C2_EV_IRQHandler,
                                            (IRQ_SERVICE_FUNC)I2C2_ER_IRQHandler,
                                            (IRQ_SERVICE_FUNC)SPI1_IRQHandler,
                                            (IRQ_SERVICE_FUNC)SPI2_IRQHandler,
                                            (IRQ_SERVICE_FUNC)USART1_IRQHandler,
                                            (IRQ_SERVICE_FUNC)USART2_IRQHandler,
                                            (IRQ_SERVICE_FUNC)USART3_IRQHandler,
                                            (IRQ_SERVICE_FUNC)EXTI15_10_IRQHandler,
                                            (IRQ_SERVICE_FUNC)RTCAlarm_IRQHandler,
                                            (IRQ_SERVICE_FUNC)OTG_FS_WKUP_IRQHandler, 
                                            (IRQ_SERVICE_FUNC)0,
                                            (IRQ_SERVICE_FUNC)0,
                                            (IRQ_SERVICE_FUNC)0,
                                            (IRQ_SERVICE_FUNC)0,
                                            (IRQ_SERVICE_FUNC)0,
                                            (IRQ_SERVICE_FUNC)0,
                                            (IRQ_SERVICE_FUNC)0,
                                            (IRQ_SERVICE_FUNC)TIM5_IRQHandler,
                                            (IRQ_SERVICE_FUNC)SPI3_IRQHandler,
                                            (IRQ_SERVICE_FUNC)UART4_IRQHandler,
                                            (IRQ_SERVICE_FUNC)UART5_IRQHandler,
                                            (IRQ_SERVICE_FUNC)TIM6_IRQHandler,
                                            (IRQ_SERVICE_FUNC)TIM7_IRQHandler,
                                            (IRQ_SERVICE_FUNC)DMA2_Channel1_IRQHandler,
                                            (IRQ_SERVICE_FUNC)DMA2_Channel2_IRQHandler,
                                            (IRQ_SERVICE_FUNC)DMA2_Channel3_IRQHandler,
                                            (IRQ_SERVICE_FUNC)DMA2_Channel4_IRQHandler,
                                            (IRQ_SERVICE_FUNC)DMA2_Channel5_IRQHandler,
                                            (IRQ_SERVICE_FUNC)ETH_IRQHandler,
                                            (IRQ_SERVICE_FUNC)ETH_WKUP_IRQHandler,
                                            (IRQ_SERVICE_FUNC)CAN2_TX_IRQHandler,
                                            (IRQ_SERVICE_FUNC)CAN2_RX0_IRQHandler,
                                            (IRQ_SERVICE_FUNC)CAN2_RX1_IRQHandler,
                                            (IRQ_SERVICE_FUNC)CAN2_SCE_IRQHandler,
                                            (IRQ_SERVICE_FUNC)OTG_FS_IRQHandler


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
    
    //*(INT8U *)__headinfo_base = 0;
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
    NVIC_InitTypeDef  nvic_inittypedef;
    OS_ASSERT(((irqid > -16) && (irqid + 16 < IRQ_ID_MAX)), RETURN_VOID);
    OS_ASSERT((IS_NVIC_SUB_PRIORITY(sub_pr)), RETURN_VOID);

    nvic_inittypedef.NVIC_IRQChannel                   = irqid - 16;
    nvic_inittypedef.NVIC_IRQChannelPreemptionPriority = pree_pr;
    nvic_inittypedef.NVIC_IRQChannelSubPriority        = sub_pr;
    nvic_inittypedef.NVIC_IRQChannelCmd                = ENABLE;
    OS_ENTER_CRITICAL();
	//NVIC_SetPriority((IRQn_Type)irqid, sub_pr);
	NVIC_Init(&nvic_inittypedef);
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

