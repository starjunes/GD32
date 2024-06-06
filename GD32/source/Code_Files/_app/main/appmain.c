/**************************************************************************************************
**                                                                                               **
**  文件名称:  appmain.c                                                                         **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2010                                    **
**  创建信息:  Lantu.Cai -- 2010年12月6日                                                        **
**  文件描述:  应用程序接口                                                                      **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/
#define   APPMAIN_G
#include  "tools.h"
#include  "man_timer.h"
#if SWITCH_DEVICE == GK_110R
#include  "app_update.h"
#endif
#if SWITCH_NET == CDMA
#include  "app_adc.h"
#endif
#include  "debug_print.h"
//#include  "app_adc.h"
#include  "man_queue.h"
#include  "scanner.h"
#include  "s_flash.h"
#include  "dal_rtc.h"
#include  "appmain.h"
#include  "os_reg.h"
#include  "Public.H"
#include  "dal_adc.h"
#include  "dal_Infrared.H"
#include  "yx_viropttsk.h"
#include  "yx_can_man.h"
static QMSG_T   s_msgbuf[SIZE_APPMSG_BUF];


/**************************************************************************************************
**  函数名称:  HdlMsg_ProtocalDataSend
**  功能描述:  消息处理函数----协议数据发送
**  输入参数:
**  返回参数:
**************************************************************************************************/
static void HdlMsg_ProtocalDataSend(void)
{
    Hdl_MSG_COM_SEND();
}

/**************************************************************************************************
**  函数名称:  HdlMsg_ProtocalDataSend
**  功能描述:  消息处理函数,hit
**  输入参数:
**  返回参数:
**************************************************************************************************/
static void HdlMsg_OPT_GSEN_HIT(void)
{
    Hdl_MSG_OPT_GSEN_HIT(0,0,0,0);
}

/**************************************************************************************************
**  函数名称:  HdlMsg_ProtocalDataSend
**  功能描述:  消息处理函数,gsenmotion
**  输入参数:
**  返回参数:
**************************************************************************************************/
static void HdlMsg_OPT_GSEN_MOTION(void)
{
    Hdl_MSG_OPT_GSEN_MOTION(0,0,0,0);
}

/**************************************************************************************************
**  函数名称:  HdlMsg_OPT_RTCWAKE
**  功能描述:  消息处理函,rtcwake
**  输入参数:
**  返回参数:
**************************************************************************************************/
static void HdlMsg_OPT_RTCWAKE(void)
{
    Hdl_MSG_RTCWAKE();
}

/**************************************************************************************************
**  函数名称:  HdlMsg_OPT_RTCWAKE
**  功能描述:  消息处理函,OTWAKE
**  输入参数:
**  返回参数:
**************************************************************************************************/
static void HdlMsg_OPT_OTWAKE(void)
{
    Hdl_MSG_OTWAKE();
}

/*************************************************************************************************/
/*                           定义消息及处理函数关联                                              */
/*************************************************************************************************/

static  const FUNCENTRY_T   s_funcentrytbl[] = {
                            MSG_OPT_COM_SEND,           HdlMsg_ProtocalDataSend,
                            MSG_OPT_GSEN_HIT_EVNT,      HdlMsg_OPT_GSEN_HIT,
                            MSG_OPT_GSEN_MOTION_EVNT,   HdlMsg_OPT_GSEN_MOTION,
                            MSG_OPT_RTCWK_EVENT,        HdlMsg_OPT_RTCWAKE,
                            MSG_OPT_OTWAKE_ENENT,       HdlMsg_OPT_OTWAKE,
                           };


/**************************************************************************************************
**  函数名称:  AppMsgHdlEntry
**  功能描述:  应用程序消息处理入口
**  输入参数:
**  返回参数:
**************************************************************************************************/
static void AppMsgHdlEntry(void)
{
    if (Q_NO_ERR == QMsgAccept(g_appQ, &g_appMsg)) {
        FindProcEntry(g_appMsg.msgid, s_funcentrytbl, sizeof(s_funcentrytbl)/sizeof(s_funcentrytbl[0]));
    }
}

/**************************************************************************************************
**  函数名称:  GetBusType
**  功能描述:  获取总线模式
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
BUS_TYPE_E GetBusType(INT8U channel)
{
    return BUS_CAN;
    //return s_bustype[channel];
}


/**************************************************************************************************
**  函数名称:  AppStart
**  功能描述:  启动初始化各个应用模块
**  输入参数:
**  返回参数:
**************************************************************************************************/
void AppStart(void)
{
    BOOLEAN result;
    g_appQ  =  QCreate(s_msgbuf, sizeof(s_msgbuf)/sizeof(s_msgbuf[0]));
	APP_ASSERT(g_appQ != PNULL);
    result = QRegister(APP_Q_ID, g_appQ, AppMsgHdlEntry);
    APP_ASSERT(result);
    //PMCore_Init();
    YX_CAN_PreInit();
    InitFlash();
    ClearWatchdog(); 
    InitAllPubPara();
    #if DEBUG_PARA > 0
    Log_All_Para();
    #endif

    #if EN_CAN > 0
    //App_CAN_Init();                                                  /* 初始化CAN模块 */
    #endif

    //PulseSignalStat_Init();
    InitFrKb();/* 硬件定时器 */
	
    #if ((EN_IO_EXTEND > 0) || (EN_IO_EXTEND_R6 > 0))
    IOExtend_Config();// IO扩展，必须在比较前配置 
    #endif
    #if EN_PRINTER > 0
    //InitPrinter();
    #endif

    #if EN_EXPDATA > 0
    //DataExport_Init();
    #endif
    //MediaExport_Init();
    #if EN_USBUPDATE > 0
    //UsbUpdate_Init();
    #endif

    ClearWatchdog();

    #if EN_ADC > 0
    InitADC();
    #endif
    #if SWITCH_DEVICE == GK_110R
    UpDateMode_Init();
    #endif
	
    dal_rtc_init();
	
	
    //mmi_resetcnt_init();
    #if EN_DEBUG > 1
    Debug_SysPrint("初始化结束\r\n");
    #endif
}

/**************************************************************************************************
**  函数名称:  Call_Circulate
**  功能描述:  调用主循环
**  输入参数:  无
**  输出参数:  无
**  返回参数:  无
**************************************************************************************************/
void Call_Circulate(INT16U m)
{
    INT16U count = 0;
    const INT16U utime = 12 * 1000;
    do
    {
        for (count = 0; count < utime; count++) {
        }
        ClearWatchdog();
        SysTimerEntryProc();                                   /* timer management entry                */
        QMsgDispatch();                                        /* message dispatch                      */
        ScannerRunning();                                      /* scan some program entry               */
    } while(m--);
}

/************************ (C) COPYRIGHT 2010 XIAMEN YAXON.LTD *******************END OF FILE******/

