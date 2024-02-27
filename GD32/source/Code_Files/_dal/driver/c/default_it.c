/**************************************************************************************************
**                                                                                               **
**  文件名称:  Default_IT.C                                                                      **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2010                                    **
**  创建信息:  LEON -- 2010年12月1日                                                             **
**  文件描述:  系统原始中断函数集                                                                **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/
#include "default_it.h"
#include "Debug_Print.H"
//#include "usbh_int.h"
//#include "usb_delay.h"
#include "Dal_Include.H"
#include "Debug_Print.H"

//extern usb_core_handle_struct usb_core_dev;

/*************************************************************************************************/
/*                           以下为默认系统中断处理函数                                          */
/*************************************************************************************************/
void NMI_Handler(void)
{
    while(1)
    {

	}
}

/*void HardFault_Handler(void)
{
#if EN_DEBUG > 0
    Debug_SysPrint("--------<HardFault_Handler>--------\r\n");
#endif    
    while (1)
    {
    	//debug_printf("--------<HardFault_Handler>--------\r\n");
    }
}*/
void hard_fault_handler_c(unsigned int * hardfault_args)
{
#if EN_DEBUG > 0
    static unsigned int stacked_r0;
    static unsigned int stacked_r1;
    static unsigned int stacked_r2;
    static unsigned int stacked_r3;
    static unsigned int stacked_r12;
    static unsigned int stacked_lr;
    static unsigned int stacked_pc;
    static unsigned int stacked_psr;
    static unsigned int SHCSR;
    static unsigned char MFSR;
    static unsigned char BFSR; 
    static unsigned short int UFSR;
    static unsigned int HFSR;
    static unsigned int DFSR;
    static unsigned int MMAR;
    static unsigned int BFAR;
    stacked_r0 = ((unsigned long) hardfault_args[0]);
    stacked_r1 = ((unsigned long) hardfault_args[1]);
    stacked_r2 = ((unsigned long) hardfault_args[2]);
    stacked_r3 = ((unsigned long) hardfault_args[3]);
    stacked_r12 = ((unsigned long) hardfault_args[4]);
    /*异常中断发生时，这个异常模式特定的物理 R14,即 lr 被设置成该异常模式将要返回的
    地址*/
    stacked_lr = ((unsigned long) hardfault_args[5]);
    stacked_pc = ((unsigned long) hardfault_args[6]);
    stacked_psr = ((unsigned long) hardfault_args[7]);
    SHCSR = (*((volatile unsigned long *)(0xE000ED24))); //系统 Handler 控制及状态寄存器
    MFSR = (*((volatile unsigned char *)(0xE000ED28))); //存储器管理 fault 状态寄存器
    BFSR = (*((volatile unsigned char *)(0xE000ED29)));  //总线 fault 状态寄存器 
    UFSR = (*((volatile unsigned short int *)(0xE000ED2A)));//用法 fault 状态寄存器 
    HFSR = (*((volatile unsigned long *)(0xE000ED2C))); //硬 fault 状态寄存器
    DFSR = (*((volatile unsigned long *)(0xE000ED30)));  //调试 fault 状态寄存器
    MMAR = (*((volatile unsigned long *)(0xE000ED34))); //存储管理地址寄存器
    BFAR = (*((volatile unsigned long *)(0xE000ED38))); //总线 fault 地址寄存器

    Debug_SysPrint("--------<hard_fault_handler_cstacked_lr = %d>--------\r\n",stacked_lr);
    Debug_SysPrint("--------<hard_fault_handler_cSHCSR = %d>--------\r\n",SHCSR);
    Debug_SysPrint("--------<hard_fault_handler_cMFSR = %d>--------\r\n",MFSR);
    Debug_SysPrint("--------<hard_fault_handler_cBFSR = %d>--------\r\n",BFSR);
    Debug_SysPrint("--------<hard_fault_handler_cUFSR = %d>--------\r\n",UFSR);
    Debug_SysPrint("--------<hard_fault_handler_cHFSR = %d>--------\r\n",HFSR);
    Debug_SysPrint("--------<hard_fault_handler_cDFSR = %d>--------\r\n",DFSR);
    Debug_SysPrint("--------<hard_fault_handler_cMMAR = %d>--------\r\n",MMAR);
    Debug_SysPrint("--------<hard_fault_handler_cBFAR = %d>--------\r\n",BFAR);

#endif        
//while (1);
}

void MemManage_Handler(void)
{
#if EN_DEBUG > 0
        debug_printf("--------<MemManage_Handler>--------\r\n");
#endif    
    while (1)
    {
    }
}

void BusFault_Handler(void)
{
#if EN_DEBUG > 0
        debug_printf("--------<BusFault_Handler>--------\r\n");
#endif    
    while (1)
    {
    }
}

void UsageFault_Handler(void)
{
#if EN_DEBUG > 0
        debug_printf("--------<UsageFault_Handler>--------\r\n");
#endif    
    while (1)
    {
    }
}

void SVC_Handler(void)
{

    //debug_printf("--------<SVC_Handler>--------\r\n");

    while (1)
    {
    }
}

void DebugMon_Handler(void)
{

    //debug_printf("--------<DebugMon_Handler>--------\r\n");
  
    while (1)
    {
    }
}

void PendSV_Handler(void)
{
    //debug_printf("--------<PendSV_Handler>--------\r\n");
   
    while (1)
    {
    }
}

/*void SysTick_Handler(void)
{
    ;
}*/

#if defined(GD32F30X_LD)
/*************************************************************************************************/
/*                           以下为默认用户中断处理函数                                          */
/*************************************************************************************************/
void WWDG_IRQHandler(void)
{
    ;                                                                // reserved...
}

void LVD_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TAMPER_IRQHandler(void)
{
    ;                                                                // reserved...
}

void RTC_IRQHandler(void)
{
    ;                                                                // reserved...
}

void FMC_IRQHandler(void)
{
    ;                                                                // reserved...
}

void RCU_IRQHandler(void)
{
    ;                                                                // reserved...
}

void EXTI0_IRQHandler(void)
{
    ;                                                                // reserved...
}

void EXTI1_IRQHandler(void)
{
    ;                                                                // reserved...
}

void EXTI2_IRQHandler(void)
{
    ;                                                                // reserved...
}

void EXTI3_IRQHandler(void)
{
    ;                                                                // reserved...
}

void EXTI4_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA0_Channel0_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA0_Channel1_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA0_Channel2_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA0_Channel3_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA0_Channel4_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA0_Channel5_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA0_Channel6_IRQHandler(void)
{
    ;                                                                // reserved...
}

void ADC0_1_IRQHandler(void)
{
    ;                                                                // reserved...
}

void USB_HP_CAN1_TX_IRQHandler(void)
{
    ;                                                                // reserved...
}

void USB_LP_CAN1_RX0_IRQHandler(void)
{
    ;                                                                // reserved...
}

void CAN0_RX1_IRQHandler(void)
{
    ;                                                                // reserved...
}

void CAN0_EWMC_IRQHandler(void)
{
    ;                                                                // reserved...
}

void EXTI5_9_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIMER0_BRK_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIMER0_UP_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIMER0_TRG_CMT_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIMER0_Channel_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIMER1_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIMER2_IRQHandler(void)
{
    ;                                                                // reserved...
}

void I2C0_EV_IRQHandler(void)
{
    ;                                                                // reserved...
}

void I2C0_ER_IRQHandler(void)
{
    ;                                                                // reserved...
}

void SPI0_IRQHandler(void)
{
    ;                                                                // reserved...
}

void USART0_IRQHandler(void)
{
    ;                                                                // reserved...
}

void USART1_IRQHandler(void)
{
    ;                                                                // reserved...
}

void EXTI10_15_IRQHandler(void)
{
    ;                                                                // reserved...
}

void RTC_Alarm_IRQHandler(void)
{
    ;                                                                // reserved...
}

void USBWakeUp_IRQHandler(void)
{
    ;                                                                // reserved...
}

#elif defined(GD32F30X_MD)
/*************************************************************************************************/
/*                           以下为默认用户中断处理函数                                          */
/*************************************************************************************************/
void WWDG_IRQHandler(void)
{
    ;                                                                // reserved...
}

void LVD_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TAMPER_IRQHandler(void)
{
    ;                                                                // reserved...
}

void RTC_IRQHandler(void)
{
    ;                                                                // reserved...
}

void FMC_IRQHandler(void)
{
    ;                                                                // reserved...
}

void RCU_IRQHandler(void)
{
    ;                                                                // reserved...
}

void EXTI0_IRQHandler(void)
{
    ;                                                                // reserved...
}

void EXTI1_IRQHandler(void)
{
    ;                                                                // reserved...
}

void EXTI2_IRQHandler(void)
{
    ;                                                                // reserved...
}

void EXTI3_IRQHandler(void)
{
    ;                                                                // reserved...
}

void EXTI4_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA0_Channel0_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA0_Channel1_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA0_Channel2_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA0_Channel3_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA0_Channel4_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA0_Channel5_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA0_Channel6_IRQHandler(void)
{
    ;                                                                // reserved...
}

void ADC0_1_IRQHandler(void)
{
    ;                                                                // reserved...
}

void USB_HP_CAN1_TX_IRQHandler(void)
{
    ;                                                                // reserved...
}

void USB_LP_CAN1_RX0_IRQHandler(void)
{
    ;                                                                // reserved...
}

void CAN0_RX1_IRQHandler(void)
{
    ;                                                                // reserved...
}

void CAN0_EWMC_IRQHandler(void)
{
    ;                                                                // reserved...
}

void EXTI5_9_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIMER0_BRK_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIMER0_UP_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIMER0_TRG_CMT_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIMER0_Channel_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIMER1_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIMER2_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIMER3_IRQHandler(void)
{
    ;                                                                // reserved...
}

void I2C0_EV_IRQHandler(void)
{
    ;                                                                // reserved...
}

void I2C0_ER_IRQHandler(void)
{
    ;                                                                // reserved...
}

void I2C1_EV_IRQHandler(void)
{
    ;                                                                // reserved...
}

void I2C1_ER_IRQHandler(void)
{
    ;                                                                // reserved...
}

void SPI0_IRQHandler(void)
{
    ;                                                                // reserved...
}

void SPI1_IRQHandler(void)
{
    ;                                                                // reserved...
}

void USART0_IRQHandler(void)
{
    ;                                                                // reserved...
}

void USART1_IRQHandler(void)
{
    ;                                                                // reserved...
}

void USART2_IRQHandler(void)
{
    ;                                                                // reserved...
}

void EXTI10_15_IRQHandler(void)
{
    ;                                                                // reserved...
}

void RTC_Alarm_IRQHandler(void)
{
    ;                                                                // reserved...
}

void USBWakeUp_IRQHandler(void)
{
    ;                                                                // reserved...
}

#elif defined(GD32F30X_HD)
/*************************************************************************************************/
/*                           以下为默认用户中断处理函数                                          */
/*************************************************************************************************/
void WWDG_IRQHandler(void)
{
    ;                                                                // reserved...
}

void LVD_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TAMPER_IRQHandler(void)
{
    ;                                                                // reserved...
}

void RTC_IRQHandler(void)
{
    ;                                                                // reserved...
}

void FMC_IRQHandler(void)
{
    ;                                                                // reserved...
}

void RCU_IRQHandler(void)
{
    ;                                                                // reserved...
}

void EXTI0_IRQHandler(void)
{
    ;                                                                // reserved...
}

void EXTI1_IRQHandler(void)
{
    ;                                                                // reserved...
}

void EXTI2_IRQHandler(void)
{
    ;                                                                // reserved...
}

void EXTI3_IRQHandler(void)
{
    ;                                                                // reserved...
}

void EXTI4_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA0_Channel0_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA0_Channel1_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA0_Channel2_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA0_Channel3_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA0_Channel4_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA0_Channel5_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA0_Channel6_IRQHandler(void)
{
    ;                                                                // reserved...
}

void ADC0_1_IRQHandler(void)
{
    ;                                                                // reserved...
}

void USB_HP_CAN1_TX_IRQHandler(void)
{
    ;                                                                // reserved...
}

void USB_LP_CAN1_RX0_IRQHandler(void)
{
    ;                                                                // reserved...
}

void CAN0_RX1_IRQHandler(void)
{
    ;                                                                // reserved...
}

void CAN0_EWMC_IRQHandler(void)
{
    ;                                                                // reserved...
}

void EXTI5_9_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIMER0_BRK_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIMER0_UP_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIMER0_TRG_CMT_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIMER0_Channel_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIMER1_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIMER2_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIMER3_IRQHandler(void)
{
    ;                                                                // reserved...
}

void I2C0_EV_IRQHandler(void)
{
    ;                                                                // reserved...
}

void I2C0_ER_IRQHandler(void)
{
    ;                                                                // reserved...
}

void I2C1_EV_IRQHandler(void)
{
    ;                                                                // reserved...
}

void I2C1_ER_IRQHandler(void)
{
    ;                                                                // reserved...
}

void SPI0_IRQHandler(void)
{
    ;                                                                // reserved...
}

void SPI1_IRQHandler(void)
{
    ;                                                                // reserved...
}

void USART0_IRQHandler(void)
{
    ;                                                                // reserved...
}

void USART1_IRQHandler(void)
{
    ;                                                                // reserved...
}

void USART2_IRQHandler(void)
{
    ;                                                                // reserved...
}

void EXTI10_15_IRQHandler(void)
{
    ;                                                                // reserved...
}

void RTC_Alarm_IRQHandler(void)
{
    ;                                                                // reserved...
}

void USBWakeUp_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIM8_BRK_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIM8_UP_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIM8_TRG_COM_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIM8_CC_IRQHandler(void)
{
    ;                                                                // reserved...
}

void ADC3_IRQHandler(void)
{
    ;                                                                // reserved...
}

void FSMC_IRQHandler(void)
{
    ;                                                                // reserved...
}

void SDIO_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIMER4_IRQHandler(void)
{
    ;                                                                // reserved...
}

void SPI2_IRQHandler(void)
{
    ;                                                                // reserved...
}

void UART3_IRQHandler(void)
{
    ;                                                                // reserved...
}

void UART4_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIMER5_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIMER6_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA1_Channel0_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA1_Channel1_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA1_Channel2_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA2_Channel4_5_IRQHandler(void)
{
    ;                                                                // reserved...
}

#elif defined(GD32F30X_CL)
/*************************************************************************************************/
/*                           以下为默认用户中断处理函数                                          */
/*************************************************************************************************/
void WWDG_IRQHandler(void)
{
    ;                                                                // reserved...
}

void LVD_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TAMPER_IRQHandler(void)
{
    ;                                                                // reserved...
}

void RTC_IRQHandler(void)
{
    rtc_flag_clear(RTC_FLAG_ALARM);
    exti_flag_clear(EXTI_17);                                                        // reserved...
}

void FMC_IRQHandler(void)
{
    ;                                                                // reserved...
}

void RCU_IRQHandler(void)
{
    ;                                                                // reserved...
}

void EXTI0_IRQHandler(void)
{
    ;                                                                // reserved...
}

void EXTI1_IRQHandler(void)
{
    ;                                                                // reserved...
}

void EXTI2_IRQHandler(void)
{
    ;                                                                // reserved...
}

void EXTI3_IRQHandler(void)
{
    ;                                                                // reserved...
}

void EXTI4_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA0_Channel0_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA0_Channel1_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA0_Channel2_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA0_Channel3_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA0_Channel4_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA0_Channel5_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA0_Channel6_IRQHandler(void)
{
    ;                                                                // reserved...
}

void ADC0_1_IRQHandler(void)
{
    ;                                                                // reserved...
}

void CAN0_TX_IRQHandler(void)
{
    ;                                                                // reserved...
}

void CAN0_RX0_IRQHandler(void)
{
    ;                                                                // reserved...
}

void CAN0_RX1_IRQHandler(void)
{
    ;                                                                // reserved...
}

void CAN0_EWMC_IRQHandler(void)
{
    ;                                                                // reserved...
}

void EXTI5_9_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIMER0_BRK_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIMER0_UP_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIMER0_TRG_CMT_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIMER0_Channel_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIMER1_IRQHandler(void)
{
    //timer_delay_irq();                                                                // reserved...
}

void TIMER2_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIMER3_IRQHandler(void)
{
    ;                                                                // reserved...
}

void I2C0_EV_IRQHandler(void)
{
    ;                                                                // reserved...
}

void I2C0_ER_IRQHandler(void)
{
    ;                                                                // reserved...
}

void I2C1_EV_IRQHandler(void)
{
    ;                                                                // reserved...
}

void I2C1_ER_IRQHandler(void)
{
    ;                                                                // reserved...
}

void SPI0_IRQHandler(void)
{
    ;                                                                // reserved...
}

void SPI1_IRQHandler(void)
{
    ;                                                                // reserved...
}

void USART0_IRQHandler(void)
{
    ;                                                                // reserved...
}

void USART1_IRQHandler(void)
{
    ;                                                                // reserved...
}

void USART2_IRQHandler(void)
{
    ;                                                                // reserved...
}

void EXTI10_15_IRQHandler(void)
{
    ;                                                                // reserved...
}

void RTC_Alarm_IRQHandler(void)
{
    ;                                                                // reserved...
}

void USBFS_WKUP_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIMER4_IRQHandler(void)
{
    ;                                                                // reserved...
}

void SPI2_IRQHandler(void)
{
    ;                                                                // reserved...
}

void UART3_IRQHandler(void)
{
    ;                                                                // reserved...
}

void UART4_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIMER5_IRQHandler(void)
{
    ;                                                                // reserved...
}

void TIMER6_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA1_Channel0_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA1_Channel1_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA1_Channel2_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA1_Channel3_IRQHandler(void)
{
    ;                                                                // reserved...
}

void DMA1_Channel4_IRQHandler(void)
{
    ;                                                                // reserved...
}

void ENET_IRQHandler(void)
{
    ;                                                                // reserved...
}

void ENET_WKUP_IRQHandler(void)
{
    ;                                                                // reserved...
}

void CAN1_TX_IRQHandler(void)
{
    ;                                                                // reserved...
}

void CAN1_RX0_IRQHandler(void)
{
    ;                                                                // reserved...
}

void CAN1_RX1_IRQHandler(void)
{
    ;                                                                // reserved...
}

void CAN1_EWMC_IRQHandler(void)
{
    ;                                                                // reserved...
}

void USBFS_IRQHandler(void)
{
    //usbh_isr (&usb_core_dev);                                                                // reserved...
}

#endif

