/********************************************************************************
**
** 文件名:     st_adc_simu.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现模拟ADC接口驱动
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2014/04/09 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "yx_include.h"
#include "stm32f0xx.h"
#include "st_gpio_drv.h"
#include "st_adc_reg.h"
#include "st_adc_drv.h"
#include "yx_debug.h"

/*
********************************************************************************
* 定义配置参数
********************************************************************************
*/

/* 定义管脚 */
#define _SENDING             0x01
#define _OPEN                0x80

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    INT8U  status;
    INT16U value[ADC_CH_MAX + 3];
} ADC_T;

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static ADC_T s_dcb;



/*******************************************************************
** 函数名称: ADC_PinsConfig
** 函数描述: 管脚配置
** 参数:     [in] pinfo: 配置表
** 返回:     无
********************************************************************/
static void ADC_PinsConfig(const ADC_REG_T *pinfo)
{
    GPIO_InitTypeDef gpio_initstruct;
    
    if (pinfo == 0) {
        return;
    }
    
    //RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);                             /* 开启系统时钟 */
    
    /* Configure gpio */
    gpio_initstruct.GPIO_Pin   = (INT16U)(1 << pinfo->pin_adc);
    gpio_initstruct.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_initstruct.GPIO_Mode  = GPIO_Mode_AN;
    gpio_initstruct.GPIO_OType = GPIO_OType_OD;
    gpio_initstruct.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    
    GPIO_Init((GPIO_TypeDef *)pinfo->gpio_base, &gpio_initstruct);
}

/*******************************************************************
** 函数名称: ADC_DMAConfig
** 函数描述: DMA配置
** 参数:     [in] pinfo: 配置表
** 返回:     无
********************************************************************/
static void ADC_DMAConfig(const ADC_REG_T *pinfo)
{
    DMA_InitTypeDef dma_initstruct;
  
    /* DMA1 clock enable */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1 , ENABLE);
    
    /* DMA1 Channel Config */
    DMA_DeInit((DMA_Channel_TypeDef *)pinfo->dma_base);                          /* 恢复DMA通道 */
    
    DMA_StructInit(&dma_initstruct);
    dma_initstruct.DMA_PeripheralBaseAddr = (INT32U)&(((ADC_TypeDef *)pinfo->adc_base)->DR);
    dma_initstruct.DMA_MemoryBaseAddr     = (INT32U)s_dcb.value;
    dma_initstruct.DMA_DIR                = DMA_DIR_PeripheralSRC;
    dma_initstruct.DMA_BufferSize         = ADC_CH_MAX + 3;
    dma_initstruct.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    dma_initstruct.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    dma_initstruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    dma_initstruct.DMA_MemoryDataSize     = DMA_MemoryDataSize_HalfWord;
    dma_initstruct.DMA_Mode               = DMA_Mode_Circular;
    dma_initstruct.DMA_Priority           = DMA_Priority_High;
    dma_initstruct.DMA_M2M                = DMA_M2M_Disable;
  
    DMA_Init((DMA_Channel_TypeDef *)pinfo->dma_base, &dma_initstruct);
  
    /* DMA1 Channel1 enable */
    DMA_Cmd(DMA1_Channel1, ENABLE);
}

/*******************************************************************
** 函数名称: ADC_AdcConfig
** 函数描述: ADC配置
** 参数:     [in] pinfo: 配置表
** 返回:     无
********************************************************************/
static void ADC_AdcConfig(void)
{
    INT8U i, nreg;
    const ADC_REG_T *pinfo;
    ADC_InitTypeDef adc_initstructure;
    
    nreg = ST_ADC_GetCfgTblMax();
    if (nreg == 0) {
        return;
    }
    pinfo = ST_ADC_GetRegTblInfo(0);
    /* ADC Periph clock enable */
    RCC_APB2PeriphClockCmd(pinfo->adc_rcc, ENABLE);
    
    /* Configure the ADC1 in continous mode withe a resolutuion equal to 12 bits  */
    ADC_DeInit((ADC_TypeDef *)pinfo->adc_base);                                /* ADC DeInit */
    ADC_StructInit(&adc_initstructure);
    adc_initstructure.ADC_Resolution           = ADC_Resolution_12b;
    adc_initstructure.ADC_ContinuousConvMode   = ENABLE;
    adc_initstructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    //adc_initstructure.ADC_ExternalTrigConv     = ADC_ExternalTrigConv_T1_TRGO;
    adc_initstructure.ADC_DataAlign            = ADC_DataAlign_Right;
    adc_initstructure.ADC_ScanDirection        = ADC_ScanDirection_Upward;
    ADC_Init((ADC_TypeDef *)pinfo->adc_base, &adc_initstructure); 
    
    for (i = 0; i < nreg; i++) {
        pinfo = ST_ADC_GetRegTblInfo(i);
        ADC_PinsConfig(pinfo);
        ADC_ChannelConfig((ADC_TypeDef *)pinfo->adc_base, pinfo->channel, ADC_SampleTime_55_5Cycles);
    }
    
    /* Convert the ADC1 temperature sensor  with 55.5 Cycles as sampling time */ 
    ADC_ChannelConfig((ADC_TypeDef *)pinfo->adc_base, ADC_Channel_TempSensor , ADC_SampleTime_55_5Cycles);  
    ADC_TempSensorCmd(ENABLE);
    
    /* Convert the ADC1 Vref  with 55.5 Cycles as sampling time */ 
    ADC_ChannelConfig(ADC1, ADC_Channel_Vrefint , ADC_SampleTime_55_5Cycles); 
    ADC_VrefintCmd(ENABLE);
    
    /* Convert the ADC1 Vbat with 55.5 Cycles as sampling time */ 
    ADC_ChannelConfig(ADC1, ADC_Channel_Vbat , ADC_SampleTime_55_5Cycles);  
    ADC_VbatCmd(ENABLE);
}

/*******************************************************************
** 函数名称: ADC_StartConvert
** 函数描述: 启动ADC采集功能
** 参数:     无
** 返回:     无
********************************************************************/
static void ADC_StartConvert(void)
{
    INT8U nreg;
    INT32U ct_delay;
    const ADC_REG_T *pinfo;
    
    nreg = ST_ADC_GetCfgTblMax();
    if (nreg == 0) {
        return;
    }
    
    pinfo = ST_ADC_GetRegTblInfo(0);
    ADC_DMAConfig(pinfo);
    ADC_AdcConfig();
    
    ADC_GetCalibrationFactor((ADC_TypeDef *)pinfo->adc_base);                  /* ADC Calibration */
    ADC_DMARequestModeConfig((ADC_TypeDef *)pinfo->adc_base, ADC_DMAMode_Circular);/* ADC DMA request in circular mode */
    ADC_DMACmd((ADC_TypeDef *)pinfo->adc_base, ENABLE);                        /* Enable ADC_DMA */
    ADC_Cmd((ADC_TypeDef *)pinfo->adc_base, ENABLE);                           /* Enable ADC1 */
    
    ct_delay = 0;
    while (++ct_delay < 0x1000) {
        ClearWatchdog();
        if (ADC_GetFlagStatus((ADC_TypeDef *)pinfo->adc_base, ADC_FLAG_ADEN)) {/* Wait the ADCEN falg */
            break;
        }
    }
    OS_ASSERT((ct_delay < 0x1000), RETURN_VOID);
    
    ADC_StartOfConversion((ADC_TypeDef *)pinfo->adc_base);                     /* ADC1 regular Software Start Conv */
}

/*******************************************************************
** 函数名称: ST_ADC_InitDrv
** 函数描述: 初始化模块
** 参数:     无
** 返回:     无
********************************************************************/
void ST_ADC_InitDrv(void)
{
    YX_MEMSET(&s_dcb, 0, sizeof(s_dcb));
    
    ADC_StartConvert();
}

/*******************************************************************
** 函数名称: ST_ADC_GetValue
** 函数描述: 获取ADC值,单位:mV
** 参数:     [in] ch: 通道号,见 ADC_CH_E
** 返回:     返回AD值
********************************************************************/
INT32S ST_ADC_GetValue(INT8U ch)
{
    if (ch < ADC_CH_MAX + 3) {
        return ((3300 * s_dcb.value[ch]) >> 12);
    } else {
        return 0;
    }
}

/**************************** (C) COPYRIGHT 2012  XIAMEN YAXON.LTD **************END OF FILE******/

