/********************************************************************************
**
** 文件名:     yx_protocol_send.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现协议数据组帧和发送管理
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
#include "yx_jt_linkman.h"


/*
********************************************************************************
* 定义配置参数
********************************************************************************
*/
#define SIZE_SEND            128

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    INT16U   frameseq;       /* 数据帧流水号 */
    STREAM_T wstrm;          /* 数据流 */
} DCB_T;

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static DCB_T s_dcb;
static INT8U s_buffer[SIZE_SEND];



/*******************************************************************
** 函数名:     SendProtocolData
** 函数描述:   发送协议数据
** 参数:       [in]  attrib:   通道属性和重发属性，见PROTOCOL_COM_T
**             [in]  data:     数据指针
**             [in]  datalen:  数据长度
**             [in]  fp:       发送结果通知回调函数
** 返回:       调用成功返回true，失败返回false，失败无fp回调
********************************************************************/
static BOOLEAN SendProtocolData(PROTOCOL_COM_T *attrib, INT8U *data, INT16U datalen, void (*fp)(INT8U result))
{
    INT8U result;
    
    result = false;
    /* 从TCP通道发送数据 */
    if (attrib->channel == SOCKET_CH_0) {
        if (YX_JTT1_SendFromGPRS(attrib, data, datalen)) {
            result = true;
            if (fp != 0) {
                fp(_SUCCESS);
            }
        }
    }
    
    if (attrib->channel == SOCKET_CH_1) {
        if (YX_JTT2_SendFromGPRS(attrib, data, datalen)) {
            result = true;
            if (fp != 0) {
                fp(_SUCCESS);
            }
        }
    }
    
    return result;
}

/*******************************************************************
** 函数名:     YX_PROTOCOL_InitSend
** 函数描述:   初始化协议发送模块
** 参数:       无
** 返回:       无
********************************************************************/
void YX_PROTOCOL_InitSend(void)
{
    YX_MEMSET(&s_dcb, 0, sizeof(s_dcb));
}

/*******************************************************************
** 函数名:     YX_PROTOCOL_GetBufferStrm
** 函数描述:   获取流缓存，目前流缓存最大为1600字节
**             备注：本接口用于需要暂时需要缓存地方，如系统协议帧发送，注意不要冲突使用
** 参数:       无
** 返回:       返回数据流指针
********************************************************************/
STREAM_T *YX_PROTOCOL_GetBufferStrm(void)
{
    YX_InitStrm(&s_dcb.wstrm, s_buffer, sizeof(s_buffer));
    return &s_dcb.wstrm;
}

/*******************************************************************
** 函数名:     YX_PROTOCOL_GetFrameSeq
** 函数描述:   获取数据帧流水号
** 参数:       无
** 返回:       数据帧流水号(强制从1开始, 0表示无效的发送流水号)
********************************************************************/
INT16U YX_PROTOCOL_GetFrameSeq(void)
{
    if (++s_dcb.frameseq == 0) {
        s_dcb.frameseq = 1;
    }
    return s_dcb.frameseq;
}

/*******************************************************************
** 函数名:     YX_PROTOCOL_AsmFrameHead
** 函数描述:   协议头组帧函数
** 参数:       [in]  wstrm:     发送帧数据流
**             [in]  msgid:     协议类型ID
**             [in]  msgattrib: 数据长度和分包属性
**             [in]  frameseq:  帧流水号
** 返回:       返回帧流水号
********************************************************************/
INT32U YX_PROTOCOL_AsmFrameHead(STREAM_T *wstrm, INT32U msgid, INT32U msgattrib, INT32U frameseq)
{
	INT8U bcdlen, tempbuf[12], bcd[6];
	TEL_T mytel;

    if (frameseq == 0) {
        frameseq = YX_PROTOCOL_GetFrameSeq();
    }
    
    YX_MEMSET(tempbuf, '0', sizeof(tempbuf));
    DAL_PP_ReadParaByID(PP_ID_MYTEL, (INT8U *)&mytel, sizeof(mytel));
    if (mytel.tellen < 12) {
        YX_MEMCPY(&tempbuf[12 - mytel.tellen], mytel.tellen, mytel.tel, mytel.tellen);
    } else {
        YX_MEMCPY(tempbuf, sizeof(tempbuf), &mytel.tel[mytel.tellen - 12], 12);
    }
    
    bcdlen = YX_AsciiToBcd(bcd, tempbuf, 12);

    YX_WriteHWORD_Strm(wstrm, msgid);                                          /* 协议ID */
    YX_WriteHWORD_Strm(wstrm, msgattrib);                                      /* 消息属性 */
    YX_WriteDATA_Strm(wstrm, bcd, bcdlen);                                     /* 本机号 */
    YX_WriteHWORD_Strm(wstrm, frameseq);                                       /* 流水号 */
    return frameseq;
}

/*******************************************************************
** 函数名:     YX_PROTOCOL_SendData
** 函数描述:   发送协议数据帧
** 参数:       [in]  wstrm:  发送帧数据流
**             [in]  attrib: 通道属性和重发属性，见PROTOCOL_COM_T
**             [in]  fp:     发送结果回调
** 返回:       调用成功返回true，失败返回false，失败无fp回调
********************************************************************/
BOOLEAN YX_PROTOCOL_SendData(STREAM_T *wstrm, PROTOCOL_COM_T *attrib, void (*fp)(INT8U result))
{
    INT8U *p_msgattrib;
    INT16U datalen, msgattrib;
    
    p_msgattrib = (INT8U *)YX_GetStrmStartPtr(wstrm) + 2;                      /* 消息体属性 */
    datalen  = YX_GetStrmLen(wstrm) - sizeof(PROTOCOL_HEAD_T) + 1;             /* 数据体长度 */

    msgattrib = (p_msgattrib[0] << 8) + p_msgattrib[1];
    if ((msgattrib & PROTOCOL_EXT_HEAD) != 0) {                                /* 带消息包封装项 */
        datalen -= 4;
    }
    
    msgattrib &= 0xFC00;
    msgattrib += (datalen & 0x3FF);
    
    p_msgattrib[0] = ((msgattrib >> 8) & 0xFF);
    p_msgattrib[1] = msgattrib & 0xFF;

    YX_WriteBYTE_Strm(wstrm, YX_ChkSum_XOR(YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm)));
    return SendProtocolData(attrib, YX_GetStrmStartPtr(wstrm), YX_GetStrmLen(wstrm), fp);
}

/*******************************************************************
** 函数名:     YX_PROTOCOL_SendCommonAck
** 函数描述:   发送通用应答
** 参数:       [in]  recvhead: 接收到的帧头
**             [in]  attrib:   发送属性
**             [in]  result:   应答结果
** 返回:       成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN YX_PROTOCOL_SendCommonAck(PROTOCOL_HEAD_T *recvhead, PROTOCOL_COM_T *attrib, INT8U result)
{
    INT32U msgattrib;
    STREAM_T *wstrm;
    
    msgattrib = 0;
    wstrm = YX_PROTOCOL_GetBufferStrm();                                       /* 获取数据流缓存 */
    YX_PROTOCOL_AsmFrameHead(wstrm, UP_ACK_COMMON, msgattrib, 0);              /* 组帧数据头 */

    YX_WriteDATA_Strm(wstrm, recvhead->flowseq, sizeof(recvhead->flowseq));
    YX_WriteDATA_Strm(wstrm, recvhead->msgid, sizeof(recvhead->msgid));    
    YX_WriteBYTE_Strm(wstrm, result);
    
    return YX_PROTOCOL_SendData(wstrm, attrib, 0);
}


