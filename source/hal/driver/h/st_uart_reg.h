/******************************************************************************
**
** Filename:     st_uart_reg.h
** Copyright:    
** Description:  该模块主要实现串口资源的配置管理
**
*******************************************************************************
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#ifndef ST_UART_CFG_H
#define ST_UART_CFG_H        1
    
/*
********************************************************************************
* define struct
********************************************************************************
*/
typedef struct {
    INT8U  com;
    INT8U  enable;
    INT8U  is_remap;
    INT32U uart_pin_tx;
    INT32U uart_pin_rx;
    
    INT32U dma_base_tx;
    INT32U dma_base_rx;
    
    INT32U uart_base;
    INT32U gpio_base;
    
    INT32U uart_rcc;
    INT32U uart_remap;
} UART_TBL_T;

/*
********************************************************************************
* 定义统一串口通道编号
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

#define UART_DEF(_COM_, _ENABLE, _PIN_TX, _PIN_RX,  _DMA_TX_BASE, _DMA_RX_BASE, _UART_BASE, _GPIO_BASE, _RCC, _ISREMAP, _REMAP) \
                 _COM_,
                 
#define END_UART_CFG

typedef enum {
    #include "st_uart_reg.def"
    UART_COM_MAX
} UART_COM_E;



/*******************************************************************
** 函数名:     ST_UART_GetRegTblInfo
** 函数描述:   获取对应串口的配置表信息
** 参数:       [in] com: 串口编号,见UART_COM_E
** 返回:       成功返回配置表指针，失败返回0
********************************************************************/
const UART_TBL_T *ST_UART_GetRegTblInfo(INT8U com);

/*******************************************************************
** 函数名:    ST_UART_GetCfgTblMax
** 函数描述:  获取已注册的串口个数
** 参数:      无
** 返回:      注册的个数
********************************************************************/
INT8U ST_UART_GetCfgTblMax(void);

#endif

