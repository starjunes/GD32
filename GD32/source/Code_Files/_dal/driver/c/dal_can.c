/********************************************************************************
**
** 文件名:     dal_can.c
** 版权所有:   (c) 2014-2020 厦门雅迅网络股份有限公司
** 文件描述:   CAN总线通信驱动模块
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2015/12/29 | JUMP   | 创建本模块
**
*********************************************************************************/
#include  "dal_include.h"
#include  "dal_can.h"
#include  "man_irq.h"
#include  "tools.h"
#include  "roundbuf.h"
#include  "man_timer.h"
#include  "dmemory.h"
#include  "dal_pinlist.h"
#include  "dal_gpio.h"
#include  "scanner.h"
#include  "yx_debugcfg.h"
#include  "yx_can_man.h"

#define     CAN_SEND_PKG_LEN        13

#define     CAN_SEND_IDLENOW        0
#define     CAN_SEND_BUSY           1
#if EN_CAN > 0
#define     MAX_ACCESS_SEND_ID_NUM  3
#if SOFT_BUSOFF_RECOBRY > 0
#define BUSOFF_RECOVER_FAST_WAIT    50       /* 快恢复100ms    */
#define BUSOFF_RECOVER_SLOW_WAIT    200      /* 慢恢复1000ms   */
#define BUSOFF_RECOVER_FAST_COUNT   5         /* 快恢复次数 */ 
#define BUSOFF_CNT_CLEAR_WAIT       1000      /* 恢复后1秒内不再产生BUSOFF，清除计数，再次发生BUSOFF执行快速初始化 */
static BOOLEAN s_can_busoff[MAX_CAN_CHN];     /* busoff开关 */
static INT16U  s_can_delay[MAX_CAN_CHN];      /* busoff延时 */
static INT16U  s_can_offcnt[MAX_CAN_CHN];     /* busoff次数 */
#endif
typedef struct {
    CAN_SEND_PACKET_E access;                       /* 是否需要判断id */
    INT32U id[MAX_ACCESS_SEND_ID_NUM];    /* 允许发送的id表 */
} SEND_ID_ACCESS_LIST_T;

static SEND_ID_ACCESS_LIST_T s_send_id_access[MAX_CANCHAN];
#endif
/*************************************************************************************************/
/*                           环形缓冲区控制块                                                    */
/*************************************************************************************************/
typedef struct {
    INT8U          bk_bufsize;                                       /* 环形缓冲块数 */
    INT8U          bk_used;                                          /* 被使用的环形缓冲块 */
    INT8U          **bk_bptr;                                        /* 开始的环形缓冲块 */
    INT8U          **bk_eptr;                                        /* 结束的环形缓冲块 */
    INT8U          **bk_wptr;                                        /* 起始写入的环形缓冲块 */
    INT8U          **bk_rptr;                                        /* 开始读取的环形缓冲块 */
} ROUNDBLOCK_T;

/*************************************************************************************************/
/*                           模块结构体定义                                                      */
/*************************************************************************************************/
typedef struct {
    FILTER_T      idcbt[MAX_RXIDOBJ + 1];                            /* ID参数属性 */
    INT32U        screenid_last;                                     /* 作为预留的一路滤波屏蔽器id，用于超过13个滤波器的场合 */
} CAN_FILTER_T;
#if 0
typedef struct {
    INT8U         rxobjused;                                         /* 存储使用了多少个接收对象个数 */
    INT8U         recvcnt;                                           /* 接收的数据包计数流水号 */
    IDPARA_T      idcbt[MAX_CANIDS];                                 /* ID参数属性 */
} CAN_MSG_T;
#endif
typedef struct {
    INT8U         sdobjused;                                         /* 存储使用了多少个发送对象个数 */
    ID_SEND_T     idcbt[MAX_RXIDOBJ];                                /* ID参数属性 */
} CAN_SEND_T;

/*************************************************************************************************/
/*                           CAN回调APP层结构体                                                  */
/*************************************************************************************************/
typedef struct {
    CAN_LBINDEX_E index;                                             /* 回调索引号 */
    void volatile (*lbhandle) (CAN_DATA_HANDLE_T *CAN_msg);                 /* 回调函数指针 */
} CAN_LBAPP_E;

/*************************************************************************************************/
/*                           模块静态变量定义                                                    */
/*************************************************************************************************/
static CAN_ATTR_T     s_ccbt[MAX_CANCHAN];                           /* CAN控制块 */
static CAN_FILTER_T   s_can_filter[MAX_CANCHAN];                     /* CAN过滤器组 */
//static CAN_MSG_T      s_msgbt[MAX_CANCHAN];                          /* 接收的CAN消息块 */
static CAN_SEND_T     s_msg_period[MAX_CANCHAN];                     /* 周期发送的CAN消息块 */

static CAN_LBAPP_E    s_lbfunctionentry[LB_MAX];
static INT8U          s_can_buf[sizeof(CAN_DATA_HANDLE_T) * 40];
static INT8U          s_can_send_buf[MAX_CANCHAN][CAN_SEND_PKG_LEN * 60];
static ROUNDBUF_T     s_can_send_round[MAX_CANCHAN];
static ROUNDBUF_T     s_can_round;
static BOOLEAN        s_sendstat[MAX_CANCHAN] = {0};

static INT32U pried,sendcnttest1 = 0,sendcnttest2=0,recvcnt1,recvcnt2;
/*******************************************************************
** 函数名:     can_msg_handler
** 函数描述:   报文处理函数
** 参数:       [in] CAN_msg            报文信息
** 返回:       无
********************************************************************/
static void can_msg_handler(CAN_DATA_HANDLE_T CAN_msg)
{
	#if 0
    INT8U        tempbuf[220];
    INT8U        rxobj;
    INT8U        i, j;
    INT16U       len;
  #endif  
	INT32U       id;
	if (s_lbfunctionentry[LB_ANALYZE].lbhandle != NULL) {
        s_lbfunctionentry[LB_ANALYZE].lbhandle(&CAN_msg);
    }

    Chartolong(&id, CAN_msg.id);
    recvcnt1++;

    #if 0
    Chartolong(&id, CAN_msg.id);
    /* UDS和J1939多帧ID重复上报问题修复 */
    if (((id == 0x18DAEEFA) || (id == 0x18DA33FA) || (id == 0x18DAFA33) || (id == 0x18DAF100) || (id == 0x18DAF110)) && ((CAN_msg.databuf[0] & 0xF0) != 0x00)){ /* 单帧继续 */
         return;
    }
    if (((id & 0x00FF0000) == 0x00EC0000) || ((id & 0x00FF0000) == 0x00EB0000)) {  /* 已经作为特殊帧处理过，直接返回 */
        if (((id & 0x0000FF00) == 0x0000FF00) || ((id & 0x0000FF00) == 0x00008100) || ((id & 0x0000FF00) == 0x0000FD00)){
            return;
        }
    }

    if (s_ccbt[CAN_msg.channel].filteronoff == CAN_FILTER_OFF) {                /* 没滤波透传模式 */
        s_msgbt[CAN_msg.channel].recvcnt++;
        tempbuf[0] = CAN_msg.channel + 1;                                       /* CAN通道号,目前暂时固定为通道1 */
        MMI_MEMCPY(&tempbuf[1], sizeof(tempbuf) - 1, CAN_msg.id, 4);            /* ID */
        tempbuf[5] = CAN_msg.type;
        tempbuf[6] = 0;
        tempbuf[7] = 0;
        tempbuf[8] = s_msgbt[CAN_msg.channel].recvcnt;                          /* 流水号 */
        tempbuf[9] = CAN_msg.len;                                               /* 数据包长度 */
        MMI_MEMCPY(&tempbuf[10], sizeof(tempbuf) - 10, CAN_msg.databuf, CAN_msg.len);/* 拷贝数据包 */
        if (s_lbfunctionentry[LUCIDLY_REPORT].lbhandle != PNULL) {
            s_lbfunctionentry[LUCIDLY_REPORT].lbhandle(tempbuf, CAN_msg.len + 10);/* 1（CAN通道)+数据长度为 4(ID)+4(流水号)+1(存放数据长度)+数据长度 */
        }
        return;
    }

    for (i = 0; i < MAX_CANIDS; i++) {
        if ((id == s_msgbt[CAN_msg.channel].idcbt[i].id
            || ((id & s_msgbt[CAN_msg.channel].idcbt[i].screeid) == (s_msgbt[CAN_msg.channel].idcbt[i].id & s_msgbt[CAN_msg.channel].idcbt[i].screeid)) )
              && (true == s_msgbt[CAN_msg.channel].idcbt[i].isused)) {
            break;
        }
    }
    if (s_msgbt[CAN_msg.channel].idcbt[i].mode == CAN_COVER_OLD) {
        rxobj = i + s_msgbt[CAN_msg.channel].idcbt[i].storeadd;
        if (id != s_msgbt[CAN_msg.channel].idcbt[rxobj].id) {
            return;
        }
    } else {
        rxobj = i;
    }
    if (rxobj >= MAX_CANIDS) {
        return;
    }
    s_msgbt[CAN_msg.channel].idcbt[rxobj].len = CAN_msg.len;                    /* 存储单包长度 */
    MMI_MEMCPY(s_msgbt[CAN_msg.channel].idcbt[rxobj].storebuf, 8, CAN_msg.databuf, CAN_msg.len);
    s_msgbt[CAN_msg.channel].recvcnt++;                                         /* 流水号增加 */
    s_msgbt[CAN_msg.channel].idcbt[rxobj].type = CAN_msg.type;
    switch (s_ccbt[CAN_msg.channel].mode) {
        case CAN_MODE_LUCIDLY:                                                  /* 透传模式 */
            tempbuf[0] = CAN_msg.channel + 1;
            Longtochar(&tempbuf[1], id);                                    /* ID */
            tempbuf[5] = s_msgbt[CAN_msg.channel].idcbt[rxobj].type;
            tempbuf[6] = 0;
            tempbuf[7] = 0;
            tempbuf[8] = s_msgbt[CAN_msg.channel].recvcnt;                      /* 流水号 */
            tempbuf[9] = CAN_msg.len;                                           /* 数据包长度 */
            MMI_MEMCPY(&tempbuf[10], sizeof(tempbuf) - 10, CAN_msg.databuf, CAN_msg.len);/* 拷贝数据包 */
            if (s_lbfunctionentry[LUCIDLY_REPORT].lbhandle != PNULL) {
                s_lbfunctionentry[LUCIDLY_REPORT].lbhandle(tempbuf, CAN_msg.len + 10);
            }
            break;
        case CAN_MODE_REPORT:                                                   /* 主动上报 */
            if (s_msgbt[CAN_msg.channel].idcbt[i].mode == CAN_COVER_REGULAR) {
                s_msgbt[CAN_msg.channel].idcbt[rxobj].id_rec = id;
                s_msgbt[CAN_msg.channel].idcbt[rxobj].storecnt = 1;
                break;
            }
            s_msgbt[CAN_msg.channel].idcbt[i].storeadd++;
            if ((++s_msgbt[CAN_msg.channel].idcbt[i].storecnt >= s_msgbt[CAN_msg.channel].idcbt[i].stores)
              && (s_msgbt[CAN_msg.channel].idcbt[i].stores != 0xff)) {
                tempbuf[0] = CAN_msg.channel + 1;
                MMI_MEMCPY(&tempbuf[1], sizeof(tempbuf) - 1, CAN_msg.id, 4);
                if (s_msgbt[CAN_msg.channel].idcbt[i].mode == CAN_COVER_OLD) {
                    tempbuf[5] = s_msgbt[CAN_msg.channel].idcbt[i].stores;
                    len = 6;
                    for (j = 0; j < s_msgbt[CAN_msg.channel].idcbt[i].stores; j++) {
                        tempbuf[len++] = s_msgbt[CAN_msg.channel].idcbt[i + j].type;
                        tempbuf[len++] = 0;
                        tempbuf[len++] = s_msgbt[CAN_msg.channel].recvcnt;      /* 流水号 */
                        tempbuf[len++] = 0;
                        tempbuf[len++] = s_msgbt[CAN_msg.channel].idcbt[i + j].len;/* 单包长度 */
                        MMI_MEMCPY(&tempbuf[len], sizeof(tempbuf) - len,
                          s_msgbt[CAN_msg.channel].idcbt[i + j].storebuf, s_msgbt[CAN_msg.channel].idcbt[i + j].len);
                        len += s_msgbt[CAN_msg.channel].idcbt[i + j].len;
                    }
                } else {
                    tempbuf[5] = 1;
                    tempbuf[6] = s_msgbt[CAN_msg.channel].idcbt[rxobj].type;
                    tempbuf[7] = 0;
                    tempbuf[8] = s_msgbt[CAN_msg.channel].recvcnt;              /* 最后一包流水号 */
                    tempbuf[9] = 0;
                    tempbuf[10] = CAN_msg.len;                                  /* 最后一包的单包长度 */
                    MMI_MEMCPY(&tempbuf[11], sizeof(tempbuf) - 11, CAN_msg.databuf, CAN_msg.len);
                    len = CAN_msg.len + 11;
                }
                if (s_lbfunctionentry[LB_REPORT].lbhandle != PNULL) {
                    s_lbfunctionentry[LB_REPORT].lbhandle(tempbuf, len);
                }
                s_msgbt[CAN_msg.channel].idcbt[rxobj].storecnt = 0;
                s_msgbt[CAN_msg.channel].idcbt[rxobj].storeadd = 0;
            }
            break;
        case CAN_MODE_QUERY:                                                    /* 被动查询 */
            if (s_msgbt[CAN_msg.channel].idcbt[i].mode == CAN_COVER_OLD) {
                if (++s_msgbt[CAN_msg.channel].idcbt[i].storeadd >= s_msgbt[CAN_msg.channel].idcbt[i].stores) {
                    s_msgbt[CAN_msg.channel].idcbt[i].storeadd = 0;
                }
                if (++s_msgbt[CAN_msg.channel].idcbt[i].storecnt > s_msgbt[CAN_msg.channel].idcbt[i].stores) {
                    s_msgbt[CAN_msg.channel].idcbt[i].storecnt = s_msgbt[CAN_msg.channel].idcbt[i].stores;
                }
            } else {
                s_msgbt[CAN_msg.channel].idcbt[i].storecnt = 1;
            }
            break;
        default :
            break;
    }
    #endif
}



/*******************************************************************
** 函数名:     can_rx_scan
** 函数描述:   扫描CAN缓冲区是否有数据，并调用处理函数
** 参数:       无
** 返回:       无
********************************************************************/
static void can_rx_scan(void)
{
    INT8U i;
    CAN_DATA_HANDLE_T CAN_msg;
    INT8U buf[sizeof(CAN_DATA_HANDLE_T)];
    //Can2515RxHdl();
    for (; UsedOfRoundBuf(&s_can_round) >= sizeof(CAN_DATA_HANDLE_T); ) {
        for (i = 0; i < sizeof(CAN_DATA_HANDLE_T); i++) {
            buf[i] = ReadRoundBuf(&s_can_round);
        }
        MMI_MEMCPY((INT8U*)&CAN_msg, sizeof(CAN_msg), buf, sizeof(CAN_DATA_HANDLE_T));
        can_msg_handler(CAN_msg);
    }
}

/*******************************************************************
** 函数名:     SendCANMsg_Period
** 函数描述:   周期性发送CAN报文
** 参数:       无
** 返回:       无
********************************************************************/
void SendCANMsg_Period(void)
{
    INT8U i, j, k;
    INT8U temp[13];

    if (++pried >= 200) {
        pried = 0;
        #if DEBUG_CAN > 0
        Debug_SysPrint("<**** [can测试], (发送中断):%d, (发送接口):%d, (接收扫描入口):%d, (接收中断):%d ****>\r\n",sendcnttest1,sendcnttest2,recvcnt1,recvcnt2);
        #endif
    }
    for (i = 0; i < MAX_CANCHAN; i++) {
        k = 0;
		DAL_ASSERT((i == 0x00) || (i == 0x01));
        for (j = 0; j < MAX_RXIDOBJ; j++) {
            if (k >= s_msg_period[i].sdobjused) {
                break;
            }
            if (s_msg_period[i].idcbt[j].isused) {
                if (++s_msg_period[i].idcbt[j].timecnt >= s_msg_period[i].idcbt[j].period) {
                    s_msg_period[i].idcbt[j].timecnt = 0;
                    Longtochar(temp, s_msg_period[i].idcbt[j].id);
                    temp[4] = s_msg_period[i].idcbt[j].len;
                    MMI_MEMCPY(&temp[5], sizeof(temp) - 5, s_msg_period[i].idcbt[j].storebuf, s_msg_period[i].idcbt[j].len);
                    
                    DAL_CAN_TxData(temp, FALSE, i);
                }
                k++;
            }
        }
    }
}

/*******************************************************************
** 函数名:     USER_CAN0_TX0_IRQHandler
** 函数描述:   CAN0中断处理函数
** 参数:       无
** 返回:       无
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) static void USER_CAN0_TX0_IRQHandler(void)
{
    INT8U cdata[13],i;
    //sendcnttest1++;
	//debug_printf("can0发送中断\r\n");
	can_interrupt_flag_clear(CAN0, CAN_INT_FLAG_MTF0);
	can_interrupt_flag_clear(CAN0, CAN_INT_FLAG_MTF1);
	can_interrupt_flag_clear(CAN0, CAN_INT_FLAG_MTF2);
    if (UsedOfRoundBuf(&s_can_send_round[0]) >= CAN_SEND_PKG_LEN) {
        for (i = 0; i < CAN_SEND_PKG_LEN; i++) {
            cdata[i] = ReadRoundBuf(&s_can_send_round[0]);
        }
        DAL_CAN_TxData_Dir(cdata, 0);
    } else {
        s_sendstat[0] = CAN_SEND_IDLENOW;
    }
	Can_TxMsg(0);
}

/*******************************************************************
** 函数名:     USER_CAN1_TX0_IRQHandler
** 函数描述:   CAN1中断处理函数
** 参数:       无
** 返回:       无
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) static void USER_CAN1_TX0_IRQHandler(void)
{
    INT8U cdata[13],i;
	sendcnttest1++;
	//debug_printf("can1发送中断\\r\n");
	//debug_printf("USER_CAN1_TX0_IRQHandler addr:%x  handle_addr:%x\r\n",s_lbfunctionentry[LB_ANALYZE].lbhandle,handle_addr);
	can_interrupt_flag_clear(CAN1, CAN_INT_FLAG_MTF0);
	can_interrupt_flag_clear(CAN1, CAN_INT_FLAG_MTF1);
	can_interrupt_flag_clear(CAN1, CAN_INT_FLAG_MTF2);
    if (UsedOfRoundBuf(&s_can_send_round[1]) >= CAN_SEND_PKG_LEN) {
        for (i = 0; i < CAN_SEND_PKG_LEN; i++) {
            cdata[i] = ReadRoundBuf(&s_can_send_round[1]);
        }
        DAL_CAN_TxData_Dir(cdata, 1);
    } else {
        s_sendstat[1] = CAN_SEND_IDLENOW;
    }
	Can_TxMsg(1);
}

/*******************************************************************
** 函数名:     USER_CAN0_RX0_IRQHandler
** 函数描述:   CAN0中断处理函数
** 参数:       无
** 返回:       无
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) static void USER_CAN0_RX0_IRQHandler(void)
{
    can_receive_message_struct RxMessage;
    INT32U       id;
    CAN_DATA_HANDLE_T CAN_msg;
	//debug_printf("USER_CAN0_RX0_IRQHandler\r\n");
  #if DEBUG_CAN > 1
    Debug_SysPrint("1");
  #endif
    can_message_receive(CAN0, CAN_FIFO0, &RxMessage);                                   /* 首先接收CAN数据 */
	CAN_msg.channel = 0;
	
    if (RxMessage.rx_ff == CAN_FF_STANDARD) {                                                   /* 解析标准帧的ID值 */
        id = (INT32U)(RxMessage.rx_sfid);
        CAN_msg.format= 0x01;
    } else {                                                                    /* 解析扩展帧的ID值 */
        id = (INT32U)(RxMessage.rx_efid);
        CAN_msg.format= 0x02;
    }
    Longtochar(CAN_msg.id, id);
    CAN_msg.len = RxMessage.rx_dlen;
    if (CAN_msg.len == 0) {
        CAN_msg.type = 1; // 远程帧
    } else {
        CAN_msg.type = 0; // 数据帧
    }
		if(RxMessage.rx_dlen <= 8) { 
        MMI_MEMCPY(CAN_msg.databuf, 8, RxMessage.rx_data, RxMessage.rx_dlen);
        WriteBlockRoundBuf(&s_can_round, (INT8U*)&CAN_msg, sizeof(CAN_msg));
		}
}

/*******************************************************************
** 函数名:     USER_CAN1_RX1_IRQHandler
** 函数描述:   CAN1中断处理函数
** 参数:       无
** 返回:       无
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) static void USER_CAN1_RX1_IRQHandler(void)
{
    can_receive_message_struct     RxMessage;
    INT32U       id;
    CAN_DATA_HANDLE_T CAN_msg;
	recvcnt2++;
	//debug_printf("CAN1接收中断\r\n");
  #if DEBUG_CAN > 1
    Debug_SysPrint("CAN2接收中断\r\n");
  #endif
    can_message_receive(CAN1, CAN_FIFO1, &RxMessage);                                   /* 首先接收CAN数据 */
    CAN_msg.channel = 1;
    if (RxMessage.rx_ff == CAN_FF_STANDARD) {                                                   /* 解析标准帧的ID值 */
        id = (INT32U)(RxMessage.rx_sfid);
        CAN_msg.format = 0x01;
    } else {                                                                    /* 解析扩展帧的ID值 */
        id = (INT32U)(RxMessage.rx_efid);
        CAN_msg.format = 0x02;
    }
    Longtochar(CAN_msg.id, id);
    CAN_msg.len = RxMessage.rx_dlen;
    if (CAN_msg.len == 0) {
        CAN_msg.type = 1; // 远程帧
    } else {
        CAN_msg.type = 0; // 数据帧
    }
		if(RxMessage.rx_dlen <= 8) { 
        MMI_MEMCPY(CAN_msg.databuf, 8, RxMessage.rx_data, RxMessage.rx_dlen);
        WriteBlockRoundBuf(&s_can_round, (INT8U*)&CAN_msg, sizeof(CAN_msg));
		}

#if EN_LOCK > 0

    #if EN_CAN_EXCHG > 0
    if (s_lbfunctionentry[LB_HANDSHAKE].lbhandle != NULL) {
        s_lbfunctionentry[LB_HANDSHAKE].lbhandle((INT8U*)&CAN_msg, sizeof(CAN_DATA_HANDLE_T));
    }
    #endif

#endif
}

/*******************************************************************
** 函数名:     Dal_CAN_Init
** 函数描述:   CAN底层初始化
** 参数:       无
** 返回:       无
********************************************************************/
void Dal_CAN_Init(void)
{
    memset(s_msg_period, 0, sizeof(s_msg_period));
    InitRoundBuf(&s_can_round, s_can_buf, sizeof(s_can_buf), NULL);
	InitRoundBuf(&s_can_send_round[0], (INT8U *)&s_can_send_buf[0], sizeof(s_can_send_buf[0]), NULL);
    InitRoundBuf(&s_can_send_round[1], (INT8U *)&s_can_send_buf[1], sizeof(s_can_send_buf[1]), NULL);
    DAL_ASSERT(InstallScanner(can_rx_scan, 0));
}

/*******************************************************************
** 函数名:     Dal_CANLBRepReg
** 函数描述:   CAN回调上报函数
** 参数:       [in] handle             指向APP层的函数指针
** 返回:       无
********************************************************************/
void Dal_CANLBRepReg(CAN_LBINDEX_E index, void (* handle) (CAN_DATA_HANDLE_T *CAN_msg))
{
    s_lbfunctionentry[index].lbhandle = handle;
}
#if 0
/*******************************************************************
** 函数名:     CANRegularReport
** 函数描述:   CAN定时采集函数
** 参数:       无
** 返回:       无
********************************************************************/
void CANRegularReport(void)
{
    INT8U i;
    INT8U channel;
    INT8U tempbuf[100];
    INT8U len;

    for (channel = 0; channel < MAX_CANCHAN; channel++) {
        for (i = 0; i < MAX_CANIDS; i++) {
            if (s_msgbt[channel].idcbt[i].mode == CAN_COVER_REGULAR) {
                if ((++s_msgbt[channel].idcbt[i].storeadd >= s_msgbt[channel].idcbt[i].stores) && (s_msgbt[channel].idcbt[i].storecnt)) {/* 用storeadd来累加计时 */
                    tempbuf[0] = channel + 1;
                    Longtochar(&tempbuf[1], s_msgbt[channel].idcbt[i].id_rec);
                    tempbuf[5] = 1;
                    tempbuf[6] = s_msgbt[channel].idcbt[i].type;
                    tempbuf[7] = 0;
                    tempbuf[8] = s_msgbt[channel].recvcnt;                      /* 最后一包流水号 */
                    tempbuf[9] = 0;
                    tempbuf[10] = s_msgbt[channel].idcbt[i].len;                /* 最后一包的单包长度 */
                    MMI_MEMCPY(&tempbuf[11], sizeof(tempbuf) - 11, s_msgbt[channel].idcbt[i].storebuf, s_msgbt[channel].idcbt[i].len);
                    len = s_msgbt[channel].idcbt[i].len + 11;

                    if (s_lbfunctionentry[LB_REPORT].lbhandle != PNULL) {
                        s_lbfunctionentry[LB_REPORT].lbhandle(tempbuf, len);
                    }
                    s_msgbt[channel].idcbt[i].storecnt = 0;
                    s_msgbt[channel].idcbt[i].storeadd = 0;
                }
            }
        }
    }
}
/*******************************************************************
** 函数名:     GetStoreNum
** 函数描述:   获取指定ID目前缓存的条数
** 参数:       [in] serial             ID序号
**             [in] channel            通道号
** 返回:       已经收到并缓存的条数
********************************************************************/
INT8U GetStoreNum(INT8U serial, INT8U channel)
{
    return (s_msgbt[channel].idcbt[serial].storecnt);
}

/*******************************************************************
** 函数名:     GetStoreData
** 函数描述:   app层获取缓存的CAN数据，用于被动查询模式
** 参数:       [in] idcnts             ID序号
**             [in] channel            通道号
**             [out] buf               存储缓存数据的起始地址
** 返回:       数据总长度
********************************************************************/
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
    Longtochar(ptr, s_msgbt[channel].idcbt[idcnts].id);              /* ID */
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
        ptr[storeaddr++] = s_msgbt[channel].idcbt[idadd].type;
        ptr[storeaddr++] = 0;
        ptr[storeaddr++] = s_msgbt[channel].recvcnt;          /* 流水号 */
        ptr[storeaddr++] = 0;
        len = s_msgbt[channel].idcbt[idadd].len;                     /* 获取数据包长度 */
        ptr[storeaddr++] = len;
        MMI_MEMCPY(&ptr[storeaddr], sizeof(ptr) - storeaddr, (s_msgbt[channel].idcbt[idadd].storebuf), len);
        storeaddr += len;
        if (++idadd >= idcnts + s_msgbt[channel].idcbt[idcnts].stores) {
            idadd = idcnts;
        }
    }
    s_msgbt[channel].idcbt[idcnts].storecnt = 0;                     /* 已经收到的数据帧数清零 */
    s_msgbt[channel].idcbt[idcnts].storeadd = 0;
    return storeaddr;                                                /* 删除最后一次加上的长度 */
}

/*******************************************************************
** 函数名:     GetIDCnts
** 函数描述:   获取当前通道已经配置的过滤ID个数
** 参数:       [in] channel            CAN 通道
** 返回:       已配置的过滤ID个数
********************************************************************/
INT8U GetIDCnts(INT8U channel)
{
    return s_msgbt[channel].rxobjused;
}

/*******************************************************************
** 函数名:     GetIDIsUsed
** 函数描述:   获取ID是否被使用
** 参数:       [in] idcnts             序号
**             [in] channel            CAN 通道
** 返回:       true  or false
********************************************************************/
BOOLEAN GetIDIsUsed(INT8U idcnts, INT8U channel)
{
    return s_msgbt[channel].idcbt[idcnts].isused;
}

/*******************************************************************
** 函数名:     GetID
** 函数描述:   获取ID值
** 参数:       [in] idcnts             序号
**             [in] channel            CAN 通道
** 返回:       ID值
********************************************************************/
INT32U GetID(INT8U idcnts, INT8U channel)
{
    return s_msgbt[channel].idcbt[idcnts].id;
}

/*******************************************************************
** 函数名:     GetIDPara
** 函数描述:   获取滤波ID属性参数
** 参数:       [in] idcnts             序号
**             [in] channel            CAN 通道
** 返回:       指向ID属性结构体的地址
********************************************************************/
IDPARA_T* GetIDPara(INT8U idcnts, INT8U channel)
{
    return &s_msgbt[channel].idcbt[idcnts];
}

/*******************************************************************
** 函数名:     SetIDPara
** 函数描述:   设置滤波ID属性参数
** 参数:       [in] idcnts             序号
**             [in] channel            CAN 通道
** 返回:       无
********************************************************************/
void SetIDPara(IDPARA_T *idset, INT8U idcnts, INT8U channel)
{
    if (idset->isused) {
        s_msgbt[channel].recvcnt = 0;                                           /* 流水号清零 */
        s_msgbt[channel].idcbt[idcnts].isused = idset->isused;                  /* 使用与否 */
        s_msgbt[channel].idcbt[idcnts].storecnt = 0;                            /* 已经接收存储的帧数清零 */
        s_msgbt[channel].idcbt[idcnts].storeadd = 0;
        s_msgbt[channel].idcbt[idcnts].stores = idset->stores;
        s_msgbt[channel].idcbt[idcnts].id = idset->id;                          /* ID */
        s_msgbt[channel].idcbt[idcnts].screeid = idset->screeid & 0x00ffffff;   /* 前两个字节不关心 */
        s_msgbt[channel].idcbt[idcnts].mode = idset->mode;

        s_msgbt[channel].rxobjused++;
    } else {
        if (s_msgbt[channel].idcbt[idcnts].isused) {
            s_msgbt[channel].idcbt[idcnts].isused = false;
            s_msgbt[channel].recvcnt= 0;
            if (s_msgbt[channel].rxobjused) {
                s_msgbt[channel].rxobjused--;
            }
        }
    }
}
#endif
/*******************************************************************
** 函数名:     DAL_CAN_ClearBuf
** 函数描述:   内存错乱，清空发送缓存，重新开始
** 参数:       [[in] channel            CAN 通道
** 返回:       无
********************************************************************/
static void DAL_CAN_ClearBuf(INT8U channel)
{
	InitRoundBuf(&s_can_send_round[channel], (INT8U *)&s_can_send_buf[channel], sizeof(s_can_send_buf[channel]), NULL);
	s_sendstat[channel] = CAN_SEND_IDLENOW;
}
/*******************************************************************
** 函数名:     DAL_CAN_TxData
** 函数描述:   CAN发送数据接口函数
** 参数:       [in] data               指向数据区域指针(排列:ID len 数据)
**             [in] wait               是否等待发送成功应答
**             [in] channel            CAN 通道
** 返回:       发送结果
********************************************************************/
BOOLEAN DAL_CAN_TxData_Dir(INT8U *data, INT8U channel)
{
    INT8U        txmailbox;
    can_trasnmit_message_struct     cdata;
    INT32U       tempid;
    //INT32S       i;
    INT32U CANx;
    Chartolong(&tempid, data);                                              /* ID */
    memset(&cdata, 0, sizeof(cdata));

    #if DEBUG_CAN > 0
    //Debug_SysPrint("DAL_CAN_TxData_Dir, %d\r\n",UsedOfRoundBuf(&s_can_send_round[0]));
    #endif

    if (tempid > 0x000007FF) {            /* 
    根据帧类型指定ID值给不同对象 */
        cdata.tx_efid = tempid;
        cdata.tx_ff = _FRAME_EXT;
    } else {
        cdata.tx_sfid= tempid;
        cdata.tx_ff = _FRAME_STD;
    }

    if (_FRAME_DATA == s_ccbt[channel].fmat) {                                  /* 根据帧格式指定帧数据的长度及其内容 */
        cdata.tx_dlen = data[4];
				if (data[4] > 8){
   			    #if DEBUG_CAN > 0
   		      Debug_SysPrint("DAL_CAN_TxData_Dir,DLC:%d\r\n",data[4]);
   		      #endif
   			    DAL_CAN_ClearBuf(channel);
   			    return FALSE;
				}
        MMI_MEMCPY(cdata.tx_data, 8, &data[5], data[4]);
    } else {
        cdata.tx_dlen = 0;
        memset(cdata.tx_data, 0, sizeof(cdata.tx_data));
    }
    cdata.tx_ft = s_ccbt[channel].fmat;

    if (channel == 0) {
        CANx = CAN0;
    } else if (channel == 1) {
        CANx = CAN1;
    } else {
        return false;
    }
	

    txmailbox = can_message_transmit(CANx, &cdata);
    //txmailbox = CAN_Transmit(CANx, &cdata);
    
    if (txmailbox == CAN_NOMAILBOX) {
        #if EN_DEBUG > 0
        Debug_SysPrint("发送失败\r\n");
        #endif        
        return false;
    }

    
    return true;
}
#if EN_UDS > 0
/*******************************************************************
** 函数名称:   HAL_CAN_SendIdAccessSet
** 函数描述:   允许发送id设置
** 参数:       [in] com :  通道编号,见CAN_COM_E
               [in] set :  TRUE:打开发送id是否允许发送判断
               [in] id  :  发送id
               [in] idx :  允许发送id列表下表号:(0 ~ (MAX_ACCESS_SEND_ID_NUM-1))
** 返回:       剩余空间字节数
********************************************************************/
void HAL_CAN_SendIdAccessSet(INT8U com, CAN_SEND_PACKET_E set, INT32U id, INT8U idx)
{
    if (com >= MAX_CANCHAN) return;

    if (idx >= MAX_ACCESS_SEND_ID_NUM) return;

    s_send_id_access[com].access  = set;
    s_send_id_access[com].id[idx] = id;
}
/*******************************************************************
** 函数名称:   SendIdAccessCheck
** 函数描述:   允许发送id检查
** 参数:       [in] com :  通道编号,见CAN_COM_E
               [in] id  :  发送id
** 返回:       剩余空间字节数
********************************************************************/
static BOOLEAN SendIdAccessCheck(INT8U com, INT32U id)
{
    INT8U idx;
    if (com >= MAX_CANCHAN) return FALSE;

    if ((!s_send_id_access[com].access) || (id == 0x1ffffff0)) {
        return TRUE;
    } 

    if(s_send_id_access[com].access == CAN_SEND_DISABLE) {
		    return FALSE;
    }
    for (idx = 0; idx < MAX_ACCESS_SEND_ID_NUM; idx++) {
        if (s_send_id_access[com].id[idx] == id) {
            return TRUE;
        }        
    }

    return FALSE;
}
#endif

/*******************************************************************
** 函数名:     DAL_CAN_TxData
** 函数描述:   CAN发送数据接口函数
** 参数:       [in] data               指向数据区域指针(排列:ID len 数据)
**             [in] wait               是否等待发送成功应答
**             [in] channel            CAN 通道
** 返回:       发送结果
********************************************************************/
BOOLEAN DAL_CAN_TxData(INT8U *data, BOOLEAN wait, INT8U channel)
{
    INT8U cdata[13],i;
    BOOLEAN res;
    INT32U CANx,tempid;
    
    if (channel >= MAX_CANCHAN) return false;
    if (s_ccbt[channel].onoff == false) return false;
		
    #if EN_UDS > 0
		Chartolong(&tempid, data); 
    if (!SendIdAccessCheck(channel, tempid)) {
        return FALSE;
    }
    #endif
    memset(cdata, 0,sizeof(cdata));
    if (channel == 0) {
        CANx = CAN0;
        sendcnttest2++;
    } else {
        CANx = CAN1;
    }
    //CAN_ITConfig(CANx, CAN_IT_TME, DISABLE);
	can_interrupt_disable(CANx,CAN_INT_TME);
    res = WriteBlockRoundBuf(&s_can_send_round[channel], data, CAN_SEND_PKG_LEN);
    if (!res) {
        #if DEBUG_CAN > 0
        Debug_SysPrint("DAL_CAN_TxData %d, %d\r\n",channel,UsedOfRoundBuf(&s_can_send_round[channel]));
        #endif
        //s_sendstat[channel] = CAN_SEND_IDLENOW;
    }
    if (s_sendstat[channel] == CAN_SEND_IDLENOW) {
        can_interrupt_enable(CANx,CAN_INT_TME);
        for (i = 0; i < CAN_SEND_PKG_LEN; i++) {
            cdata[i] = ReadRoundBuf(&s_can_send_round[channel]);
        }
        s_sendstat[channel] = CAN_SEND_BUSY;
        DAL_CAN_TxData_Dir(cdata, channel);
    } else {
        can_interrupt_enable(CANx,CAN_INT_TME);
    }
    return res;
}

/*******************************************************************
** 函数名:     CAN_RxFilterConfig
** 函数描述:   CAN接收滤波
** 参数:       [in] filter_id          标示符
**             [in] screen_id          屏蔽位，相当于掩码，为0的位不关心，为1的位必须匹配
**             [in] onoff              是否开启滤波，CAN_FILTER_ON 配置idset的滤波ID，CAN_FILTER_OFF 不进行滤波接收所有ID
**             [in] channel            CAN 通道
** 返回:       设置结果
********************************************************************/
BOOLEAN CAN_RxFilterConfig(INT32U filter_id, INT32U screen_id, CAN_FILTERCTRL_E onoff, INT8U channel)
{
    can_filter_parameter_struct CAN_FilterInitStructure;
    INT8U i;
    #if DEBUG_CAN > 0
        //Debug_SysPrint("CAN_RxFilterConfig filter_id%x, screen_id%x, channel%x\r\n",filter_id,screen_id,channel);
    #endif

    if (channel > 1) return false;

    if (CAN_FILTER_ON == onoff) {                                               /* 开启滤波 */
        for (i = 0; i < MAX_RXIDOBJ; i++) {
            if (s_can_filter[channel].idcbt[i].isused == false) {
                break;
            }
        }
        if (i != MAX_RXIDOBJ) {
            s_can_filter[channel].idcbt[i].id = filter_id;                      /* ID */
        } else {
            if (s_can_filter[channel].idcbt[i].isused == false) {
                s_can_filter[channel].idcbt[i].id = filter_id;                  /* ID */
                s_can_filter[channel].screenid_last = screen_id;
            } else {
                s_can_filter[channel].screenid_last &= screen_id;
                s_can_filter[channel].screenid_last &= (~(s_can_filter[channel].idcbt[i].id ^ filter_id));
                screen_id = s_can_filter[channel].screenid_last;
            }
        }

        CAN_FilterInitStructure.filter_enable = ENABLE;
        if (filter_id <= 0x7ff) {
            CAN_FilterInitStructure.filter_list_high = (((INT32U)filter_id << 21) & 0xFFFF0000) >> 16;
            CAN_FilterInitStructure.filter_list_low  = (((INT32U)filter_id << 21) | CAN_FF_STANDARD| CAN_FT_DATA) & 0xFFFF;
            CAN_FilterInitStructure.filter_mask_high = (((INT32U)screen_id << 21) & 0xFFFF0000) >> 16;
            CAN_FilterInitStructure.filter_mask_low  = (((INT32U)screen_id << 21) | CAN_FF_STANDARD | CAN_FT_DATA) & 0xFFFF;
		} else {
            CAN_FilterInitStructure.filter_list_high = (((INT32U)filter_id << 3) & 0xFFFF0000) >> 16;
            CAN_FilterInitStructure.filter_list_low  = (((INT32U)filter_id << 3) | CAN_FF_EXTENDED| CAN_FT_DATA) & 0xFFFF;
            CAN_FilterInitStructure.filter_mask_high = (((INT32U)screen_id << 3) & 0xFFFF0000) >> 16;
            CAN_FilterInitStructure.filter_mask_low  = (((INT32U)screen_id << 3) | CAN_FF_EXTENDED | CAN_FT_DATA) & 0xFFFF;
		}
    } else if (CAN_FILTER_OFF == onoff) {                                       /* 不滤波 */
        CAN_FilterInitStructure.filter_enable = ENABLE;
        CAN_FilterInitStructure.filter_mask_high = 0x0;
        CAN_FilterInitStructure.filter_mask_low  = 0x0;
        i = MAX_RXIDOBJ;
    } else {
        return false;
    }

    s_ccbt[channel].filteronoff = onoff;
    CAN_FilterInitStructure.filter_number			= i + 14 * channel;
    CAN_FilterInitStructure.filter_mode				= CAN_FILTERMODE_MASK;
    CAN_FilterInitStructure.filter_bits				= CAN_FILTERBITS_32BIT;
    if (channel != 1) {
        CAN_FilterInitStructure.filter_fifo_number= CAN_FIFO0;
    } else {
    	
        CAN_FilterInitStructure.filter_fifo_number = CAN_FIFO1;
    }
    can_filter_init(&CAN_FilterInitStructure);
    s_can_filter[channel].idcbt[i].isused = true;
    #if DEBUG_CAN > 0
        //Debug_SysPrint("CAN_RxFilterConfig ok\r\n");
    #endif
    return true;
}

/*******************************************************************
** 函数名:     CAN_GPIO_Configuration
** 函数描述:   CAN管脚配置
** 参数:       [in] channel            CAN 通道
** 返回:       无
********************************************************************/
static void CAN_GPIO_Configuration(INT8U channel)
{
    

    if (channel == 0) {
        
		rcu_periph_clock_enable(RCU_AF);/*?开启GPIO系统时钟?*/
		rcu_periph_clock_enable(RCU_GPIOD);/*?开启GPIO系统时钟?*/
		rcu_periph_clock_enable(RCU_CAN0);
		
        
		gpio_init(CAN1_PIN_IO,GPIO_MODE_IPU,GPIO_OSPEED_50MHZ,CAN1_PIN_RX);/*?配置接收管脚?*/
		gpio_init(CAN1_PIN_IO,GPIO_MODE_AF_PP,GPIO_OSPEED_50MHZ,CAN1_PIN_TX);/*?配置发送管脚?*/
		gpio_pin_remap_config(GPIO_CAN0_FULL_REMAP,ENABLE);
    } else if (channel == 1) {
      #if MAX_CANCHAN >= 2
        
	  
	  rcu_periph_clock_enable(RCU_AF);/*开启GPIO系统时钟*/
	  rcu_periph_clock_enable(RCU_GPIOB);/*开启GPIO系统时钟*/
	  rcu_periph_clock_enable(RCU_CAN1);
	  
	  
	  gpio_init(CAN2_PIN_IO,GPIO_MODE_IPU,GPIO_OSPEED_50MHZ,CAN2_PIN_RX);/*?配置接收管脚?*/
	  gpio_init(CAN2_PIN_IO,GPIO_MODE_AF_PP,GPIO_OSPEED_50MHZ,CAN2_PIN_TX);/*?配置发送管脚?*/
      gpio_pin_remap_config(GPIO_CAN1_REMAP,ENABLE);  
      #endif
    }
}

/*******************************************************************
** 函数名:     CAN_WorkModeSet
** 函数描述:   CAN工作模式配置: 完全透传\主动上报\被动查询
** 参数:       [in] para               参数
**             [in] channel            CAN 通道
** 返回:       无
********************************************************************/
void CAN_WorkModeSet(CAN_ATTR_T *para, INT8U channel)
{
    //s_ccbt[channel].mode = para->mode; 
}

/*******************************************************************
** 函数名:     CAN_WorkModeInit
** 函数描述:   CAN工作模式初始化
** 参数:       [in] testmode           工作模式
**             [in] channel            CAN 通道
** 返回:       无
********************************************************************/
static void CAN_WorkModeInit(INT8U sjw, INT8U bs1, INT8U bs2, INT16U prescal,CAN_TEST_MODE_E testmode, INT8U channel)
{
    can_parameter_struct CAN_InitStructure;

	  can_struct_para_init(CAN_INIT_STRUCT,&CAN_InitStructure);
    #if SOFT_BUSOFF_RECOBRY > 0
	  CAN_InitStructure.auto_bus_off_recovery = DISABLE;
		#else
		CAN_InitStructure.auto_bus_off_recovery = ENABLE;
		#endif
		CAN_InitStructure.time_triggered		= DISABLE;
    CAN_InitStructure.auto_wake_up			= DISABLE;
    CAN_InitStructure.no_auto_retrans		= DISABLE;
    CAN_InitStructure.rec_fifo_overwrite	= DISABLE;
    CAN_InitStructure.trans_fifo_order		= ENABLE;                                   /* 发送优先级改为由发送顺序决定 */
    CAN_InitStructure.working_mode			= testmode;
    CAN_InitStructure.resync_jump_width		= sjw;
    CAN_InitStructure.time_segment_1		= bs1;
    CAN_InitStructure.time_segment_2		= bs2;
    CAN_InitStructure.prescaler				= prescal;
	
    if (channel == 0) {
        can_init(CAN0,&CAN_InitStructure);
    } else if (channel == 1) {
        can_init(CAN1,&CAN_InitStructure);
    }
}

/*******************************************************************
** 函数名:     CAN_CommParaSet
** 函数描述:   CAN通信参数配置：roundbuf\波特\数据帧格式\类型
** 参数:       [in] para               参数
**             [in] channel            CAN 通道
** 返回:       无
********************************************************************/
void CAN_CommParaSet(CAN_ATTR_T *para, INT8U channel)
{
    if ((channel == 1) && (MAX_CANCHAN < 2)) {
        return;
    }
    #if DEBUG_CAN > 0
        Debug_SysPrint("CAN_CommParaSet %d\r\n",channel);
    #endif
    CAN_GPIO_Configuration(channel);

    if (channel == 0) {
        NVIC_IrqHandleInstall(CAN0_RX0_IRQ, (ExecFuncPtr)USER_CAN0_RX0_IRQHandler, CAN_PRIOTITY, true);
        NVIC_IrqHandleInstall(CAN0_TX_IRQ, (ExecFuncPtr)USER_CAN0_TX0_IRQHandler, CAN_PRIOTITY, true);
    } else if (channel == 1) {
        NVIC_IrqHandleInstall(CAN1_RX1_IRQ, (ExecFuncPtr)USER_CAN1_RX1_IRQHandler, CAN_PRIOTITY, true);
        NVIC_IrqHandleInstall(CAN1_TX_IRQ, (ExecFuncPtr)USER_CAN1_TX0_IRQHandler, CAN_PRIOTITY, true);
    } else {
        return;
    }

    s_ccbt[channel].baud      = para->baud;
    s_ccbt[channel].type      = para->type;
    s_ccbt[channel].fmat      = para->fmat;
    s_ccbt[channel].test_mode = para->test_mode;

    //s_msgbt[channel].rxobjused = 0;
    switch (s_ccbt[channel].baud)
    {
        case CAN_10:
            CAN_WorkModeInit(CANSWJ, CAN_BS1_250K, CAN_BS2_250K, CAN_PRES_10K, s_ccbt[channel].test_mode, channel);
            break;

        case CAN_20:
            CAN_WorkModeInit(CANSWJ, CAN_BS1_250K, CAN_BS2_250K, CAN_PRES_20K, s_ccbt[channel].test_mode, channel);
            break;

        case CAN_50:
            CAN_WorkModeInit(CANSWJ, CAN_BS1_250K, CAN_BS2_250K, CAN_PRES_50K, s_ccbt[channel].test_mode, channel);
            break;

        case CAN_100:
            CAN_WorkModeInit(CANSWJ, CAN_BS1_250K, CAN_BS2_250K, CAN_PRES_100K, s_ccbt[channel].test_mode, channel);
            break;

        case CAN_125:
            CAN_WorkModeInit(CANSWJ, CAN_BS1_250K, CAN_BS2_250K, CAN_PRES_125K, s_ccbt[channel].test_mode, channel);
            break;

        case CAN_250:
            CAN_WorkModeInit(CANSWJ, CAN_BS1_250K, CAN_BS2_250K, CAN_PRES_250K, s_ccbt[channel].test_mode, channel);
            break;

        case CAN_500:
            CAN_WorkModeInit(CANSWJ, CAN_BS1_500K, CAN_BS2_500K, CAN_PRES_500K, s_ccbt[channel].test_mode, channel);
            break;

        case CAN_1000:
            CAN_WorkModeInit(CANSWJ, CAN_BS1_1000K, CAN_BS2_1000K, CAN_PRES_1000K, s_ccbt[channel].test_mode, channel);
            break;

        default:
            CAN_WorkModeInit(CANSWJ, CAN_BS1_250K, CAN_BS2_250K, CAN_PRES_250K, s_ccbt[channel].test_mode, channel);
            break;
    }
}

/*******************************************************************
** 函数名:     CAN_OnOFFCtrl
** 函数描述:   CAN通讯功能开与关模块
** 参数:       [in] onoff              开关
**             [in] channel            CAN 通道
** 返回:       无
********************************************************************/
void CAN_OnOFFCtrl (BOOLEAN onoff, INT8U channel)
{
    s_ccbt[channel].onoff = onoff;
    s_sendstat[channel] = CAN_SEND_IDLENOW;
    if (onoff) {
        if (channel == 0) {
            can_fifo_release(CAN0, CAN_FIFO0);
			can_interrupt_enable(CAN0,CAN_INT_RFNE0);
			can_interrupt_enable(CAN0,CAN_INT_TME);
        } else if (channel == 1) {
            can_fifo_release(CAN1, CAN_FIFO1);
			can_interrupt_enable(CAN1,CAN_INT_RFNE1);
			can_interrupt_enable(CAN1,CAN_INT_TME);
        }
    } else {
        if (channel == 0) {
            can_fifo_release(CAN0, CAN_FIFO0);
            can_interrupt_disable(CAN0,CAN_INT_RFNE0);
			can_interrupt_disable(CAN0,CAN_INT_TME);
        } else if (channel == 1) {
            can_fifo_release(CAN1, CAN_FIFO1);
			can_interrupt_disable(CAN1,CAN_INT_RFNE1);
			can_interrupt_disable(CAN1,CAN_INT_TME);
        }
    }
}

/*******************************************************************
** 函数名:     CAN_ClearFilterPara
** 函数描述:   清除指定的CAN滤波ID
** 参数:       [in] idnums             序号，0 ~ 13
**             [in] channel            CAN 通道
** 返回:       设置结果
********************************************************************/
BOOLEAN CAN_ClearFilterPara(INT8U idnums, INT8U channel)
{
    can_filter_parameter_struct CAN_FilterInitStructure;

    /* 清除掉过滤对象，还需要验证2012-2-2 by clt*/
    CAN_FilterInitStructure.filter_enable		= DISABLE;
    CAN_FilterInitStructure.filter_mask_high	= 0xFFFF;
    CAN_FilterInitStructure.filter_mask_low		= 0xFFFF;

    if (s_ccbt[channel].type == _FRAME_STD) {
        CAN_FilterInitStructure.filter_list_high = 0;
        CAN_FilterInitStructure.filter_list_low  = 0;
    } else if (s_ccbt[channel].type == _FRAME_EXT) {
        CAN_FilterInitStructure.filter_list_high = 0;
        CAN_FilterInitStructure.filter_list_low  = 0;
    } else {
        return false;
    }

    CAN_FilterInitStructure.filter_number = idnums + 14 * channel;
    CAN_FilterInitStructure.filter_mode= CAN_FILTERMODE_MASK;
    CAN_FilterInitStructure.filter_bits= CAN_FILTERBITS_32BIT;
    if (channel == 0) {
        CAN_FilterInitStructure.filter_fifo_number= CAN_FIFO0;
    } else {
        CAN_FilterInitStructure.filter_fifo_number = CAN_FIFO1;
    }

    can_filter_init(&CAN_FilterInitStructure);

    s_can_filter[channel].idcbt[idnums].isused = false;
    s_can_filter[channel].idcbt[idnums].id     = 0;

    if (idnums == MAX_RXIDOBJ) {
        s_can_filter[channel].screenid_last = 0xFFFFFFFF;
    }
    return true;
}

/*******************************************************************
** 函数名:     CAN_ClearFilterByID
** 函数描述:   清除指定的CAN滤波ID
** 参数:       [in] id                 所要清的ID
**             [in] channel            CAN 通道
** 返回:       无
********************************************************************/
BOOLEAN CAN_ClearFilterByID(INT32U id, INT8U channel)
{
    INT8U i;
    can_filter_parameter_struct CAN_FilterInitStructure;

    for (i = 0; i < MAX_RXIDOBJ; i++) {
        if ((s_can_filter[channel].idcbt[i].isused) && (s_can_filter[channel].idcbt[i].id == id)) {
            break;
        }
    }
    if (i >= MAX_RXIDOBJ) {
        return false;
    }

    /* 清除掉过滤对象，还需要验证2012-2-2 by clt*/
    CAN_FilterInitStructure.filter_enable	= DISABLE;
    CAN_FilterInitStructure.filter_mask_high= 0xFFFF;
    CAN_FilterInitStructure.filter_mask_low = 0xFFFF;

    if (s_ccbt[channel].type == _FRAME_STD) {
        CAN_FilterInitStructure.filter_list_high= 0;
        CAN_FilterInitStructure.filter_list_low = 0;
    } else if (s_ccbt[channel].type == _FRAME_EXT) {
        CAN_FilterInitStructure.filter_list_high = 0;
        CAN_FilterInitStructure.filter_list_low  = 0;
    } else {
        return false;
    }
	
	CAN_FilterInitStructure.filter_number= i + 14 * channel;
    CAN_FilterInitStructure.filter_mode  = CAN_FILTERMODE_MASK;
    CAN_FilterInitStructure.filter_bits	 = CAN_FILTERBITS_32BIT;
    if (channel == 0) {
        CAN_FilterInitStructure.filter_fifo_number= CAN_FIFO0;
    } else {
        CAN_FilterInitStructure.filter_fifo_number = CAN_FIFO1;
    }

    can_filter_init(&CAN_FilterInitStructure);

    s_can_filter[channel].idcbt[i].isused = false;
    s_can_filter[channel].idcbt[i].id     = 0;

    return true;
}

/*******************************************************************
** 函数名:     Dal_SetCANMsg_Period
** 函数描述:   设置周期性发送CAN报文
** 参数:       [in] id                 发送ID
**             [in] ptr                发送长度和数据
**             [in] period             发送周期，单位10ms
**             [in] channel            CAN 通道
** 返回:       设置结果
********************************************************************/
BOOLEAN Dal_SetCANMsg_Period(INT32U id, INT8U *ptr, INT16U period, INT8U channel)
{
    INT8U i;
	if(MAX_CANCHAN < channel)return FALSE;
  #if DEBUG_CAN > 0
    //Debug_SysPrint("id = %x, len = %d\r\n", id, ptr[0]);
  #endif
    for (i = 0; i < MAX_RXIDOBJ; i++) {
        if ((s_msg_period[channel].idcbt[i].isused) && (s_msg_period[channel].idcbt[i].id == id)) {
            s_msg_period[channel].idcbt[i].period = period;
            s_msg_period[channel].idcbt[i].len = ptr[0];
            MMI_MEMCPY(s_msg_period[channel].idcbt[i].storebuf, 8, &ptr[1], s_msg_period[channel].idcbt[i].len);
            return true;
        }
    }

    if (s_msg_period[channel].sdobjused >= MAX_RXIDOBJ) {
        return false;
    }
    for (i = 0; i < MAX_RXIDOBJ; i++) {
        if (!s_msg_period[channel].idcbt[i].isused) {
            s_msg_period[channel].idcbt[i].isused = true;
            s_msg_period[channel].idcbt[i].id = id;
            s_msg_period[channel].idcbt[i].period = period;
            s_msg_period[channel].idcbt[i].timecnt = i;                         /* 为了让多个发送周期岔开 */
            s_msg_period[channel].idcbt[i].len = ptr[0];
            MMI_MEMCPY(s_msg_period[channel].idcbt[i].storebuf, 8, &ptr[1], s_msg_period[channel].idcbt[i].len);
            s_msg_period[channel].sdobjused++;
            return true;
        }
    }
    return false;
}

/*******************************************************************
** 函数名:     Dal_StopCANMsg_Period
** 函数描述:   停止周期发送
** 参数:       [in] id                 发送ID
**             [in] channel            CAN 通道
** 返回:       设置结果
********************************************************************/
BOOLEAN Dal_StopCANMsg_Period(INT32U id, INT8U channel)
{
    INT8U i;

    for (i = 0; i < MAX_RXIDOBJ; i++) {
        if ((s_msg_period[channel].idcbt[i].isused) && (s_msg_period[channel].idcbt[i].id == id)) {
            s_msg_period[channel].idcbt[i].isused = false;
            s_msg_period[channel].sdobjused--;
            return true;
        }
    }
    return false;
}
/*******************************************************************
 ** 函数名:        can_Reset_PeriodSendPeriod
 ** 函数描述:   周期性重新计数
 ** 参数:        无
 ** 返回:        无
 ********************************************************************/
void dal_CAN_Reset_PeriodSendPeriod(void) 
{
    INT8U i, j, k;
   // CAN_MSG_T tx_msg;
    
    for (i = 0; i < MAX_CANCHAN; i++) {
      	k = 0;
      	for (j = 0; j < MAX_RXIDOBJ; j++) {
        		if (k >= s_msg_period[i].sdobjused) {
        			break;
        		}
        		if (s_msg_period[i].idcbt[j].isused) {
        		    s_msg_period[i].idcbt[j].timecnt = 0; 
								k++;
        		}						
      	}
    }
}
#if 0
/*******************************************************************
** 函数名:     ChickCanID
** 函数描述:   确认ID是否有效
** 参数:       [in] id                 传入的CAN ID
**             [in] channel            CAN 通道
** 返回:       为0时表示未找到
********************************************************************/
INT32U  ChickCanID(INT32U id,INT8U chanl)
{
    INT8U i;
    INT32U retid = 0;

    if (s_ccbt[chanl].filteronoff == CAN_FILTER_OFF) {
        return id;
    }
    for (i = 0; i < MAX_CANIDS; i++) {
        if ((true == s_msgbt[chanl].idcbt[i].isused) && ((id == s_msgbt[chanl].idcbt[i].id)
          || ((id & s_msgbt[chanl].idcbt[i].screeid) == (s_msgbt[chanl].idcbt[i].id & s_msgbt[chanl].idcbt[i].screeid))) ) {
            retid = id;
            break;
        }
    }
    return retid;
}
#endif
/**************************************************************************************************
**  函数名称:  CheckCanIsErrer
**  功能描述:  查看CAN总线是否出错
**  输入参数:
**  返回参数:
**************************************************************************************************/
BOOLEAN CheckCanIsErrer(INT8U channel)
{
    INT32U CANx;
    FlagStatus esr;

    DAL_ASSERT((channel == 0x00) || (channel == 0x01));
    if (channel == 0x00) {
        CANx = CAN0;
    } else {
        CANx = CAN1;
    }

    #if DEBUG_CAN > 0
        //Debug_SysPrint("CAN错误标识:%d\r\n", CANx->ESR & CAN_ESR_LEC);
    #endif
    esr = can_flag_get(CANx,CAN_FLAG_ERRIF);
    if (esr) {
        return TRUE;
    } else {
        return FALSE;
    }
}
#if SOFT_BUSOFF_RECOBRY > 0
/**************************************************************************************************
**  函数名称:  GetBusOffStatus
**  功能描述:  获取busoff状态
**  输入参数:
**  返回参数:  TRUE BUSOFF FALSE 
**************************************************************************************************/
BOOLEAN GetBusOffStatus(INT8U chn)
{
    if(chn >= MAX_CAN_CHN) return false;
    return s_can_busoff[chn];
}

/**************************************************************************************************
**  函数名称:  CheckCanIsBusOff
**  功能描述:  查看CAN总线是否进入busoff
**  输入参数:
**  返回参数:
**************************************************************************************************/
BOOLEAN CheckCanIsBusOff(INT8U channel)
{
    INT32U CANx;
    FlagStatus esr;

    DAL_ASSERT((channel == 0x00) || (channel == 0x01));
    if (channel == 0x00) {
        CANx = CAN0;
    } else {
        CANx = CAN1;
    }

    esr = can_flag_get(CANx,CAN_FLAG_BOERR);
    if (esr) {
        return TRUE;
    } else {
        return FALSE;
    }
}
/**************************************************************************************************
**  函数名称:  CanBussOffManual
**  功能描述:  手动退出busoff
**  输入参数:
**  返回参数:
**************************************************************************************************/
BOOLEAN CanBussOffManual(INT8U channel)
{
    INT32U CANx;

	  DAL_ASSERT((channel == 0x00) || (channel == 0x01));
	  if (channel == 0x00) {
		    CANx = CAN0;
	  } else {
		  	CANx = CAN1;
	  }

	  if (can_working_mode_set(CANx,CAN_MODE_INITIALIZE) != SUCCESS) {
		    return FALSE;
	  } 
		if (can_working_mode_set(CANx,CAN_MODE_NORMAL) != SUCCESS) {
		    return FALSE;
		}

		return TRUE;
}
/*******************************************************************************
** 函数名:    CanBusoffHal
** 函数描述:   CAN BusOff处理
** 参数:       无
** 返回:       无
******************************************************************************/
void CanBusOffHal(void)
{

    INT8U chn;
	  INT16U recover_wait = BUSOFF_RECOVER_FAST_WAIT;
    #if DEBUG_BUS_OFF > 0
	  static INT16U j = 0;
    #endif

	  for (chn= 0; chn < (MAX_CAN_CHN -1); chn++) {
		    if (s_ccbt[chn].onoff == FALSE) {
				    continue;
			  } 
			
		   #if DEBUG_BUS_OFF > 1 
			 if(j++ >= 999) {
					debug_printf("CheckCanIsBusOff%d = %d \r\n", chn, CheckCanIsBusOff(chn));
					j = 0;
			 }
		   #endif
			 if ((s_can_busoff[chn] == FALSE) && CheckCanIsBusOff(chn)) {
			     #if DEBUG_BUS_OFF > 0
					 debug_printf("\r\n CheckCanIsBusOff[%d] = %d 处于BUSOFF状态，准备恢复\r\n", chn, CheckCanIsBusOff(chn));
				   #endif
					 s_can_busoff[chn] = TRUE;
					 s_can_delay[chn] = 0;
					 s_can_offcnt[chn]++;					
			 }
			
			 if (s_can_busoff[chn]) {
			     if (s_can_offcnt[chn] <= 5) {
					     recover_wait = BUSOFF_RECOVER_FAST_WAIT;
						   #if DEBUG_BUS_OFF > 1
							 debug_printf("CAN%d BUSOFF快恢复\r\n", chn);
							 debug_printf("s_can_para[chn].onoff %d \r\n",s_can_para[chn].onoff);
						   #endif
					 } else {
							 s_can_offcnt[chn] = 6;
							 recover_wait = BUSOFF_RECOVER_SLOW_WAIT;
						   #if DEBUG_BUS_OFF > 1
							 debug_printf("CAN%d BUSOFF慢恢复\r\n", chn);
						   #endif
					 }
					 s_can_delay[chn]++;
					
					 if (s_can_delay[chn] >= recover_wait - 1) {
						   #if DEBUG_BUS_OFF > 0
							 debug_printf("s_can_delay[%d] = %d recover_wait = %d\r\n", chn,s_can_delay[chn],recover_wait);
						   #endif
							 //s_can_para[chn].onoff = FALSE;
							 //hal_clear_cancbrxbuf(chn);
							 CanBussOffManual(chn);
							 //s_can_para[chn].onoff = TRUE;

						   #if DEBUG_BUS_OFF > 1
							 debug_printf("CAN%d BUSOFF恢复\r\n", chn);
						   #endif
							 s_can_delay[chn] = 0;
							 s_can_busoff[chn] = FALSE;
					  }
        } else {
				    s_can_delay[chn]++;
				
					  if (s_can_delay[chn] >= BUSOFF_CNT_CLEAR_WAIT) {
						    #if DEBUG_BUS_OFF > 1
							  debug_printf("CAN%d 恢复后超过5s没有busoff\r\n", chn);
						    #endif
							  s_can_delay[chn] = 0;
							  s_can_offcnt[chn] = 0;
					  }
			  } 
    } 	
}
#endif

/*********************** (C) COPYRIGHT 2012 XIAMEN YAXON.LTD *******************END OF FILE******/

