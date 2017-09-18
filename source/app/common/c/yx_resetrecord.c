/********************************************************************************
**
** 文件名:     yx_resetrecord.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现复位信息记录和统计功能
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2014/08/26 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "yx_include.h"
#include "st_rtc_drv.h"
#include "dal_pp_drv.h"
#include "yx_resetrecord.h"
#include "yx_debug.h"

/*
*****************************************************************
*   宏定义
*****************************************************************
*/


/*
*****************************************************************
*   静态变量定义
*****************************************************************
*/



/*******************************************************************
** 函数名:     FillResetRecordInfo
** 函数描述:   存储信息
** 参数:       [in] file： 文件名
**             [in] line： 出错行号
** 返回:       无
********************************************************************/
static void FillResetRecordInfo(RESET_RECORD_T *resetrecord, char *file, INT32U line)
{
    INT8U i, len, weekday;
    INT32U subsecond;
    
    for (i = MAX_RESET_RECORD - 1; i > 0; i--) {
        YX_MEMCPY(&resetrecord->rst_record[i], sizeof(RESET_T), &resetrecord->rst_record[i - 1], sizeof(RESET_T));
    }
    
    ST_RTC_GetSystime(&resetrecord->rst_record[0].systime.date, &resetrecord->rst_record[0].systime.time, &weekday, &subsecond);
    
    len = sizeof(resetrecord->rst_record[0].file) - 1;
    YX_MEMCPY(resetrecord->rst_record[0].file, len, file, len);
    resetrecord->rst_record[0].file[len] = '\0';
    resetrecord->rst_record[0].line = line;
}

/*******************************************************************
** 函数名:     ResetInform
** 函数描述:   复位回调接口
** 参数:       [in] event：复位类型
**             [in] file： 文件名
**             [in] line： 出错行号
** 返回:       无
********************************************************************/
static void ResetInform(INT8U event, char *file, INT32U line)
{
    RESET_RECORD_T resetrecord;
    
    if (event == RESET_EVENT_INITIATE) {
        if (DAL_PP_ReadParaByID(PP_RESET_, (INT8U *)&resetrecord, sizeof(resetrecord))) {
            resetrecord.rst_int++;
            resetrecord.rst_total = resetrecord.rst_int + resetrecord.rst_ext + resetrecord.rst_err - 1;
        } else {
            resetrecord.rst_int  = 1;
            resetrecord.rst_ext  = 0;
            resetrecord.rst_err  = 0;
            resetrecord.rst_total= 0;    // 与总和差1才能在下次启动时判断出不是外部复位
        }
        FillResetRecordInfo(&resetrecord, file, line);
    } else if (event == RESET_EVENT_ERR) {
        if (DAL_PP_ReadParaByID(PP_RESET_, (INT8U *)&resetrecord, sizeof(resetrecord))) {
            resetrecord.rst_err++;
            resetrecord.rst_total = resetrecord.rst_int + resetrecord.rst_ext + resetrecord.rst_err - 1;
        } else {
            resetrecord.rst_int  = 0;
            resetrecord.rst_ext  = 0;
            resetrecord.rst_err  = 1;
            resetrecord.rst_total= 0;   // 与总和差1才能在下次启动时判断出不是外部复位
        }
        FillResetRecordInfo(&resetrecord, file, line);
    } else {
        return;
    }
    DAL_PP_StoreParaByID(PP_RESET_, (INT8U *)&resetrecord, sizeof(RESET_RECORD_T));
}

/*******************************************************************
** 函数名:     YX_InitResetRecord
** 函数描述:   初始化模块
** 参数:       无
** 返回:       无
********************************************************************/
void YX_InitResetRecord(void)
{
    INT32U total;
    RESET_RECORD_T resetrecord;
    
    DAL_PP_ReadParaByID(PP_RESET_, (INT8U *)&resetrecord, sizeof(RESET_RECORD_T));

    total = resetrecord.rst_int + resetrecord.rst_ext + resetrecord.rst_err;
    if (total == resetrecord.rst_total + 1) {                              /* 说明是已知的复位, 而不是外部的复位 */
        resetrecord.rst_total++;
    } else if (total == resetrecord.rst_total) {                           /* 说明是外部复位或者未知原因复位 */
        resetrecord.rst_total++;
        resetrecord.rst_ext++;
        FillResetRecordInfo(&resetrecord, "unknow", 0);
    } else {                                                               /* 说明统计和异常,重新加之 */
        resetrecord.rst_total = total;
    }
    DAL_PP_StoreParaByID(PP_RESET_, (INT8U *)&resetrecord, sizeof(RESET_RECORD_T));
    
    #if DEBUG_ERR > 0
    {
    INT32U i;
    
    printf_com("\r\n总复位次数(%d)=主动(%d)+出错(%d)+外部(%d)\r\n", 
                 resetrecord.rst_total, resetrecord.rst_int, resetrecord.rst_err, resetrecord.rst_ext);
    for (i = 0; i < MAX_RESET_RECORD; i++) {
        printf_com("No.%d:time", i);
        printf_hex((INT8U *)&resetrecord.rst_record[i].systime, sizeof(resetrecord.rst_record[i].systime));
        printf_com(",file(%s),line(%d)\r\n", resetrecord.rst_record[i].file, resetrecord.rst_record[i].line);
    }
    }
    #endif
   
    OS_RegistResetInform(RESET_PRI_0, ResetInform);
}



