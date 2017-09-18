/******************************************************************************
**
** filename:     main.c
** copyright:    
** description:  该模块主要实现主程序入口函数main调用
**
-------------------------------------------------------------------------------
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#pragma import(__use_no_heap_region)
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "yx_include.h"
#include "dal_gpio_cfg.h"
#include "yx_debug.h"



/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
GPIO_InitTypeDef GPIO_InitStructure;


/* Private function prototypes -----------------------------------------------*/


/* Private functions ---------------------------------------------------------*/

/**
  * @brief   Main program
  * @param  None
  * @retval None
  */
  
BOOLEAN printf_com(const char *fmt, ...);
int main(void)
{
	//RCC_ClocksTypeDef RCC_Clocks;
  /*!< At this stage the microcontroller clock setting is already configured, 
       this is done through SystemInit() function which is called from startup
       file (startup_stm32f0xx.s) before to branch to application main.
       To reconfigure the default setting of SystemInit() function, refer to
       system_stm32f0xx.c file
     */
     
     
  SYSCFG_MemoryRemapConfig(SYSCFG_MemoryRemap_SRAM);
		 
       
  /* Initialize Leds mounted on STM32F0-discovery */
  //STM_EVAL_LEDInit(LED3);
  //STM_EVAL_LEDInit(LED4);

  /* Turn on LED3 and LED4 */
  //STM_EVAL_LEDOn(LED3);
  //STM_EVAL_LEDOn(LED4);
	
	/* Initialize User_Button on STM32F0-Discovery */
  //STM_EVAL_PBInit(BUTTON_USER, BUTTON_MODE_GPIO);

  /* Setup SysTick Timer for 1 msec interrupts.
     ------------------------------------------
    1. The SysTick_Config() function is a CMSIS function which configure:
       - The SysTick Reload register with value passed as function parameter.
       - Configure the SysTick IRQ priority to the lowest value (0x0F).
       - Reset the SysTick Counter register.
       - Configure the SysTick Counter clock source to be Core Clock Source (HCLK).
       - Enable the SysTick Interrupt.
       - Start the SysTick Counter.
    
    2. You can change the SysTick Clock source to be HCLK_Div8 by calling the
       SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8) just after the
       SysTick_Config() function call. The SysTick_CLKSourceConfig() is defined
       inside the stm32f0xx_misc.c file.

    3. You can change the SysTick IRQ priority by calling the
       NVIC_SetPriority(SysTick_IRQn,...) just after the SysTick_Config() function 
       call. The NVIC_SetPriority() is defined inside the core_cm0.h file.

    4. To adjust the SysTick time base, use the following formula:
                            
         Reload Value = SysTick Counter Clock (Hz) x  Desired Time base (s)
    
       - Reload Value is the parameter to be passed for SysTick_Config() function
       - Reload Value should not exceed 0xFFFFFF
   */
	 
	 
	
	/* SysTick end of count event each 1ms */
  //RCC_GetClocksFreq(&RCC_Clocks);
  //SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000);
    
    DAL_GPIO_InitWatchdog();
    ClearWatchdog();
    
    OS_InitErrMan();
    OS_InitTimer();
    OS_InitQMsg();
    
    #if DEBUG_SYS > 0
    printf_com("<-------------system start------------->\r\n");
    #endif
    
    //系统线程循环
    for (;;) {
        OS_TmrTskEntry();
        OS_MsgSchedEntry();
        OS_ErrTskEntry();
    }
}

/**
  * @brief  Inserts a delay time.
  * @param  nTime: specifies the delay time length, in milliseconds.
  * @retval None
  */
void Delay(__IO uint32_t nTime)
{
    //INT32U cpu_sr;
    
  //TimingDelay = nTime;
	
	//__disable_irq();
    //__enable_irq();
    
    //OS_ENTER_CRITICAL();
    //OS_EXIT_CRITICAL();
    

  //while(TimingDelay != 0);
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */ 

/**
  * @}
  */ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
