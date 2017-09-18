/******************************************************************************
**
** Filename:     st_uart_reg.c
** Copyright:    
** Description:  该模块主要实现串口资源的配置管理
**
*******************************************************************************
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#include "yx_include.h"
#include "stm32f0xx.h"
#include "stm32f0xx_conf.h"
#include "st_uart_reg.h"

/*
********************************************************************************
* 定义各个串口的配置信息
********************************************************************************
*/
#ifdef BEGIN_UART_CFG
#undef BEGIN_UART_CFG
#endif

#ifdef END_UART_CFG
#undef END_UART_CFG
#endif

#ifdef UART_DEF
#undef UART_DEF
#endif

#define BEGIN_UART_CFG

#define UART_DEF(_COM_, _ENABLE, _PIN_TX, _PIN_RX,  _DMA_TX_BASE, _DMA_RX_BASE, _UART_BASE, _GPIO_BASE, _RCC, _REMAP) \
   {(INT8U)_COM_, _ENABLE, (INT32U)_PIN_TX, (INT32U)_PIN_RX,  (INT32U)_DMA_TX_BASE, (INT32U)_DMA_RX_BASE, (INT32U)_UART_BASE, (INT32U)_GPIO_BASE, (INT32U)_RCC, (INT32U)_REMAP},

#define END_UART_CFG


static const UART_TBL_T s_uart_tbl[] = {
    #include "st_uart_reg.def"
    {0}
};



/*******************************************************************
** 函数名:     ST_UART_GetRegTblInfo
** 函数描述:   获取对应串口的配置表信息
** 参数:       [in] com: 串口编号,见UART_COM_E
** 返回:       成功返回配置表指针，失败返回0
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) const UART_TBL_T *ST_UART_GetRegTblInfo(INT8U com)
{
    if (com >= UART_COM_MAX) {
        return 0;
    }
    return &s_uart_tbl[com];
}

/*******************************************************************
** 函数名:    ST_UART_GetCfgTblMax
** 函数描述:  获取已注册的串口个数
** 参数:      无
** 返回:      注册的个数
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) INT8U ST_UART_GetCfgTblMax(void)
{
    return UART_COM_MAX;
}
