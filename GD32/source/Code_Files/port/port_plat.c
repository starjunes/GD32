/********************************************************************************
**
** 文件名:     port_plat.c
** 版权所有:   (c) 1998-2017 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现平台复位、省电、清看门狗等平台接口
**
*********************************************************************************
**                  修改历史记录
**===============================================================================
**| 日期              | 作者        |  修改记录
**===============================================================================
**| 2017/05/15 | 黄运峰    |  创建该文件
********************************************************************************/
#include "dal_include.h"
#include "port_plat.h"
#include "port_gpio.h"
#include "app_include.h"

/*
*********************************************************************************
*                   模块数据类型、变量及宏定义
*********************************************************************************
*/
// static char const s_app_file[] = "/mnt/disk2/UPDATE/app.bin";
/*
*********************************************************************************
*                   本地接口实现
*********************************************************************************
*/

/*
*********************************************************************************
*                   对外接口实现
*********************************************************************************
*/
//----------------------------------------------------------
//                  系统时钟相关接口
//----------------------------------------------------------

SYSTIME_T systime;

/********************************************************************************
**  函数名:     PORT_SetSysTime
**  函数描述:   设置系统时间
**  参数:       [in] stime: 系统时间
**  返回:       成功返回true，失败返回false
********************************************************************************/
BOOLEAN PORT_SetSysTime(SYSTIME_T *dt)
{

	INT8U time[7],rtc_time[7];
	
	dal_rtc_settime((INT8U *)dt);
    systime.date.year   = dt->date.year;
    systime.date.month  = dt->date.month;
    systime.date.day    = dt->date.day;

    systime.time.hour   = dt->time.hour;
    systime.time.minute = dt->time.minute;
    systime.time.second = dt->time.second;

	dal_rtc_gettime(time, 7);
	rtc_time[0] = time[5];	//秒
	rtc_time[1] = time[4];	//分
	rtc_time[2] = time[3];  //时
	rtc_time[3] = time[6];  //周
	rtc_time[4] = time[2];  //日
	rtc_time[5] = time[1];  //月
	rtc_time[6] = time[0];  //年
	HAL_sd2058_SetCalendar(rtc_time);
	#if DEBUG_TEMP > 0
	HAL_sd2058_ReadCalendar(time);
	debug_printf("time:%d/%d/%d %d:%d:%d %d\r\n",time[6],time[5],time[4],time[2],\
				time[1],time[0],time[3]);
	#endif
    return TRUE;
    // return SHELL_PF_SetLocalTime(class_obj[CLASS_ID_PLATFORM], (INT8U *)&systime);
}
BOOLEAN PORT_GetSysTime1(SYSTIME_T *dtime)
{
	INT8U time[7];
	if(HAL_sd2058_ReadCalendar(time) == FALSE){
		dtime->date.year  = systime.date.year;
		dtime->date.month = systime.date.month;
		dtime->date.day   = systime.date.day;

		dtime->time.hour   = systime.time.hour;
		dtime->time.minute = systime.time.minute;
		dtime->time.second = systime.time.second;
	}else{
		dtime->date.year  = time[6];
		dtime->date.month = time[5];
		dtime->date.day   = time[4];

		dtime->time.hour   = time[5];
		dtime->time.minute = time[1];
		dtime->time.second = time[0];
	}

    return true;
}

/********************************************************************************
** 函数名:     PORT_GetSysTime
** 函数描述:   从平台中读取当前时间
** 参数:       [out] dtime:   时间
** 返回:       成功返回true，失败返回false
********************************************************************************/
BOOLEAN PORT_GetSysTime(TIME_T *dtime)
{
//	SYSTIME_T systime;
//    if (SHELL_PF_GetLocalTime(class_obj[CLASS_ID_PLATFORM], (INT8U *)&systime)) {
//        dtime->hour   = systime.time.hour;
//        dtime->minute = systime.time.minute;
//        dtime->second = systime.time.second;
//
//        return true;
//    } else {
//        return false;
//    }
    return FALSE;
}

/********************************************************************************
** 函数名:     PORT_GetSysDate
** 函数描述:   从平台中读取当前日期
** 参数:       [out] ddate:   日期
** 返回:       成功或失败
********************************************************************************/
BOOLEAN PORT_GetSysDate(DATE_T *ddate)
{
//	SYSTIME_T systime;
//    if (SHELL_PF_GetLocalTime(class_obj[CLASS_ID_PLATFORM], (INT8U *)&systime)) {
//        ddate->year  = systime.date.year;
//        ddate->month = systime.date.month;
//        ddate->day   = systime.date.day;
//
//        return true;
//    } else {
//        return false;
//    }
    return FALSE;
}
//----------------------------------------------------------
//                  系统复位及控制相关接口
//----------------------------------------------------------
/********************************************************************************
** 函数名:     PORT_ClearWatchdog
** 函数描述:   清看门狗(MCU软件看门狗)
** 参数:       无
** 返回:       无
********************************************************************************/
void PORT_ClearWatchdog(void)
{
     ClearWatchdog();
}
/*
********************************************************************************
** 函数名:   PORT_ClearWatchdogHS
** 函数描述: 清软硬件看门狗，须定时调用，以免溢出
** 参数:     无
** 返回:     无
********************************************************************************
*/
void PORT_ClearWatchdogHS(void)
{
    static BOOLEAN hw_wd = FALSE;
    if (hw_wd) {
        hw_wd = FALSE;
        PORT_SetGpioPin(PIN_FDWDG);
    } else {
        hw_wd = TRUE;
        PORT_ClearGpioPin(PIN_FDWDG);
    }

}

/********************************************************************************
** 函数名:     PORT_ResetCPU
** 函数描述:   正常关机复位接口
** 参数:       无
** 返回:       无
********************************************************************************/
void PORT_ResetCPU(void)
{
    Dal_HardSysReset();
}

/********************************************************************************
** 函数名:     PORT_ResetCPU_EXT
** 函数描述:   死循环等待外部看门狗复位
** 参数:       无
** 返回:       无
********************************************************************************/
void PORT_ResetCPU_EXT(void)
{
    for(;;);
}

/********************************************************************************
** 函数名:     PORT_GetResetReason
** 函数描述:   查询复位原因
** 参数:       无
** 返回:       复位原因类型，参见RST_REASON_E
********************************************************************************/
RST_REASON_E PORT_GetResetReason(void)
{
#if 0
    INT16U rstat;

    rstat = hal_GetResetReason();
    if (rstat & RST_PWRON) return RST_TYPE_PWRON;
    if (rstat & RST_EXTR)  return RST_TYPE_EXTR;
    if (rstat & RST_WDTO)  return RST_TYPE_WDTO;
    if (rstat & RST_SWR)   return RST_TYPE_SWR;
    if (rstat & RST_LVD)   return RST_TYPE_LVD;
    if (rstat != 0)        return RST_TYPE_OTHER;
#endif
    return RST_TYPE_NONE;
}
/********************************************************************************
** 函数名:     PORT_Assert
** 函数描述:   系统断言接口
** 参数:       [in] exp:  表达式
**             [in] file: 文件名
**             [in] line: 行号
** 返回:       无
********************************************************************************/
void PORT_Assert(void *exp, void *file, INT32U line)
{
    // SHELL_PF_Assert(class_obj[CLASS_ID_PLATFORM], exp, file, line);
}

/********************************************************************************
** 函数名:     PORT_DlyNumMS
** 函数描述:   延时若干毫秒(近似时间，受主频、中断等因素影响)
** 参数:       [in] num:  需要延时的毫秒数
** 返回:       无
********************************************************************************/
void PORT_DlyNumMS(INT32U num)
{
    //hal_DlyNumMS(num);
}
//----------------------------------------------------------
//                  系统信息读写相关接口
//----------------------------------------------------------
/********************************************************************************
** 函数名:     PORT_ReadSNCode
** 函数描述:   读取dataflash中的设备SN码
** 参数:       [out] sn: 返回sn码的缓存指针
**             [in] len: 要读取的sn码长度
** 返回:       TRUE, 读取成功； FALSE, 读取失败。
********************************************************************************/
BOOLEAN PORT_ReadSNCode(INT8U *sn, INT8U len)
{
    // return ReadDataFromSflash(sn, SN_CODE_ADDR, len);
    return FALSE;
}

/********************************************************************************
** 函数名:     PORT_WriteSNCode
** 函数描述:   向dataflash写入设备SN码
** 参数:       [in] sn: SN码缓存指针
**                 len: 要写入的sn码长度
** 返回:       TRUE, 写入成功； FALSE， 写入失败。
********************************************************************************/
BOOLEAN PORT_WriteSNCode(INT8U *sn, INT8U len)
{
//    INT32U rdata;
//    void *hdl;
//
//    if (!ReadDataFromSflash((INT8U *)&rdata, SN_CODE_ADDR, sizeof(rdata))) return FALSE;
//#if EN_DEBUG == 0
//    if (rdata != 0xffffffff) return FALSE;      // 已经写入过了，不能允许再次写入
//#endif
//    hdl = hal_sfm_Erase(SFM_INDEX_0, DRV_SFM_OP_SECTERZ, SN_CODE_ADDR, 1);
//    if (hdl == NULL) return FALSE;
//    return WriteDataToSflash(sn, SN_CODE_ADDR, len);
    return FALSE;
}

/********************************************************************************
** 函数名:     YX_PORT_GetHalVersion
** 函数描述:   获取平台版本号
** 参数:       无
** 返回:       成功返回版本号字符串指针，以'\0'为结束符，失败返回0
********************************************************************************/
char *PORT_GetHalVersion(void)
{
    return NULL;//(char *)hal_GetVersionStr();
}
//----------------------------------------------------------
//                  程序更新相关接口
//----------------------------------------------------------
/********************************************************************************
** 函数名:     PORT_GetAppFileName
** 函数描述:   获取无线下载文件名
** 参数:       无
** 返回:       指向文件名字符串的指针(ASCII码),以'\0'为结束符
********************************************************************************/
char const *PORT_GetAppFileName(void)
{
    // return s_app_file;
    return 0;
}

/********************************************************************************
** 函数名:     PORT_UpdateAppToCodeArea
** 函数描述:   将数据写入codeflash中的APP程序区
** 参数:       [in] fullname: 绝对路径文件名
**             [in] filesize: 文件大小
**             [in] proc:     需要在本函数调用的函数，如清看门狗
** 返回:       成功返回true，失败返回false
********************************************************************************/
BOOLEAN PORT_UpdateAppToCodeArea(char const *fullname, INT32U filesize, void (*proc)(void))
{
    // return SHELL_PF_UpdateAppToCodeArea(class_obj[CLASS_ID_PLATFORM], fullname, filesize, proc);
    return FALSE;
}

/********************************************************************************
** 函数名:     PORT_CheckLoadDataValid
** 函数描述:   检验下载到codeflash IMAGE区的程序数据有效性。
** 参数:       无
** 返回:       TRUE：校验成功且有效； FALSE：校验失败或无效
********************************************************************************/
BOOLEAN PORT_CheckLoadDataValid(void)
{
    return FALSE;
}
//----------------------------------------------------------
//                  系统低功耗、省电相关接口
//----------------------------------------------------------
/********************************************************************************
** 函数名:     PORT_EnterTransportMode
** 函数描述:   平台进入运输模式
** 参数:       [out] 无
**             [in] 无
** 返回:       无
********************************************************************************/
void PORT_EnterTransportMode(void)
{
    // SHELL_PF_PowerDown(class_obj[CLASS_ID_PLATFORM]);   // 即进入关机状态
}

/********************************************************************************
** 函数名:     PORT_Sleep
** 函数描述:   进入省电状态
** 参数:       无
** 返回:       无
********************************************************************************/
void PORT_Sleep(INT16U (*hook)(void))
{
	Dal_McuLowPower(hook);
}

/********************************************************************************
**  函数名称:  PORT_SetAlarm
**  功能描述:  RTC设置提醒中断函数
**  输入参数:  [in] instance:  RTC编号
**             [in] alarmtime: 设置时间（单位为分钟 /取值范围1-43200 [即1分钟-1个月]）
**  返回参数:  无
********************************************************************************/
void PORT_SetAlarm(INT16U alarmtime)
{
	//hal_rtc_SetAlarm(RTC_IDX_0, alarmtime);
}

/********************************************************************************
**  函数名称:  PORT_SetAlarmRepeat
**  功能描述:  RTC设置提醒中断函数
**  输入参数:  [in] instance:       RTC编号
**             [in] alarmtime:      设置时间（单位为分钟 /取值范围1-43200 [即1分钟-1个月]）
**             [in] repeatenable:   是否使能重复唤醒
**             [in] repeatinterval: 重复唤醒间隔(单位秒)
**  返回参数:  无
********************************************************************************/
void PORT_SetAlarmRepeat(INT16U alarmtime, BOOLEAN repeatenable, INT32U repeatinterval)
{
	//hal_rtc_SetAlarmRepeat(RTC_IDX_0, alarmtime, repeatenable, repeatinterval);
}




# if 0 // 暂不支持
/********************************************************************************
** 函数名:     PORT_EnterVLPRMode
** 函数描述:   MCU进入低功耗运行模式
** 参数:       无
** 返回:       成功返回true，失败返回false
********************************************************************************/
BOOLEAN PORT_EnterVLPMode(void)
{
    return FALSE;
}

/********************************************************************************
** 函数名:     PORT_ExitVLPRMode
** 函数描述:   MCU退出低功耗运行模式
** 参数:       无
** 返回:       成功返回true，失败返回false
********************************************************************************/
BOOLEAN PORT_ExitVLPMode(void)
{
    return FALSE;
}
#endif

//------------------------------------------------------------------------------
/* End of File */

