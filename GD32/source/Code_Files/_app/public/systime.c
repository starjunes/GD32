/**************************************************************************************************
**                                                                                               **
**  文件名称:  systime.c                                                                         **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2010                                    **
**  创建信息:  jump -- 2011年1月20日                                                             **
**  文件描述:  系统时间管理                                                                      **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/

#define SYSTIME_GLOBALS
#include "app_include.h"
#include "dal_include.h"
#include "Man_Timer.h"
#include "Tools.H"
#include "systime.h"
//#include "accesspara.h"

/*
********************************************************************************
*                   DEFINE MODULE VARIANT
********************************************************************************
*/

static SYSTIME_T systime;
#if 0
static INT8U const CDAY[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

/**************************************************************************************************
**  函数名称:  GetWeekDay
**  功能描述:  获取星期
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
INT8U GetWeekDay(void)
{
    INT8U i, WeekDay;
     
    WeekDay = 5;                   /* 从2010年1月1日开始计，星期5 */
    for (i = 3; i < systime.date.year; i++) {
        WeekDay++;                 /* 每年多1天 */
        if (i % 4 == 0) {
           WeekDay++;
        }
    }
    for (i = 1; i < systime.date.month; i++) {
        WeekDay += (CDAY[i-1] - 28);
        if ((i == 2) && (systime.date.year % 4 == 0)) {
           WeekDay++;
        }
    }
    WeekDay += systime.date.day - 1;
    WeekDay %= 7;
    return WeekDay;
}
#endif

/**************************************************************************************************
**  函数名称:  ReviseSysTime
**  功能描述:  更新时间
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
void ReviseSysTime(INT8U *curtime)
{
    memcpy(&systime, curtime, sizeof(SYSTIME_T));
}
#if 0
static void SecondTmrProc(void)
{
    INT8U curday;
    //StartTmr(secondtmr, _SECOND, 1);//可自动重载,无需手动
    
    if (++systime.time.second >= 60) {
        if (systime.date.month == 2 && (systime.date.year % 4) == 0)
            curday = 29;
        else
            curday = CDAY[systime.date.month - 1];

        if (systime.time.second >= 60) {
            systime.time.second -= 60;
            systime.time.minute++;
        }
        if (systime.time.minute >= 60) {
            systime.time.minute -= 60;
            systime.time.hour++;
        }
        if (systime.time.hour >= 24) {
            systime.time.hour -= 24;
            systime.date.day++;
        }
        if (systime.date.day > curday) {
            systime.date.day = 1;
            systime.date.month++;
        }
        if (systime.date.month > 12) {
            systime.date.month = 1;
            systime.date.year++;
        }
        /*if (--ct_revise == 0) {
            ClaimForSysTime();//探询时间
            if (revised) {
                ct_revise = PERIOD_MAXREVISE;
            } else {
                ct_revise = PERIOD_MINREVISE;
            }
        }*/
    }
}
#endif
/**************************************************************************************************
**  函数名称:  InitSysTime
**  功能描述:  初始化默认时间
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
void InitSysTime(void)
{
    systime.date.year   = 10;                    /* systime = 20010/1/1  0:0:0 */
    systime.date.month  = 1;
    systime.date.day    = 1;
    
    systime.time.hour   = 0;
    systime.time.minute = 0;
    systime.time.second = 0;
}

/**************************************************************************************************
**  函数名称:  GetSysTime
**  功能描述:  获取系统时间
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
void GetSysTime(SYSTIME_T *ptr)
{
    //memcpy(ptr, &systime, sizeof(SYSTIME_T));
	PORT_GetSysTime1(ptr);
}

/**************************************************************************************************
**  函数名称:  GetAsciiTime
**  功能描述:  获取系统时间Ascii形式
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
void GetAsciiTime(ASCII_TIME *asciitime)
{
    HexToAscii(asciitime->ac_hour,   &systime.time.hour, 1);
    HexToAscii(asciitime->ac_minute, &systime.time.minute, 1);
    HexToAscii(asciitime->ac_second, &systime.time.second, 1);
}

/**************************************************************************************************
**  函数名称:  GetAsciiDate
**  功能描述:  获取系统日期Ascii形式
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
void GetAsciiDate(ASCII_DATE *asciidate)
{
    HexToAscii(asciidate->ac_year, &systime.date.year,  1);
    HexToAscii(asciidate->ac_month, &systime.date.month, 1);
    HexToAscii(asciidate->ac_day, &systime.date.day, 1);
}

/**************************************************************************************************
**  函数名称:  GetFatFileDate
**  功能描述:  获取FAT系统时间
**  输入参数:  
**  FAT时间格式: 16位构成
**                Bits 0-4  : 秒，2秒1单位，可以是0-29
**                Bits 5-10 : 分，可以是0-59
**                Bits 11-15: 时，可以是0-23
**  返回参数:  
**************************************************************************************************/
INT16U GeFatFileTime(void)
{
    INT16U  file_time;

    file_time = (((INT16U)systime.time.hour << 11) | ((INT16U)systime.time.minute << 5) | (INT16U)(systime.time.second / 2));
    return file_time;
}

/**************************************************************************************************
**  函数名称:  GetFatFileDate
**  功能描述:  获取FAT系统日期
**  输入参数:  
**  FAT日期格式: 16位构成，相对于1980年1月1日的偏差量
**                Bits 0-4  : 日期 可以是1-31区间
**                Bits 5-8  : 月份 可以是1-12区间，1表示一月份
**                Bits 9-15 : 年份,从1980开始计算，可以在0-127区间(1980-2107)
**  返回参数:  
**************************************************************************************************/
INT16U GetFatFileDate(void)
{
    INT16U  file_date;
    
    file_date = (((INT16U)(systime.date.year + 0x7D0 - 0x7BC) << 9)   /* xx + 2000 - 1980 */
                  | ((INT16U)systime.date.month << 5) | (INT16U)systime.date.day);
    return file_date;
}

void YX_GetBcdSysTime(BCD_TIME_T *Bcd)
{
    Bcd->Year = ((systime.date.year   / 10) << 4) + systime.date.year   % 10;
    Bcd->Mon  = ((systime.date.month  / 10) << 4) + systime.date.month  % 10;
    Bcd->Day  = ((systime.date.day    / 10) << 4) + systime.date.day    % 10;

    Bcd->Hour = ((systime.time.hour   / 10) << 4) + systime.time.hour   % 10;
    Bcd->Min  = ((systime.time.minute / 10) << 4) + systime.time.minute % 10;
    Bcd->Sec  = ((systime.time.second / 10) << 4) + systime.time.second % 10;
}

