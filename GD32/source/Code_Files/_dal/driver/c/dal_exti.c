/**************************************************************************************************
**                                                                                               **
**  文件名称:  dal_Exti.c                                                                        **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2010                                    **
**  创建信息:  Lantu.Cai -- 2010年12月3日                                                        **
**  文件描述:  外部中断接口函数                                                                  **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/ 
#include  "dal_exti.h"
#include  "man_irq.h"

/**************************************************************************************************
**  函数名称:  Exit_GPIO_Configuration
**  功能描述:  外部中断管脚配置
**  输入参数:  grp : 管脚端口
**          :  pin : 管脚号
**  返回参数:  None
**************************************************************************************************/
static void Exit_GPIO_Configuration(PORT_GRP_E grp, PORT_IDX_E pin)
{
      INT16U gpio_pin;
      INT32U gpio;
      
      DAL_ASSERT(grp < MAX_GRP);
      DAL_ASSERT(pin < MAX_PIN);
      
      switch(grp)
      {
        case GRP_A:
          gpio = GPIOA;
          break;
        case GRP_B:
          gpio = GPIOB;
          break;
        case GRP_C:
          gpio = GPIOC;
          break;
        case GRP_D:
          gpio = GPIOD;
          break;
        case GRP_E:
          gpio = GPIOE;          
          break;
        default:
          gpio = GPIOA;
          DAL_ASSERT(0);
          break;
      }
      
      switch(pin)
      {
         case PIN_0:
           gpio_pin = GPIO_PIN_0;
           break;
         case PIN_1:
           gpio_pin = GPIO_PIN_1;
           break; 
         case PIN_2:
           gpio_pin = GPIO_PIN_2;
           break; 
         case PIN_3:
           gpio_pin = GPIO_PIN_3;
           break;
         case PIN_4:
           gpio_pin = GPIO_PIN_4;
           break;
         case PIN_5:
           gpio_pin = GPIO_PIN_5;
           break;
         case PIN_6:
           gpio_pin = GPIO_PIN_6;
           break;
         case PIN_7:
           gpio_pin = GPIO_PIN_7;
           break; 
         case PIN_8:
           gpio_pin = GPIO_PIN_8;
           break; 
         case PIN_9:
           gpio_pin = GPIO_PIN_9;
           break;
         case PIN_10:
           gpio_pin = GPIO_PIN_10;
           break;
         case PIN_11:
           gpio_pin = GPIO_PIN_11;
           break;
         case PIN_12:
           gpio_pin = GPIO_PIN_12;
           break;
         case PIN_13:
           gpio_pin = GPIO_PIN_13;
           break;
         case PIN_14:
           gpio_pin = GPIO_PIN_14;
           break;
         case PIN_15:
           gpio_pin = GPIO_PIN_15;
           break;
         default:
           gpio_pin = GPIO_PIN_0;
           DAL_ASSERT(0);
           break;
      }
      
	  gpio_init(gpio, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, gpio_pin);
} 
 
/**************************************************************************************************
**  函数名称:  GPIO_ExtiLineConfiguartion
**  功能描述:  链接外部中断到对应管脚
**  输入参数:  grp : 管脚端口
**          :  pin : 管脚号
**  返回参数:  None
**************************************************************************************************/
static void GPIO_ExtiLineConfiguartion(PORT_GRP_E grp, PORT_IDX_E pin)
{
      INT8U  port_source,pin_source;
      
      DAL_ASSERT(grp < MAX_GRP);
      DAL_ASSERT(pin < MAX_PIN);
      
      switch(grp)
      {
        case GRP_A:
          port_source = GPIO_PORT_SOURCE_GPIOA;
          break;
        case GRP_B:
          port_source = GPIO_PORT_SOURCE_GPIOB;
          break;
        case GRP_C:
          port_source = GPIO_PORT_SOURCE_GPIOC;
          break;
        case GRP_D:
          port_source = GPIO_PORT_SOURCE_GPIOD;
          break;
        case GRP_E:
          port_source = GPIO_PORT_SOURCE_GPIOE;          
          break;
        default:
          port_source = GPIO_PORT_SOURCE_GPIOA;
          DAL_ASSERT(0);
          break;
      }
      
      switch(pin)
      {
        case PIN_0:
          pin_source = GPIO_PIN_SOURCE_0;
          break;
        case PIN_1:
          pin_source = GPIO_PIN_SOURCE_1;
          break;
        case PIN_2:
          pin_source = GPIO_PIN_SOURCE_2;
          break;
        case PIN_3:
          pin_source = GPIO_PIN_SOURCE_3;
          break;
        case PIN_4:
          pin_source = GPIO_PIN_SOURCE_4;
          break;
        case PIN_5:
          pin_source = GPIO_PIN_SOURCE_5;
          break;
        case PIN_6:
          pin_source = GPIO_PIN_SOURCE_6;
          break;
        case PIN_7:
          pin_source = GPIO_PIN_SOURCE_7;
          break;
        case PIN_8:
          pin_source = GPIO_PIN_SOURCE_8;
          break;
        case PIN_9:
          pin_source = GPIO_PIN_SOURCE_9;
          break;
        case PIN_10:
          pin_source = GPIO_PIN_SOURCE_10;
          break;
        case PIN_11:
          pin_source = GPIO_PIN_SOURCE_11;
          break;
        case PIN_12:
          pin_source = GPIO_PIN_SOURCE_12;
          break;
        case PIN_13:
          pin_source = GPIO_PIN_SOURCE_13;
          break;
        case PIN_14:
          pin_source = GPIO_PIN_SOURCE_14;
          break;
        case PIN_15:
          pin_source = GPIO_PIN_SOURCE_15;
          break;
        default:
          pin_source = GPIO_PIN_SOURCE_0;
          DAL_ASSERT(0);
          break;
      }
      
      gpio_exti_source_select(port_source, pin_source);
}


/**************************************************************************************************
**  函数名称:  EXIT_Configuartion
**  功能描述:  配置外部中断及触发方式
**  输入参数:  grp       : 管脚端口
**          :  pin       : 管脚号
**          :  trig_mode : 触发方式
**  返回参数:  中断号
**************************************************************************************************/
INT8U EXIT_Configuartion(PORT_GRP_E grp, PORT_IDX_E pin, TRIG_MODE_E trig_mode, BOOLEAN enable)
{
      INT8U   irq;
      exti_line_enum  exti_line;
      exti_trig_type_enum  trig_type;
      
      DAL_ASSERT(grp < MAX_GRP);
      DAL_ASSERT(pin < MAX_PIN);
      DAL_ASSERT(trig_mode < MAX_TRIG_MODE);
      
      grp = grp;
      
      switch(pin)
      {
        case PIN_0:
          irq = EXTI0_IRQ;
          exti_line = EXTI_0;
          break;
        case PIN_1:
          irq = EXTI1_IRQ;
          exti_line = EXTI_1;
          break;
        case PIN_2:
          irq = EXTI2_IRQ;
          exti_line = EXTI_2;
          break;
        case PIN_3:
          irq = EXTI3_IRQ;
          exti_line = EXTI_3;
          break;
        case PIN_4:
          irq = EXTI4_IRQ;
          exti_line = EXTI_4;
          break;
        case PIN_5:
          irq = EXTI5_9_IRQ;
          exti_line = EXTI_5;
          break;
        case PIN_6:
          irq = EXTI5_9_IRQ;
          exti_line = EXTI_6;
          break;
        case PIN_7:
          irq = EXTI5_9_IRQ;
          exti_line = EXTI_7;
          break;
        case PIN_8:
          irq = EXTI5_9_IRQ;
          exti_line = EXTI_8;
          break;
        case PIN_9:
          irq = EXTI5_9_IRQ;
          exti_line = EXTI_9;
          break;
        case PIN_10:
          irq = EXTI10_15_IRQ;
          exti_line = EXTI_10;
          break;
        case PIN_11:
          irq = EXTI10_15_IRQ;
          exti_line = EXTI_11;
          break;
        case PIN_12:
          irq = EXTI10_15_IRQ;
          exti_line = EXTI_12;
          break;
        case PIN_13:
          irq = EXTI10_15_IRQ;
          exti_line = EXTI_13;
          break;
        case PIN_14:
          irq = EXTI10_15_IRQ;
          exti_line = EXTI_14;
          break;
        case PIN_15:
          irq = EXTI10_15_IRQ;
          exti_line = EXTI_15;
          break;
        default:
          irq = EXTI0_IRQ;
          exti_line = EXTI_15;
          DAL_ASSERT(0);
          break;
      }
      
      switch(trig_mode)
      {
         case TRIG_RAISING:
           trig_type = EXTI_TRIG_RISING;
           break;
         case TRIG_FAILING:
           trig_type = EXTI_TRIG_FALLING;
           break;
         case TRIG_RAISING_FAILING:
           trig_type = EXTI_TRIG_BOTH;
           break;
         default:
           trig_type = EXTI_TRIG_RISING;
           DAL_ASSERT(0);
           break;
      }
	  
	  
	  if (enable == ENABLE){
		 exti_init(exti_line, EXTI_INTERRUPT, trig_type);
		 exti_interrupt_enable(exti_line);
	  }else {
		 exti_interrupt_disable(exti_line);
		 exti_interrupt_flag_clear(exti_line);
	  }
      
	  return irq;
}

/**************************************************************************************************
**  函数名称:  CreateExtIntLine
**  功能描述:  创建外部中断
**  输入参数:  grp         : 管脚端口
**          :  pin         : 管脚号
**          :  trig_mode   : 触发方式
**          :  callback    : 中断处理函数
**  返回参数:  None
**************************************************************************************************/
void CreateExtIntLine(PORT_GRP_E grp, PORT_IDX_E pin, TRIG_MODE_E trig_mode, CALLBACK_EXTI callback)
{
     INT8U exti_irq;
     DAL_ASSERT(grp < MAX_GRP);
     DAL_ASSERT(pin < MAX_PIN);
     DAL_ASSERT(trig_mode < MAX_TRIG_MODE);
     
     
     /* Configure the GPIO ports */
     Exit_GPIO_Configuration(grp, pin);                        
     
     /* Enable AFIO clock */
     rcu_periph_clock_enable(RCU_AF);  /* 开启GPIO系统时钟 */
     
     /* Connect EXTI Line to  GPIO Pin */
     GPIO_ExtiLineConfiguartion(grp, pin);
     
     /* Configure EXTI Line to generate an interrupt on trig_mode */ 
     exti_irq = EXIT_Configuartion(grp, pin, trig_mode, ENABLE);
     
     /* Install irq handle and set irq priotity,and enable the irq channel */
     NVIC_IrqHandleInstall(exti_irq, callback, EXTI_PRIOTITY, TRUE);  
}



  /*******************************************************************
** 函数名:     SetExtIntLine
** 函数描述:   创建外部中断
** 参数:       [in] grp:       管脚端口
**             [in] pin:       管脚号
**             [in] trig_mode: 触发方式
** 返回:       无
********************************************************************/
void SetExtIntLine(PORT_GRP_E grp, PORT_IDX_E pin, TRIG_MODE_E trig_mode)
{
    EXIT_Configuartion(grp, pin, trig_mode, ENABLE);
}

/*******************************************************************
** 函数名:     ClearExtIntLine
** 函数描述:   清除外部中断
** 参数:       [in] grp:       管脚端口
**             [in] pin:       管脚号
**             [in] trig_mode: 触发方式
** 返回:       无
********************************************************************/
void ClearExtIntLine(PORT_GRP_E grp, PORT_IDX_E pin, TRIG_MODE_E trig_mode)
{
    EXIT_Configuartion(grp, pin, trig_mode, DISABLE);
}
/**************************************************************************************************
**  函数名称:  GetExtiItStatus
**  功能描述:  获取外部中断状态
**  输入参数:  grp         : 管脚端口
**          :  pin         : 管脚号
**  返回参数:  中断响应返回TRUE,无响应返回FALSE
**************************************************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) BOOLEAN GetExtiItStatus(PORT_GRP_E grp, PORT_IDX_E pin)
{
      
      exti_line_enum  exti_line;

      DAL_ASSERT(grp < MAX_GRP);
      DAL_ASSERT(pin < MAX_PIN);
      
      grp = grp;
      
      switch(pin)
      {
        case PIN_0:
          exti_line = EXTI_0;
          break;
        case PIN_1:
          exti_line = EXTI_1;
          break;
        case PIN_2:
          exti_line = EXTI_2;
          break;
        case PIN_3:
          exti_line = EXTI_3;
          break;
        case PIN_4:
          exti_line = EXTI_4;
          break;
        case PIN_5:
          exti_line = EXTI_5;
          break;
        case PIN_6:
          exti_line = EXTI_6;
          break;
        case PIN_7:
          exti_line = EXTI_7;
          break;
        case PIN_8:
          exti_line = EXTI_8;
          break;
        case PIN_9:
          exti_line = EXTI_9;
          break;
        case PIN_10:
          exti_line = EXTI_10;
          break;
        case PIN_11:
          exti_line = EXTI_11;
          break;
        case PIN_12:
          exti_line = EXTI_12;
          break;
        case PIN_13:
          exti_line = EXTI_13;
          break;
        case PIN_14:
          exti_line = EXTI_14;
          break;
        case PIN_15:
          exti_line = EXTI_15;
          break;
        default:
          exti_line = EXTI_0;
          DAL_ASSERT(0);
          break;
      }
      
      if (exti_interrupt_flag_get(exti_line) != RESET) return TRUE;
      return FALSE;
}

/**************************************************************************************************
**  函数名称:  ClrExtiITPendingBit
**  功能描述:  清除寄存器中的外部中断置位
**  输入参数:  grp         : 管脚端口
**          :  pin         : 管脚号
**  返回参数:  None
**************************************************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void ClrExtiITPendingBit(PORT_GRP_E grp, PORT_IDX_E pin)
{
      exti_line_enum  exti_line;
      
      DAL_ASSERT(grp < MAX_GRP);
      DAL_ASSERT(pin < MAX_PIN);
      grp = grp;
      
      switch(pin)
      {
        case PIN_0:
          exti_line = EXTI_0;
          break;
        case PIN_1:
          exti_line = EXTI_1;
          break;
        case PIN_2:
          exti_line = EXTI_2;
          break;
        case PIN_3:
          exti_line = EXTI_3;
          break;
        case PIN_4:
          exti_line = EXTI_4;
          break;
        case PIN_5:
          exti_line = EXTI_5;
          break;
        case PIN_6:
          exti_line = EXTI_6;
          break;
        case PIN_7:
          exti_line = EXTI_7;
          break;
        case PIN_8:
          exti_line = EXTI_8;
          break;
        case PIN_9:
          exti_line = EXTI_9;
          break;
        case PIN_10:
          exti_line = EXTI_10;
          break;
        case PIN_11:
          exti_line = EXTI_11;
          break;
        case PIN_12:
          exti_line = EXTI_12;
          break;
        case PIN_13:
          exti_line = EXTI_13;
          break;
        case PIN_14:
          exti_line = EXTI_14;
          break;
        case PIN_15:
          exti_line = EXTI_15;
          break;
        default:
          exti_line = EXTI_0;
          DAL_ASSERT(0);
          break;
      }
      
      exti_interrupt_flag_clear(exti_line);
}


/************************* (C) COPYRIGHT 2010 XIAMEN YAXON.LTD *********************END OF FILE******/
