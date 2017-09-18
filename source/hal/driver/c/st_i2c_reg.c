/******************************************************************************
**
** Filename:     st_i2c_reg.c
** Copyright:    
** Description:  该模块主要实现I2C资源的配置管理
**
*******************************************************************************
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#include "hal_include.h"
#include "stm32f0xx.h"
#include "stm32f0xx_conf.h"
#include "st_i2c_reg.h"

/*
********************************************************************************
* 定义各个I2C的配置信息
********************************************************************************
*/
#ifdef BEGIN_I2C_CFG
#undef BEGIN_I2C_CFG
#endif

#ifdef END_I2C_CFG
#undef END_I2C_CFG
#endif

#ifdef I2C_DEF
#undef I2C_DEF
#endif

#define BEGIN_I2C_CFG

#define I2C_DEF(_COM_, _ENABLE, _PIN_SCL, _PIN_SDA,  _I2C_BASE, _GPIO_BASE, _RCC) \
   {(INT8U)_COM_, _ENABLE, (INT32U)_PIN_SCL, (INT32U)_PIN_SDA,  (INT32U)_I2C_BASE, (INT32U)_GPIO_BASE, (INT32U)_RCC},

#define END_I2C_CFG


static const I2C_REG_T s_i2c_tbl[] = {
    #include "st_i2c_reg.def"
    {0}
};



/*******************************************************************
** 函数名:     ST_I2C_GetRegTblInfo
** 函数描述:   获取对应串口的配置表信息
** 参数:       [in] com: 串口编号,见I2C_COM_E
** 返回:       成功返回配置表指针，失败返回0
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) const I2C_REG_T *ST_I2C_GetRegTblInfo(INT8U com)
{
    if (com >= I2C_COM_MAX) {
        return 0;
    }
    return &s_i2c_tbl[com];
}

/*******************************************************************
** 函数名:    ST_I2C_GetCfgTblMax
** 函数描述:  获取已注册的串口个数
** 参数:      无
** 返回:      注册的个数
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) INT8U ST_I2C_GetCfgTblMax(void)
{
    return I2C_COM_MAX;
}
