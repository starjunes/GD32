/********************************************************************************
**
** 文件名:     at_q_tcpip.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现手机模块TCP/IP指令发送队列管理
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
#include "yx_list.h"
#include "at_recv.h"
#include "at_send.h"
#include "at_cmd_common.h"
#include "at_q_set.h"

#if EN_AT > 0
/*
********************************************************************************
* 定义模块配置参数
********************************************************************************
*/
#define NUM_CMD              5
#define SIZE_BUF             50

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    INT8U  len;
    INT8U  memlen;
    INT8U *memptr;
    AT_CMD_PARA_T const *cmdpara;
    void (*fp)(INT8U result);
} CELL_T;

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
static struct {
    NODE_T  next;
    CELL_T  command;
} memblock[NUM_CMD];

static LIST_T s_freelist, s_usedlist;
static CELL_T *s_curscb;


/*******************************************************************
** 函数名:     AllocCell
** 函数描述:   申请链表节点
** 参数:       无
** 返回:       返回链表节点指针
********************************************************************/
static CELL_T *AllocCell(void)
{
    CELL_T *cell;
    
    cell = (CELL_T *)YX_LIST_DeleListHead(&s_freelist);
    if (cell == 0) {
        return 0;
    }
    YX_MEMSET((INT8U *)cell, 0, sizeof(CELL_T));
    
    cell->memlen = SIZE_BUF;
    cell->memptr = YX_DYM_Alloc(cell->memlen);
    if (cell->memptr == 0) {
        YX_LIST_AppendListEle(&s_freelist, (INT8U *)cell);
        return 0;
    }
    return cell;
}

/*******************************************************************
** 函数名:     Callback_Send
** 函数描述:   发送结果回调
** 参数:       [in] result: 发送结果
** 返回:       无
********************************************************************/
static void Callback_Send(INT8U result)
{
    void (*fp)(INT8U result);

    if (s_curscb != 0) {
        fp = s_curscb->fp;
        if (s_curscb->memptr != 0) {
            YX_DYM_Free(s_curscb->memptr);
            s_curscb->memptr = 0;
        }
        
        YX_LIST_AppendListEle(&s_freelist, (INT8U *)s_curscb);
        s_curscb = 0;
        
        if (fp != 0) {
            if (result == AT_SUCCESS) {
                result = _SUCCESS;
            } else {
                result = _FAILURE;
            }
            fp(result);
        }
    }
}

/*void DiagnoseProc(void)
{
    INT8U count;
    
    //if (!CheckList(&s_freelist) || !CheckList(&s_usedlist)) 
    //    ErrExit(ERR_ATSET_MEM);

   count = YX_LIST_GetNodeNum(&s_freelist) + YX_LIST_GetNodeNum(&s_usedlist);
   if (s_curscb != 0) count++;
   OS_ASSERT((count == sizeof(memblock) / sizeof(memblock[0])), RETURN_VOID);
}*/

/*******************************************************************
** 函数名:     AT_Q_InitTcpip
** 函数描述:   初始化模块
** 参数:       无
** 返回:       无
********************************************************************/
void AT_Q_InitTcpip(void)
{
    s_curscb = 0;
    YX_LIST_CreateList(&s_freelist, (INT8U *)memblock, sizeof(memblock) / sizeof(memblock[0]), sizeof(memblock[0]));
    YX_LIST_Init(&s_usedlist);
    //OS_InstallDiag(DiagnoseProc);
}

/*******************************************************************
** 函数名:     AT_Q_TcpipSendEntry
** 函数描述:   发送入口
** 参数:       无
** 返回:       无
********************************************************************/
void AT_Q_TcpipSendEntry(void)
{
    if (s_curscb != 0 || YX_LIST_GetNodeNum(&s_usedlist) == 0) {
        return;
    }
    
    if (!AT_SEND_CanSendATCmd()) {
        return;
    }
    
    s_curscb = (CELL_T *)YX_LIST_DeleListHead(&s_usedlist);
    AT_SEND_SendCmd(s_curscb->cmdpara, s_curscb->memptr, s_curscb->len, Callback_Send);
}

/*******************************************************************
** 函数名:     AT_Q_ClearTcpipQueue
** 函数描述:   清除TCPIP指令队列
** 参数:       无
** 返回:       无
********************************************************************/
void AT_Q_ClearTcpipQueue(void)
{
    CELL_T *cell;
    CELL_T *tmp;
    
    if (s_curscb != 0) {
        YX_SEND_AbandonATCmd();                                                /* 放弃当前指令 */
        Callback_Send(_FAILURE);
    }

    cell = (CELL_T  *)YX_LIST_GetListHead(&s_usedlist);
    for (;;) {
        if (cell == 0) {
            break;
        }
        
        tmp = cell;
        if (cell->memptr != 0) {
            YX_DYM_Free(cell->memptr);
            cell->memptr = 0;
        }
            
        cell = (CELL_T  *)YX_LIST_DeleListEle(&s_usedlist, (INT8U *)cell);
        YX_LIST_AppendListEle(&s_freelist, (INT8U *)tmp);
    }
}

/*******************************************************************
** 函数名:     AT_Q_SetAuthentication
** 函数描述:   启动任务并设置APN 
** 参数:       [in] fp：       回调函数
**             [in] profileid: 参数组号
**             [in] apn:       APN，以'\0'未结束符
**	           [in] username:  用户名，以'\0'未结束符
**	           [in] password:  密码，以'\0'未结束符
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN AT_Q_SetAuthentication(void (*fp)(INT8U result), INT8U profileid, char *apn, char *username, char *password)
{
    CELL_T  *cell;

    if ((cell = AllocCell()) == 0) {
        return false;
    }
    cell->len     = AT_CMD_SetAuthentication(cell->memptr, cell->memlen, 0, apn, username, password);
    cell->cmdpara = &g_pdp_setauth_para;
    cell->fp      = fp;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;
}

/*******************************************************************
** 函数名:     AT_Q_ActivePDPContext
** 函数描述:   激活移动场景
** 参数:       [in] fp：       回调函数
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN AT_Q_ActivePDPContext(void (*fp)(INT8U result))
{
    CELL_T  *cell;

    if ((cell = AllocCell()) == 0) {
        return false;
    }
    cell->len     = AT_CMD_ActivePDPContext(cell->memptr, cell->memlen);
    cell->cmdpara = &g_pdp_active_para;
    cell->fp      = fp;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;
}

/*******************************************************************
** 函数名:     AT_Q_DeactivePDPContext
** 函数描述:   关闭移动场景,去活GPRS上下文
** 参数:       [in] fp：       回调函数
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN AT_Q_DeactivePDPContext(void (*fp)(INT8U result))
{
    CELL_T  *cell;

    if ((cell = AllocCell()) == 0) {
        return false;
    }
    cell->len     = AT_CMD_DeactivePDPContext(cell->memptr, cell->memlen);
    cell->cmdpara = &g_pdp_deactive_para;
    cell->fp      = fp;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;
}

/*******************************************************************
** 函数名:     AT_Q_GetLocalIp
** 函数描述:   获得本地IP地址
** 参数:       [in] fp：       回调函数
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN AT_Q_GetLocalIp(void (*fp)(INT8U result))
{
    CELL_T  *cell;

    if ((cell = AllocCell()) == 0) {
        return false;
    }
    cell->len     = AT_CMD_GetLocalIp(cell->memptr, cell->memlen);
    cell->cmdpara = &g_pdp_getlocalip_para;
    cell->fp      = fp;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;
}

/*******************************************************************
** 函数名:     AT_Q_SocketConnect
** 函数描述:   socket连接
** 参数:       [in] fp：       回调函数
**             [in] ch：       socket通道
**             [in] type：     socket类型，见SOCKET_TYPE_E
**             [in] ip：       ip地址
**             [in] port：     端口
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN AT_Q_SocketConnect(void (*fp)(INT8U result), INT8U ch, INT8U type, char *ip, INT16U port)
{
    CELL_T  *cell;

    if ((cell = AllocCell()) == 0) {
        return false;
    }
    cell->len     = AT_CMD_SocketConnect(cell->memptr, cell->memlen, ch, type, ip, port);
    cell->cmdpara = &g_socket_connect_para;
    cell->fp      = fp;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;
}

/*******************************************************************
** 函数名:     AT_Q_SocketClose
** 函数描述:   socket关闭
** 参数:       [in] fp：       回调函数
**             [in] ch：       socket通道
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN AT_Q_SocketClose(void (*fp)(INT8U result), INT8U ch)
{
    CELL_T  *cell;

    if ((cell = AllocCell()) == 0) {
        return false;
    }
    cell->len     = AT_CMD_SocketClose(cell->memptr, cell->memlen, ch);
    cell->cmdpara = &g_socket_close_para;
    cell->fp      = fp;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;
}

/*******************************************************************
** 函数名:     AT_Q_QuerySocketStatus
** 函数描述:   查询socket连接状态
** 参数:       [in] fp：       回调函数
**             [in] ch：       socket通道
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN AT_Q_QuerySocketStatus(void (*fp)(INT8U result), INT8U ch)
{
    CELL_T  *cell;

    if ((cell = AllocCell()) == 0) {
        return false;
    }
    
    cell->len     = AT_CMD_QuerySocketStatus(cell->memptr, cell->memlen, ch);
    cell->cmdpara = &g_socket_query_para;
    cell->fp      = fp;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;
}

/*******************************************************************
** 函数名:     AT_Q_SetMultiSocket
** 函数描述:   设置多路IP
** 参数:       [in] fp：       回调函数
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN AT_Q_SetMultiSocket(void (*fp)(INT8U result))
{
    CELL_T  *cell;

    if ((cell = AllocCell()) == 0) {
        return false;
    }
    cell->len     = AT_CMD_SetMultiSocket(cell->memptr, cell->memlen);
    cell->cmdpara = &g_socket_multi_para;
    cell->fp      = fp;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;
}

/*******************************************************************
** 函数名:     AT_Q_GetIpByDomainName
** 函数描述:   根据域名获取IP地址
** 参数:       [in] fp：        回调函数
**             [in] domainname：域名
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN AT_Q_GetIpByDomainName(void (*fp)(INT8U result), char *domainname)
{
    CELL_T  *cell;

    if ((cell = AllocCell()) == 0) {
        return false;
    }
    cell->len     = AT_CMD_GetIpByDomainName(cell->memptr, cell->memlen, domainname);
    cell->cmdpara = &g_dns_getipbyname_para;
    cell->fp      = fp;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;
}

/*******************************************************************
** 函数名:     AT_Q_SetDNS
** 函数描述:   设置域名解析服务器地址
** 参数:       [in] fp：    回调函数
**             [in] pri_ip：主服务器IP
**             [in] sec_ip：次服务器IP
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN AT_Q_SetDNS(void (*fp)(INT8U result), char *pri_ip, char *sec_ip)
{
    CELL_T  *cell;

    if ((cell = AllocCell()) == 0) {
        return false;
    }
    cell->len     = AT_CMD_SetDNS(cell->memptr, cell->memlen, pri_ip, sec_ip);
    cell->cmdpara = &g_dns_setdns_para;
    cell->fp      = fp;
    YX_LIST_AppendListEle(&s_usedlist, (INT8U *)cell);
    return true;
}


#endif

