/********************************************************************************
**
** 文件名:     st_adc_reg.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现ADC资源的配置管理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2014/08/15 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "yx_include.h"
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "st_adc_reg.h"

/*
********************************************************************************
* 定义各个ADC的配置信息
********************************************************************************
*/
#ifdef BEGIN_ADC_CFG
#undef BEGIN_ADC_CFG
#endif

#ifdef END_ADC_CFG
#undef END_ADC_CFG
#endif

#ifdef ADC_DEF
#undef ADC_DEF
#endif

#define BEGIN_ADC_CFG

#define ADC_DEF(_CH_, _PIN_ADC_, _ADC_CH_, _DMA_BASE,  _ADC_BASE, _GPIO_BASE, _RCC) \
   {(INT8U)_CH_, (INT32U)_PIN_ADC_, _ADC_CH_, (INT32U)_DMA_BASE,  (INT32U)_ADC_BASE, (INT32U)_GPIO_BASE, (INT32U)_RCC},

#define END_ADC_CFG


static const ADC_REG_T s_adc_tbl[] = {
    #include "st_adc_reg.def"
    {0}
};



/*******************************************************************
** 函数名:     ST_ADC_GetRegTblInfo
** 函数描述:   获取对应ADC的配置表信息
** 参数:       [in] com: ADC编号,见ADC_CH_E
** 返回:       成功返回配置表指针，失败返回0
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) const ADC_REG_T *ST_ADC_GetRegTblInfo(INT8U com)
{
    if (com >= ADC_CH_MAX) {
        return 0;
    }
    return &s_adc_tbl[com];
}

/*******************************************************************
** 函数名:    ST_ADC_GetCfgTblMax
** 函数描述:  获取已注册的ADC个数
** 参数:      无
** 返回:      注册的个数
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) INT8U ST_ADC_GetCfgTblMax(void)
{
    return ADC_CH_MAX;
}
