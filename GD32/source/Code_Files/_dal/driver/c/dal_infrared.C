/**************************************************************************************************
**                                                                                               **
**  文件名称:  Infrared.C                                                                        **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2010                                    **
**  创建信息:  jump -- 2011年1月20日                                                             **
**  文件描述:  用于can定时发送处理                                                                      **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/

#define	INFRARED_GLOBALS

#include "dal_include.h"
#include "dal_InFrared.H"
#include "Man_Timer.h"
#include "dal_timer.h"
#include "dal_exti.h"
#include "man_irq.h"
#include "dal_pinList.h"
#include "debug_print.h"
#include "dal_can.h"

#define MS_250 250    /* 25ms */
#define MS_150 150    /* 15ms */
#define MS_125 125    /* 12.5ms */
#define MS_100 100    /* 10ms */
#define MS_28  28     /* 2.8ms */
#define MS_17  17     /* 1.7ms */
#define MS_6   6      /* 0.6ms */

#define  MAX_CAP_CNT         40

typedef enum {
    TM_CANCF    = 0x00,                                                /* 定时连续帧 */
		TM_CANSTMIN = 0x01,                                                /* STmin */
    TM_MAX
}TM_LBINDEX_E;

typedef struct {
    TM_LBINDEX_E index;                                             /* 回调索引号 */
    void  (*lbhandle) (void);                 /* 回调函数指针 */
} TIME_LBAPP_E;

static INT16U  can_cnt,lock_can;
static TIME_LBAPP_E    s_TMfunctionentry[TM_MAX];
/*******************************************************************
** 函数名:     Dal_CANLBRepReg
** 函数描述:   CAN回调上报函数
** 参数:       [in] handle             指向APP层的函数指针
** 返回:       无
********************************************************************/
void dal_CanSeqCFSendCallbakFunc(void (* handle) (void))
{
    s_TMfunctionentry[TM_CANCF].lbhandle = handle;
}
/*******************************************************************
** 函数名:     Dal_CANLBRepReg
** 函数描述:   CAN回调上报函数
** 参数:       [in] handle             指向APP层的函数指针
** 返回:       无
********************************************************************/
void dal_CanSTminTimeoutCallbakFunc(void (* handle) (void))
{
    s_TMfunctionentry[TM_CANSTMIN].lbhandle = handle;
}

/**************************************************************************************************
**  函数名称:  ss_TIM6_IRQHandler
**  功能描述:  定时器6中断处理函数，为红外扫描计时1ms。
**  输入参数:
**  返回参数:
**************************************************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) static void ss_TIM6_IRQHandler(void) __irq
{

    if (timer_interrupt_flag_get(TIMER6,TIMER_INT_FLAG_UP) != RESET) {
        if (++can_cnt >= CAN_SENDPERIOD) {
            can_cnt = 0;
            SendCANMsg_Period();
        }
				CanBusOffHal();
				if (s_TMfunctionentry[TM_CANCF].lbhandle != PNULL) {
				    if(++lock_can >= 5) {
                s_TMfunctionentry[TM_CANCF].lbhandle();
					  }
        }
				if (s_TMfunctionentry[TM_CANSTMIN].lbhandle != PNULL) {
                s_TMfunctionentry[TM_CANSTMIN].lbhandle();
        }
    }
	timer_interrupt_flag_clear(TIMER6, TIMER_INT_FLAG_UP);
}

/**************************************************************************************************
**  函数名称:  InitFrKb
**  功能描述:  红外遥控模块初始化
**  输入参数:
**  返回参数:
**************************************************************************************************/
void InitFrKb(void)
{
    TimerX_Initiate(TIMER_NO_7);
    NVIC_IrqHandleInstall(TIM6_IRQ, (ExecFuncPtr)ss_TIM6_IRQHandler, INFRAID_PRIOTITY, TRUE);
    TimerX_IT_Enable(TIMER_NO_7);
    TimerX_Func_Enable(TIMER_NO_7);
}


