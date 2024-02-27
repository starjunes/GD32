/********************************************************************************
**
** 文件名:     dal_rtc.c
** 版权所有:   (c) 2014-2020 厦门雅迅网络股份有限公司
** 文件描述:   实时时钟模块驱动层接口函数
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2015/12/30 | JUMP   | 创建本模块
**
*********************************************************************************/
#include  "dal_rtc.h"
#include  "dal_include.h"

#include  "app_include.h"
#include  "dal_gpio.h"
#include  "dal_pinlist.h"
#include  "tools.h"
#include  "systime.h"
#include  "debug_print.h"
#include "yx_system.h"

#define RTC_CLOCK_SRC       1     /* 1:LSI(内部时钟源:32k), 0:LSE(外部时钟源:32.768k) */
#define FIRST_DATA          0x32F3

static BOOLEAN s_init_flag = TRUE;
INT8U const CWEEK[] = {7,1,2,3,4,5,6};//localtime转换后的周数据

/*******************************************************************
** 函数名:     isleap
** 函数描述:   判断是否为闰年
** 参数:       具体年份
** 返回:       0:平年  其他:闰年
********************************************************************/
int isleap(int year)
{
	return (year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0);
}


/*******************************************************************
** 函数名:     dal_rtc_config
** 函数描述:   实时时钟模块初始化
** 参数:       无
** 返回:       无
********************************************************************/
void dal_rtc_config(void)
{
    INT32U           err_cnt;
		INT8U timebuf[6];
  #if DEBUG_RTC > 0
    Debug_SysPrint("dal_rtc_config\r\n");
  #endif

	/* enable PMU and BKPI clocks */
    rcu_periph_clock_enable(RCU_BKPI);
    rcu_periph_clock_enable(RCU_PMU);
    /* allow access to BKP domain */
    pmu_backup_write_enable();

    /* reset backup domain */
    bkp_deinit();
  	#if RTC_CLOCK_SRC > 0
    rcu_osci_on(RCU_IRC40K);
	rcu_osci_stab_wait(RCU_IRC40K);
	rcu_rtc_clock_config(RCU_RTCSRC_IRC40K);
    #else
    /* Enable the LSE OSC */
    rcu_osci_on(RCU_LXTAL);
	rcu_osci_stab_wait(RCU_LXTAL);
	rcu_rtc_clock_config(RCU_RTCSRC_LXTAL);	
    #endif
    /* Enable the RTC Clock */
	rcu_periph_clock_enable(RCU_RTC);
    /* Wait for RTC APB registers synchronisation */
    //RTC_WaitForSynchro();
	rtc_register_sync_wait();
    /* reset backup domain */
	timebuf[0] = 21;
	timebuf[1]  = 9;
	timebuf[2] = 2;
	timebuf[3] = 14;
	timebuf[4]  = 0;
	timebuf[5]  = 0;
	dal_rtc_settime(timebuf);
	rtc_prescaler_set(39999);
	rtc_lwoff_wait();

    bkp_write_data(BKP_DATA_0,FIRST_DATA);
	rtc_lwoff_wait();
}

/*******************************************************************
** 函数名:     dal_rtc_init
** 函数描述:   实时时钟模块初始化
** 参数:       无
** 返回:       无
********************************************************************/
void dal_rtc_init(void)
{
    
    INT8U temp[6];

  #if DEBUG_RTC > 0
    Debug_SysPrint("dal_rtc_init()\r\n");
  #endif
    /* Enable the PWR APB1 Clock Interface */
    //RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    
	rcu_periph_clock_enable(RCU_PMU);
  	rcu_periph_clock_enable(RCU_BKPI);
    /* Allow access to BKP Domain */
	pmu_backup_write_enable();
	rcu_periph_clock_enable(RCU_RTC);
    //if (RTC_ReadBackupRegister(RTC_BKP_DR0) != FIRST_DATA) {
	if (bkp_read_data(BKP_DATA_0) != FIRST_DATA) {
        /* RTC Configuration */
        dal_rtc_config();
    } else {
        /* Wait for RTC APB registers synchronisation */
        rtc_register_sync_wait();
    }
	
	
    /* Connect EXTI_Line17 to the RTC Wakeup event */
	exti_flag_clear(EXTI_17);
	exti_init(EXTI_17,EXTI_EVENT,EXTI_TRIG_RISING);
  #if DEBUG_RTC > 0
    Debug_SysPrint(" s_init_flag:%d\r\n", s_init_flag);
  #endif
    //NVIC_IrqHandleInstall(RTC_IRQ, (ExecFuncPtr)USER_RTC_IRQHandler, CAN_PRIOTITY, true);

    /* RTC定时唤醒设置。用于在MCU休眠时定时唤醒喂狗 */
    dal_rtc_gettime(temp,6);
	rtc_interrupt_disable(RTC_INT_ALARM);
    ReviseSysTime(temp);
	
}

/*******************************************************************
** 函数名:     dal_rtc_gettime
** 函数描述:   获取实时时钟时间
** 参数:       [OUT] temo 时间输出地址
			   length 时间信息位数
** 返回:       无
********************************************************************/
void dal_rtc_gettime(INT8U* temp, INT8U length)
{
	struct tm* nowtime;
	INT32U curcnt = rtc_counter_get();
	if((PNULL == temp) || (length < 6)){
		return;
	}
	nowtime = localtime(&curcnt);
	temp[0] = nowtime->tm_year-100;
	temp[1] = nowtime->tm_mon+1;
	temp[2] = nowtime->tm_mday;
	temp[3] = nowtime->tm_hour;
	temp[4] = nowtime->tm_min;
	temp[5] = nowtime->tm_sec;
	if(length > 6){
		temp[6] = CWEEK[nowtime->tm_wday];
	}
    #if DEBUG_RTC > 0
    Debug_SysPrint("curcnt:%u dal_rtc_gettime: \r\n",curcnt);
    //Debug_PrintHex(TRUE, ptr, 6);
	debug_printf("%d/%d/%d %d:%d:%d\r\n",temp[0],temp[1],temp[2],\
					temp[3],temp[4],temp[5]);
    #endif
}

/*******************************************************************
** 函数名:     dal_rtc_settime
** 函数描述:   设置实时时钟时间
** 参数:       [IN] ptr 时间数据指针
** 返回:       无
********************************************************************/
void dal_rtc_settime(INT8U* ptr)
{
    struct tm time;
	time_t curtime=0;
	if(PNULL == ptr){
		return;
	}
	/* allow access to BKP domain */
    pmu_backup_write_enable();
	
	time.tm_year = ptr[0]+100;
	time.tm_mon	 = ptr[1]-1;
	time.tm_mday = ptr[2];
	time.tm_hour = ptr[3];
	time.tm_min  = ptr[4];
	time.tm_sec  = ptr[5];
	curtime = mktime(&time);
	rtc_counter_set(curtime);

      #if DEBUG_RTC > 0
        Debug_SysPrint("dal_rtc_settime!\r\n");
        debug_printf("%d/%d/%d %d:%d:%d\r\n", time.tm_year, time.tm_mon,\
			time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);
      #endif
}

/*******************************************************************
** 函数名:     dal_rtc_set_current_time
** 函数描述:   设置实时时钟时间为当前系统时钟
** 参数:       无
** 返回:       无
********************************************************************/
void dal_rtc_set_current_time(void)
{
    INT8U temp[6];
	dal_rtc_gettime(temp,6);
	rtc_interrupt_disable(RTC_INT_ALARM);
    ReviseSysTime(temp);

      #if DEBUG_RTC > 0
        Debug_SysPrint("dal_rtc_settime!\r\n");
        //Debug_PrintHex(TRUE, ptr, 6);
        //Debug_PrintHex(TRUE, (INT8U*)&cur_systime, 6);
      #endif
}

/*******************************************************************
** 函数名:     dal_rtc_reinit
** 函数描述:   实时时钟模块重新初始化
** 参数:       无
** 返回:       无
********************************************************************/
void dal_rtc_reinit(void)
{
    INT8U temp[6];

  #if DEBUG_RTC > 0
    Debug_SysPrint("dal_rtc_init()\r\n");
  #endif
    /* Enable the PWR APB1 Clock Interface */
    //RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
	
    /* Allow access to BKP Domain */
    //PWR_BackupAccessCmd(ENABLE);
	pmu_backup_write_enable();
    //if (RTC_ReadBackupRegister(RTC_BKP_DR0) != FIRST_DATA) {
	if (bkp_read_data(BKP_DATA_0) != FIRST_DATA) {
        /* RTC Configuration */
        dal_rtc_config();
    } else {
        /* Wait for RTC APB registers synchronisation */
        rtc_register_sync_wait();
    }

    /* Connect EXTI_Line17 to the RTC Wakeup event */
	exti_flag_clear(EXTI_17);
	exti_init(EXTI_17,EXTI_EVENT,EXTI_TRIG_RISING);
  #if DEBUG_RTC > 0
    Debug_SysPrint(" s_init_flag:%d\r\n", s_init_flag);
  #endif
    //NVIC_IrqHandleInstall(RTC_IRQ, (ExecFuncPtr)USER_RTC_IRQHandler, CAN_PRIOTITY, true);

    /* RTC定时唤醒设置。用于在MCU休眠时定时唤醒喂狗 */
    /*RTC_WakeUpCmd(DISABLE);
    RTC_ClearFlag(RTC_FLAG_WUTF);
    RTC_SetWakeUpCounter(9*2000);// 9秒
    RTC_WakeUpClockConfig(RTC_WakeUpClock_RTCCLK_Div16);
    RTC_ITConfig(RTC_IT_WUT, DISABLE);*/
	rtc_alarm_config(9*2000);
    dal_rtc_gettime(temp,6);
	rtc_interrupt_disable(RTC_INT_ALARM);
    ReviseSysTime(temp);
}

/*******************************************************************
** 函数名:     dal_rtc_getinitsta
** 函数描述:   获取RTC初始化状态
** 参数:       无
** 返回:       TRUE-RTC正常；FALSE-RTC初始化失败
********************************************************************/
BOOLEAN dal_rtc_getinitsta(void)
{
    return s_init_flag;
}


/**************************** (C) COPYRIGHT 2010  XIAMEN YAXON.LTD **************END OF FILE******/

