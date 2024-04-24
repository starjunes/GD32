/********************************************************************************
**
** 文件名:     dal_hard.c
** 版权所有:   (c) 2014-2020 厦门雅迅网络股份有限公司
** 文件描述:   系统板级硬件初始化和驱动相关函数集
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2015/12/30 | JUMP   | 创建本模块
**
*********************************************************************************/
#include "dal_include.h"
#include "dal_hard.h"
#include "dal_pinlist.h"
#include "dal_exti.h"
#include "dal_adc.h"
#include "dal_exti.h"
#include "debug_print.h"

#define VECTOR_TAB_OFFSET  0x00 /*!< Vector Table base offset field. This value must be a multiple of 0x200. */

#define EN_SAVE            0

#if EN_DEBUG > 1
#define DEBUG_LP           1
#else
#define DEBUG_LP           0
#endif


/*******************************************************************
** 函数名:     RCC_Configuration
** 函数描述:   RCC系统时钟初始化配置
** 参数:       无
** 返回:       无
********************************************************************/
static void RCC_Configuration(void)
{
    //rcu_deinit();                                                    /* RCC system reset */
	
    SystemInit();
	rcu_osci_on(RCU_HXTAL);
    //rcu_hxtal_clock_monitor_enable();                              /* Enable Clock Security System(CSS) */
	

	rcu_periph_clock_enable(RCU_DMA0);
    rcu_periph_clock_enable(RCU_DMA1);
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_GPIOE);
    rcu_periph_clock_enable(RCU_AF);             /*开复用时钟*/
	//禁用外部低速时钟
	rcu_periph_clock_enable(RCU_PMU);
	rcu_periph_clock_enable(RCU_AF);
    pmu_backup_write_enable();
	rcu_osci_off(RCU_LXTAL);
	pmu_backup_write_disable();
	gpio_pin_remap_config(GPIO_SWJ_SWDPENABLE_REMAP,ENABLE);	//PA15作为普通IO
}

/*******************************************************************
** 函数名:     NVIC_Configuration
** 函数描述:   系统中断向量优先级配置
** 参数:       无
** 返回:       无
** 注意事项:   此处配置的中断优先级，将影响Man_IRQ.H中的枚举取值，修改时请注意关联
********************************************************************/
static void NVIC_Configuration(void)
{
    nvic_vector_table_set(NVIC_VECTTAB_RAM, 0);                   /* 中断向量重定位到RAM开头(0x20000000) */
    nvic_priority_group_set(NVIC_PRIGROUP_PRE1_SUB3);             /* 配置一位抢占优先级 */
}

/*******************************************************************
** 函数名:     IWDG_Configuration
** 函数描述:   内部看门狗初始化配置
** 参数:       无
** 返回:       无
********************************************************************/
static void IWDG_Configuration(void)
{
    fwdgt_write_enable();
    fwdgt_config(1562 + 100,FWDGT_PSC_DIV256);
//    IWDG_SetReload(625 * sec + 100);             /* 100预留点时间, 防止应用sec清狗一次 */
    fwdgt_write_disable();
    fwdgt_counter_reload();
    fwdgt_enable();                                                  /* Enable IWDG */
}

/*******************************************************************
** 函数名:     System_Initiate
** 函数描述:   系统板级配置初始化
** 参数:       无
** 返回:       无
********************************************************************/
void System_Initiate(void)
{
  RCC_Configuration();                                             /* SYSCLK, HCLK, PCLK2 and PCLK1 configuration*/
   // ICCard_SpeciConfig();                                            /* IC卡模块需要的特殊配置 */
    NVIC_Configuration();                                            /* Config vecter table base address,priority group*/
    IWDG_Configuration();   
}
#if EN_SAVE > 0
/*******************************************************************
** 函数名:     mmi_entersavemode
** 函数描述:   进入休眠模式
** 参数:       [IN] mode               休眠模式
** 返回:       无
********************************************************************/
void mmi_entersavemode(uint8_t mode)
{
    ClearWatchdog();                                                        /* 本函数中的几个清看门狗不要去掉 */

    ADC_DeInit();
    RCC_HSEConfig(RCC_HSE_OFF);
    EXTI_ClearITPendingBit(EXTI_Line22);
  #if EN_DEBUG > 1
    Debug_SysPrint("mmi_entersavemode() EXTI->PR = %x IMR = %x!!!\r\n", EXTI->PR, EXTI->IMR);
  #endif
    RTC_WakeUpCmd(ENABLE);
    RTC_ITConfig(RTC_IT_WUT, ENABLE);
    PWR_EnterSTOPMode(PWR_Regulator_LowPower, mode);
    ClearWatchdog();

    while (EXTI_GetITStatus(EXTI_LINE_ACC) == RESET) {
        if (RTC_GetITStatus(RTC_IT_WUT) != RESET) {
            RTC_ClearITPendingBit(RTC_IT_WUT);
            EXTI_ClearITPendingBit(EXTI_Line22);
        }
        PWR_EnterSTOPMode(PWR_Regulator_LowPower, mode);
        ClearWatchdog();
    }
    /* 执行到此处表示已被ACC中断唤醒 */
    RTC_WakeUpCmd(DISABLE);
    RTC_ITConfig(RTC_IT_WUT, DISABLE);
    RCC_Configuration();
    InitADC();
  #if EN_DEBUG > 1
    Debug_SysPrint("PWR_ExitSTOPMode = %x!!!\r\n", EXTI->PR);
  #endif
}
#endif

/*******************************************************************
** 函数名:     ClearWatchdog
** 函数描述:   定时喂狗函数
** 参数:       无
** 返回:       无
********************************************************************/
void ClearWatchdog(void)
{
    fwdgt_counter_reload();                                      /* Reload IWDG counter */
	PORT_ClearWatchdogHS();    /* 清除硬件看门狗 */
}

/*******************************************************************
** 函数名:     ReStartDevice
** 函数描述:   重启设备函数
** 参数:       无
** 返回:       无
********************************************************************/
void ReStartDevice(void)
{
    while(1);
}

/*******************************************************************
** 函数名:     Dal_HardSysReset
** 函数描述:   系统复位
** 参数:       无
** 返回:       无
********************************************************************/
void Dal_HardSysReset(void)
{
    __set_FAULTMASK(1);                                                         /* 软件复位 */
    NVIC_SystemReset();
}
/*******************************************************************
** 函数名:     Delay_ms
** 函数描述:   延时函数
** 参数:       无
** 返回:       无
********************************************************************/
static void Delay_ms(INT16U time)
{
    INT32U  delaycnt;

    delaycnt = 0xA000 * time;                                          /* 约150ms */
    while (delaycnt--) {
    }
}

/*******************************************************************
** 函数名:     Dal_McuLowPower
** 函数描述:   低功耗处理函数
** 参数:       [in] hook : 钩子函数
               [in] wktime:休眠唤醒时间 0:无定时休眠
** 返回:       无
********************************************************************/
void Dal_McuLowPower(INT16U (*hook)(void), INT16U wktime)
{
    INT16U hook_ret;
    INT16U time = 0;
    INT8U wake_time = 9;//闹钟唤醒时间，保证设备不被看门狗复位，最大9s
    if (hook == NULL) return;
    
    ClearWatchdog();                                                        /* 本函数中的几个清看门狗不要去掉 */

    adc_deinit(ADC0);
	adc_deinit(ADC1);
	rcu_osci_off(RCU_HXTAL);
    #if EN_DEBUG > 1
    Debug_SysPrint("mmi_entersavemode() EXTI->PR = %x IMR = %x!!!\r\n", EXTI->PR, EXTI->IMR);
    #endif

	dal_rtc_init();
	rcu_hxtal_clock_monitor_disable();
	rtc_lwoff_wait();//等待寄存器写完成
	rtc_alarm_config(rtc_counter_get() + wake_time);
	rtc_lwoff_wait();//等待寄存器写完成
	rtc_interrupt_enable(RTC_INT_ALARM);
	rcu_all_reset_flag_clear();
	ClearWatchdog();
	Delay_ms(1);//需要加延时，否则唤醒失败
    pmu_to_deepsleepmode(PMU_LDO_LOWPOWER, WFE_CMD);	
    while ((hook_ret = hook()) == 0) {
        time++;
        if ((time*9/60 >= wktime) && (wktime > 0)){
			break;
		}
        if (rtc_flag_get(RTC_FLAG_ALARM) != RESET) {
            rtc_flag_clear(RTC_FLAG_ALARM);
            exti_flag_clear(EXTI_17);
        }
        /* Mcu进入stop低功耗模式 */
		dal_rtc_init();
		rtc_lwoff_wait();//等待寄存器写完成
		rtc_alarm_config(rtc_counter_get() + wake_time);
		rtc_lwoff_wait();//等待寄存器写完成
		rtc_interrupt_enable(RTC_INT_ALARM);
		
		ClearWatchdog();
        Delay_ms(1);
        pmu_to_deepsleepmode(PMU_LDO_LOWPOWER, WFE_CMD);
        ClearWatchdog();
    }
    
    /* 执行到此处表示已被ACC或者其他唤醒源唤醒中断唤醒 */
	ClearWatchdog();
	SystemInit();
	rtc_interrupt_disable(RTC_INT_ALARM);
	rtc_flag_clear(RTC_FLAG_ALARM);
	exti_flag_clear(EXTI_17);
	pmu_flag_clear(PMU_FLAG_RESET_WAKEUP);
    
	rcu_osci_on(RCU_HXTAL);
    //rcu_hxtal_clock_monitor_enable();                              /* Enable Clock Security System(CSS) */
    InitADC();
    #if DEBUG_LP > 0
    debug_printf_dir("<***** 唤醒源:0x%08x *****>\r\n",hook_ret);
    #endif
}

/**************************** (C) COPYRIGHT 2010  XIAMEN YAXON.LTD **************END OF FILE******/
