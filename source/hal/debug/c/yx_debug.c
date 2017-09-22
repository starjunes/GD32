/******************************************************************************
**
** Filename:     yx_debug.c
** Copyright:    
** Description:  该模块主要实现调试信息数据输出
**
-------------------------------------------------------------------------------
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#include "hal_include.h"
#include "yx_dym_drv.h"
#include "yx_config.h"
#include "yx_debug.h"
#include "st_rtc_drv.h"
#include "yx_mmi_drv.h"



//#if EN_DEBUG > 0

/*
********************************************************************************
* define config parameters
********************************************************************************
*/

/*
********************************************************************************
* define struct
********************************************************************************
*/

/*
********************************************************************************
* define module variants
********************************************************************************
*/



/*-------------------------------------------------------------------
** 函数名称:   printf_com
** 函数描述:   通用打印函数，与printf类似
** 参数:       [in]  fmt: 打印参数
** 返回:       成功返回true,失败返回false
-------------------------------------------------------------------*/
BOOLEAN printf_com(const char *fmt, ...)
{
#if 0
return true;
#else
    BOOLEAN result;
    va_list ap;
    INT16U len;
    INT8U sendbuf[128];
    INT8U weekday;
    INT32U subsecond;
    SYSTIME_T systime;

    #if EN_DEBUG == 0
    if (!YX_MMI_GetLogFlag()) {
        return FALSE;
    }
    #endif

    if (!(fmt[0] == '\r' || fmt[0] == '\n' || fmt[1] == '\r' || fmt[1] == '\n')) {
        result = ST_RTC_GetSystime(&systime.date, &systime.time, &weekday, &subsecond);
        if (result) {
            len = sprintf((char *)sendbuf, "%d-%2d %2d:%2d:%2d ", systime.date.month, systime.date.day, 
                          systime.time.hour, systime.time.minute, systime.time.second);
            DEBUG_UART_WriteBlock(UART_COM_DEBUG, sendbuf, len);
        }
    }
    
    YX_VA_START(ap, fmt);
    len = YX_VSPRINTF((char *)sendbuf, fmt, ap);
    YX_VA_END(ap);
    
    if (len > sizeof(sendbuf)) {
        OS_ASSERT(0, RETURN_FALSE);
    }
    
    result = DEBUG_UART_WriteBlock(UART_COM_DEBUG, sendbuf, len);
	return result;
#endif
}

/*-------------------------------------------------------------------
** 函数名称:   printf_hex
** 函数描述:   将缓冲区中的数据转换成可见字符输出
** 参数:       [in]  ptr: 数据指针
**             [in]  len: 数据长度
** 返回:       成功返回true,失败返回false
-------------------------------------------------------------------*/
BOOLEAN printf_hex(INT8U *ptr, INT16U len)
{
#if 0
return true;
#else
    BOOLEAN result;
    INT8U *memptr;
    INT16U i;
    INT8U  ch;

    #if EN_DEBUG == 0
    if (!YX_MMI_GetLogFlag()) {
        return FALSE;
    }
    #endif
    
    if (ptr == 0 || len == 0) {
        return false;
    }
    
    memptr = YX_DYM_AllocEx(len * 3 + 1, DYM_OT_5S);
    if (memptr == 0) {
        return false;
    }

    for (i = 0; len > 0; len--) {
        ch = *ptr++;
        memptr[i++] = YX_HexToChar((ch >> 4) & 0x0F);
        memptr[i++] = YX_HexToChar(ch & 0x0F);
        memptr[i++] = ' ';
    }
    result = DEBUG_UART_WriteBlock(UART_COM_DEBUG, memptr, i);
    YX_DYM_Free(memptr);
        
	return result;
#endif
}

/*-------------------------------------------------------------------
** 函数名称:   printf_raw
** 函数描述:   将原始数据直接通过串口输出
** 参数:       [in]  ptr: 数据指针
**             [in]  len: 数据长度
** 返回:       成功返回true,失败返回false
-------------------------------------------------------------------*/
BOOLEAN printf_raw(INT8U *ptr, INT16U len)
{
    #if EN_DEBUG == 0
    if (!YX_MMI_GetLogFlag()) {
        return FALSE;
    }
    #endif

#if 0
return true;
#else
    if (ptr == 0 || len == 0) {
        return false;
    }
    
    return DEBUG_UART_WriteBlock(UART_COM_DEBUG, ptr, len);
#endif
}

/*-------------------------------------------------------------------
** 函数名称:   printf_irq
** 函数描述:   中断打印接口,将原始数据直接通过串口输出
** 参数:       [in]  ptr: 数据指针
**             [in]  len: 数据长度
** 返回:       成功返回true,失败返回false
-------------------------------------------------------------------*/
BOOLEAN printf_irq(INT8U *ptr, INT16U len)
{
    BOOLEAN result = 0;
    INT16U i;

    #if EN_DEBUG == 0
    if (!YX_MMI_GetLogFlag()) {
        return FALSE;
    }
    #endif
    
    if (ptr == 0 || len == 0) {
        return false;
    }
    
    for (i = 0; i < len; i++) {
        result = DEBUG_UART_SendChar(UART_COM_DEBUG, ptr[i]);
        if (!result) {
            break;
        }
    }
    return result;
}

//#endif


