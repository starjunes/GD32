/********************************************************************************
**
** 文件名:     man_irq.c
** 版权所有:   (c) 2014-2020 厦门雅迅网络股份有限公司
** 文件描述:   系统中断管理模块
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2010/12/30 | LEON   | 创建本模块
**
*********************************************************************************/
#include "default_it.h"
#include "man_error.h"
#include "man_irq.h"

/*************************************************************************************************/
/*                           以下定义一个中断函数向量表                                          */
/*************************************************************************************************/
#ifdef BEGIN_SYS_IRQ_TABLE
#undef BEGIN_SYS_IRQ_TABLE
#endif

#ifdef ENDOF_SYS_IRQ_TABLE
#undef ENDOF_SYS_IRQ_TABLE
#endif

#ifdef DEF_SYS_IRQ
#undef DEF_SYS_IRQ
#endif

#define BEGIN_SYS_IRQ_TABLE                      __attribute__((section ("VECTOR_RAM"))) ExecFuncPtr s_vector_tbl[] = {
#define DEF_SYS_IRQ(_IRQ_NUMBER_, _IRQ_HANDLE_)  (ExecFuncPtr)_IRQ_HANDLE_,
#define ENDOF_SYS_IRQ_TABLE                      (ExecFuncPtr)0};

#include "man_irq.def"

/*******************************************************************
** 函数名:     NVIC_IrqHandleInstall
** 函数描述:   向系统注册一个用户自定义中断处理函数
** 参数:       [in] irq: 中断号
**             [in] handle: 中断处理函数
**             [in] ppri: 主优先级
**             [in] spri: 次优先级
**             [in] enable: 是否使能
** 返回:       无
********************************************************************/
void NVIC_IrqHandleInstall(INT8U irq, ExecFuncPtr handle, PRE_PRI_E ppri, SUB_PRI_E spri, BOOLEAN enable)
{
    volatile INT32U cpu_sr;    
    DAL_ASSERT(irq < MAX_IRQ_NUMABER);
    DAL_ASSERT(handle != NULL);

    s_vector_tbl[irq] = handle;
    
    ENTER_CRITICAL(); 
    nvic_irq_enable(irq - 16, ppri, spri);
    EXIT_CRITICAL();
}

