/******************************************************************************
**
** Filename:     yx_app_tsk.c
** Copyright:    
** Description:  该模块主要实现APP层任务管理
**
-------------------------------------------------------------------------------
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#include "yx_include.h"
#include "dal_output_drv.h"
#include "dal_pp_drv.h"
#include "at_drv.h"
#include "yx_debug.h"



/*
********************************************************************************
* define config parameters
********************************************************************************
*/


/*
********************************************************************************
* define struct
********************************************************************************
*/
typedef struct {
    INT8U creg;
} DCB_T;


/*
********************************************************************************
* define module variants
********************************************************************************
*/
static DCB_T s_dcb;
static INT8U s_apptmr;


/*******************************************************************
** 函数名:     Hdl_MSG_APP_OVERTIME
** 函数描述:   定时器超时处理
** 参数:       [in] tskid:任务编号
**              [in] msgid:消息编号
**              [in] para1:参数1
**              [in] para2:参数2
** 返回:       无
********************************************************************/
void Hdl_MSG_APP_OVERTIME(INT16U tskid, INT16U msgid, INT32U para1, INT32U para2)
{
    OS_ExeTmrFunc(tskid);
}



/*******************************************************************
** 函数名:     AppTmrProc
** 函数描述:   定时器
** 参数:       [in] index:定时器标识
** 返回:       无
********************************************************************/
#include "stm32f0_discovery.h"
#include "yx_dym_cfg.h"
#include "st_rtc_drv.h"
#include "hal_hit_drv.h"
#include "st_adc_drv.h"
#include "hal_can_drv.h"
INT32U DAL_PULSE_GetTotalPulse(void);
static void AppTmrProc(void *index)
{
    INT8U rssi, ber, simcard, creg, cgreg;
    
    ADP_NET_GetNetworkState(&simcard, &creg, &cgreg, &rssi, &ber);
    if (creg == NETWORK_STATE_HOME || creg == NETWORK_STATE_ROAMING) {
        if (!s_dcb.creg) {
            s_dcb.creg = TRUE;
            DAL_OUTPUT_StartPermentFlash(OPT_LEDRED, 2, 20);
            DAL_OUTPUT_StartPermentFlash(OPT_LEDGREEN, 2, 20);
        }
    } else {
        if (s_dcb.creg) {
            s_dcb.creg = FALSE;
            DAL_OUTPUT_StartPermentFlash(OPT_LEDRED, 2, 50);
            DAL_OUTPUT_StartPermentFlash(OPT_LEDGREEN, 2, 50);
        }
    }
    
#if DEBUG_SYS > 0
#if 0
    INT32U pfilterid, pmaskid;
    CAN_CFG_T cfg;
    CAN_DATA_T data;
    
    if (!HAL_CAN_IsOpened(CAN_COM_0)) {
        printf_com("<AppTmrProc start>\r\n");
    
        cfg.com     = CAN_COM_0;
        cfg.baud    = 250000;
        cfg.mode    = CAN_WORK_MODE_SILENT_LOOPBACK;
    
        HAL_CAN_OpenCan(&cfg);
        
        pfilterid = 0;
        pmaskid = 0;
        HAL_CAN_SetFilterParaByMask(CAN_COM_0, CAN_ID_TYPE_EXT, 1, &pfilterid, &pmaskid);
    
        printf_com("<AppTmrProc end>\r\n");
    } else {
        data.id     = 0xAABB;
        data.idtype = CAN_ID_TYPE_EXT;
        data.dlc    = 8;
        data.datatype = CAN_RTR_TYPE_DATA;
        
        if (HAL_CAN_SendData(CAN_COM_0, &data)) {
            printf_com("<HAL_CAN_SendData>\r\n");
        }
    }
#endif
    
#if 0
    INT8U i, tempbuf[16];
    TEL_T mytel;
    RESET_RECORD_T resetrecord;
    
    printf_com("<test timer(%d)(%d)(%d)\r\n", DAL_PULSE_GetTotalPulse(), RCC_GetFlagStatus(RCC_FLAG_HSERDY), RCC_GetFlagStatus(RCC_FLAG_LSERDY));
    
    /*HAL_SFLASH_Read(9, tempbuf, sizeof(tempbuf));
    printf_hex(tempbuf, sizeof(tempbuf));
    printf_com(">\r\n");
    
    YX_MEMSET(tempbuf, 0x55, sizeof(tempbuf));
    HAL_SFLASH_Write(9, tempbuf, sizeof(tempbuf));
    HAL_SFLASH_Read(9, tempbuf, sizeof(tempbuf));
    printf_hex(tempbuf, sizeof(tempbuf));
    printf_com(">\r\n");*/
    
    if (!DAL_PP_ReadParaByID(PP_ID_MYTEL, (INT8U *)&mytel, sizeof(mytel))) {
        mytel.tellen = 11;
        YX_MEMSET(mytel.tel, 0x30, 11);
        DAL_PP_StoreParaByID(PP_ID_MYTEL, (INT8U *)&mytel, sizeof(mytel));
        printf_hex(mytel.tel, 11);
        printf_com("<DAL_PP_ReadParaByID error>\r\n");
    } else {
        printf_hex(mytel.tel, 11);
        printf_com(">\r\n");
    }
#endif
    
#if 0
    INT8U i, tempbuf[32];
    GSENSOR_XYZ_T xyz;
    
    if (HAL_HIT_Read(0, tempbuf, 32)) {
        printf_com("HAL_HIT_Read:");
        printf_hex(tempbuf, 32);
        printf_com(">\r\n");
    } else {
        printf_com("HAL_HIT_Read error\r\n");
        //OS_ASSERT(0, RETURN_VOID);
    }
    
    for (i = 0; i < 32; i++) {
        if (!HAL_HIT_Read(i, &tempbuf[i], 1)) {
            printf_com("HAL_HIT_Read error\r\n");
        }
    }
    printf_com("HAL_HIT_Read:");
        printf_hex(tempbuf, 32);
        printf_com(">\r\n");
    
    if (HAL_HIT_GetValueXYZ(&xyz)) {
        printf_com("HAL_HIT_GetValueXYZ(%d)(%d)(%d)\r\n", xyz.x, xyz.y, xyz.z);
    }
#endif
    //ST_RTC_CloseRtcFunction();
    //ST_RTC_OpenRtcFunction(RTC_CLOCK_LSE);
    printf_com("ST_ADC_GetValue(%d)(%d)(%d)(%d)(%d)(%d)\r\n", ST_ADC_GetValue(0), 
                                                      ST_ADC_GetValue(1), 
                                                      ST_ADC_GetValue(2), 
                                                      ST_ADC_GetValue(3),
                                                      ST_ADC_GetValue(4), 
                                                      ST_ADC_GetValue(5));
#endif
}

/*******************************************************************
** 函数名:     YX_APP_InitTsk
** 函数描述:   HAL任务初始化
** 参数:       无
** 返回:       无
********************************************************************/
void YX_APP_InitTsk(void)
{
    YX_MEMSET(&s_dcb, 0, sizeof(s_dcb));
    
    DAL_OUTPUT_StartFlash(OPT_LEDRED, OUTPUT_PRI_MID, 25, 1, 1, 0, 0);
    DAL_OUTPUT_StartFlash(OPT_LEDGREEN, OUTPUT_PRI_MID, 25, 1, 1, 0, 0);
    DAL_OUTPUT_StartPermentFlash(OPT_LEDRED, 2, 50);
    DAL_OUTPUT_StartPermentFlash(OPT_LEDGREEN, 2, 50);
    
    s_apptmr = OS_CreateTmr(TSK_ID_APP, 0, AppTmrProc);
    OS_StartTmr(s_apptmr, _SECOND, 10);
}

