/********************************************************************************
**
** 文件名:     at_recv.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现手机模块数据接收解析处理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2014/06/15 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "yx_include.h"
#include "yx_misc.h"
#include "yx_dym_drv.h"
#include "at_com.h"
#include "at_recv.h"
#include "at_send.h"
 
#include "at_pdu.h"
#include "at_socket_drv.h"

#if EN_AT > 0
/*
********************************************************************************
* define config parameters
********************************************************************************
*/

#define PERIOD_SCAN          _MILTICK, 1
#define SIZE_AT              64
#define _OPEN                0x01

/*
********************************************************************************
* define command control block event
********************************************************************************
*/
#define EVT_RESET                           0
#define EVT_OVERTIME                        1
#define EVT_NORMAL                          2


/*
********************************************************************************
* define at commands handler return
********************************************************************************
*/
#define RET_CONTINUE                        0
#define RET_END                             1
#define RET_ABNORMAL                        2

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    char const  *keyword;
    INT8U        overtime;
    INT8U        fromhead;
    INT8U        (*handler)(INT8U event, INT8U *sptr, INT16U slen);
} URC_HDL_TBL_T;

typedef struct {
    INT8U       pdulen;
    INT8U       ct_clip;
    INT8U       n_recv;
    INT8U       (*handler)(INT8U event, INT8U *sptr, INT16U slen);
} RCB_T;

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static RCB_T s_rcb;
static INT8U s_overtmr;




/*******************************************************************
** 函数名:     PORT_Hdl_SHELL_MSG_SMS_RECV_IND
** 函数描述:   接收到短信息
** 参数:       [in] sm：   短信结构体
** 返回:       无
********************************************************************/
static void PORT_Hdl_SHELL_MSG_SMS_RECV_IND(SM_T *sm)
{
#if 0
    SM_T *ptr;
    
    ptr = (SM_T *)YX_DYM_Alloc(sizeof(SM_T));
    if (ptr == 0) {
        return;
    }
    YX_MEMCPY(ptr, sizeof(SM_T), sm, sizeof(SM_T));
    
    if (SHELL_SendEvent(SHELL_MAIN_TSK, SHELL_MSG_SMS_RECV_IND, (void *)ptr) == 0) {
        ;
    } else {
        YX_DYM_Free(ptr);
    }
#endif
}

/*******************************************************************
** 函数名:     PORT_Hdl_SHELL_MSG_SMS_UNREAD_IND
** 函数描述:   处理SHELL_MSG_SMS_UNREAD_IND未读短信消息事件
** 参数:       [in] msg: 消息事件携带的参数指针
** 返回:       true:  消息事件被接受处理
**             false: 消息事件被拒绝处理
********************************************************************/
static void PORT_Hdl_SHELL_MSG_SMS_UNREAD_IND(void *msg)
{
#if 0
    if (SHELL_SendEvent(SHELL_MAIN_TSK, SHELL_MSG_SMS_UNREAD_IND, (void *)msg) == 0) {
        ;
    } else {
        ;
    }
#endif
}

#if EN_AT_PHONE > 0
/*******************************************************************
** 函数名:     PORT_Hdl_SHELL_MSG_RING_IND
** 函数描述:   处理SHELL_MSG_RING_IND消息事件
**             来电的消息处理
** 参数:       [in] msg: 消息事件携带的参数指针
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN PORT_Hdl_SHELL_MSG_RING_IND(void *msg)
{
    INT8U *memptr;
    INT8U tellen;
    
    #if DEBUG_PHONE > 0
    printf_com("<Handler_CLIP(%s)>\r\n", (char *)msg);
    #endif
    
    tellen = YX_STRLEN((char *)msg) + 1;
    memptr = (INT8U *)YX_DYM_Alloc(tellen);
    if (memptr == 0) {
        return;
    }
    
    YX_MEMCPY(memptr, msg, tellen);
    if (SHELL_SendEvent(SHELL_MAIN_TSK, SHELL_MSG_RING_IND, (void *)memptr) == 0) {
        ;
    } else {
        YX_DYM_Free(memptr);
    }
    
    return true;
}

#endif

/*
********************************************************************************
* HANDLER: NO CARRIER
********************************************************************************
*/
static INT8U Handler_NOCARRIER(INT8U event, INT8U *sptr, INT16U slen)
{
    s_rcb.ct_clip = 0;
    
	if (event == EVT_NORMAL) {
#if EN_AT_PHONE > 0
        SHELL_SendEvent(SHELL_MAIN_TSK, SHELL_MSG_CALL_DISCONNECTED_IND, (void *)0);
	    //DetectNoCarrier();
#endif
	}
	return RET_END;
}

/*
********************************************************************************
* HANDLER: RING
********************************************************************************
*/
static INT8U Handler_RING(INT8U event, INT8U *sptr, INT16U slen)
{
    if (event == EVT_NORMAL) {
        //DetectRing();
    }
	return RET_END;
}

/*
********************************************************************************
* HANDLER: +CLIP
********************************************************************************
*/
static INT8U Handler_CLIP(INT8U event, INT8U *sptr, INT16U slen)
{
    INT8U tellen;
    INT8U tel[31];
    
    if (event == EVT_NORMAL) {
        tellen = YX_SearchString(tel, sizeof(tel) - 1, sptr, slen, '"', 1);
        tel[tellen] = '\0';
        
        if (tellen > 0) {
            s_rcb.ct_clip = 0;
#if EN_AT_PHONE > 0
            PORT_Hdl_SHELL_MSG_RING_IND((void *)tel);
#endif
        } else {
            if (++s_rcb.ct_clip >= 3) {
                s_rcb.ct_clip = 0;
#if EN_AT_PHONE > 0
                PORT_Hdl_SHELL_MSG_RING_IND((void *)tel);
#endif
            }
        }
                
    }
    return RET_END;
}

/*
********************************************************************************
* HANDLER: +CLCC
********************************************************************************
*/
static INT8U Handler_CLCC(INT8U event, INT8U *sptr, INT16U slen)
{
#if EN_AT_PHONE > 0
    INT8U id, dir, state;
    
    id    = YX_SearchDigitalString(sptr, 30, ',', 1);
    dir   = YX_SearchDigitalString(sptr, 30, ',', 2);
    state = YX_SearchDigitalString(sptr, 30, ',', 3);
    
    if (dir != 0) {                                                            /* 非拨打电话 */
        return RET_END;
    }
    
    switch (state)
    {
    case 0:
        SHELL_SendEvent(SHELL_MAIN_TSK, SHELL_MSG_CALL_SETUP_CNF, (void *)1);
        
        break;
    case 1:
        break;
    case 2:
        break;
    case 3:
        break;
    case 4:
        break;
    case 5:
        break;
    case 6:
        break;
    default:
        break;
    }
#endif
    return RET_END;
}

/*
********************************************************************************
* HANDLER: +CPBR:
********************************************************************************
*/
static INT8U Handler_CPBR(INT8U event, INT8U *sptr, INT16U slen)
{
#if 0
    INT8U index, tellen, textlen;
    INT8U  *tel,  *text;

    tel     = &s_recvbuf[0];
    tellen  = YX_SearchString(tel, sizeof(s_recvbuf), sptr, slen, '"', 1);
    text    = &s_recvbuf[tellen];
    textlen = YX_SearchString(text, sizeof(s_recvbuf) - tellen, sptr, slen, '"', 2);
    index   = YX_SearchDigitalString(sptr, slen, ',', 1);
    HdlPhoneBookRec(index, tel, tellen, text, textlen);
    return RET_END;
#endif
    return RET_END;
}

/*
********************************************************************************
* HANDLER: +CMT:
********************************************************************************
*/
static INT8U Handler_CMT(INT8U event, INT8U *sptr, INT16U slen)
{
    SM_T *sm;
    
    if (event == EVT_RESET) {
        return RET_END;
    }
    
    if (event == EVT_OVERTIME) {
        return RET_END;
    }

    if (s_rcb.n_recv == 0) {
        s_rcb.pdulen = YX_SearchDigitalString(sptr, slen, '\r', 1);
        return RET_CONTINUE;
    } else if (s_rcb.n_recv == 1) {
        sm = (SM_T *)YX_DYM_Alloc(sizeof(SM_T));
        if (sm != 0) {
            if (ParsePDUData(sm, sptr, slen, s_rcb.pdulen)) {
                PORT_Hdl_SHELL_MSG_SMS_RECV_IND(sm);
            } else {
                #if DEBUG_AT > 0
                printf_com("<receive error SM>\r\n");
                #endif
            }
            YX_DYM_Free(sm);
        }
        
        return RET_END;
    } else {
        return RET_END;
    }
}

/*
********************************************************************************
* HANDLER: +CMTI:
********************************************************************************
*/
static INT8U Handler_CMTI(INT8U event, INT8U *sptr, INT16U slen)
{
    INT16U index;
    
    event = event;
    index = YX_SearchDigitalString(sptr, slen, ',', 1);
    if (index != 0xffff) {
        //NotifyUnReadSM(index);
        PORT_Hdl_SHELL_MSG_SMS_UNREAD_IND((void *)index);
    }
    
    #if DEBUG_AT > 0
    printf_com("<Handler_CMTI(%d)>\r\n", index);
    #endif
    
    return RET_END;
}

/*
********************************************************************************
* HANDLER: +CMGL:
********************************************************************************
*/
static INT8U Handler_CMGL(INT8U event, INT8U *sptr, INT16U slen)
{
    INT16U index;
    
    event = event;
    if (s_rcb.n_recv == 0) {
        index = YX_SearchDigitalString(sptr, slen, ',', 1);
        if (index != 0xffff) {
            //NotifyUnReadSM(index);
            PORT_Hdl_SHELL_MSG_SMS_UNREAD_IND((void *)index);
        }
        
        #if DEBUG_AT > 0
        printf_com("<Handler_CMGL(%d)>\r\n", index);
        #endif
        
        return RET_CONTINUE;
    } else {
        return RET_END;
    }
}

#if GSM_TYPE == GSM_SIMCOM
/*
********************************************************************************
* HANDLER: +PDP: DEACT
********************************************************************************
*/
static INT8U Handler_DeactivePDP(INT8U event, INT8U *sptr, INT16U slen)
{
    AT_GPRS_InformPDPDeactive(0);
    return RET_END;
}

/*
********************************************************************************
* HANDLER: +CDNSGIP:根据域名获取IP地址
********************************************************************************
*/
static INT8U Handler_GetIpByDomainName(INT8U event, INT8U *sptr, INT16U slen)
{
    INT8U i, result, pos, snbits;
    INT32U ip[2];
    char temp[16];
    char domainname[50];
    
    domainname[0] = '\0'; 
    result = YX_SearchDigitalString(sptr, slen, ',', 1);
    if (result == 1) {
        pos = YX_SearchString((INT8U *)domainname, sizeof(domainname) - 1, sptr, slen, '"', 1);/* 查找域名 */
        if (pos == 0) {
            AT_GPRS_InformGetIpByDomainName(false, domainname, 0, ip);
            return RET_END;
        }
        domainname[pos] = '\0';

        for (i = 0; i < 10; i++) {                                             /* 解析IP */
            pos = YX_SearchString((INT8U *)temp, sizeof(temp) - 1, sptr, slen, '"', i + 2);
            if (pos == 0) {
                break;
            }
            
            temp[pos] = '\0';
            if (YX_ConvertIpStringToHex(&ip[i], &snbits, temp) != 0) {
                break;
            }
        }
        AT_GPRS_InformGetIpByDomainName(true, domainname, i, ip);
    } else {
        AT_GPRS_InformGetIpByDomainName(false, domainname, 0, ip);
    }

    return RET_END;
}

/*
********************************************************************************
* HANDLER: CONNECT OK
********************************************************************************
*/
static INT8U Handler_SocketConnect(INT8U event, INT8U *sptr, INT16U slen)
{
    INT8U socket;
    
    socket = YX_SearchDigitalString(sptr, 30, ',', 1);
    if (socket >= SOCKET_CH_MAX) {
        return RET_END;
    }
    
    AT_SOCKET_InformSocketConnect(socket);
    return RET_END;
}

/*
********************************************************************************
* HANDLER: CONNECT FAIL
********************************************************************************
*/
static INT8U Handler_SocketClose(INT8U event, INT8U *sptr, INT16U slen)
{
    INT8U socket;
    
    socket = YX_SearchDigitalString(sptr, 30, ',', 1);
    if (socket >= SOCKET_CH_MAX) {
        return RET_END;
    }
    
    AT_SOCKET_InformSocketDisconnect(socket);
    return RET_END;
}

/*
********************************************************************************
* HANDLER: +RECEIVE
********************************************************************************
*/
static INT8U Handler_SocketRecv(INT8U event, INT8U *sptr, INT16U slen)
{
    INT16U iplen, ch;
    
    iplen = YX_SearchDigitalString(sptr, 30, ':', 1);
    if (iplen >= slen) {
        return RET_END;
    }
    
    ch = YX_SearchDigitalString(sptr, 30, ',', 2);
    if (ch >= SOCKET_CH_MAX) {
        return RET_END;
    }
    
    sptr += (slen - iplen);
    AT_SOCKET_HdlRecvData(ch, sptr, iplen);
    return RET_END;
}
/*
********************************************************************************
* HANDLER: DATA ACCEPT:
********************************************************************************
*/
static INT8U Handler_SocketSend(INT8U event, INT8U *sptr, INT16U slen)
{
    INT16U iplen, ch;
    
    ch = YX_SearchDigitalString(sptr, 30, ',', 1);
    if (ch >= SOCKET_CH_MAX) {
        return RET_END;
    }
    
    iplen = YX_SearchDigitalString(sptr, 30, '\r', 1);
    AT_SOCKET_HdlSendDataAck(ch, iplen);
    
    return RET_END;
}
/*
********************************************************************************
* HANDLER: SEND FAIL
********************************************************************************
*/
static INT8U Handler_SocketSendFail(INT8U event, INT8U *sptr, INT16U slen)
{
    INT16U ch;
    
    ch = YX_SearchDigitalString(sptr, 30, ',', 1);
    if (ch >= SOCKET_CH_MAX) {
        return RET_END;
    }
    AT_SOCKET_HdlSendDataAck(ch, 0);
    
    return RET_END;
}

/*
********************************************************************************
* HANDLER: +IPD
********************************************************************************
*/
static INT8U Handler_IPD(INT8U event, INT8U *sptr, INT16U slen)
{
    INT16U iplen;
    
    event = event;

    if ((iplen = YX_SearchDigitalString(sptr, slen, ':', 1)) >= slen) {
        return RET_END;
    }
    sptr += (slen - iplen);
    //HdlMsg_GPRS_DATA(sptr, iplen);
    return RET_END;
}
#endif

#if 0
/*
********************************************************************************
* HANDLER: +CTTS
********************************************************************************
*/
void ADP_TTS_InformPlayOver(void);
static INT8U Handler_TTSStatus(INT8U event, INT8U *sptr, INT16U slen)
{
    if (YX_SearchDigitalString(sptr, slen, '\r', 1) == 0) {
        ADP_TTS_InformPlayOver();
    }
    return RET_END;
}

/*
********************************************************************************
* HANDLER: +QADC
********************************************************************************
*/
static INT8U Handler_ADC(INT8U event, INT8U *sptr, INT16U slen)
{
    if (s_rcb.n_recv <= 1) {
        return RET_CONTINUE;
    } else {
        return RET_END;
    }
}

/*
********************************************************************************
* HANDLER: +QENG
********************************************************************************
*/

static RF_CELL_INFO_T s_cell;
void GetCellInfo(RF_CELL_INFO_T *cell)
{
    YX_MEMCPY(cell, &s_cell, sizeof(s_cell));
}

static INT16U ReadValue(INT8U **ptr, INT16U *len)
{
    INT8U *sptr;
    INT16U value, slen, pos;
    
    sptr = *ptr;
    slen = *len;
    value = YX_SearchDigitalString(sptr, *len, ',', 1);
    pos = YX_FindCharPos(sptr, ',', 0, *len) + 1;
    *ptr = sptr + pos;
    *len = slen - pos;
    
    if (*sptr == 'x') {
        return 0;
    } else if (*sptr == '-') {
        return (-value);
    } else {
        return value;
    }
}

static INT16U ReadHexValue(INT8U **ptr, INT16U *len)
{
    INT8U *sptr;
    INT16U value, slen, pos;
    
    sptr = *ptr;
    slen = *len;
    value = YX_SearchHexString(sptr, *len, ',', 1);
    pos = YX_FindCharPos(sptr, ',', 0, *len) + 1;
    *ptr = sptr + pos;
    *len = slen - pos;
    
    if (*sptr == 'x') {
        return 0;
    } else if (*sptr == '-') {
        return (-value);
    } else {
        return value;
    }
}

static INT8U Handler_QENG(INT8U event, INT8U  *sptr, INT16U slen)
{
    INT8U *ptr;
    INT8U i, index, mode, dump;
    INT16U len;

    /*  实例解析
    [15:24:26] AT_RECV(0):AT+QENG?
    [15:24:26] AT_RECV(0):+QENG: 1,0
    [15:24:27] AT_RECV(0):+QENG: 0,460,00,592f,2976,520,44,-78,45,69,0,20,x,x,x,x,x,x,x
    [15:24:27] AT_RECV(0):OK
    
    AT_RECV(0):+QENG: 0,460,00,592f,66fa,65,25,-76,85,85,5,12,x,x,x,x,x,x,x
<Handler_QENG,mcc:(460) mnc:(0) lac:(0x592f) cellid:(65535) bcch:(65) bsic:(25) dbm:(76) c1:(85) c2:(85) 
txp:(5) rla:(12) tch:(-1) ts:(65535) maio:(-1) hsn:(-1) ta:(65535) rxq_sub:(65535) rxq_full:(65535)> 
    */
    
    ptr = sptr;
    len = slen;
    if (len < 20) {
        mode = YX_SearchDigitalString(sptr, len, ',', 1);
        dump = YX_SearchDigitalString(sptr, len, '\r', 1);
        if (mode != 1 || dump != 1) {
            SetQeng(true, 0);
        }
        #if DEBUG_AT > 0
        printf_com("<Handler_QENG(%d)(%d)>\r\n", mode, dump);
        #endif
    } else {
        index = ReadValue(&ptr, &len);
        if (index == 0) {
            YX_MEMSET(&s_cell, 0, sizeof(s_cell));
            s_cell.num = 1;
            s_cell.cell[index].cmcc_cell.mcc      = ReadValue(&ptr, &len);
            s_cell.cell[index].cmcc_cell.mnc      = ReadValue(&ptr, &len);
            s_cell.cell[index].cmcc_cell.lac      = ReadHexValue(&ptr, &len);
            s_cell.cell[index].cmcc_cell.cellid   = ReadHexValue(&ptr, &len);
            s_cell.cell[index].cmcc_cell.bcch     = ReadValue(&ptr, &len);
            s_cell.cell[index].cmcc_cell.bsic     = ReadValue(&ptr, &len);
            s_cell.cell[index].cmcc_cell.dbm      = ReadValue(&ptr, &len);
            s_cell.cell[index].cmcc_cell.c1       = ReadValue(&ptr, &len);
            s_cell.cell[index].cmcc_cell.c2       = ReadValue(&ptr, &len);
            s_cell.cell[index].cmcc_cell.txp      = ReadValue(&ptr, &len);
            s_cell.cell[index].cmcc_cell.rla      = ReadValue(&ptr, &len);
            s_cell.cell[index].cmcc_cell.tch      = ReadValue(&ptr, &len);//X
            s_cell.cell[index].cmcc_cell.ts       = ReadValue(&ptr, &len);
            s_cell.cell[index].cmcc_cell.maio     = ReadValue(&ptr, &len);
            s_cell.cell[index].cmcc_cell.hsn      = ReadValue(&ptr, &len);
            s_cell.cell[index].cmcc_cell.ta       = ReadValue(&ptr, &len);
            s_cell.cell[index].cmcc_cell.rxq_sub  = ReadValue(&ptr, &len);
            s_cell.cell[index].cmcc_cell.rxq_full = YX_SearchDigitalString(ptr, len, '\r', 1);
            #if DEBUG_AT > 0
            printf_com("<Handler_QENG mcc(%d),mnc(%d),lac(0x%x),cellid(0x%x),bcch(%d),bsic(%d),dbm(%d),c1(%d),c2(%d),txp(%d),rla(%d),tch(%d),ts(%d),maio(%d),hsn(%d),ta(%d),rxq_sub(%d),rxq_full(%d)>\r\n", 
                                        s_cell.cell[index].cmcc_cell.mcc,
                                        s_cell.cell[index].cmcc_cell.mnc,
                                        s_cell.cell[index].cmcc_cell.lac,
                                        s_cell.cell[index].cmcc_cell.cellid,
                                        s_cell.cell[index].cmcc_cell.bcch,
                                        s_cell.cell[index].cmcc_cell.bsic,
                                        s_cell.cell[index].cmcc_cell.dbm,
                                        s_cell.cell[index].cmcc_cell.c1,
                                        s_cell.cell[index].cmcc_cell.c2,
                                        s_cell.cell[index].cmcc_cell.txp,
                                        s_cell.cell[index].cmcc_cell.rla,
                                        s_cell.cell[index].cmcc_cell.tch,
                                        s_cell.cell[index].cmcc_cell.ts,
                                        s_cell.cell[index].cmcc_cell.maio,
                                        s_cell.cell[index].cmcc_cell.hsn,
                                        s_cell.cell[index].cmcc_cell.ta,
                                        s_cell.cell[index].cmcc_cell.rxq_sub,
                                        s_cell.cell[index].cmcc_cell.rxq_full);
            #endif
        } else if (index == 1) {
            s_cell.num = 7;
            for (i = 0; i < 6; i++) {
                index = ReadValue(&ptr, &len);
                s_cell.cell[index].cmcc_cell.bcch       = ReadValue(&ptr, &len);
                s_cell.cell[index].cmcc_cell.dbm        = ReadValue(&ptr, &len);
                s_cell.cell[index].cmcc_cell.bsic       = ReadValue(&ptr, &len);
                s_cell.cell[index].cmcc_cell.c1         = ReadValue(&ptr, &len);
                s_cell.cell[index].cmcc_cell.c2         = ReadValue(&ptr, &len);
                s_cell.cell[index].cmcc_cell.mcc        = ReadValue(&ptr, &len);
                s_cell.cell[index].cmcc_cell.mnc        = ReadValue(&ptr, &len);
                s_cell.cell[index].cmcc_cell.lac        = ReadHexValue(&ptr, &len);
                if (i != 5) {
                    s_cell.cell[index].cmcc_cell.cellid = ReadHexValue(&ptr, &len);
                } else {
                    s_cell.cell[index].cmcc_cell.cellid = YX_SearchHexString(ptr, len, '\r', 1);
                }
            
                #if DEBUG_AT > 0
                printf_com("<Handler_QENG mcc(%d),mnc(%d),lac(0x%x),cellid(0x%x),bcch(%d),bsic(%d),dbm(%d),c1(%d),c2(%d),txp(%d),rla(%d),tch(%d),ts(%d),maio(%d),hsn(%d),ta(%d),rxq_sub(%d),rxq_full(%d)>\r\n", 
                                        s_cell.cell[index].cmcc_cell.mcc,
                                        s_cell.cell[index].cmcc_cell.mnc,
                                        s_cell.cell[index].cmcc_cell.lac,
                                        s_cell.cell[index].cmcc_cell.cellid,
                                        s_cell.cell[index].cmcc_cell.bcch,
                                        s_cell.cell[index].cmcc_cell.bsic,
                                        s_cell.cell[index].cmcc_cell.dbm,
                                        s_cell.cell[index].cmcc_cell.c1,
                                        s_cell.cell[index].cmcc_cell.c2,
                                        s_cell.cell[index].cmcc_cell.txp,
                                        s_cell.cell[index].cmcc_cell.rla,
                                        s_cell.cell[index].cmcc_cell.tch,
                                        s_cell.cell[index].cmcc_cell.ts,
                                        s_cell.cell[index].cmcc_cell.maio,
                                        s_cell.cell[index].cmcc_cell.hsn,
                                        s_cell.cell[index].cmcc_cell.ta,
                                        s_cell.cell[index].cmcc_cell.rxq_sub,
                                        s_cell.cell[index].cmcc_cell.rxq_full);
                #endif
            }
        }
    }
    return RET_END;
}

/*
********************************************************************************
* HANDLER: +CPIN: SIM PIN
********************************************************************************
*/
static INT8U Handler_SIMPIN(INT8U event, INT8U *sptr, INT16U slen)
{
    ADP_NET_InformSimPinCodeStatus(true);
    return RET_END;
}
#endif
/*
********************************************************************************
* HANDLER: +CPIN: READY
********************************************************************************
*/
static INT8U Handler_SIMREADY(INT8U event, INT8U *sptr, INT16U slen)
{
    //ADP_NET_InformSimPinCodeStatus(false);
    return RET_END;
}



/*
********************************************************************************
* define receive control block
********************************************************************************
*/
static URC_HDL_TBL_T const s_hdl_tbl[] = {
                  {"START",                2,   true,   0},
                  {"+STIN:",               2,   true,   0},
                  {"SMS DONE",             2,   true,   0},
                  {"PB DONE",              2,   true,   0},
                  
                  {"NO CARRIER",           2,   true,   Handler_NOCARRIER},
                  {"BUSY",                 2,   true,   Handler_NOCARRIER},
                  {"NO ANSWER",            2,   true,   Handler_NOCARRIER},
                  {"NO DIALTONE",          2,   true,   Handler_NOCARRIER},
                                        
                  //{"+QADC",                2,  true,    Handler_ADC},
                  //{"AT+QADC?",             2,   true,   Handler_ADC},
                  //{"+QENG:",               2,   true,   Handler_QENG},
                  
                  //{"+CPIN: SIM PIN",       2,   true,   Handler_SIMPIN},
                  {"+CPIN: READY",         2,   true,   Handler_SIMREADY},

                                        
#if GSM_TYPE == GSM_BENQ
                  {"Wait Socket Open",     2,   true,   0},
#endif

#if GSM_TYPE == GSM_SIM800
                  {"+PDP: DEACT",          2,   true,   Handler_DeactivePDP},
                  {"+CDNSGIP:",            2,   true,   Handler_GetIpByDomainName},
                  
                  {"ALREADY CONNECT",      2,   false,  Handler_SocketConnect},
                  {"CONNECT OK",           2,   false,  Handler_SocketConnect},
                  {"CONNECT FAIL",         2,   false,  Handler_SocketClose},
                  {"CLOSED",               2,   false,  Handler_SocketClose},
                  {"+RECEIVE",             2,   true,   Handler_SocketRecv},
                  {"DATA ACCEPT:",         2,   true,   Handler_SocketSend},
                  {"SEND FAIL",            2,   false,  Handler_SocketSendFail},
                                        
                  {"+IPD",                 2,   true,   Handler_IPD},
                                        
                  //{"+CTTS:",               2,   true,   Handler_TTSStatus},
                                        
#endif
                  {"+CPBR:",               2,   true,   Handler_CPBR},
                  {"RING",                 2,   true,   Handler_RING},
                  {"+CLIP:",               2,   true,   Handler_CLIP},
                  {"+COLP:",               2,   true,   0},
                  {"+CLCC:",               2,   true,   Handler_CLCC},
                  {"+CMGS:",               2,   true,   0},
                  {"+CMT:",                2,   true,   Handler_CMT},
                  {"+CMTI:",               2,   true,   Handler_CMTI},
                  {"+CMGL:",               2,   true,   Handler_CMGL},
                  {"+CMGR:",               2,   true,   Handler_CMT}
                  {"$QCMTI:",              2,   true,   Handler_CMT}
                                     };


static void DestroyRCB(void)
{
    s_rcb.handler = 0;
    OS_StopTmr(s_overtmr);
}

void OverTmrProc(void *pdata)
{
    if (s_rcb.handler != 0) {
        s_rcb.handler(EVT_OVERTIME, 0, 0);
    }
    DestroyRCB();
}

/*void DiagnoseProc_ATRECV(void)
{
    if (s_rcb.handler != 0) {
        OS_ASSERT(OS_TmrIsRun(s_overtmr), RETURN_VOID);
    }
}*/

/*******************************************************************
** 函数名:     AT_RECV_Init
** 函数描述:   初始化函数
** 参数:       无
** 返回:       无
********************************************************************/
void AT_RECV_Init(void)
{
    YX_MEMSET(&s_rcb, 0, sizeof(s_rcb));
    
    s_overtmr = OS_CreateTmr(TSK_ID_HAL, 0, OverTmrProc);
    //OS_InstallDiag(DiagnoseProc_ATRECV);
}

#endif
