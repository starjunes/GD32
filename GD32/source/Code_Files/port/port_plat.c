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
#include "time.h"

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
**  函数名:     PORT_GetSysTimestamp
**  函数描述:   获取系统时间戳
**  参数:       [in] rtc_time:系统时间数组
**  返回:       成功返回时间戳
********************************************************************************/
INT32U PORT_GetSysTimestamp(INT8U* rtc_time)
{
	struct tm time;
	INT32U stamp;
	INT16U len = sizeof(rtc_time);

	time.tm_year = rtc_time[6]+100;
	time.tm_mon	 = rtc_time[5]-1;
	time.tm_mday = rtc_time[4];
	time.tm_hour = rtc_time[2];
	time.tm_min	 = rtc_time[1];
	time.tm_sec	 = rtc_time[0];
	stamp = mktime(&time);
	#if DEBUG_TEMP > 0
	debug_printf("PORT_GetSysTimestamp :%d/%d/%d %d:%d:%d stamp:%d\r\n",time.tm_year,time.tm_mon,time.tm_mday,time.tm_hour,\
				time.tm_min,time.tm_sec, stamp);
	#endif
	return stamp;
}
/********************************************************************************
**  函数名:     PORT_StampTotime
**  函数描述:   时间戳转时间
**  参数:       [in] stamp:时间戳
				[out]temp:转换的时间
**  返回:       
********************************************************************************/
BOOLEAN PORT_StampTotime(INT32U stamp, INT8U* temp)
{
	struct tm* nowtime;
	nowtime = localtime(&stamp);
	temp[6] = nowtime->tm_year-100;
	temp[5] = nowtime->tm_mon+1;
	temp[4] = nowtime->tm_mday;
	temp[2] = nowtime->tm_hour;
	temp[1] = nowtime->tm_min;
	temp[0] = nowtime->tm_sec;
	return TRUE;
}

/********************************************************************************
**  函数名:     PORT_SetSysTime
**  函数描述:   设置系统时间
**  参数:       [in] stime: 系统时间
**  返回:       成功返回true，失败返回false
********************************************************************************/
BOOLEAN PORT_SetSysTime(SYSTIME_T *dt)
{

	INT8U time[7] = {0},rtc_time[7], ret;
	INT16U temp;
	INT32U t1 = 0,t2 = 0;
	static INT8U hop_cnt = 0;	// 数值连续跳变次数>=3，修改时间
	// 如果设置全0为无效值，不做处理
	if (memcmp(time, (INT8U*)dt, 6) == 0) {
		#if DEBUG_TEMP > 0
		debug_printf("PORT_SetSysTime 时间为全0，无效值，不做处理\r\n");
		#endif
		return FALSE;
	}
		
	dal_rtc_settime((INT8U *)dt);
	//dal_rtc_gettime(time, 7);
	ret = HAL_sd2058_ReadCalendar(time);
	if (ret == TRUE) {
		rtc_time[0] = dt->time.second;	//秒
		rtc_time[1] = dt->time.minute;	//分
		rtc_time[2] = dt->time.hour;  	//时
		rtc_time[4] = dt->date.day;		//日
		rtc_time[5] = dt->date.month;  	//月
		rtc_time[6] = dt->date.year;  	//年
		t1 = PORT_GetSysTimestamp(rtc_time);
		t2 = PORT_GetSysTimestamp(time);
		temp = abs(t1- t2);
	} else {
		temp = 31;
	}
	if (temp > 30) {
		if (++hop_cnt >= 3) {
			hop_cnt = 0;
			rtc_time[0] = dt->time.second;	//秒
			rtc_time[1] = dt->time.minute;	//分
			rtc_time[2] = dt->time.hour;  	//时
			rtc_time[4] = dt->date.day;		//日
			rtc_time[5] = dt->date.month;  	//月
			rtc_time[6] = dt->date.year;  	//年
			ret = HAL_sd2058_SetCalendar(rtc_time);
			#if DEBUG_TEMP > 0
			debug_printf("HAL_sd2058_SetCalendar ret:%d\r\n",ret);
			#endif
		}
	} else {
		hop_cnt = 0;
	}
	#if DEBUG_TEMP > 0
	//ret = HAL_sd2058_ReadCalendar(time);
	debug_printf("settime:%d/%d/%d %d:%d:%d\r\ncurtime:%d/%d/%d %d:%d:%d %d hop_cnt:%d\r\n",dt->date.year, dt->date.month, dt->date.day,dt->time.hour, dt->time.minute, dt->time.second,
		time[6],time[5],time[4],time[2],time[1],time[0],time[3], hop_cnt);
	#endif
    return TRUE;
    // return SHELL_PF_SetLocalTime(class_obj[CLASS_ID_PLATFORM], (INT8U *)&systime);
}
BOOLEAN PORT_GetSysTime1(SYSTIME_T *dtime)
{
	INT8U time[7];
	if(HAL_sd2058_ReadCalendar(time) == FALSE){
		dal_rtc_gettime(time, 7);
		dtime->date.year  = time[0];
		dtime->date.month = time[1];
		dtime->date.day   = time[2];

		dtime->time.hour   = time[3];
		dtime->time.minute = time[4];
		dtime->time.second = time[5];
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
BOOLEAN PORT_GetSysTime(SYSTIME_T *dtime)
{
	INT8U time[7];
	if(HAL_sd2058_ReadCalendar(time) == FALSE){
		return FALSE;
	}
	dtime->date.year  = time[6];
	dtime->date.month = time[5];
	dtime->date.day   = time[4];

	dtime->time.hour   = time[2];
	dtime->time.minute = time[1];
	dtime->time.second = time[0];
    return TRUE;
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
** 参数:       wktime:休眠唤醒时间
** 返回:       无
********************************************************************************/
void PORT_Sleep(INT16U (*hook)(void), INT16U wktime)
{
	Dal_McuLowPower(hook, wktime);
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

