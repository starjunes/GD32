/******************************************************************************
**
** Filename:     st_irq_def.c
** Copyright:    
** Description:  该模块主要实现默认的中断处理函数
**
*******************************************************************************
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#include "hal_include.h"
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

/*******************************************************************
* Function Name  : NMIException
* Description    : This function handles NMI exception.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void NMIException(void)
{
}

/*******************************************************************
* Function Name  : HardFaultException
* Description    : This function handles Hard Fault exception.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void HardFaultException(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/*******************************************************************
* Function Name  : MemManageException
* Description    : This function handles Memory Manage exception.
* Input          : None
* Output         : None
* Return         : None                                                                                                                                                                                          
*********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void MemManageException(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/*******************************************************************
* Function Name  : BusFaultException
* Description    : This function handles Bus Fault exception.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void BusFaultException(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/*******************************************************************
* Function Name  : UsageFaultException
* Description    : This function handles Usage Fault exception.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void UsageFaultException(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/*******************************************************************
* Function Name  : DebugMonitor
* Description    : This function handles Debug Monitor exception.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void DebugMonitor(void)
{
}

/*******************************************************************
* Function Name  : SVCHandler
* Description    : This function handles SVCall exception.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void SVCHandler(void)
{
}

/*******************************************************************
* Function Name  : PendSVC
* Description    : This function handles PendSVC exception.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void PendSVC(void)
{
}

/*******************************************************************
* Function Name  : SysTick_Handler
* Description    : This function handles SysTick Handler.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void SysTick_Handler(void)
{
}

/*******************************************************************
* Function Name  : WWDG_IRQHandler
* Description    : This function handles WWDG interrupt request.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void WWDG_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : PVD_IRQHandler
* Description    : This function handles PVD interrupt request.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void PVD_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : TAMPER_IRQHandler
* Description    : This function handles Tamper interrupt request. 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void TAMPER_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : RTC_IRQHandler
* Description    : This function handles RTC global interrupt request.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void RTC_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : FLASH_IRQHandler
* Description    : This function handles Flash interrupt request.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void FLASH_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : RCC_CRS_IRQHandler
* Description    : This function handles RCC interrupt request. 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void RCC_IRQHandler(void)
{
}


/*******************************************************************
* Function Name  : EXTI0_IRQHandler
* Description    : This function handles EXTI0 interrupt request. 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void EXTI0_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : EXTI1_IRQHandler
* Description    : This function handles EXTI1 interrupt request. 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void EXTI1_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : EXTI2_IRQHandler
* Description    : This function handles EXTI2 interrupt request. 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void EXTI2_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : EXTI3_IRQHandler
* Description    : This function handles EXTI3 interrupt request. 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void EXTI3_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : EXTI4_IRQHandler
* Description    : This function handles EXTI4 interrupt request. 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void EXTI4_IRQHandler(void)
{
}



#if 0

/*******************************************************************
* Function Name  : EXTI0_1_IRQHandler
* Description    : This function handles External interrupt Line 0 request.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void EXTI0_1_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : EXTI2_3_IRQHandler
* Description    : This function handles External interrupt Line 1 request.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void EXTI2_3_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : EXTI2_IRQHandler
* Description    : This function handles External interrupt Line 2 request.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void EXTI4_15_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : TSC_IRQHandler
* Description    : This function handles touch sensor.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void TSC_IRQHandler(void)
{
}
#endif

/*******************************************************************
* Function Name  : DMA1_Channel1_IRQHandler
* Description    : This function handles DMA1 Channel 1 interrupt request.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void DMA1_Channel1_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : DMA1_Channel2_IRQHandler
* Description    : This function handles DMA1 Channel 2 interrupt request.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void DMA1_Channel2_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : DMA1_Channel3_IRQHandler
* Description    : This function handles DMA1 Channel 3 interrupt request.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void DMA1_Channel3_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : DMA1_Channel4_IRQHandler
* Description    : This function handles DMA1 Channel 4 interrupt request.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void DMA1_Channel4_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : DMA1_Channel5_IRQHandler
* Description    : This function handles DMA1 Channel 5 interrupt request.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void DMA1_Channel5_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : DMA1_Channel6_IRQHandler
* Description    : This function handles DMA1 Channel 6 interrupt request.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void DMA1_Channel6_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : DMA1_Channel7_IRQHandler
* Description    : This function handles DMA1 Channel 7 interrupt request.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void DMA1_Channel7_IRQHandler(void)
{
}


#if 0

/*******************************************************************
* Function Name  : DMA1_Channel2_3_IRQHandler
* Description    : This function handles DMA1 Channel 2 interrupt request.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void DMA1_Channel2_3_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : DMA1_Channel4_5_6_7_IRQHandler
* Description    : This function handles DMA1 Channel 3 interrupt request.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void DMA1_Channel4_5_6_7_IRQHandler(void)
{
}

#endif

/*******************************************************************
* Function Name  : ADC1_COMP_IRQHandler
* Description    : This function handles ADC1, COMP1 and COMP2 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void ADC1_2_IRQHandler(void)
{
}


/*******************************************************************
* Function Name  : CAN1_TX_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void CAN1_TX_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : CAN1_RX0_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void CAN1_RX0_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : CAN1_RX1_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void CAN1_RX1_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : CAN1_SCE_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void CAN1_SCE_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : EXTI9_5_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void EXTI9_5_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : TIM1_BRK_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void TIM1_BRK_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : TIM1_UP_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void TIM1_UP_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : TIM1_TRG_COM_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void TIM1_TRG_COM_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : TIM1_CC_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void TIM1_CC_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : TIM2_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void TIM2_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : TIM3_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void TIM3_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : TIM4_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void TIM4_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : I2C1_EV_IRQHandler
* Description    :
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void I2C1_EV_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : I2C1_ER_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void I2C1_ER_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : I2C2_EV_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void I2C2_EV_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : I2C2_ER_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void I2C2_ER_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : SPI1_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void SPI1_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : SPI2_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void SPI2_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : USART1_IRQHandler
* Description    :
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void USART1_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : USART2_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void USART2_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : USART3_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void USART3_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : EXTI15_10_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void EXTI15_10_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : RTCAlarm_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void RTCAlarm_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : OTG_FS_WKUP_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void OTG_FS_WKUP_IRQHandler(void)
{
} 


/*******************************************************************
* Function Name  : TIM5_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void TIM5_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : SPI3_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void SPI3_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : UART4_IRQHandler
* Description    :
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void UART4_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : UART5_IRQHandler
* Description    :
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void UART5_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : TIM6_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void TIM6_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : TIM7_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void TIM7_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : DMA2_Channel1_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void DMA2_Channel1_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : DMA2_Channel2_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void DMA2_Channel2_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : DMA2_Channel3_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void DMA2_Channel3_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : DMA2_Channel4_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void DMA2_Channel4_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : DMA2_Channel5_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void DMA2_Channel5_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : ETH_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void ETH_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : ETH_WKUP_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void ETH_WKUP_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : CAN2_TX_IRQHandler
* Description    :
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void CAN2_TX_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : CAN2_RX0_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void CAN2_RX0_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : CAN2_RX1_IRQHandler
* Description    :
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void CAN2_RX1_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : CAN2_SCE_IRQHandler
* Description    : 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void CAN2_SCE_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : OTG_FS_IRQHandler
* Description    :
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void OTG_FS_IRQHandler(void)
{
}


/******************* (C) COPYRIGHT 2009 XIAMEN YAXON.LTD *********END OF FILE******/
