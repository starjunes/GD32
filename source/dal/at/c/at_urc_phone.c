/********************************************************************************
**
** 文件名:     at_urc_phone.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现手机模块电话消息接收解析处理
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
#include "at_urc_phone.h"


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
    INT8U       ct_clip;
    AT_PHONE_CALLBACK_T callback_phone;
} RCB_T;

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static RCB_T s_rcb;


/*
********************************************************************************
* HANDLER: NO CARRIER
********************************************************************************
*/
static INT8U Handler_NOCARRIER(INT8U ct_recv, INT8U event, INT8U *sptr, INT16U slen)
{
    s_rcb.ct_clip = 0;
	if (s_rcb.callback_phone.callback_disconnect != 0) {
	    s_rcb.callback_phone.callback_disconnect(0);
    }
    
	return AT_SUCCESS;
}

/*
********************************************************************************
* HANDLER: RING
********************************************************************************
*/
static INT8U Handler_RING(INT8U ct_recv, INT8U event, INT8U *sptr, INT16U slen)
{
	return AT_SUCCESS;
}

/*
********************************************************************************
* HANDLER: +CLIP
********************************************************************************
*/
static INT8U Handler_CLIP(INT8U ct_recv, INT8U event, INT8U *sptr, INT16U slen)
{
    INT8U tellen;
    INT8U tel[31];
    
    if (event == EVT_NORMAL) {
        tellen = YX_SearchString(tel, sizeof(tel) - 1, sptr, slen, '"', 1);
        tel[tellen] = '\0';
        
        if (tellen > 0) {
            s_rcb.ct_clip = 0;
            if (s_rcb.callback_phone.callback_ring != 0) {
	            s_rcb.callback_phone.callback_ring(0, (char *)tel);
            }
        } else {
            if (++s_rcb.ct_clip >= 3) {
                s_rcb.ct_clip = 0;
                if (s_rcb.callback_phone.callback_ring != 0) {
	                s_rcb.callback_phone.callback_ring(0, (char *)tel);
                }
            }
        }
                
    }
    return AT_SUCCESS;
}

/*
********************************************************************************
* HANDLER: +CLCC
********************************************************************************
*/
static INT8U Handler_CLCC(INT8U ct_recv, INT8U event, INT8U *sptr, INT16U slen)
{
    INT8U id, dir, state;
    
    id    = YX_SearchDigitalString(sptr, 30, ',', 1);
    dir   = YX_SearchDigitalString(sptr, 30, ',', 2);
    state = YX_SearchDigitalString(sptr, 30, ',', 3);
    
    if (dir != 0) {                                                            /* 非拨打电话 */
        return AT_SUCCESS;
    }
    
    switch (state)
    {
    case 0:
        if (s_rcb.callback_phone.callback_connect != 0) {
	        s_rcb.callback_phone.callback_connect((void *)id);
        }
        break;
    case 1:
        break;
    case 2:
        break;
    case 3:
        break;
    case 4:
        break;
    case 5:
        break;
    case 6:
        break;
    default:
        break;
    }
    
    return AT_SUCCESS;
}

/*
********************************************************************************
* define receive control block
********************************************************************************
*/
static URC_HDL_TBL_T const s_hdl_tbl[] = {
                   {"PB DONE",              2,   true,   0}
                  ,{"NO CARRIER",           2,   true,   Handler_NOCARRIER}
                  ,{"BUSY",                 2,   true,   Handler_NOCARRIER}
                  ,{"NO ANSWER",            2,   true,   Handler_NOCARRIER}
                  ,{"NO DIALTONE",          2,   true,   Handler_NOCARRIER}
                  ,{"RING",                 2,   true,   Handler_RING}
                  ,{"+CLIP:",               2,   true,   Handler_CLIP}
                  ,{"+COLP:",               2,   true,   0}
                  ,{"+CLCC:",               2,   true,   Handler_CLCC}

                                     };


/*******************************************************************
** 函数名:     AT_URC_InitPhone
** 函数描述:   初始化函数
** 参数:       无
** 返回:       无
********************************************************************/
void AT_URC_InitPhone(void)
{
    INT8U i;
    
    YX_MEMSET(&s_rcb, 0, sizeof(s_rcb));
    
    for (i = 0; i < sizeof(s_hdl_tbl) / sizeof(s_hdl_tbl[0]); i++) {
        AT_RECV_RegistUrcHandler((URC_HDL_TBL_T *)&s_hdl_tbl[i]);
    }
}

/*******************************************************************
** 函数名:     AT_URC_RegistPhoneHandler
** 函数描述:   注册电话消息接收处理器
** 参数:       [in] callback:   消息处理器
** 返回:       注册成功返回true，注册失败返回false
********************************************************************/
BOOLEAN AT_URC_RegistPhoneHandler(AT_PHONE_CALLBACK_T *callback)
{
    YX_MEMCPY(&s_rcb.callback_phone, sizeof(s_rcb.callback_phone), callback, sizeof(AT_PHONE_CALLBACK_T));
    return true;
}

#endif
