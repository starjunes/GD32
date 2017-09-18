/********************************************************************************
**
** 文件名:     yx_jt2_trecv.c
** 版权所有:   (c) 2005-2012 厦门雅迅网络股份有限公司
** 文件描述:   该模块实现交通部协议第一服务器TCP数据接收
** 
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2013/10/21 | 叶德焰 |  创建第一版本
*********************************************************************************/
#define GLOBALS_JT2_TRECV     1

#include "yx_include.h"
#include "at_drv.h"
#include "at_gprs_drv.h"
#include "yx_protocol_type.h"
#include "yx_protocol_recv.h"
#include "yx_jt2_tlink.h"

/*
********************************************************************************
* 定义配置参数
********************************************************************************
*/
#define COM_TCP              SOCKET_CH_1
#define SIZE_HDL             64
#define PERIOD_SCAN          MILTICK,  1

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    INT16U rlen;
    INT8U rbuf[SIZE_HDL];
} RCB_T;

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
//static INT8U s_recvtmr;
static RCB_T s_rcb;


/*******************************************************************
** 函数名:     HdlGprsFrame
** 函数描述:   处理GPRS协议帧
** 参数:       [in] sptr: 数据指针
**             [in] slen: 数据长度
** 返回:       无
********************************************************************/
static void HdlGprsFrame(INT8U *sptr, INT32U slen)
{
    PROTOCOL_COM_T attrib;
    
    YX_MEMSET(&attrib, 0, sizeof(attrib));
    //attrib.attrib  = SM_ATTR_TCP;                                              /* 发送属性 */
    attrib.channel = COM_TCP;                                                  /* 发送通道 */
    attrib.type    = PTOTOCOL_TYPE_DATA;                                       /* 协议数据类型 */
    
    if (!YX_PROTOCOL_HandleFrame(&attrib, sptr, slen)) {
        #if DEBUG_TLINK > 0
        printf_com("<yx_jt2_trecv, sorry, protocol check error>\r\n");
        #endif
    }
}

#if 0

/*******************************************************************
** 函数名:     RecvTmrProc
** 函数描述:   定时器处理函数
** 参数:       [in] pdata:定时器特征值
** 返回:       无
********************************************************************/
static void RecvTmrProc(void *pdata)
{
    INT8U ch;
    INT16S recv;
    
    pdata = pdata;
    YX_StartTmr(s_recvtmr, PERIOD_SCAN);
    
    if (!YX_JTT2_LinkIsConnect()) {
        s_rcb.rlen = 0;
        return;
    }
    
    for (;;) {
        if ((recv = DAL_TCP_Read(COM_TCP)) == -1) {
            break;
        }
        ch = recv;

        if (s_rcb.rlen == 0) {
            if (ch == g_jtt2_rules.c_flags) {                                    /* 检测协议头 */
                s_rcb.rbuf[0] = 0;
                s_rcb.rlen++;
            }
        } else {
            if (ch == g_jtt2_rules.c_flags) {
                if (s_rcb.rlen > 1) {
                    s_rcb.rlen = YX_DeassembleByRules(s_rcb.rbuf, &s_rcb.rbuf[1], s_rcb.rlen - 1, (ASMRULE_T *)&g_jtt2_rules);
                    if (s_rcb.rlen >= 2) {
                        HdlGprsFrame(s_rcb.rbuf, s_rcb.rlen);
                    }
                    s_rcb.rlen = 0;
                }
            } else {
                if (s_rcb.rlen >= SIZE_HDL) {
                    s_rcb.rlen = 0;
                } else {
                    s_rcb.rbuf[s_rcb.rlen++] = ch;
                }
            }
        }
    }
}

/*******************************************************************
** 函数名:     DiagnoseProc
** 函数描述:   诊断函数
** 参数:       无
** 返回:       无
********************************************************************/
static void DiagnoseProc(void)
{
    OS_ASSERT(YX_TmrIsRun(s_recvtmr), RETURN_VOID);
}

#endif

/*******************************************************************
** 函数名:     YX_JTT2_InitRecv
** 函数描述:   初始化接收模块
** 参数:       无
** 返回:       无
********************************************************************/
void YX_JTT2_InitRecv(void)
{
    s_rcb.rlen = 0;

    //s_recvtmr = YX_InstallTmr(PRIO_OPTTASK, (void *)0, RecvTmrProc);
    //YX_StartTmr(s_recvtmr, PERIOD_SCAN);
    
    //OS_InstallDiag(DiagnoseProc);
}

/*******************************************************************
** 函数名:     YX_JTT2_RecvData
** 函数描述:   接收解析数据
** 参数:       [in] socketid: socket编号 
** 返回:       无
********************************************************************/
void YX_JTT2_RecvData(INT8U socketid, INT8U *sptr, INT32U slen)
{
    INT8U ch;
    INT32U i;
    
    if (socketid != COM_TCP) {
        return;
    }
    
    if (!YX_JTT2_LinkIsConnect()) {
        s_rcb.rlen = 0;
        return;
    }
    
    for (i = 0; i < slen; i++) {
        ch = sptr[i];

        if (s_rcb.rlen == 0) {
            if (ch == g_jtt2_rules.c_flags) {                                    /* 检测协议头 */
                s_rcb.rbuf[0] = 0;
                s_rcb.rlen++;
            }
        } else {
            if (ch == g_jtt2_rules.c_flags) {
                if (s_rcb.rlen > 1) {
                    s_rcb.rlen = YX_DeassembleByRules(s_rcb.rbuf, sizeof(s_rcb.rbuf), &s_rcb.rbuf[1], s_rcb.rlen - 1, &g_jtt2_rules);
                    if (s_rcb.rlen >= 2) {
                        HdlGprsFrame(s_rcb.rbuf, s_rcb.rlen);
                    }
                    s_rcb.rlen = 0;
                }
            } else {
                if (s_rcb.rlen >= SIZE_HDL) {
                    s_rcb.rlen = 0;
                } else {
                    s_rcb.rbuf[s_rcb.rlen++] = ch;
                }
            }
        }
    }
}


