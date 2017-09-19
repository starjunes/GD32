/********************************************************************************
**
** 文件名:     at_socket_drv.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现SOCKET驱动管理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2014/03/15 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "yx_include.h"
#include "yx_misc.h"
#include "yx_dym_drv.h"
#include "yx_list.h"
#include "at_drv.h"


#if EN_AT > 0

/*
********************************************************************************
* define config parameters
********************************************************************************
*/
enum {
    _FREE = 0,
    _CREATE,       /* socket以创建 */
    _CONNECTING,   /* socket连接中 */
    _CONNECTED,    /* socket已连接 */
    _DISCONNECTING,/* 关闭socket中 */
    _SENDING,      /* 发送中状态 */
    _ACKING,       /* 等待应答中状态 */
    _MAX
};

#define SIZE_OF_SEND         1450

/*
********************************************************************************
* define struct
********************************************************************************
*/

typedef struct {
    INT8U   status[SOCKET_CH_MAX];
    INT8U   sockettype[SOCKET_CH_MAX];
    INT8U   contxtid[SOCKET_CH_MAX];
    
    INT8U   socketid;
    INT8U   sendstatus[SOCKET_CH_MAX];
    INT16U  sendlen[SOCKET_CH_MAX];
    INT16U  offset[SOCKET_CH_MAX];
    INT8U  *memptr[SOCKET_CH_MAX];
} DCB_T;

/*
********************************************************************************
* define module variants
********************************************************************************
*/
static DCB_T s_dcb;
static SOCKET_CALLBACK_T s_socket_callback;




/*******************************************************************
** 函数名:     Callback_SocketRecv
** 函数描述:   处理接收的数据
** 参数:       [in] socketid:  sock编号
**             [in] sptr:      接收数据缓冲区
**             [in] len:       接收数据缓冲区长度
** 返回:       无
********************************************************************/
static void Callback_SocketRecv(INT8U contexid, INT8S socketid, INT8U *sptr, INT32U slen)
{
    if (socketid >= SOCKET_CH_MAX || slen == 0) {
        return;
    }
    
    if (s_dcb.status[socketid] != _CONNECTED) {
        return;
    }
    
    if (s_socket_callback.callback_socket_recv != 0) {
        s_socket_callback.callback_socket_recv(contexid, socketid, sptr, slen);  
    }
}

/*******************************************************************
** 函数名:     Callback_SocketConnect
** 函数描述:   通知socket已连接
** 参数:       [in] socketid:   socket编号
** 返回:       无
********************************************************************/
static void Callback_SocketConnect(INT8U contexid, INT8S socketid, INT8U result, INT32S error_code)
{
    #if DEBUG_AT > 0
    printf_com("<Callback_SocketConnect(%d)(%d)>\r\n", socketid, s_dcb.status[socketid]);
    #endif
    
    if (socketid >= SOCKET_CH_MAX) {
        return;
    }
    if (s_dcb.status[socketid] != _CONNECTING) {
        return;
    }
    
    s_dcb.status[socketid] = _CONNECTED;
    if (s_socket_callback.callback_socket_connect != 0) {
        s_socket_callback.callback_socket_connect(PDP_COM_0, socketid, result, 0);  
    }
}

/*******************************************************************
** 函数名:     Callback_SocketClose
** 函数描述:   通知socket已断开
** 参数:       [in] socketid:   socket编号
** 返回:       无
********************************************************************/
static void Callback_SocketClose(INT8U contexid, INT8S socketid, INT8U result, INT32S error_code)
{
    #if DEBUG_AT > 0
    printf_com("<Callback_SocketClose(%d)(%d)>\r\n", socketid, s_dcb.status[socketid]);
    #endif
    
    if (socketid >= SOCKET_CH_MAX) {
        return;
    }
    
    if (s_dcb.status[socketid] == _FREE) {
        return;
    }
    
    AT_SOCKET_ClearDataBySocket(socketid);
    s_dcb.status[socketid] = _FREE;
    if (s_socket_callback.callback_socket_close != 0) {
        s_socket_callback.callback_socket_close(PDP_COM_0, socketid, result, 0);  
    }
}

/*******************************************************************
** 函数名:     Callback_SocketSend
** 函数描述:   发送数据应答
** 参数:       [in] socketid:  sock编号
**             [in] sendlen:       接收数据缓冲区长度
** 返回:       无
********************************************************************/
void Callback_SocketSend(INT8U contexid, INT8S socketid, INT8U result, INT32S error_code, INT16U sendlen)
{
    #if DEBUG_AT > 0
    printf_com("Callback_SocketSend(%d)(%d)>\r\n", socketid, sendlen);
    #endif
    
    if (socketid >= SOCKET_CH_MAX) {
        return;
    }
    
    if (s_dcb.sendstatus[socketid] != _FREE) {
        s_dcb.sendstatus[socketid] = _FREE;
        if (s_dcb.sendlen[socketid] > sendlen) {
            s_dcb.sendlen[socketid] -= sendlen;
            s_dcb.offset[socketid] += sendlen;
        } else {
            s_dcb.sendlen[socketid] = 0;
            YX_DYM_Free(s_dcb.memptr[socketid]);
            s_dcb.memptr[socketid] = 0;
        }
    }
}

/*******************************************************************
** 函数名:     AT_SOCKET_InitDrv
** 函数描述:   初始化SOCKET网络
** 参数:       无
** 返回:       无
********************************************************************/
void AT_SOCKET_InitDrv(void)
{
    SOCKET_CALLBACK_T callback;
    
    YX_MEMSET(&s_dcb, 0, sizeof(s_dcb));
    YX_MEMSET(&s_socket_callback, 0, sizeof(s_socket_callback));
    
    callback.callback_socket_connect = Callback_SocketConnect;
    callback.callback_socket_close   = Callback_SocketClose;
    callback.callback_socket_recv    = Callback_SocketRecv;
    callback.callback_socket_send    = Callback_SocketSend;
    
    AT_URC_RegistSocketHandler(&callback);
}

/*******************************************************************
** 函数名:     AT_SOCKET_RegistHandler
** 函数描述:   注册SOCKET消息处理器
** 参数:       [in]  callback: 注册回调函数参数
** 返回:       成功返回TRUE, 失败返回FALSE
********************************************************************/
BOOLEAN AT_SOCKET_RegistHandler(SOCKET_CALLBACK_T *callback)
{
    YX_MEMCPY(&s_socket_callback, sizeof(s_socket_callback), callback, sizeof(s_socket_callback));
    return true;
}

/*******************************************************************
** 函数名:     AT_SOCKET_ClearDataBySocket
** 函数描述:   根据socket编号，清除通道数据
** 参数:       [in] socketid:   socket编号
** 返回:       无
********************************************************************/
void AT_SOCKET_ClearDataBySocket(INT8U socketid)
{
    OS_ASSERT((socketid < SOCKET_CH_MAX), RETURN_VOID);
    
    s_dcb.sendstatus[socketid] = _FREE;
    s_dcb.sendlen[socketid] = 0;
    
    if (s_dcb.memptr[socketid] != 0) {
        YX_DYM_Free(s_dcb.memptr[socketid]);
        s_dcb.memptr[socketid] = 0;
    }
}

/*******************************************************************
** 函数名:     AT_SOCKET_CloseAllSocket
** 函数描述:   关闭通道数据，并清除通道数据
** 参数:       无
** 返回:       无
********************************************************************/
void AT_SOCKET_CloseAllSocket(void)
{
    INT8U i;
    
    for (i = 0; i < SOCKET_CH_MAX; i++) {
        s_dcb.status[i] = _FREE;
        AT_SOCKET_ClearDataBySocket(i);                                        /* 清除发送缓存 */
    }
}

/*******************************************************************
** 函数名:     AT_SOCKET_SocketIsClosed
** 函数描述:   判断socket是否已关闭
** 参数:       [in] socketid:  sock编号
** 返回:       是返回true,否返回false
********************************************************************/
BOOLEAN AT_SOCKET_SocketIsClosed(INT8U socketid)
{
    if (socketid >= SOCKET_CH_MAX) {
        return false;
    }
    
    if (s_dcb.status[socketid] == _FREE) {
        return true;
    } else {
        return false;
    }
}

/*******************************************************************
** 函数名:     AT_SOCKET_SocketBind
** 函数描述:   绑定socket,为该socket绑定本地端口
** 参数:       [in]  socketid:   socket编号
**             [in]  socket_type:SOCKET类型,SOCKET_TYPE_E
**             [in]  local_ip:   本地ip地址，依次从从高位到低位
**             [in]  local_port: 本地端口
** 返回:       成功返回0，失败返回-1
********************************************************************/
INT32S AT_SOCKET_SocketBind(INT8U socketid, INT32U socket_type, INT32U local_ip, INT16U local_port)
{
#if 0
    INT32S result;
    
    local_ip = local_ip;
    result = Ql_SocketBind(socketid, socket_type, local_port);
    
    if (result == QL_SOC_SUCCESS) {
        return 0;
    } else {
        return -1;
    }
#endif
    return -1;
}

/*******************************************************************
** 函数名:     AT_SOCKET_SocketConnect
** 函数描述:   连接sock
** 参数:       [in]  socketid:  sock编号
**             [in]  socket_type:SOCKET类型,见SOCKET_TYPE_E
**             [in]  host_ip:   主机ip地址,依次从从高位到低位
**             [in]  host_port: 主机端口
** 返回:       0表示连接成功；
**            -1表示阻塞中，等待callback回调；
**            -2表示参数有误；
**            -3其他错误
********************************************************************/
void Callback_Connect(INT8U result)
{
    AT_Q_QuerySocketStatus(0, 0);
}

INT32S AT_SOCKET_SocketConnect(INT8U socketid, INT8U socket_type, char *host_ip, INT16U host_port)
{
    if (socketid >= SOCKET_CH_MAX) {
        return -3;
    }
    
    if (AT_Q_SocketConnect(Callback_Connect, socketid, s_dcb.sockettype[socketid], host_ip, host_port)) {
        s_dcb.status[socketid]     = _CONNECTING;
        s_dcb.sockettype[socketid] = socket_type;
        //s_dcb.contxtid[socketid]   = contxtid;
        return -1;
    } else {
        return -3;
    }
}

/*******************************************************************
** 函数名:     AT_SOCKET_SocketClose
** 函数描述:   关闭sock
** 参数:       [in]  socketid:  sock编号
** 返回:       0表示关闭成功；
**            -1表示阻塞中，等待callback回调；
**            -2表示参数有误；
**            -3其他错误
********************************************************************/
void Callback_Close(INT8U result)
{
    AT_Q_QuerySocketStatus(0, 0);
}

INT32S AT_SOCKET_SocketClose(INT8U socketid)
{
    if (socketid >= SOCKET_CH_MAX) {
        return -3;
    }
    
    AT_SOCKET_ClearDataBySocket(socketid);                                    /* 清除收发缓存 */
    
    if (s_dcb.status[socketid] == _FREE) {
        return 0;
    }
        
    if (AT_Q_SocketClose(Callback_Close, socketid)) {
        s_dcb.status[socketid] = _DISCONNECTING;
        return -1;
    } else {
        return -3;
    }
}

/*******************************************************************
** 函数名:     AT_SOCKET_SocketSend
** 函数描述:   通过sock发送数据
** 参数:       [in]  socketid:  sock编号
**             [in]  sptr:      待发送数据缓冲区
**             [in]  slen:      待发送数据长度
** 返回:      -1表示阻塞中，等待callback回调
**            -2表示链路可能已断开，需要close
**            >0表示实际已发送的数据长度
********************************************************************/
INT32S AT_SOCKET_SocketSend(INT8U socketid, INT8U *sptr, INT32U slen)
{
    #if DEBUG_AT > 0
    printf_com("AT_SOCKET_SocketSend(%d)\r\n", slen);
    #endif
    
    if (socketid >= SOCKET_CH_MAX) {
        return -1;
    }
    
    if (s_dcb.status[socketid] != _CONNECTED) {
        return -1;
    }
    
    OS_ASSERT((s_dcb.sendstatus[socketid] == _FREE || s_dcb.sendlen[socketid] > 0), -1);

    if (s_dcb.sendlen[socketid] > 0) {
        return -1;
    }

    if (s_dcb.sockettype[socketid] == SOCKET_TYPE_DATAGRAM) {
        if (slen > SIZE_OF_SEND) {
            return -2;
        }
    } else {
        if (slen > SIZE_OF_SEND) {
            slen = SIZE_OF_SEND;
        }
    }
    
    if (s_dcb.memptr[socketid] != 0) {
        YX_DYM_Free(s_dcb.memptr[socketid]);
        s_dcb.memptr[socketid] = 0;
    }
    
    s_dcb.memptr[socketid] = YX_DYM_Alloc(slen);
    if (s_dcb.memptr[socketid] == 0) {
        return -1;
    }
    
    s_dcb.sendstatus[socketid]  = _FREE;
    s_dcb.offset[socketid]      = 0;
    s_dcb.sendlen[socketid]     = slen;
    
    YX_MEMCPY(s_dcb.memptr[socketid], slen, sptr, slen);
    return slen;
}

/*******************************************************************
** 函数名:     AT_SOCKET_SocketRecv
** 函数描述:   从sock上接收数据
** 参数:       [in]  socketid:  sock编号
**             [out] dptr:      接收数据缓冲区
**             [in]  maxlen:    接收数据缓冲区长度
** 返回:       -1:表示阻塞中，无数据，等待callback回调
**             -2:表示服务器关闭socket，需要close
**             -3:表示链路可能已断开，需要close
**             >0:实际接收的数据长度
********************************************************************/
INT32S AT_SOCKET_SocketRecv(INT8U socketid, INT8U *dptr, INT32U maxlen)
{
    INT32S readlen = -1;

    return readlen;
}

#if 0
/*******************************************************************
** 名称: AT_SOCKET_UdpSend
*  描述: udp数据发送接口
** 参数:       [in]  socketid:  sock编号
**             [in]  host_ip:   服务器IP地址
**             [in]  host_port: 服务器端口号
**			   [in]	 localport:	本地端口号
**             [in]  sptr:      待发送数据
**             [in]  len:       待发送数据长度
** 返回:      -1表示阻塞中，等待callback回调
**            -2表示发送失败
**             0表示发送成功
*******************************************************************/
INT32S AT_SOCKET_UdpSend(INT8U socketid, char *host_ip, INT16U host_port, INT16U localport, INT8U *sptr, INT32U len)
{
    INT32S result;
    
    if (socketid >= SOCKET_CH_MAX) {
        return -1;
    }
    
    if (s_dcb.status[socketid] == _CREATE) {
        AT_SOCKET_SocketConnect(socketid, host_ip, host_port);
        return -1;
    }
    
    if (s_dcb.status[socketid] != _CONNECTED) {
        return -1;
    }
    
    result = AT_SOCKET_SocketSend(socketid, sptr, len);
    if (result == len) {
        return 0;
    } else {
        return result;
    }
}
#endif

/*******************************************************************
** 函数名:     AT_SOCKET_UdpRecv
** 函数描述:   UDP数据报接收
** 参数:       [in]  socketid:  sock编号
**             [out] dptr:      接收数据缓冲区
**             [in]  maxlen:    接收数据缓冲区最大长度
** 返回:       -1:表示阻塞中，无数据，等待callback回调
**             -2:接收失败
**             >0:实际接收的数据长度
********************************************************************/
INT32S AT_SOCKET_UdpRecv(INT32S socketid, INT8U *dptr, INT32U maxlen)
{
    INT32S result;
    
    result = AT_SOCKET_SocketRecv(socketid, dptr, maxlen);
    return result;
}


/*******************************************************************
** 函数名:     AT_SOCKET_SendEntry
** 函数描述:   发送socket数据入口函数
** 参数:       无
** 返回:       无
********************************************************************/
static void Callback_Send(INT8U result)
{
    OS_ASSERT((s_dcb.socketid < SOCKET_CH_MAX), RETURN_VOID);
    
    if (s_dcb.sendstatus[s_dcb.socketid] == _SENDING) {
        if (result == AT_SUCCESS) {
            s_dcb.sendstatus[s_dcb.socketid] = _ACKING;
        } else {
            s_dcb.sendstatus[s_dcb.socketid] = _FREE;
        }
    }
}

void AT_SOCKET_SendEntry(void)
{
    INT8U i;
    INT16U sendlen, memlen;
    INT8U *memptr;
    
    if (!AT_SEND_CanSendATCmd()) {
        return;
    }
    
    if (s_dcb.sendstatus[s_dcb.socketid] != _FREE) {
        return;
    }
    
    for (i = 0; i < SOCKET_CH_MAX; i++) {
        if (++s_dcb.socketid >= SOCKET_CH_MAX) {
            s_dcb.socketid = 0;
        }
        
        if (s_dcb.sendlen[s_dcb.socketid] == 0) {
            continue;
        }
        
        i = s_dcb.socketid;
        
        memlen = s_dcb.sendlen[i] + 20;
        memptr = YX_DYM_Alloc(memlen);
        if (memptr == 0) {
            break;
        }
        
        s_dcb.sendstatus[i] = _SENDING;
        sendlen = AT_CMD_SocketSend(memptr, memlen, i, s_dcb.sockettype[i], s_dcb.memptr[i] + s_dcb.offset[i], s_dcb.sendlen[i]);
        AT_SEND_SendCmd(&g_socket_send_para, memptr, sendlen, Callback_Send);
        YX_DYM_Free(memptr);
        break;
    }
}

/*******************************************************************
** 函数名:     AT_SOCKET_HaveTask
** 函数描述:   是否AT需要发送
** 参数:       无
** 返回:       有返回true，否则返回false
********************************************************************/
BOOLEAN AT_SOCKET_HaveTask(void)
{
    INT8U i;
    
    for (i = 0; i < SOCKET_CH_MAX; i++) {
        if (s_dcb.sendlen[i] > 0) {
            return true;
        }
    }
    return false;
}

#endif

