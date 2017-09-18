/********************************************************************************
**
** 文件名:     yx_jt2_urecv.c
** 版权所有:   (c) 2005-2012 厦门雅迅网络股份有限公司
** 文件描述:   该模块实现交通部协议第二服务器UDP数据接收
** 
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2013/10/21 | 叶德焰 |  创建第一版本
*********************************************************************************/
#define GLOBALS_JT2_URECV     1

#include "yx_includes.h"
#include "yx_timer.h"
#include "yx_diagnose.h"
#include "yx_tools.h"
#include "yx_roundbuf.h"
#include "yx_dm.h"
#include "dal_gprs_drv.h"
#include "dal_sm_drv.h"
#include "yx_protocol_recv.h"
#include "yx_protocol_send.h"
#include "yx_jt_linkman.h"
#include "yx_debug.h"

/*
********************************************************************************
* 定义配置参数
********************************************************************************
*/
#define COM_UDP              CHA_JT2_ULINK      
#define SIZE_HDL             1600
#define PERIOD_SCAN          MILTICK,  1,  LOW_PRECISION_TIMER

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
static INT8U s_recvtmr;
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
    attrib.attrib  = SM_ATTR_UDP;                                              /* 发送属性 */
    attrib.channel = COM_UDP;                                                  /* 发送通道 */
    attrib.type    = PTOTOCOL_TYPE_DATA;                                       /* 协议数据类型 */
    
    if (!YX_PROTOCOL_HandleFrame(&attrib, sptr, slen)) {
        #if DEBUG_ULINK > 0
        printf_com("<yx_jt2_urecv, sorry, protocol check error>\r\n");
        #endif
    }
}

/*******************************************************************
** 函数名:     RecvTmrProc
** 函数描述:   定时器处理函数
** 参数:       [in] pdata:定时器特征值
** 返回:       无
********************************************************************/
static void RecvTmrProc(void *pdata)
{
    NETDATA_T *cur;
    
    YX_StartTmr(s_recvtmr, PERIOD_SCAN);
    
    if (!YX_JTU1_LinkIsConnect()) {
        return;
    }
    
    for (;;) {
        cur = DAL_UDP_ReadData(COM_UDP);
        if (cur == 0) {
            break;
        }
        
        cur->len = YX_DeassembleByRules(cur->data, (INT8U *)&cur->data[1], cur->len - 2, (ASMRULE_T *)&g_jtu1_rules);        
        HdlGprsFrame(cur->data, cur->len);
        YX_DYM_Free(cur->data);
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

/*******************************************************************
** 函数名:     YX_JTU2_InitRecv
** 函数描述:   初始化 UDP接收模块
** 参数:       无
** 返回:       无
********************************************************************/
void YX_JTU2_InitRecv(void)
{
    s_rcb.rlen = 0;

    s_recvtmr = YX_InstallTmr(PRIO_OPTTASK, (void *)0, RecvTmrProc);
    YX_StartTmr(s_recvtmr, PERIOD_SCAN);
    
    OS_InstallDiag(DiagnoseProc);
}