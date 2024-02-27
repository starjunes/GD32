/********************************************************************************
**
** 文件名:     dal_gpio.c
** 版权所有:   (c) 2014-2020 厦门雅迅网络股份有限公司
** 文件描述:   GPIO驱动接口函数
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
#include  "dal_gpio.h"
  

/*******************************************************************
** 函数名:     CreateInputPort
** 函数描述:   创建输入管脚
** 参数:       [in] pgrp  管脚端口,如GPIOA,GPIOB,GPIOC,GPIOD,GPIOE等 
**             [in] pnum  管脚号
**             [in] mode  输入的模式
**             [in] lock  是否锁住配置
** 返回:       无
********************************************************************/
void CreateInputPort(INT32U pgrp, INT16U pnum, INT32U mode, BOOLEAN lock)
{
	/* Configure port as input */
    gpio_init(pgrp, mode, GPIO_OSPEED_2MHZ, pnum);
    if (lock) {
        gpio_pin_lock(pgrp, pnum);
    } 
}

/*******************************************************************
** 函数名:     CreateOutputPort
** 函数描述:   创建输出管脚
** 参数:       [in] pgrp  管脚端口,如GPIOA,GPIOB,GPIOC,GPIOD,GPIOE等 
**             [in] pnum  管脚号 GPIO_PIN_x	(x=0,1,2....15)
**             [in] mode  输出的模式
**             [in] speed  :输出的速率
**             [in] lock  是否锁住配置
** 返回:       无
********************************************************************/
void CreateOutputPort(INT32U  pgrp, INT16U pnum, INT32U mode, INT32U speed, BOOLEAN lock)
{
    gpio_init(pgrp, mode, speed, pnum);

	 if (lock) {
       gpio_pin_lock(pgrp, pnum);
     } 
}
 
/*******************************************************************
** 函数名:     ReadInputPort
** 函数描述:   读取输入管脚状态
** 参数:       [in] pgrp  管脚端口,如GPIOA,GPIOB,GPIOC,GPIOD,GPIOE等 
**             [in] pnum  管脚号
** 返回:       高电平情况下返回TRUE，低电平情况下返回FALSE
********************************************************************/
BOOLEAN  ReadInputPort(INT32U pgrp, INT16U pnum)
{
    if (SET == gpio_input_bit_get(pgrp, pnum)) return TRUE;
      return FALSE;
}


/*******************************************************************
** 函数名:     ReadOutputPort
** 函数描述:   读取输出管脚状态
** 参数:       [in] pgrp  管脚端口,如GPIOA,GPIOB,GPIOC,GPIOD,GPIOE等 
**             [in] pnum  管脚号
** 返回:       高电平情况下返回TRUE，低电平情况下返回FALSE
********************************************************************/
BOOLEAN  ReadOutputPort(INT32U pgrp, INT16U pnum)
{
    if (SET == gpio_output_bit_get(pgrp, pnum)) return TRUE;
      return FALSE;
}

/*******************************************************************
** 函数名:     WriteOutputPort
** 函数描述:   写输出管脚
** 参数:       [in] pgrp    管脚端口,如GPIOA,GPIOB,GPIOC,GPIOD,GPIOE等 
**             [in] pnum    管脚号
**             [in] ishigh  TRUE表示输出高电平，FALSE表示输出低电平 
** 返回:       无
********************************************************************/
void WriteOutputPort(INT32U  pgrp, INT16U pnum, BOOLEAN ishigh)
{
    if (ishigh) {
        gpio_bit_set(pgrp, pnum);
     } else {
        gpio_bit_reset(pgrp, pnum);
     }
}

/*******************************************************************
** 函数名:     Read_InputGPIOxData
** 函数描述:   读取某个GPIO管脚组的输入寄存器的数据
** 参数:       [in] pgrp    管脚端口,如GPIOA,GPIOB,GPIOC,GPIOD,GPIOE等 
** 返回:       对应管脚组16个管脚的当前输入寄存器的数值
********************************************************************/
INT16U Read_InputGPIOxData(INT32U  pgrp)
{
    return gpio_input_port_get(pgrp);
}

/*******************************************************************
** 函数名:     Read_OutPutGPIOxData
** 函数描述:   读取某个GPIO管脚组的输出寄存器的数据
** 参数:       [in] pgrp    管脚端口,如GPIOA,GPIOB,GPIOC,GPIOD,GPIOE等 
** 返回:       对应管脚组16个管脚的当前输出寄存器的数值
********************************************************************/
INT16U Read_OutPutGPIOxData(INT32U  pgrp)
{
    return gpio_output_port_get(pgrp);
}

/*******************************************************************
** 函数名:     Write_GPIOx
** 函数描述:   对某个GPIO管脚组的输出寄存器进行赋值
** 参数:       [in] pgrp       管脚端口,如GPIOA,GPIOB,GPIOC,GPIOD,GPIOE等
**             [in] portvalue  16个管脚寄存器的值，注意，如果没有用到所有管脚，那么其他管脚的值必须保留
** 返回:       无
********************************************************************/
void Write_GPIOx(INT32U  pgrp, INT16U portvalue)
{
     gpio_port_write(pgrp, portvalue);
}

/*******************************************************************
** 函数名:     dal_gpio_getsource
** 函数描述:   转化为pinsource
** 参数:       [in] pnum  管脚号
** 返回:       
********************************************************************/
INT8U  dal_gpio_getsource(INT16U pnum)
{
    INT8U pinsource;
    
    for (pinsource = 0; pinsource < 0x0F; pinsource++) {
        if (pnum & 0x0001) {
            break;
        }
        pnum >>= 1;
    }
    return pinsource;
}


/*******************************************************************
** 函数名:     DAL_GpioPortCfg
** 函数描述:   引脚配置
** 参数:       [in] pgrp  管脚端口,如GPIOA,GPIOB,GPIOC,GPIOD,GPIOE等 
**             [in] pnum  管脚号
**             [in] mode  输出的模式
**             [in] speed  :输出的速率
**             [in] lock  是否锁住配置
** 返回:       无
********************************************************************/
void DAL_GpioPortCfg(INT32U  pgrp, INT16U pnum, INT32U mode, INT32U speed, BOOLEAN lock)
{
	 gpio_init(pgrp,mode,speed,pnum);
    //if (lock) {
    //    GPIO_PinLockConfig(pgrp, pnum);
    //} 
}

/************************** (C) COPYRIGHT 2010 XIAMEN YAXON.LTD *****************END OF FILE******/

