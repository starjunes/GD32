/********************************************************************************
**
** 文件名:     dal_adc.c
** 版权所有:   (c) 2014-2020 厦门雅迅网络股份有限公司
** 文件描述:   ADC驱动接口函数
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2015/12/29 | JUMP   | 创建本模块
**
*********************************************************************************/
#include  "dal_include.h"
#include  "dal_adc.h"
#include  "dal_pinlist.h"

#if EN_ADC > 0
static volatile INT16U  s_adcvalue[ADC_MAX];
 
/*******************************************************************
** 函数名:     adc_gpio_configuration
** 函数描述:   ADC管脚初始化
** 参数:       无
** 返回:       无
********************************************************************/
static void adc_gpio_configuration(void)
{    
	/* 配置成模拟输入 */

	gpio_init(USER0_ADC_GPIOx,   GPIO_MODE_AIN, GPIO_OSPEED_50MHZ, USER0_ADC_Pin);
    gpio_init(USER1_ADC_GPIOx,   GPIO_MODE_AIN, GPIO_OSPEED_50MHZ, USER1_ADC_Pin);
	gpio_init(VIN_ADC_GPIOx	 ,   GPIO_MODE_AIN, GPIO_OSPEED_50MHZ, VIN_ADC_Pin);
}

/*******************************************************************
** 函数名:     adc_dma_configuration
** 函数描述:   ADC对应的DMA初始化
** 参数:       无
** 返回:       无
********************************************************************/
static void adc_dma_configuration(void)
{
    dma_parameter_struct dma_data_parameter;
    rcu_periph_clock_enable(RCU_DMA0);
    /* ADC DMA_channel configuration */
    dma_deinit(DMA0, DMA_CH0);
    
    /* initialize DMA single data mode */
    dma_data_parameter.periph_addr  = (INT32U)(&ADC_RDATA(ADC0));
    dma_data_parameter.periph_inc   = DMA_PERIPH_INCREASE_DISABLE;
    dma_data_parameter.memory_addr  = (INT32U)s_adcvalue;
    dma_data_parameter.memory_inc   = DMA_MEMORY_INCREASE_ENABLE;
    dma_data_parameter.periph_width = DMA_PERIPHERAL_WIDTH_16BIT;
    dma_data_parameter.memory_width = DMA_MEMORY_WIDTH_16BIT;  
    dma_data_parameter.direction    = DMA_PERIPHERAL_TO_MEMORY;
    dma_data_parameter.number       = ADC_MAX ;
    dma_data_parameter.priority     = DMA_PRIORITY_HIGH;
    dma_init(DMA0, DMA_CH0, &dma_data_parameter);

    /* enable DMA circulation mode */
    dma_circulation_enable(DMA0, DMA_CH0);

    dma_memory_to_memory_disable(DMA0, DMA_CH0);
  
    /* enable DMA channel */
    dma_channel_enable(DMA0, DMA_CH0);   
}

/*******************************************************************
** 函数名:     InitADC
** 函数描述:   初始化ADC模块
** 参数:       无
** 返回:       无
********************************************************************/
void InitADC(void)
{
   /* Configure analog input pins */
    adc_gpio_configuration();
     
    /* Configure ADC DMA Mode */
    adc_dma_configuration();
     
    /* ADC Periph clock enable */
    rcu_periph_clock_enable(RCU_ADC0);

    /* Configure the ADC1 in continous mode withe a resolutuion equal to 12 bits  */
    adc_deinit(ADC0);
    /* ADC mode config */
    adc_mode_config(ADC_MODE_FREE); 
    /* ADC continous function enable */
    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, ENABLE); 
    /* ADC scan function enable */
    adc_special_function_config(ADC0, ADC_SCAN_MODE, ENABLE);
    /* ADC data alignment config */
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);
    /* ADC channel length config */
    adc_channel_length_config(ADC0, ADC_REGULAR_CHANNEL, ADC_MAX );
    /* ADC external trigger enable */
    adc_external_trigger_source_config(ADC0, ADC_REGULAR_CHANNEL, ADC0_1_2_EXTTRIG_REGULAR_NONE);
    adc_external_trigger_config(ADC0, ADC_REGULAR_CHANNEL, ENABLE);
    /* configure ADC discontinuous mode */
    //adc_discontinuous_mode_config(ADC0, ADC_REGULAR_CHANNEL, ADC_CH_MAX);

//    adc_regular_channel_config(ADC0, 0, PRN_ADC_CH, ADC_SAMPLETIME_239POINT5);
    adc_regular_channel_config(ADC0, 0, USER0_ADC_CH, ADC_SAMPLETIME_28POINT5);
    adc_regular_channel_config(ADC0, 1, USER1_ADC_CH, ADC_SAMPLETIME_55POINT5);
    adc_regular_channel_config(ADC0, 2, VIN_ADC_CH  , ADC_SAMPLETIME_55POINT5);
    //adc_regular_channel_config(ADC0, 3, ADC_CH_RESERVE1, ADC_SAMPLETIME_239POINT5);

    /* enable ADC interface */
    adc_enable(ADC0);
    /* ADC calibration and reset calibration */
    adc_calibration_enable(ADC0);
    /* ADC DMA function enable */
    adc_dma_mode_enable(ADC0);   
    /* Start ADC1 Software Conversion */
    adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL);
}

/**************************************************************************************************
**  函数名称:  GetPrinterADCValue
**  功能描述:  获取ADC转换后的数值
**  输入参数:  None
**  返回参数:  s_adcvalue中ADC转换后的数值
**************************************************************************************************/
INT16U GetPrinterADCValue(void)
{
    //Debug_SysPrint("PrintAD = %d\r\n", s_adcvalue[0]);
    return (s_adcvalue[0]);
}

/*******************************************************************
** 函数名:     GetADCValue
** 函数描述:   获取ADC转换后的数值
** 参数:       [in] channel            所要获取的通道号
** 返回:       s_adcvalue中ADC转换后的数值
********************************************************************/
INT16U GetADCValue(INT8U channel)
{
	if (channel < ADC_MAX) {
        return ((3300 * s_adcvalue[channel]) >> 12);
    } else {
        return 0;
    }
}

#endif
/**************************** (C) COPYRIGHT 2010  XIAMEN YAXON.LTD **************END OF FILE******/
 
