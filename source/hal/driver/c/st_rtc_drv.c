/********************************************************************************
**
** 文件名:     st_rtc_drv.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现RTC实时时钟配置管理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2014/04/09 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include <time.h>
#include "hal_include.h"
#include "stm32f10x.h"
#include "st_gpio_drv.h"
#include "st_irq_drv.h"
#include "st_rtc_drv.h"
#include "yx_debug.h"

BOOLEAN printf_com(const char *fmt, ...);

/*
********************************************************************************
* 定义配置参数
********************************************************************************
*/

#define RTC_CONFIG           0xAA

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    INT8U status;
    INT8U battle;
} DCB_T;

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static DCB_T s_rtc;


/*******************************************************************
** 函数名称: RTC_WaitForSynchro2
** 函数描述: 对st库RTC_WaitForSynchro函数进行重写,加入计数防止无限循环
** 参数:     无
** 返回:     是否执行成功.
********************************************************************/
BOOLEAN RTC_WaitForSynchro2(void)
{

  INT32U ct_delay;

  ct_delay = 0;
  /* Clear RSF flag */
  RTC->CRL &= (uint16_t)~RTC_FLAG_RSF;
  /* Loop until RSF flag is set */
  while ((RTC->CRL & RTC_FLAG_RSF) == (uint16_t)RESET)
  {

    ClearWatchdog();
    ct_delay++;
    if(ct_delay > 0x4f000) {
        break;
    }
  }

  if(ct_delay <= 0x4f000) {
      return TRUE;
  }

  return FALSE;
}

/*******************************************************************
** 函数名称: RTC_WaitForLastTask2
** 函数描述: 对st库RTC_WaitForLastTask函数进行重写,加入计数防止无限循环
** 参数:     无
** 返回:     是否执行成功.
********************************************************************/

BOOLEAN RTC_WaitForLastTask2(void)
{

    INT32U ct_delay;
  
    ct_delay = 0;

    /* Loop until RTOFF flag is set */
    while ((RTC->CRL & RTC_FLAG_RTOFF) == (uint16_t)RESET)
    {
        ClearWatchdog();
        ct_delay++;
        if(ct_delay > 0x4f000) {
            break;
        }
    }

    if(ct_delay <= 0x4f000) {
      return TRUE;
    }

    return FALSE;
}



/*******************************************************************
** 函数名称: RTC_PwrConfig
** 函数描述: 电源控制配置
** 参数:     无
** 返回:     无
********************************************************************/
static void RTC_PwrConfig(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);                        /* 使能PWR 时钟*/
    PWR_BackupAccessCmd(ENABLE);                                               /* 使能RTC和备份域寄存器写访问 */
}

/*******************************************************************
** 函数名称: RTC_ClockConfig
** 函数描述: RTC时钟配置
** 参数:     [in] clock:  时钟源, 见 RTC_CLOCK_E
** 返回:     成功返回TRUE,失败返回FALSE
********************************************************************/
static BOOLEAN RTC_ClockConfig(INT8U clock)
{
    INT32U ct_delay;
    
    ct_delay = 0;
    if (clock == RTC_CLOCK_LSI) {                                              /* 当使用LSI 作为RTC 时钟源*/
        RCC_LSICmd(ENABLE);                                                    /* 使能LSI 振荡*/
        
        ct_delay = 0;
        while (++ct_delay < 0x8000) {                                          /* 等待到LSI 准备就绪 */
            ClearWatchdog();
            if (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == SET) {
                break;
            }
        }
        
        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);                                /* 把RTC 时钟源配置为LSI */
    } else if (clock == RTC_CLOCK_LSE) {                                       /* 当使用LSE 最为RTC 时钟源*/

        if(!ST_RTC_ClockIsOpen(clock)) {
            RCC_LSEConfig(RCC_LSE_ON);                                             /* 使能LSE 振荡*/
        }
        
        //RCC_LSEDriveConfig(RCC_LSEDrive_High);
        ct_delay = 0;

        while (++ct_delay < 0x4f000) {                                          /* 等待到LSE 准备就绪 */
            ClearWatchdog();
            if (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == SET) {
                break;
            }
        }

        
        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);                                /* 把RTC 时钟源配置为使用LSE */
    } else {
        ;
    }
    
    if (ct_delay < 0x4f000) {
        RCC_RTCCLKCmd(ENABLE);                                                 /* 使能RTC 时钟*/
        if(!RTC_WaitForSynchro2()) {                                            /* Wait for RTC APB registers synchronisation */
            return FALSE;
        }
        
        if(!RTC_WaitForLastTask2()) {
            return FALSE;
        }
        return true;
    } else {
        return false;
    }
}

/*******************************************************************
** 函数名称: RTC_RtcConfig
** 函数描述: RTC参数配置
** 参数:     [in] clock:  时钟源, 见 RTC_CLOCK_E
** 返回:     成功返回TRUE,失败返回FALSE
********************************************************************/
static BOOLEAN RTC_RtcConfig(INT8U clock)
{
    //INT32U synprediv, asynprediv;
    //RTC_InitTypeDef RTC_InitStruct;
    
    if (clock == RTC_CLOCK_LSI) {                                              /* 当使用LSI 作为RTC 时钟源*/
        //synprediv  = 400 - 1;                                                  /* 同步分频值和异步分频值*/
        //asynprediv = 100 - 1;                                                  /* 40000HZ / (400 * 100) = 1HZ */
        RTC_SetPrescaler(40000);
    } else if (clock == RTC_CLOCK_LSE) {                                       /* 当使用LSE 最为RTC 时钟源*/
        //synprediv  = 256 - 1;                                                  /* 同步分频值和异步分频值*/
        //asynprediv = 128 - 1;                                                  /* 32768HZ/(256 * 128) =1HZ */
        RTC_SetPrescaler(32767);                               /* RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1) */
    } else {
        //synprediv  = 2500 - 1;                                                 /* 同步分频值和异步分频值*/
        //asynprediv = 100 - 1;                                                  /* 32768HZ/(256 * 128) =1HZ */
        return FALSE;
    }

    if(!RTC_WaitForLastTask2()) {
        return FALSE;
    }
    BKP_WriteBackupRegister(BKP_DR1, RTC_CONFIG);                      /* Indicator for the RTC configuration */

    return TRUE;

    #if 0
    RTC_InitStruct.RTC_HourFormat   = RTC_HourFormat_24;                       /*!< Specifies the RTC Hour Format.This parameter can be a value of @ref RTC_Hour_Formats */
    RTC_InitStruct.RTC_AsynchPrediv = asynprediv;                              /*!< Specifies the RTC Asynchronous Predivider value.This parameter must be set to a value lower than 0x7F */
    RTC_InitStruct.RTC_SynchPrediv  = synprediv;                               /*!< Specifies the RTC Synchronous Predivider value.This parameter must be set to a value lower than 0x1FFF */
    
    if (RTC_Init(&RTC_InitStruct) == SUCCESS) {
        RTC_WaitForSynchro();
        BKP_WriteBackupRegister(BKP_DR1, RTC_CONFIG);                      /* Indicator for the RTC configuration */
        return true;
    } else {
        //BKP_WriteBackupRegister(BKP_DR1, RTC_CONFIG);                      /* Indicator for the RTC configuration */
        return false;
    }
    #endif
}


/*******************************************************************
** 函数名称: RTC_SetSystime
** 函数描述: 设置系统时间
** 参数:     [in] date:    系统日期
**           [in] time:    系统时间
**           [in] weekday:  周天,1-7
** 返回:     成功返回TRUE,失败返回FALSE
********************************************************************/
static BOOLEAN RTC_SetSystime(DATE_T *date, TIME_T *time, INT8U weekday)
{
    struct tm timetm;
    INT32U timemap;


    //weekday = weekday;
    timetm.tm_year  = (date->year + 100);  
    timetm.tm_mon   = (date->month - 1);                                       /* c库月份0~11代表1到12月份 */
    timetm.tm_mday  = date->day;  
    timetm.tm_hour  = time->hour;  
    timetm.tm_min   = time->minute;  
    timetm.tm_sec   = time->second;
    timetm.tm_wday  = (weekday == 7)? 0 : weekday;
    timemap = mktime(&timetm);
    RTC_SetCounter(timemap);

    BKP_WriteBackupRegister(BKP_DR2, RTC_CONFIG);                              /* 标记时间已经被设置 */

    return TRUE;
    #if 0
    ErrorStatus result;
    RTC_TimeTypeDef RTC_TimeStruct;
    RTC_DateTypeDef RTC_DateStruct;
    
    RTC_TimeStruct.RTC_Hours   = time->hour;                                   /*!< Specifies the RTC Time Hour */
    RTC_TimeStruct.RTC_Minutes = time->minute;                                 /*!< Specifies the RTC Time Minutes.This parameter must be set to a value in the 0-59 range. */
    RTC_TimeStruct.RTC_Seconds = time->second;                                 /*!< Specifies the RTC Time Seconds.This parameter must be set to a value in the 0-59 range. */
    if (time->hour < 12) {
        RTC_TimeStruct.RTC_H12 = RTC_H12_AM;                                   /*!< Specifies the RTC AM/PM Time.This parameter can be a value of @ref RTC_AM_PM_Definitions */
    } else {
        RTC_TimeStruct.RTC_H12 = RTC_H12_PM;
    }
    
    result = RTC_SetTime(RTC_Format_BIN, &RTC_TimeStruct);
    if (result != SUCCESS) {
        return false;
    }
    
    RTC_DateStruct.RTC_WeekDay = weekday;                                      /*!< Specifies the RTC Date WeekDay.This parameter can be a value of @ref RTC_WeekDay_Definitions */
    RTC_DateStruct.RTC_Month   = date->month;                                  /*!< Specifies the RTC Date Month.This parameter can be a value of @ref RTC_Month_Date_Definitions */
    RTC_DateStruct.RTC_Date    = date->day;                                    /*!< Specifies the RTC Date.This parameter must be set to a value in the 1-31 range. */
    RTC_DateStruct.RTC_Year    = date->year;                                   /*!< Specifies the RTC Date Year.This parameter must be set to a value in the 0-99 range. */
    
    result = RTC_SetDate(RTC_Format_BIN, &RTC_DateStruct);
    if (result != SUCCESS) {
        return false;
    }
    
    BKP_WriteBackupRegister(BKP_DR2, RTC_CONFIG);                          /* Indicator for the RTC configuration */
    
    return true;
    #endif
}

/*******************************************************************
** 函数名称: RTC_GetSystime
** 函数描述: 获取系统时间
** 参数:     [in] date:       系统日期
**           [in] time:       系统时间
**           [in] weekday:    周天,1-7
**           [in] subsecond:  小数点秒
** 返回:     成功返回TRUE,失败返回FALSE
********************************************************************/
static BOOLEAN RTC_GetSystime(DATE_T *date, TIME_T *time, INT8U *weekday, INT32U *subsecond)
{
    INT32U timemap;
    struct tm *timetm;
    
    //if((date == NULL) || (time == NULL) || (weekday == NULL)) {
    //    return FALSE;
    //}

    if(subsecond != 0) {
        subsecond = subsecond;
    } 
    
    timemap = RTC_GetCounter();
    timetm = localtime(&timemap);

    if(date != 0) {
        date->year      = (timetm->tm_year - 100);
        date->month     = (timetm->tm_mon + 1);                                    /* c库月份0~11代表1到12月份 */
        date->day       = timetm->tm_mday;  
    }

    if(time != 0) {
        time->hour      = timetm->tm_hour;
        time->minute    = timetm->tm_min;
        time->second    = timetm->tm_sec;
    }

    if(weekday != 0) {
        (*weekday)      = (timetm->tm_wday == 0) ? 7 : (timetm->tm_wday);          /* C库0为周天 */
    }
    

    return TRUE;
    #if 0
    RTC_TimeTypeDef RTC_TimeStruct;
    RTC_DateTypeDef RTC_DateStruct;
    
    *subsecond = RTC_GetSubSecond();
    RTC_GetTime(RTC_Format_BIN, &RTC_TimeStruct);
    RTC_GetDate(RTC_Format_BIN, &RTC_DateStruct);
    
    time->hour   = RTC_TimeStruct.RTC_Hours;
    time->minute = RTC_TimeStruct.RTC_Minutes;
    time->second = RTC_TimeStruct.RTC_Seconds;
    
    date->month = RTC_DateStruct.RTC_Month;
    date->day   = RTC_DateStruct.RTC_Date;
    date->year  = RTC_DateStruct.RTC_Year; 
    
    *weekday    = RTC_DateStruct.RTC_WeekDay;
    
    return true;
    #endif
}

/*******************************************************************
** 函数名称: ST_RTC_InitDrv
** 函数描述: 初始化模块
** 参数:     无
** 返回:     无
********************************************************************/
void ST_RTC_InitDrv(void)
{
    YX_MEMSET(&s_rtc, 0, sizeof(s_rtc));
    
    if (BKP_ReadBackupRegister(BKP_DR1) != RTC_CONFIG) {                   /* 未开启实时时钟功能 */
        s_rtc.battle = FALSE;
    } else {
        s_rtc.battle = TRUE;
    }
}

/*******************************************************************
** 函数名称: ST_RTC_OpenRtcFunction
** 函数描述: 打开RTC功能
** 参数:     [in] clock:  时钟源, 见 RTC_CLOCK_E
** 返回:     成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN ST_RTC_OpenRtcFunction(INT8U clock)
{
    BOOLEAN result;
    INT8U curclock;

    /* 只有当被设置时钟源与当前时钟源不一致才复位 */
    curclock = ST_RTC_GetClock();
    if((curclock != clock) && (curclock != RTC_CLOCK_MAX)) {
        BKP_DeInit();                                                              /* Reset Backup Domain */
    }
    
    if (BKP_ReadBackupRegister(BKP_DR1) == RTC_CONFIG) {
        if (s_rtc.status == 0x00) {
            s_rtc.status = 0x01;
            
            RTC_PwrConfig();
            if (clock == RTC_CLOCK_LSI) {
                RCC_LSICmd(ENABLE);                                            /* 使能LSI 振荡*/
            }
        
            if(!RTC_WaitForSynchro2()) {                                              /* Wait for RTC APB registers synchronisation */
                return false;
            }
        }
        return true;
    }

    RTC_PwrConfig();
    //RTC_DeInit();                                                              /* 恢复RTC寄存器 */
    
    //RCC_BackupResetCmd(ENABLE);                                                /* 复位备份区和时钟配置 */
    //RCC_BackupResetCmd(DISABLE);                                               /* 不复位 */
    
    //RTC_PwrConfig();
    result = RTC_ClockConfig(clock);
    if (result) {
        s_rtc.status = 0x01;
        result = RTC_RtcConfig(clock);
        
        #if DEBUG_ERR > 0
        printf_com("<RTC_RtcConfig(%d)>\r\n", result);
        #endif
    } else {
        #if DEBUG_ERR > 0
        printf_com("<RTC_ClockConfig(%d)>\r\n", result);
        #endif
    }
    return result;
}

/*******************************************************************
** 函数名称: ST_RTC_CloseRtcFunction
** 函数描述: 关闭RTC功能
** 参数:     无
** 返回:     成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN ST_RTC_CloseRtcFunction(void)
{

    #if EN_M3_TEMP > 0
    return FALSE;
    #else
    if (BKP_ReadBackupRegister(BKP_DR1) == 0x00000000) {
        s_rtc.status = 0x00;
        return true;
    }
    
    s_rtc.status = 0x00; 
    RTC_PwrConfig();
    //RTC_DeInit();                                                                                      /* 恢复RTC寄存器 */
    BKP_DeInit();
    RCC_BackupResetCmd(ENABLE);                                                /* 恢复备份区和时钟配置 */
    RCC_BackupResetCmd(DISABLE);                                               /* 不复位 */
    PWR_BackupAccessCmd(DISABLE);                                              /* 关闭RTC和备份域寄存器写访问 */
    RCC_LSEConfig(RCC_LSE_OFF);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, DISABLE);                       /* 关闭PWR 时钟*/
    
    return true;
    #endif
}

/*******************************************************************
** 函数名称: ST_RTC_SetSystime
** 函数描述: 设置系统时间
** 参数:     [in] date:    系统日期
**           [in] time:    系统时间
**           [in] weekday:  周天,1-7
** 返回:     成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN ST_RTC_SetSystime(DATE_T *date, TIME_T *time, INT8U weekday)
{
    if (BKP_ReadBackupRegister(BKP_DR1) == RTC_CONFIG) {
        return RTC_SetSystime(date, time, weekday);
    } else {
        return false;
    }
}

/*******************************************************************
** 函数名称: ST_RTC_GetSystime
** 函数描述: 获取系统时间
** 参数:     [in] date:       系统日期
**           [in] time:       系统时间
**           [in] weekday:    周天,1-7
**           [in] subsecond:  小数点秒
** 返回:     成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN ST_RTC_GetSystime(DATE_T *date, TIME_T *time, INT8U *weekday, INT32U *subsecond)
{
    if (BKP_ReadBackupRegister(BKP_DR1) != RTC_CONFIG) {                   /* 未开启实时时钟功能 */
        return false;
    }
    
    if (BKP_ReadBackupRegister(BKP_DR2) != RTC_CONFIG) {                   /* 未配置实时时钟 */
        return false;
    }
    
    return RTC_GetSystime(date, time, weekday, subsecond);
}

/*******************************************************************
** 函数名称: ST_RTC_BattleIsNormal
** 函数描述: 获取RTC电池状态
** 参数:     无
** 返回:     正常返回TRUE,否则返回FALSE
********************************************************************/
BOOLEAN ST_RTC_BattleIsNormal(void)
{
    return s_rtc.battle;
}

/*******************************************************************
** 函数名称: ST_RTC_GetTimeMap
** 函数描述: 获取时间计数器
** 参数:     void
** 返回:     timemap
********************************************************************/
INT32U ST_RTC_GetTimeMap(void)
{
    return RTC_GetCounter();
}

/*******************************************************************
** 函数名称: ST_RTC_ClockIsOpen
** 函数描述: 检测RTC时钟是否开启
** 参数:     [in] clock: 时钟源.见.RTC_CLOCK_E
** 返回:     正常返回TRUE,否则返回FALSE
********************************************************************/
BOOLEAN ST_RTC_ClockIsOpen(INT8U clock)
{
    if(clock == RTC_CLOCK_LSI) {
        if (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == SET) {
            return TRUE;
        }
    } else if(clock == RTC_CLOCK_LSE) {
        if (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == SET) {
            return TRUE;
        }
    }

    return FALSE;
}

/*******************************************************************
** 函数名称: ST_RTC_ClockIsOpen
** 函数描述: 检测RTC时钟是否开启
** 参数:     [in] clock: 时钟源.见.RTC_CLOCK_E
** 返回:     正常返回TRUE,否则返回FALSE
********************************************************************/
BOOLEAN ST_RTC_OpenClock(INT8U clock)
{
    #if 0
    if(ST_RTC_ClockIsOpen(clock)) {
        return TRUE;
    }
    #endif
    
    if(clock == RTC_CLOCK_LSI) {
        RCC_LSICmd(ENABLE);
    }else if (clock == RTC_CLOCK_LSE){
        RCC_LSEConfig(RCC_LSE_ON);  
    }

    return TRUE;
}

/*******************************************************************
** 函数名称: ST_RTC_CloseClock
** 函数描述: 关闭时钟源
** 参数:     [in] clock: 时钟源.见.RTC_CLOCK_E
** 返回:     正常返回TRUE,否则返回FALSE
********************************************************************/
BOOLEAN ST_RTC_CloseClock(INT8U clock)
{
    
    if(clock == RTC_CLOCK_LSI) {
        RCC_LSICmd(DISABLE);
    }else if (clock == RTC_CLOCK_LSE){
        RCC_LSEConfig(RCC_LSE_OFF);  
    }

    return TRUE;
}

/*******************************************************************
** 函数名称: ST_RTC_GetClock
** 函数描述: 获取RTC当前时钟源
** 参数:     void
** 返回:     见RTC_CLOCK_E
********************************************************************/
INT8U ST_RTC_GetClock(void)
{
    INT32U value;

    value = RCC->BDCR;
    value = ((value & 0x300) >> 8);

    if(value == 1) {
        return RTC_CLOCK_LSE;
    } else if(value == 2) {
        return RTC_CLOCK_LSI;
    } else if(value == 3) {
        return RTC_CLOCK_HSE_DIV32;
    }

    return RTC_CLOCK_MAX;
}



/**************************** (C) COPYRIGHT 2012  XIAMEN YAXON.LTD **************END OF FILE******/

