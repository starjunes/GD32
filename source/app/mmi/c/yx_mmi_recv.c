/********************************************************************************
**
** 文件名:     yx_mmi_recv.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现外设数据接收处理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2011/01/22 | 叶德焰 |  创建第一版本
*********************************************************************************/
#define GLOBALS_MMI_RECV    1
#include "yx_include.h"
#include "yx_debug.h"

#if EN_MMI > 0
#include "yx_mmi_drv.h"


/*
********************************************************************************
* 宏定义
********************************************************************************
*/
#define MAX_REG              45
#define PE_TYPE_DEF          PE_TYPE_YXMMI

#define PERIOD_SCAN          _TICK, 1

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct protocolreg {
    struct protocolreg  *next;
    INT8U type;
    void  (*c_handler)(INT8U type, INT8U *ptr, INT16U len);
    void  (*b_handler)(INT8U type, INT8U *ptr, INT16U len);
} PROTOCOL_REG_T;

typedef struct {
    INT16U rlen;
    INT8U rbuf[MMI_SIZE_HDL];
} RCB_T;

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static INT8U s_scantmr;
static RCB_T s_rcb;
static PROTOCOL_REG_T s_reg_tbl[MAX_REG];
static PROTOCOL_REG_T *s_usedlist, *s_freelist;


/*******************************************************************
** 函数名:     HdlRecvData
** 函数描述:   MMI调度屏协议处理
** 参数:       无
** 返回:       无
********************************************************************/
static void HdlRecvData(INT8U *ptr, INT16U len)
{
    INT8U type;
    PROTOCOL_REG_T *curptr;
    
    type = *(ptr + 3);
    curptr = s_usedlist;

    while (curptr != 0) {
        if (curptr->type == type) {                                            /* 搜索对应类型 */
            OS_ASSERT((curptr->c_handler != 0), RETURN_VOID);                  /* 注册的处理函数不能为空 */
            OS_ASSERT((curptr->c_handler == curptr->b_handler), RETURN_VOID);  /* 注册的处理函数有效 */
            
            curptr->c_handler(type, ptr + 4, len - 4);                         /* 去掉校验码和协议字段 */
            break;
        } else {
            curptr = curptr->next;
        }
    }
}

/*******************************************************************
** 函数名:     ScanTmrProc
** 函数描述:   扫描定时器
** 参数:       [in] pdata:定时器特征值
** 返回:       无
********************************************************************/
static void ScanTmrProc(void *pdata)
{
    INT8U ch;
    INT16S recv;
    
    OS_StartTmr(s_scantmr, PERIOD_SCAN);

    for (;;) {
        if ((recv = YX_MMI_ReadByte()) == -1) {
            break;
        }
        
        ch = recv;
        //printf_com(" 0x%02x", ch);

        if (s_rcb.rlen == 0) {
            if (ch == g_dvr_rules.c_flags) {                                  /* 检测协议头 */
                s_rcb.rbuf[0] = 0;
                s_rcb.rlen++;
            }
        } else {
            if (ch == g_dvr_rules.c_flags) {
                if (s_rcb.rlen > 1) {
                    s_rcb.rlen = YX_DeassembleByRules(s_rcb.rbuf, sizeof(s_rcb.rbuf), &s_rcb.rbuf[1], s_rcb.rlen - 1, &g_dvr_rules);
                
                    if (s_rcb.rlen >= 4) {
                        if (s_rcb.rbuf[2] == PE_TYPE_DEF) {                    /* 先判断外设类型 */
                            if (s_rcb.rbuf[0] == YX_GetChkSum(&s_rcb.rbuf[1], s_rcb.rlen - 1)) { /* 计算校验和 */
                                HdlRecvData(s_rcb.rbuf, s_rcb.rlen);
                            }
                        }
                    }
                    s_rcb.rlen = 0;
                }
            } else {
                if (s_rcb.rlen >= MMI_SIZE_HDL) {
                    s_rcb.rlen = 0;
                } else {
                    s_rcb.rbuf[s_rcb.rlen++] = ch;
                }
            }
        }
    }
}

/*******************************************************************
** 函数名:     YX_MMI_InitRecv
** 函数描述:   外设接收模块初始化
** 参数:       无
** 返回:       无
********************************************************************/
void YX_MMI_InitRecv(void)
{
    INT8U i;
    
    YX_MEMSET(&s_rcb, 0, sizeof(s_rcb));
    
    for (i = 0; i < MAX_REG - 1; i++) {
        s_reg_tbl[i].next = &s_reg_tbl[i + 1];
        s_reg_tbl[i].type = 0;
        s_reg_tbl[i].c_handler = 0;
        s_reg_tbl[i].b_handler = 0;
    }
    s_reg_tbl[i].next      = 0;
    s_reg_tbl[i].type      = 0;
    s_reg_tbl[i].c_handler = 0;
    s_reg_tbl[i].b_handler = 0;
    
    s_freelist             = &s_reg_tbl[0];
    s_usedlist             = 0;
    
    s_scantmr = OS_CreateTmr(TSK_ID_APP, (void *)0, ScanTmrProc);
    OS_StartTmr(s_scantmr, PERIOD_SCAN);
}

/*******************************************************************
** 函数名:     YX_MMI_Register
** 函数描述:   外设接收处理注册
** 参数:       [in] type:   协议类型
**             [in] handler:协议处理函数
** 返回:       注册成功返回true，注册失败返回false
********************************************************************/
BOOLEAN YX_MMI_Register(INT8U type, void (*handler)(INT8U type, INT8U *ptr, INT16U len))
{
    PROTOCOL_REG_T *curptr;

    curptr = s_usedlist;
    while (curptr != 0) {                                                      /* 不能重复注册 */
        OS_ASSERT((curptr->type != type), RETURN_FALSE);
        curptr = curptr->next;
    }
    
    OS_ASSERT(handler != 0, RETURN_FALSE);                                     /* 注册的处理函数不能为空 */
    OS_ASSERT(s_freelist != 0, RETURN_FALSE);                                  /* 注册表已满则出错 */
    
    curptr = s_freelist;                                                       /* 新注册 */
    if (curptr != 0) {
        s_freelist        = curptr->next;
        curptr->next      = s_usedlist;
        s_usedlist        = curptr;
        curptr->type      = type;
        curptr->c_handler = handler;                                           /* 处理函数 */
        curptr->b_handler = handler;
        return true;
    } else {
        return false;
    }
}

#endif
