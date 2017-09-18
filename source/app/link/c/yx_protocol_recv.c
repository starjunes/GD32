/********************************************************************************
**
** 文件名:     yx_protocol_recv.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现协议数据接收解析处理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2013/11/09 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "yx_include.h"
#include "at_drv.h"
#include "dal_pp_drv.h"
#include "yx_protocol_type.h"
#include "yx_protocol_send.h"
#include "yx_protocol_recv.h"
#include "yx_jt1_tlink.h"
#include "yx_jt_linkman.h"
#include "yx_sleep.h"



/*
********************************************************************************
* 定义配置参数
********************************************************************************
*/

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    INT16U index;
    void (*handler)(PROTOCOL_COM_T *attrib, PROTOCOL_HEAD_T *head, INT8U *data, INT16U datalen);
} FUNCENTRY_PROTOCOL_T;

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/


/*******************************************************************
** 函数名:     CheckFrameValid
** 函数描述:   判断协议帧是否正确
** 参数:       [in]  frameptr: 数据帧地址
               [in]  framelen: 数据帧长度
** 返回:       有效返回true，无效返回false
********************************************************************/
static BOOLEAN CheckFrameValid(PROTOCOL_HEAD_T *frameptr, INT16U framelen)
{
    INT8U  *ptr;
    INT16U msgid, msglen, msgattrib, exheadlen, totalpkg, curpkg;
    
    if (framelen < sizeof(PROTOCOL_HEAD_T)) {                                  /* 数据帧长度不够 */
        return false;
    }
    
    msgid = (frameptr->msgid[0] << 8) + frameptr->msgid[1];                    /* 消息ID不对 */
    if (msgid < 0x8000) {
        return FALSE;
    }
    
    msgattrib = (frameptr->msgattrib[0] << 8) + frameptr->msgattrib[1];
    msglen    = (msgattrib & 0x03FF);

    exheadlen = 0;
    if ((msgattrib & PROTOCOL_EXT_HEAD) != 0) {
        exheadlen = 4;
    }
    
    if (msglen != (framelen - SYSHEAD_LEN - SYSTAIL_LEN - exheadlen)) {        /* 长度格式不对 */
        return FALSE;
    }
    /*
    if (!YX_ChkMyTel_SYSFrame(frameptr)) {
        return FALSE;
    }*/
    if ((msgattrib & PROTOCOL_EXT_HEAD) != 0) {                                /* 帧序号不对 */
        ptr = frameptr->data;
        totalpkg = (ptr[0] << 8) + ptr[1];
        curpkg   = (ptr[2] << 8) + ptr[3];
        
        if (curpkg > totalpkg) {
            return FALSE;
        }
    }

    if (YX_ChkSum_XOR((INT8U *)frameptr, framelen - 1) == ((INT8U *)frameptr)[framelen - 1]) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/*******************************************************************
** 函数名:     HdlMsg_DN_ACK_REG
** 函数描述:   终端注册应答
** 参数:       无
** 返回:       无
********************************************************************/
static void HdlMsg_DN_ACK_REG(PROTOCOL_COM_T *attrib, PROTOCOL_HEAD_T *head, INT8U *data, INT16U datalen)
{
    //INT16U ackflowseq;
    INT8U result;

    //ackflowseq = (data[0] << 8) + data[1];                                     /* 应答流水号 */
    result = data[2];                                                          /* 注册结果 */

    if (attrib->channel == SOCKET_CH_0) {
        YX_JTT1_LinkInformReged(result, &data[3], datalen - 3);
    }
    
    if (attrib->channel == SOCKET_CH_1) {
        YX_JTT2_LinkInformReged(result, &data[3], datalen - 3);
    }
}

/*******************************************************************
** 函数名:     HdlMsg_DN_ACK_COMMON
** 函数描述:   处理中心下发的平台通用应答
** 参数:       无
** 返回:       无
********************************************************************/
static void HdlMsg_DN_ACK_COMMON(PROTOCOL_COM_T *attrib, PROTOCOL_HEAD_T *head, INT8U *data, INT16U datalen)
{
    INT16U  /*ackflowseq, */ackmsgid;    	
    INT8U   result;

    //ackflowseq = (data[0] << 8) + data[1];                                     /* 应答流水号 */
    ackmsgid   = (data[2] << 8) + data[3];                                     /* 应答消息ID */
    result     = data[4];                                                      /* 应答结果 */

    switch (ackmsgid)
    {
    case UP_CMD_AULOG:                                                         /* 登入应答 */
        if (attrib->channel == SOCKET_CH_0) {
            if (result == 0) {
                YX_JTT1_LinkInformLoged(true);
            } else {
                YX_JTT1_LinkInformLoged(false);
            }
        }
        
        if (attrib->channel == SOCKET_CH_1) {
            if (result == 0) {
                YX_JTT2_LinkInformLoged(true);
            } else {
                YX_JTT2_LinkInformLoged(false);
            }
        }
        break;
    case UP_CMD_HEART:
    default:                                                                   /* 接收到任何通用应答,都可以当作链路维护帧 */
        if (attrib->channel == SOCKET_CH_0) {
            if (result == 0x00 || result == 0x04) {
                YX_JTT1_LinkInformQuery((void *)1);
            } else {
                YX_JTT1_LinkInformQuery((void *)0);
            }
        }
        
        if (attrib->channel == SOCKET_CH_1) {
            if (result == 0x00 || result == 0x04) {
                YX_JTT2_LinkInformQuery((void *)1);
            } else {
                YX_JTT2_LinkInformQuery((void *)0);
            }
        }
        break;
    }
}

/*******************************************************************
** 函数名:      HdlMsg_DN_CMD_POSQRY
** 函数描述:    MSG_8201 位置信息查询
** 参数:        [in] attrib: 通道属性
**              [in] head:   协议头
** 返回:        NULL
********************************************************************/
static void HdlMsg_DN_CMD_POSQRY(PROTOCOL_COM_T *attrib, PROTOCOL_HEAD_T *head, INT8U *data, INT16U datalen)
{    
    STREAM_T *wstrm;
    INT32U msgattrib;
    
    msgattrib = 0;                                                             /* 消息属性 */
    wstrm = YX_PROTOCOL_GetBufferStrm();                                       /* 获取数据流缓存 */
    YX_PROTOCOL_AsmFrameHead(wstrm, UP_ACK_POSQRY, msgattrib, 0);              /* 组帧数据头 */
    YX_WriteDATA_Strm(wstrm, head->flowseq, 2);
    YX_JT_GetPositionInfo(wstrm);
    YX_PROTOCOL_SendData(wstrm, attrib, 0);
    
    YX_SLEEP_Wakeup(20, WAKEUP_EVENT_SERVER);                                  /* 默认唤醒20分钟 */
}

/*******************************************************************
** 函数名:      HdlMsg_DN_CMD_POSMONITOR
** 函数描述:    MSG_8202 位置追踪控制
** 参数:        [in]  NULL                  
** 返回:        NULL
********************************************************************/
static void HdlMsg_DN_CMD_POSMONITOR(PROTOCOL_COM_T *attrib, PROTOCOL_HEAD_T *head, INT8U *data, INT16U datalen)
{
    //INT16U period;
    INT32U lifecycle;
    STREAM_T rstrm;
    
    YX_InitStrm(&rstrm, data, datalen);
    //period = YX_ReadHWORD_Strm(&rstrm);
    YX_ReadHWORD_Strm(&rstrm);
    lifecycle = YX_ReadLONG_Strm(&rstrm);
    lifecycle = (lifecycle + 59) / 60;
    if (lifecycle > 60) {                                                      /* 最大唤醒60分钟 */
        lifecycle = 60;
    }
    
    if (lifecycle > 0) {
        YX_SLEEP_Wakeup(lifecycle, WAKEUP_EVENT_SERVER);
    }

    YX_PROTOCOL_SendCommonAck(head, attrib, ACK_FAIL);
}

/*******************************************************************
** 函数名:      HdlMsg_DN_CMD_SETPARA
** 函数描述:    MSG_8103 参数设置接口
** 参数:        [in]  NULL                  
** 返回:        NULL
********************************************************************/
static void HdlMsg_DN_CMD_SETPARA(PROTOCOL_COM_T *attrib, PROTOCOL_HEAD_T *head, INT8U *data, INT16U datalen)
{
    YX_SLEEP_Wakeup(20, WAKEUP_EVENT_SERVER);
    YX_PROTOCOL_SendCommonAck(head, attrib, ACK_FAIL);
}


/*******************************************************************
** 函数名:      HdlMsg_DN_CMD_CONTROLDEV
** 函数描述:    MSG_8105 终端控制
** 参数:        [in]  NULL                  
** 返回:        NULL
********************************************************************/
static void HdlMsg_DN_CMD_CONTROLDEV(PROTOCOL_COM_T *attrib, PROTOCOL_HEAD_T *head, INT8U *data, INT16U datalen)
{
    switch (data[0])
    {
    case 0x01:                                                                 /* 无线升级 */
        YX_SLEEP_Wakeup(40, WAKEUP_EVENT_SERVER);
        break;
    case 0x02:                                                                 /* 控制终端连接指定服务器 */
        YX_SLEEP_Wakeup(20, WAKEUP_EVENT_SERVER);
        break;
    case 0x03:                                                                 /* 终端关机 */
        break;
    case 0x04:                                                                 /* 终端复位 */
        YX_PROTOCOL_SendCommonAck(head, attrib, ACK_SUCCESS);
        return;  
    case 0x05:                                                                 /* 恢复出厂设置 */
        YX_SLEEP_Wakeup(20, WAKEUP_EVENT_SERVER);
        break;
    case 0x06:                                                                 /* 关闭数据通信 */
        break;
    case 0x07:                                                                 /* 终端所有无线通信 */
        break;
    default:
        break;
    }
    YX_PROTOCOL_SendCommonAck(head, attrib, ACK_FAIL);
}
    

static FUNCENTRY_PROTOCOL_T const s_functionentry[] = {
     DN_ACK_REG,                    HdlMsg_DN_ACK_REG         /* 终端注册应答 */
    ,DN_ACK_COMMON,                 HdlMsg_DN_ACK_COMMON      /* 通用应答 */
    ,DN_CMD_POSQRY,                 HdlMsg_DN_CMD_POSQRY      /* 位置信息查询 */
    ,DN_CMD_POSMONITOR,             HdlMsg_DN_CMD_POSMONITOR  /* 位置跟踪 */
    ,DN_CMD_SETPARA,                HdlMsg_DN_CMD_SETPARA     /* 设置参数 */
    ,DN_CMD_CONTROLDEV,             HdlMsg_DN_CMD_CONTROLDEV  /* 终端控制 */
};

/*******************************************************************
** 函数名:     HandlerSysFrame
** 函数描述:   处理协议数据帧
**             [in]  attrib: 通道属性和重发属性，见PROTOCOL_COM_T
**             [in]  sptr:    数据指针
**             [in]  slen:    数据长度
** 返回:       成功返回true，失败返回false
********************************************************************/
static void HandlerSysFrame(PROTOCOL_COM_T *attrib, INT8U *sptr, INT16U slen)
{
    INT8U i;
    INT16U framelen, msgid, msgattrib, exheadlen, msglen;
    PROTOCOL_HEAD_T *frameptr;
    
    #if DEBUG_TLINK > 0
    printf_com("<HandlerSysFrame(%d):", slen);
    printf_hex(sptr, slen);
    printf_com(">\r\n");
    #endif

    framelen = slen;
    frameptr = (PROTOCOL_HEAD_T *)sptr;
    msgid     = (frameptr->msgid[0] << 8) + frameptr->msgid[1];
    msgattrib = (frameptr->msgattrib[0] << 8) + frameptr->msgattrib[1];
    msglen    = (msgattrib & 0x03FF);

    exheadlen = 0;
    if ((msgattrib & PROTOCOL_EXT_HEAD) != 0) {
        exheadlen = 4;
    }
    msglen = framelen - SYSHEAD_LEN - SYSTAIL_LEN - exheadlen;

    for (i = 0; i < sizeof(s_functionentry) / sizeof(s_functionentry[0]); i++) {
        if (s_functionentry[i].index == msgid) {
            s_functionentry[i].handler(attrib, frameptr, frameptr->data + exheadlen, msglen);
            return;
        }
    }
    
    YX_PROTOCOL_SendCommonAck(frameptr, attrib, ACK_FAIL);                     /* 接收到其他消息,则应答失败 */
}

/*******************************************************************
** 函数名:     YX_PROTOCOL_InitRecv
** 函数描述:   初始化协议接收处理模块
** 参数:       无
** 返回:       无
********************************************************************/
void YX_PROTOCOL_InitRecv(void)
{
    ;
}

/*******************************************************************
** 函数名:     YX_PROTOCOL_HandleFrame
** 函数描述:   处理协议数据帧
**             [in]  attrib: 通道属性和重发属性，见PROTOCOL_COM_T
**             [in]  sptr:    数据指针
**             [in]  slen:    数据长度
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN YX_PROTOCOL_HandleFrame(PROTOCOL_COM_T *attrib, INT8U *sptr, INT16U slen)
{
    INT16U framelen;
    PROTOCOL_HEAD_T *frameptr;
    
    framelen = slen;
    frameptr = (PROTOCOL_HEAD_T *)sptr;
    
    if (!CheckFrameValid(frameptr, framelen)) {                                /* 判断校验码的正确性 */
        #if DEBUG_TLINK > 0
        printf_com("<system data checksum error(%d):", slen);
        printf_hex(sptr, slen);
        printf_com(">\r\n");
        #endif
        
        goto HANDLE_EXIT;
    }
    
    HandlerSysFrame(attrib, sptr, slen);
    return true;
    
HANDLE_EXIT:
    return false;
}


