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
__attribute__ ((section ("IRQ_HANDLE"))) void RCC_CRS_IRQHandler(void)
{
}

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

/*******************************************************************
* Function Name  : ADC1_COMP_IRQHandler
* Description    : This function handles ADC1, COMP1 and COMP2 
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void ADC1_COMP_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : TIM1_BRK_UP_TRG_COM_IRQHandler
* Description    : This function handles TIM1 Break, Update, Trigger and Commutation
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void TIM1_BRK_UP_TRG_COM_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : TIM1_CC_IRQHandler
* Description    : This function handles TIM1 capture compare interrupt request.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void TIM1_CC_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : TIM2_IRQHandler
* Description    : This function handles TIM2 global interrupt request.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void TIM2_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : TIM3_IRQHandler
* Description    : This function handles TIM3 global interrupt request.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void TIM3_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : TIM6_DAC_IRQHandler
* Description    : This function handles TIM6 and DAC global interrupt request.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void TIM6_DAC_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : TIM7_IRQHandler
* Description    : This function handles TIM6 and DAC global interrupt request.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void TIM7_IRQHandler(void)
{
}


/*******************************************************************
* Function Name  : TIM14_IRQHandler
* Description    : This function handles TIM14 Event interrupt request.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void TIM14_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : TIM15_IRQHandler
* Description    : This function handles TIM15 Error interrupt request.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void TIM15_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : TIM16_IRQHandler
* Description    : This function handles TIM16 Event interrupt request.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void TIM16_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : TIM17_IRQHandler
* Description    : This function handles TIM17 Event interrupt request.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void TIM17_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : I2C1_IRQHandler
* Description    : This function handles I2C1 global interrupt request.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void I2C1_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : I2C2_IRQHandler
* Description    : This function handles I2C2 global interrupt request.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void I2C2_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : SPI1_IRQHandler
* Description    : This function handles SPI1 global interrupt request.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void SPI1_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : SPI2_IRQHandler
* Description    : This function handles SPI2 global interrupt request.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void SPI2_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : USART1_IRQHandler
* Description    : This function handles USART1 global interrupt request.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void USART1_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : USART2_IRQHandler
* Description    : This function handles USART2 global interrupt request.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void USART2_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : USART3_4_IRQHandler
* Description    : This function handles USART2 global interrupt request.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void USART3_4_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : CEC_IRQHandler
* Description    : This function handles CEC global interrupt request.
*                  requests.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void CEC_CAN_IRQHandler(void)
{
}

/*******************************************************************
* Function Name  : USB_IRQHandler
* Description    : This function handles USB global interrupt request.
*                  requests.
* Input          : None
* Output         : None
* Return         : None
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void USB_IRQHandler(void)
{
}




/******************* (C) COPYRIGHT 2009 XIAMEN YAXON.LTD *********END OF FILE******/
