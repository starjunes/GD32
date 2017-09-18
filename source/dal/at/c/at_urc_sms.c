/********************************************************************************
**
** 文件名:     at_urc_sms.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现手机模块短消息数据接收解析处理
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
#include "at_com.h"
#include "at_recv.h"
#include "at_send.h"
#include "at_pdu.h"
#include "at_urc_sms.h"

#if EN_AT > 0

/*
********************************************************************************
* 定义模块配置参数
********************************************************************************
*/



/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    INT8U       pdulen;
    AT_SMS_CALLBACK_T callback_sms;
} RCB_T;

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static RCB_T s_rcb;


/*
********************************************************************************
* HANDLER: 短消息内容
********************************************************************************
*/
static INT8U Handler_CMT(INT8U ct_recv, INT8U event, INT8U *sptr, INT16U slen)
{
    SM_T *sm;
    
    if (event == EVT_RESET) {
        return AT_SUCCESS;
    }
    
    if (event == EVT_OVERTIME) {
        return AT_SUCCESS;
    }

    if (ct_recv == 0) {
        s_rcb.pdulen = YX_SearchDigitalString(sptr, slen, '\r', 1);
        return AT_CONTINUE;
    } else if (ct_recv == 1) {
        sm = (SM_T *)YX_DYM_Alloc(sizeof(SM_T));
        if (sm != 0) {
            if (ParsePDUData(sm, sptr, slen, s_rcb.pdulen)) {
                if (s_rcb.callback_sms.callback_recvsm != 0) {
                    s_rcb.callback_sms.callback_recvsm(sm);
                }
            } else {
                #if DEBUG_AT > 0
                printf_com("<receive error SM>\r\n");
                #endif
            }
            YX_DYM_Free(sm);
        }
        
        return AT_SUCCESS;
    } else {
        return AT_SUCCESS;
    }
}

/*
********************************************************************************
* HANDLER: 短消息索引号通知
********************************************************************************
*/
static INT8U Handler_CMTI(INT8U ct_recv, INT8U event, INT8U *sptr, INT16U slen)
{
    INT16U index;
    
    index = YX_SearchDigitalString(sptr, slen, ',', 1);
    if (index != 0xffff) {
        if (s_rcb.callback_sms.callback_recvsmindex != 0) {
            s_rcb.callback_sms.callback_recvsmindex(index);
        }
    }
    
    #if DEBUG_AT > 0
    printf_com("<Handler_CMTI(%d)>\r\n", index);
    #endif
    
    return AT_SUCCESS;
}

/*
********************************************************************************
* HANDLER: 读取短消息索引号
********************************************************************************
*/
static INT8U Handler_CMGL(INT8U ct_recv, INT8U event, INT8U *sptr, INT16U slen)
{
    INT16U index;
    
    if (ct_recv == 0) {
        index = YX_SearchDigitalString(sptr, slen, ',', 1);
        if (index != 0xffff) {
            if (s_rcb.callback_sms.callback_recvsmindex != 0) {
                s_rcb.callback_sms.callback_recvsmindex(index);
            }
        }
        
        #if DEBUG_AT > 0
        printf_com("<Handler_CMGL(%d)>\r\n", index);
        #endif
        
        return AT_CONTINUE;
    } else {
        return AT_SUCCESS;
    }
}


/*
********************************************************************************
* define receive control block
********************************************************************************
*/
static URC_HDL_TBL_T const s_hdl_tbl[] = {
                   {"SMS DONE",             2,   true,   0}
                  ,{"+CMGS:",               2,   true,   0}
                  ,{"+CMT:",                4,   true,   Handler_CMT}
                  ,{"+CMTI:",               2,   true,   Handler_CMTI}
                  ,{"+CMGL:",               2,   true,   Handler_CMGL}
                  ,{"+CMGR:",               2,   true,   Handler_CMT}
                                     };




/*******************************************************************
** 函数名:     AT_URC_InitSms
** 函数描述:   初始化函数
** 参数:       无
** 返回:       无
********************************************************************/
void AT_URC_InitSms(void)
{
    INT8U i;
    
    YX_MEMSET(&s_rcb, 0, sizeof(s_rcb));
    
    for (i = 0; i < sizeof(s_hdl_tbl) / sizeof(s_hdl_tbl[0]); i++) {
        AT_RECV_RegistUrcHandler((URC_HDL_TBL_T *)&s_hdl_tbl[i]);
    }
}

/*******************************************************************
** 函数名:     AT_URC_RegistSmsHandler
** 函数描述:   注册短消息接收处理器
** 参数:       [in] callback:   消息处理器
** 返回:       注册成功返回true，注册失败返回false
********************************************************************/
BOOLEAN AT_URC_RegistSmsHandler(AT_SMS_CALLBACK_T *callback)
{
    YX_MEMCPY(&s_rcb.callback_sms, sizeof(s_rcb.callback_sms), callback, sizeof(AT_SMS_CALLBACK_T));
    return true;
}

#endif

