/********************************************************************************
**
** 文件名:     yx_protocol_common.c
** 版权所有:   (c) 2005-2012 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现登入、注册等普通业务协议解析
** 
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2014/02/10 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "yx_includes.h"
#include "yx_tools.h"
#include "yx_rx_frame.h"
#include "yx_callrht.h"
#include "yx_autodial.h"
#include "yx_oil_ctl.h"
#include "yx_generalman.h"
#include "yx_debug.h"
#include "yx_db_core.h"
#include "dal_pp_drv.h"
#include "yx_jt1_tlink.h"
#include "yx_jt1_ulink.h"
#include "yx_win_send.h"
#include "yx_resend.h"
#include "yx_tr_man.h"
#include "yx_Recordsave.h"
#include "yx_dm.h"
#include "yx_timer.h"
#include "dal_gprs_drv.h"
#include "dal_tcp_drv.h"
#include "dal_gps_drv.h"
#include "yx_protocol_type.h"
#if EN_TTS > 0
#include "dal_tts_drv.h"
#include "yx_ttscore.h"
#endif
#if EN_120R > 0
#include "yx_120r_drv.h"
#include "yx_120r_info.h"
#include "yx_120r_phone.h"
#endif
#include "yx_rx_tpframe.h"
#include "yx_mfilesend.h"
#include "yx_jt_linkman.h"
#include "yx_rangedrv.h"
#include "yx_apptool.h"
#include "yx_msgman.h"
#include "yx_message.h"
#include "yx_alarm_roadline.h"
#include "yx_generalman.h"
#include "yx_doorcontrol.h"
#if EN_HANDSET > 0
#include "yx_hst_h.h"
#endif
#if EN_RUNRECORD > 0
#include "yx_tr_loginreport.h"
#endif
#if EN_IC > 0
#include "yx_ic_tlink.h"
#endif
#include "yx_120nd70_drv.h"
/*
********************************************************************************
* 宏定义
********************************************************************************
*/
#define PERIOD_DELAY            SECOND, 6, LOW_PRECISION_TIMER

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
typedef struct {
    void (*inform_logout_auto)(void);
    void (*inform_logout_byhand)(void);
} LOGOUT_INFORM_T;

static STREAM_T s_rstrm;
static INT32U s_hdlframelen;
static INT32U s_channeltype;
static INT8U  s_rx_channeltype;
static PROTOCOL_HEAD_T *s_hdlframe;
static DB_EVENTLIST_T  s_event;
static DB_INFOMENU_T   s_info;
static TELBOOK_T s_telbook;
static INT8U s_tmptel[30];
static INT8U s_tmplen;
static INT8U s_delaycalltmr;
#if EN_AREARANGE_ALRAM > 0
static INT8U  s_def_lable[4] = {0x00,0x00, 0x00, 0x01};
#endif
static LOGOUT_INFORM_T s_logout_inform = {0, 0};

/*******************************************************************
** 函数名:     HdlRegAck
** 函数描述:   处理注册请求应答
** 参数:       无
** 返回:       无
********************************************************************/
static void InformMsg_AuthcodeLost(void)
{
    #if EN_120R > 0 && EN_DEBUG > 0
    if (YX_120R_IsON()) {
        YX_120R_DisplayText((INT8U *)"鉴权码无效,请先注销", 19, TEXT_120R_ALARM);
    }
    #endif
    #if EN_HANDSET > 0 && EN_DEBUG > 0
    if (YX_HST_IsON()) {
        YX_MMI_PromptWin_Hit((char *)"鉴权码无效,请先注销");
    }
    #endif
}

/*******************************************************************
** 函数名:     HdlMsg_DN_ACK_REG
** 函数描述:   终端注册应答
** 参数:       无
** 返回:       无
********************************************************************/
static void HdlMsg_DN_ACK_REG(void)
{
    INT16U  ackflowseq, len;
    INT8U   result;
    AUTH_CODE_T authcode;
    void (*LinkInformReged)(INT8U result) = NULL;
    BOOLEAN (*SendLogOutFrame)(void) = NULL;

    switch (s_rx_channeltype)
    {
    case RX_FRAME_TCP:
        LinkInformReged = YX_JTT1_LinkInformReged;
        SendLogOutFrame = YX_JTT1_SendReqUnregist;
        break;
    case RX_FRAME_TCP2:
        LinkInformReged = YX_JTT2_LinkInformReged;
        SendLogOutFrame = YX_JTT2_SendLogOutFrame;
        break;
    case RX_FRAME_UDP:
        LinkInformReged = YX_JTU1_LinkInformReged;
        SendLogOutFrame = YX_JTU1_SendLogOutFrame;            
        break;
    case RX_FRAME_UDP2:
        LinkInformReged = YX_JTU2_LinkInformReged;
        SendLogOutFrame = YX_JTU2_SendLogOutFrame;            
        break;                        
    default:
        LinkInformReged = NULL;
        SendLogOutFrame = NULL;
        return;
    }

    ackflowseq = YX_ReadHWORD_Strm_APP(&s_rstrm);
    result = YX_ReadBYTE_Strm(&s_rstrm);
    if (result == 0 || result == 1 || result == 3) {
        len = YX_GetStrmLeftLen(&s_rstrm);
        if (len > sizeof(AUTH_CODE_T)) {
            if (NULL != LinkInformReged) {
                LinkInformReged(_FAILURE);
            }
            return;
        }
        if (s_rx_channeltype == RX_FRAME_TCP || s_rx_channeltype == RX_FRAME_UDP) {
            if (!DAL_PP_ReadParaByID(PP_ID_AUTHCODE1, (INT8U *)&authcode, sizeof(AUTH_CODE_T))) {
                YX_MEMSET((INT8U *)&authcode, 0, sizeof(AUTH_CODE_T));
            }
            if (len > 1) {
                YX_ReadDATA_Strm(&s_rstrm, authcode.authcode, len);
                if (len < sizeof(AUTH_CODE_T)) {
                    authcode.authcode[len] = '\0';
                }
            }   
            DAL_PP_StoreParaByID(PP_ID_AUTHCODE1, (INT8U *)&authcode, sizeof(AUTH_CODE_T));
            
            if (YX_STRLEN((char *)authcode.authcode) > 0) {
                if (NULL != LinkInformReged) {
                    LinkInformReged(_SUCCESS);
                }
            } else {
                if (SendLogOutFrame()) {
                    YX_RegInform_Logout(LOGOUT_AUTO, 0);
                } else {
                    InformMsg_AuthcodeLost();  
                }
            }
        } else if (s_rx_channeltype == RX_FRAME_TCP2 || s_rx_channeltype == RX_FRAME_UDP2) {
            if (!DAL_PP_ReadParaByID(AUTHCODE2_, (INT8U *)&authcode, sizeof(AUTH_CODE_T))) {
                YX_MEMSET((INT8U *)&authcode, 0, sizeof(AUTH_CODE_T));
            }
            if (len > 1) {
                YX_ReadDATA_Strm(&s_rstrm, authcode.authcode, len);
                if (len < sizeof(AUTH_CODE_T)) {
                    authcode.authcode[len] = '\0';
                }
            } 
            DAL_PP_StoreParaByID(AUTHCODE2_, (INT8U *)&authcode, sizeof(AUTH_CODE_T));
            if (YX_STRLEN((char *)authcode.authcode) > 0) {
                if (NULL != LinkInformReged) {
                    LinkInformReged(_SUCCESS);
                }
            } else {
                if (SendLogOutFrame()) {
                    YX_RegInform_Logout(LOGOUT_AUTO, 0);
                } else {
                    InformMsg_AuthcodeLost();  
                }
            }            
        }
    } else {
        if (NULL != LinkInformReged) {
            LinkInformReged(_FAILURE);
        }
    }
}

/*******************************************************************
** 函数名:     YX_Logoutbyhand_Ack
** 函数描述:   手动终端注销确认处理
** 参数:       无
** 返回:       无
********************************************************************/
void YX_Logoutbyhand_Ack(void)
{
    AUTH_CODE_T auinfo;
    TEL_T mytel;

    YX_MEMSET(&mytel, 0, sizeof(mytel));
    DAL_PP_StoreParaByID(PP_ID_MYTEL, (INT8U *)&mytel, sizeof(mytel));
    
    YX_MEMSET((INT8U*)&auinfo, 0x00, sizeof(auinfo));            
    auinfo.authcode[0] = '\0';
    DAL_PP_StoreParaByID(PP_ID_AUTHCODE1, (INT8U *)&auinfo, sizeof(auinfo));
    DAL_PP_StoreParaByID(AUTHCODE2_, (INT8U *)&auinfo, sizeof(auinfo));
}
/*******************************************************************
** 函数名:      YX_RegInform_Logout
** 函数描述:    注册"收到注销应答后的通知函数"
** 参数:        [in]  type:             注销方式
                [in]  fp:               通知函数
** 返回:        void
********************************************************************/
void YX_RegInform_Logout(INT8U type, void (*fp)(void))
{
    if (type == LOGOUT_AUTO) {
        s_logout_inform.inform_logout_auto = fp;
    } else if (type == LOGOUT_BYHAND) {
        s_logout_inform.inform_logout_byhand = fp;
    }
}

/*******************************************************************
** 函数名:     HdlMsg_DN_ACK_COMMON
** 函数描述:   处理中心下发的平台通用应答
** 参数:       无
** 返回:       无
********************************************************************/
static void HdlMsg_DN_ACK_COMMON(void)
{
    INT16U  ackflowseq, acktypeid;    	
    INT8U   result;

    ackflowseq = YX_ReadHWORD_Strm_APP(&s_rstrm);
    acktypeid = YX_ReadHWORD_Strm_APP(&s_rstrm);
    result = YX_ReadBYTE_Strm(&s_rstrm);

#if DEBUG_GSMSTATUS > 0
    printf_com("<ackflowseq:0x%x, acktypeid:0x%x,result:%d>\r\n", ackflowseq, acktypeid, result);
#endif

    switch (acktypeid) {
        case UP_CMD_AULOG: 
            if (result == 0) {
                if (s_rx_channeltype == RX_FRAME_TCP) {
                    YX_JTT1_LinkInformLoged(_SUCCESS); 
                    #if EN_RESEND > 0
                    YX_CanResendInformer();
                    #endif
                    #if EN_RUNRECORD > 0
                    YX_DriverLogCanResendInform();
                    #endif
                } else {
                    YX_JTT2_LinkInformLoged(_SUCCESS); 
                }
            } else {
                if (s_rx_channeltype == RX_FRAME_TCP) {
                    YX_JTT1_LinkInformLoged(_FAILURE); 
                } else if (s_rx_channeltype == RX_FRAME_TCP2) {
                    YX_JTT2_LinkInformLoged(_FAILURE); 
                }
            }     
            return;
        case UP_CMD_HEART:
            if (result == 0) {
                if (s_rx_channeltype == RX_FRAME_TCP) {
                    YX_JTT1_LinkInformQuery(_SUCCESS); 
                } else if (s_rx_channeltype == RX_FRAME_TCP2) {
                    YX_JTT2_InformRecvQueryAck(_SUCCESS);
                }
            } else {
                if (s_rx_channeltype == RX_FRAME_TCP) {
                    YX_JTT1_LinkInformQuery(_FAILURE); 
                } else if (s_rx_channeltype == RX_FRAME_TCP2) {
                    YX_JTT2_InformRecvQueryAck(_FAILURE);
                }
            }        
            return;
        case UP_CMD_REG_UNREGIST:
            if (result == 0) {
                if (s_rx_channeltype == RX_FRAME_TCP) {
                    DAL_TCP_ClearBuf(SOCKET_CH_0);                
                } else if (s_rx_channeltype == RX_FRAME_TCP2) {
                    DAL_TCP_ClearBuf(CHA_JT2_TLINK);
                }
                if (NULL != s_logout_inform.inform_logout_auto) {
                    s_logout_inform.inform_logout_auto();
                    s_logout_inform.inform_logout_auto = NULL;
                }
                if (NULL != s_logout_inform.inform_logout_byhand) {            
                    s_logout_inform.inform_logout_byhand();
                    s_logout_inform.inform_logout_byhand = NULL;
                }
                #if 0
                RegoutParaChange();
                #endif
                #if EN_120R > 0  && EN_DEBUG > 0
                if (YX_120R_IsON()) {
                    YX_120R_DisplayText((INT8U *)"注销成功", 8, TEXT_120R_ALARM);
                }
                #endif                
                #if EN_HANDSET > 0 && EN_DEBUG > 0
                if (YX_HST_IsON()) {
                    YX_MMI_PromptWin_Hit((char *)"注销成功");
                }
                #endif
            }
            return;             
        #if EN_RUNRECORD > 0
        case UP_CMD_DRIVERINFO:
            YX_InformMsg_LoginCheckAck(ackflowseq, result);
            break;  
            #endif
        #if EN_120R > 0 || EN_120ND70 > 0
        case UP_CMD_EVENTREPORT:
            YX_InformMsg_EventAck(ackflowseq, result);
            YX_InformMsgND_EventAck(ackflowseq, result);
            break;
        case UP_CMD_INFOSWITCH:
            YX_InformMsg_InfoSwitchAck(ackflowseq, result);
            YX_InformMsgND_InfoSwitchAck(ackflowseq, result);
            break;
        case UP_ACK_ASK:
            YX_InformMsg_AskAck(ackflowseq, result);
            YX_InformMsgND_AskAck(ackflowseq, result);
            break;
        case UP_CMD_NEAR_INFOQRYREQ:
            YX_InformMsg_AroundAck(ackflowseq, result);
            YX_InformMsgND_AroundAck(ackflowseq, result);
            break;
        case UP_CMD_EBILL:
            YX_InformMsg_EbillAck(ackflowseq, result);
            YX_InformMsgND_EbillAck(ackflowseq, result);
            break;
        #endif
        case UP_CMD_GPS_INFO:
            if (result == _ALMACK) {                                           /*报警确认应答*/
                YX_AlarmAck(_ACKSHIELD_FLAG, 0);
            }
            break;
        default:
            break;
    }
    #if CUR_VERSION == VER_JTB_VER1
    if (acktypeid == UP_CMD_PICAUDIO)  {
        #if EN_DC_SNAP > 0
        YX_MFSend_RecvCommAck(ackflowseq, result);
        return;
        #endif
    }    
    #endif
    YX_WIN_InformRecvComAck(s_hdlframe, s_hdlframelen + SYSHEAD_LEN + SYSTAIL_LEN);  
    if (s_rx_channeltype == RX_FRAME_TCP) {
        YX_JTT1_LinkInformQuery(_SUCCESS);
    } else if (s_rx_channeltype == RX_FRAME_TCP2) {
        YX_JTT2_InformRecvQueryAck(_SUCCESS);
    }
}

/*******************************************************************
** 函数名:     HdlMsg_DN_CMD_TRANSDATA
** 函数描述:   处理中心下发的透传协议帧
** 参数:       无
** 返回:       无
********************************************************************/
static void HdlMsg_DN_CMD_TRANSDATA(void)
{
    INT8U   tmsgtype;
    BOOLEAN result;

    result = ACK_SUCCESS;
    
#if EN_IC > 0
    if (s_channeltype == (SM_ATTR_TCP | (CHA_IC_TLINK << 24))) {                /* 认证中心通道 */
        YX_IC_IdentifyAck(YX_GetStrmPtr(&s_rstrm), YX_GetStrmLeftLen(&s_rstrm));
        YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, result);
        return;
    }
#endif

    tmsgtype = YX_ReadBYTE_Strm(&s_rstrm);

    YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, result);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_CMD_TEXTINFO
** 函数描述:   MSG_0x8300 文本信息下发
** 参数:       无
** 返回:       无
********************************************************************/
static void HdlMsg_DN_CMD_TEXTINFO(void)
{
    INT8U flag;
    #if EN_TTS > 0
    INT8U *ptr;
    #endif
    INT16U len;

    flag = YX_ReadBYTE_Strm(&s_rstrm);

    len = YX_GetStrmLeftLen(&s_rstrm);
    if (len == 0) {
        YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, ACK_ERR);
        return;
    } 
    
#if EN_120ND70> 0
    if (YX_120ND70_IsON()) {
        YX_Inform120ND70_OtherMsg(flag, YX_GetStrmPtr(&s_rstrm), len);
    }
#endif
#if EN_120R> 0
    if (YX_120R_IsON()) {
        YX_Inform120R_OtherMsg(flag, YX_GetStrmPtr(&s_rstrm), len);
    }
#endif
    if ((flag & 0x04) == 0x04 || (flag & 0x01) == 0x01) {  /* 终端显示器显示 */
        #if EN_120R > 0
        if (YX_120R_IsON()) {
            YX_Inform120R_Attemper(YX_GetStrmPtr(&s_rstrm), len);
        }
        #endif   
        #if EN_120ND70> 0
        if (YX_120ND70_IsON()) {
            YX_Inform120ND70_Attemper(YX_GetStrmPtr(&s_rstrm), len);
   	    }
        #endif   
    }
    if ((flag & 0x08) == 0x08 || (flag & 0x01) == 0x01) {  //最长500字节  TTS_MAX_SIZE才140
        #if EN_TTS > 0
        if (len > TTS_MAX_SIZE) {
            len = TTS_MAX_SIZE;
        }
        if ((ptr = (INT8U *)YX_DYM_Alloc(len + 12)) == NULL) {
            return;
        }
        YX_MEMCPY(ptr, 12, (INT8U*)"收到文本信息", 12);
        YX_PlayTts(TTS_TEXT, ptr, 12, P_LOW);
        YX_MEMCPY(ptr, TTS_MAX_SIZE, YX_GetStrmPtr(&s_rstrm), len);
        YX_PlayTts(TTS_TEXT, ptr, len, P_LOW);
        YX_DYM_Free(ptr);
        #endif
    }
    YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, ACK_SUCCESS);
}
#if EN_120R > 0
/*******************************************************************
** 函数名:     SetEvent
** 函数描述:   设置事件类型
** 参数:       [in]attr:设置类型
**             [in]ptr:指向配置参数的指针
**             [in]datalen:数据长度
** 返回:       无
********************************************************************/
static BOOLEAN SetEvent(INT8U attr, INT8U *ptr, INT16U len)
{
    INT8U i, totalnum;
    STREAM_T strm;
    INT16U templen;

    if (attr > 0x04) {
        return false;    //设置类型错误
    }

    /* 删除所有 */
    if (attr == 0x00) {     
        YX_MEMSET((INT8U *)&s_event, 0, sizeof(DB_EVENTLIST_T));
        YX_DB_AddRec(DB_EVENTMENU, true, (INT8U *)&s_event, sizeof(DB_EVENTLIST_T));
        return true;
    }

    /* 更新 追加 修改 事件 */ 
    YX_InitStrm(&strm, ptr, len);
    YX_MEMSET((INT8U *)&s_event, 0, sizeof(DB_EVENTLIST_T));                   //清空所有
        
    totalnum = YX_ReadBYTE_Strm(&strm);
    if (totalnum > MAX_EVENTLIST) {                                            //超过最大设置个数  
        return false;
    }

    s_event.settype = attr;
    s_event.totalnum = totalnum;

    for (i = 0; i < totalnum; i++) {
        s_event.eventitem[i].id = YX_ReadBYTE_Strm(&strm);

        templen = YX_ReadBYTE_Strm(&strm);
        if (s_event.settype == 0x04) {
            s_event.eventitem[i].len = 0;
            YX_MEMSET(s_event.eventitem[i].event, 0x20, 20);
            YX_MovStrmPtr(&strm, templen);
            continue;
        }
        
        if (YX_GetStrmLeftLen(&strm) < templen) {
            return false;
        }
        //解析事件类型
        s_event.eventitem[i].len  = templen;
        if (s_event.eventitem[i].len > 20) {
            s_event.eventitem[i].len = 20;
        }
        YX_MEMCPY(s_event.eventitem[i].event, 20, YX_GetStrmPtr(&strm), s_event.eventitem[i].len);
        YX_MEMSET(&s_event.eventitem[i].event[s_event.eventitem[i].len], 0x20, 20 - s_event.eventitem[i].len);
        YX_MovStrmPtr(&strm, templen);
    }

    YX_DB_AddRec(DB_EVENTMENU, true, (INT8U *)&s_event, sizeof(DB_EVENTLIST_T));
    return true;
}

/*******************************************************************
** 函数名:     SetAsk
** 函数描述:   设置提问内容
** 参数:       [in]ptr:指向配置参数的指针
**             [in]len:数据长度
** 返回:       无
********************************************************************/
static BOOLEAN SetAsk(INT16U seq, INT8U *ptr, INT16U len, ASK_T *ask)
{
    INT8U    i, templen;
    INT16U   anslen, tlen;
    STREAM_T strm;
    
    YX_InitStrm(&strm, ptr, len);
    if (ask == NULL) {
        return false;
    }

    //解析提问内容字符串
    templen = YX_ReadBYTE_Strm(&strm);
    if (YX_GetStrmLeftLen(&strm) < templen) {
        return false;
    }
    ask->asklen = templen;
    if (ask->asklen == 0) {
        return false;
    } 
    if (ask->asklen > MAX_ASKLEN) {
        ask->asklen = MAX_ASKLEN;
    }
    YX_MEMCPY(ask->ask, MAX_ASKLEN, YX_GetStrmPtr(&strm), ask->asklen);
    YX_MEMSET(&ask->ask[ask->asklen], 0x20, MAX_ASKLEN - ask->asklen);
    YX_MovStrmPtr(&strm, templen);

    for (i = 0; i < MAX_ANS; i++) {
        //解析答案列表
        if (YX_GetStrmLeftLen(&strm) < 3) {
            break;
        }
        ask->ans[i].id = YX_ReadBYTE_Strm(&strm);
        tlen = YX_ReadHWORD_Strm_APP(&strm);
        if (YX_GetStrmLeftLen(&strm) < tlen) { 
            break;
        }
        anslen = tlen;
        if (anslen == 0) {
            break;
        } 
        if (anslen > 20) {
            anslen = 20;
        }
        ask->ans[i].len[0] = anslen >> 8;
        ask->ans[i].len[1] = anslen & 0xff;
        
        YX_MEMCPY(ask->ans[i].ans, 20, YX_GetStrmPtr(&strm), anslen);
        YX_MEMSET(&ask->ans[i].ans[anslen], 0x20, 20 - anslen);
        YX_MovStrmPtr(&strm, tlen);

        ask->ansnum++;
    }

    ask->seq[0] = seq >> 8;
    ask->seq[1] = seq & 0xff;
    ask->valid = true; 
    return true;
}

/*******************************************************************
** 函数名:     SetInfoMenu
** 函数描述:   设置信息点播菜单
** 参数:       [in]attr:设置类型
**             [in]ptr:指向配置参数的指针
**             [in]datalen:数据长度
** 返回:       无
********************************************************************/
static BOOLEAN SetInfoMenu(INT8U attr, INT8U *ptr, INT16U len)
{
    INT8U i, totalnum;
    STREAM_T strm;
    INT16U templen, menulen;

    if (attr > 0x03) {
        return false;    //设置类型错误
    }
    /* 删除所有 */
    if (attr == 0x00) {     
        YX_MEMSET((INT8U *)&s_info, 0, sizeof(DB_INFOMENU_T));
        YX_DB_AddRec(DB_INFOMENU, true, (INT8U *)&s_info, sizeof(DB_INFOMENU_T));
        return true;
    }

    /* 更新 追加 修改 */ 
    YX_InitStrm(&strm, ptr, len);
    YX_MEMSET((INT8U *)&s_info, 0, sizeof(DB_INFOMENU_T));  //清空所有
    
    totalnum = YX_ReadBYTE_Strm(&strm);
    if (totalnum > MAX_INFOLIST) {                       //超过最大设置个数  
        return false;
    }

    s_info.settype = attr;
    s_info.totalnum = totalnum;
    
    for (i = 0; i < totalnum; i++) {
        s_info.menuitem[i].id = YX_ReadBYTE_Strm(&strm);

        templen = YX_ReadHWORD_Strm_APP(&strm);
        if (YX_GetStrmLeftLen(&strm) < templen) {
            return false;
        }
        //解析信息菜单内容
        menulen = templen;
        if (menulen > 20) {
            menulen = 20;
        }
        s_info.menuitem[i].len[0] = menulen >> 8;
        s_info.menuitem[i].len[1] = menulen & 0xff;
        
        YX_MEMCPY(s_info.menuitem[i].menu, 20, YX_GetStrmPtr(&strm), menulen);
        YX_MEMSET(&s_info.menuitem[i].menu[menulen], 0x20, 20 - menulen);
        YX_MovStrmPtr(&strm, templen);
    }
    
    YX_DB_AddRec(DB_INFOMENU, true, (INT8U *)&s_info, sizeof(DB_INFOMENU_T));
    return true;
}

/*******************************************************************
** 函数名:     AddNewTel
** 函数描述:   添加单个联系人
** 参数:       [in]ptr:指向配置参数的指针
**             [out]dtel:目标电话本
** 返回:       添加的数据长度
********************************************************************/
static INT8U AddNewTel(TELBOOK_ITEM_T *dtel,  INT8U *stel, INT16U len)
{
    STREAM_T strm;
    INT8U templen;
    INT8U addlen;
    
    YX_InitStrm(&strm, stel, len);
    dtel->valid = true;
    dtel->attr = YX_ReadBYTE_Strm(&strm);
    addlen = 1;
    #if DEBUG_GSMSTATUS > 0
    //printf_com("\r\n AddNewTel attr %d :\r\n", dtel->attr);
    //printf_hex(YX_GetStrmPtr(&strm),YX_GetStrmLeftLen(&strm));
    //printf_com("\r\n");
    #endif
    
    if (dtel->attr > 0x03) return 0;

    //解析电话号码字符串
    templen = YX_ReadBYTE_Strm(&strm);
    if (YX_GetStrmLeftLen(&strm) < templen) {
        return 0;
    }
    dtel->tellen = templen;
    if (dtel->tellen > MAX_TELLEN) {
        dtel->tellen = MAX_TELLEN;
    }
    YX_MEMCPY(dtel->tel, MAX_TELLEN, YX_GetStrmPtr(&strm), dtel->tellen);
    YX_MEMSET(&dtel->tel[dtel->tellen], 0x20, MAX_TELLEN - dtel->tellen);
    YX_MovStrmPtr(&strm, templen);
    addlen += templen + 1;

    //解析联系人字符串
    templen = YX_ReadBYTE_Strm(&strm);
    if (YX_GetStrmLeftLen(&strm) < templen) {
        return 0;
    }
    dtel->namelen = templen;
    if (dtel->namelen > MAX_NAMELEN) {
        dtel->namelen = MAX_NAMELEN;
    }
    YX_MEMCPY(dtel->name, MAX_NAMELEN, (INT8U *)YX_GetStrmPtr(&strm), dtel->namelen);
    YX_MEMSET(&dtel->name[dtel->namelen], 0x20, MAX_NAMELEN - dtel->namelen);
    YX_MovStrmPtr(&strm, templen);
    addlen += templen + 1;

    return addlen;
}

/*******************************************************************
** 函数名:     SetTelBook
** 函数描述:   设置呼叫限制电话本
** 参数:       [in]ptr:指向配置参数的指针
**             [in]datalen:数据长度
** 返回:       无
********************************************************************/
static BOOLEAN SetTelBook(INT8U attr, INT8U *ptr, INT16U len)
{
    INT8U i, j, k, totalnum, seq;
    STREAM_T strm;
    TELBOOK_ITEM_T stel;
    INT8U result;
    INT16U addlen;

    if (!DAL_PP_ReadParaByID(TELBOOK_, (INT8U *)&s_telbook, sizeof(TELBOOK_T))) {
        #if DEBUG_GSMSTATUS > 0
        printf_com("\r\n TELBOOK_T invalid\r\n");
        #endif
        YX_MEMSET((INT8U *)&s_telbook, 0, sizeof(TELBOOK_T));
    }
    
    if (attr == 0x00) {     /* 删除所有 */
        seq = s_telbook.seq;
        if (++seq == 0) {                             /* 流水号为0表示没有记录 */ 
            seq++;
        }
        YX_MEMSET((INT8U *)&s_telbook, 0, sizeof(TELBOOK_T));
        s_telbook.seq = seq;
        DAL_PP_StoreParaByID(TELBOOK_, (INT8U *)&s_telbook, sizeof(TELBOOK_T));
        return true;
    }

    YX_InitStrm(&strm, ptr, len);
    totalnum = YX_ReadBYTE_Strm(&strm);

    #if DEBUG_GSMSTATUS > 0
    printf_com("\r\n SetTelBook totalnum %d :\r\n", totalnum);
    //printf_hex(YX_GetStrmPtr(&strm), YX_GetStrmLeftLen(&strm));
    #endif

    if (totalnum > MAX_TELBOOK) {                           //超过最大设置个数  
        return false;
    }
    
    k = 0;
    result = TRUE;    
    switch (attr)
    {
        case 0x01: /* 更新电话本 */  
        case 0x02: /* 追加电话本 */
            if (attr == 0x01) { 
                YX_MEMSET((INT8U *)&s_telbook.totalnum, 0, sizeof(TELBOOK_T) - 1);  //清空所有,流水号不能清 
            } else {
                if (s_telbook.totalnum >= MAX_TELBOOK) {
                    return false;
                }
                if (totalnum > (MAX_TELBOOK - s_telbook.totalnum)) {
                    totalnum = MAX_TELBOOK - s_telbook.totalnum;
                }
            }
            for (j =0, i = 0; i < totalnum; i++) {
                if (s_telbook.totalnum > MAX_TELBOOK) {
                    return false;
                }
                addlen = AddNewTel(&s_telbook.telitem[s_telbook.totalnum], YX_GetStrmPtr(&strm), YX_GetStrmLeftLen(&strm));
                #if DEBUG_GSMSTATUS > 0
                printf_com("\r\naddlen %d:\r\n", addlen);
                #endif
                
                if (addlen != 0) {
                    YX_MovStrmPtr(&strm, addlen);
                    s_telbook.totalnum++;
                } else {
                    j++;
                }
            }
            if (j == totalnum) {
                result = FALSE;
            } 
            break;
        case 0x03: /* 修改电话本 */
            //if (s_telbook.totalnum == 0) {                          //没有可修改的记录
            //    return false;
            //}
            result = FALSE;
            for (i = 0; i < totalnum; i++) {
                addlen = AddNewTel(&stel, YX_GetStrmPtr(&strm), YX_GetStrmLeftLen(&strm));
                #if DEBUG_GSMSTATUS > 0
                printf_com("\r\naddlen %d:\r\n", addlen);
                #endif
                if (addlen != 0) {
                    YX_MovStrmPtr(&strm, addlen);
                    // 查找可修改的记录
                    if (s_telbook.totalnum > MAX_TELBOOK) {
                        return FALSE;
                    }
                    for (j = 0; j < s_telbook.totalnum; j++) {
                        if (s_telbook.telitem[j].valid == true) {
                            if (!YX_CmpString((INT8U *)s_telbook.telitem[j].name, (INT8U *)stel.name, MAX_NAMELEN)) {
                                s_telbook.telitem[j].attr = stel.attr;
                                s_telbook.telitem[j].tellen = stel.tellen;
                                s_telbook.telitem[j].namelen = stel.namelen;
                                YX_MEMCPY((INT8U *)s_telbook.telitem[j].tel, MAX_TELLEN, (INT8U *)stel.tel, MAX_TELLEN);
                                YX_MEMCPY((INT8U *)s_telbook.telitem[j].name, MAX_NAMELEN, (INT8U *)stel.name, MAX_NAMELEN);
                                k++;
                                break;
                            }
                        }
                    }
                    
                    if (j == s_telbook.totalnum) {
                        YX_MEMCPY((INT8U *)&s_telbook.telitem[j].valid, sizeof(TELBOOK_ITEM_T), (INT8U *)&stel.valid, sizeof(TELBOOK_ITEM_T));
                        s_telbook.totalnum++;
                        k++;
                    }                    
                }
            }
            if (k != 0) {
                result = TRUE;
            }
            break;
        default:
            break;
    }

    #if DEBUG_GSMSTATUS > 0
    printf_com("\r\n result %d: total num %d\r\n", result, s_telbook.totalnum);
    #endif
    
    if (result == TRUE) {
        if (++s_telbook.seq == 0) {                             /* 流水号为0表示没有记录 */ 
            s_telbook.seq++;
        }
        DAL_PP_StoreParaByID(TELBOOK_, (INT8U *)&s_telbook, sizeof(TELBOOK_T));
    }
    return result;
}

/*******************************************************************
** 函数名:     SetInfoContent
** 函数描述:   设置信息服务内容
** 参数:       [in]ptr:指向配置参数的指针
**             [in]datalen:数据长度
** 返回:       无
********************************************************************/
static BOOLEAN SetInfoContent(INT8U infotype, INT8U *dataptr, INT16U datalen)
{
    INT8U    i;
    INT16U   infolen;
    STREAM_T strm;
    INFOSERVER_T infoser;
    
    YX_InitStrm(&strm, dataptr, datalen);
    
    YX_MEMSET(&infoser, 0, sizeof(infoser));
    YX_DelAllRecordSaveEntry(RECSAVE_TYPE_INFOS);

    infoser.valid = true;
    infoser.infotype = infotype;
    infolen = YX_ReadHWORD_Strm_APP(&strm);
    
    for (i = 0; i < MAX_INFOSERVER; i++) {
        if (YX_GetStrmLeftLen(&strm) > MAX_INFOSERVERLEN) {
            infoser.infolen = MAX_INFOSERVERLEN;
            YX_ReadDATA_Strm(&strm, infoser.info, infolen);
            YX_AddRecordSaveEntry(RECSAVE_TYPE_INFOS, sizeof(INFOSERVER_T), (INT8U*)&infoser);
        } else if (YX_GetStrmLeftLen(&strm) > 0) {
            infoser.infolen = YX_GetStrmLeftLen(&strm);
            YX_ReadDATA_Strm(&strm, infoser.info, YX_GetStrmLeftLen(&strm));
            YX_AddRecordSaveEntry(RECSAVE_TYPE_INFOS, sizeof(INFOSERVER_T), (INT8U*)&infoser);
            break;
        }
    }
    return true;
}

/*******************************************************************
** 函数名:     HdlMsg_DN_CMD_EVENTSET
** 函数描述:   MSG_0x8301 事件设置
** 参数:       无
** 返回:       无
********************************************************************/
static void HdlMsg_DN_CMD_EVENTSET(void)
{
    INT8U settype, ack;
    INT16U len;

    settype = YX_ReadBYTE_Strm(&s_rstrm);  

    if (settype > 0x04) {       //设置类型错误
        YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, ACK_ERR);
        return;    
    }
    
    #if EN_120R > 0 
    if (YX_Is120RSetEvent()) {  //正在更新中
        YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, ACK_FAIL);
        return;
    } 
    #endif

    if (settype == 0) {         //删除
        if (SetEvent(settype, NULL, 0)) {
            #if EN_120R > 0         
            YX_Inform120R_SetEvent();
            #endif
            #if EN_120ND70 > 0         
            YX_Inform120ND70_SetEvent();
            #endif
            ack = ACK_SUCCESS;
        } else {
            ack = ACK_FAIL;
        }
        
        YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, ack);
        return;
    }

    len = YX_GetStrmLeftLen(&s_rstrm);
    if (len < 3) {
        YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, ACK_ERR);
        return;
    }
    if (SetEvent(settype, YX_GetStrmPtr(&s_rstrm), len)) {
        #if EN_120R > 0         
        YX_Inform120R_SetEvent();
        #endif
        #if EN_120ND70 > 0         
        YX_Inform120ND70_SetEvent();
        #endif
        ack = ACK_SUCCESS;
    } else {
        ack = ACK_ERR;
    }
    
    YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, ack);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_CMD_ASK
** 函数描述:   MSG_0x8302 提问下发
** 参数:       无
** 返回:       无
********************************************************************/
static void HdlMsg_DN_CMD_ASK(void)
{
    INT8U flag, i, *temptr, cnt, j;
    INT16U len, seq;
    ASK_T ask;
    STREAM_T wstrm;
    
    seq = YX_GetFlowseq_SYSFrame(s_hdlframe);
    flag = YX_ReadBYTE_Strm(&s_rstrm);
    len = YX_GetStrmLeftLen(&s_rstrm);
    if (len == 0) {
        YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, ACK_ERR);
        return;
    } 

    YX_MEMSET(&ask, 0, sizeof(ask));
    if (!SetAsk(seq, YX_GetStrmPtr(&s_rstrm), len, &ask)) {
        YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, ACK_FAIL);
        return;
    }

    if ((temptr = YX_RecordSave_read(RECSAVE_TYPE_ASK, 0)) != NULL) {
        YX_ModifyRecordSaveEntry(RECSAVE_TYPE_ASK, 0, sizeof(ASK_T), (INT8U*)&ask);
    } else {
        YX_AddRecordSaveEntry(RECSAVE_TYPE_ASK, sizeof(ASK_T), (INT8U*)&ask);
    } 
    YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, ACK_SUCCESS);
    
    if ((flag & 0x01) || (flag & 0x04)) {  /* 紧急\终端显示器显示 */
        #if EN_120R > 0
        if (YX_120R_IsON()) {
            YX_Inform120R_SetAsk();
   	    }
        #endif
        #if EN_120ND70 > 0
        if (YX_120ND70_IsON()) {
            YX_Inform120ND70_SetAsk();
   	    }
        #endif
    }
    if ((flag & 0x01) || (flag & 0x08)) {  //最长500字节  TTS_MAX_SIZE才140
        #if EN_TTS > 0
        YX_PlayTts(TTS_TEXT, (INT8U*)"收到提问", 8, P_LOW);
        if (ask.asklen <= TTS_MAX_SIZE) {
            YX_PlayTts(TTS_TEXT, ask.ask, ask.asklen, P_LOW);
        } else {
            YX_PlayTts(TTS_TEXT, ask.ask, TTS_MAX_SIZE, P_LOW);
        }
        temptr = NULL;
        temptr = YX_DYM_Alloc(TTS_MAX_SIZE);
        if (temptr == NULL) {
            return;
        }
        YX_InitStrm(&wstrm, temptr, TTS_MAX_SIZE);
        if (ask.ansnum <= 5) {
            cnt = ask.ansnum;
        } else {
            cnt = 5;
        }
        for (j = 0; j < 2; j++) {
            for (i = 0; i < cnt ; i++) {
                YX_WriteDATA_Strm(&wstrm, (INT8U*)"答案", 4);
                YX_WriteBYTE_Strm(&wstrm, ('1' + i + j*5));
                YX_WriteBYTE_Strm(&wstrm, '?');
                len = ask.ans[i + j*5].len[0] << 8;
                len += ask.ans[i + j*5].len[1];
                YX_WriteDATA_Strm(&wstrm, ask.ans[i + j*5].ans, len);
            }
            if (YX_GetStrmLen(&wstrm) <= TTS_MAX_SIZE) {
                YX_PlayTts(TTS_TEXT, temptr, YX_GetStrmLen(&wstrm), P_LOW);
            } else {
                YX_PlayTts(TTS_TEXT, temptr, TTS_MAX_SIZE, P_LOW);
            }
            if (ask.ansnum > 5) {
                cnt = ask.ansnum - 5;
                YX_InitStrm(&wstrm, temptr, TTS_MAX_SIZE);
            } else {
                cnt = 0;
                break;
            }
        }
        if (temptr != NULL) {
            YX_DYM_Free(temptr);
        }
        #endif
    }    
}

/*******************************************************************
** 函数名:     HdlMsg_DN_CMD_INFOMENU
** 函数描述:   MSG_0x8303 信息点播设置
** 参数:       无
** 返回:       无
********************************************************************/
static void HdlMsg_DN_CMD_INFOMENU(void)
{
    INT8U settype, ack;
    INT16U len;

    settype = YX_ReadBYTE_Strm(&s_rstrm);  
    len = YX_GetStrmLeftLen(&s_rstrm);

    if (settype > 0x03) {//设置类型错误
        YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, ACK_ERR);
        return;    
    }
    
    #if EN_120R > 0 
    if (YX_Is120RSetInfoMenu()) {  //正在更新中
        YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, ACK_FAIL);
        return;
    } 
    #endif

    if (settype == 0) {         //删除
        if (SetInfoMenu(settype, NULL, 0)) {
            #if EN_120R > 0         
            YX_Inform120R_SetInfoMenu();
            #endif
            #if EN_120ND70 > 0         
            YX_Inform120ND70_SetInfoMenu();
            #endif
            ack = ACK_SUCCESS;
        } else {
            ack = ACK_FAIL;
        }
        
        YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, ack);
        return;
    }

    len = YX_GetStrmLeftLen(&s_rstrm);
    if (len < 4) {                //至少要有一项
        YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, ACK_ERR);
        return;
    }

    if (SetInfoMenu(settype, YX_GetStrmPtr(&s_rstrm), len)) {
        #if EN_120R > 0         
        YX_Inform120R_SetInfoMenu();
        #endif
        #if EN_120ND70 > 0         
        YX_Inform120ND70_SetInfoMenu();
        #endif
        ack = ACK_SUCCESS;
    } else {
        ack = ACK_ERR;
    }

    YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, ack);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_CMD_INFOSEVER
** 函数描述:   MSG_0x8304 信息服务
** 参数:       无
** 返回:       无
********************************************************************/
static void HdlMsg_DN_CMD_INFOSEVER(void)
{
    INT8U infotype, ack;
    INT16U len;

    infotype = YX_ReadBYTE_Strm(&s_rstrm);  
    len = YX_GetStrmLeftLen(&s_rstrm);

    if (len == 0) {
        YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, ACK_ERR);
        return;
    } 

    if (SetInfoContent(infotype, YX_GetStrmPtr(&s_rstrm), len)) {
        #if EN_120R > 0         
        YX_Inform120R_SendInfoContent();
        #endif
        #if EN_120ND70 > 0         
        YX_Inform120ND70_SendInfoContent();
        #endif
        ack = ACK_SUCCESS;
    } else {
        ack = ACK_ERR;
    }

    YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, ack);
}


static void DelayCallBackProc(void *data)
{
    YX_RemoveTmr(s_delaycalltmr);
    s_delaycalltmr = 0;
    YX_120R_DisplayText((INT8U *)"收到中心电话回拨指令", 20, TEXT_120R_COMMON);
    DAL_DialPhone(AUDIO_CHANNEL_FREE, s_tmptel, s_tmplen);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_CMD_CALLBACKTEL
** 函数描述:   MSG_0x8400 电话回拨接口
** 参数:       无
** 返回:       无
********************************************************************/
static void HdlMsg_DN_CMD_CALLBACKTEL(void)
{
    INT8U res, type;
    //YX_PHONE_RESULT_E result;
    
    type = YX_ReadBYTE_Strm(&s_rstrm);
    switch (type)
    {
        case 0x00:
            if (!YX_AlarmerPermitTalk()) {
                res = ACK_FAIL;
                break;
            }
            
            YX_MEMSET(s_tmptel, 0x0, sizeof(s_tmptel));
            s_tmplen = YX_GetStrmLeftLen(&s_rstrm);
            if (s_tmplen > sizeof(s_tmptel)) {
                res = ACK_FAIL;
            } else {
                if (s_delaycalltmr == 0) { 
                    YX_MEMCPY(s_tmptel, sizeof(s_tmptel), YX_GetStrmPtr(&s_rstrm), s_tmplen);
                    s_delaycalltmr = YX_InstallTmr(PRIO_OPTTASK, 0, DelayCallBackProc);
                    YX_StartTmr(s_delaycalltmr, PERIOD_DELAY);
                }
                res = ACK_SUCCESS;
            }
            break;
        case 0x01:
            /*由于石家庄的检测平台下的电话号码是空号，因此这里指定自己的手机号*/
            if (YX_StartAutoDial(AUDIO_CHANNEL_LISTEN, YX_GetStrmLeftLen(&s_rstrm), YX_GetStrmPtr(&s_rstrm), 10)) {
                res = ACK_SUCCESS;
            } else {
                res = ACK_FAIL;
            }  
            break;
        default:
            res = ACK_FAIL;
            break;
    }
    YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, res);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_CMD_SETTELNOTE
** 函数描述:   MSG_0x8401 设置电话本
** 参数:       无
** 返回:       无
********************************************************************/
static void HdlMsg_DN_CMD_SETTELNOTE(void)
{
    INT8U attr, ack;
    INT16U len;

    ack = ACK_SUCCESS;
    
    attr = YX_ReadBYTE_Strm(&s_rstrm);  

    if (attr > 0x03) {//设置类型错误
        YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, ACK_ERR);
        return;    
    }
    
    #if EN_120R > 0 
    if (YX_Is120RSetTelBook()) {
        YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, ACK_FAIL); 
        return;
    }
    #endif

    /* 删除所有 */
    if (attr == 0) {         //删除
        if (SetTelBook(attr, NULL, 0)) {
            #if EN_120R > 0
            if (YX_120R_IsON()) {
                YX_Inform120R_ClearTelBook();
            }
            #endif
            #if EN_120ND70 > 0
            if (YX_120ND70_IsON()) {
                YX_Inform120ND70_ClearTelBook();
            }
            #endif
            ack = ACK_SUCCESS;
        } else {
            ack = ACK_FAIL;
        }
        
        YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, ack);
        return;
    }
    
    /* 更新 追加 修改 电话本 */ 
    len = YX_GetStrmLeftLen(&s_rstrm);
    if (len < 2) {                    //至少要有一项
        YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, ACK_ERR);
        return;
    }

    if (SetTelBook(attr, YX_GetStrmPtr(&s_rstrm), len)) {
        #if EN_120R > 0 
        if (YX_120R_IsON()) {
            YX_Inform120R_UpdateTelBook();
        }
        #endif
        #if EN_120ND70 > 0 
        if (YX_120ND70_IsON()) {
            YX_Inform120ND70_UpdateTelBook();
        }
        #endif
        ack = ACK_SUCCESS;
    } else {
        ack = ACK_ERR;
    }

    YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, ack);
}
#endif

#if EN_AREARANGE_ALRAM > 0
/*******************************************************************
** 函数名:     HdlMsg_DN_CMD_SETCIRCLE
** 函数描述:   MSG_0x8600 设置圆形区域
** 参数:       无
** 返回:       无
********************************************************************/
static void HdlMsg_DN_CMD_SETCIRCLE(void)
{
    INT8U   *memptr, *ptr_areaparalen, *ptr_paracnt;
    INT8U   optype, areacnt, paracnt, paratype, longitude[4], latitude[4], result;
    INT16U  memlen, i, paralen, vt_kmh;
    INT32U  dwareaid, areaparalen, dwparaattr, dwlongitude, dwlatitude, dwradius;
    STREAM_T wstrm;

    memlen = s_hdlframelen + MAX_AREA_NUM * DIFLEN_POTOCOL_RANGE + 100;                 /* 不能超过125个区域; memlen reserve 100 */
    if (NULL == (memptr = YX_DYM_Alloc(memlen))) {
        result = ACK_FAIL;
        goto RET_SETCIRCLE;
    }

    YX_InitStrm(&wstrm, memptr, memlen);
    YX_WriteDATA_Strm(&wstrm, s_def_lable, sizeof(s_def_lable));
    YX_WriteHWORD_Strm(&wstrm, memlen);

    optype = YX_ReadBYTE_Strm(&s_rstrm);
    if (optype == 1) {
        optype = APPEND_OP;
    } else if  (optype == 2) {
        optype = MODIFY_OP;
    }
    YX_WriteBYTE_Strm(&wstrm, optype);
    YX_WriteBYTE_Strm(&wstrm, CIRCLE_AREA);

    areacnt   = YX_ReadBYTE_Strm(&s_rstrm);
    YX_WriteBYTE_Strm(&wstrm, areacnt);
    if (areacnt > MAX_AREA_NUM) {                                                       /* 最大125个区域 */
        YX_DYM_Free(memptr);
        memptr = NULL;
        result = ACK_FAIL;
        goto RET_SETCIRCLE;        
    }
    //【区域id(4B)  +区域参数长度(4B) + 区域参数设置属性(4B) + 区域参数个数n1(1B) +【参数类型(1B) + 参数长度n2(2B) + 参数内容(n2)】*n1】*M
    for (i = 0; i < areacnt; i++) {
        if (YX_GetStrmLeftLen(&wstrm) < 28 || YX_GetStrmLeftLen(&s_rstrm) < 14) {
            YX_DYM_Free(memptr);
            memptr = NULL;
            result = ACK_FAIL;
            goto RET_SETCIRCLE;            
        }
    
        dwareaid = YX_ReadLONG_Strm_APP(&s_rstrm);
        YX_WriteLONG_Strm(&wstrm, dwareaid);

        areaparalen = 5;                                                                /* 5 for 区域参数设置属性(4B) + 区域参数个数n1(1B) */
        ptr_areaparalen = YX_GetStrmPtr(&wstrm);
        YX_WriteLONG_Strm(&wstrm, areaparalen);
        
        dwparaattr = YX_ReadHWORD_Strm_APP(&s_rstrm);
        YX_WriteLONG_Strm(&wstrm, dwparaattr);

        ptr_paracnt = YX_GetStrmPtr(&wstrm);
        paracnt = 1;
        YX_WriteBYTE_Strm(&wstrm, paracnt);

        paratype = AREA_BASEPARA_ID;
        paralen  = 12;
        areaparalen += (3 + paralen);                                                   /* 3 for 参数类型(1B) + 参数长度n2(2B) */
        YX_WriteBYTE_Strm(&wstrm, paratype);
        YX_WriteHWORD_Strm(&wstrm, paralen);

        dwlatitude  = YX_ReadLONG_Strm_APP(&s_rstrm);
        dwlongitude = YX_ReadLONG_Strm_APP(&s_rstrm);
        dwradius    = YX_ReadLONG_Strm_APP(&s_rstrm);
        YX_ConvertDegreeToDMMM(longitude, dwlongitude);
        YX_ConvertDegreeToDMMM(latitude, dwlatitude);

        YX_WriteDATA_Strm(&wstrm, longitude, sizeof(longitude));
        YX_WriteDATA_Strm(&wstrm, latitude, sizeof(latitude));
        YX_WriteLONG_Strm(&wstrm, dwradius);

        if (dwparaattr & EN_AREA_TIMERANGE) {
            paracnt++;
            paratype = AREA_TIMERANGE_ID;
            paralen  = 12;
            areaparalen += (3 + paralen);
            YX_WriteBYTE_Strm(&wstrm, paratype);
            YX_WriteHWORD_Strm(&wstrm, paralen);
            
            YX_WriteDATA_Strm(&wstrm, YX_GetStrmPtr(&s_rstrm), 6);
            YX_MovStrmPtr(&s_rstrm, 6);
            YX_WriteDATA_Strm(&wstrm, YX_GetStrmPtr(&s_rstrm), 6);
            YX_MovStrmPtr(&s_rstrm, 6);
        }
        if (dwparaattr & EN_AREA_VECTOR) {
            paracnt++;        
            paratype = AREA_VECTORPARA_ID;
            paralen  = 2;
            areaparalen += (3 + paralen);
            YX_WriteBYTE_Strm(&wstrm, paratype);
            YX_WriteHWORD_Strm(&wstrm, paralen);            
            
            vt_kmh = YX_ReadHWORD_Strm_APP(&s_rstrm);
            YX_WriteBYTE_Strm(&wstrm, vt_kmh *1000 /1852);
            YX_WriteBYTE_Strm(&wstrm, YX_ReadBYTE_Strm(&s_rstrm));            
        }
        if (dwparaattr & EN_INAREA_AUDIO) {
            paracnt++;
            paratype = AREA_INAREA_AUDIO_ID;
            paralen  = YX_ReadBYTE_Strm(&s_rstrm);
            if (paralen > MAX_AREAAUDIO_LEN) {
                YX_DYM_Free(memptr);
                memptr = NULL;
                result = ACK_FAIL;
                goto RET_SETCIRCLE;                  
            }
            areaparalen += (3 + paralen);
            YX_WriteBYTE_Strm(&wstrm, paratype);
            YX_WriteHWORD_Strm(&wstrm, paralen); 
            while (paralen--) {
                YX_WriteBYTE_Strm(&wstrm, YX_ReadBYTE_Strm(&s_rstrm));    
            }
        }
        if (dwparaattr & EN_OUTAREA_AUDIO) {
            paracnt++;
            paratype = AREA_OUTAREA_AUDIO_ID;
            paralen  = YX_ReadBYTE_Strm(&s_rstrm);
            if (paralen > MAX_AREAAUDIO_LEN) {
                YX_DYM_Free(memptr);
                memptr = NULL;
                result = ACK_FAIL;
                goto RET_SETCIRCLE;                  
            }
            areaparalen += (3 + paralen);
            YX_WriteBYTE_Strm(&wstrm, paratype);
            YX_WriteHWORD_Strm(&wstrm, paralen); 
            while (paralen--) {
                YX_WriteBYTE_Strm(&wstrm, YX_ReadBYTE_Strm(&s_rstrm));    
            }
        }
        
        *ptr_paracnt = paracnt;
        ptr_areaparalen[0] = (areaparalen >> 24);
        ptr_areaparalen[1] = (areaparalen >> 16);
        ptr_areaparalen[2] = (areaparalen >> 8);
        ptr_areaparalen[3] = areaparalen;
    }
    if (YX_PolyRangePosParaSet((INT8U *)YX_GetStrmStartPtr(&wstrm), YX_GetStrmLen(&wstrm))) {
        result = ACK_SUCCESS;
    } else {
        result = ACK_FAIL;
    }
RET_SETCIRCLE:
    YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, result);
    if (NULL != memptr) {
        YX_DYM_Free(memptr);
    }
}

/*******************************************************************
** 函数名:     HdlMsg_DN_CMD_DELCIRCLE
** 函数描述:   MSG_0x8601 删除圆形区域
** 参数:       无
** 返回:       无
********************************************************************/
static void HdlMsg_DN_CMD_DELCIRCLE(void)
{
    INT8U   *memptr;
    INT8U   optype, areacnt, result;
    INT16U  memlen, i;
    INT32U  dwareaid;
    STREAM_T wstrm;

    memlen = s_hdlframelen + 100;                                                       /* 不能超过125个区域; memlen reserve 100 */
    if (NULL == (memptr = YX_DYM_Alloc(memlen))) {
        result = ACK_FAIL;
        goto RET_DELCIRCLE;         
    }

    YX_InitStrm(&wstrm, memptr, memlen);
    YX_WriteDATA_Strm(&wstrm, s_def_lable, sizeof(s_def_lable));
    YX_WriteHWORD_Strm(&wstrm, memlen);

    optype = DELETE_OP;
    YX_WriteBYTE_Strm(&wstrm, optype);
    YX_WriteBYTE_Strm(&wstrm, CIRCLE_AREA);

    areacnt   = YX_ReadBYTE_Strm(&s_rstrm);
    YX_WriteBYTE_Strm(&wstrm, areacnt);
    if (areacnt > MAX_AREA_NUM) {                                                       /* 最大125个区域 */
        YX_DYM_Free(memptr);
        memptr = NULL;
        result = ACK_FAIL;
        goto RET_DELCIRCLE;         
    }
    //【区域id(4B)】*M
    for (i = 0; i < areacnt; i++) {
        if (YX_GetStrmLeftLen(&wstrm) < 4 || YX_GetStrmLeftLen(&s_rstrm) < 4) {
            YX_DYM_Free(memptr);
            memptr = NULL;
            result = ACK_FAIL;
            goto RET_DELCIRCLE;              
        }        
        dwareaid = YX_ReadLONG_Strm_APP(&s_rstrm);
        YX_WriteLONG_Strm(&wstrm, dwareaid);
    }
    ////00 00 00 01 00 69 03 00 01 00 00 00 01
    if (YX_PolyRangePosParaSet((INT8U *)YX_GetStrmStartPtr(&wstrm), YX_GetStrmLen(&wstrm))) {
        result = ACK_SUCCESS;
    } else {
        result = ACK_FAIL;
    }
RET_DELCIRCLE:
    YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, result);    
    if (NULL != memptr) {
        YX_DYM_Free(memptr);
    }    
}

/*******************************************************************
** 函数名:     HdlMsg_DN_CMD_SETRECT
** 函数描述:   MSG_0x8602 设置矩形区域
** 参数:       无
** 返回:       无
********************************************************************/
static void HdlMsg_DN_CMD_SETRECT(void)
{
    INT8U   *memptr, *ptr_areaparalen, *ptr_paracnt;
    INT8U   optype, areacnt, paracnt, paratype, longitude[4], latitude[4], result;
    INT16U  memlen, i, paralen, vt_kmh;
    INT32U  dwareaid, areaparalen, dwparaattr, dwlongitude_LU, dwlatitude_LU, dwlongitude_RD, dwlatitude_RD;
    STREAM_T wstrm;

    memlen = s_hdlframelen + MAX_AREA_NUM * DIFLEN_POTOCOL_RANGE + 100;                 /* 不能超过125个区域; memlen reserve 100 */
    if (NULL == (memptr = YX_DYM_Alloc(memlen))) {
        result = ACK_FAIL;
        goto RET_SETRECT;         
    }

    YX_InitStrm(&wstrm, memptr, memlen);
    YX_WriteDATA_Strm(&wstrm, s_def_lable, sizeof(s_def_lable));
    YX_WriteHWORD_Strm(&wstrm, memlen);

    optype = YX_ReadBYTE_Strm(&s_rstrm);
    if (optype == 1) {
        optype = APPEND_OP;
    } else if  (optype == 2) {
        optype = MODIFY_OP;
    }    
    YX_WriteBYTE_Strm(&wstrm, optype);
    YX_WriteBYTE_Strm(&wstrm, RECT_AREA);

    areacnt   = YX_ReadBYTE_Strm(&s_rstrm);
    YX_WriteBYTE_Strm(&wstrm, areacnt);
    if (areacnt > MAX_AREA_NUM) {                                                       /* 最大125个区域 */
        YX_DYM_Free(memptr);
        memptr = NULL;
        result = ACK_FAIL;
        goto RET_SETRECT;         
    }
    //【区域id(4B)  +区域参数长度(4B) + 区域参数设置属性(4B) + 区域参数个数n1(1B) +【参数类型(1B) + 参数长度n2(2B) + 参数内容(n2)】*n1】*M
    for (i = 0; i < areacnt; i++) {
        if (YX_GetStrmLeftLen(&wstrm) < 28 || YX_GetStrmLeftLen(&s_rstrm) < 18) {
            YX_DYM_Free(memptr);
            memptr = NULL;
            result = ACK_FAIL;
            goto RET_SETRECT;              
        }
    
        dwareaid = YX_ReadLONG_Strm_APP(&s_rstrm);
        YX_WriteLONG_Strm(&wstrm, dwareaid);

        areaparalen = 5;                                                                /* 5 for 区域参数设置属性(4B) + 区域参数个数n1(1B) */
        ptr_areaparalen = YX_GetStrmPtr(&wstrm);
        YX_WriteLONG_Strm(&wstrm, areaparalen);
        
        dwparaattr = YX_ReadHWORD_Strm_APP(&s_rstrm);
        YX_WriteLONG_Strm(&wstrm, dwparaattr);

        ptr_paracnt = YX_GetStrmPtr(&wstrm);
        paracnt = 1;
        YX_WriteBYTE_Strm(&wstrm, paracnt);

        paratype = AREA_BASEPARA_ID;
        paralen  = 16;
        areaparalen += (3 + paralen);                                                   /* 3 for 参数类型(1B) + 参数长度n2(2B) */
        YX_WriteBYTE_Strm(&wstrm, paratype);
        YX_WriteHWORD_Strm(&wstrm, paralen);

        dwlatitude_LU  = YX_ReadLONG_Strm_APP(&s_rstrm);                                /* 左上角 */
        dwlongitude_LU = YX_ReadLONG_Strm_APP(&s_rstrm);
        YX_ConvertDegreeToDMMM(longitude, dwlongitude_LU);
        YX_ConvertDegreeToDMMM(latitude, dwlatitude_LU);
        YX_WriteDATA_Strm(&wstrm, longitude, sizeof(longitude));
        YX_WriteDATA_Strm(&wstrm, latitude, sizeof(latitude));        
        
        dwlatitude_RD  = YX_ReadLONG_Strm_APP(&s_rstrm);                                /* 右下角 */
        dwlongitude_RD = YX_ReadLONG_Strm_APP(&s_rstrm);
        YX_ConvertDegreeToDMMM(longitude, dwlongitude_RD);
        YX_ConvertDegreeToDMMM(latitude, dwlatitude_RD);        
        YX_WriteDATA_Strm(&wstrm, longitude, sizeof(longitude));
        YX_WriteDATA_Strm(&wstrm, latitude, sizeof(latitude));
        
        if (dwparaattr & EN_AREA_TIMERANGE) {
            paracnt++;
            paratype = AREA_TIMERANGE_ID;
            paralen  = 12;
            areaparalen += (3 + paralen);
            YX_WriteBYTE_Strm(&wstrm, paratype);
            YX_WriteHWORD_Strm(&wstrm, paralen);            
            
            YX_WriteDATA_Strm(&wstrm, YX_GetStrmPtr(&s_rstrm), 6);
            YX_MovStrmPtr(&s_rstrm, 6);
            YX_WriteDATA_Strm(&wstrm, YX_GetStrmPtr(&s_rstrm), 6);
            YX_MovStrmPtr(&s_rstrm, 6);
        }
        if (dwparaattr & EN_AREA_VECTOR) {
            paracnt++;        
            paratype = AREA_VECTORPARA_ID;
            paralen  = 2;
            areaparalen += (3 + paralen);
            YX_WriteBYTE_Strm(&wstrm, paratype);
            YX_WriteHWORD_Strm(&wstrm, paralen);            
            
            vt_kmh = YX_ReadHWORD_Strm_APP(&s_rstrm);
            YX_WriteBYTE_Strm(&wstrm, vt_kmh *1000 /1852);
            YX_WriteBYTE_Strm(&wstrm, YX_ReadBYTE_Strm(&s_rstrm));            
        }
        
        if (dwparaattr & EN_INAREA_AUDIO) {
            paracnt++;
            paratype = AREA_INAREA_AUDIO_ID;
            paralen  = YX_ReadBYTE_Strm(&s_rstrm);
            if (paralen > MAX_AREAAUDIO_LEN) {
                YX_DYM_Free(memptr);
                memptr = NULL;
                result = ACK_FAIL;
                goto RET_SETRECT;                  
            }
            areaparalen += (3 + paralen);
            YX_WriteBYTE_Strm(&wstrm, paratype);
            YX_WriteHWORD_Strm(&wstrm, paralen); 
            while (paralen--) {
                YX_WriteBYTE_Strm(&wstrm, YX_ReadBYTE_Strm(&s_rstrm));    
            }
        }
        if (dwparaattr & EN_OUTAREA_AUDIO) {
            paracnt++;
            paratype = AREA_OUTAREA_AUDIO_ID;
            paralen  = YX_ReadBYTE_Strm(&s_rstrm);
            if (paralen > MAX_AREAAUDIO_LEN) {
                YX_DYM_Free(memptr);
                memptr = NULL;
                result = ACK_FAIL;
                goto RET_SETRECT;                  
            }
            areaparalen += (3 + paralen);
            YX_WriteBYTE_Strm(&wstrm, paratype);
            YX_WriteHWORD_Strm(&wstrm, paralen); 
            while (paralen--) {
                YX_WriteBYTE_Strm(&wstrm, YX_ReadBYTE_Strm(&s_rstrm));    
            }
        }
        
        *ptr_paracnt = paracnt;
        ptr_areaparalen[0] = (areaparalen >> 24);
        ptr_areaparalen[1] = (areaparalen >> 16);
        ptr_areaparalen[2] = (areaparalen >> 8);
        ptr_areaparalen[3] = areaparalen;
    }
    if (YX_PolyRangePosParaSet((INT8U *)YX_GetStrmStartPtr(&wstrm), YX_GetStrmLen(&wstrm))) {
        result = ACK_SUCCESS;
    } else {
        result = ACK_FAIL;
    }
RET_SETRECT:
    YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, result);
    if (NULL != memptr) {
        YX_DYM_Free(memptr);
    }    
}

/*******************************************************************
** 函数名:     HdlMsg_DN_CMD_DELRECT
** 函数描述:   MSG_0x8603 删除矩形区域
** 参数:       无
** 返回:       无
********************************************************************/
static void HdlMsg_DN_CMD_DELRECT(void)
{
    INT8U   *memptr;
    INT8U   optype, areacnt, result;
    INT16U  memlen, i;
    INT32U  dwareaid;
    STREAM_T wstrm;

    memlen = s_hdlframelen + 100;                                                       /* 不能超过125个区域; memlen reserve 100 */
    if (NULL == (memptr = YX_DYM_Alloc(memlen))) {
        result = ACK_FAIL;
        goto RET_DELRECT;        
    }

    YX_InitStrm(&wstrm, memptr, memlen);
    YX_WriteDATA_Strm(&wstrm, s_def_lable, sizeof(s_def_lable));
    YX_WriteHWORD_Strm(&wstrm, memlen);

    optype = DELETE_OP;
    YX_WriteBYTE_Strm(&wstrm, optype);
    YX_WriteBYTE_Strm(&wstrm, RECT_AREA);

    areacnt   = YX_ReadBYTE_Strm(&s_rstrm);
    YX_WriteBYTE_Strm(&wstrm, areacnt);
    if (areacnt > MAX_AREA_NUM) {                                                       /* 最大125个区域 */
        YX_DYM_Free(memptr);
        memptr = NULL;
        result = ACK_FAIL;
        goto RET_DELRECT;        
    }
    //【区域id(4B)】*M
    for (i = 0; i < areacnt; i++) {
        if (YX_GetStrmLeftLen(&wstrm) < 4 || YX_GetStrmLeftLen(&s_rstrm) < 4) {
            YX_DYM_Free(memptr);
            memptr = NULL;
            result = ACK_FAIL;
            goto RET_DELRECT;                
        }        
        dwareaid = YX_ReadLONG_Strm_APP(&s_rstrm);
        YX_WriteLONG_Strm(&wstrm, dwareaid);
    }
    if (YX_PolyRangePosParaSet((INT8U *)YX_GetStrmStartPtr(&wstrm), YX_GetStrmLen(&wstrm))) {
        result = ACK_SUCCESS;
    } else {
        result = ACK_FAIL;
    }
RET_DELRECT:
    YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, result);    
    if (NULL != memptr) {
        YX_DYM_Free(memptr);
    }    
}

/*******************************************************************
*   名称: ConvertPolyDotData
*   作者: 赖荣东  2011年5月27日
*
*   描述: 转换多边形区域点数据到预定格式
*   输入: [in]  sptr:                   源点数据 (点个数N + 点数据 * N ) 
          [in]  len:                    源点数据长度
          [out]  sp:                    目的缓冲区
*   返回: 转换后的点参数长度
*******************************************************************/
static INT16U ConvertPolyDotData(STREAM_T *sp, INT8U *sptr, INT16U len)
{
    STREAM_T rstrm;
    INT16U 	i, paralen, totaldotnum;
    INT32U  dwlongitude, dwlatitude;
    INT8U   paratype, longitude[4], latitude[4];
    
    YX_InitStrm(&rstrm, sptr, len);
    
    totaldotnum = YX_ReadHWORD_Strm(&rstrm);

    paratype = AREA_BASEPARA_ID;
    paralen  = totaldotnum * POLYAREA_DOTSIZE;                                          /* 8 for longitue and latitude */
    #if DEBUG_GSMSTATUS > 0
    printf_com("<paralen:%d, datalen:%d>\r\n", paralen, YX_GetStrmLeftLen(&rstrm));
    #endif
    if (paralen > YX_GetStrmLeftLen(&rstrm)) {
        return 0;
    }

    paralen += 2;                                                                       /* 2 for dotnum */
    YX_WriteBYTE_Strm(sp, paratype);
    YX_WriteHWORD_Strm(sp, paralen);
    YX_WriteHWORD_Strm(sp, totaldotnum);

    for (i = 0; i < totaldotnum; i++) {
        dwlatitude  = YX_ReadLONG_Strm_APP(&rstrm);
        dwlongitude = YX_ReadLONG_Strm_APP(&rstrm);
        YX_ConvertDegreeToDMMM(longitude, dwlongitude);
        YX_ConvertDegreeToDMMM(latitude, dwlatitude);
        YX_WriteDATA_Strm(sp, longitude, sizeof(longitude));
        YX_WriteDATA_Strm(sp, latitude, sizeof(latitude));        
    }
    return paralen;
}

/*******************************************************************
** 函数名:     HdlMsg_DN_CMD_SETPOLYRANGE
** 函数描述:   MSG_0x8604 设置多边形区域
** 参数:       无
** 返回:       无
********************************************************************/
static void HdlMsg_DN_CMD_SETPOLYRANGE(void)
{
    INT8U   *memptr, *ptr_areaparalen, *ptr_paracnt;
    INT8U   optype, pkgareacnt, paracnt, paratype, result;
    INT16U  memlen, i, paralen, vt_kmh;
    INT32U  dwareaid, areaparalen, dwparaattr;
    STREAM_T wstrm;

    result = ACK_SUCCESS;
    memlen = s_hdlframelen + MAX_AREA_NUM * DIFLEN_POTOCOL_RANGE + 100;                 /* 不能超过125个区域; memlen reserve 100 */
    if (NULL == (memptr = YX_DYM_Alloc(memlen))) {
        result = ACK_FAIL;
        goto RET_SETPOLY;          
    }

    YX_InitStrm(&wstrm, memptr, memlen);
    YX_WriteDATA_Strm(&wstrm, s_def_lable, sizeof(s_def_lable));
    YX_WriteHWORD_Strm(&wstrm, memlen);

    optype = MODIFY_OP;
    YX_WriteBYTE_Strm(&wstrm, optype);
    YX_WriteBYTE_Strm(&wstrm, POLY_AREA);

    pkgareacnt   = 1;
    YX_WriteBYTE_Strm(&wstrm, pkgareacnt);

    //【区域id(4B)  +区域参数长度(4B) + 区域参数设置属性(4B) + 区域参数个数n1(1B) +【参数类型(1B) + 参数长度n2(2B) + 参数内容(n2)】*n1】*M
    for (i = 0; i < pkgareacnt; i++) {
        if (YX_GetStrmLeftLen(&wstrm) < 28) {
            YX_DYM_Free(memptr);
            memptr = NULL;
            result = ACK_FAIL;
            goto RET_SETPOLY;              
        }
    
        dwareaid = YX_ReadLONG_Strm_APP(&s_rstrm);
        YX_WriteLONG_Strm(&wstrm, dwareaid);

        areaparalen = 5;                                                                /* 5 for 区域参数设置属性(4B) + 区域参数个数n1(1B) */
        ptr_areaparalen = YX_GetStrmPtr(&wstrm);
        YX_WriteLONG_Strm(&wstrm, areaparalen);
        
        dwparaattr = YX_ReadHWORD_Strm_APP(&s_rstrm);
        YX_WriteLONG_Strm(&wstrm, dwparaattr);

        ptr_paracnt = YX_GetStrmPtr(&wstrm);
        paracnt = 0;
        YX_WriteBYTE_Strm(&wstrm, paracnt);
       
        if (dwparaattr & EN_AREA_TIMERANGE) {
            paracnt++;
            paratype = AREA_TIMERANGE_ID;
            paralen  = 12;
            areaparalen += (3 + paralen);
            YX_WriteBYTE_Strm(&wstrm, paratype);
            YX_WriteHWORD_Strm(&wstrm, paralen);            
            
            YX_WriteDATA_Strm(&wstrm, YX_GetStrmPtr(&s_rstrm), 6);
            YX_MovStrmPtr(&s_rstrm, 6);
            YX_WriteDATA_Strm(&wstrm, YX_GetStrmPtr(&s_rstrm), 6);
            YX_MovStrmPtr(&s_rstrm, 6);
        }
        if (dwparaattr & EN_AREA_VECTOR) {
            paracnt++;        
            paratype = AREA_VECTORPARA_ID;
            paralen  = 2;
            areaparalen += (3 + paralen);
            YX_WriteBYTE_Strm(&wstrm, paratype);
            YX_WriteHWORD_Strm(&wstrm, paralen);            
            
            vt_kmh = YX_ReadHWORD_Strm_APP(&s_rstrm);
            YX_WriteBYTE_Strm(&wstrm, vt_kmh *1000 /1852);
            YX_WriteBYTE_Strm(&wstrm, YX_ReadBYTE_Strm(&s_rstrm));            
        }       
        if (dwparaattr & EN_INAREA_AUDIO) {
            paracnt++;
            paratype = AREA_INAREA_AUDIO_ID;
            paralen  = YX_ReadBYTE_Strm(&s_rstrm);
            if (paralen > MAX_AREAAUDIO_LEN) {
                YX_DYM_Free(memptr);
                memptr = NULL;
                result = ACK_FAIL;
                goto RET_SETPOLY;                  
            }
            areaparalen += (3 + paralen);
            YX_WriteBYTE_Strm(&wstrm, paratype);
            YX_WriteHWORD_Strm(&wstrm, paralen); 
            while (paralen--) {
                YX_WriteBYTE_Strm(&wstrm, YX_ReadBYTE_Strm(&s_rstrm));    
            }
        }
        if (dwparaattr & EN_OUTAREA_AUDIO) {
            paracnt++;
            paratype = AREA_OUTAREA_AUDIO_ID;
            paralen  = YX_ReadBYTE_Strm(&s_rstrm);
            if (paralen > MAX_AREAAUDIO_LEN) {
                YX_DYM_Free(memptr);
                memptr = NULL;
                result = ACK_FAIL;
                goto RET_SETPOLY;                  
            }
            areaparalen += (3 + paralen);
            YX_WriteBYTE_Strm(&wstrm, paratype);
            YX_WriteHWORD_Strm(&wstrm, paralen); 
            while (paralen--) {
                YX_WriteBYTE_Strm(&wstrm, YX_ReadBYTE_Strm(&s_rstrm));    
            }
        }
        
        if (0 < (paralen = ConvertPolyDotData(&wstrm, YX_GetStrmPtr(&s_rstrm), YX_GetStrmLeftLen(&s_rstrm)))) {            
            paracnt++;
            *ptr_paracnt = paracnt;
            areaparalen += (3 + paralen);                
            ptr_areaparalen[0] = (areaparalen >> 24);
            ptr_areaparalen[1] = (areaparalen >> 16);
            ptr_areaparalen[2] = (areaparalen >> 8);
            ptr_areaparalen[3] = areaparalen;

            if (YX_PolyRangePosParaSet((INT8U *)YX_GetStrmStartPtr(&wstrm), YX_GetStrmLen(&wstrm))) {
                result = ACK_SUCCESS;
            } else {
                result = ACK_ERR;
            }                
        } else {
            result = ACK_ERR;
        }          
    }
RET_SETPOLY:
    YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, result);
    if (NULL != memptr) {
        YX_DYM_Free(memptr);
    }    
}

/*******************************************************************
** 函数名:     HdlMsg_DN_CMD_DELPOLYRANGE
** 函数描述:   MSG_0x8605 删除多边形区域
** 参数:       无
** 返回:       无
********************************************************************/
static void HdlMsg_DN_CMD_DELPOLYRANGE(void)
{
    INT8U   *memptr;
    INT8U   optype, areacnt, result;
    INT16U  memlen, i;
    INT32U  dwareaid;
    STREAM_T wstrm;

    memlen = s_hdlframelen + 100;                                                       /* 不能超过125个区域; memlen reserve 100 */
    if (NULL == (memptr = YX_DYM_Alloc(memlen))) {
        result = ACK_FAIL;
        goto RET_DELPOLY;         
    }

    YX_InitStrm(&wstrm, memptr, memlen);
    YX_WriteDATA_Strm(&wstrm, s_def_lable, sizeof(s_def_lable));
    YX_WriteHWORD_Strm(&wstrm, memlen);

    optype = DELETE_OP;
    YX_WriteBYTE_Strm(&wstrm, optype);
    YX_WriteBYTE_Strm(&wstrm, POLY_AREA);

    areacnt   = YX_ReadBYTE_Strm(&s_rstrm);
    YX_WriteBYTE_Strm(&wstrm, areacnt);
    if (areacnt > MAX_AREA_NUM) {                                                       /* 最大125个区域 */
        YX_DYM_Free(memptr);
        memptr = NULL;
        result = ACK_FAIL;
        goto RET_DELPOLY;         
    }
    //【区域id(4B)】*M
    for (i = 0; i < areacnt; i++) {
        if (YX_GetStrmLeftLen(&wstrm) < 4 || YX_GetStrmLeftLen(&s_rstrm) < 4) {
            YX_DYM_Free(memptr);
            memptr = NULL;
            result = ACK_FAIL;
            goto RET_DELPOLY;             
        }        
        dwareaid = YX_ReadLONG_Strm_APP(&s_rstrm);
        YX_WriteLONG_Strm(&wstrm, dwareaid);
    }
    if (YX_PolyRangePosParaSet((INT8U *)YX_GetStrmStartPtr(&wstrm), YX_GetStrmLen(&wstrm))) {
        result = ACK_SUCCESS;
    } else {
        result = ACK_FAIL;
    }
RET_DELPOLY:
    YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, result);    
    if (NULL != memptr) {
        YX_DYM_Free(memptr);
    }    
}
#endif

#if EN_ROADLINE > 0
/*******************************************************************
** 函数名:     HdlMsg_DN_CMD_SETROADLINE
** 函数描述:   MSG_0x8606 设置线路
** 参数:       无
** 返回:       无
********************************************************************/
static void HdlMsg_DN_CMD_SETROADLINE(void)
{
    INT8U result;

    if (YX_SetRoadline(YX_GetStrmPtr(&s_rstrm), s_hdlframelen)) {
        result = ACK_SUCCESS;
    } else {
        result = ACK_FAIL;
    }
    YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, result);
}

/*******************************************************************
** 函数名:     HdlMsg_DN_CMD_DELROADLINE
** 函数描述:   MSG_0x8607 删除线路
** 参数:       无
** 返回:       无
********************************************************************/
static void HdlMsg_DN_CMD_DELROADLINE(void)
{
    INT8U   roadnum, i, result;
    INT32U  r_dwroadid;
    LIST_T  *readylist, *freelist;
    LISTMEM *cell;

    roadnum = YX_ReadBYTE_Strm(&s_rstrm);
    result = ACK_SUCCESS;
    

//86 07 00 01 01 38 00 00 00 02 00 18 00 A3
    /* if (roadnum > MAX_AREA_NUM) {
        result = ACK_FAIL;
        YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, result); 
        return;
    }   */ 
    if (roadnum == 0) {
        if (YX_ClearRLFolder() == 0) {
            result = ACK_FAIL;
        }
    }    
    for (i = 0; i < roadnum; i++) {
        if (YX_GetStrmLeftLen(&s_rstrm) < 4) {
            result = ACK_FAIL;
            break;
        }        
        r_dwroadid = YX_ReadLONG_Strm_APP(&s_rstrm); 
        if (NULL != (cell = (LISTMEM *)YX_GetCell_RLList(r_dwroadid, &readylist, &freelist))) {
            YX_DelRLFile(r_dwroadid);
            YX_DelListEle(readylist, (LISTMEM *)cell);
            YX_AppendListEle(freelist, (LISTMEM *)cell);
        }
    }
    
    YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, result);   

}
#endif

/*******************************************************************
** 函数名:      HdlMsg_DN_CMD_RESEND
** 函数描述:    8003补传分包请求
** 参数:        [in]  NULL                  
** 返回:        NULL
********************************************************************/
static void HdlMsg_DN_CMD_RESEND(void)
{
    YX_PROTOCOL_SendCommonAck(s_hdlframe, s_channeltype, ACK_SUCCESS);
    YX_WIN_InformRecvAck(DN_CMD_RESEND, s_hdlframe, s_hdlframelen);
    YX_JTT1_LinkInformQuery(_SUCCESS);    
}

#if EN_FREECONST > 0  /* EN_FREECONST > 0 */
static FUNCENTRY_T s_functionentry[] = {
                                         DN_ACK_REG,                    HdlMsg_DN_ACK_REG,  
                                         DN_ACK_COMMON,                 HdlMsg_DN_ACK_COMMON,
#if EN_TPPROTOCOL > 0   
                                         DN_CMD_TRANSDATA,              HdlMsg_DN_CMD_TRANSDATA,
#endif                                         
                                         #if EN_120R > 0
                                         DN_CMD_TEXTINFO,               HdlMsg_DN_CMD_TEXTINFO,      /* 文本信息下发 */
                                         DN_CMD_EVENTSET,               HdlMsg_DN_CMD_EVENTSET,      /* 事件设置 */
                                         DN_CMD_ASK,                    HdlMsg_DN_CMD_ASK,           /* 提问下发 */
                                         DN_CMD_INFOMENU,               HdlMsg_DN_CMD_INFOMENU,      /* 信息点播设置 */
                                         DN_CMD_INFOSEVER,              HdlMsg_DN_CMD_INFOSEVER,     /* 信息服务 */
                                         DN_CMD_CALLBACKTEL,            HdlMsg_DN_CMD_CALLBACKTEL,   /* 电话回拨接口 */
                                         DN_CMD_SETTELNOTE,             HdlMsg_DN_CMD_SETTELNOTE,    /* 设置电话本 */
                                         #endif
                                         //DN_NEAR_MENULIST,          HdlMsg_0x8900, /* 周边信息分类菜单设置 */
                                         //DN_NEAR_INFOQRYACK,        HdlMsg_0x8901, /* 周边信息查询应答 */
                                         #if EN_AREARANGE_ALRAM > 0
                                         DN_CMD_SETCIRCLE,              HdlMsg_DN_CMD_SETCIRCLE,     /* 设置圆形区域 */
                                         DN_CMD_DELCIRCLE,              HdlMsg_DN_CMD_DELCIRCLE,     /* 删除圆形区域 */
                                         DN_CMD_SETRECT,                HdlMsg_DN_CMD_SETRECT,       /* 设置矩形区域 */
                                         DN_CMD_DELRECT,                HdlMsg_DN_CMD_DELRECT,       /* 删除矩形区域 */
                                         DN_CMD_SETPOLYRANGE,           HdlMsg_DN_CMD_SETPOLYRANGE,  /* 设置多边形 */
                                         DN_CMD_DELPOLYRANGE,           HdlMsg_DN_CMD_DELPOLYRANGE,  /* 删除多边形 */
                                         DN_CMD_SETROADLINE,            HdlMsg_DN_CMD_SETROADLINE,   /* 设置线路 */
                                         DN_CMD_DELROADLINE,            HdlMsg_DN_CMD_DELROADLINE,   /* 删除线路 */
                                         #endif
                                         DN_CMD_RESEND,                 HdlMsg_DN_CMD_RESEND        /* 补传分包请求 */
                                         
                                     };
static INT8U s_funnum = sizeof(s_functionentry)/sizeof(s_functionentry[0]);

#else /* EN_FREECONST == 0 */

static INT8U s_funnum;
static FUNCENTRY_T s_functionentry[19];
/*
****************************************************************
*   手动初始化
****************************************************************
*/
DECLARE_GPS_FUN_INIT_CONSTVAR(GPS_GENERAL_MAN_STATICDAT_ID)
{
    s_funnum = 0;
    
    s_functionentry[s_funnum].index = DN_ACK_REG;           s_functionentry[s_funnum].entryproc = HdlMsg_DN_ACK_REG;           s_funnum++;
    s_functionentry[s_funnum].index = DN_ACK_COMMON;        s_functionentry[s_funnum].entryproc = HdlMsg_DN_ACK_COMMON;        s_funnum++;
    s_functionentry[s_funnum].index = DN_CMD_TRANSDATA;     s_functionentry[s_funnum].entryproc = HdlMsg_DN_CMD_TRANSDATA;     s_funnum++;
    #if EN_120R > 0
    s_functionentry[s_funnum].index = DN_CMD_TEXTINFO;      s_functionentry[s_funnum].entryproc = HdlMsg_DN_CMD_TEXTINFO;      s_funnum++;
    s_functionentry[s_funnum].index = DN_CMD_EVENTSET;      s_functionentry[s_funnum].entryproc = HdlMsg_DN_CMD_EVENTSET;      s_funnum++;
    s_functionentry[s_funnum].index = DN_CMD_ASK;           s_functionentry[s_funnum].entryproc = HdlMsg_DN_CMD_ASK;           s_funnum++;
    s_functionentry[s_funnum].index = DN_CMD_INFOMENU;      s_functionentry[s_funnum].entryproc = HdlMsg_DN_CMD_INFOMENU;      s_funnum++;
    s_functionentry[s_funnum].index = DN_CMD_INFOSEVER;     s_functionentry[s_funnum].entryproc = HdlMsg_DN_CMD_INFOSEVER;     s_funnum++;
    s_functionentry[s_funnum].index = DN_CMD_CALLBACKTEL;   s_functionentry[s_funnum].entryproc = HdlMsg_DN_CMD_CALLBACKTEL;   s_funnum++;
    s_functionentry[s_funnum].index = DN_CMD_SETTELNOTE;    s_functionentry[s_funnum].entryproc = HdlMsg_DN_CMD_SETTELNOTE;    s_funnum++;
    #endif
    #if EN_AREARANGE_ALRAM > 0
    s_functionentry[s_funnum].index = DN_CMD_SETCIRCLE;     s_functionentry[s_funnum].entryproc = HdlMsg_DN_CMD_SETCIRCLE;     s_funnum++;
    s_functionentry[s_funnum].index = DN_CMD_DELCIRCLE;     s_functionentry[s_funnum].entryproc = HdlMsg_DN_CMD_DELCIRCLE;     s_funnum++;
    s_functionentry[s_funnum].index = DN_CMD_SETRECT;       s_functionentry[s_funnum].entryproc = HdlMsg_DN_CMD_SETRECT;       s_funnum++;
    s_functionentry[s_funnum].index = DN_CMD_DELRECT;       s_functionentry[s_funnum].entryproc = HdlMsg_DN_CMD_DELRECT;       s_funnum++;
    s_functionentry[s_funnum].index = DN_CMD_SETPOLYRANGE;  s_functionentry[s_funnum].entryproc = HdlMsg_DN_CMD_SETPOLYRANGE;  s_funnum++;
    s_functionentry[s_funnum].index = DN_CMD_DELPOLYRANGE;  s_functionentry[s_funnum].entryproc = HdlMsg_DN_CMD_DELPOLYRANGE;  s_funnum++;
    s_functionentry[s_funnum].index = DN_CMD_SETROADLINE;   s_functionentry[s_funnum].entryproc = HdlMsg_DN_CMD_SETROADLINE;   s_funnum++;
    s_functionentry[s_funnum].index = DN_CMD_DELROADLINE;   s_functionentry[s_funnum].entryproc = HdlMsg_DN_CMD_DELROADLINE;   s_funnum++;
    #endif
    s_functionentry[s_funnum].index = DN_CMD_RESEND;        s_functionentry[s_funnum].entryproc = HdlMsg_DN_CMD_RESEND;        s_funnum++;

    OS_ASSERT((s_funnum <= sizeof(s_functionentry) / sizeof(s_functionentry[0])), RETURN_VOID);
}

#endif /* end EN_FREECONST */




/*******************************************************************
** 函数名:     HdlGeneralSysFrameMsg
** 函数描述:   业务数据处理入口
** 参数:       [in]  curframe:              系统帧
               [in]  framelen:              系统帧长度
               [in]  chaneltype:            接收通道类型     
** 返回:       无
********************************************************************/
static void HdlGeneralSysFrameMsg(PROTOCOL_HEAD_T *curframe, INT32U framelen, INT8U chaneltype)
{
    s_hdlframe    = curframe;
    s_hdlframelen = framelen - SYSHEAD_LEN - SYSTAIL_LEN;
    s_rx_channeltype = chaneltype;

    YX_RxchannelToTxchannelAttr(chaneltype, &s_channeltype);
#if DEBUG_GSMSTATUS > 0
    ////printf_com("<HdlGeneralSysFrameMsg YX_GetType_SYSFrame %x>\r\n", YX_GetType_SYSFrame(curframe));
    ////printf_hex(curframe->data, s_hdlframelen);
#endif
    YX_InitStrm(&s_rstrm, curframe->data, s_hdlframelen);
    YX_FindProcEntry(YX_GetType_SYSFrame(curframe), s_functionentry, s_funnum);
}

/*******************************************************************
** 函数名:     YX_InitGeneralMan
** 函数描述:   普通协议处理初始化函数
** 参数:       无
** 返回:       无
********************************************************************/
void YX_InitGeneralMan(void)
{
    INT8U i;

    YX_InitAutoDial();
    YX_InitCallRHT();
    YX_InitOilControl();
    YX_InitDoorControl();
    for (i = 0; i < s_funnum; i++) {
        YX_PROTOCOL_Register(TYPE_PROTOCOL, s_functionentry[i].index, HdlGeneralSysFrameMsg);
    }
    s_delaycalltmr = 0;
    YX_MEMSET(&s_logout_inform, 0, sizeof(s_logout_inform));
}
