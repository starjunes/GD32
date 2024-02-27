/********************************************************************************
**
** 文件名:     dal_buzzer.c
** 版权所有:   (c) 2014-2020 厦门雅迅网络股份有限公司
** 文件描述:   蜂鸣器模块驱动层接口函数
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2015/12/29 | JUMP   | 创建本模块
**
*********************************************************************************/
#include  "dal_buzzer.h"
#include  "dal_gpio.h"  
#include  "dal_pinlist.h"

#define  TIM2_PWM_PRES           119    
#define  TIM2_BASEPWM_FREQ       (SYS_CLK_FREQ / (TIM2_PWM_PRES + 1))         /* 预分频84 TIM计数时钟84M*2/(73+1)=1M  */
#define  DEFAULT_FREQ            1000                                           /* 默认频率1K */
 
static timer_parameter_struct    TIM_TimeBaseStructure;
static timer_oc_parameter_struct TIM_OCInitStructure;



/*******************************************************************
** 函数名:     buzzer0_port_config
** 函数描述:   蜂鸣器端口管脚配置
** 参数:       无
** 返回:       无
********************************************************************/
static void buzzer0_port_config(void)
{
   CreateOutputPort(BUZZER0_PORT,BUZZER0_PIN, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, TRUE);    /* 采用PWM输出驱动蜂鸣器 */  
   gpio_pin_remap_config(GPIO_TIMER2_FULL_REMAP,ENABLE);
}

/*******************************************************************
** 函数名:     buzzer0_pwm_para_config
** 函数描述:   蜂鸣器PWM驱动参数配置
** 参数:       [in] freq               频率
** 返回:       无
********************************************************************/
static void buzzer0_pwm_para_config(INT16U freq)
{
    INT16U  temp;

    temp = TIM2_BASEPWM_FREQ / freq;
     
    TIM_TimeBaseStructure.period = (temp - 1);
    TIM_TimeBaseStructure.prescaler = TIM2_PWM_PRES;  
    TIM_TimeBaseStructure.alignedmode = TIMER_COUNTER_EDGE;
    TIM_TimeBaseStructure.clockdivision = TIMER_CKDIV_DIV1;
    TIM_TimeBaseStructure.counterdirection = TIMER_COUNTER_UP;
    TIM_TimeBaseStructure.repetitioncounter = 0;
    timer_init(TIMER2, &TIM_TimeBaseStructure);

    TIM_OCInitStructure.ocpolarity   = TIMER_OC_POLARITY_HIGH;
    TIM_OCInitStructure.outputstate  = TIMER_CCX_DISABLE;
    TIM_OCInitStructure.ocnpolarity  = TIMER_OCN_POLARITY_HIGH;
    TIM_OCInitStructure.outputnstate = TIMER_CCXN_DISABLE;
    TIM_OCInitStructure.ocidlestate  = TIMER_OC_IDLE_STATE_LOW;
    TIM_OCInitStructure.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;
    timer_channel_output_config(TIMER2,TIMER_CH_2,&TIM_OCInitStructure);

    timer_channel_output_pulse_value_config(TIMER2,TIMER_CH_2,temp / 2);
    timer_channel_output_mode_config(TIMER2,TIMER_CH_2,TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER2,TIMER_CH_2,TIMER_OC_SHADOW_DISABLE);

    /* auto-reload preload enable */
    timer_auto_reload_shadow_disable(TIMER2);
    /* auto-reload preload enable */
    timer_disable(TIMER2);
}

/*******************************************************************
** 函数名:     Buzzer0_Init
** 函数描述:   蜂鸣器模块初始化,默认为1K的频率
** 参数:       无
** 返回:       无
********************************************************************/
void Buzzer0_Init(void)
{
	rcu_periph_clock_enable(RCU_TIMER2);
    timer_deinit(TIMER2);
    buzzer0_port_config();
    buzzer0_pwm_para_config(DEFAULT_FREQ);
    Buzzer0_PWM_Control(FALSE);
    Buzzer0_PWM_Control(FALSE);
}

/*******************************************************************
** 函数名:     Buzzer0_Freq_Set
** 函数描述:   设置蜂鸣器的驱动频率
** 参数:       [in] freqvalue          频率
** 返回:       无
********************************************************************/
void Buzzer0_Freq_Set(INT16U freqvalue)
{
    INT16U  temp;

    temp = TIM2_BASEPWM_FREQ / freqvalue;

    TIM_TimeBaseStructure.period = (temp - 1); 
    timer_init(TIMER2, &TIM_TimeBaseStructure);
    timer_channel_output_pulse_value_config(TIMER2,TIMER_CH_2,temp / 2);
}

/*******************************************************************
** 函数名:     Buzzer0_PWM_Control
** 函数描述:   蜂鸣器PWM驱动开关控制
** 参数:       [in] onoff    TRUE为开启PWM驱动输出,FALSE为关闭PWM驱动输出
** 返回:       无
********************************************************************/
void Buzzer0_PWM_Control(BOOLEAN onoff)
{
    if (TRUE == onoff) {    /* 打开PWM输出 */
        TIM_OCInitStructure.outputstate = TIMER_CCX_ENABLE;
        timer_channel_output_config(TIMER2,TIMER_CH_2,&TIM_OCInitStructure);
        timer_channel_output_shadow_config(TIMER2,TIMER_CH_2,TIMER_OC_SHADOW_DISABLE);
        /* auto-reload preload enable */
        timer_auto_reload_shadow_enable(TIMER2);
        /* auto-reload preload enable */
        timer_enable(TIMER2);
    } else {                /* 关闭PWM输出 */
        TIM_OCInitStructure.outputstate = TIMER_CCX_DISABLE;
        timer_channel_output_config(TIMER2,TIMER_CH_2,&TIM_OCInitStructure);
        timer_channel_output_shadow_config(TIMER2,TIMER_CH_2,TIMER_OC_SHADOW_DISABLE);
        /* auto-reload preload enable */
        timer_auto_reload_shadow_disable(TIMER2);
        /* auto-reload preload enable */
        timer_disable(TIMER2);
    }
}

/**************************** (C) COPYRIGHT 2010  XIAMEN YAXON.LTD **************END OF FILE******/

