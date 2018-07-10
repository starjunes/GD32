/********************************************************************************
**
** 文件名:     yx_lowpower_drv.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现芯片低功耗管理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2017/11/06 | 谢宁鸿 |  创建第一版本
*********************************************************************************/
#include "hal_include.h"
#include "st_gpio_drv.h"
#include "st_exti_drv.h"
#include "st_rtc_drv.h"
#include "st_irq_drv.h"
#include "hal_can_drv.h"
#include "hal_wdg_drv.h"
#include "hal_lowpower_drv.h"

#define SLEEP_MODE_SLEEP_MASK   0x5555
#define SLEEP_MODE_STOP_MASK    0xAAAA
#define SLEEP_MODE_STANDBY_MASK 0xCCCC

#define WKUP_PIN_ACC GPIO_PIN_A5
#define WKUP_PIN_GSM GPIO_PIN_B0
#define BKP_SLEEP_FLAG  BKP_DR4

typedef struct {
    BOOLEAN acc;
    BOOLEAN rtc;
    BOOLEAN gsm;
    BOOLEAN rtcwake;
    INT32U sleeptime;
}TCB_T;

static TCB_T s_tcb;

/*******************************************************************
** 函数名称: config_sleep_rcc
** 函数描述: 配置休眠后时钟
** 参数:     无
** 返回:     无
********************************************************************/
static void config_sleep_rcc(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

    #if 0
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1 ,DISABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN2 ,DISABLE);

    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, DISABLE);
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_USART1, DISABLE);
    
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_USART2, DISABLE);
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_USART3, DISABLE);
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_UART4, DISABLE);
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_UART5, DISABLE);
    //RCC_APB1PeriphResetCmd(RCC_APB1Periph_USART6, DISABLE);


    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, DISABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, DISABLE);
    
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, DISABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, DISABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, DISABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, DISABLE);
    #endif

}

/*******************************************************************
** 函数名称: config_sleep_gpio
** 函数描述: 配置休眠io
** 参数:     [in]onoff: 1 进入休眠 0 退出休眠
** 返回:     无
********************************************************************/
static void config_sleep_gpio(INT8U onoff)
{
    if(onoff) {

        #if 0
       /* 关闭串口 */
       //ST_UART_CloseUart(UART_COM_0);
       //ST_UART_CloseUart(UART_COM_1);
       //ST_UART_CloseUart(UART_COM_2);
       //ST_UART_CloseUart(UART_COM_3);
       ST_UART_Pause(UART_COM_0, 0, 0);
       ST_UART_Pause(UART_COM_1, 0, 0);
       ST_UART_Pause(UART_COM_2, 0, 0);
       ST_UART_Pause(UART_COM_3, 0, 0);
       /* 关闭led */
       ST_GPIO_WritePin(GPIO_PIN_A13, TRUE);

        ST_GPIO_SetPin(GPIO_PIN_A14, GPIO_DIR_IN, GPIO_MODE_ANALOG, 0);  /* IC卡复位脚 */
        ST_GPIO_SetPin(GPIO_PIN_A10, GPIO_DIR_IN, GPIO_MODE_ANALOG, 0);  /* 4G RX */
        ST_GPIO_SetPin(GPIO_PIN_A9,  GPIO_DIR_IN, GPIO_MODE_ANALOG, 0);  /* 4G TX */
        ST_GPIO_SetPin(GPIO_PIN_C8,  GPIO_DIR_IN, GPIO_MODE_ANALOG, 0);  /* IC CLK */
        ST_GPIO_SetPin(GPIO_PIN_C0,  GPIO_DIR_IN, GPIO_MODE_ANALOG, 0);  /* 按键_确认 */
        ST_GPIO_SetPin(GPIO_PIN_C1,  GPIO_DIR_IN, GPIO_MODE_ANALOG, 0);  /* 按键_上 */
        ST_GPIO_SetPin(GPIO_PIN_C2,  GPIO_DIR_IN, GPIO_MODE_ANALOG, 0);  /* 按键_下 */
        ST_GPIO_SetPin(GPIO_PIN_C3,  GPIO_DIR_IN, GPIO_MODE_ANALOG, 0);  /* 按键_菜单 */
        #endif
    }else {

        #if 0
        //ST_GPIO_SetPin(GPIO_PIN_A14, GPIO_DIR_OUT, GPIO_MODE_PP, 0);  /* IC卡复位脚 hal_ic_drv.c PinsConfig会恢复 *
        // ST_GPIO_SetPin(GPIO_PIN_C8,  GPIO_DIR_OUT, GPIO_MODE_PP, 0);  /* IC CLK 同上,或者IC驱动会恢复*/
        //ST_GPIO_SetPin(GPIO_PIN_A10, GPIO_DIR_IN, GPIO_MODE_ANALOG, 0);  /* 4G RX ST_UART_Restart会对其复原*/
       
        ST_GPIO_SetPin(GPIO_PIN_C0,  GPIO_DIR_IN, GPIO_MODE_UP, 0);  /* 按键_确认 */
        ST_GPIO_SetPin(GPIO_PIN_C1,  GPIO_DIR_IN, GPIO_MODE_UP, 0);  /* 按键_上 */
        ST_GPIO_SetPin(GPIO_PIN_C2,  GPIO_DIR_IN, GPIO_MODE_UP, 0);  /* 按键_下 */
        ST_GPIO_SetPin(GPIO_PIN_C3,  GPIO_DIR_IN, GPIO_MODE_UP, 0);  /* 按键_菜单 */
        
        ST_UART_Restart(UART_COM_0);
        ST_UART_Restart(UART_COM_1);
        ST_UART_Restart(UART_COM_2);
        ST_UART_Restart(UART_COM_3);
        #endif
    }
}

/*******************************************************************
** 函数名称: config_sleep_initvalue
** 函数描述: 初始化变量值
** 参数:     [in] void
** 返回:     无
********************************************************************/
static void config_sleep_initvalue(void)
{
    s_tcb.acc = FALSE;
    s_tcb.rtc = FALSE;
    s_tcb.gsm = FALSE;
    s_tcb.rtcwake = FALSE;
}

/*******************************************************************
** 函数名称: enter_sleep_mode
** 函数描述: 进入休眠模式
** 参数:     无
** 返回:     无
********************************************************************/
static void enter_sleep_mode(void)
{
    __WFI();
}

/*******************************************************************
** 函数名称: enter_stop_mode
** 函数描述: 进入停止模式
** 参数:     无
** 返回:     无
********************************************************************/
static void enter_stop_mode(void)
{
    PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);
}

/*******************************************************************
** 函数名称: enter_standby_mode
** 函数描述: 进入待机模式
** 参数:     无
** 返回:     无
********************************************************************/
static void enter_standby_mode(void)
{
    PWR_WakeUpPinCmd(ENABLE);
    PWR_EnterSTANDBYMode();
}


/*******************************************************************
** 函数名称: wakeup_int_callback
** 函数描述: 外部中断处理函数
** 参数:     无
** 返回:     无
********************************************************************/
static __attribute__ ((section ("IRQ_HANDLE"))) void wakeup_exti_int_callback(void)
{
    s_tcb.acc = TRUE;
    if(EXTI_GetITStatus(EXTI_Line5) != RESET) {
        EXTI_ClearITPendingBit(EXTI_Line5);
    }
}

/*******************************************************************
** 函数名称: wakeup_int_callback
** 函数描述: 外部中断处理函数
** 参数:     无
** 返回:     无
********************************************************************/
static __attribute__ ((section ("IRQ_HANDLE"))) void wakeup_gsm_exti_int_callback(void)
{
    s_tcb.gsm = TRUE;

    if(EXTI_GetITStatus(EXTI_Line0) != RESET) {
        EXTI_ClearITPendingBit(EXTI_Line0);
    }
}

/*******************************************************************
** 函数名称: wakeup_rtc_int_callback
** 函数描述: RTC中断处理函数
** 参数:     无
** 返回:     无
********************************************************************/
static __attribute__ ((section ("IRQ_HANDLE"))) void wakeup_rtc_int_callback(void)
{
    
     if(EXTI_GetITStatus(EXTI_Line17) != RESET) {
        EXTI_ClearITPendingBit(EXTI_Line17);
     }

     if(HAL_LowPower_GetFlag(LOW_POWER_FLAG_WUF)) {
        HAL_LowPower_ClearFlag(LOW_POWER_FLAG_WUF);
     }
     
     /* 不知道105这芯片抽什么风, (RTC->CRH & RTC_IT_ALR)获取的状态一直为0,导致RTC_GetITStatus(RTC_IT_ALR)无法使用*/     
     /* Wait until last write operation on RTC registers has finished */
     RTC_WaitForLastTask2();
     if((RTC->CRL & RTC_IT_ALR)) {
        RTC_ClearITPendingBit(RTC_IT_ALR);
     }
     /* Wait until last write operation on RTC registers has finished */
     RTC_WaitForLastTask2();
        
}

/*******************************************************************
** 函数名称: wakeup_config_sysclk
** 函数描述: 配置唤醒时钟
** 参数:     无
** 返回:     无
********************************************************************/
static void wakeup_config_sysclk(void)
{

   ErrorStatus HSEStartUpStatus;
   
  /* Enable HSE */
  RCC_HSEConfig(RCC_HSE_ON);

  /* Wait till HSE is ready */
  HSEStartUpStatus = RCC_WaitForHSEStartUp();

  if(HSEStartUpStatus == SUCCESS)
  {

#ifdef STM32F10X_CL
    /* Enable PLL2 */ 
    RCC_PLL2Cmd(ENABLE);

    /* Wait till PLL2 is ready */
    while(RCC_GetFlagStatus(RCC_FLAG_PLL2RDY) == RESET)
    {
    }

#endif

    /* Enable PLL */ 
    RCC_PLLCmd(ENABLE);

    /* Wait till PLL is ready */
    while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)
    {
    }

    /* Select PLL as system clock source */
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

    /* Wait till PLL is used as system clock source */
    while(RCC_GetSYSCLKSource() != 0x08)
    {
    }
  }
}


/*******************************************************************
** 函数名称: wakeup_config_int
** 函数描述: 唤醒中断配置
** 参数:     无
** 返回:     无
********************************************************************/
static void wakeup_config_interrupt(INT8U onoff)
{
   if(onoff) {
        ST_EXTI_OpenExtiFunction(WKUP_PIN_ACC, EXTI_TRIG_RAIS, wakeup_exti_int_callback); 
        ST_EXTI_OpenExtiFunction(WKUP_PIN_GSM, EXTI_TRIG_RAIS, wakeup_gsm_exti_int_callback); 
   }else {

        ST_EXTI_CloseExtiFunction(WKUP_PIN_ACC);
        ST_EXTI_CloseExtiFunction(WKUP_PIN_GSM);
   }
   

   #if 0
   ST_GPIO_WritePin(GPIO_PIN_C6, 0);
   ST_GPIO_WritePin(GPIO_PIN_C7, 0);

   ST_GPIO_WritePin(GPIO_PIN_D5, 0);
   ST_GPIO_WritePin(GPIO_PIN_D6, 0);

   ST_GPIO_WritePin(GPIO_PIN_D8, 0);
   ST_GPIO_WritePin(GPIO_PIN_D9, 0);
   #endif
}

/*******************************************************************
** 函数名称: wakeup_config_rtc
** 函数描述: rtc唤醒配置
** 参数:     无
** 返回:     无
********************************************************************/
static BOOLEAN wakeup_config_rtc(INT8U time)
{
    INT8U result;
    //INT32U ct_delay = 0;
    EXTI_InitTypeDef  EXTI_InitStructure;

    //ST_BKP_ClearAll();
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    PWR_BackupAccessCmd(ENABLE);	
    result = ST_RTC_OpenRtcFunction(RTC_CLOCK_LSE);
    if(!result) {
        return FALSE;
    }
    
    /* Enable the RTC Clock */  
    //RCC_RTCCLKCmd(ENABLE);
    //RTC_WaitForSynchro();
    
    //#if 0
    /* Connect EXTI_Line22 to the RTC Wakeup event */
    EXTI_ClearITPendingBit(EXTI_Line17);
    EXTI_StructInit(&EXTI_InitStructure);
    EXTI_InitStructure.EXTI_Line = EXTI_Line17;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    /* Enable the RTC Wakeup Interrupt */
    ST_IRQ_ConfigIrqEnable(RTCAlarm_IRQn, false);                                      /* 关闭中断 */
    ST_IRQ_InstallIrqHandler(RTCAlarm_IRQn, (IRQ_SERVICE_FUNC)wakeup_rtc_int_callback);                  /* install irq handle */
    ST_IRQ_ConfigIrqPriority(RTCAlarm_IRQn, 0, 0);
    ST_IRQ_ConfigIrqEnable(RTCAlarm_IRQn, true);                                       /* 打开中断 */

    //#endif

    
    RTC_ClearFlag(RTC_FLAG_SEC);
    #if 0
    while(RTC_GetFlagStatus(RTC_FLAG_SEC) == RESET) {
        ClearWatchdog();
        ct_delay++;
        if(ct_delay > 0x24f000) {
            break;
        }
    }

        
    if(ct_delay >= 0x24f000) {

        return FALSE;
    }
    #endif

    RTC_ClearFlag(RTC_FLAG_ALR);
    RTC_SetAlarm(RTC_GetCounter() + time);
    result =  RTC_WaitForLastTask2();
    if(!result) {
        return FALSE;
    }

    /* Enable the Wakeup Interrupt */
    RTC_ClearITPendingBit(RTC_IT_ALR);

    RTC_ITConfig(RTC_IT_ALR, ENABLE);
    

    result = RTC_WaitForSynchro2();
    if(!result) {
        return FALSE;
    }

    return result;
    
}

/*******************************************************************
** 函数名称: HAL_LowPower_InitDrv
** 函数描述: 初始化驱动
** 参数:     无
** 返回:     无
********************************************************************/
void HAL_LowPower_InitDrv(void)
{
    
    YX_MEMSET(&s_tcb, 0, sizeof(s_tcb));
}


/*******************************************************************
** 函数名称: HAL_LowPower_Enter
** 函数描述: 进入低功耗模式
** 参数:     [in] mode 见LOW_POWER_MODE_E
**           [in] time 定时唤醒时间
** 返回:     无
********************************************************************/
void HAL_LowPower_Enter(INT8U mode, INT32U time)
{

    if(HAL_LowPower_GetFlag(LOW_POWER_FLAG_WUF)) {
        HAL_LowPower_ClearFlag(LOW_POWER_FLAG_WUF);
    }
    
    config_sleep_rcc();
    config_sleep_gpio(TRUE);
    wakeup_config_interrupt(TRUE);
    config_sleep_initvalue();
    
    switch(mode) {
        case LOW_POWER_MODE_SLEEP:
            BKP_WriteBackupRegister(BKP_SLEEP_FLAG, SLEEP_MODE_SLEEP_MASK);
            enter_sleep_mode();
            break;
        case LOW_POWER_MODE_STOP: 
            if(time > 0) {
                s_tcb.sleeptime = ST_RTC_GetTimeMap() + time;
                s_tcb.rtcwake   = TRUE;
            }else {
                s_tcb.rtcwake   = FALSE;
            }
            
            BKP_WriteBackupRegister(BKP_SLEEP_FLAG, SLEEP_MODE_STOP_MASK);
            do {
                
                ClearWatchdog();
                wakeup_config_rtc(2);
                
                enter_stop_mode();
                if(HAL_LowPower_GetFlag(LOW_POWER_FLAG_WUF)) {
                    HAL_LowPower_ClearFlag(LOW_POWER_FLAG_WUF);
                }

               /* 定时唤醒计数 */
               if((s_tcb.rtcwake) && (s_tcb.sleeptime > 0)) {
                  if(ST_RTC_GetTimeMap() >= s_tcb.sleeptime) {
                      s_tcb.rtc = TRUE;
                  }
               }
                
            }while((s_tcb.acc != TRUE) && (s_tcb.rtc != TRUE) && (s_tcb.gsm != TRUE));
            
            /* 执行到此处说明设备已经被唤醒 */
            wakeup_config_sysclk();
            HAL_LowPower_Exti();

            break;
        case LOW_POWER_MODE_STANDBY:
            BKP_WriteBackupRegister(BKP_SLEEP_FLAG, SLEEP_MODE_STANDBY_MASK);
            /* 清唤醒及standby标志 */
            if(HAL_LowPower_GetFlag(LOW_POWER_FLAG_SB)) {
                HAL_LowPower_ClearFlag(LOW_POWER_FLAG_SB);
            }
            enter_standby_mode();
            break;
    }

}

/*******************************************************************
** 函数名称: HAL_LowPower_Exti
** 函数描述: 退出低功耗模式
** 参数:     [in] void
** 返回:     无
********************************************************************/
BOOLEAN HAL_LowPower_Exti(void)
{
    BKP_WriteBackupRegister(BKP_SLEEP_FLAG, 0);
    config_sleep_gpio(FALSE);
    wakeup_config_interrupt(FALSE);
    
    return TRUE;
}

/*******************************************************************
** 函数名称: HAL_LowPower_GetFlag
** 函数描述: 获取低功耗标志
** 参数:     [in] mode 见LOW_POWER_FLAG_E
** 返回:     flag
********************************************************************/
BOOLEAN HAL_LowPower_GetFlag(INT8U flag)
{
    switch(flag) {
        case LOW_POWER_FLAG_WUF:
            if (PWR_GetFlagStatus(PWR_FLAG_WU) == SET) {
                return TRUE;
            }
            break;
        case LOW_POWER_FLAG_SB:
            if (PWR_GetFlagStatus(PWR_FLAG_SB) == SET) {
                return TRUE;
            }
			break;
    }
    
    return FALSE;
}

/*******************************************************************
** 函数名称: HAL_LowPower_ClearFlag
** 函数描述: 清低功耗标志
** 参数:     [in] mode 见LOW_POWER_FLAG_E
** 返回:     无
********************************************************************/
void HAL_LowPower_ClearFlag(INT8U flag)
{
    /* st库有问题,用库函数无法清446芯片flag */
    switch(flag) {
        case LOW_POWER_FLAG_WUF:
            /* CWUF */
            //PWR->CR |=  1 << 2;
            PWR_ClearFlag(PWR_FLAG_WU);
            break;
        case LOW_POWER_FLAG_SB: 
            /* CSBF */
            //PWR->CR |=  1 << 3;
            PWR_ClearFlag(PWR_FLAG_SB);
            break;
    }
}


