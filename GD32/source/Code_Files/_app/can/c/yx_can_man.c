/*
********************************************************************************
** 文件名:     yx_can_man.c
** 版权所有:   (c) 2017 厦门雅迅网络股份有限公司
** 文件描述:   can业务逻辑及协议处理
** 创建人：        谢金成，2017.5.18
********************************************************************************
*/
#include "yx_includes.h"
#include "yx_can_man.h"
#include "yx_com_send.h"
#include "yx_com_recv.h"
#include "yx_protocal_hdl.h"
#include "yx_debugcfg.h"
#include "port_gpio.h"
#include "yx_lock.h"
#include "appmain.h"
#include "Dal_Structs.h"
#include "public.h"
#include "dal_structs.h"
#include "app_update.h"
#include "dal_flash.h"
#include "bal_pp_drv.h"
#include  "bal_input_drv.h"
#include "yx_signal_man.h"
#if EN_UDS > 0
#include "yx_uds_drv.h"
#endif
#define UDS_ID_REC              0x18DA1DF9
#define UDS_ID_SEND             0x18DAF91D

#define UDS_ID_REC1             0x18DA1D00
#define UDS_ID_SEND1            0x18DA001D

#define UDS_ID_REC2             0x18DA1CF9                /* 诊断仪请求CAN ID */
#define UDS_ID_SEND2            0x18DAF91C                /* 终端响应CAN ID */

#define UDS_ID_REC3             0x7E8
#define UDS_ID_SEND3            0x7E0

#define UDS_ID_REC4             0x18EA1DFF
#define UDS_ID_SEND4            0x18EAFF1D

#define UDS_ID_REC5             0x18EA1700
#define UDS_ID_SEND5            0x18EA0017

#define UDS_ID_REC6             0x18DAF100
#define UDS_ID_SEND6            0x18DA00F1


#define UDS_ID_FUNC             0x18DB33F1   /* 功能地址  */


#define NONE_TYPE   0
#define UDS_TYPE    1
#define J1939_TYPE  2
static CAN_PARA_T s_can_para[MAX_CAN_CHN];
static AAPCAN_MSG_T  s_msgbt[MAX_CAN_CHN];                    /* 接收的CAN id表 */
#define MAXPACKETPARANUM         10                            /* 多包接收最多缓存包数 */
#define PACKETOVERTMR            500                            /* 超时清除计数值 */
static PACKET_PARA_T s_packetpara[MAXPACKETPARANUM] ;
static MULTIPACKET_SEND_T s_sendpacket[MAXPACKETPARANUM];
static INT8U  s_packet_tmr;
static INT8U  s_packet_cnt;                                    /* 多包传输的累加流水号 */
static INT8U  s_oilsum_check = 0;                             /* 1表示不发送请求，0表示发送请求 */

static INT32U s_last_packetid;                                 /* 最新的帧id */
static INT8U s_canRxMsg = 0;                             /* can报文接收标志 */
static INT16U  s_canMsgLossTmr;                                /* 清除报文接收标志计数器 */

typedef struct {
    INT32U id;
    INT8U  data[8];
    INT8U  dlc;
    INT8U  can_ch;
    INT8U  resend_num;
    INT8U  time_cnt;
    INT8U  period;
    BOOLEAN is_start;
} RESENT_CAN_T;
#define MAX_RESEND_NUM    2
static RESENT_CAN_T s_resend_can[MAX_RESEND_NUM];

typedef struct {
    CAN_DATA_SEND_T send;   // 发送数据信息
	BOOLEAN send_en;		// 发送使能
	INT16U  period;			// 发送周期
    INT8U   send_cnt;		// 已发送次数
} PERIOD_SEND_T;
static PERIOD_SEND_T s_period_can;  // 固定次数周期报文
/*****************************************************************************
**  函数名:  StartCanResend
**  函数描述: 启动重发
**  参数:    [in] chn  : can通道
**           [in] id   : can id
**           [in] data : can数据
**           [in] dlc  : can长度
**  返回:    无
*****************************************************************************/
static void StartCanResend(INT8U chn, INT32U id, INT8U* data, INT8U dlc)
{    
    INT8U idx;
    for (idx = 0; idx < MAX_RESEND_NUM; idx++) {
        if (s_resend_can[idx].is_start == FALSE) {
            break;
        }
    }
    
    if (idx == MAX_RESEND_NUM) return;
    
    s_resend_can[idx].id = id;
    s_resend_can[idx].dlc = dlc;
    memcpy(s_resend_can[idx].data, data, dlc);
    s_resend_can[idx].can_ch = chn;
    s_resend_can[idx].resend_num = 0;
    s_resend_can[idx].time_cnt = 0;
    s_resend_can[idx].period = 3;    /* 30ms */
    s_resend_can[idx].is_start = TRUE;
}

/*****************************************************************************
**  函数名:  CanResendHdl
**  函数描述: 重发
**  参数:    无
**  返回:    无
*****************************************************************************/
static void CanResendHdl(void)
{      
    INT8U idx;
    CAN_DATA_SEND_T candata;
    for (idx = 0; idx < MAX_RESEND_NUM; idx++) {
        if (s_resend_can[idx].is_start) {
            if (++s_resend_can[idx].time_cnt >= s_resend_can[idx].period) {
                s_resend_can[idx].time_cnt = 0;
                if (++s_resend_can[idx].resend_num >= 2) {
                    s_resend_can[idx].resend_num = 0;
                    s_resend_can[idx].is_start = FALSE;
            }
			
            memset(&candata, 0x00, sizeof(candata));
                candata.can_DLC = s_resend_can[idx].dlc;
                candata.can_id = s_resend_can[idx].id;
            if(candata.can_id <= 0x7ff){
                candata.can_IDE = 0;  /*标准帧*/
            } else {
                candata.can_IDE = 1;
            }
            
            candata.channel = s_resend_can[idx].can_ch;
        	candata.period = 0xffff;
            memcpy(candata.Data, s_resend_can[idx].data, candata.can_DLC);
            PORT_CanSend(&candata);
        }
    }
}
}

/*******************************************************************************
**  函数名称:  CheckCanID
**  功能描述:  确认ID是否有效
**  输入参数:  GPN: 输入GPN TRUE:表示为GPN FALSE:实完整ID
               ID : 传入的CAN ID
               chanl: CAN 通道号  0:通道1; 1:通道2
**  返回参数:  ID: 为0时表示未找到
*******************************************************************************/
static INT32U  CheckCanID(INT32U id,INT8U chanl)
{
    INT8U i;
    INT32U retid = 0;

    for (i = 0; i < MAX_CANIDS; i++) {
        if ((TRUE == s_msgbt[chanl].idcbt[i].isused)
            && ((id == s_msgbt[chanl].idcbt[i].id)
                || ((id & s_msgbt[chanl].idcbt[i].screeid) == (s_msgbt[chanl].idcbt[i].id & s_msgbt[chanl].idcbt[i].screeid))) ) {
            retid = id;
            break;
        }
    }
    return retid;
}

/**************************************************************************************************
**  函数名称:  CAN_TxData
**  功能描述:  CAN发送数据接口函数
**  输入参数:  *data 指向数据区域指针 排列 ID len 数据
**  返回参数:  None
**************************************************************************************************/
BOOLEAN CAN_TxData(INT8U *data, BOOLEAN wait, INT8U channel)
{
    CAN_DATA_SEND_T candata;
    INT32U id;

    memset(&candata, 0x00, sizeof(candata));
    id = bal_chartolong(data);
    candata.can_DLC = data[4];
    candata.can_id = id;
    if(id <= 0x7ff){
        candata.can_IDE = 0;  /*标准帧*/
    } else {
        candata.can_IDE = 1;
    }

    candata.channel = channel;
    candata.period = 0xffff;
    memcpy(candata.Data, &data[5], candata.can_DLC);
    return PORT_CanSend(&candata);
}
/**************************************************************************************************
**  函数名称:  SetCANMsg_Period
**  功能描述:  设置周期性发送CAN报文
**  输入参数:
**  返回参数:
**************************************************************************************************/
BOOLEAN SetCANMsg_Period(INT32U id, INT8U *ptr, INT16U period, INT8U channel)
{
    CAN_DATA_SEND_T candata;

    memset(&candata, 0x00, sizeof(candata));
    candata.can_DLC = ptr[0];
    candata.can_id = id;
    candata.can_IDE = 1;
    candata.channel = channel;
    candata.period = period;
    memcpy(candata.Data, &ptr[1], candata.can_DLC);

    return PORT_CanSend(&candata);

}

/**************************************************************************************************
**  函数名称:  StopCANMsg_Period
**  功能描述:  停止周期发送
**  输入参数:
**  返回参数:
**************************************************************************************************/
BOOLEAN StopCANMsg_Period(INT32U id, INT8U channel)
{
    CAN_DATA_SEND_T candata;


    memset(&candata, 0x00, sizeof(candata));
    candata.can_DLC = 8;
    candata.can_id = id;
    candata.can_IDE = 1;
    candata.channel = channel;
    candata.period = 0;

    return PORT_CanSend(&candata);

}

/**************************************************************************************************
**  函数名称:  FindFreeItem_SendList
**  功能描述:  查找可用的发送项
**  输入参数:  None
**  返回参数:  MAXPACKETPARANUM表示未找到
**************************************************************************************************/
static INT8U FindFreeItem_SendList(void)
{
    INT8U i;

    for (i = 0;i < MAXPACKETPARANUM; i++) {
        if (s_sendpacket[i].packet_com == FALSE) {
            break;
        }
    }
    return i;
}

/**************************************************************************************************
**  函数名称:  Find_J1939Sending
**  功能描述:  查找是否有J1939长帧在发送
**  输入参数:  None
**  返回参数:  TRUE表示找到
**************************************************************************************************/
static BOOLEAN Find_J1939Sending(void)
{
    INT8U i;

    for (i = 0;i < MAXPACKETPARANUM; i++) {
        if ((s_sendpacket[i].prot_type == J1939_TYPE) && (TRUE == s_sendpacket[i].sendcontinue)) {
            return TRUE;
        }
    }
    return FALSE;
}

/**************************************************************************************************
**  函数名称:  SendJ1939FirPacket
**  功能描述:  CAN发送多帧中的第一帧
**  输入参数:  MULTIPACKET_SEND_T
**  返回参数:  None
**************************************************************************************************/
static void SendJ1939FirPacket(MULTIPACKET_SEND_T *sendpacket)
{
    INT8U data[13] = {0x00,0x00,0x00,0x00,0x08,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
    bal_longtochar(data, ((sendpacket->sendid & 0xFF0000FF) | 0xECFF00));
    data[5] = 0x20;//BAM类型报文
    data[6] = sendpacket->packet_totallen;//LSB message size
    data[7] = 0x00;//MSB message size
    data[8] = (sendpacket->packet_totallen+6)/7;//total number of packet
    data[9] = 0xFF;//reserved
    data[10] = (INT8U)((sendpacket->sendid & 0x0000FF00) >>  8);//PGN
    data[11] = (INT8U)((sendpacket->sendid & 0x00FF0000) >> 16);//PGN
    data[12] = 0x00;
    CAN_TxData(data, FALSE, sendpacket->channel);

    sendpacket->sendcontinue = TRUE;//表明正在发送中了
    sendpacket->periods = 8;       //80ms
    sendpacket->tmrcnt = 0;         //计时器,到发送周期的话就开始发送
    sendpacket->packet_tmrcnt = 0;  //用途:超时发送不成功就删除发送帧
    sendpacket->cf_cnt = 0;          //不需要用到块大小
    sendpacket->sendpacket = 1;     //接下去发送第01数据帧
    sendpacket->sendlen = 0;        //已发送数据为0

}

/**************************************************************************************************
**  函数名称:  SendJ1939ContinuePacket
**  功能描述:  CAN发送中间帧
**  输入参数:  MULTIPACKET_SEND_T
**  返回参数:  None
**************************************************************************************************/
static void SendJ1939ContinuePacket(MULTIPACKET_SEND_T *sendpacket)
{
    INT8U len;
    INT8U data[13] = {0x00,0x00,0x00,0x00,0x08,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
    bal_longtochar(data, ((sendpacket->sendid & 0xFF0000FF) | 0xEBFF00));
    data[5] = sendpacket->sendpacket;   //序号
    len = sendpacket->packet_totallen - sendpacket->sendlen;
    if (len > 7){
        memcpy(&data[6],&sendpacket->packet_buf[sendpacket->sendlen],7);
        sendpacket->sendlen = sendpacket->sendlen + 7;
        sendpacket->periods = 8;       //80ms
        sendpacket->tmrcnt = 0;         //计时器,到发送周期的话就开始发送
        sendpacket->packet_tmrcnt = 0;  //用途:超时发送不成功就删除发送帧
        sendpacket->sendpacket++;       //接下去发送第01数据帧
    }else{
        memcpy(&data[6],&sendpacket->packet_buf[sendpacket->sendlen],len);
        sendpacket->sendcontinue = FALSE;//表明发送结束
        sendpacket->packet_com = FALSE;
        sendpacket->prot_type = NONE_TYPE;
        PORT_Free(sendpacket->packet_buf);
    }
    CAN_TxData(data, FALSE, sendpacket->channel);
}

/**************************************************************************************************
**  函数名称:  SendCF
**  功能描述:  CAN发送多帧中的连续帧
**  输入参数:
**  返回参数:  None
**************************************************************************************************/
void SendCF(void)
{
    INT8U i,j;
    //INT8U data[13] = {0x00,0x00,0x00,0x00,0x08,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
    INT8U data[13] = {0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
   // CAN_TxData(data, FALSE, 0);
    for (i = 0; i < MAXPACKETPARANUM; i++) {
        if(UDS_TYPE == s_sendpacket[i].prot_type){
            if ((s_sendpacket[i].packet_com) && (s_sendpacket[i].sendcontinue)) {
                if (s_sendpacket[i].tmrcnt++ >= s_sendpacket[i].periods) {
                    if (s_sendpacket[i].blocksize > 0) {
                        if (s_sendpacket[i].cf_cnt > 0) {
                            s_sendpacket[i].cf_cnt--;
                            if (s_sendpacket[i].cf_cnt == 0) {
                                s_sendpacket[i].sendcontinue = FALSE;       /* 达到块大小 停止下次发送  */
                                #if DEBUG_CAN > 0
                                debug_printf("将停止下次CF发送,%d->0\r\n",  s_sendpacket[i].blocksize);
                                #endif
                            }
                        }
                    }

                    s_sendpacket[i].tmrcnt = 0;
                    #if DEBUG_CAN > 0
                    debug_printf("completelength,%d\r\n",s_sendpacket[i].sendlen);
                    #endif
                    bal_longtochar(data, s_sendpacket[i].sendid);
                    data[5] = 0x20 + s_sendpacket[i].sendpacket;
                    if ((s_sendpacket[i].packet_totallen - s_sendpacket[i].sendlen) > 7) {
                        #if DEBUG_CAN > 0
                        debug_printf("发送连续帧,%d,%x\r\n",i, data[5]);
                        #endif
                        for (j = 0; j < 7 ; j++) {
                            data[6 + j] = s_sendpacket[i].packet_buf[s_sendpacket[i].sendlen + j];
                        }
                        CAN_TxData(data, FALSE, s_sendpacket[i].channel);
                        s_sendpacket[i].sendlen += 7;
                        s_sendpacket[i].sendpacket++;
                        s_sendpacket[i].packet_tmrcnt = 0;
                        if (s_sendpacket[i].sendpacket == 16) {
                            s_sendpacket[i].sendpacket = 0;
                        }
                    } else {
                        #if DEBUG_CAN > 0
                        debug_printf("发送最后帧,%d,%x\r\n",i, data[5]);
                        #endif
                        for (j = 0; j < (s_sendpacket[i].packet_totallen - s_sendpacket[i].sendlen) ; j++) {

                            data[6 + j] = s_sendpacket[i].packet_buf[s_sendpacket[i].sendlen + j];
                        }
                        CAN_TxData(data, FALSE, s_sendpacket[i].channel);
                        s_sendpacket[i].packet_com = FALSE;
                        s_sendpacket[i].sendcontinue = FALSE;
                        if (s_sendpacket[i].packet_buf != NULL) {
                            PORT_Free(s_sendpacket[i].packet_buf);
                            s_sendpacket[i].packet_buf = NULL;
                        }
                        s_sendpacket[i].packet_tmrcnt = 0;
                    }
                }
            }
        }else if((J1939_TYPE == s_sendpacket[i].prot_type) && (s_sendpacket[i].packet_com)){
            if(s_sendpacket[i].sendcontinue){   //已经在发送中了
                 if (s_sendpacket[i].tmrcnt++ >= s_sendpacket[i].periods) {
                    SendJ1939ContinuePacket(&s_sendpacket[i]);
                 }
            }else{                              //刚要发送第一帧
                if(FALSE == Find_J1939Sending())//没有J1939的长帧在发送,才发送下一长帧
                {
                    SendJ1939FirPacket(&s_sendpacket[i]);
                }
            }
        }
    }
}

/**************************************************************************************************
**  函数名称:  SendFirstPacket
**  功能描述:  CAN发送多帧中的第一帧
**  输入参数:  MULTIPACKET_SEND_T
**  返回参数:  None
**************************************************************************************************/
static void SendFF(MULTIPACKET_SEND_T *sendpacket)
{
    INT8U data[13] = {0x00,0x00,0x00,0x00,0x08,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
    INT8U i, highbyte;
    INT32U dw_tmp;

    if (sendpacket->sendid < 0xFFF) {
        sendpacket->recvid = sendpacket->sendid + 8;
    } else {
        dw_tmp  = (sendpacket->sendid & 0xFFFF0000);
        dw_tmp |= ((sendpacket->sendid & 0x0000FF00) >> 8);
        dw_tmp |= ((sendpacket->sendid & 0x000000FF) << 8);
        sendpacket->recvid = dw_tmp;
    }
    bal_longtochar(data, sendpacket->sendid);

    highbyte = (sendpacket->packet_totallen & 0x0F00) >> 8;
    data[5] = 0x10 | highbyte;
    data[6] = sendpacket->packet_totallen;
    for (i = 0; i < 6; i++) {
        data[7 + i] = sendpacket->packet_buf[i];
    }
    sendpacket->sendlen = 6;
    if (FALSE == CAN_TxData(data, FALSE, sendpacket->channel)) {
        #if DEBUG_CAN > 0
        debug_printf("发送第一帧失败\r\n");
        #endif
    }
    sendpacket->sendpacket++;
    sendpacket->packet_tmrcnt = 0;
    #if DEBUG_CAN > 0
    debug_printf("长帧第一帧,%d, %x, %x\r\n",sendpacket->channel, sendpacket->sendid, sendpacket->recvid);
    #endif
}
/**************************************************************************************************
**  函数名称:  J1939_CANMsgAnalyze
**  功能描述:  J1939 CAN报文解析处理函数
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
static BOOLEAN  J1939_CANMsgAnalyze(INT8U *data, INT16U datalen)
{
    CAN_DATA_HANDLE_T* CAN_msg;
    INT32U id;
    INT32U packet_id;
    INT8U  i = 0;

    CAN_msg =  (CAN_DATA_HANDLE_T*)data;

    id      =  bal_chartolong(CAN_msg->id);

    if ((id & 0x00ffff00) == 0x00ecff00) {
        packet_id = (id & 0xff000000) | (CAN_msg->databuf[6] << 16) | (CAN_msg->databuf[5] << 8) | (id & 0x000000ff);

        for (i = 0; i < MAXPACKETPARANUM; i++) {
            /* 已保存id，未传输完，继续接收数据 */
            if ((s_packetpara[i].packet_id == packet_id) && (s_packetpara[i].packet_com == TRUE)) {
                s_last_packetid = packet_id;
                #if DEBUG_J1939 > 0
                debug_printf("EC 已保存id，未传输完，继续接收数据 \r\n");
                #endif
                return TRUE;
            }
        }

        for (i = 0; i < MAXPACKETPARANUM; i++) {
            if (s_packetpara[i].packet_com == FALSE) {
                #if DEBUG_J1939 > 0
                debug_printf("1939位置 i:%d\r\n", i);
                #endif
                break;
            }
        }

        if (i == MAXPACKETPARANUM) {
            #if DEBUG_J1939 > 0
            debug_printf("EC 多帧缓存已满\r\n");
            #endif
            return FALSE;
        }

        if ((CAN_msg->databuf[0] == 0x20) && (CheckCanID(packet_id, CAN_msg->channel))) {
            s_packetpara[i].packet_com       = TRUE;
            s_packetpara[i].packet_totallen  = (CAN_msg->databuf[2] << 8) + CAN_msg->databuf[1];
            if (s_packetpara[i].packet_totallen > (0xfc)) {
                s_packetpara[i].packet_totallen = 0xfc; //7的倍数
            }
            s_packetpara[i].packet_buf = PORT_Malloc(CAN_msg->databuf[3] * 7 + 11);
            if (s_packetpara[i].packet_buf == NULL) {
                s_packetpara[i].packet_com = FALSE;
            } else {
                s_last_packetid = packet_id;
                s_packetpara[i].packet_id = packet_id;
                s_packetpara[i].packet_index = 0;
                s_packetpara[i].packet_total = CAN_msg->databuf[3];
                s_packetpara[i].packet_tmrcnt  = 0;
            }
        }
        #if DEBUG_J1939 > 0
        debug_printf("EC 首帧成功处理\r\n");
        #endif
        return TRUE;

     } else if ((id & 0x00ffff00) == 0x00ebff00) {
        for (i = 0; i < MAXPACKETPARANUM; i++) {
            if (((s_packetpara[i].packet_id & 0x000000ff) == (id & 0x000000ff)) 
                && (s_packetpara[i].packet_com)) {
                if (s_packetpara[i].packet_id == s_last_packetid) {
                    break;
                }
            }
        }

        if (i == MAXPACKETPARANUM) {
            #if DEBUG_J1939 > 0
            debug_printf("EB 多帧缓存已满\r\n");
            #endif
           return FALSE;
        }

        if ((CAN_msg->databuf[0] <= s_packetpara[i].packet_total) && (CAN_msg->databuf[0] != 0)) {
            if ((CAN_msg->databuf[0] == s_packetpara[i].packet_index) || (CAN_msg->databuf[0] == s_packetpara[i].packet_index + 1)) {
                if (((CAN_msg->databuf[0]) * 7) <= s_packetpara[i].packet_totallen) {
                    memcpy(&s_packetpara[i].packet_buf[(CAN_msg->databuf[0] - 1) * 7 + 11], &CAN_msg->databuf[1], 7);
                } else if (((CAN_msg->databuf[0] - 1) * 7) <= s_packetpara[i].packet_totallen) {   /* 最后一包不足7个字节 */
                    memcpy(&s_packetpara[i].packet_buf[(CAN_msg->databuf[0] - 1) * 7 + 11], &CAN_msg->databuf[1], s_packetpara[i].packet_totallen - (CAN_msg->databuf[0] - 1) * 7);
                }
                s_packetpara[i].packet_index++;
				s_packetpara[i].packet_tmrcnt = 0;	/* 收到连续帧后, 重新计时 */
                #if DEBUG_J1939 > 0
                debug_printf("中间多帧\r\n");
                #endif

                if (CAN_msg->databuf[0] == s_packetpara[i].packet_total) {
                    s_packetpara[i].packet_buf[0] = CAN_msg->channel + 1;
                    bal_longtochar(&s_packetpara[i].packet_buf[1], s_packetpara[i].packet_id);
                    s_packetpara[i].packet_buf[5] = 0x01;                           /* 一包 */
                    s_packetpara[i].packet_buf[6] = CAN_msg->format;
                    s_packetpara[i].packet_buf[7] = 0x00;                           /* reserve */
                    s_packetpara[i].packet_buf[8] = s_packet_cnt++;                 /* seq */
                    bal_shorttochar(&s_packetpara[i].packet_buf[9], s_packetpara[i].packet_totallen);/* LEN: 2 BYTE*/

                    YX_COM_DirSend(DATA_REPORT_CAN, s_packetpara[i].packet_buf , s_packetpara[i].packet_totallen + 11);
                    #if DEBUG_J1939 > 0
                    debug_printf("多帧接收完成\r\n");
                    printf_hex(s_packetpara[i].packet_buf, s_packetpara[i].packet_totallen + 11);
                    debug_printf("\r\n");
                    #endif
                    s_packetpara[i].packet_com = FALSE;
                    s_packetpara[i].packet_tmrcnt = 0;
                    if (s_packetpara[i].packet_buf != NULL) {
                        PORT_Free(s_packetpara[i].packet_buf);
                        s_packetpara[i].packet_buf = NULL;
                    }
                    s_packetpara[i].packet_id = 0x00;
                }

                return TRUE;
            } else {
                if (CAN_msg->databuf[0] == s_packetpara[i].packet_total) {
                    #if DEBUG_J1939 > 0
                    debug_printf("异常帧最后一帧, i:%d packet_id:%x total:%d index:%d\r\n", i, s_packetpara[i].packet_id, s_packetpara[i].packet_total, s_packetpara[i].packet_index);
                    #endif
                    s_packetpara[i].packet_com = FALSE;
                    if (s_packetpara[i].packet_buf != NULL ) {
                        PORT_Free(s_packetpara[i].packet_buf);
                        s_packetpara[i].packet_buf = NULL;
                    }
                    s_packetpara[i].packet_id = 0x00;
                }
            }
        }
   }


   #if DEBUG_J1939 > 0
   debug_printf("id:%x error\r\n", id);
   #endif

   return FALSE;
}

/**************************************************************************************************
**  函数名称:  UDS_CANMsgAnalyze
**  功能描述:  UDS CAN报文解析处理函数
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
static BOOLEAN UDS_CANMsgAnalyze(INT8U *data, INT16U datalen)
{
    INT8U data2[13] = {0x00,0x00,0x00,0x01,0x08,0x30,0x00,0x32,0x00,0x00,0x00,0x00,0x00};
    CAN_DATA_SEND_T senddata;
    CAN_DATA_HANDLE_T* CAN_msg;
    INT32U id;
    INT8U  i = 0;
    INT8U  leftlen;
    INT8U  midata;

    CAN_msg = (CAN_DATA_HANDLE_T*)data;

    id = bal_chartolong(CAN_msg->id);

    if ((id == UDS_ID_REC) || (id == UDS_ID_REC1) || (id == UDS_ID_REC2) || (id == UDS_ID_REC3)
     || (id == UDS_ID_REC4) || (id == UDS_ID_REC5) || (id == UDS_ID_REC6) || (id == FUNC_REQID) || (id == UDS_PHSCL_REQID)) {//--RF--  处理多包发送时候需要的流控回复
        if (CAN_msg->databuf[0] == 0x30) {
            for (i = 0; i < MAXPACKETPARANUM; i++) {
                #if DEBUG_UDS > 0
                debug_printf("遍历,%d recv%x send%x com%d channel%d\r\n",i,s_sendpacket[i].recvid,s_sendpacket[i].sendid,s_sendpacket[i].packet_com,s_sendpacket[i].channel);
                #endif
                if ((s_sendpacket[i].recvid == id) && (s_sendpacket[i].packet_com)) {
                    break;
                }
            }
            #if DEBUG_UDS > 0
            debug_printf("收到流控帧,%d\r\n",i);
            #endif

            if (i == MAXPACKETPARANUM) {
                return FALSE;
            }

            s_sendpacket[i].blocksize = CAN_msg->databuf[1];
            s_sendpacket[i].cf_cnt = s_sendpacket[i].blocksize;

            if ((CAN_msg->databuf[2]) <= 0x7f) {
                s_sendpacket[i].periods = CAN_msg->databuf[2]/10;
            } else {
                s_sendpacket[i].periods = 1;
            }
            s_sendpacket[i].tmrcnt = 0;
            s_sendpacket[i].packet_tmrcnt = 0;
            s_sendpacket[i].sendcontinue = TRUE;

            #if DEBUG_UDS > 0
            debug_printf("数据长度%d,已发长度%d,seq:%x,bs:%d,period:%dms",s_sendpacket[i].packet_totallen,s_sendpacket[i].sendlen, s_sendpacket[i].sendpacket, s_sendpacket[i].blocksize, s_sendpacket[i].periods);
            //Debug_PrintHex(true, s_sendpacket[i].packet_buf, s_sendpacket[i].packet_totallen);
            #endif
            return TRUE;
        }else if (CAN_msg->databuf[0] == 0x31) {//wait
            ;//不清packet_tmrcnt,发送超时后放弃该帧
        } else if (CAN_msg->databuf[0] == 0x32) {//溢出
            ;//等待发送超时后放弃该帧
        }
    }
    if ((id == UDS_ID_REC) || (id == UDS_ID_REC1)|| (id == UDS_ID_REC2)|| (id == UDS_ID_REC3)
        || (id == UDS_ID_REC4) || (id == UDS_ID_REC5) || (id == UDS_ID_REC6) || (id == UDS_PHSCL_REQID)) {//--RF--  多包接收处理
        #if DEBUG_UDS > 1
        debug_printf("CAN帧 ");
        printf_hex(CAN_msg->databuf, CAN_msg->len);
        debug_printf("\r\n");
        #endif
        if (((CAN_msg->databuf[0]) & 0xf0) == 0x10) {
            #if DEBUG_UDS > 0
            debug_printf("UDS第一帧1\r\n");
            #endif
            for (i = 0; i < MAXPACKETPARANUM; i++) {
               if(id == s_packetpara[i].packet_id && s_packetpara[i].packet_com == TRUE){/*修改相同id覆盖掉第一包*/
                  s_packetpara[i].packet_com = FALSE;
                  if (s_packetpara[i].packet_buf != NULL ) {
                       PORT_Free(s_packetpara[i].packet_buf);
                       #if DEBUG_UDS > 0
                       debug_printf("接收到相同id， packet_buf 内存释放 i:%d id:%x\r\n", i, s_sendpacket[i].sendid);
                       #endif
                       s_packetpara[i].packet_buf = NULL;
                       break;
                  }
               }
               if (s_packetpara[i].packet_com == FALSE) {
                    #if DEBUG_UDS > 0
                    debug_printf("UDS位置 i:%d\r\n", i);
                    #endif
                    break;
               }
            }
            if (i == MAXPACKETPARANUM) {
                #if DEBUG_UDS > 0
                debug_printf("UDS 多帧缓存已满\r\n");
                #endif
                return FALSE;
            }
            s_packetpara[i].packet_com = TRUE;
            s_packetpara[i].packet_totallen = (((CAN_msg->databuf[0]) & 0x0f) << 8) + CAN_msg->databuf[1] + 1;
            if (s_packetpara[i].packet_totallen < (8 + 1)) { /* 多帧最少8个字节以上 */
		#if DEBUG_UDS > 0
                debug_printf("多帧数据错误\r\n");
                #endif
                s_packetpara[i].packet_com = FALSE;
                return false;
            }
            s_packetpara[i].packet_buf = PORT_Malloc(s_packetpara[i].packet_totallen + 11);
            if (s_packetpara[i].packet_buf == NULL) {
                #if DEBUG_UDS > 0
                debug_printf("没内存\r\n");
                #endif
                s_packetpara[i].packet_com = FALSE;
                return false;
            } else {
                s_packetpara[i].packet_id = id;
                s_packetpara[i].packet_tmrcnt  = 0;
                s_packetpara[i].packet_total  = 0;                                    //这里用于计算已接受到的连续帧包数
                s_packetpara[i].packet_index = 1;                                     //这里表示接下去想要的序号
                memcpy(&s_packetpara[i].packet_buf[11], &CAN_msg->databuf[1], 7);
                if (id > 0xFFF) {
                    bal_longtochar(data2, s_packetpara[i].packet_id);
                    midata = data2[2];
                    data2[2] = data2[3];
                    data2[3] = midata;
                } else {
                    bal_longtochar(data2, s_packetpara[i].packet_id - 8);
                }
                #if 1
                senddata.can_id = bal_chartolong(data2);
                if(senddata.can_id <= 0x7ff){
                     senddata.can_IDE = 0;  /*标准帧*/
                } else {
                     senddata.can_IDE = 1;
                }
                senddata.can_DLC = 8;
                senddata.period = 0xffff;
                senddata.channel = CAN_msg->channel;
                memcpy(senddata.Data, data2 + 5, senddata.can_DLC);
                #endif


                if (FALSE == PORT_CanSend(&senddata)) {              /* 发送流控帧,使其没50毫秒发送一帧 */
                    s_packetpara[i].packet_com = FALSE;
                    if (s_packetpara[i].packet_buf != NULL ) {
                        PORT_Free(s_packetpara[i].packet_buf);
                        s_packetpara[i].packet_buf = NULL;
                    }
                    #if DEBUG_UDS > 0
                    debug_printf("流控不成功\r\n");
                    #endif
                }
                return TRUE;
            }
        }
        if (((CAN_msg->databuf[0]) & 0xf0) == 0x20) {
            for (i = 0; i < MAXPACKETPARANUM; i++) {
                if ((s_packetpara[i].packet_id == id) && (s_packetpara[i].packet_com)) {
                    #if DEBUG_UDS > 0
                    debug_printf("符合\r\n");
                    #endif
                    break;
                }
                #if DEBUG_UDS > 0
                debug_printf("packet_id[%d]:%x==id:%x, pk_com:%d\r\n", i, s_packetpara[i].packet_id, id, s_packetpara[i].packet_com);
                #endif
            }
            if (i == MAXPACKETPARANUM) {
                #if DEBUG_UDS > 0
                debug_printf("meiyouweizhi\r\n");
                #endif
                return FALSE;
            }

            #if DEBUG_UDS > 0
            debug_printf("index[%d]:%x, total:%d, pklen:%d\r\n", s_packetpara[i].packet_index, CAN_msg->databuf[0], s_packetpara[i].packet_total, s_packetpara[i].packet_totallen);
            #endif
            if (((CAN_msg->databuf[0]) & 0x0f) == s_packetpara[i].packet_index++) {
                s_packetpara[i].packet_total++;
                if (s_packetpara[i].packet_index > 15) {
                    s_packetpara[i].packet_index = 0;
                }
                if ((s_packetpara[i].packet_total * 7 + 7) < s_packetpara[i].packet_totallen) {
                    memcpy(&s_packetpara[i].packet_buf[11 + s_packetpara[i].packet_total * 7], &CAN_msg->databuf[1], 7);
                    return true;
                } else {
                    leftlen = s_packetpara[i].packet_totallen - ((s_packetpara[i].packet_total) * 7);
                    memcpy(&s_packetpara[i].packet_buf[11 + s_packetpara[i].packet_total * 7], &CAN_msg->databuf[1], leftlen);
                    s_packetpara[i].packet_com = FALSE;
                       #if 0
                        s_packetpara[i].packet_buf[3] = CAN_msg->channel + 1;
                        bal_longtochar(&s_packetpara[i].packet_buf[4], s_packetpara[i].packet_id);
                        s_packetpara[i].packet_buf[4] = 1;
                    s_packetpara[i].packet_buf[9] = CAN_msg->format + 1;
                        s_packetpara[i].packet_buf[10] = s_packetpara[i].packet_totallen;
                        YX_COM_DirSend(DATA_REPORT_CAN, s_packetpara[i].packet_buf + 3, s_packetpara[i].packet_totallen + 8);
                      #else
											#if EN_UDS > 0
										  if(!YX_UDS_MultFrameRecv(s_packetpara[i].packet_id,&s_packetpara[i].packet_buf[12],s_packetpara[i].packet_totallen -1)) {		
                          s_packetpara[i].packet_buf[0] = CAN_msg->channel + 1;
                          bal_longtochar(&s_packetpara[i].packet_buf[1], s_packetpara[i].packet_id);
                          s_packetpara[i].packet_buf[5] = 0x01;                           /* 一包 */
                          s_packetpara[i].packet_buf[6] = CAN_msg->format + 1;
                          s_packetpara[i].packet_buf[7] = 0x00;                           /* reserve */
                          s_packetpara[i].packet_buf[8] = s_packet_cnt++;                 /* seq */
                          bal_shorttochar(&s_packetpara[i].packet_buf[9], s_packetpara[i].packet_totallen);/* LEN: 2 BYTE*/
                          YX_COM_DirSend(DATA_REPORT_CAN, s_packetpara[i].packet_buf, s_packetpara[i].packet_totallen + 11);
										  } 
											#else
                         s_packetpara[i].packet_buf[0] = CAN_msg->channel + 1;
                         bal_longtochar(&s_packetpara[i].packet_buf[1], s_packetpara[i].packet_id);
                         s_packetpara[i].packet_buf[5] = 0x01;                           /* 一包 */
                         s_packetpara[i].packet_buf[6] = CAN_msg->format + 1;
                         s_packetpara[i].packet_buf[7] = 0x00;                           /* reserve */
                         s_packetpara[i].packet_buf[8] = s_packet_cnt++;                 /* seq */
                         bal_shorttochar(&s_packetpara[i].packet_buf[9], s_packetpara[i].packet_totallen);/* LEN: 2 BYTE*/
                         YX_COM_DirSend(DATA_REPORT_CAN, s_packetpara[i].packet_buf, s_packetpara[i].packet_totallen + 11);
											#endif
                      #endif
                    s_packetpara[i].packet_tmrcnt  = 0;
                    //CANDataReprot(s_packetpara[i].packet_buf, s_packetpara[i].packet_totallen + 11);
                    #if DEBUG_CAN > 0
                    debug_printf_dir("最后一帧:");
                    printf_hex_dir(s_packetpara[i].packet_buf, s_packetpara[i].packet_totallen + 11);
                    debug_printf_dir("\r\n");
                    #endif
                    
                    if (s_packetpara[i].packet_buf != NULL) {
                        PORT_Free(s_packetpara[i].packet_buf);
                        s_packetpara[i].packet_buf = NULL;
                    }
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

/*******************************************************************
** 函数名:     Can_LossCanMsg
** 函数描述:   丢失CAN 报文计数
** 参数:       无
** 返回:       无
********************************************************************/
static void Can_LossCanMsg(void)
{
    if (s_canRxMsg & 0x3F) {
        if (s_canMsgLossTmr) {
            s_canMsgLossTmr--;
        } else {
            s_canRxMsg = 0;
        }
    }
}

/*******************************************************************
** 函数名:     Can_RxMsg
** 函数描述:   接收到CAN 报文
** 参数:       无
** 返回:       无
********************************************************************/
static void Can_RxMsg(INT8U channel)
{
    switch (channel){
		case 0:
			s_canRxMsg |= 0x01;
			break;
		case 1:
			s_canRxMsg |= 0x02;
			break;
		default:
			s_canRxMsg = 0;
			break;
	}
    s_canMsgLossTmr = 200;
}
/*******************************************************************
** 函数名:     Can_TxMsg
** 函数描述:   发送CAN 报文成功
** 参数:       无
** 返回:       无
********************************************************************/
void Can_TxMsg(INT8U channel)
{
    switch (channel){
		case 0:
			s_canRxMsg |= 0x01;
			break;
		case 1:
			s_canRxMsg |= 0x02;
			break;
		default:
			s_canRxMsg = 0;
			break;
	}
    s_canMsgLossTmr = 200;
}

/*******************************************************************
**  函数名称:  Can_GetRxStat
**  功能描述:  获取can通信状态
**  输入参数:
**  返回参数:
*******************************************************************/
INT8U Can_GetRxStat(void)
{
    return s_canRxMsg;
}

/*******************************************************************************
**  函数名称:  CANDataHdl
**  功能描述:  CAN报文底层回调解析处理函数
**  输入参数:  None
**  返回参数:  None
*******************************************************************************/
void CANDataHdl(CAN_DATA_HANDLE_T *CAN_msg)
{
    INT32U id;
    //INT32U packet_id;
    INT8U        tempbuf[220];
    INT8U        rxobj;
    INT8U        i, j;
    INT16U       len;

    Can_RxMsg(CAN_msg->channel);
	
    id = bal_chartolong(CAN_msg->id);
    if(id == 0x18FEE900){   //油耗报文
        s_oilsum_check = 1;
		#if DEBUG_CAN > 0
	    debug_printf("can:%d, 接收到id = %x ",CAN_msg->channel, id);
	    printf_hex(CAN_msg->databuf, CAN_msg->len);
	    #endif
    }
	if (id == 0x18EA4A00){	//防拆报文
		INT8U data[13] = {0x18, 0xFD, 0xA9, 0x4A, 0x08, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
		CAN_TxData(data, FALSE,CAN_msg->channel);
		return ;
	}
    #if DEBUG_CAN > 0
    debug_printf("can:%d, 接收到id = %x ",CAN_msg->channel, id);
    printf_hex(CAN_msg->databuf, CAN_msg->len);
    #endif
    HandShakeMsgAnalyze(CAN_msg,sizeof(CAN_msg));
    #if EN_UDS > 0
		if(UDS_SingleFrameHdl((INT8U*)CAN_msg, sizeof(CAN_DATA_HANDLE_T))) {
		   return;
		}
    #endif

    for (i = 0; i < MAX_CANIDS; i++) {
        if ((id == s_msgbt[CAN_msg->channel].idcbt[i].id
            || ((id & s_msgbt[CAN_msg->channel].idcbt[i].screeid) == (s_msgbt[CAN_msg->channel].idcbt[i].id & s_msgbt[CAN_msg->channel].idcbt[i].screeid)) )
              && (TRUE == s_msgbt[CAN_msg->channel].idcbt[i].isused)) {
            break;
        }
    }
    if (s_msgbt[CAN_msg->channel].idcbt[i].mode == CAN_COVER_OLD) {
        rxobj = i + s_msgbt[CAN_msg->channel].idcbt[i].storeadd;
        if (id != s_msgbt[CAN_msg->channel].idcbt[rxobj].id) {
            return;
        }
    } else {
        rxobj = i;
    }
    if (rxobj >= MAX_CANIDS) {
        return;
    }
    s_msgbt[CAN_msg->channel].idcbt[rxobj].len = CAN_msg->len;          //存储单包长度
    memcpy(s_msgbt[CAN_msg->channel].idcbt[rxobj].storebuf, CAN_msg->databuf, CAN_msg->len);
    s_msgbt[CAN_msg->channel].recvcnt[rxobj]++;                        //流水号增加
    s_msgbt[CAN_msg->channel].format[rxobj] = CAN_msg->format + 1;
    s_msgbt[CAN_msg->channel].type[rxobj] = CAN_msg->type;

    if (J1939_CANMsgAnalyze((INT8U*)CAN_msg, sizeof(CAN_DATA_HANDLE_T))) {
        return;
    }

    if (UDS_CANMsgAnalyze((INT8U*)CAN_msg, sizeof(CAN_DATA_HANDLE_T))) {
        return;
    }
    switch (s_can_para[CAN_msg->channel].mode) {
        case CAN_MODE_LUCIDLY:                                        //透传模式
            tempbuf[0] = CAN_msg->channel + 1;
            memcpy(&tempbuf[1], CAN_msg->id, 4);                      //ID
            tempbuf[5] = s_msgbt[CAN_msg->channel].format[rxobj];
            tempbuf[6] = 0;
            tempbuf[7] = 0;
            tempbuf[8] = s_msgbt[CAN_msg->channel].recvcnt[rxobj];    //流水号
            tempbuf[9] = CAN_msg->len;                                //数据包长度
            memcpy(&tempbuf[10], CAN_msg->databuf, CAN_msg->len);     //拷贝数据包
            YX_COM_DirSend( DATA_LUCIDLY_CAN, tempbuf, CAN_msg->len + 10);
            break;
        case CAN_MODE_REPORT:                                         //主动上报
            s_msgbt[CAN_msg->channel].idcbt[i].storeadd++;
            if ((++s_msgbt[CAN_msg->channel].idcbt[i].storecnt >= s_msgbt[CAN_msg->channel].idcbt[i].stores) && (s_msgbt[CAN_msg->channel].idcbt[rxobj].stores != 0xff)) {
                tempbuf[0] = CAN_msg->channel + 1;
                memcpy(&tempbuf[1], CAN_msg->id, 4);
                if (s_msgbt[CAN_msg->channel].idcbt[i].mode == CAN_COVER_OLD) {
                    tempbuf[5] = s_msgbt[CAN_msg->channel].idcbt[i].stores;
                    len = 6;
                    for (j = 0; j < s_msgbt[CAN_msg->channel].idcbt[i].stores; j++) {
                        tempbuf[len++] = s_msgbt[CAN_msg->channel].format[i + j];
                        tempbuf[len++] = 0;
                        tempbuf[len++] = s_msgbt[CAN_msg->channel].recvcnt[i + j]; //流水号
                        tempbuf[len++] = 0;
                        tempbuf[len++] = s_msgbt[CAN_msg->channel].idcbt[i + j].len; //单包长度
                        memcpy(&tempbuf[len], s_msgbt[CAN_msg->channel].idcbt[i + j].storebuf, s_msgbt[CAN_msg->channel].idcbt[i + j].len);
                        len += s_msgbt[CAN_msg->channel].idcbt[i + j].len;
                    }
                } else {
                    tempbuf[5] = 1;
                    tempbuf[6] = s_msgbt[CAN_msg->channel].format[rxobj];
                    tempbuf[7] = 0;
                    tempbuf[8] = s_msgbt[CAN_msg->channel].recvcnt[rxobj]; //最后一包流水号
                    tempbuf[9] = 0;
                    tempbuf[10] = CAN_msg->len;                            //最后一包的单包长度
                    memcpy(&tempbuf[11], CAN_msg->databuf, CAN_msg->len);
                    len = CAN_msg->len + 11;
                }
                YX_COM_DirSend( DATA_REPORT_CAN, tempbuf, len);
                s_msgbt[CAN_msg->channel].idcbt[rxobj].storecnt = 0;
                s_msgbt[CAN_msg->channel].idcbt[rxobj].storeadd = 0;
            }
            break;
        case CAN_MODE_QUERY:                                              //被动查询
            if (s_msgbt[CAN_msg->channel].idcbt[i].mode == CAN_COVER_OLD) {
                if (++s_msgbt[CAN_msg->channel].idcbt[i].storeadd >= s_msgbt[CAN_msg->channel].idcbt[i].stores) {
                    s_msgbt[CAN_msg->channel].idcbt[i].storeadd = 0;
                }
                if (++s_msgbt[CAN_msg->channel].idcbt[i].storecnt > s_msgbt[CAN_msg->channel].idcbt[i].stores) {
                    s_msgbt[CAN_msg->channel].idcbt[i].storecnt = s_msgbt[CAN_msg->channel].idcbt[i].stores;
                }
            } else {
                s_msgbt[CAN_msg->channel].idcbt[i].storecnt = 1;
            }
            break;
        default :
            break;
    }




}
/**************************************************************************************************
**  函数名称:  GetOilMsg_State
**  功能描述:  获取油耗报文ID：0x18FEE900状态
**  输入参数:  无
**  返回参数:  FALSE:收到该ID，TRUE:没收到该ID
**************************************************************************************************/
BOOLEAN GetOilMsg_State(void)
{
    if(s_oilsum_check == 0){
        return TRUE;
    }else{
        return FALSE;
    }
}

/**************************************************************************************************
**  函数名称:  ResetOilMsg_State
**  功能描述:  重置油耗报文ID：0x18FEE900状态
**  输入参数:  无
**  返回参数:  无
**************************************************************************************************/
void ResetOilMsg_State(void)
{
    s_oilsum_check = 0;
}

/*******************************************************************************
**  函数名称:  GetIDIsUsed
**  功能描述:  获取ID是否被使用
**  输入参数:  idcnts 序号
**  返回参数:  TRUE  or FALSE
*******************************************************************************/
BOOLEAN GetIDIsUsed(INT8U idcnts, INT8U channel)
{
    return s_msgbt[channel].idcbt[idcnts].isused;
}
/**************************************************************************************************
**  函数名称:  GetIDCnts
**  功能描述:  获取当前通道已经配置的过滤ID个数
**  输入参数:  channel: CAN 通道
**  返回参数:  s_can0objcnt_rx / s_can1objcnt_rx
**************************************************************************************************/
INT8U GetIDCnts(INT8U channel)
{
    return s_msgbt[channel].rxobjused;
}

/**************************************************************************************************
**  函数名称:  GetID
**  功能描述:  获取ID值
**  输入参数:  idcnts 序号
**  返回参数:  ID值
**************************************************************************************************/
INT32U GetID(INT8U idcnts, INT8U channel)
{
    return s_msgbt[channel].idcbt[idcnts].id;
}
/**************************************************************************************************
**  函数名称:  GetStoreData
**  功能描述:  app层获取缓存的CAN数据，用于被动查询模式
**  输入参数:
               idcnts  : ID序号
               buf     : 存储缓存数据的起始地址
**  返回参数:  长度
**************************************************************************************************/
INT16U GetStoreData(INT8U idcnts, INT8U *buf, INT8U channel)
{
    INT16U storeaddr;
    INT8U  len;
    INT8U  *ptr;
    INT8U  i;
    INT8U  idadd;
    INT8U  storecnt;

    ptr = buf;

    storeaddr = 0;
    bal_longtochar(ptr, s_msgbt[channel].idcbt[idcnts].id);              /* ID */
    storeaddr = 4;
    if (s_msgbt[channel].idcbt[idcnts].mode == CAN_COVER_OLD) {
        storecnt = s_msgbt[channel].idcbt[idcnts].storecnt;      /* 数据帧数 */
        if (s_msgbt[channel].idcbt[idcnts].storecnt >= s_msgbt[channel].idcbt[idcnts].stores) {
            idadd = idcnts + s_msgbt[channel].idcbt[idcnts].storeadd;
        } else {
            idadd = idcnts;
        }
    } else {
        if (s_msgbt[channel].idcbt[idcnts].storecnt) {
            storecnt = 1;
        } else {
            storecnt = 0;
        }
        idadd = idcnts;
    }
    ptr[storeaddr++] = storecnt;
    for (i = 0; i < storecnt; i++) {
        ptr[storeaddr++] = s_msgbt[channel].type[idadd];
        ptr[storeaddr++] = 0;
        ptr[storeaddr++] = s_msgbt[channel].recvcnt[idadd];          /* 流水号 */
        ptr[storeaddr++] = 0;
        len = s_msgbt[channel].idcbt[idadd].len;                     /* 获取数据包长度 */
        ptr[storeaddr++] = len;
        memcpy(&ptr[storeaddr], (s_msgbt[channel].idcbt[idadd].storebuf), len);
        storeaddr += len;
        if (++idadd >= idcnts + s_msgbt[channel].idcbt[idcnts].stores) {
            idadd = idcnts;
        }
    }
    s_msgbt[channel].idcbt[idcnts].storecnt = 0;                     /* 已经收到的数据帧数清零 */
    s_msgbt[channel].idcbt[idcnts].storeadd = 0;
    return storeaddr;                                                /* 删除最后一次加上的长度 */
}

/**************************************************************************************************
**  函数名称:  GetIDPara
**  功能描述:  获取滤波ID属性参数
**  输入参数:  idcnts 序号
               channel 通道号
**  返回参数:  指向ID属性结构体的地址
**************************************************************************************************/
IDPARA_T* GetIDPara(INT8U idcnts, INT8U channel)
{
    return &s_msgbt[channel].idcbt[idcnts];
}

/**************************************************************************************************
**  函数名称:  SetID
**  功能描述:  设置滤波ID
**  输入参数:  idnums : 要配置的ID的数目
               idpara : ID的相关参数
**  返回参数:  成功TRUE 失败FALSE
**************************************************************************************************/
static BOOLEAN SetID(INT8U idnums, INT8U *idpara, CAN_FILTERCTRL_E onoff, INT8U channel)
{
    INT8U  idindex;
    INT8U  i, k;
    INT8U  tempi;
    IDPARA_T canid;
    INT8U frameformat=0;

    idindex = 0;
    if (onoff == CAN_FILTER_OFF) {
        PORT_ClearCanFilter((CAN_CHN_E)channel);
        return TRUE;
    }
    for (i = 0; i < idnums; i++) {
        for (; idindex < MAX_CANIDS; idindex++) {
            if (FALSE == GetIDIsUsed(idindex, channel)) break;       /* 查找到未被配置的，可以配置 */
        }
        tempi = 2 + i * 6;
        canid.id = bal_chartolong(&idpara[tempi]);
        canid.screeid  = 0xffffffff;
        #if DEBUG_CAN > 0
          debug_printf_dir("id = %x\r\n", canid.id);
        #endif
        canid.stores = idpara[tempi + 4];
        canid.mode = idpara[tempi + 5];
        canid.isused = TRUE;                                         /* 置ID被配置标志 */

        if(canid.id <= 0x7ff){//根据ID判断标准帧还是扩展帧
        	frameformat= FMAT_STD;
        } else {
        	frameformat =FMAT_EXT;
        }
        if (!PORT_SetCanFilter((CAN_CHN_E)channel, frameformat, canid.id, 0xffffffff)){
            return FALSE;
        }

        if (canid.mode == CAN_COVER_OLD) {
            k = canid.stores;
        } else {
            k = 1;
        }
        while (k--) {
            if (idindex >= MAX_CANIDS) {
                return FALSE;
            }
            SetIDPara(&canid, idindex, channel);
            idindex++;
        }
    }
    return TRUE;
}

/*******************************************************************************
**  函数名称:  SetIDPara
**  功能描述:  设置滤波ID属性参数
**  输入参数:  idcnts 序号
               channel 通道号
**  返回参数:  指向ID属性结构体的地址
*******************************************************************************/
void SetIDPara(IDPARA_T *idset, INT8U idcnts, INT8U channel)
{
    if (idset->isused) {
        s_msgbt[channel].recvcnt[idcnts] = 0;                                /* 流水号清零 */
        s_msgbt[channel].idcbt[idcnts].isused = idset->isused;               /* 使用与否 */
        s_msgbt[channel].idcbt[idcnts].storecnt = 0;                         /* 已经接收存储的帧数清零 */
        s_msgbt[channel].idcbt[idcnts].storeadd = 0;
        s_msgbt[channel].idcbt[idcnts].stores = idset->stores;
        s_msgbt[channel].idcbt[idcnts].id = idset->id;                       /* ID */
        s_msgbt[channel].idcbt[idcnts].screeid = idset->screeid & 0x00ffffff;/* 前两个字节不关心 */
        s_msgbt[channel].idcbt[idcnts].mode = idset->mode;

        s_msgbt[channel].rxobjused++;
    } else {
        if (s_msgbt[channel].idcbt[idcnts].isused) {
            s_msgbt[channel].idcbt[idcnts].isused = FALSE;
            s_msgbt[channel].recvcnt[idcnts] = 0;
            if (s_msgbt[channel].rxobjused) {
                s_msgbt[channel].rxobjused--;
            }
        }
    }
}

/*******************************************************************************
**  函数名称:  SetScreenID
**  功能描述:  设置屏蔽模式滤波ID
**  输入参数:  idnums : 要配置的ID的数目
               idpara : ID的相关参数
**  返回参数:  成功TRUE 失败FALSE
*******************************************************************************/
static BOOLEAN SetScreenID(INT8U idnums, INT8U *idpara, CAN_FILTERCTRL_E onoff, INT8U channel)
{
    INT8U  idindex, idcnt;
    INT8U  i, j, k;
    INT16U  tempi;
    IDPARA_T canid;
    INT32U filter_id, screen_id;
    INT8U frameformat=0;

    idindex = 0;
    if (onoff == CAN_FILTER_OFF) {
        PORT_ClearCanFilter((CAN_CHN_E)channel);
    }
    tempi = 2;
    for (i = 0; i < idnums; i++) {
    	filter_id = bal_chartolong(&idpara[tempi]);
        tempi += 4;
        screen_id = bal_chartolong(&idpara[tempi]);
        tempi += 4;
        if(filter_id <= 0x7ff){//根据ID判断标准帧还是扩展帧
        	frameformat= FMAT_STD;
        } else {
        	frameformat =FMAT_EXT;
        }
         #if DEBUG_CAN > 0
            debug_printf_dir("SetID:%x,%x,%d\r\n",filter_id,screen_id,frameformat);
        #endif
        if (!PORT_SetCanFilter((CAN_CHN_E)channel, frameformat, filter_id, screen_id)) {
            #if DEBUG_CAN > 0
                debug_printf_dir("CAN配置失败\r\n");
            #endif
            return FALSE;
        }

        idcnt = idpara[tempi++];
        #if DEBUG_CAN > 0
            debug_printf_dir("idcnt%d\r\n",idcnt);
        #endif

        if (idcnt == 0) {                                            // id个数为0时表示不关心位全接收
            for (; idindex < MAX_CANIDS; idindex++) {
                if (FALSE == GetIDIsUsed(idindex, channel)) break;    //查找到未被配置的，可以配置
            }
            if (idindex >= MAX_CANIDS) {
                return FALSE;
            }

            canid.id      = filter_id;
            canid.screeid = screen_id;
            canid.stores  = 0x01;
            canid.isused  = TRUE;
            SetIDPara(&canid, idindex, channel);
            continue;
        }

        for (j = 0; j < idcnt; j++) {
            for (; idindex < MAX_CANIDS; idindex++) {
                if (FALSE == GetIDIsUsed(idindex, channel)) break;  //查找到未被配置的，可以配置
            }

            canid.id = bal_chartolong(&idpara[tempi]);
            canid.screeid  = 0xffffffff;
            tempi += 4;
            canid.stores = idpara[tempi++];
            canid.mode = idpara[tempi++];
            canid.isused = TRUE;                                   // 置ID被配置标志
            if (canid.mode == CAN_COVER_OLD) {
                k = canid.stores;
            } else {
                k = 1;
            }
            while (k--) {
                if (idindex >= MAX_CANIDS) {
                    return FALSE;
                }
                SetIDPara(&canid, idindex, channel);
                idindex++;
            }
        }
    }
    return TRUE;
}

void Test_CAN_Send(void)
{
    CAN_DATA_SEND_T candata;
	memset(&candata, 0x11, sizeof(candata));
	candata.can_DLC = 8;
	candata.can_id = 0xaa;
	candata.can_IDE = 0;
	candata.period = 0xffff;
	candata.channel = 0;
	PORT_CanSend(&candata);
    candata.can_id = 0xbb;
    PORT_CanSend(&candata);
    
	candata.channel = 1;
    candata.can_id = 0xaa;
	PORT_CanSend(&candata);
    candata.can_id = 0xbb;
    PORT_CanSend(&candata);    
}
/*****************************************************************************
**  函数名:  YX_PeriodDataTran
**  函数描述: 特殊数据发送 发送三帧后停发
**  参数:    [in] arg : 无
**  返回:    无
*****************************************************************************/
static void YX_PeriodDataTran(void )
{
    static INT8U spe_cnt = 0;

	if (s_period_can.send_en){
		spe_cnt++;
		if (s_period_can.period == spe_cnt){		// 达到发送周期
			spe_cnt = 0;
			if (s_period_can.send_cnt++ < 3){
				PORT_CanSend(&s_period_can.send);
			}else{
                s_period_can.send_en = FALSE;
				s_period_can.send_cnt = 0;
			}
		}
	}
}
#if ACC_OFF_STOP_SEND  > 0
/*******************************************************************************
**  函数名称:  SendCanMsg_OnOff
**  功能描述:  开启和关闭CAN发送
**  输入参数:  无
**  输出参数:  无
**  返回参数:  无
*******************************************************************************/
static void SendCanMsg_OnOff(void)
{
    static INT8U   s_op_delay  = 0;
		static BOOLEAN s_op_static = FALSE;
		INT8U  acc_sta;
		
		acc_sta = bal_input_ReadSensorFilterStatus(TYPE_ACC);
		if(acc_sta && (s_op_static == FALSE)) {                        /* ACC off并且没有停发报文 */	
       
        if(s_op_delay++ >= 50) {
            s_op_delay = 0;					
            s_op_static = TRUE;
            HAL_CAN_SendIdAccessSet(CAN_CHN_1, CAN_SEND_DISABLE, UDS_PHSCL_RESPID, 0);
            HAL_CAN_SendIdAccessSet(CAN_CHN_2, CAN_SEND_DISABLE, UDS_PHSCL_RESPID, 0);        
        }
    } else {	
        if((!acc_sta) && s_op_static) {                            /* ACC On并且已经停发报文 */
			      s_op_delay  = 0;
					  s_op_static = FALSE;
					  HAL_CAN_SendIdAccessSet(CAN_CHN_1, CAN_SEND_ALL_PACKET, UDS_PHSCL_RESPID, 0);
            HAL_CAN_SendIdAccessSet(CAN_CHN_2, CAN_SEND_ALL_PACKET, UDS_PHSCL_RESPID, 0); 
        }
    }
}
#endif
/*******************************************************************************
**  函数名称:  PacketTimeOut
**  功能描述:  分包传输超时
**  输入参数:  无
**  输出参数:  无
**  返回参数:  无
*******************************************************************************/
static void PacketTimeOut(void* pdata)
{
    INT8U i = 0;
    #if DEBUG_CAN > 1
    static INT8U test_cnt = 0;
    if (++test_cnt >= 100) {
        test_cnt = 0;
        Test_CAN_Send();
    }
    #endif
    YX_PeriodDataTran();
    CanResendHdl();
    for (i = 0 ; i < MAXPACKETPARANUM; i++) {
        if (s_packetpara[i].packet_com == TRUE) {
            if (++s_packetpara[i].packet_tmrcnt > PACKETOVERTMR) {
           #if DEBUG_UDS > 0
               debug_printf("接收超时，清除第%d组多帧\r\n", i);
           #endif
               s_packetpara[i].packet_com = FALSE;
               if (s_packetpara[i].packet_buf != NULL) {
                  PORT_Free(s_packetpara[i].packet_buf);
                  s_packetpara[i].packet_buf = NULL;
               }
            }
        }

        if (s_sendpacket[i].packet_com == TRUE) {               /* 多包发送超时处理 */
            if (++s_sendpacket[i].packet_tmrcnt > PACKETOVERTMR) {
               s_sendpacket[i].packet_com = FALSE;
               s_sendpacket[i].sendcontinue = FALSE;
               if (s_sendpacket[i].packet_buf != NULL ) {
                  PORT_Free(s_sendpacket[i].packet_buf);
                  s_sendpacket[i].packet_buf = NULL;
               }
            }
        }
    }

    SendCF();

    Can_LossCanMsg(); 
		#if ACC_OFF_STOP_SEND  > 0
    SendCanMsg_OnOff();
		#endif
}

/**************************************************************************************************
**  函数名称:  BusTypeSetReqHdl
**  功能描述:  总线设置请求处理函数
**  输入参数:
**  返回参数:  None
**************************************************************************************************/
void BusTypeSetReqHdl(INT8U mancode,  INT8U command, INT8U *data, INT16U datalen)
{
    INT8U ack[3];
    #if DEBUG_CAN > 0
    debug_printf_dir("BusTypeSetReqHdl()");
    printf_hex_dir(data, datalen);
    debug_printf_dir("\r\n");
    #endif


    ack[0] = data[0];
    ack[1] = data[1];

    if (0x01 == data[0]) {

    } else {
        ack[2] = 0x02;                                                         /* 命令参数非法 */
        YX_COM_DirSend( BUS_TYPE_SET_ACK, ack, 3);
        return;
    }
    ack[2] = 0x01;                                                             /* 命令参数合法 */
    YX_COM_DirSend( BUS_TYPE_SET_ACK, ack, 3);

}

/**************************************************************************************************
**  函数名称:  BusOnOffReqHdl
**  功能描述:  总线开关请求处理函数
**  输入参数:
**  返回参数:  None
**************************************************************************************************/
void BusOnOffReqHdl(INT8U mancode,INT8U command, INT8U *data, INT16U datalen)
{
    INT8U ack[3];
    INT8U channel;
    #if DEBUG_CAN > 0
    debug_printf_dir("BusOnOffReqHdl()");
    printf_hex_dir(data, datalen);
    debug_printf_dir("\r\n");
    #endif


    ack[0] = data[0];
    ack[1] = data[1];
    channel = data[0] - 1;


    if (channel >= MAX_CAN_CHN) {
        ack[2] = 0x02;
        YX_COM_DirSend( BUS_ONOFF_CTRL_ACK, ack, 3);
        return;
    }


    if (0x01 == data[1]) {                                                 /* 开启总线通信 */
         PORT_CanEnable((CAN_CHN_E)channel, true);
         s_can_para[channel].onoff = true;
       //}else{
           if( s_can_para[channel].onoff == true){/*如果已经打开则认为成功*/

           }else{
              #if DEBUG_CAN > 0
                  debug_printf_dir("PORT_OpenCan(%d)fail,ONOFF%d\r\n",channel, s_can_para[channel].onoff);
              #endif
              ack[2] = 0x02;
              YX_COM_DirSend( BUS_ONOFF_CTRL_ACK, ack, 3);
              return;
           }
       //}
    } else if (0x02 == data[1]) {                                          /* 关闭总线通信 */
       PORT_CanEnable((CAN_CHN_E)channel,false);
       s_can_para[channel].onoff = false;
    } else {                                                                   /* 命令非法，失败 */
        ack[2] = 0x02;
        YX_COM_DirSend( BUS_ONOFF_CTRL_ACK, ack, 3);
        return;
    }

    ack[2] = 0x01;
    YX_COM_DirSend( BUS_ONOFF_CTRL_ACK, ack, 3);
}

/*******************************************************************************
 ** 函数名:     HdlMsg_PARA_SET_CAN
 ** 函数描述:   CAN通信参数设置请求处理函数，配置波特率、帧格式、帧类型
 ** 参数:       [in]cmd:命令编码
 **             [in]data:数据指针
 **             [in]datalen:数据长度
 ** 返回:       无
 ******************************************************************************/
void CANCommParaSetReqHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen)
{
	INT8U ack[2];
	INT8U channel;

	#if DEBUG_CAN > 0
	debug_printf_dir("HdlMsg_PARA_SET_CAN(), CAN通信参数设置:");
	printf_hex_dir(data, datalen);
	debug_printf_dir("\r\n");
	#endif
        if (!GetFiltenableStat()) return;
		ack[0] = data[0];
		channel = data[0]-1;

		if (channel >= MAX_CAN_CHN) {
			ack[1] = 0x03; /* 命令参数非法 */
			YX_COM_DirSend( PARA_SET_CAN_ACK, ack, 2);
			return;
		}

		//休眠唤醒，失去心跳的时候，SIMCOM会再下发配置CAN参数，如果不关闭，再配置会导致复位。
		//PORT_CloseCan(channel);//先关掉，设置完参数再开启，否则会设置失败

		if ((data[1] < 0x01) || (data[1] > 0x08)) {
			ack[1] = 0x03;
			YX_COM_DirSend( PARA_SET_CAN_ACK, ack, 2);
			return;
		}

		if ((data[4] < 0x01) || (data[4] > 0x03)) {
			ack[1] = 0x05;
			YX_COM_DirSend( PARA_SET_CAN_ACK, ack, 2);
			return;
		}

		switch (data[1]) {
		case 0x01:
			s_can_para[channel].baud = CAN_BAUD_10K;
			break;
		case 0x02:
			s_can_para[channel].baud = CAN_BAUD_20K;
			break;
		case 0x03:
			s_can_para[channel].baud = CAN_BAUD_50K;
			break;
		case 0x04:
			s_can_para[channel].baud = CAN_BAUD_100K;
			break;
		case 0x05:
			s_can_para[channel].baud = CAN_BAUD_125K;
			break;
		case 0x06:
			s_can_para[channel].baud = CAN_BAUD_250K;
			break;
		case 0x07:
			s_can_para[channel].baud = CAN_BAUD_500K;
			break;
		case 0x08:
			s_can_para[channel].baud = CAN_BAUD_1000K;
			break;
		default:
			ack[1] = 0x03;
			YX_COM_DirSend( PARA_SET_CAN_ACK, ack, 2);
			return;
		}
		// 保存波特率
		CanBaudSet(s_can_para[channel].baud, channel);
		
		switch (data[2]) {
		case 0x01:
		    s_can_para[channel].frameformat = FMAT_STD;
			break;
		case 0x02:
		    s_can_para[channel].frameformat = FMAT_EXT;
			break;
		default:
			ack[1] = 0x05;
			YX_COM_DirSend( PARA_SET_CAN_ACK, ack, 2);
			return;
		}

		switch (data[4]) {
		case 0x01:
		    s_can_para[channel].mode = CAN_MODE_LUCIDLY;
			break;
		case 0x02:
		    s_can_para[channel].mode = CAN_MODE_REPORT;
			break;
		case 0x03:
		    s_can_para[channel].mode = CAN_MODE_QUERY;
			break;
		default:
			ack[1] = 0x05;
			YX_COM_DirSend( PARA_SET_CAN_ACK, ack, 2);
			return;
		}

        ack[1] = 0x01;
        YX_COM_DirSend( PARA_SET_CAN_ACK, ack, 2);
        #if 0
        if (PORT_OpenCan(channel, s_can_para[channel].baud , s_can_para[channel].frameformat)) {
            ack[1] = 0x01;
            YX_COM_DirSend( PARA_SET_CAN_ACK, ack, 2);
            s_can_para[channel].onoff = true;
        } else {
            #if DEBUG_CAN > 0
                debug_printf_dir("para set fail\r\n");
            #endif
            ack[1] = 0x05;
            YX_COM_DirSend( PARA_SET_CAN_ACK, ack, 2);
        }
        #endif
}

/**************************************************************************************************
**  函数名称:  CANWorkModeSetHdl
**  功能描述:  CAN工作模式设置处理函数
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
void CANWorkModeSetHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen)
{
    INT8U  ack[3];
    INT8U  channel;
    IDPARA_T canid;
    INT8U  i;


    ack[0] = data[0];
    ack[1] = data[1];
    channel = data[0] - 1;

    #if DEBUG_CAN > 0
    debug_printf_dir("总线模式设置:");
    printf_hex_dir(data,datalen);
    #endif

    if (channel >= MAX_CAN_CHN) {
        ack[2] = 0x04;                                               /* 命令参数非法 */
        YX_COM_DirSend( WORKMODE_SET_CAN_ACK, ack, 3);
        return;
    }

    PORT_CanEnable((CAN_CHN_E)channel,false);//先关掉，设置完参数再开启，否则会设置失败

    #if DEBUG_CAN > 0
        debug_printf_dir("CAN%d模式设置:%d\r\n",data[0], data[1]);
    #endif

    if (0x01 == data[1]) {                                       /* 完全透传模式 */
        if (s_can_para[channel].mode != CAN_MODE_LUCIDLY) {
            s_can_para[channel].mode = CAN_MODE_LUCIDLY;
        }
    } else if (0x02 == data[1]) {                                /* 主动上报模式 */
        if (s_can_para[channel].mode != CAN_MODE_REPORT) {
            s_can_para[channel].mode = CAN_MODE_REPORT;
        }
    } else if (0x03 == data[1]) {                                /* 被动查询模式 */
        if (s_can_para[channel].mode != CAN_MODE_QUERY) {
            s_can_para[channel].mode = CAN_MODE_QUERY;
        }
    } else {                                                         /* 出错 */
        ack[2] = 0x04;
        YX_COM_DirSend( WORKMODE_SET_CAN_ACK, ack, 3);
        return;
    }
    /* 初始化滤波ID参数配置，当工作模式改变时，所有已使用的滤波ID均要清空，并设置ID未设置 */
    canid.isused = FALSE;
    for (i = 0; i < MAX_CANIDS; i++) {                      /* 清除所有配置过的ID滤波对象 */
        SetIDPara(&canid, i, channel);
    }
    PORT_ClearCanFilter((CAN_CHN_E)channel);                  /* 清除消息对象 */

  //  YX_COM_DirSend( WORKMODE_SET_CAN_ACK, ack, 3);
    //ack[2] = 0x01;
   // YX_COM_DirSend( WORKMODE_SET_CAN_ACK, ack, 3);
    #if 1
    if (PORT_OpenCan((CAN_CHN_E)channel, s_can_para[channel].baud , s_can_para[channel].frameformat)) {
        ack[2] = 0x01;
        YX_COM_DirSend( WORKMODE_SET_CAN_ACK, ack, 3);
        s_can_para[channel].onoff = true;
    } else {
        #if DEBUG_CAN > 0
            debug_printf_dir("para set fail\r\n");
        #endif
        ack[2] = 0x05;
        YX_COM_DirSend( WORKMODE_SET_CAN_ACK, ack, 3);
    }
   #endif
}

/**************************************************************************************************
**  函数名称:  CANFilterIDSetReqHdl
**  功能描述:  CAN滤波ID配置请求处理函数
**  输入参数:  version
            :  command
            :  userdata
            :  userdatalen
**  返回参数:  None
**************************************************************************************************/
void CANFilterIDSetReqHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen)
{
    INT8U  idcnts;
    INT8U  idused;
    INT8U  i, tempi;
    INT8U  ack[3];
    INT32U tempid;
    INT8U  channel;
    IDPARA_T canid;


    #if DEBUG_CAN > 0
        debug_printf_dir("总线滤波设置:");
       // printf_hex_dir(data,datalen);
    #endif
   if (!GetFiltenableStat()) return;// if (!s_idfiltenable) return;

    ack[0] = data[0];
    ack[1] = data[1];
    channel = data[0] - 1;

    if (channel >= MAX_CAN_CHN) {
        ack[2] = 0x05;                                               /* 命令参数非法 */
        YX_COM_DirSend( FILTER_SET_CAN_ACK, ack, 3);
        return;
    }


    switch (data[1]) {
        case ID_CLRALL:                                              /* 清空重填 */
            #if DEBUG_CAN > 0
                debug_printf_dir("D7_ID_CLRALL:%d\r\n",channel);
            #endif
            #if 1
            canid.isused = FALSE;

            for (i = 0; i < MAX_CANIDS; i++) {                      /* 清除所有配置过的ID滤波对象 */
                SetIDPara(&canid, i, channel);
            }

            PORT_ClearCanFilter((CAN_CHN_E)channel);                           /* 清除消息对象 */

            if (SetID(data[2], &data[1], CAN_FILTER_ON, channel)) {
                ack[2] = 0x01;
            } else {
                ack[2] = 0x07;
            }

            #endif
            YX_COM_DirSend( FILTER_SET_CAN_ACK, ack, 3);
            break;
        case ID_ADD:                                                 /* 追加补充 */
             #if DEBUG_CAN > 0
                debug_printf_dir("D7 ID_ADD:\r\n");
            #endif
            for (i = 0; i < data[2]; i++) {
                tempi = 3 + i * 6;
                tempid = bal_chartolong(&data[tempi]);               /* 获取ID */
                for (idcnts = 0; idcnts < MAX_CANIDS; idcnts++) {
                    if ((tempid == GetID(idcnts, channel)) && (TRUE == GetIDIsUsed(idcnts, channel))) {
                        ack[2] = 0x05;
                        YX_COM_DirSend( FILTER_SET_CAN_ACK, ack, 3);
                        return;
                    }
                }
            }
            if (SetID(data[2], &data[1], CAN_FILTER_ON, channel)) {
                ack[2] = 0x01;
            } else {
                ack[2] = 0x07;
            }
            ack[2] = 0x01;
            YX_COM_DirSend( FILTER_SET_CAN_ACK, ack, 3);
            break;
        case ID_DEL:                                                 /* 删除指定 待验证*/
            #if DEBUG_CAN > 0
                debug_printf_dir("D7 ID_DEL:\r\n");
            #endif
            idused = GetIDCnts(channel);                             /* 获取现有ID个数 */
            if (idused < data[2]) {                              /* 已设定的还小于要删除的个数，出错 */
                ack[2] = 0x05;
                YX_COM_DirSend( FILTER_SET_CAN_ACK, ack, 3);
                return;
            }
            for (i = 0; i < data[2]; i++) {
                tempi = 3 + i * 6;
                tempid = bal_chartolong(&data[tempi]);               /* 获取ID */
                for (idcnts = 0; idcnts < MAX_CANIDS; idcnts++) {
                    if ((tempid == GetID(idcnts, channel)) && (TRUE == GetIDIsUsed(idcnts, channel))) {
                        canid.isused = FALSE;
                        SetIDPara(&canid, idcnts, channel);
                        break;
                    }
                }
                if (idcnts >= MAX_CANIDS) {                         /* 搜索到最后，还是没有相同的ID，出错 */
                    ack[2] = 0x05;
                    YX_COM_DirSend( FILTER_SET_CAN_ACK, ack, 3);
                    return;
                }
                //CAN_ClearFilterByID(tempid, channel);
            }
            if (idused == data[2]) {                             /* 完全删除 */
            }
            ack[2] = 0x01;
            YX_COM_DirSend( FILTER_SET_CAN_ACK, ack, 3);
            break;
        case ID_NOFILTER:
            #if DEBUG_CAN > 0
                debug_printf_dir("D7 ID_NOFILTER:\r\n");
            #endif
            SetID(0, 0, CAN_FILTER_OFF, channel);
            ack[2] = 0x01;
            YX_COM_DirSend( FILTER_SET_CAN_ACK, ack, 3);
            break;
        default :
            ack[2] = 0x05;                                                     /* 参数超过设定范围 */
            YX_COM_DirSend( FILTER_SET_CAN_ACK, ack, 3);
            break;
    }
}

/**************************************************************************************************
**  函数名称:  CANFilterIDQueryHdl
**  功能描述:  CAN滤波配置查询处理函数
**  输入参数:
**  返回参数:  None
**************************************************************************************************/
void CANFilterIDQueryHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen)
{
    INT8U ack[4 + 52];
    INT8U i, start;
    IDPARA_T *idptr;
    INT8U channel;



    ack[0] = data[0];
    channel = data[0] - 1;

    if (channel >= MAX_CAN_CHN) {
        ack[1] = 0x02;                                               /* 命令参数非法 */
        YX_COM_DirSend( FILTER_QUERY_CAN_ACK, ack, 2);
        return;
    }


    ack[1] = 0x01;                                                   /* 符合条件，可以查询 */
    ack[2] = MAX_CANIDS;
    ack[3] = GetIDCnts(channel);

    if (0 == ack[3]) {                                               /* 没有配置ID */
        YX_COM_DirSend( FILTER_QUERY_CAN_ACK, ack, 4);
    } else {
        start = 0x04;
        for (i = 0; i < MAX_CANIDS; i++) {                           /* 从头到尾搜索，目的是以防有些被删除的中间有空白 */
            idptr = GetIDPara(i, channel);
            if (TRUE == idptr->isused) {
                bal_longtochar(&ack[start], idptr->id);
                start = start + 4;
            }
        }
        if (start <= 0x04) {                                         /* 没有搜到ID 出错 */
            ack[1] = 0x03;
            YX_COM_DirSend( FILTER_QUERY_CAN_ACK, ack, 2);
            return;
        }
        YX_COM_DirSend( FILTER_QUERY_CAN_ACK, ack, start);
    }
}

/**************************************************************************************************
**  函数名称:  CANWorkModeQueryHdl
**  功能描述:  CAN工作模式查询处理函数
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
void CANWorkModeQueryHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen)
{
    INT8U  ack[3];
    INT8U  channel;


    ack[0] = data[0];
    channel = data[0] - 1;

    if (channel >= MAX_CAN_CHN) {
        ack[1] = 0x02;                                               /* 命令参数非法 */
        YX_COM_DirSend( WORKMODE_QUERY_CAN_ACK, ack, 2);
        return;
    }

    ack[1] = 0x01;
    ack[2] = (INT8U)s_can_para[channel].mode;
    YX_COM_DirSend( WORKMODE_QUERY_CAN_ACK, ack, 3);
}

/*******************************************************************************
 ** 函数名:     HdlMsg_FILTER_SCREEN_CAN
 ** 函数描述:   CAN屏蔽滤波器组配置请求处理函数
 ** 参数:       [in]cmd:命令编码
 **             [in]data:数据指针
 **             [in]datalen:数据长度
 ** 返回:       无
 ******************************************************************************/
void CANScreenIDSetReqHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen) {
	INT8U i;
	INT8U ack[3];
	INT8U channel;
	IDPARA_T canid;

	ack[0] = data[0];
	ack[1] = data[1];
	channel = data[0]-1;

    if (!GetFiltenableStat()) return;
     #if DEBUG_CAN > 0
         debug_printf_dir("CANScreenIDSetReqHdl:\r\n");
     #endif

	if (channel >= MAX_CAN_CHN) {
        #if DEBUG_CAN > 0
            debug_printf_dir("CANcommand false:\r\n");
        #endif
		ack[2] = 0x05;  //命令参数非法
		YX_COM_DirSend( FILTER_SCREEN_CAN_ACK, ack, 3);
		return;
	}

	switch (data[1]) {
	    case ID_CLRALL:  //清空重填
	        #if DEBUG_CAN > 0
                debug_printf_dir(" DE_ID_CLRALL:idnum = %d\r\n",data[2]);
            #endif
			canid.isused = false;
			for (i = 0; i < MAX_CANIDS; i++) {  //清除所有配置过的ID滤波对象
				SetIDPara(&canid, i, channel);
			}
			PORT_ClearCanFilter((CAN_CHN_E)channel);
			ack[2] = 0x01;
			if (SetScreenID(data[2], &data[1], CAN_FILTER_ON, channel)) {
				ack[2] = 0x01;
			} else {
				ack[2] = 0x07;
			}
			YX_COM_DirSend( FILTER_SCREEN_CAN_ACK, ack, 3);
			break;
	    case ID_ADD:  //追加补充
	    	 #if DEBUG_CAN > 0
                debug_printf_dir("DE_ID_ADD:\r\n");
            #endif
			if (SetScreenID(data[2], &data[1], CAN_FILTER_ON, channel)) {
				ack[2] = 0x01;
			} else {
				ack[2] = 0x07;
			}
			YX_COM_DirSend( FILTER_SCREEN_CAN_ACK, ack, 3);
			break;
	default:
         #if DEBUG_CAN > 0
             debug_printf_dir("DE_CAN SET DEFAUL:\r\n");
         #endif
		ack[2] = 0x05;  //参数超过设定范围
		YX_COM_DirSend( FILTER_SCREEN_CAN_ACK, ack, 3);
		break;
	}
}

/*******************************************************************************
 ** 函数名:     HdlMsg_DATA_TRANSF_CAN
 ** 函数描述:   CAN数据转发请求处理函数
 ** 参数:       [in]cmd:命令编码
 **             [in]data:数据指针
 **             [in]datalen:数据长度
 ** 返回:       无
 ******************************************************************************/
void CANDataTransReqHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen) {
	INT8U ack[6];
	INT8U temp[4];
	INT8U i,j;
	INT8U num;
	INT8U type,sendlen;
	INT16U period;
	INT32U id;
	STREAM_T strm;
	CAN_DATA_SEND_T senddata;

    mancode     = mancode;
    command     = command;

#if DEBUG_UDS > 0
    debug_printf_dir("<*****CANDataTransReqHdl*****>:\r\n");
    printf_hex_dir(data, datalen);
    debug_printf_dir("\r\n");
#endif
    senddata.channel = data[0]-1;
	ack[0] = data[0];
   	ack[1] = data[1];
	ack[2] = data[2];                              /* 流水号 */
 
	if (senddata.channel >= MAX_CAN_CHN)  {
		ack[3] = 0x05;  //命令参数非法
		YX_COM_DirSend( DATA_TRANSF_CAN_ACK, ack, 4);
		return;
	}

	ack[3] = 0x01;
	bal_InitStrm(&strm, data + 4, datalen - 4);
	num = bal_ReadBYTE_Strm(&strm);
    #if DEBUG_UDS > 0
    debug_printf_dir("<*****CANDataTransReqHdl:num:%d *****>\r\n",num);
    #endif
	for (i = 0; i < num; i++) {
		bal_ReadDATA_Strm(&strm, temp, 4);
		id = bal_chartolong(temp);
		type = bal_ReadBYTE_Strm(&strm);
		period = (INT16U) bal_ReadBYTE_Strm(&strm) << 8;
		period |= bal_ReadBYTE_Strm(&strm);
		sendlen = bal_ReadBYTE_Strm(&strm);

		if (sendlen > 8) {             //长度大于8，采用多帧发送
            #if DEBUG_CAN > 0
            debug_printf("长帧\r\n");
            #endif
            j = FindFreeItem_SendList();
            if (j == MAXPACKETPARANUM) {
                return;
            }
            s_sendpacket[j].packet_com = TRUE;
            s_sendpacket[j].channel = data[0]-1;
            s_sendpacket[j].sendid = id;
            s_sendpacket[j].packet_totallen = sendlen - 1;
            s_sendpacket[j].sendlen = 0;
            s_sendpacket[j].sendpacket = 0;
            s_sendpacket[j].packet_buf= PORT_Malloc(s_sendpacket[j].packet_totallen);
            if (s_sendpacket[j].packet_buf == NULL) {
                s_sendpacket[j].packet_com = FALSE;
                bal_MovStrmPtr(&strm, sendlen);
            } else {
                bal_MovStrmPtr(&strm, 1);                      /* 1 for PCItype 不需要用到 */
                bal_ReadDATA_Strm(&strm, s_sendpacket[j].packet_buf, sendlen-1);
            }
            if((UDS_ID_SEND == id) || (UDS_ID_SEND1 == id)|| (id == UDS_ID_SEND2)|| (id == UDS_ID_SEND3)
            || (UDS_ID_SEND4 == id)|| (id == UDS_ID_SEND5)|| (id == UDS_ID_SEND6)) {//UDS长帧
                #if DEBUG_CAN > 0
                debug_printf("UDS长帧长度,%d\r\n",s_sendpacket[j].packet_totallen);
                #endif
                s_sendpacket[j].prot_type = UDS_TYPE;
                if (s_sendpacket[j].packet_com) {
                    SendFF(&s_sendpacket[j]);
                }
            } else {
                #if DEBUG_CAN > 0
                    debug_printf("J1939长帧长度,%d\r\n",s_sendpacket[j].packet_totallen);
                #endif
                s_sendpacket[j].prot_type = J1939_TYPE;
                if ((s_sendpacket[j].packet_com) && (FALSE == Find_J1939Sending())) {//没找到正在发送的J1939长帧
                    SendJ1939FirPacket(&s_sendpacket[j]);
                }
            }
            YX_COM_DirSend( DATA_TRANSF_CAN_ACK, ack, 4);
            return;
        } else {
    		senddata.can_id = id;
            #if DEBUG_CAN > 0
                if (id == 0x18ea0021)
                debug_printf("玉柴锁车下发\r\n");
            #endif
    		senddata.period = period/PERTICK;
    		senddata.can_DLC = sendlen;
            if(id <= 0x7ff){
                senddata.can_IDE = 0;  /*标准帧*/
            } else {
                senddata.can_IDE = 1;
            }
    		bal_ReadDATA_Strm(&strm, senddata.Data, senddata.can_DLC);
			#if LOCK_COLLECTION > 0
			INT8U ret;
			INT8U canbuf[12] = {0};
			ret = YX_IsLOCKCMD(id);
			bal_longtochar(canbuf, id);
			memcpy(&canbuf[4], senddata.Data, senddata.can_DLC);
			if (ret == TRUE) {
				#if DEBUG_LOCK > 0
				debug_printf("id :%08x senddata.Data:", id);
				Debug_PrintHex(TRUE, senddata.Data, senddata.can_DLC);
				#endif
				YX_AddDataCollection(canbuf);
			}
			#endif
        }
		switch (type) {
		case CAN_SEND_NOMAL:
		    senddata.period = 0xffff;
			if (false == PORT_CanSend(&senddata)){  //总线忙碌，出错
				ack[3] = 0x02;
			}
            if ((id == 0x0CFE21EE) || (id == 0x18FFD034)) {
                StartCanResend(senddata.channel, id, senddata.Data, sendlen);
            }
			break;
		case CAN_SEND_PERIOD:
			if (false == PORT_CanSend(&senddata)) {
				ack[3] = 0x02;
			}
			break;
		case CAN_SEND_STOP:
		    senddata.period = 0;
			if (false == PORT_CanSend(&senddata)) {
				ack[3] = 0x02;
			}
			break;
        case CAN_SEND_THREE:
            if (s_period_can.send_en == FALSE) {
                s_period_can.send_cnt       = 0;
                s_period_can.send.can_DLC   = sendlen;
                s_period_can.send.can_id	= id;
                s_period_can.send.can_IDE	= senddata.can_IDE;
                s_period_can.send.channel	= senddata.channel;
                memcpy(s_period_can.send.Data, senddata.Data, sendlen);
                s_period_can.period 		= senddata.period;
                s_period_can.send.period 	= 0xffff;
                if (s_period_can.period){
                    s_period_can.send_en	= TRUE;
                }else {
                    ack[3] = 0x02;
                    s_period_can.send_en	= FALSE;
                }
            } else {
                ack[3] = 0x02;
            }
            break;
		default:
			ack[3] = 0x02;
			break;
		}
	}
	YX_COM_DirSend( DATA_TRANSF_CAN_ACK, ack, 4);
}

/*****************************************************************************
**  函数名:  YX_MMI_UDS_CanSendMul
**  函数描述: uds发送多帧数据
**  参数:    [in] data :
**           [in] len  :
**  返回:    FALSE:执行失败 TRUE:执行成功
*****************************************************************************/
BOOLEAN YX_MMI_UDS_CanSendMul(INT8U com,INT8U* data, INT16U len) 
{
    INT8U j;
		STREAM_T strm;
		
    #if DEBUG_CAN > 0
    debug_printf("长帧\r\n");
    #endif
		
		
		bal_InitStrm(&strm, data, len);
		
    j = FindFreeItem_SendList();
    if (j == MAXPACKETPARANUM) {    		
    		return false;
    }
    s_sendpacket[j].packet_com = TRUE;
    s_sendpacket[j].channel = com;
    s_sendpacket[j].sendid = UDS_PHSCL_RESPID;
    s_sendpacket[j].packet_totallen = len;
    s_sendpacket[j].sendlen = 0;
    s_sendpacket[j].sendpacket = 0;
    s_sendpacket[j].packet_buf = PORT_Malloc(s_sendpacket[j].packet_totallen);
    if (s_sendpacket[j].packet_buf == NULL) {
    		s_sendpacket[j].packet_com = FALSE;
				return false;
    } else {		                
        bal_ReadDATA_Strm(&strm, s_sendpacket[j].packet_buf, len);	
    		#if DEBUG_ERR > 0
    		debug_printf("len = %d",len);
    		printf_hex(s_sendpacket[j].packet_buf,len);
    		#endif
    }
   
  	 #if DEBUG_CAN > 0
  	 debug_printf("UDS长帧长度,%d\r\n",s_sendpacket[j].packet_totallen);
  	 #endif		 
  	 s_sendpacket[j].prot_type = UDS_TYPE;
  	 if (s_sendpacket[j].packet_com) {
  			 SendFF(&s_sendpacket[j]);
  	 }
		 return TRUE;
}
/**************************************************************************************************
**  函数名称:  GetCANDataReqHdl
**  功能描述:  车台获取CAN数据请求处理函数
**  输入参数:  version
            :  command
            :  userdata
            :  userdatalen
**  返回参数:  None
**************************************************************************************************/
void GetCANDataReqHdl(INT8U mancode, INT8U command, INT8U *data, INT16U datalen)
{
    INT32U  tempid;
    INT8U   i, j;
    INT8U   ack[3 + (4 + 1 + 4 + 1 + 8) * MAX_CANIDS / 2];
    INT8U   IDSum = 0;
    INT8U   IDCnt = 0;
    INT8U   channel;
    INT16U  addr, temlen;

    ack[0] = data[0];
    channel = data[0] - 1;

    if (channel >= MAX_CAN_CHN) {
        ack[1] = 0x03;                                               /* 命令参数非法 */
        YX_COM_DirSend( GET_CAN_DATA_REQ_ACK, ack, 2);
        return;
    }


    if ((CAN_MODE_QUERY != s_can_para[channel].mode)) {
        ack[1] = 0x02;                                               /* 工作模式错误，查询失败 */
        YX_COM_DirSend( GET_CAN_DATA_REQ_ACK, ack, 2);
        return;
    }

    IDSum = data[1];                                             //ID总数
    if (IDSum > (MAX_CANIDS / 2)) {
        IDSum = MAX_CANIDS / 2;
    }
    addr = 3;
    for (j = 0; j < IDSum; j++) {
        tempid = bal_chartolong(&data[2 + 4 * j]);
        for (i = 0; i < MAX_CANIDS; i++) {
            if ((tempid == GetID(i, channel)) && (GetIDIsUsed(i, channel))) {
                break;
            }
        }
        if (i >= MAX_CANIDS) {
            continue ;
        }
        temlen = GetStoreData(i, &ack[addr], channel);

        if (temlen == 0) {                                           /* 没缓冲数据 */
            memcpy(&ack[addr], &data[2 + 4 * j], 4);
            addr += 4;
            ack[addr] = 0;
            addr += 1;
        }
        addr += temlen;

        IDCnt++;
    }

    ack[1] = 0x01;                                                   /* 查询成功 */
    ack[2] = IDCnt;
    YX_COM_DirSend( GET_CAN_DATA_REQ_ACK, ack, addr);
}

/**************************************************************************************************
**  函数名称:  CANDataReportAckHdl
**  功能描述:  CAN数据主动上报后后收到应答
**  输入参数:  version
            :  command
            :  userdata
            :  userdatalen
**  返回参数:  None
**************************************************************************************************/
void CANDataReportAckHdl(INT8U mancode, INT8U command, INT8U *data, INT16U datalen)
{
    mancode     = mancode;
    command     = command;
    data        = data;
    datalen     = datalen;
}
/**************************************************************************************************
**  函数名称:  CANDataLucidlyAckHdl
**  功能描述:  CAN数据向上透传后收到应答
**  输入参数:  version
            :  command
            :  userdata
            :  userdatalen
**  返回参数:  None
**************************************************************************************************/
void CANDataLucidlyAckHdl(INT8U mancode, INT8U command, INT8U *data, INT16U datalen)
{
    mancode     = mancode;
    command     = command;
    data        = data;
    datalen     = datalen;
}

/*******************************************************************************
 ** 函数名:    YX_CAN_SelfCheckInit
 ** 函数描述:   CAN总线故障自检初始化
 ** 参数:       无
 ** 返回:       无
 ******************************************************************************/
#if DEBUG_CANSFLFSEND >= 0
static void YX_CAN_SelfCheckInit(void)
{
	//MCU向接收芯片TJA1043发送报文，如果CAN总线存在异常则ERR_N脚会变成有效状态
	INT8U dat[]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	CAN_DATA_SEND_T sdata;
	sdata.can_DLC=8;
	sdata.can_IDE=0;
	sdata.can_id=0x0;
	sdata.channel=0;
	sdata.period =200;//发送周期200*10ms
	memcpy(sdata.Data,dat,8);
	PORT_CanSend(&sdata);
	sdata.channel=1;
	PORT_CanSend(&sdata);
	sdata.channel=2;
	//PORT_CanSend(&sdata);
}
#endif
/*******************************************************************************
 ** 函数名:    YX_CAN_SoftStatus
 ** 函数描述:   获取CAN软件的工作状态
 ** 参数:       无
 ** 返回:       无
 ******************************************************************************/
INT8U YX_CAN_SoftStatus(void)
{
	INT8U cansoftstatus = 0;
	if(true == s_can_para[0].onoff) {
		cansoftstatus |= 0x01;
	} else {
		cansoftstatus &= ~0x01;
	}

	if(true == s_can_para[1].onoff) {
		cansoftstatus |= 0x02;
	} else {
		cansoftstatus &= ~0x02;
	}

	if(true == s_can_para[2].onoff) {
		cansoftstatus |= 0x04;
	} else {
		cansoftstatus &= ~0x04;
	}
	return cansoftstatus;

}
/*******************************************************************************
 ** 函数名:    YX_CAN_Init
 ** 函数描述:   CAN通讯驱动模块初始化
 ** 参数:       无
 ** 返回:       无
 ******************************************************************************/
void YX_CAN_Init(void)
{
    INT8U i;
	SCLOCKPARA_T t_sclockpapra;
    PORT_InitCan();
    for (i = 0; i < MAX_RESEND_NUM; i++) {
        YX_MEMSET((INT8U*)&s_resend_can[i], 0x00, sizeof(RESENT_CAN_T));
    }
	
	bal_pp_ReadParaByID(SCLOCKPARA_,(INT8U *)&t_sclockpapra, sizeof(SCLOCKPARA_T));
	
    s_oilsum_check = 0;
	s_can_para[0].baud = t_sclockpapra.canbaud[0];
	s_can_para[0].frameformat= FMAT_EXT;
	s_can_para[0].mode = CAN_MODE_REPORT;

	s_can_para[1].baud = t_sclockpapra.canbaud[1];
	s_can_para[1].frameformat= FMAT_EXT;
	s_can_para[1].mode = CAN_MODE_REPORT;

	s_can_para[2].baud = t_sclockpapra.canbaud[2];
	s_can_para[2].frameformat= FMAT_EXT;
	s_can_para[2].mode = CAN_MODE_REPORT;

    for (i = CAN_CHN_1; i < MAX_CAN_CHN-1; i++) {
    	if(PORT_OpenCan((CAN_CHN_E)i, s_can_para[i].baud, s_can_para[i].frameformat)){
    		s_can_para[i].onoff = true;
    	} else {
    		s_can_para[i].onoff = false;
    	}

        #if DEBUG_CAN > 0
    	PORT_SetCanFilter((CAN_CHN_E)i, 0, 0x7e8, 0xffffffff);
        #endif
        PORT_CanEnable((CAN_CHN_E)i, true);
    }
    Lock_Init();
	PORT_RegCanCallbakFunc(CANDataHdl);

	#if DEBUG_CANSFLFSEND > 0
	YX_CAN_SelfCheckInit();//CAN自检初始化
	#endif
    s_packet_tmr = OS_InstallTmr(TSK_ID_OPT, 0, PacketTimeOut);
    OS_StartTmr(s_packet_tmr, MILTICK, 1);
    
    #if EN_UDS > 0
		YX_UDS_Init();
		#endif
    #if DEBUG_CAN > 1
    // test
    CAN_DATA_SEND_T candata;
	memset(&candata, 0x00, sizeof(candata));
	candata.can_DLC = 8;
	candata.can_id = 0x401;
	candata.can_IDE = 0;
	candata.period = 0xffff;
	candata.channel = 1;
	PORT_CanSend(&candata);
    #endif
}
