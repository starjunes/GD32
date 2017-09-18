/******************************************************************************
**
** Filename:     hal_can_reg.c
** Copyright:    
** Description:  该模块主要实现CAN资源的配置管理
**
*******************************************************************************
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#include "hal_include.h"
#include "stm32f0xx.h"
#include "stm32f0xx_conf.h"
#include "hal_can_reg.h"

/*
********************************************************************************
* 定义各个CAN的配置信息
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

#define CAN_DEF(_COM_, _ENABLE, _PIN_TX, _PIN_RX,  _CAN_BASE, _GPIO_BASE, _RCC) \
   {(INT8U)_COM_, _ENABLE, (INT32U)_PIN_TX, (INT32U)_PIN_RX,  (INT32U)_CAN_BASE, (INT32U)_GPIO_BASE, (INT32U)_RCC},

#define END_CAN_CFG


static const CAN_REG_T s_can_tbl[] = {
    #include "hal_can_reg.def"
    {0}
};



/*******************************************************************
** 函数名:     HAL_CAN_GetRegTblInfo
** 函数描述:   获取对应串口的配置表信息
** 参数:       [in] com: 串口编号,见CAN_COM_E
** 返回:       成功返回配置表指针，失败返回0
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) const CAN_REG_T *HAL_CAN_GetRegTblInfo(INT8U com)
{
    if (com >= CAN_COM_MAX) {
        return 0;
    }
    return &s_can_tbl[com];
}

/*******************************************************************
** 函数名:    HAL_CAN_GetCfgTblMax
** 函数描述:  获取已注册的串口个数
** 参数:      无
** 返回:      注册的个数
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) INT8U HAL_CAN_GetCfgTblMax(void)
{
    return CAN_COM_MAX;
}
