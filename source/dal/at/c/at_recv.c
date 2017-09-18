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
#include "at_drv.h"


#if EN_AT > 0
/*
********************************************************************************
* 定义模块配置参数
********************************************************************************
*/

#define PERIOD_SCAN          _MILTICK, 1
#define SIZE_AT              192
#define _OPEN                0x01

#define MAX_REG              40

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    INT8U       status;
    INT8U       socket;
    INT8U       prechar;
    INT16U      at_rlen;
    INT8U       at_rbuf[SIZE_AT];
    
    INT16U      ip_rlen;
    INT16U      ip_leftlen;
    
    INT8U       ct_regist;
    INT8U       ct_recv;
    INT8U     (*handler)(INT8U ct_recv, INT8U event, INT8U *sptr, INT16U slen);
} RCB_T;

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static RCB_T s_rcb;
static INT8U s_scantmr;
static INT8U s_overtmr;
static URC_HDL_TBL_T *s_hdl_tbl[MAX_REG];


/*******************************************************************
** 函数名:     DestroyRCB
** 函数描述:   结束当前处理进程
** 参数:       无
** 返回:       无
********************************************************************/
static void DestroyRCB(void)
{
    s_rcb.handler = 0;
    OS_StopTmr(s_overtmr);
}

/*******************************************************************
** 函数名:     HandleUrcData
** 函数描述:   解析接收到数据帧
** 参数:       [in] sptr: 数据指针
**              [in] slen: 数据长度
** 返回:       成功返回TRUE,失败返回FALSE
********************************************************************/
static BOOLEAN HandleUrcData(INT8U *sptr, INT16U slen)
{
    INT8U i;
    BOOLEAN (*search)(INT8U *ptr, INT16U maxlen, char *sptr);
    
    if (slen <= 3) {
        return false;
    }
    
    if (!((sptr[0] >= 'A' && sptr[0] <= 'Z') || (sptr[0] >= '0' && sptr[0] <= '9') || sptr[0] == '+' || sptr[0] == '\"')) {
        return false;
    }

    if (s_rcb.handler != 0) {
        s_rcb.ct_recv++;
        if (s_rcb.handler(s_rcb.ct_recv, EVT_NORMAL, sptr, slen) != AT_CONTINUE) {
            DestroyRCB();
        }
        return true;
    } else {
        for (i = 0; i < s_rcb.ct_regist; i++) {
            if (s_hdl_tbl[i]->fromhead) {
                search = YX_SearchKeyWordFromHead;
            } else {
                search = YX_SearchKeyWord;
            }
            
            if (search(sptr, slen, (char *)s_hdl_tbl[i]->keyword)) {
                if (s_hdl_tbl[i]->handler != 0) {
                    s_rcb.ct_recv = 0;
                    s_rcb.handler = s_hdl_tbl[i]->handler;
                    OS_StartTmr(s_overtmr, _SECOND, s_hdl_tbl[i]->overtime);
                    if (s_rcb.handler(s_rcb.ct_recv, EVT_NORMAL, sptr, slen) != AT_CONTINUE) {
                        DestroyRCB();
                    }
                }
                return true;
            }
        }
        return false;
    }
}

/*******************************************************************
** 函数名:     HdlMsg_ATCMD_RECV
** 函数描述:   处理AT命令
** 参数:       [in] ptr: 数据指针
**             [in] len: 数据长度
** 返回:       无
********************************************************************/
static void HdlMsg_ATCMD_RECV(INT8U *ptr, INT16U len)
{
    if (!HandleUrcData(ptr, len)) {
        HdlATCmdAck(ptr, len);
    }
}

/*******************************************************************
** 函数名:     OverTmrProc
** 函数描述:   定时器处理
** 参数:       [in] pdata: 定时器特征值
** 返回:       无
********************************************************************/
static void OverTmrProc(void *pdata)
{
    if (s_rcb.handler != 0) {
        s_rcb.handler(s_rcb.ct_recv, EVT_OVERTIME, 0, 0);
    }
    DestroyRCB();
}

/*******************************************************************
** 函数名:     ScanTmrProc
** 函数描述:   定时器处理
** 参数:       [in] pdata: 定时器特征值
** 返回:       无
********************************************************************/
static void ScanTmrProc(void *pdata)
{
    INT16S ch;
    INT8U  curchar;

    OS_StartTmr(s_scantmr, PERIOD_SCAN);

    for (;;) {
        if ((ch = AT_COM_ReadByte()) == -1) {
            break;
        }
        curchar = ch;
        //printf_com("%c", curchar);
        //printf_com("\r\n");

        if (s_rcb.ip_rlen == 0) {                                              /* 未接收socket数据 */
            if (s_rcb.at_rlen >= sizeof(s_rcb.at_rbuf)) {
                s_rcb.prechar = 0;
                s_rcb.at_rlen = 0;
            }
            s_rcb.at_rbuf[s_rcb.at_rlen++] = curchar;
        } else {                                                               /* 接收socket数据 */
            if (s_rcb.ip_rlen >= sizeof(s_rcb.at_rbuf)) {
                s_rcb.ip_rlen++;
            } else {
                s_rcb.at_rbuf[s_rcb.ip_rlen++] = curchar;
            }
        }

        if (s_rcb.ip_rlen == 0) {
            if (s_rcb.prechar == 'V' && curchar =='E' && s_rcb.at_rlen >= 8) { /* 检测IP数据头 */
                s_rcb.prechar = curchar;
                if (YX_SearchKeyWord(&s_rcb.at_rbuf[s_rcb.at_rlen - 8], 8, "+RECEIVE")) {
                    s_rcb.at_rlen = 0;
                    YX_MEMCPY(s_rcb.at_rbuf, sizeof(s_rcb.at_rbuf), "+RECEIVE", 8);
                    s_rcb.ip_leftlen = 0;
                    s_rcb.ip_rlen    = 8;
                    continue;
                }
            }
        } else {
            if (s_rcb.ip_leftlen == 0) {
                if (s_rcb.ip_rlen > 16) {
                    s_rcb.ip_rlen = 0;
                    continue;
                }
                
                if (curchar == ':' || curchar == 0x0D) {                       /* 计算接收的IP数据长度 */
                    if (curchar == ':') {
                        s_rcb.ip_leftlen = YX_SearchDigitalString(s_rcb.at_rbuf, s_rcb.ip_rlen, ':', 1);
                    } else {
                        s_rcb.ip_leftlen = YX_SearchDigitalString(s_rcb.at_rbuf, s_rcb.ip_rlen, 0x0D, 1);
                    }
                    if (s_rcb.ip_leftlen == 0 || s_rcb.ip_leftlen > 2048) {
                        s_rcb.ip_rlen = 0;
                        continue;
                    }
                    
                    s_rcb.socket = YX_SearchDigitalString(s_rcb.at_rbuf, s_rcb.ip_rlen, ',', 2);
                    if (s_rcb.socket >= SOCKET_CH_MAX) {
                        s_rcb.ip_rlen = 0;
                        continue;
                    }
                    if (curchar == ':') {
                        s_rcb.ip_leftlen += (s_rcb.ip_rlen + 2);
                    } else {
                        s_rcb.ip_leftlen += (s_rcb.ip_rlen + 1);
                    }
                }
            } else {
                if (s_rcb.ip_rlen >= s_rcb.ip_leftlen) {
                    if (s_rcb.ip_rlen > sizeof(s_rcb.at_rbuf)) {
                        s_rcb.ip_leftlen = sizeof(s_rcb.at_rbuf);
                        
                        #if DEBUG_AT > 0
                        printf_com("<IP数据发生了丢失(%d)------->\r\n", (INT16U)(s_rcb.ip_rlen - sizeof(s_rcb.at_rbuf)));
                        #endif
                    }
                    
                    #if DEBUG_AT > 0
                    printf_com("<收到完整IP数据包(%d)(%d):", s_rcb.socket, s_rcb.ip_leftlen);
                    printf_hex(s_rcb.at_rbuf, (s_rcb.ip_leftlen > 64) ? 64 : s_rcb.ip_leftlen);
                    printf_com(">\r\n");
                    #endif
                    
                    HdlMsg_ATCMD_RECV(s_rcb.at_rbuf, s_rcb.ip_leftlen);            /* 处理接收到的IP数据 */
                    s_rcb.ip_rlen  = 0;
                }
            }
            continue;
        }

        #if DEBUG_AT > 0
        if (curchar == '\r' && (s_rcb.at_rlen > 1 && s_rcb.prechar != '\r')) {
            printf_com("AT_RECV(%d):", s_rcb.at_rlen);
            printf_raw(s_rcb.at_rbuf, s_rcb.at_rlen);
        }
        #endif
                
        if (curchar == '\r' && s_rcb.at_rlen >= 10 && s_rcb.at_rbuf[6] == 'S' && s_rcb.at_rbuf[9] == 'D') {
            #if DEBUG_AT > 0
            printf_com("\n");
            #endif
            
            /* handle one at command */
            HdlMsg_ATCMD_RECV(s_rcb.at_rbuf, s_rcb.at_rlen);
            s_rcb.prechar   = 0;
            s_rcb.at_rlen = 0;
        } else if (s_rcb.prechar == '\r' && curchar == '\n') {
            #if DEBUG_AT > 0
            printf_com("\n");

            if (!((s_rcb.at_rbuf[0] >= 'A' && s_rcb.at_rbuf[0] <= 'Z') || (s_rcb.at_rbuf[0] >= '0' && s_rcb.at_rbuf[0] <= '9') || s_rcb.at_rbuf[0] == '+') && s_rcb.at_rlen >= 4) {
                printf_hex(s_rcb.at_rbuf, s_rcb.at_rlen);
                printf_com(">\r\n");
            }
            #endif
            
            /* handle one at command */
            HdlMsg_ATCMD_RECV(s_rcb.at_rbuf, s_rcb.at_rlen);
            s_rcb.prechar   = 0;
            s_rcb.at_rlen = 0;
        } else {
            s_rcb.prechar   = curchar;
        }
    }
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
    YX_MEMSET(&s_hdl_tbl, 0, sizeof(s_hdl_tbl));
    
    s_overtmr = OS_CreateTmr(TSK_ID_DAL, 0, OverTmrProc);
    s_scantmr = OS_CreateTmr(TSK_ID_DAL, 0, ScanTmrProc);
    //OS_StartTmr(s_scantmr, PERIOD_SCAN);
    
    //OS_InstallDiag(DiagnoseProc_ATRECV);
}

/*******************************************************************
** 函数名:     AT_RECV_Open
** 函数描述:   打开模块数据接收解析功能
** 参数:       无
** 返回:       无
********************************************************************/
void AT_RECV_Open(void)
{
    if ((s_rcb.status & _OPEN) == 0) {
        s_rcb.status = _OPEN;

        OS_StartTmr(s_scantmr, PERIOD_SCAN);
    }
}

/*******************************************************************
** 函数名:     AT_RECV_Close
** 函数描述:   关闭模块数据接收解析功能
** 参数:       无
** 返回:       无
********************************************************************/
void AT_RECV_Close(void)
{
    if ((s_rcb.status & _OPEN) != 0) {
        s_rcb.status = 0;
        
        OS_StopTmr(s_scantmr);
        AT_RECV_Reset();
    }
}

/*******************************************************************
** 函数名:     AT_RECV_Reset
** 函数描述:   复位接收状态
** 参数:       无
** 返回:       无
********************************************************************/
void AT_RECV_Reset(void)
{
    if (s_rcb.handler != 0) {
        s_rcb.handler(s_rcb.ct_recv, EVT_RESET, 0, 0);
    }
    DestroyRCB();
}

/*******************************************************************
** 函数名:     AT_RECV_RegistUrcHandler
** 函数描述:   注册URC消息接收处理器
** 参数:       [in] type:   协议类型
**             [in] handler:协议处理函数
** 返回:       注册成功返回true，注册失败返回false
********************************************************************/
BOOLEAN AT_RECV_RegistUrcHandler(URC_HDL_TBL_T *tbl)
{
    INT8U i;
    
    for (i = 0; i < MAX_REG; i++) {
        if (s_hdl_tbl[i] == 0) {
            break;
        }
    }
    
    OS_ASSERT((tbl != 0), RETURN_FALSE);                                       /* 注册的处理函数不能为空 */
    OS_ASSERT((i < MAX_REG), RETURN_FALSE);                                    /* 注册表已满则出错 */
    
    s_hdl_tbl[i] = tbl;
    s_rcb.ct_regist++;
    return true;
}


#endif
