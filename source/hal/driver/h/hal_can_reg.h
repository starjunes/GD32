/******************************************************************************
**
** Filename:     hal_can_reg.h
** Copyright:    
** Description:  该模块主要实现串口资源的配置管理
**
*******************************************************************************
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#ifndef HAL_CAN_REG_H
#define HAL_CAN_REG_H         1
    
/*
********************************************************************************
* define struct
********************************************************************************
*/
typedef struct {
    INT8U  com;                        /* 统一编号 */
    INT8U  enable;                     /* 通道使能 */
    INT8U  fifo;                       /* fifo */
    INT32U pin_tx;                     /* TX管脚编号 */
    INT32U pin_rx;                     /* RX管脚编号 */
    
    INT32U can_base;                   /* CAN寄存器基址 */
    INT32U gpio_base;                  /* GPIO寄存器基址 */
    INT32U can_rcc;                    /* CAN系统时钟 */
} CAN_REG_T;

/*
********************************************************************************
* 定义统一串口通道编号
********************************************************************************
*/
#ifdef BEGIN_CAN_CFG
#undef BEGIN_CAN_CFG
#endif

#ifdef END_CAN_CFG
#undef END_CAN_CFG
#endif 

#ifdef CAN_DEF
#undef CAN_DEF
#endif 

#define BEGIN_CAN_CFG

#define CAN_DEF(_COM_, _ENABLE, _PIN_TX, _PIN_RX,  _CAN_BASE, _GPIO_BASE, _RCC, _FIFO) \
                 _COM_,
                 
#define END_CAN_CFG

typedef enum {
    #include "hal_can_reg.def"
    CAN_COM_MAX
} CAN_COM_E;



/*******************************************************************
** 函数名:     HAL_CAN_GetRegTblInfo
** 函数描述:   获取对应串口的配置表信息
** 参数:       [in] com: 串口编号,见CAN_COM_E
** 返回:       成功返回配置表指针，失败返回0
********************************************************************/
const CAN_REG_T *HAL_CAN_GetRegTblInfo(INT8U com);

/*******************************************************************
** 函数名:    HAL_CAN_GetCfgTblMax
** 函数描述:  获取已注册的串口个数
** 参数:      无
** 返回:      注册的个数
********************************************************************/
INT8U HAL_CAN_GetCfgTblMax(void);

#endif

