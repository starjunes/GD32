/********************************************************************************
**
** 文件名:     yx_mmi_sensor.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现外设传感器采集管理
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
#include "st_adc_drv.h"
#include "dal_input_drv.h"
#include "dal_pulse_drv.h"
#include "dal_hit_drv.h"
#include "yx_sleep.h"
#include "yx_debug.h"

#if EN_MMI > 0
#include "yx_mmi_drv.h"

/*
********************************************************************************
* 定义模块配置参数
********************************************************************************
*/

#define PERIOD_SCAN           _MILTICK, 1

#define KEY_ACK              0x0F
#define KEY_UP               0x0D
#define KEY_DOWN             0x0C
#define KEY_NAK              0x0E

#define KEY_LONG_TIME        2000      /* 长按时间,单位：ms */
#define KEY_ATTRIB_SHORT     0x01      /* 单击 */  
#define KEY_ATTRIB_LONG      0x02      /* 长按 */

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/

typedef struct {
    INT32U sensor;                /* 传感器状态 */
    INT16U ct_gpio;
    INT8U  gpio_onoff;            /* 主动上报开关 */
    INT16U gpio_period;           /* 主动上报时间间隔 */
    
    INT16U ct_rtc;                /* RTC上报计数器 */
    
    INT8U  pulse_onoff;           /* 里程脉冲开关 */
    INT16U pulse_period;          /* 里程脉冲采集周期 */
    INT8U  pulse_pkt;             /* 里程脉冲打包个数*/
    
    INT8U  ct_ad[ADC_CH_MAX];
    INT8U  ad_onoff[ADC_CH_MAX];
    INT16U ad_period[ADC_CH_MAX];
    INT8U  ad_pkt[ADC_CH_MAX];
    
    INT8U  acceleration;          /* 加速度 */
    INT16U sensitivity;           /* 碰撞灵敏度，ms */
    INT8U  hit_onoff;             /* 开关 */
    INT8U  hit_period;            /* 上报周期，秒 */
    INT8U  angle;                 /* 侧翻角度 */
    INT8U  ct_lockrob;            /* 锁住抢劫报警信号时间 */
    
    INT8U  key_time[4];           /* 按键起始时间 */
} TCB_T;
/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
static INT8U s_scantmr;
static TCB_T s_tcb;


/*******************************************************************
** 函数名:     GetSensorStatus
** 函数描述:   获取传感器状态
** 参数:       无
** 返回:       无
********************************************************************/
static INT32U GetSensorStatus(void)
{
    LONGWORD_UNION senstatus;

    senstatus.ulong = 0;
    
    /* 第1字节 */
    if (DAL_INPUT_ReadInstantStatus(IPT_DOOR))       senstatus.bytes.byte1 |= 0x80;   /* 车门 */
    if (DAL_INPUT_ReadInstantStatus(IPT_LLAMP))      senstatus.bytes.byte1 |= 0x20;   /* 左转向灯 */
    if (DAL_INPUT_ReadInstantStatus(IPT_RLAMP))      senstatus.bytes.byte1 |= 0x10;   /* 右转向灯*/
    if (DAL_INPUT_ReadInstantStatus(IPT_FBRAKE))     senstatus.bytes.byte1 |= 0x04;   /* 脚刹 */
    
    /* 第2字节 */
    if (DAL_INPUT_ReadInstantStatus(IPT_ACC))        senstatus.bytes.byte2 |= 0x80;   /* acc */
    
    /* 第3字节 */
    if (DAL_INPUT_ReadInstantStatus(IPT_GPSOPEN))    senstatus.bytes.byte3 |= 0x80;   /* 天线开路 */
    if (DAL_INPUT_ReadInstantStatus(IPT_GPSSHORT))   senstatus.bytes.byte3 |= 0x40;   /* 天线短路 */
    if (DAL_INPUT_ReadFilterStatus(IPT_POWDECT))     senstatus.bytes.byte3 |= 0x20;   /* 断电检测 */
    if (DAL_INPUT_ReadInstantStatus(IPT_LAMP))       senstatus.bytes.byte3 |= 0x10;   /* 车灯 */
    if (DAL_INPUT_ReadInstantStatus(IPT_FLAMP))      senstatus.bytes.byte3 |= 0x08;   /* 远光灯 */
    if (DAL_INPUT_ReadInstantStatus(IPT_NLAMP))      senstatus.bytes.byte3 |= 0x04;   /* 近光灯 */
    if (DAL_INPUT_ReadInstantStatus(IPT_ROB))        senstatus.bytes.byte3 |= 0x02;   /* 抢劫 */
    if (YX_SLEEP_GetWakeupEvent() == WAKEUP_EVENT_ROB) {                              /* 抢劫报警唤醒,需要锁住 */
        if (s_tcb.ct_lockrob > 0) {
            --s_tcb.ct_lockrob;
            senstatus.bytes.byte3 |= 0x02;
        }
    }

    /* 第4字节 */
    
    return senstatus.ulong;
}

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
** 函数名:     HdlMsg_DN_PE_ACK_REPORT_SENSOR_STATUS
** 函数描述:   主动上报GPIO传感器状态应答
** 参数:       [in]cmd:    命令编码
**             [in]data:   数据指针
**             [in]datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_ACK_REPORT_SENSOR_STATUS(INT8U cmd, INT8U *data, INT16U datalen)
{
    YX_MMI_ListAck(UP_PE_CMD_REPORT_SENSOR_STATUS, _SUCCESS);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_SET_SENSOR_FILTER
** 函数描述:   设置GPIO滤波参数
** 参数:       [in]cmd:    命令编码
**             [in]data:   数据指针
**             [in]datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_CMD_SET_SENSOR_FILTER(INT8U cmd, INT8U *data, INT16U datalen)
{
    SendAck(UP_PE_ACK_SET_SENSOR_FILTER, PE_ACK_MMI);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_SET_SENSOR_PARA
** 函数描述:   设置GPIO传感器参数
** 参数:       [in]cmd:    命令编码
**             [in]data:   数据指针
**             [in]datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_CMD_SET_SENSOR_PARA(INT8U cmd, INT8U *data, INT16U datalen)
{
    if ((cmd != DN_PE_CMD_SET_SENSOR_PARA) ||(data == 0) || (datalen == 0)) {
        return;
    }
    
    s_tcb.gpio_onoff   = data[0];
    s_tcb.gpio_period  = (data[1] << 8) + data[2];
    
    #if DEBUG_MMI > 0
    printf_com("<设置GPIO传感器参数, 开关(0x%x), 周期(%d)>\r\n", s_tcb.gpio_onoff, s_tcb.gpio_period);
    #endif
    
    SendAck(UP_PE_ACK_SET_SENSOR_PARA, PE_ACK_MMI);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_ACK_REPORT_ODOPULSE
** 函数描述:   主动上报里程脉冲应答
** 参数:       [in]cmd:    命令编码
**             [in]data:   数据指针
**             [in]datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_ACK_REPORT_ODOPULSE(INT8U cmd, INT8U *data, INT16U datalen)
{
    if (data[0] == PE_ACK_MMI) {
        YX_MMI_ListAck(UP_PE_CMD_REPORT_ODOPULSE, _SUCCESS);
    }
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_SET_ODOPULSE_PARA
** 函数描述:   设置里程脉冲参数
** 参数:       [in]cmd:    命令编码
**             [in]data:   数据指针
**             [in]datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_CMD_SET_ODOPULSE_PARA(INT8U cmd, INT8U *data, INT16U datalen)
{
    if (cmd != DN_PE_CMD_SET_ODOPULSE_PARA) {
        return;
    }
    
    s_tcb.pulse_onoff   = data[0];
    s_tcb.pulse_period  = (data[1] << 8) + data[2];
    s_tcb.pulse_pkt     = data[3];
    
    if (s_tcb.pulse_onoff == 0x01 && s_tcb.pulse_period != 0 && s_tcb.pulse_pkt != 0) {/* 间隔上报里程脉冲 */
        DAL_PULSE_OpenPulseCalFunction(s_tcb.pulse_period);
    } else {
        if (DAL_PULSE_IsOpen()) {
            DAL_PULSE_ClosePulseCalFunction();
        }
    }
    
    #if DEBUG_MMI > 0
    printf_com("<设置里程脉冲参数, 开关(0x%x), 周期(%d), 打包数(%d)>\r\n", s_tcb.pulse_onoff, s_tcb.pulse_period, s_tcb.pulse_pkt);
    #endif
    
    SendAck(UP_PE_ACK_SET_ODOPULSE_PARA, PE_ACK_MMI);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_SET_AD_PARA
** 函数描述:   设置AD采集参数
** 参数:       [in]cmd:    命令编码
**             [in]data:   数据指针
**             [in]datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_CMD_SET_AD_PARA(INT8U cmd, INT8U *data, INT16U datalen)
{
    INT8U i, num, ch, ad_onoff, ad_pkt;
    INT16U ad_period;
    STREAM_T rstrm;
    
    if (cmd != DN_PE_CMD_SET_AD_PARA) {
        return;
    }
    
    YX_InitStrm(&rstrm, data, datalen);
    num = YX_ReadBYTE_Strm(&rstrm);
    
    #if DEBUG_MMI > 0
    printf_com("<设置AD采集参数, 通道数(%d):\r\n", num);
    #endif
    
    for (i = 0; i < num; i++) {
        ch         = YX_ReadBYTE_Strm(&rstrm);                                 /* 通道号 */
        ad_onoff   = YX_ReadBYTE_Strm(&rstrm);                                 /* 开关 */
        ad_period  = YX_ReadHWORD_Strm(&rstrm);                                /* 采集周期 */
        ad_pkt     = YX_ReadBYTE_Strm(&rstrm);                                 /* 打包数 */
        
        if (ch <= ADC_CH_MAX) {
            s_tcb.ad_onoff[ch - 1]   = ad_onoff;
            s_tcb.ad_period[ch - 1]  = ad_period;
            s_tcb.ad_pkt[ch - 1]     = ad_pkt;
        }
        
        #if DEBUG_MMI > 0
        printf_com("(%d)(%d)(%d)(%d)\r\n", ch, ad_onoff, ad_period, ad_pkt);
        #endif
    }
    
    SendAck(UP_PE_ACK_SET_AD_PARA, PE_ACK_MMI);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_GET_AD
** 函数描述:   获取AD值
** 参数:       [in]cmd:    命令编码
**             [in]data:   数据指针
**             [in]datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_CMD_GET_AD(INT8U cmd, INT8U *data, INT16U datalen)
{
    INT8U i, num, channel;
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    
    num = data[0];
    
    #if DEBUG_MMI > 0
    printf_com("<获取AD值请求, 通道数(%d)>\r\n", num);
    #endif
    
    YX_WriteBYTE_Strm(wstrm, num);
    for (i = 0; i < num; i++) {
        channel = data[i + 1];
        YX_WriteBYTE_Strm(wstrm, channel);
        YX_WriteBYTE_Strm(wstrm, 0x01);
        YX_WriteHWORD_Strm(wstrm, ST_ADC_GetValue(channel - 1));
    }
    
    YX_MMI_ListSend(UP_PE_ACK_GET_AD, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 0, 0, 0);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_ACK_REPORT_AD
** 函数描述:   主动上报AD值应答
** 参数:       [in]cmd:    命令编码
**             [in]data:   数据指针
**             [in]datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_ACK_REPORT_AD(INT8U cmd, INT8U *data, INT16U datalen)
{
    if (data[0] == PE_ACK_MMI) {
        YX_MMI_ListAck(UP_PE_CMD_REPORT_AD, _SUCCESS);
    }
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_ACK_REPORT_KEY
** 函数描述:   主动上报按键值应答
** 参数:       [in]cmd:    命令编码
**             [in]data:   数据指针
**             [in]datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_ACK_REPORT_KEY(INT8U cmd, INT8U *data, INT16U datalen)
{
    #if DEBUG_MMI > 0
    printf_com("<按键信号上报应答>\r\n");
    #endif
    //if (data[0] == PE_ACK_MMI) {
        YX_MMI_ListAck(UP_PE_CMD_REPORT_KEY, _SUCCESS);
    //}
}

#if EN_GSENSOR > 0
/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_HITCK_DMC_START
** 函数描述:   启动碰撞检测标定请求
** 参数:       [in]cmd:    命令编码
**             [in]data:   数据指针
**             [in]datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_CMD_HITCK_DMC_START(INT8U cmd, INT8U *data, INT16U datalen)
{
    #if DEBUG_MMI > 0
    printf_com("<启动碰撞侧翻标定, ctl(%d)>\r\n", data[0]);
    #endif
    
    if (DAL_HIT_StartCalibration()) {
        SendAck(UP_PE_ACK_HITCK_DMC_START, PE_ACK_MMI);
    } else {
        SendAck(UP_PE_ACK_HITCK_DMC_START, PE_NAK_MMI);
    }
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_HITCK_DMC_STOP
** 函数描述:   停止碰撞检测标定请求
** 参数:       [in]cmd:    命令编码
**             [in]data:   数据指针
**             [in]datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_CMD_HITCK_DMC_STOP(INT8U cmd, INT8U *data, INT16U datalen)
{
    #if DEBUG_MMI > 0
    printf_com("<停止碰撞侧翻标定, ctl(%d)>\r\n", data[0]);
    #endif
    
    if (DAL_HIT_StopCalibration()) {
        SendAck(UP_PE_ACK_HITCK_DMC_STOP, PE_ACK_MMI);
    } else {
        SendAck(UP_PE_ACK_HITCK_DMC_STOP, PE_NAK_MMI);
    }
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_CMD_HITCK_PARA_SET
** 函数描述:   碰撞检测参数设置请求
** 参数:       [in]cmd:    命令编码
**             [in]data:   数据指针
**             [in]datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_CMD_HITCK_PARA_SET(INT8U cmd, INT8U *data, INT16U datalen)
{
    if (cmd != DN_PE_CMD_HITCK_PARA_SET) {
        return;
    }
    
    s_tcb.acceleration = data[0];                                              /* 加速度 */
    s_tcb.sensitivity  = (data[1] << 8) + data[2];                             /* 碰撞灵敏度 */
    s_tcb.hit_onoff    = data[3];                                              /* 开关 */
    s_tcb.hit_period   = data[4];                                              /* 上报周期，秒 */
    s_tcb.angle        = data[5];                                              /* 侧翻角度 */
    
    #if DEBUG_MMI > 0
    printf_com("<碰撞检测参数设置请求(%d)(%d)(%d)(%d)(%d)>\r\n", s_tcb.acceleration, s_tcb.sensitivity, s_tcb.hit_onoff, s_tcb.hit_period, s_tcb.angle);
    #endif
    
    if (DAL_HIT_SetGsensorPara(s_tcb.acceleration, s_tcb.sensitivity, s_tcb.angle, s_tcb.hit_onoff, s_tcb.hit_period)) {
        SendAck(UP_PE_ACK_HITCK_PARA_SET, PE_ACK_MMI);
    } else {
        SendAck(UP_PE_ACK_HITCK_PARA_SET, PE_NAK_MMI);
    }
    YX_MMI_SendGsensorInfo(DAL_HIT_GetGsensorStatus());
}

/*******************************************************************
** 函数名:     HdlMsg_DN_PE_ACK_HITCK_REPORT
** 函数描述:   碰撞检测信号上报
** 参数:       [in]cmd:    命令编码
**             [in]data:   数据指针
**             [in]datalen:数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_DN_PE_ACK_HITCK_REPORT(INT8U cmd, INT8U *data, INT16U datalen)
{
    #if DEBUG_MMI > 0
    printf_com("<碰撞检测信号上报应答(%d)>\r\n", data[0]);
    #endif
    
    YX_MMI_ListAck(UP_PE_CMD_HITCK_REPORT, _SUCCESS);
}
#endif

static FUNCENTRY_MMI_T s_functionentry[] = {
     DN_PE_ACK_REPORT_SENSOR_STATUS,        HdlMsg_DN_PE_ACK_REPORT_SENSOR_STATUS     // GPIO传感器信号上报
    ,DN_PE_CMD_SET_SENSOR_FILTER,           HdlMsg_DN_PE_CMD_SET_SENSOR_FILTER        // GPIO滤波设置
    ,DN_PE_CMD_SET_SENSOR_PARA,             HdlMsg_DN_PE_CMD_SET_SENSOR_PARA          // GPIO参数设置
       
    ,DN_PE_ACK_REPORT_ODOPULSE,             HdlMsg_DN_PE_ACK_REPORT_ODOPULSE          // 里程脉冲数上报
    ,DN_PE_CMD_SET_ODOPULSE_PARA,           HdlMsg_DN_PE_CMD_SET_ODOPULSE_PARA        // 里程脉冲参数设置
       
    ,DN_PE_CMD_SET_AD_PARA,                 HdlMsg_DN_PE_CMD_SET_AD_PARA              // 设置AD采集参数
    ,DN_PE_CMD_GET_AD,                      HdlMsg_DN_PE_CMD_GET_AD                   // 获取AD
    ,DN_PE_ACK_REPORT_AD,                   HdlMsg_DN_PE_ACK_REPORT_AD                // AD转换结果上报请求
       
    ,DN_PE_ACK_REPORT_KEY,                  HdlMsg_DN_PE_ACK_REPORT_KEY               // 按键上报应答
#if EN_GSENSOR > 0
    ,DN_PE_CMD_HITCK_DMC_START,             HdlMsg_DN_PE_CMD_HITCK_DMC_START          // 启动碰撞检测标定请求
    ,DN_PE_CMD_HITCK_DMC_STOP,              HdlMsg_DN_PE_CMD_HITCK_DMC_STOP           // 停止碰撞检测标定请求
    ,DN_PE_CMD_HITCK_PARA_SET,              HdlMsg_DN_PE_CMD_HITCK_PARA_SET           // 碰撞检测参数设置
    ,DN_PE_ACK_HITCK_REPORT,                HdlMsg_DN_PE_ACK_HITCK_REPORT             // 碰撞检测信号上报
#endif
};

static INT8U s_funnum = sizeof(s_functionentry) / sizeof(s_functionentry[0]);                                             



/*******************************************************************
** 函数名:     ScanTmrProc
** 函数描述:   链路探寻定时器
** 参数:       [in] pdata：定时器特征值
** 返回:       无
********************************************************************/
static void ScanTmrProc(void *pdata)
{
    INT8U i, sendadc;
    INT32U sensor;
    KEY_VALUE_T key;
    
    OS_StartTmr(s_scantmr, PERIOD_SCAN);
    
    if (YX_MMI_IsON()) {
        for (i = 0; i < sizeof(s_tcb.key_time) / sizeof(s_tcb.key_time[0]); i++) {/* 按键上报 */
            if (s_tcb.key_time[i] < (KEY_LONG_TIME / 100)) {
                if (++s_tcb.key_time[i] >= (KEY_LONG_TIME / 100)) {
                    if (i == 0) {
                        key.key    = KEY_ACK;
                    } else if (i == 1) {
                        key.key    = KEY_UP;
                    } else if (i == 2) {
                        key.key    = KEY_DOWN;
                    } else {
                        key.key    = KEY_NAK;
                    }
                    
                    key.attrib = KEY_ATTRIB_LONG;
                    YX_MMI_SendKeyValue(&key);
                }
            }
        }
        
        sensor = GetSensorStatus();
        if (sensor != s_tcb.sensor) {                                          /* 传感器边沿变化上报 */
            s_tcb.sensor = sensor;
            YX_MMI_SendSensorStatus();
        }
        
        if (s_tcb.gpio_onoff == 0x01 && s_tcb.gpio_period != 0) {              /* 间隔上报传感器状态 */
            if (++s_tcb.ct_gpio >= ((s_tcb.gpio_period + 99) / 100)) {
                s_tcb.ct_gpio = 0;
                YX_MMI_SendSensorStatus();
            }
        }
        
        sendadc = 0;
        for (i = 0; i < ADC_CH_MAX; i++) {                                     /* 间隔上报AD采样值 */
            if (s_tcb.ad_onoff[i] == 0x01 && s_tcb.ad_period[i] != 0 && s_tcb.ad_pkt[i] != 0) {
                if (++s_tcb.ct_ad[i] >= ((s_tcb.ad_period[i] + 99) / 100)) {
                    s_tcb.ct_ad[i] = 0;
                    sendadc = true;
                    break;
                }
            }
        }
        
        if (sendadc) {
            YX_MEMSET(s_tcb.ct_ad, 0, sizeof(s_tcb.ct_ad));
            YX_MMI_SendAdcValue();
        }
        
        if (s_tcb.pulse_onoff == 0x01 && s_tcb.pulse_period != 0 && s_tcb.pulse_pkt != 0) {/* 间隔上报里程脉冲 */
            if (DAL_PULSE_GetGroupNum() >= s_tcb.pulse_pkt) {
                YX_MMI_SendOdometerPulse();
            }
        } else {
            if (DAL_PULSE_IsOpen()) {
                DAL_PULSE_ClosePulseCalFunction();
            }
        }
        
        if (++s_tcb.ct_rtc >= 600) {
            s_tcb.ct_rtc = 0;
            YX_MMI_SendRealClock();
        }
    }
}

/*******************************************************************
** 函数名:     SignalChangeInformer
** 函数描述:   信号跳变通知处理
** 参数:       [in] port: 输入口编号，见INPUT_IO_E
**             [in] mode: 信号跳变触发模式,INPUT_TRIG_E
** 返回:       无
********************************************************************/
static void SignalChangeInformer(INT8U port, INT8U mode)
{
    KEY_VALUE_T key;
    
    #if DEBUG_MMI > 0
    printf_com("<按键信号变化(%d)(%d)>\r\n", port, mode);
    #endif
    
    switch (port) 
    {
    case IPT_KEYACK:
        if (mode == INPUT_TRIG_POSITIVE) {
            s_tcb.key_time[0] = 0;
        } else {
            if (s_tcb.key_time[0] < 20) {
                key.key    = KEY_ACK;
                key.attrib = KEY_ATTRIB_SHORT;
                
                YX_MMI_SendKeyValue(&key);
            }
            s_tcb.key_time[0] = 0xFF;
        }
        break;
    case IPT_KEYUP:
        if (mode == INPUT_TRIG_POSITIVE) {
            s_tcb.key_time[1] = 0;
        } else {
            if (s_tcb.key_time[1] < 20) {
                key.key    = KEY_UP;
                key.attrib = KEY_ATTRIB_SHORT;
                
                YX_MMI_SendKeyValue(&key);
            }
            s_tcb.key_time[1] = 0xFF;
        }
        break;
    case IPT_KEYDOWN:
        if (mode == INPUT_TRIG_POSITIVE) {
            s_tcb.key_time[2] = 0;
        } else {
            if (s_tcb.key_time[2] < 20) {
                key.key    = KEY_DOWN;
                key.attrib = KEY_ATTRIB_SHORT;
                
                YX_MMI_SendKeyValue(&key);
            }
            s_tcb.key_time[2] = 0xFF;
        }
        break;
    case IPT_KEYNAK:
        if (mode == INPUT_TRIG_POSITIVE) {
            s_tcb.key_time[3] = 0;
        } else {
            if (s_tcb.key_time[3] < 20) {
                key.key    = KEY_NAK;
                key.attrib = KEY_ATTRIB_SHORT;
                
                YX_MMI_SendKeyValue(&key);
            }
            s_tcb.key_time[3] = 0xFF;
        }
        break;
    default:
        break;
    }
}

/*******************************************************************
** 函数名:     SignalChangeInformer_ROB
** 函数描述:   信号跳变通知处理
** 参数:       [in] port: 输入口编号，见INPUT_IO_E
**             [in] mode: 信号跳变触发模式,INPUT_TRIG_E
** 返回:       无
********************************************************************/
static void SignalChangeInformer_ROB(INT8U port, INT8U mode)
{
    OS_ASSERT((port == IPT_ROB), RETURN_VOID);
    
    if (mode == INPUT_TRIG_POSITIVE) {                                         /* 抢劫有效 */
        if (!YX_MMI_IsON()) {
            s_tcb.ct_lockrob = 100;
        } else {
            s_tcb.ct_lockrob = 0;
        }
    }
}

#if EN_GSENSOR > 0
/*******************************************************************
** 函数名:     Callback_Gsensor
** 函数描述:   加速度传感器事件通知处理
** 参数:       [in] event: 事件类型,见 GSENSOR_EVENT_E
** 返回:       无
********************************************************************/
static void Callback_Gsensor(INT8U event)
{
    YX_MMI_SendGsensorInfo(event);
}
#endif

/*******************************************************************
** 函数名:     YX_MMI_InitSensor
** 函数描述:   MMI驱动模块初始化
** 参数:       无
** 返回:       无
********************************************************************/
void YX_MMI_InitSensor(void)
{
    INT8U i;
    
    YX_MEMSET(&s_tcb, 0, sizeof(s_tcb));
    YX_MEMSET(&s_tcb.key_time, 0xFF, sizeof(s_tcb.key_time));
    
    s_tcb.gpio_onoff  = 0x01;            /* 主动上报开关 */
    s_tcb.gpio_period = 1000;            /* 主动上报时间间隔 */
    
    s_tcb.pulse_onoff  = 0x01;           /* 里程脉冲上报开关 */
    s_tcb.pulse_period = 200;            /* 里程脉冲采集周期 */
    s_tcb.pulse_pkt    = 1;              /* 里程脉冲打包个数 */
    
    if (s_tcb.pulse_onoff == 0x01 && s_tcb.pulse_period != 0 && s_tcb.pulse_pkt != 0) {
        DAL_PULSE_OpenPulseCalFunction(s_tcb.pulse_period);
    }
    
    for (i = 0; i < s_funnum; i++) {
        YX_MMI_Register(s_functionentry[i].cmd, s_functionentry[i].entryproc);
    }
    
    s_scantmr  = OS_CreateTmr(TSK_ID_APP, (void *)0, ScanTmrProc);
    OS_StartTmr(s_scantmr,  PERIOD_SCAN);
    
    DAL_INPUT_InstallTriggerProc(IPT_KEYACK,  INPUT_TRIG_POSITIVE | INPUT_TRIG_NEGATIVE, SignalChangeInformer);
    DAL_INPUT_InstallTriggerProc(IPT_KEYUP,   INPUT_TRIG_POSITIVE | INPUT_TRIG_NEGATIVE, SignalChangeInformer);
    DAL_INPUT_InstallTriggerProc(IPT_KEYDOWN, INPUT_TRIG_POSITIVE | INPUT_TRIG_NEGATIVE, SignalChangeInformer);
    DAL_INPUT_InstallTriggerProc(IPT_KEYNAK,  INPUT_TRIG_POSITIVE | INPUT_TRIG_NEGATIVE, SignalChangeInformer);
    DAL_INPUT_InstallTriggerProc(IPT_ROB,     INPUT_TRIG_POSITIVE | INPUT_TRIG_NEGATIVE, SignalChangeInformer_ROB);
#if EN_GSENSOR > 0
    DAL_HIT_RegistEventHandler(Callback_Gsensor);
#endif
}

/*******************************************************************
** 函数名:     YX_MMI_SendSensorStatus
** 函数描述:   发送传感器状态
** 参数:       无
** 返回:       成功返回TRUE, 失败返回FALSE
********************************************************************/
BOOLEAN YX_MMI_SendSensorStatus(void)
{
    STREAM_T *wstrm;
    
    if (!YX_MMI_IsON()) {
        return false;
    }
    
    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteLONG_Strm(wstrm, GetSensorStatus());
    //YX_MMI_ListSend(UP_PE_CMD_REPORT_SENSOR_STATUS, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 0, 0, 0);
    return YX_MMI_DirSend(UP_PE_CMD_REPORT_SENSOR_STATUS, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm));
}

/*******************************************************************
** 函数名:     YX_MMI_SendOdometerPulse
** 函数描述:   发送里程脉冲数
** 参数:       无
** 返回:       成功返回TRUE, 失败返回FALSE
********************************************************************/
BOOLEAN YX_MMI_SendOdometerPulse(void)
{
    INT8U i, num;
    STREAM_T *wstrm;
    
    if (!YX_MMI_IsON()) {
        return false;
    }
    
    num = DAL_PULSE_GetGroupNum();
    if (num > 0) {
        wstrm = YX_STREAM_GetBufferStream();
        YX_WriteBYTE_Strm(wstrm, num);
        for (i = 0; i < num; i++) {
            YX_WriteHWORD_Strm(wstrm, DAL_PULSE_GetPulse());
        }
        
        YX_MMI_ListSend(UP_PE_CMD_REPORT_ODOPULSE, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 0, 0, 0);
    }
    return true;
}

/*******************************************************************
** 函数名:     YX_MMI_SendAdcValue
** 函数描述:   发送AD采样值
** 参数:       无
** 返回:       成功返回TRUE, 失败返回FALSE
********************************************************************/
BOOLEAN YX_MMI_SendAdcValue(void)
{
    INT8U i, num;
    INT8U *ptr;
    STREAM_T *wstrm;
    
    if (!YX_MMI_IsON()) {
        return false;
    }
    
    wstrm = YX_STREAM_GetBufferStream();
    
    ptr = YX_GetStrmPtr(wstrm);
    YX_WriteBYTE_Strm(wstrm, 0);                                               /* 预填通道个数 */
    num = 0;
    for (i = 0; i < ADC_CH_MAX; i++) {
        if (s_tcb.ad_onoff[i] == 0x01 && s_tcb.ad_period[i] != 0 && s_tcb.ad_pkt[i] != 0) {/* AD开关 */
            num++;
            YX_WriteBYTE_Strm(wstrm, i + 1);                                   /* 通道号 */
            YX_WriteBYTE_Strm(wstrm, 1);                                       /* AD值个数 */
            YX_WriteHWORD_Strm(wstrm, ST_ADC_GetValue(i));                     /* AD值 */
        }
    }
    
    if (num > 0) {
        *ptr = num;
        YX_MMI_DirSend(UP_PE_CMD_REPORT_AD, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm));
        //YX_MMI_ListSend(UP_PE_CMD_REPORT_AD, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 0, 0, 0);
    }
    return true;
}

/*******************************************************************
** 函数名:     YX_MMI_SendKeyValue
** 函数描述:   发送按键值
** 参数:       无
** 返回:       成功返回TRUE, 失败返回FALSE
********************************************************************/
BOOLEAN YX_MMI_SendKeyValue(KEY_VALUE_T *key)
{
    STREAM_T *wstrm;
    
    if (!YX_MMI_IsON()) {
        return false;
    }
    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteBYTE_Strm(wstrm, key->key);
    YX_WriteBYTE_Strm(wstrm, key->attrib);
    return YX_MMI_DirSend(UP_PE_CMD_REPORT_KEY, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm));
}

/*******************************************************************
** 函数名:     YX_MMI_SendGsensorInfo
** 函数描述:   发送加速度传感器报警信息
** 参数:       无
** 返回:       成功返回TRUE, 失败返回FALSE
********************************************************************/
BOOLEAN YX_MMI_SendGsensorInfo(INT8U event)
{
    STREAM_T *wstrm;
    
    if (!YX_MMI_IsON()) {
        return false;
    }
    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteBYTE_Strm(wstrm, event);
    return YX_MMI_DirSend(UP_PE_CMD_HITCK_REPORT, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm));
}

#endif /* end of EN_MMI */

