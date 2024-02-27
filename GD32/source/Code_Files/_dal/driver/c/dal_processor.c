/********************************************************************************
**
** 文件名:     dal_processor.c
** 版权所有:   (c) 2014-2020 厦门雅迅网络股份有限公司
** 文件描述:   音效开关接口
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2015/12/30 | JUMP   | 创建本模块
**
*********************************************************************************/
#include  "dal_include.h"
#include  "dal_processor.h"
#include  "dal_pinlist.h"
#include  "dal_gpio.h"

/*************************************************************************************************/
/*                           宏定义                                                              */
/*************************************************************************************************/

#define P_SCL_H         GPIOA->BSRRL = GPIO_Pin_8
#define P_SCL_L         GPIOA->BSRRH = GPIO_Pin_8
    
#define P_SDA_H         GPIOC->BSRRL = GPIO_Pin_9
#define P_SDA_L         GPIOC->BSRRH = GPIO_Pin_9

#define P_SCL_read      GPIOA->IDR  & GPIO_Pin_8
#define P_SDA_read      GPIOC->IDR  & GPIO_Pin_9

#if 0
/*******************************************************************
** 函数名:     processor_i2c_delaytime
** 函数描述:   延时函数
** 参数:       无
** 返回:       无
********************************************************************/
static void processor_i2c_delaytime(INT32U ticks)
{
    while(ticks--);
}
#endif

/*******************************************************************
** 函数名:     processor_i2c_delay
** 函数描述:   延时函数
** 参数:       无
** 返回:       无
********************************************************************/
static void processor_i2c_delay(void)
{
    INT8U i = 20; //150 //这里可以优化速度 ，经测试最低到5还能写入 
    while(i--);
}

/*******************************************************************
** 函数名:     processor_i2c_star
** 函数描述:   发送启始位
** 参数:       无
** 返回:       无
********************************************************************/
static BOOLEAN processor_i2c_star(void)
{
    P_SDA_H;
    P_SCL_H;
    processor_i2c_delay(); 
    if (!P_SDA_read) {
        return false; //SDA线为低电平则总线忙,退出 
    }
    P_SDA_L;
    processor_i2c_delay();
    if (P_SDA_read) {
        return false; //SDA线为高电平则总线出错,退出
    }
    P_SDA_L;
    processor_i2c_delay();
    return true;
}

/*******************************************************************
** 函数名:     processor_i2c_stop
** 函数描述:   发送停止位
** 参数:       无
** 返回:       无
********************************************************************/
static void processor_i2c_stop(void)
{
    P_SCL_L;
    processor_i2c_delay();
    P_SDA_L;
    processor_i2c_delay();
    P_SCL_H;
    processor_i2c_delay();
    P_SDA_H;
    processor_i2c_delay();
}

#if 0
/*******************************************************************
** 函数名:     simi2c_ack
** 函数描述:   发送应答位
** 参数:       无
** 返回:       无
********************************************************************/
static void simi2c_ack(void)
{
    P_SCL_L;
    processor_i2c_delay();
    P_SDA_L;
    processor_i2c_delay();
    P_SCL_H;
    processor_i2c_delay();
    P_SCL_L;
    processor_i2c_delay();
}

/*******************************************************************
** 函数名:     simi2c_noack
** 函数描述:   不发送应答位
** 参数:       无
** 返回:       无
********************************************************************/
static void simi2c_noack(void)
{
    P_SCL_L;
    processor_i2c_delay();
    P_SDA_H;
    processor_i2c_delay();
    P_SCL_H;
    processor_i2c_delay();
    P_SCL_L;
    processor_i2c_delay();
}
#endif

/*******************************************************************
** 函数名:     processor_i2c_waitack
** 函数描述:   等待应答
** 参数:       无
** 返回:       返回为:=1有ACK,=0无ACK 
********************************************************************/
static BOOLEAN processor_i2c_waitack(void)
{
    P_SCL_L;
    processor_i2c_delay();
    P_SDA_H;
    processor_i2c_delay();
    P_SCL_H;
    processor_i2c_delay();
    if (P_SDA_read) {
        P_SCL_L;
        return false;
    }
    P_SCL_L;
    return true;
}

/*******************************************************************
** 函数名:     processor_i2c_sendbyte
** 函数描述:   发送一个byte ,数据从高位到低位
** 参数:       无
** 返回:       无
********************************************************************/
static void processor_i2c_sendbyte(INT8U SendByte)
{
    INT8U i = 8;
    
    while (i--) {
        P_SCL_L;
        processor_i2c_delay();
        if (SendByte & 0x80) {
            P_SDA_H;
        } else {
            P_SDA_L;
        }
        SendByte<<=1;
        processor_i2c_delay();
        P_SCL_H;
        processor_i2c_delay();
    }
    
    P_SCL_L;
} 

#if 0
/*******************************************************************
** 函数名:     simi2c_receivebyte
** 函数描述:   接收一个byte ,数据从高位到低位
** 参数:       无
** 返回:       无
********************************************************************/
static INT8U simi2c_receivebyte(void)
{
    u8 i = 8;
    u8 ReceiveByte = 0;

    P_SDA_H;
    while(i--) {
        ReceiveByte <<= 1;
        P_SCL_L;
        processor_i2c_delay();
        P_SCL_H;
        processor_i2c_delay();
        if (P_SDA_read) {
            ReceiveByte |= 0x01;
        } 
    }

  #if 0
    for (i = 0; i < 8; i ++) {
        P_SCL_H;
        processor_i2c_delay();
        ReceiveByte<<=1; 
        if(P_SDA_read) 
        { 
          ReceiveByte|=0x01; 
        } 
        P_SCL_L;
        processor_i2c_delay();
    }
  #endif    
    P_SCL_L;
    return ReceiveByte;
}
#endif

/*******************************************************************
** 函数名:     dal_processor_writebyte
** 函数描述:   写一个字节到指定设备的指定地址中
** 参数:       [in] devaddr        器件地址
**             [in] value          写入值
** 返回:       无
********************************************************************/
BOOLEAN dal_processor_writebyte(INT8U devaddr, INT8U value)
{
    INT32U ticks;
    
    if (!processor_i2c_star()) {
        return false;
    }
    processor_i2c_sendbyte(devaddr);
    if (!processor_i2c_waitack()) {
        processor_i2c_stop();
        return false;
    }

    processor_i2c_sendbyte(value);
    if (!processor_i2c_waitack()) {
        processor_i2c_stop();
        return false;
    }

    processor_i2c_stop();
    ticks = 80000;
    while(ticks--);
    return true;
}



/**************************** (C) COPYRIGHT 2012  XIAMEN YAXON.LTD **************END OF FILE******/

