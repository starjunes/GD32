/*
********************************************************************************
** 文件名:     yx_com_send.c
** 版权所有:   (c) 2017 厦门雅迅网络股份有限公司
** 文件描述:   协议数据处理接口（发送）
** 创建人：        谢金成，2017.5.18
********************************************************************************
*/
#include "yx_includes.h"
#include "yx_com_recv.h"
#include "yx_com_send.h"
#include "yx_com_man.h"

#define  MAXCELL          30
#define  SIZE_SENDBUF     1792//(650)
#define  WAITTIME         3
#define  RESENDTIMES      3
#define  PERIOD_RESEND    1
//#define  MAN_CODE         0x01                             /* 厂商编号 */
//#define  DEV_CODE         0x0f                             /* 120R外设编码0x02 */

/******************************************************************************/
/*                           定义发送数据帧控制模块结构体                                                                                                             */
/******************************************************************************/
typedef struct {
    INT8U   attrib;               /* 命令的属性:不需要应答(normal) or 需要应答(need ack)*/
    INT8U   rs_times;             /* 重发次数              */
    INT16U  rs_waittime;          /* 重发间隔的时间        */
    INT16U  framelen;             /* 数据帧长度(包括0x7e)  */
    INT8U   *frame;               /* 指向数据帧的指针      */
}SEND_FRAME_C_S;

static INT8U temp_send[SIZE_SENDBUF];

/*************************************************************************************************/
/*                           定义数据帧头结构体                                                  */
/*************************************************************************************************/
/* define structure of frame head */
 typedef struct {
    INT8U  chksum;                          /* 校验码    */
    INT8U  mansion;                         /* 协议版本  */
    INT8U  devcode;                         /* 外设编码  */
    INT8U  command;                         /* 命令      */
}SEND_FRAME_HEAD_S;

/******************************************************************************/
/*                           定义发送节点                                                                                                                                                */
/******************************************************************************/
 typedef struct {
    NODE_T node;                                       /* 节点头   */
    SEND_FRAME_C_S frame_c;                            /* 节点主体 */
 }SEND_FRAME_NODE_S;

/******************************************************************************/
/*                           定义相关变量                                                                                                                                                */
/******************************************************************************/
static  LIST_T    s_sendlist;                         /* 发送链表   */
static  LIST_T    s_resendlist;                       /* 重发链表   */
static  LIST_T    s_freelist;                         /* 空闲链表   */
static  SEND_FRAME_NODE_S  s_frame_mem[MAXCELL];
static  INT8U              s_resendtmrid;
static  ASMRULE_T    s_rules = {0x7e, 0x7d, 0x02, 0x01};

/*******************************************************************************
**  函数名称:  CheckSendChReady
**  功能描述:  检测发送通道是否可用
**  输入参数:  datalen : 待发送的数据长度
**  返回参数:  可用返回TRUE,不可用返回FALSE
*******************************************************************************/
static BOOLEAN CheckSendChReady(INT16U datalen)
{
    INT32S leftlen;

    if (datalen == 0 ) return TRUE;

    leftlen = PORT_uart_LeftOfWrbuf(MAIN_COM);

    if (leftlen == -1) return FALSE;
    if (datalen > leftlen) return FALSE;

    return TRUE;
}

/*******************************************************************************
**  函数名称:  DelSendCell
**  功能描述:  删除发送单元
**  输入参数:  cell : 指向要发送的单元
**  返回参数:  None
*******************************************************************************/
static void DelSendCell(SEND_FRAME_C_S *cell)
{
    if (cell == 0) return;
    PORT_Free(cell->frame);
    cell->framelen = 0;
    bal_AppendListEle(&s_freelist,(INT8U*)cell);

	#if DEBUG_COM_SEND > 0
       // debug_printf("申请后：%d \r\n",s_freelist.Item);
    #endif
}

/*******************************************************************************
**  函数名称:  ResendTmrProc
**  功能描述:  定时重新发送函数入口
**  输入参数:  None
**  返回参数:  None
*******************************************************************************/
static void SendTmrProc(void *pdata)
{
    SEND_FRAME_C_S  *cur_frame_c,*next_frame_c;


    cur_frame_c = (SEND_FRAME_C_S *)bal_GetListHead(&s_resendlist);
    while (cur_frame_c != NULL) {
        if ((cur_frame_c->rs_waittime -= PERIOD_RESEND) == 0) {
            cur_frame_c->rs_waittime = WAITTIME;
            next_frame_c = (SEND_FRAME_C_S *)bal_DelListEle(&s_resendlist,(INT8U*)cur_frame_c);
            if (cur_frame_c->rs_times-- == 0) {
                DelSendCell(cur_frame_c);
            } else {
                bal_AppendListEle(&s_sendlist, (INT8U*)cur_frame_c);
                OS_PostMsg(TSK_ID_OPT,MSG_OPT_COM_SEND, 0, 0);
            }
            cur_frame_c = next_frame_c;
        } else {
            cur_frame_c = (SEND_FRAME_C_S *)bal_ListNextEle((INT8U*)cur_frame_c);
        }
    }

    if (bal_ListItem(&s_sendlist) > 0) { // 就绪链表中存在待发送节点
        cur_frame_c = (SEND_FRAME_C_S *)bal_GetListHead(&s_sendlist);
        for (;;) {
            if (cur_frame_c == 0) break;
            PORT_UartWriteBlock(MAIN_COM, cur_frame_c->frame, cur_frame_c->framelen);

            next_frame_c = (SEND_FRAME_C_S *)bal_DelListEle(&s_sendlist, (INT8U*)cur_frame_c);
            if ((cur_frame_c->rs_times > 0) && (cur_frame_c->attrib == NEED_ACK)) {  //需要重传的数据帧
                cur_frame_c->rs_waittime = WAITTIME;
                bal_AppendListEle(&s_resendlist,(INT8U*)cur_frame_c);
            } else {  //无需继续重传的数据帧
                DelSendCell(cur_frame_c);
            }
            cur_frame_c = next_frame_c;
        }
    }
}

/*******************************************************************************
**  函数名称:  YX_ComSend_Init
**  功能描述:  初始化发送协议数据模块
**  输入参数:  None
**  返回参数:  None
*******************************************************************************/
void YX_ComSend_Init(void)
{
    YX_MEMSET(temp_send, 0x00, sizeof(SIZE_SENDBUF));
    bal_InitList(&s_sendlist);
    bal_InitList(&s_resendlist);
    bal_InitMemList(&s_freelist,(void*)s_frame_mem,sizeof(s_frame_mem)/sizeof(s_frame_mem[0]),sizeof(s_frame_mem[0]));
    s_resendtmrid = OS_InstallTmr(TSK_ID_OPT, 0, SendTmrProc);
    OS_StartTmr(s_resendtmrid,SECOND,PERIOD_RESEND);
}

/*******************************************************************************
**  函数名称:  YX_ComSend_ListInit
**  功能描述:  初始化发送协议数据相关的链表
**  输入参数:
**  返回参数:
*******************************************************************************/
void YX_ComSend_ListInit(void)
{
}
#if 0
/*******************************************************************************
**  函数名称:  YX_COM_DirSend
**  功能描述:  请求发送协议数据
**  输入参数:  attrib       : 属性，需要应答与否
**          :  command      : 命令
**          :  userdata     : 指向用户数据的指针
**          :  userdatalen  : 长度
**  返回参数:  None
*******************************************************************************/
void YX_COM_DirSend(INT8U command, INT8U *userdata, INT16U userdatalen)
{
    INT8U *ptr;

    if ((userdata == 0) && (userdatalen != 0)) return;
    if ((userdata != 0) && (userdatalen == 0)) return;

    ptr = PORT_Malloc(userdatalen + 7);
    if (ptr == NULL) {
        return;
    }
    ptr[0] = 0x55;
    ptr[1] = 0x7a;
    ptr[3] = DEVICETYPE;
    ptr[4] = command;
    ptr[5] = userdatalen >> 8;
    ptr[6] = userdatalen;
    memcpy(ptr + 7, userdata, userdatalen);
    ptr[2] = bal_ChkSum_Xor(ptr + 4, userdatalen + 3);
    PORT_UartWriteBlock(MAIN_COM, ptr, userdatalen + 7);
    PORT_Free(ptr);
}
#endif
/**************************************************************************************************
**  函数名称:  AssembleSendFrameHead
**  功能描述:  对数据帧头进行组帧
**  输入参数:  command : 命令
**          :  outdata : 指向存放组帧后的数据的存储单元
**          :  indata  : 要发送的用户数据
**          :  datalen : 数据长度
**  返回参数:  None
**************************************************************************************************/
static void AssembleSendFrameHead(INT8U command,INT8U *outdata, INT8U *indata, INT16U datalen)
{
    SEND_FRAME_HEAD_S  *send_head;

    send_head = (SEND_FRAME_HEAD_S *)outdata;
    send_head->mansion = MAN_CODE;
    send_head->devcode = DEV_CODE;
    send_head->command = command;
    if (datalen != 0) {
        memcpy(&outdata[sizeof(SEND_FRAME_HEAD_S)],indata, datalen);
    }
    send_head->chksum = bal_GetChkSum(&outdata[1], datalen + sizeof(SEND_FRAME_HEAD_S) - 1);
}

/*******************************************************************************
**  函数名称:  YX_COM_Send
**  功能描述:  请求发送协议数据
**  输入参数:  attrib       : 属性，需要应答与否
**          :  command      : 命令
**          :  userdata     : 指向用户数据的指针
**          :  userdatalen  : 长度
**  返回参数:  None
*******************************************************************************/
void YX_COM_Send(INT8U attrib, INT8U command, INT8U *userdata, INT16U userdatalen)
{

    INT8U   *framedata;
    INT16U  framedatalen;
    SEND_FRAME_C_S * frame_c;
    //INT8U temp[SIZE_SENDBUF];
    //INT8U *ptr;

    if ((userdata == 0) && (userdatalen != 0)) return;
    if ((userdata != 0) && (userdatalen == 0)) return;

    framedatalen = userdatalen + sizeof(SEND_FRAME_HEAD_S);
    framedata = PORT_Malloc(framedatalen);
    if (framedata == NULL) {
        return;
    }
    AssembleSendFrameHead(command, framedata, userdata, userdatalen);

    frame_c = (SEND_FRAME_C_S *)bal_DelListHead(&s_freelist);
	#if DEBUG_COM_SEND > 0
       // debug_printf("删除后：%d \r\n",s_freelist.Item);
	#endif
    if (frame_c == NULL) {
        PORT_Free(framedata);
        return;
    }

    frame_c->attrib      = attrib;
    frame_c->rs_waittime = WAITTIME;
    frame_c->rs_times    = RESENDTIMES;
    frame_c->framelen    = bal_AssembleByRules(temp_send, framedata, framedatalen, &s_rules);
    PORT_Free(framedata);
    frame_c->frame = PORT_Malloc(frame_c->framelen);
    if (frame_c->frame == NULL) {
        return;
    }
    memcpy(frame_c->frame, temp_send, frame_c->framelen);

    bal_AppendListEle(&s_sendlist, (INT8U*)frame_c);
	#if DEBUG_COM_SEND > 0
    debug_printf("send:");
    printf_hex(temp_send, frame_c->framelen);
    debug_printf("\r\n");
	#endif
    OS_APostMsg(TRUE, TSK_ID_OPT,MSG_OPT_COM_SEND, 0, 0);

}

/*******************************************************************************
**  函数名称:  YX_COM_DirSend
**  功能描述:  请求发送协议数据
**  输入参数:  command      : 命令
**          :  userdata     : 指向用户数据的指针
**          :  userdatalen  : 长度
**  返回参数:  None
*******************************************************************************/
void YX_COM_DirSend(INT8U command, INT8U *userdata, INT16U userdatalen)
{
    INT8U   *framedata;
    INT16U  framedatalen;
    //INT8U temp[SIZE_SENDBUF];
    INT16U len;

    if ((userdata == 0) && (userdatalen != 0)) return;
    if ((userdata != 0) && (userdatalen == 0)) return;

    framedatalen = userdatalen + sizeof(SEND_FRAME_HEAD_S);
    framedata = PORT_Malloc(framedatalen);
    if (framedata == NULL) {
        return;
    }
    AssembleSendFrameHead(command, framedata, userdata, userdatalen);

    len = bal_AssembleByRules(temp_send, framedata, framedatalen, &s_rules);
    PORT_Free(framedata);
    PORT_UartWriteBlock(MAIN_COM, temp_send, len);
	#if DEBUG_COM_SEND > 0
    debug_printf("send:");
    printf_hex(temp_send, len);
    debug_printf("\r\n");
	#endif

}

/*******************************************************************************
**  函数名称:  YX_ConfirmSend
**  功能描述:  确认协议数据已经发送
**  输入参数:  command  : 命令
**  返回参数:  None
*******************************************************************************/
void YX_ConfirmSend(INT8U command)
{
    SEND_FRAME_C_S    *frame_c;
    INT8U *ptr;

    frame_c = (SEND_FRAME_C_S*)bal_GetListHead(&s_resendlist);
    while (frame_c != NULL) {
        ptr = frame_c->frame;
        if (ptr[4] == command) {
            bal_DelListEle(&s_resendlist, (INT8U*)frame_c);
			#if DEBUG_COM_SEND > 0
                //debug_printf("r删除后：%d \r\n",s_resendlist.Item);
			#endif
		    DelSendCell(frame_c);
            return;
        }
        frame_c = (SEND_FRAME_C_S *)bal_ListNextEle((INT8U*)frame_c);
    }
}

/*******************************************************************************
**  函数名称:  Hdl_MSG_COM_SEND
**  功能描述:  协议数据发送入口
**  输入参数:  None
**  返回参数:  None
*******************************************************************************/
void Hdl_MSG_COM_SEND(void)
{
    SEND_FRAME_C_S  *frame_c;

	frame_c = (SEND_FRAME_C_S *)bal_GetListHead(&s_sendlist);
	if (frame_c != 0) {
		if (!CheckSendChReady(frame_c->framelen)) {
            OS_APostMsg(TRUE, TSK_ID_OPT,MSG_OPT_COM_SEND, 0, 0);
			return;
		}
		PORT_UartWriteBlock(MAIN_COM, frame_c->frame, frame_c->framelen);
		bal_DelListEle(&s_sendlist, (INT8U*)frame_c);

		#if DEBUG_COM_SEND > 0
            //debug_printf("send删除后：%d \r\n",s_sendlist.Item);
		#endif

		if (frame_c->attrib == NEED_ACK){
			bal_AppendListEle(&s_resendlist, (INT8U*)frame_c);

			#if DEBUG_COM_SEND > 0
                //debug_printf("r申请后：%d \r\n",s_resendlist.Item);
			#endif

		} else {
			DelSendCell(frame_c);
		}
	    OS_APostMsg(TRUE, TSK_ID_OPT,MSG_OPT_COM_SEND, 0, 0);
	}
}
/************************ (C) COPYRIGHT 2010 XIAMEN YAXON.LTD *****************/
/******END OF FILE******/

