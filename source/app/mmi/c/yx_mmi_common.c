/********************************************************************************
**
** 文件名:     yx_mmi_common.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现基础业务功能
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2014/05/22 | 叶德焰 |  创建第一版本
********************************************************************************/
#include "yx_include.h"
#include "yx_dym_drv.h"
#include "st_rtc_drv.h"
#include "dal_gpio_cfg.h"
#include "yx_debug.h"

#if EN_MMI > 0
#include "yx_mmi_drv.h"

/*
********************************************************************************
* 定义模块配置参数
********************************************************************************
*/
typedef enum {
    STEP_POWEROFF = 0,
    STEP_LOW,
    STEP_HIGH,
    STEP_MAX
} STEP_E;

#define PERIOD_DELAY          _SECOND, 2
#define PERIOD_LOW            _MILTICK, 3

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/




/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/



/*******************************************************************
** 函数名:     SendAck
** 函数描述:   发送应答
** 参数:       [in]cmd:    命令编码
**             [in]type:   应答类型
** 返回:       无
********************************************************************/
static void SendAck(INT8U cmd, INT8U type)
{
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteBYTE_Strm(wstrm, type);
    YX_MMI_ListSend(cmd, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 0, 0, 0);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_CTL_GPIO
** 函数描述:   控制GPIO输出
** 参数:       [in] cmd:    命令编码
**             [in] data:   数据指针
**             [in] datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_CMD_CTL_GPIO(INT8U cmd, INT8U *data, INT16U datalen)
{
    INT8U i, num, ctl, type;
    STREAM_T rstrm;
    
    if (cmd != DN_PE_CMD_CTL_GPIO) {
        return;
    }
    
    #if DEBUG_MMI > 0
    printf_com("<收到GPIO控制请求命令:");
    printf_hex(data, datalen);
    printf_com(">\r\n");
    #endif
    
    YX_InitStrm(&rstrm, data, datalen);
    
    num = YX_ReadBYTE_Strm(&rstrm);
    for (i = 0; i < num; i++) {
        type = YX_ReadBYTE_Strm(&rstrm);
        ctl  = YX_ReadBYTE_Strm(&rstrm);
        
        switch (type)
        {
        case 0x01:                                                             /* 手机模块电源控制 */
            if (ctl == 0x01) {
                DAL_GPIO_PullupGsmPower();
            } else {
                DAL_GPIO_PulldownGsmPower();
            }
            break;
        case 0x02:                                                             /* 手机模块开关机控制 */
            if (ctl == 0x01) {
                DAL_GPIO_PullupGsmOnOff();
            } else {
                DAL_GPIO_PulldownGsmOnOff();
            }
            break;
        case 0x03:                                                             /* 脉冲通道选择1 */
            if (ctl == 0x01) {
                DAL_GPIO_PullupPulseCh1();
            } else {
                DAL_GPIO_PulldownPulseCh1();
            }
            break;
        case 0x04:                                                             /* 脉冲通道选择2 */
            if (ctl == 0x01) {
                DAL_GPIO_PullupPulseCh2();
            } else {
                DAL_GPIO_PulldownPulseCh2();
            }
            break;
        case 0x05:                                                             /* 外围电路省电控制 */
            if (ctl == 0x01) {
                DAL_GPIO_PullupPowerSave();
            } else {
                //DAL_GPIO_PulldownPowerSave();
            }
            break;
        default:
            break;
        }
    }
        
    SendAck(UP_PE_ACK_CTL_GPIO, PE_ACK_MMI);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_SET_REALCLOCK
** 函数描述:   主机设置实时时钟
** 参数:       [in] cmd:    命令编码
**             [in] data:   数据指针
**             [in] datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_CMD_SET_REALCLOCK(INT8U cmd, INT8U *data, INT16U datalen)
{
    INT8U result, weekday;
    STREAM_T rstrm;
    SYSTIME_T systime;
    
    if (cmd != DN_PE_CMD_SET_REALCLOCK) {
        return;
    }
    
    #if EN_DEBUG > 0
    printf_com("<主机设置实时时钟:");
    printf_hex(data, datalen);
    printf_com(">\r\n");
    #endif

    YX_InitStrm(&rstrm, data, datalen);
    
    systime.date.year   = YX_ReadBYTE_Strm(&rstrm);
    systime.date.month  = YX_ReadBYTE_Strm(&rstrm);
    systime.date.day    = YX_ReadBYTE_Strm(&rstrm);
    
    systime.time.hour   = YX_ReadBYTE_Strm(&rstrm);
    systime.time.minute = YX_ReadBYTE_Strm(&rstrm);
    systime.time.second = YX_ReadBYTE_Strm(&rstrm);
    
    weekday = YX_ReadBYTE_Strm(&rstrm);
    if (weekday < 1 || weekday > 7) {
        weekday = 1;
    }
    
    result = ST_RTC_OpenRtcFunction(RTC_CLOCK_LSE);
    if (result) {
        result = ST_RTC_SetSystime(&systime.date, &systime.time, weekday);
    }
    
    if (result) {
        SendAck(UP_PE_ACK_SET_REALCLOCK, PE_ACK_MMI);
    } else {
        SendAck(UP_PE_ACK_SET_REALCLOCK, PE_NAK_MMI);
    }
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_ACK_REPORT_REALCLOCK
** 函数描述:   上报实时时钟应答
** 参数:       [in] cmd:    命令编码
**             [in] data:   数据指针
**             [in] datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_ACK_REPORT_REALCLOCK(INT8U cmd, INT8U *data, INT16U datalen)
{
    if (cmd != DN_PE_ACK_REPORT_REALCLOCK) {
        return;
    }
    
    #if DEBUG_MMI > 0
    printf_com("<上报实时时钟应答>\r\n");
    #endif
    
    YX_MMI_ListAck(UP_PE_CMD_REPORT_REALCLOCK, _SUCCESS);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_READ_REALCLOCK
** 函数描述:   主机读取实时时钟
** 参数:       [in] cmd:    命令编码
**             [in] data:   数据指针
**             [in] datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_CMD_READ_REALCLOCK(INT8U cmd, INT8U *data, INT16U datalen)
{
    INT8U result, weekday;
    INT32U subsecond;
    STREAM_T *wstrm;
    SYSTIME_T systime;
    
    if (cmd != DN_PE_CMD_READ_REALCLOCK) {
        return;
    }
    
    #if DEBUG_MMI > 0
    printf_com("<主机读取实时时钟>\r\n");
    #endif
    
    result = ST_RTC_GetSystime(&systime.date, &systime.time, &weekday, &subsecond);
    if (result) {
        wstrm = YX_STREAM_GetBufferStream();
        YX_WriteBYTE_Strm(wstrm, PE_ACK_MMI);
        
        YX_WriteBYTE_Strm(wstrm, systime.date.year);
        YX_WriteBYTE_Strm(wstrm, systime.date.month);
        YX_WriteBYTE_Strm(wstrm, systime.date.day);
        
        YX_WriteBYTE_Strm(wstrm, systime.time.hour);
        YX_WriteBYTE_Strm(wstrm, systime.time.minute);
        YX_WriteBYTE_Strm(wstrm, systime.time.second);
        
        YX_WriteBYTE_Strm(wstrm, weekday);
        if (ST_RTC_BattleIsNormal()) {                                     /* RTC电池是否正常 */
            YX_WriteBYTE_Strm(wstrm, 0x01);
        } else {
            YX_WriteBYTE_Strm(wstrm, 0x02);
        }
        
        YX_MMI_DirSend(UP_PE_ACK_READ_REALCLOCK, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm));
    } else {
        SendAck(UP_PE_ACK_READ_REALCLOCK, PE_NAK_MMI);
    }
}


static FUNCENTRY_MMI_T s_functionentry[] = {
                       DN_PE_CMD_CTL_GPIO,                 HdlMsg_DN_PE_CMD_CTL_GPIO          /* 主机控制GPIO输出 */
                      ,DN_PE_CMD_SET_REALCLOCK,            HdlMsg_DN_PE_CMD_SET_REALCLOCK     /* 主机设置实时时钟 */
                      ,DN_PE_ACK_REPORT_REALCLOCK,         HdlMsg_DN_PE_ACK_REPORT_REALCLOCK  /* 上报实时时钟应答 */
                      ,DN_PE_CMD_READ_REALCLOCK,           HdlMsg_DN_PE_CMD_READ_REALCLOCK    /* 主机读取实时时钟 */
                      
                                         };
                                         
static INT8U s_funnum = sizeof(s_functionentry)/sizeof(s_functionentry[0]);



/*******************************************************************
** 函数名:     YX_MMI_InitCommon
** 函数描述:   功能初始化
** 参数:       无
** 返回:       无
********************************************************************/
void YX_MMI_InitCommon(void)
{
    INT8U i;

    for (i = 0; i < s_funnum; i++) {
        YX_MMI_Register(s_functionentry[i].cmd, s_functionentry[i].entryproc);
    }
    DAL_GPIO_SelectOdometerPulseFromEXT();
}

/***************************************************************
** 函数名:    YX_MMI_SendRealClock
** 功能描述:  上报实时时钟
** 参数:  	  无
** 返回值:    成功返回TRUE,失败返回FALSE
***************************************************************/
BOOLEAN YX_MMI_SendRealClock(void)
{
    INT8U result, weekday;
    INT32U subsecond;
    STREAM_T *wstrm;
    SYSTIME_T systime;
    
    if (!YX_MMI_IsON()) {
        return false;
    }
    
    result = ST_RTC_GetSystime(&systime.date, &systime.time, &weekday, &subsecond);
    if (result) {
        wstrm = YX_STREAM_GetBufferStream();
        
        YX_WriteBYTE_Strm(wstrm, systime.date.year);
        YX_WriteBYTE_Strm(wstrm, systime.date.month);
        YX_WriteBYTE_Strm(wstrm, systime.date.day);
    
        YX_WriteBYTE_Strm(wstrm, systime.time.hour);
        YX_WriteBYTE_Strm(wstrm, systime.time.minute);
        YX_WriteBYTE_Strm(wstrm, systime.time.second);
    
        YX_WriteBYTE_Strm(wstrm, weekday);
        if (ST_RTC_BattleIsNormal()) {                                     /* RTC电池是否正常 */
            YX_WriteBYTE_Strm(wstrm, 0x01);
        } else {
            YX_WriteBYTE_Strm(wstrm, 0x02);
        }
        
        YX_MMI_ListAck(UP_PE_CMD_REPORT_REALCLOCK, _SUCCESS);
        result = YX_MMI_DirSend(UP_PE_CMD_REPORT_REALCLOCK, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm));
    }
    return result;
}

/***************************************************************
** 函数名:    YX_MMI_QueryCommonPara
** 功能描述:  发送查询通用参数
** 参数:  	  [in] type: 参数类型, 见 PARA_E
**        	  [in] fp:   查询结果回调函数
** 返回值:    成功返回TRUE,失败返回FALSE
***************************************************************/
BOOLEAN YX_MMI_QueryCommonPara(INT8U type, void(*fp)(INT8U result))
{
    STREAM_T *wstrm;
    
    if (!YX_MMI_IsON()) {
        return false;
    }
        
    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteBYTE_Strm(wstrm, 0x01);                                        /* 参数个数 */
    YX_WriteBYTE_Strm(wstrm, type);
    return YX_MMI_ListSend(UP_PE_CMD_SLAVE_GET_PARA, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 3, 3, fp);
}

#endif


