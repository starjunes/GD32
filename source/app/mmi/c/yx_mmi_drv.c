/********************************************************************************
**
** ÎÄ¼þÃû:     yx_mmi_drv.c
** °æÈ¨ËùÓÐ:   (c) 2007-2008 ÏÃÃÅÑÅÑ¸ÍøÂç¹É·ÝÓÐÏÞ¹«Ë¾
** ÎÄ¼þÃèÊö:   ¸ÃÄ£¿éÖ÷ÒªÊµÏÖdvrÍâÉèÇý¶¯»ù±¾¹¦ÄÜÒµÎñ£¬ÍâÉè×´Ì¬¹ÜÀí
**
*********************************************************************************
**             ÐÞ¸ÄÀúÊ·¼ÇÂ¼
**===============================================================================
**| ÈÕÆÚ       | ×÷Õß   |  ÐÞ¸Ä¼ÇÂ¼
**===============================================================================
**| 2014/03/22 | Ò¶µÂÑæ |  ´´½¨µÚÒ»°æ±¾
*********************************************************************************/
#include "yx_include.h"
#include "yx_dym_drv.h"
#include "st_rtc_drv.h"
#include "dal_pp_drv.h"
#include "yx_debug.h"

#if EN_MMI > 0
#include "yx_mmi_drv.h"

/*
********************************************************************************
*ºê¶¨Òå
********************************************************************************
*/

#define _ON                   0x01
#define _SLEEP                0x02

#define MAX_QUERY             20                 /* Á´Â··¢ËÍÖÜÆÚ */
#define MAX_OVERTIME          360               /* ¼à¿Ø³¬Ê±ÖÜÆÚ */
#define MAX_WATCHDOG          30                /* ¿´ÃÅ¹·Òç³öÊ±¼ä */
#define MAX_VER               41

#define PERIOD_LINK           _SECOND, 1
#define PERIOD_RESET          _SECOND, 3

/*
********************************************************************************
* ¶¨ÒåÄ£¿éÊý¾Ý½á¹¹
********************************************************************************
*/

typedef struct {
    INT8U     status;                  /* MMI×´Ì¬ */
    INT16U    ct_query;                /* Á´Â·Î¬»¤¼ÆÊýÆ÷ */
    INT16U    ct_overtime;             /* Á´Â·³¬Ê±¼ÆÊýÆ÷ */
    INT16U    ct_watchdog;             /* ¿´ÃÅ¹·Ê±¼äÖÜÆÚ */
    INT8U     powersave;               /* Ê¡µç¿ØÖÆ */
    INT8U     verlen;                  /* °æ±¾³¤¶È */
    INT8U     ver[MAX_VER];            /* °æ±¾ */
    INT8U     fun[4];                  /* ¹¦ÄÜÅäÖÃ */
} TCB_T;

/*
********************************************************************************
* ¶¨ÒåÄ£¿é±äÁ¿
********************************************************************************
*/
static TCB_T s_tcb;
static INT8U s_linktmr, s_resettmr;

/*******************************************************************
** º¯ÊýÃû:     SendSetupLinkCommand
** º¯ÊýÃèÊö:   ÍâÉè·¢ËÍÉÏµç½¨Á¢Á¬½ÓÇëÇóÖ¸Áî
** ²ÎÊý:       ÎÞ
** ·µ»Ø:       ÎÞ
********************************************************************/
static void SendSetupLinkCommand(void)   
{
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteBYTE_Strm(wstrm, MAX_QUERY);                                       /* Á´Â·Î¬»¤Ê±¼ä */
    YX_MMI_ListSend(UP_PE_CMD_LINK_REQ, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 0, 0, 0);
}

/*******************************************************************
** º¯ÊýÃû:     SendBeatCommand
** º¯ÊýÃèÊö:   ÍâÉè·¢ËÍÐÄÌøÇëÇóÖ¸Áî
** ²ÎÊý:       ÎÞ
** ·µ»Ø:       ÎÞ
********************************************************************/
static void SendBeatCommand(void)   
{
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteBYTE_Strm(wstrm, MAX_QUERY);                                       /* Á´Â·Î¬»¤Ê±¼ä */
    YX_MMI_ListSend(UP_PE_CMD_BEAT_REQ, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 0, 0, 0);
}

/*******************************************************************
** º¯ÊýÃû:     HdlMsg_DN_PE_ACK_LINK_REQ
** º¯ÊýÃèÊö:   Á¬½Ó×¢²áÇëÇóÐ­ÒéÓ¦´ð
** ²ÎÊý:       [in]cmd:ÃüÁî±àÂë
**             [in]data:Êý¾ÝÖ¸Õë
**             [in]datalen:Êý¾Ý³¤¶È
** ·µ»Ø:       ÎÞ
********************************************************************/
static void HdlMsg_DN_PE_ACK_LINK_REQ(INT8U cmd, INT8U *data, INT16U datalen)
{
    if (cmd != DN_PE_ACK_LINK_REQ) {
        return;
    }
    
    #if DEBUG_MMI > 0
    printf_com("<ÊÕµ½MMIÁ¬½Ó×¢²áÇëÇóÓ¦´ð(0x%x)(%d)>\r\n", s_tcb.status, data[0]);
    #endif
    
    if (data[0] == 0x01) {
        s_tcb.ct_query = 0;
        s_tcb.ct_overtime = 0;
        s_tcb.status |= _ON;                                                   /* MMIÁ¬½Ó×´Ì¬ */
    
        YX_MMI_ListAck(UP_PE_CMD_LINK_REQ, _SUCCESS);
        SendBeatCommand();
        YX_MMI_SendRealClock();
        //YX_MMI_SendIccardInfo();
    }
}

/*******************************************************************
** º¯ÊýÃû:     HdlMsg_DN_PE_ACK_BEAT_REQ
** º¯ÊýÃèÊö:   MMIÁ´Â·Î¬»¤ÇëÇó
** ²ÎÊý:       [in]cmd:ÃüÁî±àÂë
**             [in]data:Êý¾ÝÖ¸Õë
**             [in]datalen:Êý¾Ý³¤¶È
** ·µ»Ø:       ÎÞ
********************************************************************/
static void HdlMsg_DN_PE_ACK_BEAT_REQ(INT8U cmd, INT8U *data, INT16U datalen)
{
    if (cmd != DN_PE_ACK_BEAT_REQ) {
        return;
    }
    
    #if DEBUG_MMI > 0
    printf_com("<ÊÕµ½MMIÁ´Â·Î¬»¤ÇëÇóÓ¦´ð>\r\n");
    #endif
    
    if (data[0] == 0x01) {
        s_tcb.ct_query = 0;
        s_tcb.ct_overtime = 0;
        s_tcb.status |= _ON;                                                       /* MMIÁ¬½Ó×´Ì¬ */
        YX_MMI_ListAck(UP_PE_CMD_BEAT_REQ, _SUCCESS);
    }
}

/*******************************************************************
** º¯ÊýÃû:     HdlMsg_DN_PE_CMD_VERSION_REQ
** º¯ÊýÃèÊö:   °æ±¾²éÑ¯Ó¦´ð
** ²ÎÊý:       [in]cmd:    ÃüÁî±àÂë
**             [in]data:   Êý¾ÝÖ¸Õë
**             [in]datalen:Êý¾Ý³¤¶È
** ·µ»Ø:       ÎÞ
********************************************************************/
static void HdlMsg_DN_PE_CMD_VERSION_REQ(INT8U cmd, INT8U *data, INT16U datalen)
{
    STREAM_T *wstrm;
    char *pver;
    
    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteBYTE_Strm(wstrm, COM_VER_MMI);
    YX_WriteBYTE_Strm(wstrm, MAX_QUERY);                                       /* Á´Â·Î¬»¤Ê±¼ä */
    pver = YX_GetVersion();                                                    /* °æ±¾ºÅ */
    YX_WriteBYTE_Strm(wstrm, YX_STRLEN(pver));
    YX_WriteSTR_Strm(wstrm, pver);
    
    YX_MMI_ListSend(UP_PE_ACK_VERSION_REQ, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 0, 0, 0);
}

/*******************************************************************
** º¯ÊýÃû:     HdlMsg_DN_PE_CMD_RESET_INFORM
** º¯ÊýÃèÊö:   Ö÷»ú¼´½«¸´Î»Í¨ÖªÇëÇó
** ²ÎÊý:       [in]cmd:    ÃüÁî±àÂë
**             [in]data:   Êý¾ÝÖ¸Õë
**             [in]datalen:Êý¾Ý³¤¶È
** ·µ»Ø:       ÎÞ
********************************************************************/
static void HdlMsg_DN_PE_CMD_RESET_INFORM(INT8U cmd, INT8U *data, INT16U datalen)
{
    STREAM_T *wstrm;
    
    #if DEBUG_MMI > 0
    printf_com("<Ö÷»úÍ¨ÖªÍâÉè,Ö÷»ú¼´½«¸´Î»(%d)>\r\n", data[0]);
    #endif
    
    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteBYTE_Strm(wstrm, data[0]);
    YX_WriteBYTE_Strm(wstrm, PE_ACK_MMI);
    YX_MMI_ListSend(UP_PE_ACK_RESET_INFORM, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 0, 0, 0);
}

/*******************************************************************
** º¯ÊýÃû:     HdlMsg_DN_PE_ACK_PE_RESET_INFORM
** º¯ÊýÃèÊö:   ÍâÉè¼´½«¸´Î»Í¨ÖªÇëÇóÓ¦´ð
** ²ÎÊý:       [in]cmd:ÃüÁî±àÂë
**             [in]data:Êý¾ÝÖ¸Õë
**             [in]datalen:Êý¾Ý³¤¶È
** ·µ»Ø:       ÎÞ
********************************************************************/
static void HdlMsg_DN_PE_ACK_PE_RESET_INFORM(INT8U cmd, INT8U *data, INT16U datalen)
{
    YX_MMI_ListAck(UP_PE_CMD_PE_RESET_INFORM, _SUCCESS);
}

/*******************************************************************
** º¯ÊýÃû:     HdlMsg_DN_PE_ACK_HOST_RESET_INFORM
** º¯ÊýÃèÊö:   ÍâÉèÍ¨ÖªÖ÷»ú£¬ÍâÉè¼´½«¹Ø±Õ»òÖØÆôÖ÷»úÍ¨ÖªÇëÇóÓ¦´ð
** ²ÎÊý:       [in]cmd:ÃüÁî±àÂë
**             [in]data:Êý¾ÝÖ¸Õë
**             [in]datalen:Êý¾Ý³¤¶È
** ·µ»Ø:       ÎÞ
********************************************************************/
static void HdlMsg_DN_PE_ACK_HOST_RESET_INFORM(INT8U cmd, INT8U *data, INT16U datalen)
{
    YX_MMI_ListAck(UP_PE_CMD_HOST_RESET_INFORM, _SUCCESS);
}

/*******************************************************************
** º¯ÊýÃû:     HdlMsg_DN_PE_CMD_RESTART_REQ
** º¯ÊýÃèÊö:   ¸´Î»ÖØÆôÇëÇó
** ²ÎÊý:       [in]cmd:    ÃüÁî±àÂë
**             [in]data:   Êý¾ÝÖ¸Õë
**             [in]datalen:Êý¾Ý³¤¶È
** ·µ»Ø:       ÎÞ
********************************************************************/
static void Reset(INT8U result)
{
    OS_RESET(RESET_EVENT_INITIATE);
}

static void HdlMsg_DN_PE_CMD_RESTART_REQ(INT8U cmd, INT8U *data, INT16U datalen)
{
    STREAM_T *wstrm;
    
    #if DEBUG_MMI > 0
    printf_com("<Ö÷»úÇëÇó¸´Î»ÍâÉè(%d)>\r\n", data[0]);
    #endif
    
    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteBYTE_Strm(wstrm, data[0]);
    YX_WriteBYTE_Strm(wstrm, PE_ACK_MMI);
    YX_MMI_ListSend(UP_PE_ACK_RESTART_REQ, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 1, 5, Reset);
    
}

/*******************************************************************
** º¯ÊýÃû:     HdlMsg_DN_PE_CMD_CLEAR_WATCHDOG
** º¯ÊýÃèÊö:   ¿´ÃÅ¹·Î¹¹·
** ²ÎÊý:       [in]cmd:    ÃüÁî±àÂë
**             [in]data:   Êý¾ÝÖ¸Õë
**             [in]datalen:Êý¾Ý³¤¶È
** ·µ»Ø:       ÎÞ
********************************************************************/
static void HdlMsg_DN_PE_CMD_CLEAR_WATCHDOG(INT8U cmd, INT8U *data, INT16U datalen)
{
    STREAM_T *wstrm;
    
    if (data[0] == 0xAA) {
        s_tcb.ct_watchdog = MAX_WATCHDOG;
        
        wstrm = YX_STREAM_GetBufferStream();
        YX_WriteBYTE_Strm(wstrm, PE_ACK_MMI);
        YX_MMI_ListSend(UP_PE_ACK_CLEAR_WATCHDOG, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 0, 0, 0);
    }
}

/*******************************************************************
** º¯ÊýÃû:     HdlMsg_DN_PE_CMD_GET_RESET_REC
** º¯ÊýÃèÊö:   ×î½ü¸´Î»ÌõÊý
** ²ÎÊý:       [in]cmd:    ÃüÁî±àÂë
**             [in]data:   Êý¾ÝÖ¸Õë
**             [in]datalen:Êý¾Ý³¤¶È
** ·µ»Ø:       ÎÞ
********************************************************************/
static void HdlMsg_DN_PE_CMD_GET_RESET_REC(INT8U cmd, INT8U *data, INT16U datalen)
{
#if EN_APP > 0
    INT8U i;
    STREAM_T *wstrm;
    RESET_RECORD_T resetrecord;
    
    DAL_PP_ReadParaByID(PP_RESET_, (INT8U *)&resetrecord, sizeof(RESET_RECORD_T));
    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteHWORD_Strm(wstrm, resetrecord.rst_total);                          /* ×Ü¸´Î»´ÎÊý */
    YX_WriteHWORD_Strm(wstrm, resetrecord.rst_int);                            /* Ö÷¶¯¸´Î»´ÎÊý */
    YX_WriteHWORD_Strm(wstrm, resetrecord.rst_err);                            /* ³ö´í¸´Î»´ÎÊý */
    YX_WriteHWORD_Strm(wstrm, resetrecord.rst_ext);                            /* Íâ²¿¸´Î»´ÎÊý */
    YX_WriteBYTE_Strm(wstrm, MAX_RESET_RECORD);                                /* ×î½ü¸´Î»ÌõÊý */
    
    for (i = 0; i < MAX_RESET_RECORD; i++) {
        YX_WriteDATA_Strm(wstrm, (INT8U *)&resetrecord.rst_record[i].systime, 6);
        YX_WriteDATA_Strm(wstrm, (INT8U *)&resetrecord.rst_record[i].file, 15);
        YX_WriteHWORD_Strm(wstrm, resetrecord.rst_record[i].line);
    }
    
    YX_MMI_ListSend(UP_PE_ACK_GET_RESET_REC, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 0, 0, 0);
#endif
}



static FUNCENTRY_MMI_T s_functionentry[] = {
        DN_PE_ACK_LINK_REQ,                   HdlMsg_DN_PE_ACK_LINK_REQ            // Á¬½Ó×¢²áÇëÇóÐ­Òé
       ,DN_PE_ACK_BEAT_REQ,                   HdlMsg_DN_PE_ACK_BEAT_REQ            // Á´Â·Î¬»¤ÇëÇóÐ­Òé
       ,DN_PE_CMD_VERSION_REQ,	              HdlMsg_DN_PE_CMD_VERSION_REQ         // °æ±¾²éÑ¯Ó¦´ð
       
       ,DN_PE_CMD_RESET_INFORM,               HdlMsg_DN_PE_CMD_RESET_INFORM        // Ö÷»ú¼´½«¸´Î»Í¨ÖªÇëÇó
       ,DN_PE_ACK_PE_RESET_INFORM ,		      HdlMsg_DN_PE_ACK_PE_RESET_INFORM     // ÍâÉèÍ¨ÖªÖ÷»ú£¬ÍâÉè¼´½«¸´Î»Í¨ÖªÇëÇó
       ,DN_PE_ACK_HOST_RESET_INFORM ,		  HdlMsg_DN_PE_ACK_HOST_RESET_INFORM   // ÍâÉèÍ¨ÖªÖ÷»ú£¬ÍâÉè¼´½«¹Ø±Õ»òÖØÆôÖ÷»úÍ¨ÖªÇëÇóÓ¦´ð
       ,DN_PE_CMD_RESTART_REQ,                HdlMsg_DN_PE_CMD_RESTART_REQ         // ¸´Î»ÖØÆôÇëÇó
       
       ,DN_PE_CMD_CLEAR_WATCHDOG,             HdlMsg_DN_PE_CMD_CLEAR_WATCHDOG      // ¿´ÃÅ¹·Î¹¹·
       ,DN_PE_CMD_GET_RESET_REC,              HdlMsg_DN_PE_CMD_GET_RESET_REC       // ¸´Î»¼ÇÂ¼²éÑ¯
};

static INT8U s_funnum = sizeof(s_functionentry) / sizeof(s_functionentry[0]);


                                             
/*******************************************************************
** º¯ÊýÃû:     ResetTmrProc
** º¯ÊýÃèÊö:   ÑÓÊ±ÉÏµç¶¨Ê±Æ÷
** ²ÎÊý:       [in] pdata£º¶¨Ê±Æ÷ÌØÕ÷Öµ
** ·µ»Ø:       ÎÞ
********************************************************************/
static void ResetTmrProc(void *pdata)
{
    OS_StopTmr(s_resettmr);
    
    YX_MMI_PullUp();
    YX_MMI_Reset();                                                           /* ½«Í¨µÀ¸´Î»Îª¹²Ïí×´Ì¬ */
    //YX_MMI_CfgBaud(115200, UART_DATABIT_8, UART_STOPBIT_1, UART_PARITY_NONE);
}

/*******************************************************************
** º¯ÊýÃû:     LinkTmrProc
** º¯ÊýÃèÊö:   Á´Â·Ì½Ñ°¶¨Ê±Æ÷
** ²ÎÊý:       [in] pdata£º¶¨Ê±Æ÷ÌØÕ÷Öµ
** ·µ»Ø:       ÎÞ
********************************************************************/
static void LinkTmrProc(void *pdata)
{
    OS_StartTmr(s_linktmr, PERIOD_LINK);
    
    if (!YX_MMI_IsON()) {
        SendSetupLinkCommand();
    } else {
        if (++s_tcb.ct_query >= MAX_QUERY) {                                   /* ÐÄÌø·¢ËÍÖÜÆÚ */
            s_tcb.ct_query = 0;
            SendBeatCommand();
        }
        #if 1
        if (s_tcb.ct_watchdog > 0) {                                           /* ¿´ÃÅ¹·¼à¿Ø */
            if (--s_tcb.ct_watchdog == 0) {
                //s_tcb.ct_watchdog = MAX_WATCHDOG;
                s_tcb.ct_watchdog = MAX_OVERTIME;
                
                s_tcb.status &= (~_ON);
                s_tcb.ct_query = 0;
                s_tcb.ct_overtime = 0;
                
                YX_MMI_PullDown();
                OS_StartTmr(s_resettmr, PERIOD_RESET);                         /* ÑÓÊ±ÉÏµç */
                return;
            } else if (s_tcb.ct_watchdog == 10) {
                YX_MMI_CfgBaud(115200, UART_DATABIT_8, UART_STOPBIT_1, UART_PARITY_NONE);
                YX_MMI_SendHostResetInform(MMI_RESET_EVENT_WDG);
            }
        }
        #endif
    }
    #if 1
    if (++s_tcb.ct_overtime >= MAX_OVERTIME) {                                 /* Á´Â·Î¬»¤³¬Ê± */
        #if DEBUG_MMI > 0
        printf_com("<MMIÁ´Â·Î¬»¤Òì³£>\r\n");
        #endif
        
        s_tcb.status &= (~_ON);
        s_tcb.ct_query = 0;
        s_tcb.ct_overtime = 0;
        
        YX_MMI_PullDown();
        OS_StartTmr(s_resettmr, PERIOD_RESET);                                 /* ÑÓÊ±ÉÏµç */
    } else if (s_tcb.ct_overtime == MAX_OVERTIME - 10) {
        YX_MMI_CfgBaud(115200, UART_DATABIT_8, UART_STOPBIT_1, UART_PARITY_NONE);
        YX_MMI_SendHostResetInform(MMI_RESET_EVENT_NORMAL);
    }
    #endif
}

/*******************************************************************
** º¯ÊýÃû:     ResetInform
** º¯ÊýÃèÊö:   ¸´Î»»Øµ÷½Ó¿Ú
** ²ÎÊý:       [in] event£º¸´Î»ÀàÐÍ
**             [in] file£º ÎÄ¼þÃû
**             [in] line£º ³ö´íÐÐºÅ
** ·µ»Ø:       ÎÞ
********************************************************************/
static void ResetInform(INT8U event, char *file, INT32U line)
{
#if EN_APP > 0
    INT8U weekday;
    INT32U subsecond;
    GPS_DATA_T gpsdata;
    SYSTIME_T systime;
    
    if (event == RESET_EVENT_INITIATE) {
        YX_MMI_SendPeResetInform(MMI_RESET_EVENT_NORMAL);
    } else if (event == RESET_EVENT_ERR) {
        YX_MMI_SendPeResetInform(MMI_RESET_EVENT_ERROR);
    } else if (event == RESET_EVENT_UPDATE) {
        YX_MMI_SendPeResetInform(MMI_RESET_EVENT_UPDATE);
    } else {
        ;
    }
    
    if (ST_RTC_GetSystime(&systime.date, &systime.time, &weekday, &subsecond)) {
        DAL_PP_ReadParaByID(PP_ID_GPSDATA, (INT8U *)&gpsdata, sizeof(gpsdata));
        YX_MEMCPY(&gpsdata.systime, sizeof(gpsdata.systime), &systime, sizeof(systime));
        DAL_PP_StoreParaByID(PP_ID_GPSDATA, (INT8U *)&gpsdata, sizeof(gpsdata));
    }
#else
    if (event == RESET_EVENT_INITIATE) {
        YX_MMI_SendPeResetInform(MMI_RESET_EVENT_NORMAL);
    } else if (event == RESET_EVENT_ERR) {
        YX_MMI_SendPeResetInform(MMI_RESET_EVENT_ERROR);
    } else if (event == RESET_EVENT_UPDATE) {
        YX_MMI_SendPeResetInform(MMI_RESET_EVENT_UPDATE);
    } else {
        ;
    }
#endif
}

/*******************************************************************
** º¯ÊýÃû:     YX_MMI_InitDrv
** º¯ÊýÃèÊö:   MMIÇý¶¯Ä£¿é³õÊ¼»¯
** ²ÎÊý:       ÎÞ
** ·µ»Ø:       ÎÞ
********************************************************************/
void YX_MMI_InitDrv(void)
{
    INT8U i;
    
    YX_MEMSET(&s_tcb, 0, sizeof(s_tcb));
    //s_tcb.ct_watchdog = MAX_WATCHDOG;
    s_tcb.ct_watchdog = MAX_OVERTIME;
    
    YX_MMI_InitPower();
    YX_MMI_InitCom();
    YX_MMI_InitRecv();
    YX_MMI_InitSend();
    YX_MMI_InitCommon();
    YX_MMI_InitCan();
    //YX_MMI_InitUart();
    YX_MMI_InitGps();
    YX_MMI_InitSensor();
    YX_MMI_InitTr();
    YX_MMI_InitDownload();
#if EN_UARTEXT > 0
    YX_MMI_InitUartext();
#endif

    for (i = 0; i < s_funnum; i++) {
        YX_MMI_Register(s_functionentry[i].cmd, s_functionentry[i].entryproc);
    }
    
    s_linktmr  = OS_CreateTmr(TSK_ID_APP, (void *)0, LinkTmrProc);
    s_resettmr = OS_CreateTmr(TSK_ID_APP, (void *)0, ResetTmrProc);
    //OS_StartTmr(s_linktmr,  PERIOD_LINK);
    //YX_MMI_PullUp();
    OS_RegistResetInform(RESET_PRI_0, ResetInform);
}

/*******************************************************************
** º¯ÊýÃû:     YX_MMI_IsON
** º¯ÊýÃèÊö:   ÅÐ¶ÏMMIÊÇ·ñÁ¬½Ó
** ²ÎÊý:       ÎÞ
** ·µ»Ø:       ÒÑÁ¬½Ó·µ»Øtrue£¬Î´Á¬½Ó·µ»Øfalse
********************************************************************/
BOOLEAN YX_MMI_IsON(void)
{
    if ((s_tcb.status & _ON) != 0) {
        return true;
    } else {
        return false;
    }
}

/*******************************************************************
** º¯ÊýÃû:     YX_MMI_IsSleep
** º¯ÊýÃèÊö:   ÅÐ¶ÏMMIÊÇ·ñ´¦ÓÚÊ¡µç×´Ì¬
** ²ÎÊý:       ÎÞ
** ·µ»Ø:       ÊÇ·µ»Øtrue£¬·ñ·µ»Øfalse
********************************************************************/
BOOLEAN YX_MMI_IsSleep(void)
{
    if ((s_tcb.status & _SLEEP) != 0) {
        return true;
    } else {
        return false;
    }
}

/*******************************************************************
** º¯ÊýÃû:     YX_MMI_Sleep
** º¯ÊýÃèÊö:   Ê¡µç
** ²ÎÊý:       ÎÞ
** ·µ»Ø:       ÒÑÁ¬½Ó·µ»Øtrue£¬Î´Á¬½Ó·µ»Øfalse
********************************************************************/
BOOLEAN YX_MMI_Sleep(void)
{
    OS_StopTmr(s_linktmr);
    s_tcb.status = 0;
    return true;
}

/*******************************************************************
** º¯ÊýÃû:     YX_MMI_Wakeup
** º¯ÊýÃèÊö:   »½ÐÑ
** ²ÎÊý:       ÎÞ
** ·µ»Ø:       ÒÑÁ¬½Ó·µ»Øtrue£¬Î´Á¬½Ó·µ»Øfalse
********************************************************************/
BOOLEAN YX_MMI_Wakeup(void)
{
    OS_StartTmr(s_linktmr,  PERIOD_LINK);
    //s_tcb.status = 0;
    return true;
}

/*******************************************************************
** º¯ÊýÃû:     YX_MMI_GetVer
** ¹¦ÄÜÃèÊö:   »ñÈ¡MMI°æ±¾ºÅ
** ²ÎÊý:  	   ÎÞ
** ·µ»Ø:       °æ±¾ºÅÖ¸Õë
********************************************************************/
INT8U *YX_MMI_GetVer(void)
{
    return s_tcb.ver;
}

/***************************************************************
** º¯ÊýÃû:    YX_MMI_ResetReq
** ¹¦ÄÜÃèÊö:  ¸´Î»ÇëÇó
** ²ÎÊý:  	  [in] type: ¸´Î»ÀàÐÍ: 0x00Éý¼¶¸´Î»;0x01³£¹æ¸´Î»;0x02Òì³£¸´Î»
** ·µ»ØÖµ:    ÎÞ
***************************************************************/
static void Callback_Reset(INT8U result)
{
    YX_MMI_PullDown();
    OS_StartTmr(s_resettmr, PERIOD_RESET);                                     /* ÑÓÊ±ÉÏµç */
}

BOOLEAN YX_MMI_ResetReq(INT8U type)
{
    STREAM_T *wstrm;
    
    if (!YX_MMI_IsON()) {
        return FALSE;
    }
    
    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteBYTE_Strm(wstrm, type);
    return YX_MMI_ListSend(DN_PE_CMD_RESTART_REQ, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), 3, 1, Callback_Reset);
}

/***************************************************************
** º¯ÊýÃû:    YX_MMI_SendPeResetInform
** ¹¦ÄÜÃèÊö:  ÍâÉèÍ¨ÖªÖ÷»ú£¬ÍâÉè¼´½«¸´Î»£¬ÒÔ±ãÖ÷»ú×öºÃ¸´Î»Ç°´¦Àí¹¤×÷
** ²ÎÊý:  	  [in] type: ¸´Î»ÀàÐÍ,¼û MMI_RESET_EVENT_E
** ·µ»Ø:      ³É¹¦·µ»Øtrue£¬·ñ·µ»Øfalse
***************************************************************/
BOOLEAN YX_MMI_SendPeResetInform(INT8U type)
{
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteBYTE_Strm(wstrm, type);
    return YX_MMI_DirSend(UP_PE_CMD_PE_RESET_INFORM, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm));
}

/***************************************************************
** º¯ÊýÃû:    YX_MMI_SendPeResetInform
** ¹¦ÄÜÃèÊö:  ÍâÉèÍ¨ÖªÖ÷»ú£¬ÍâÉè¼´½«¹Ø±Õ»òÖØÆôÖ÷»ú(ÍâÉè×Ô¼º²»¸´Î»)÷
** ²ÎÊý:  	  [in] type: ¸´Î»ÀàÐÍ,¼û MMI_RESET_EVENT_E
** ·µ»Ø:      ³É¹¦·µ»Øtrue£¬·ñ·µ»Øfalse
***************************************************************/
BOOLEAN YX_MMI_SendHostResetInform(INT8U type)
{
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    YX_WriteBYTE_Strm(wstrm, type);
    return YX_MMI_DirSend(UP_PE_CMD_HOST_RESET_INFORM, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm));
}

#endif

