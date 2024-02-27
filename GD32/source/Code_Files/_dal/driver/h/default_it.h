/**************************************************************************************************
**                                                                                               **
**  文件名称:  Default_IT.H                                                                      **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2010                                    **
**  创建信息:  LEON -- 2010年12月1日                                                             **
**  文件描述:  系统原始中断函数集                                                                **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/
#ifndef __DEFAULT_IT_H
#define __DEFAULT_IT_H

#include "dal_include.h"

void NMI_Handler(void);
void HardFault_Handler(void);
void hard_fault_handler_c(unsigned int * hardfault_args);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);

#if defined(GD32F30X_LD)

void WWDG_IRQHandler(void);
void LVD_IRQHandler(void);
void TAMPER_IRQHandler(void);
void RTC_IRQHandler(void);
void FMC_IRQHandler(void);
void RCU_IRQHandler(void);
void EXTI0_IRQHandler(void);
void EXTI1_IRQHandler(void);
void EXTI2_IRQHandler(void);
void EXTI3_IRQHandler(void);
void EXTI4_IRQHandler(void);
void DMA0_Channel0_IRQHandler(void);
void DMA0_Channel1_IRQHandler(void);
void DMA0_Channel2_IRQHandler(void);
void DMA0_Channel3_IRQHandler(void);
void DMA0_Channel4_IRQHandler(void);
void DMA0_Channel5_IRQHandler(void);
void DMA0_Channel6_IRQHandler(void);
void ADC0_1_IRQHandler(void);
void USB_HP_CAN1_TX_IRQHandler(void);
void USB_LP_CAN1_RX0_IRQHandler(void);
void CAN0_RX1_IRQHandler(void);
void CAN0_EWMC_IRQHandler(void);
void EXTI5_9_IRQHandler(void);
void TIMER0_BRK_IRQHandler(void);
void TIMER0_UP_IRQHandler(void);
void TIMER0_TRG_CMT_IRQHandler(void);
void TIMER0_Channel_IRQHandler(void);
void TIMER1_IRQHandler(void);
void TIMER2_IRQHandler(void);
void I2C0_EV_IRQHandler(void);
void I2C0_ER_IRQHandler(void);
void SPI0_IRQHandler(void);
void USART0_IRQHandler(void);
void USART1_IRQHandler(void);
void EXTI10_15_IRQHandler(void);
void RTC_Alarm_IRQHandler(void);
void USBWakeUp_IRQHandler(void);

#elif defined(GD32F30X_MD)

void WWDG_IRQHandler(void);
void LVD_IRQHandler(void);
void TAMPER_IRQHandler(void);
void RTC_IRQHandler(void);
void FMC_IRQHandler(void);
void RCU_IRQHandler(void);
void EXTI0_IRQHandler(void);
void EXTI1_IRQHandler(void);
void EXTI2_IRQHandler(void);
void EXTI3_IRQHandler(void);
void EXTI4_IRQHandler(void);
void DMA0_Channel0_IRQHandler(void);
void DMA0_Channel1_IRQHandler(void);
void DMA0_Channel2_IRQHandler(void);
void DMA0_Channel3_IRQHandler(void);
void DMA0_Channel4_IRQHandler(void);
void DMA0_Channel5_IRQHandler(void);
void DMA0_Channel6_IRQHandler(void);
void ADC0_1_IRQHandler(void);
void USB_HP_CAN1_TX_IRQHandler(void);
void USB_LP_CAN1_RX0_IRQHandler(void);
void CAN0_RX1_IRQHandler(void);
void CAN0_EWMC_IRQHandler(void);
void EXTI5_9_IRQHandler(void);
void TIMER0_BRK_IRQHandler(void);
void TIMER0_UP_IRQHandler(void);
void TIMER0_TRG_CMT_IRQHandler(void);
void TIMER0_Channel_IRQHandler(void);
void TIMER1_IRQHandler(void);
void TIMER2_IRQHandler(void);
void TIMER3_IRQHandler(void);
void I2C0_EV_IRQHandler(void);
void I2C0_ER_IRQHandler(void);
void I2C1_EV_IRQHandler(void);
void I2C1_ER_IRQHandler(void);
void SPI0_IRQHandler(void);
void SPI1_IRQHandler(void);
void USART0_IRQHandler(void);
void USART1_IRQHandler(void);
void USART2_IRQHandler(void);
void EXTI10_15_IRQHandler(void);
void RTC_Alarm_IRQHandler(void);
void USBWakeUp_IRQHandler(void);

#elif defined(GD32F30X_HD)

void WWDG_IRQHandler(void);
void LVD_IRQHandler(void);
void TAMPER_IRQHandler(void);
void RTC_IRQHandler(void);
void FMC_IRQHandler(void);
void RCU_IRQHandler(void);
void EXTI0_IRQHandler(void);
void EXTI1_IRQHandler(void);
void EXTI2_IRQHandler(void);
void EXTI3_IRQHandler(void);
void EXTI4_IRQHandler(void);
void DMA0_Channel0_IRQHandler(void);
void DMA0_Channel1_IRQHandler(void);
void DMA0_Channel2_IRQHandler(void);
void DMA0_Channel3_IRQHandler(void);
void DMA0_Channel4_IRQHandler(void);
void DMA0_Channel5_IRQHandler(void);
void DMA0_Channel6_IRQHandler(void);
void ADC0_1_IRQHandler(void);
void USB_HP_CAN1_TX_IRQHandler(void);
void USB_LP_CAN1_RX0_IRQHandler(void);
void CAN0_RX1_IRQHandler(void);
void CAN0_EWMC_IRQHandler(void);
void EXTI5_9_IRQHandler(void);
void TIMER0_BRK_IRQHandler(void);
void TIMER0_UP_IRQHandler(void);
void TIMER0_TRG_CMT_IRQHandler(void);
void TIMER0_Channel_IRQHandler(void);
void TIMER1_IRQHandler(void);
void TIMER2_IRQHandler(void);
void TIMER3_IRQHandler(void);
void I2C0_EV_IRQHandler(void);
void I2C0_ER_IRQHandler(void);
void I2C1_EV_IRQHandler(void);
void I2C1_ER_IRQHandler(void);
void SPI0_IRQHandler(void);
void SPI1_IRQHandler(void);
void USART0_IRQHandler(void);
void USART1_IRQHandler(void);
void USART2_IRQHandler(void);
void EXTI10_15_IRQHandler(void);
void RTC_Alarm_IRQHandler(void);
void USBWakeUp_IRQHandler(void);
void TIM8_BRK_IRQHandler(void);
void TIM8_UP_IRQHandler(void);
void TIM8_TRG_COM_IRQHandler(void);
void TIM8_CC_IRQHandler(void);
void ADC3_IRQHandler(void);
void FSMC_IRQHandler(void);
void SDIO_IRQHandler(void);
void TIMER4_IRQHandler(void);
void SPI2_IRQHandler(void);
void UART3_IRQHandler(void);
void UART4_IRQHandler(void);
void TIMER5_IRQHandler(void);
void TIMER6_IRQHandler(void);
void DMA1_Channel0_IRQHandler(void);
void DMA1_Channel1_IRQHandler(void);
void DMA1_Channel2_IRQHandler(void);
void DMA2_Channel4_5_IRQHandler(void);

#elif defined(GD32F30X_CL)

void WWDG_IRQHandler(void);
void LVD_IRQHandler(void);
void TAMPER_IRQHandler(void);
void RTC_IRQHandler(void);
void FMC_IRQHandler(void);
void RCU_IRQHandler(void);
void EXTI0_IRQHandler(void);
void EXTI1_IRQHandler(void);
void EXTI2_IRQHandler(void);
void EXTI3_IRQHandler(void);
void EXTI4_IRQHandler(void);
void DMA0_Channel0_IRQHandler(void);
void DMA0_Channel1_IRQHandler(void);
void DMA0_Channel2_IRQHandler(void);
void DMA0_Channel3_IRQHandler(void);
void DMA0_Channel4_IRQHandler(void);
void DMA0_Channel5_IRQHandler(void);
void DMA0_Channel6_IRQHandler(void);
void ADC0_1_IRQHandler(void);
void CAN0_TX_IRQHandler(void);
void CAN0_RX0_IRQHandler(void);
void CAN0_RX1_IRQHandler(void);
void CAN0_EWMC_IRQHandler(void);
void EXTI5_9_IRQHandler(void);
void TIMER0_BRK_IRQHandler(void);
void TIMER0_UP_IRQHandler(void);
void TIMER0_TRG_CMT_IRQHandler(void);
void TIMER0_Channel_IRQHandler(void);
void TIMER1_IRQHandler(void);
void TIMER2_IRQHandler(void);
void TIMER3_IRQHandler(void);
void I2C0_EV_IRQHandler(void);
void I2C0_ER_IRQHandler(void);
void I2C1_EV_IRQHandler(void);
void I2C1_ER_IRQHandler(void);
void SPI0_IRQHandler(void);
void SPI1_IRQHandler(void);
void USART0_IRQHandler(void);
void USART1_IRQHandler(void);
void USART2_IRQHandler(void);
void EXTI10_15_IRQHandler(void);
void RTC_Alarm_IRQHandler(void);
void USBFS_WKUP_IRQHandler(void);
void TIMER4_IRQHandler(void);
void SPI2_IRQHandler(void);
void UART3_IRQHandler(void);
void UART4_IRQHandler(void);
void TIMER5_IRQHandler(void);
void TIMER6_IRQHandler(void);
void DMA1_Channel0_IRQHandler(void);
void DMA1_Channel1_IRQHandler(void);
void DMA1_Channel2_IRQHandler(void);
void DMA1_Channel3_IRQHandler(void);
void DMA1_Channel4_IRQHandler(void);
void ENET_IRQHandler(void);
void ENET_WKUP_IRQHandler(void);
void CAN1_TX_IRQHandler(void);
void CAN1_RX0_IRQHandler(void);
void CAN1_RX1_IRQHandler(void);
void CAN1_EWMC_IRQHandler(void);
void USBFS_IRQHandler(void);

#endif

#endif

