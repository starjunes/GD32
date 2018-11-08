/******************************************************************************
**
** Filename:     yx_jieyou_drv.c
** Copyright:    
** Description:  该模块主要实现检测节油产品驱动
**
-------------------------------------------------------------------------------
**             Revision history
**-----------------------------------------------------------------------------
**| 2018/07/10 | nhong |  创建文件
*******************************************************************************/
#include "yx_include.h"
#include "dal_pp_drv.h"
#include "st_gpio_drv.h"
#include "st_adc_drv.h"
#include "hal_can_drv.h"
#include "yx_jieyou_cancheck.h"
#include "yx_debug.h"
#include "yx_jieyou_drv.h"

/* can滤波信息 */
typedef struct {
    INT8U isvaild;      /* 是否有效 */
    INT8U filtertype;   /* 滤波类型 */
    INT8U idtype;       /* id类型 */
    INT8U idnum;        /* id数量 */
    union {
        struct {
            INT32U canid[56];
        } list;

        struct {
            INT32U id[14];
            INT32U mask[14];
        } mask;
    }filter;    
} CAN_FILTER_INFO_T;


typedef struct {
    INT8U   com;            /* 确认后的can通道 */
    INT8U isconfirm;
    INT16U checkcnt;
    INT8U isrec[CAN_COM_MAX];
    CAN_FILTER_INFO_T can_filter;
}TCB_T;

#define CAN_FILTER_MASK 1
#define CAN_FILTER_LIST 2

#define CAN2_POWER_IO GPIO_PIN_B15


static INT8U s_tmr_scan;
static TCB_T s_tcb;

/*******************************************************************
** 函数名:     ScanTmrProc
** 函数描述:   定时器处理
** 参数:       [in] pdata：定时器特征值
** 返回:       无
********************************************************************/
static void ScanTmrProc(void *pdata)
{
    INT32U canid = 0xffffffff;
    INT8U com = 0xff;
    INT8U i;
    
    for(i = 0; i < CAN_COM_MAX; i++) {
        if(s_tcb.isrec[i]) {
            com = i;
            /* 搜索到一个直接退出,这样can0就会优先 */
            break;
        }
    }

    /* 已经有通道接收到数据,则停止检测 */
    if(com != 0xff) {
        
        /* 保持确认后的can通道 */
        s_tcb.com = com;
        
        s_tcb.isconfirm = TRUE;
        YX_JieYou_StopCanCheck();
        OS_StopTmr(s_tmr_scan);

        if(com == CAN_COM_0) {
            ST_GPIO_WritePin(CAN2_POWER_IO, FALSE);
        }else if(com == CAN_COM_1) {
            /* CAN_COM_0通道不能关闭,关闭后CAN_COM_1也无法收到数据 */
            HAL_CAN_SetFilterParaByList(CAN_COM_0, CAN_ID_TYPE_EXT, 1, &canid);
        }

        
        
        if(s_tcb.can_filter.isvaild) {
            if(s_tcb.can_filter.filtertype == CAN_FILTER_MASK) {
                HAL_CAN_SetFilterParaByMask(com
                                          , s_tcb.can_filter.idtype
                                          , s_tcb.can_filter.idnum
                                          , s_tcb.can_filter.filter.mask.id
                                          , s_tcb.can_filter.filter.mask.mask);
            } else if(s_tcb.can_filter.filtertype == CAN_FILTER_LIST) {
                HAL_CAN_SetFilterParaByList(com
                                          , s_tcb.can_filter.idtype
                                          , s_tcb.can_filter.idnum
                                          , s_tcb.can_filter.filter.list.canid);
            }
            
        }

        #if EN_DEBUG > 0
        printf_com("确认can通道为:%d\r\n", com);
        #endif
    } 

    /* 10s都没检测到can数据,则开启can2检测 */
    if(s_tcb.checkcnt < 10) {
        s_tcb.checkcnt++;
    } else if((s_tcb.checkcnt == 10) && (com == 0xff)) {
        #if EN_DEBUG > 0
        printf_com("超时未收到can数据,进行can2检测\r\n");
        #endif
        YX_JieYou_StartCanCheck();
        s_tcb.checkcnt++;
    }
}

/*******************************************************************
** 函数名:     YX_JieYou_InitCanCheck
** 函数描述:   节油can检测初始化
** 参数:       无
** 返回:       无
********************************************************************/
void YX_JieYou_InitDrv(void)
{
    CAN_CFG_T can;
    CAN_CFG_INFO_T can_pp;
    CAN_ADAPTER_INTO_T can_adapter;
    
    YX_MEMSET(&s_tcb, 0, sizeof(s_tcb));

    YX_JieYou_InitCanCheck();
    YX_JieYou_InitADCheck();
       
    if(!DAL_PP_ReadParaByID(PP_ID_CAN_CFG, (INT8U *)&can_pp, sizeof(can_pp))) {
        YX_MEMSET(&can_pp, 0, sizeof(can_pp));
    }

    if(!DAL_PP_ReadParaByID(PP_ID_CAN_ADAPTER, (INT8U *)&can_adapter, sizeof(can_adapter))) {
        YX_MEMSET(&can_adapter, 0, sizeof(can_adapter));
    }

    if(can_pp.isvaild) {
        can.baud    = can_pp.baud;        
        can.idtype  = can_pp.idtype;
        can.mode    = can_pp.mode;
    } else {
        can.baud    = 250000;        
        can.idtype  = CAN_ID_TYPE_EXT;
        can.mode    = CAN_WORK_MODE_NORMAL;
    }

    can.com = CAN_COM_0;
    if(HAL_CAN_OpenCan(&can)) {
        #if EN_DEBUG > 0
        printf_com("can1打开成功\r\n");
        #endif
    } else {
        #if EN_DEBUG > 0
        printf_com("can1打开失败\r\n");
        #endif
    }

    can.com = CAN_COM_1;
    
    if(HAL_CAN_OpenCan(&can)) {
        #if EN_DEBUG > 0
        printf_com("can2打开成功\r\n");
        #endif
    } else {
        #if EN_DEBUG > 0
        printf_com("can2打开失败\r\n");
        #endif
    }

    HAL_CAN_SetFilterParaByMask(CAN_COM_0, CAN_ID_TYPE_EXT, 0, 0, 0);
    HAL_CAN_SetFilterParaByMask(CAN_COM_1, CAN_ID_TYPE_EXT, 0, 0, 0);

    if(can_adapter.cam2_low!=0 && can_adapter.can2_high != 0) {
        YX_JieYou_SetCanHighIo(can_adapter.can2_high);
        YX_JieYou_SetCanLowIo(can_adapter.cam2_low);
        #if EN_DEBUG > 0
        printf_com("当前can2有效 高:%d 低:%d\r\n", can_adapter.can2_high, can_adapter.cam2_low);
        #endif
    }
    
    s_tmr_scan = OS_CreateTmr(TSK_ID_APP, (void *)0, ScanTmrProc); 
    OS_StartTmr(s_tmr_scan, _SECOND, 1);
    
}

/*******************************************************************
** 函数名:     YX_JieYou_Confirm
** 函数描述:   确认收到can消息
** 参数:       无
** 返回:       无
********************************************************************/
INT8U YX_JieYou_Confirm(INT8U com, CAN_DATA_T * candata)
{
    if(com >= CAN_COM_MAX) {
        return FALSE;
    }
    
    s_tcb.isrec[com] = TRUE;

    return TRUE;
}

/*******************************************************************
** 函数名:     YX_JieYou_IsConfirm
** 函数描述:   can通道是否已经确认
** 参数:       无
** 返回:       无
********************************************************************/
BOOLEAN YX_JieYou_IsConfirm(void)
{
    return s_tcb.isconfirm;
}

/*******************************************************************
** 函数名:     YX_JieYou_GetCanCom
** 函数描述:   获取确认的can通道
** 参数:       无
** 返回:       无
********************************************************************/
INT8U YX_JieYou_GetCanCom(void)
{
    return s_tcb.com;
}

/*******************************************************************
** 函数名:     YX_JieYou_SetCanFilterByMask
** 函数描述:   设置can滤波
** 参数:       无
** 返回:       无
********************************************************************/
BOOLEAN YX_JieYou_SetCanFilterByMask(INT8U idtype, INT8U idnum, INT32U *pfilterid, INT32U *pmaskid)
{
    INT8U i;

    idnum = idnum > 14 ? 14 : idnum;
    s_tcb.can_filter.isvaild = TRUE;
    s_tcb.can_filter.filtertype = CAN_FILTER_MASK;
    s_tcb.can_filter.idtype = idtype;
    s_tcb.can_filter.idnum = idnum;

    for(i = 0; i < idnum; i++) {
        s_tcb.can_filter.filter.mask.id[i]     = pfilterid[i];
        s_tcb.can_filter.filter.mask.mask[i]   = pmaskid[i];
    }
    
    return TRUE;
}

/*******************************************************************
** 函数名:     YX_JieYou_SetCanFilterByList
** 函数描述:   设置can滤波
** 参数:       无
** 返回:       无
********************************************************************/
BOOLEAN YX_JieYou_SetCanFilterByList(INT8U idtype, INT8U idnum, INT32U *pfilterid)
{
    INT8U i;
    
    s_tcb.can_filter.isvaild = TRUE;
    s_tcb.can_filter.filtertype = CAN_FILTER_LIST;
    s_tcb.can_filter.idtype = idtype;
    s_tcb.can_filter.idnum = idnum;

    for(i = 0; i < idnum; i++) {
        s_tcb.can_filter.filter.list.canid[i] = pfilterid[i];
    }
    
    return TRUE;
}

