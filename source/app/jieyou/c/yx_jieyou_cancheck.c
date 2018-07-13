/******************************************************************************
**
** Filename:     yx_jieyou_cancheck.c
** Copyright:    
** Description:  该模块主要实现检测节油产品obd can引脚位置
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
#include "yx_debug.h"

#define MAX_IO_STATE 7
#define MAX_CHECKCNT 5000

#define CAN_HIGH 0xaa
#define CAN_LOW  0xcc

/* 误差值 */
#define CAN_AD_DIFF 200

/* 以下电压2/3分压 */
/* can基准电压值 2.5v */
#define CAN_AD_BASE 1666
#define CAN_AD_BASE_MAX (CAN_AD_BASE + CAN_AD_DIFF)
#define CAN_AD_BASE_MIN (CAN_AD_BASE - CAN_AD_DIFF)


/* can高电压值 3.5v */
#define CAN_AD_HIGH 2333
#define CAN_AD_HIGH_MAX (CAN_AD_HIGH +  CAN_AD_DIFF)
#define CAN_AD_HIGH_MIN (CAN_AD_HIGH -  CAN_AD_DIFF)

/* can低电压值 1.5v */
#define CAN_AD_LOW 1000
#define CAN_AD_LOW_MAX  (CAN_AD_LOW + CAN_AD_DIFF)
#define CAN_AD_LOW_MIN (CAN_AD_LOW - CAN_AD_DIFF)

#define SWITCH_AD_IO0  GPIO_PIN_B2
#define SWITCH_AD_IO1  GPIO_PIN_A2
#define SWITCH_AD_IO2  GPIO_PIN_A1

#define SWITCH_CANL_IO0  GPIO_PIN_A3
#define SWITCH_CANL_IO1  GPIO_PIN_A4
#define SWITCH_CANL_IO2  GPIO_PIN_A5


#define SWITCH_CANH_IO0  GPIO_PIN_B1
#define SWITCH_CANH_IO1  GPIO_PIN_B0
#define SWITCH_CANH_IO2  GPIO_PIN_A6

#define CAN_AD_CH   ADC_CH_1

#define PERIOD_SCAN           _HIGHTICK, 1

typedef struct {
    INT16U base;
    INT16U low;
    INT16U high;
} CAN_STATE_T;

typedef struct {
    INT8U cur;                  /* 当前检测io */
    INT16U cnt;                 /* 当前io检测次数 */
    INT8U can_lowpin;
    INT8U can_highpin;
    CAN_STATE_T state[MAX_IO_STATE];
} TCB_T;

static INT8U s_tmr_scan;
static TCB_T s_tcb;

/*******************************************************************
** 函数名:     switch_ad_io
** 函数描述:   切换ad检测io
** 参数:       [in] pdata：定时器特征值
** 返回:       无
********************************************************************/
static BOOLEAN switch_ad_io(INT8U state)
{
    INT8U io0, io1, io2;

    if(state > MAX_IO_STATE) {
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
** 函数名:     switch_canl_io
** 函数描述:   切换canl检测io
** 参数:       [in] state
** 返回:       无
********************************************************************/
static BOOLEAN switch_canl_io(INT8U state)
{
    INT8U io0, io1, io2;
    
    io0 = (state & 0x01);
    io1 = ((state & 0x02) >> 1);
    io2 = ((state & 0x04) >> 2);

    ST_GPIO_WritePin(SWITCH_CANL_IO0, io0);
    ST_GPIO_WritePin(SWITCH_CANL_IO1, io1);
    ST_GPIO_WritePin(SWITCH_CANL_IO2, io2);

    return TRUE;
}

/*******************************************************************
** 函数名:     switch_canh_io
** 函数描述:   切换canh检测io
** 参数:       [in] state
** 返回:       无
********************************************************************/
static BOOLEAN switch_canh_io(INT8U state)
{
    INT8U io0, io1, io2;
    
    io0 = (state & 0x01);
    io1 = ((state & 0x02) >> 1);
    io2 = ((state & 0x04) >> 2);

    ST_GPIO_WritePin(SWITCH_CANH_IO0, io0);
    ST_GPIO_WritePin(SWITCH_CANH_IO1, io1);
    ST_GPIO_WritePin(SWITCH_CANH_IO2, io2);

    return TRUE;
}

/*******************************************************************
** 函数名:     check_start
** 函数描述:   开始检测io
** 参数:       void
** 返回:       无
********************************************************************/
static BOOLEAN check_start(void)
{   
    if(OS_TmrIsRun(s_tmr_scan)) {
        return FALSE;
    }
    YX_MEMSET(&s_tcb, 0, sizeof(s_tcb));

    OS_StartTmr(s_tmr_scan, PERIOD_SCAN);

    switch_ad_io(1);

    switch_canl_io(0);
    switch_canh_io(0);

    return TRUE;
}

/*******************************************************************
** 函数名:     check_stop
** 函数描述:   停止检测io
** 参数:       void
** 返回:       无
********************************************************************/
static BOOLEAN check_stop(void)
{
    switch_ad_io(0);
    
    if(OS_TmrIsRun(s_tmr_scan)) {
        OS_StopTmr(s_tmr_scan);
    }

    return TRUE;
}

/*******************************************************************
** 函数名:     start_check
** 函数描述:   开始检测io
** 参数:       void
** 返回:       无
********************************************************************/
static BOOLEAN start_over(void)
{
    INT8U i;
    INT8U high = 0xff, low=0xff;
    CAN_ADAPTER_INTO_T can_pin_info;

    for(i = 0; i < MAX_IO_STATE; i++) {
        if(s_tcb.state[i].base > 100) {
            if(s_tcb.state[i].high > 1) {
                high = i;
            }

            if(s_tcb.state[i].low > 1) {
                low  = i;
            }
        }

        #if EN_DEBUG > 0
        printf_com("第%d个 base:%d high:%d low:%d\r\n", i, s_tcb.state[i].base, s_tcb.state[i].high, s_tcb.state[i].low);
        #endif
    }

    /* 完毕后,将AD切换脚置位为0 */
    switch_ad_io(0);

    if(high != 0xff && low != 0xff && high != low) {
        switch_canl_io(low+1);
        switch_canh_io(high+1);
        DAL_PP_ReadParaByID(PP_ID_CAN_ADAPTER, (INT8U*)&can_pin_info, sizeof(can_pin_info));
        can_pin_info.cam2_low = (low + 1);
        can_pin_info.can2_high = (high + 1);
        DAL_PP_StoreParaByID(PP_ID_CAN_ADAPTER, (INT8U*)&can_pin_info, sizeof(can_pin_info));
        #if EN_DEBUG > 0
        printf_com("当前can2为 高:%d 低:%d\r\n", can_pin_info.can2_high, can_pin_info.cam2_low);
        #endif
    } else {

        #if EN_DEBUG > 0
        printf_com("未检测到can2\r\n");
        #endif
        
        /* 未发现can设备，则持续检测 */
        check_start();
    }

    
    //check_start();
		
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
    INT16U advalue;
    
    s_tcb.cnt++;
    
    /* 完成当次检测 */
    if(s_tcb.cnt >= MAX_CHECKCNT) {
        s_tcb.cnt = 0;

        //printf_com("state:%d 最大:%d 最小:%d\r\n",s_tcb.cur+1, s_tcb.max, s_tcb.min );
        s_tcb.cur++;
        switch_ad_io(s_tcb.cur+1);
        
    }

    /* 所有io检测完成 */
    if(s_tcb.cur >= MAX_IO_STATE) {
        OS_StopTmr(s_tmr_scan);
        start_over();
        return;
    }

    advalue = ST_ADC_GetValue(CAN_AD_CH);
    /* 先检测是否是can的基准电压 */
    if((advalue > CAN_AD_BASE_MIN) && (advalue < CAN_AD_BASE_MAX)) {
        s_tcb.state[s_tcb.cur].base++;
    }else if((advalue > CAN_AD_HIGH_MIN) && (advalue < CAN_AD_HIGH_MAX)) {
        s_tcb.state[s_tcb.cur].high++;
    } else if((advalue > CAN_AD_LOW_MIN) && (advalue < CAN_AD_LOW_MAX)) {
        s_tcb.state[s_tcb.cur].low++;
    }

    //s_tcb.max =  advalue >= s_tcb.max ? advalue : s_tcb.max;
    //s_tcb.min =  advalue <= s_tcb.min ? advalue : s_tcb.min;
    
}

/*******************************************************************
** 函数名:     YX_JieYou_InitCanCheck
** 函数描述:   节油can检测初始化
** 参数:       无
** 返回:       无
********************************************************************/
void YX_JieYou_InitCanCheck(void)
{
    YX_MEMSET(&s_tcb, 0, sizeof(s_tcb));
    
    s_tmr_scan = OS_CreateTmr(TSK_ID_APP, (void *)0, ScanTmrProc); 

    //check_start();
}

/*******************************************************************
** 函数名:     YX_JieYou_StartCanCheck
** 函数描述:   开始can检测
** 参数:       无
** 返回:       无
********************************************************************/
BOOLEAN YX_JieYou_StartCanCheck(void)
{
    return check_start();
}

/*******************************************************************
** 函数名:     YX_JieYou_StopCanCheck
** 函数描述:   停止can检测
** 参数:       无
** 返回:       无
********************************************************************/
BOOLEAN YX_JieYou_StopCanCheck(void)
{
    return check_stop();
}

/*******************************************************************
** 函数名:     YX_JieYou_GetCanHigh
** 函数描述:   获取can高引脚编号
** 参数:       无
** 返回:       无
********************************************************************/
INT8U YX_JieYou_GetCanHigh(void)
{
    return s_tcb.can_highpin;
}

/*******************************************************************
** 函数名:     YX_JieYou_GetCanLow
** 函数描述:   获取can低引脚编号
** 参数:       无
** 返回:       无
********************************************************************/
INT8U YX_JieYou_GetCanLow(void)
{
    return s_tcb.can_lowpin;
}

/*******************************************************************
** 函数名:     YX_JieYou_SetCanHighIo
** 函数描述:   设置can高引脚编号
** 参数:       无
** 返回:       无
********************************************************************/
BOOLEAN YX_JieYou_SetCanHighIo(INT8U pin)
{
    return switch_canh_io(pin);
}

/*******************************************************************
** 函数名:     YX_JieYou_SetCanLowIo
** 函数描述:   设置can低引脚编号
** 参数:       无
** 返回:       无
********************************************************************/
BOOLEAN YX_JieYou_SetCanLowIo(INT8U pin)
{
    return switch_canl_io(pin);
}



