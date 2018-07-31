/******************************************************************************
**
** Filename:     yx_jieyou_adcheck.c
** Copyright:    
** Description:  该模块主要实现检测节油产品obd can引脚 AD值检测，用于生产检测
**
-------------------------------------------------------------------------------
**             Revision history
**-----------------------------------------------------------------------------
**| 2018/07/31 | nhong |  创建文件
*******************************************************************************/
#include "yx_include.h"
#include "dal_pp_drv.h"
#include "st_gpio_drv.h"
#include "st_adc_drv.h"
#include "yx_debug.h"
#include "yx_jieyou_cancheck.h"

#define MAX_AD_IO 7
#define CAN_AD_CH   ADC_CH_1

#define SWITCH_AD_IO0  GPIO_PIN_B2
#define SWITCH_AD_IO1  GPIO_PIN_A2
#define SWITCH_AD_IO2  GPIO_PIN_A1


#define PERIOD_SCAN           _TICK, 30

typedef struct {
    INT8U  cur;                  /* 当前检测io */
    void (*result)(INT16U *ad, INT8U count);
    INT16U ad[MAX_AD_IO];
} TCB_T;


static INT8U s_tmr_scan;
static TCB_T s_tcb;

/*******************************************************************
** 函数名:     switch_ad_io
** 函数描述:   切换ad检测io
** 参数:       
** 返回:       无
********************************************************************/
static BOOLEAN switch_ad_io(INT8U state)
{
    INT8U io0, io1, io2;
    
    if(state > MAX_AD_IO) {
        return FALSE;
    }
    
    io0 = (state & 0x01);
    io1 = ((state & 0x02) >> 1);
    io2 = ((state & 0x04) >> 2);

    ST_GPIO_WritePin(SWITCH_AD_IO0, io0);
    ST_GPIO_WritePin(SWITCH_AD_IO1, io1);
    ST_GPIO_WritePin(SWITCH_AD_IO2, io2);

    //printf_com("state:%x 切换adc io sad2:%d sad1:%d sad0:%d\r\n",state, io2, io1, io0);
    return TRUE;
}


/*******************************************************************
** 函数名:     ScanTmrProc
** 函数描述:   定时器处理
** 参数:       [in] pdata：定时器特征值
** 返回:       无
********************************************************************/
static void ScanTmrProc(void *pdata)
{
    if(s_tcb.cur >= MAX_AD_IO) {
        OS_StopTmr(s_tmr_scan);
        return;
    }
    
    s_tcb.ad[s_tcb.cur] = ST_ADC_GetValue(CAN_AD_CH);

    s_tcb.cur++;
    if(s_tcb.cur >= MAX_AD_IO) {
        OS_StopTmr(s_tmr_scan);
        switch_ad_io(0);
        if(s_tcb.result != 0) {
            s_tcb.result(s_tcb.ad, MAX_AD_IO);
        }
        return;
    } else {
        switch_ad_io(s_tcb.cur+1);
    }

    
}

/*******************************************************************
** 函数名:     YX_JieYou_InitCanCheck
** 函数描述:   节油ad检测初始化
** 参数:       无
** 返回:       无
********************************************************************/
void YX_JieYou_InitADCheck(void)
{
    YX_MEMSET(&s_tcb, 0, sizeof(s_tcb));
    
    s_tmr_scan = OS_CreateTmr(TSK_ID_APP, (void *)0, ScanTmrProc); 
}

/*******************************************************************
** 函数名:     YX_JieYou_StartADCheck
** 函数描述:   启动AD检测
** 参数:       无
** 返回:       无
********************************************************************/
void YX_JieYou_StartADCheck(void (*result)(INT16U *ad, INT8U count))
{
    /* 先关闭can检测 */
    YX_JieYou_StopCanCheck();

    s_tcb.result = result;
    
    s_tcb.cur = 0;
    switch_ad_io(s_tcb.cur + 1);
    
    OS_StartTmr(s_tmr_scan, PERIOD_SCAN);
}

