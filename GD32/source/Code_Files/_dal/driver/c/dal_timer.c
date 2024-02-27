/********************************************************************************
**
** 文件名:     dal_timer.c
** 版权所有:   (c) 2014-2020 厦门雅迅网络股份有限公司
** 文件描述:   系统定时器驱动接口函数
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2015/12/30 | JUMP   | 创建本模块
**
*********************************************************************************/
#include "dal_timer.h"

/*******************************************************************
** 函数名:     TimerX_Func_Enable
** 函数描述:   定时器X功能使能
** 参数:       [in] index 定时器序号
** 返回:       无
********************************************************************/
void TimerX_Func_Enable(TIMER_NO_E index)
{
     switch (index)
    {
        case TIMER_NO_1:
            timer_enable(TIMER0);                                   /* TIM enable counter */
            break;

        case TIMER_NO_2:
            timer_enable(TIMER1);                                   /* TIM enable counter */
            break;
            
        case TIMER_NO_3:
            timer_enable(TIMER2);                                   /* TIM enable counter */
            break;
            
        case TIMER_NO_4:
            timer_enable(TIMER3);                                   /* TIM enable counter */
            break;
            
        case TIMER_NO_5:
            timer_enable(TIMER4);                                   /* TIM enable counter */
            break;
            
        case TIMER_NO_6:
            timer_enable(TIMER5);                                   /* TIM enable counter */
            break;
            
        case TIMER_NO_7:
            timer_enable(TIMER6);                                   /* TIM enable counter */
            break;

        default:
            break;
    }
}

/*******************************************************************
** 函数名:     TimerX_IT_Enable
** 函数描述:   定时器X中断使能,如果要使用中断，则必须打开功能使能
** 参数:       [in] index 定时器序号
** 返回:       无
********************************************************************/
void TimerX_IT_Enable(TIMER_NO_E index)
{
		switch(index){
    case TIMER_NO_1:
            timer_interrupt_enable(TIMER0, TIMER_INT_UP);               /* TIM IT enable */
            break;

        case TIMER_NO_2:
            timer_interrupt_enable(TIMER1, TIMER_INT_UP);               /* TIM IT enable */
            break;
            
        case TIMER_NO_3:
            timer_interrupt_enable(TIMER2, TIMER_INT_UP);               /* TIM IT enable */
            break;
            
        case TIMER_NO_4:
            timer_interrupt_enable(TIMER3, TIMER_INT_UP);               /* TIM IT enable */
            break;
            
        case TIMER_NO_5:
            timer_interrupt_enable(TIMER4, TIMER_INT_UP);               /* TIM IT enable */
            break;
            
        case TIMER_NO_6:
            timer_interrupt_enable(TIMER5, TIMER_INT_UP);               /* TIM IT enable */
            break;
            
        case TIMER_NO_7:
            timer_interrupt_enable(TIMER6, TIMER_INT_UP);               /* TIM IT enable */
            break;

        default:
            break;
    }
}

/*******************************************************************
** 函数名:     TimerX_Disable
** 函数描述:   定时器X禁能
** 参数:       [in] index 定时器序号
** 返回:       无
********************************************************************/
void TimerX_Disable(TIMER_NO_E index)
{
    switch (index)
    {
        case TIMER_NO_1:
            timer_enable(TIMER0);                                  /* TIM enable counter */
            timer_interrupt_disable(TIMER0, TIMER_INT_UP);              /* TIM IT enable */
            break;

        case TIMER_NO_2:
            timer_enable(TIMER1);                                  /* TIM enable counter */
            timer_interrupt_disable(TIMER1, TIMER_INT_UP);              /* TIM IT enable */
            break;
            
        case TIMER_NO_3:
            timer_enable(TIMER2);                                  /* TIM enable counter */
            timer_interrupt_disable(TIMER2, TIMER_INT_UP);              /* TIM IT enable */
            break;
            
        case TIMER_NO_4:
            timer_enable(TIMER3);                                  /* TIM enable counter */
            timer_interrupt_disable(TIMER3, TIMER_INT_UP);              /* TIM IT enable */
            break;
            
        case TIMER_NO_5:
            timer_enable(TIMER4);                                  /* TIM enable counter */
            timer_interrupt_disable(TIMER4, TIMER_INT_UP);              /* TIM IT enable */
            break;
            
        case TIMER_NO_6:
            timer_enable(TIMER5);                                  /* TIM enable counter */
            timer_interrupt_disable(TIMER5, TIMER_INT_UP);              /* TIM IT enable */
            break;
            
        case TIMER_NO_7:
            timer_enable(TIMER6);                                  /* TIM enable counter */
            timer_interrupt_disable(TIMER6, TIMER_INT_UP);              /* TIM IT enable */
            break;

        default:
            break;
    }
}

/*******************************************************************
** 函数名:     TimerX_Initiate
** 函数描述:   定时器X初始化 - 设置成每1毫秒中断一次，系统时钟168M, 定时器时钟总线84M, 根据数据手册:
**             定时器的时钟频率为168M, Prescaler = 167, 则定时器的计数器时钟 168M / (167 + 1) = 1M
**             Period = 999 所以定时器计数到中断频率为 1M / (999 + 1) = 1K
** 参数:       [in] index 定时器序号
** 返回:       无
**  注意事项:  RCC_APB2PeriphClockCmd语句一定要置于其他初始化指令之前
********************************************************************/
void TimerX_Initiate(TIMER_NO_E index)
{
    
	timer_parameter_struct	ICB;
	
		ICB.period			  = 999;
		ICB.prescaler		  = 119;
		ICB.clockdivision	  = TIMER_CKDIV_DIV1;
		ICB.counterdirection	   = TIMER_COUNTER_UP;
		ICB.repetitioncounter = 0;
	  
		switch (index)
		{
			case TIMER_NO_1:
				rcu_periph_clock_enable(RCU_TIMER0);
				timer_init(TIMER0, &ICB);
				break;
	
			case TIMER_NO_2:
				rcu_periph_clock_enable(RCU_TIMER1);
				timer_init(TIMER1, &ICB);
				break;
				
			case TIMER_NO_3:
				rcu_periph_clock_enable(RCU_TIMER2);
				timer_init(TIMER2, &ICB);
				break;
				
			case TIMER_NO_4:
				rcu_periph_clock_enable(RCU_TIMER3);
				timer_init(TIMER3, &ICB);
				break;
				
			case TIMER_NO_5:
				rcu_periph_clock_enable(RCU_TIMER4);
				timer_init(TIMER4, &ICB);
				break;
				
			case TIMER_NO_6:
				rcu_periph_clock_enable(RCU_TIMER5);
				timer_init(TIMER5, &ICB);
				break;
				
			case TIMER_NO_7:
				rcu_periph_clock_enable(RCU_TIMER6);
				timer_init(TIMER6, &ICB);
				break;
	
			default:
				break;
		}
		TimerX_Disable(index);											 /* 初始化后是关闭功能的，要使用必须开启 */
                                             /* 初始化后是关闭功能的，要使用必须开启 */
}

/*******************************************************************
** 函数名:     Get_TimerX_Counter
** 函数描述:   获取定时器x计数器的数值
** 参数:       [in] index 定时器序号
** 返回:       返回该定时器计数器的数值
********************************************************************/
INT16U Get_TimerX_Counter(TIMER_NO_E index)
{
    INT16U counter;
    
    switch (index) {
        case TIMER_NO_1:
            counter = timer_counter_read(TIMER0);
            break;
        case TIMER_NO_2:
            counter = timer_counter_read(TIMER1);
            break;
        case TIMER_NO_3:
            counter = timer_counter_read(TIMER2);
            break;
        case TIMER_NO_4:
            counter = timer_counter_read(TIMER3);
            break;
        case TIMER_NO_5:
            counter = timer_counter_read(TIMER4);
            break;
        case TIMER_NO_6:
            counter = timer_counter_read(TIMER5);
            break;
        case TIMER_NO_7:
            counter = timer_counter_read(TIMER6);
            break;
        default :
            counter = 0;
            break;
    }
    return counter;
}
/**************************** (C) COPYRIGHT 2010  XIAMEN YAXON.LTD **************END OF FILE******/

