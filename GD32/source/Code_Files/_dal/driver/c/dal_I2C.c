/**************************************************************************************************
**                                                                                               **
**  文件名称:  dal_i2c.h                                                                         **
**  版权所有:  CopyRight @ Xiamen Yaxon NetWork CO.LTD. 2012                                     **
**  创建信息:  2012-9-14 By lyj: 创建本文件                                                      **
**  文件描述:  模拟I2C接口                                                                       **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/
#include  "dal_include.h"
#include  "dal_i2c.h"
#if EN_DEBUG > 0
#include  "debug_print.h"
#endif
#include  "dal_pinlist.h"
#include  "dal_gpio.h"

/*************************************************************************************************/
/*                           宏定义                                                              */
/*************************************************************************************************/

#define SCL_H         PORT_I2C_CLK->BSRRL = PIN_I2C_CLK
#define SCL_L         PORT_I2C_CLK->BSRRH = PIN_I2C_CLK
    
#define SDA_H         PORT_I2C_DATA->BSRRL = PIN_I2C_DATA
#define SDA_L         PORT_I2C_DATA->BSRRH = PIN_I2C_DATA

#define SCL_read      PORT_I2C_CLK->IDR  & PIN_I2C_CLK
#define SDA_read      PORT_I2C_DATA->IDR & PIN_I2C_DATA

/**************************************************************************************************
**  函数名称:  iic_delay_cnt
**  功能描述:  延时函数
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
static void iic_delay_cnt(INT32U ticks)
{
    while(ticks--);
}

/**************************************************************************************************
**  函数名称:  iic_delay
**  功能描述:  延时函数
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
void iic_delay(void)
{
    INT16U i = 10;
    while(i--);
}

/**************************************************************************************************
**  函数名称:  iic_start
**  功能描述:  发送启始位
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
BOOLEAN iic_start(void)
{
    SDA_H;
    SCL_H;
    iic_delay(); 
    if (!SDA_read) {
        return FALSE; //SDA线为低电平则总线忙,退出 
    }
    SDA_L;
    iic_delay();
    if (SDA_read) {
        return FALSE; //SDA线为高电平则总线出错,退出
    }
    SDA_L;
    iic_delay();
    return TRUE;
}

/**************************************************************************************************
**  函数名称:  iic_stop
**  功能描述:  发送停止位
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
void iic_stop(void)
{
    SCL_L;
    iic_delay();
    SDA_L;
    iic_delay();
    SCL_H;
    iic_delay();
    SDA_H;
    iic_delay();
}

/**************************************************************************************************
**  函数名称:  iic_ack
**  功能描述:  发送应答位
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
void iic_ack(void)
{
    SCL_L;
    iic_delay();
    SDA_L;
    iic_delay();
    SCL_H;
    iic_delay();
    SCL_L;
    iic_delay();
}
/**************************************************************************************************
**  函数名称:  iic_noack
**  功能描述:  不发送应答位
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
void iic_noack(void)
{
    SCL_L;
    iic_delay();
    SDA_H;
    iic_delay();
    SCL_H;
    iic_delay();
    SCL_L;
    iic_delay();
}
/**************************************************************************************************
**  函数名称:  iic_waitack
**  功能描述:  等待应答
**  输入参数:  None
**  返回参数:  返回为:=1有ACK,=0无ACK 
**************************************************************************************************/
BOOLEAN iic_waitack(void)
{
    SCL_L;
    iic_delay();
    SDA_H;
    iic_delay();
    SCL_H;
    iic_delay();
    if (SDA_read) {
        SCL_L;
        return FALSE;
    }
    SCL_L;
    return TRUE;
}

/**************************************************************************************************
**  函数名称:  iic_sendbyte
**  功能描述:  发送一个byte ,数据从高位到低位
**  输入参数:  None
**  返回参数:  无
**************************************************************************************************/
void iic_sendbyte(INT8U SendByte)
{
    u8 i = 8;
    
    while (i--) {
        SCL_L;
        iic_delay();
        if (SendByte & 0x80) {
            SDA_H;
        } else {
            SDA_L;
        }
        SendByte<<=1;
        iic_delay();
        SCL_H;
        iic_delay();
    }
    
    SCL_L;
}

/**************************************************************************************************
**  函数名称:  iic_receivebyte
**  功能描述:  接收一个byte ,数据从高位到低位
**  输入参数:  None
**  返回参数:  无
**************************************************************************************************/
INT8U iic_receivebyte(void)
{
    u8 i = 8;
    u8 ReceiveByte = 0;

    SDA_H;
    while(i--) {
        ReceiveByte <<= 1;
        SCL_L;
        iic_delay();
        SCL_H;
        iic_delay();
        if (SDA_read) {
            ReceiveByte |= 0x01;
        } 
    }

#if 0

    for (i = 0; i < 8; i ++) {
        SCL_H;
        iic_delay();
        ReceiveByte<<=1; 
        if(SDA_read) 
        { 
          ReceiveByte|=0x01; 
        } 
        SCL_L;
        iic_delay();
    }
  #endif    
    SCL_L;
    return ReceiveByte;
} 

/**************************************************************************************************
**  函数名称:  dal_iic_init
**  功能描述:  管脚配置
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
void dal_iic_init(void)
{
    CreateOutputPort(PORT_I2C_CLK, PIN_I2C_CLK, GPIO_Mode_Out_PP, GPIO_Speed_50MHz, true);/* 创建时钟输出管脚，推挽输出 */
    CreateOutputPort(PORT_I2C_DATA, PIN_I2C_DATA, GPIO_Mode_Out_OD, GPIO_Speed_50MHz, true);/* 创建数据IO管脚，开漏输出，可以读取管脚状态*/
    WriteOutputPort(PORT_I2C_CLK, PIN_I2C_CLK, true);
    WriteOutputPort(PORT_I2C_DATA, PIN_I2C_DATA, true);
}

/**************************************************************************************************
**  函数名称:  dal_iic_writebyte
**  功能描述:  写一个字节到指定设备的指定地址中
**  输入参数:  None
**  返回参数:  无
**************************************************************************************************/
BOOLEAN dal_iic_writebyte(INT8U devaddr, INT8U mode, INT8U devaddrextend, INT8U writeaddr, INT8U value)
{
    if (!iic_start()) {
        return FALSE;
    }
    iic_sendbyte(devaddr);
    if (!iic_waitack()) {
        iic_stop();
        return FALSE;
    }

    iic_sendbyte(mode);
    if (!iic_waitack()) {
        iic_stop();
        return FALSE;
    }

    iic_sendbyte(devaddrextend);
    if (!iic_waitack()) {
        iic_stop();
        return FALSE;
    }
 

    iic_sendbyte(writeaddr);                                         /* 设置段内地址       */
    if (!iic_waitack()) {
        iic_stop();
        return FALSE;
    }

    iic_sendbyte(value);
    if (!iic_waitack()) {
        iic_stop();
        return FALSE;
    }
 
    iic_stop();
     
    iic_delay_cnt(2000);
     
    return TRUE;
}

/*******************************************************************
** 函数名:     dal_iic_writedata
** 函数描述:   写数据到指定设备的指定地址中
** 参数:       [in] devaddr          地址
**             [in] mode             模块
**             [in] devaddrextend    扩展地址
**             [in] writeaddr        写入寄存器
**             [in] data             数据指针
**             [in] num              数据长度
** 返回:       无
********************************************************************/
BOOLEAN dal_iic_writedata(INT8U devaddr, INT8U mode, INT8U devaddrextend, INT8U writeaddr, INT8U *data, INT8U num)
{
  #if DEBUG_RADIO > 0
    Debug_SysPrint("I2CW %02x %02x %02x %02x ", devaddr, mode, devaddrextend, writeaddr);
    Debug_PrintHex(data, num);
  #endif
    if (!iic_start()) {
        return FALSE;
    }
    
    iic_sendbyte(devaddr);                                           /* 设置器件地址+段地址  */
    if (!iic_waitack()) {
        iic_stop();
      #if DEBUG_RADIO > 0
        Debug_SysPrint("I2CW err1..\r\n");
      #endif
        return FALSE;
    }

    iic_sendbyte(mode);
    if (!iic_waitack()) {
        iic_stop();
      #if DEBUG_RADIO > 0
        Debug_SysPrint("I2CW err2..\r\n");
      #endif
        return FALSE;
    }

    iic_sendbyte(devaddrextend);
    if (!iic_waitack()) {
        iic_stop();
      #if DEBUG_RADIO > 0
        Debug_SysPrint("I2CW err3..\r\n");
      #endif
        return FALSE;
    }
 

    iic_sendbyte(writeaddr);                                         /* 设置段内地址       */
    if (!iic_waitack()) {
        iic_stop();
      #if DEBUG_RADIO > 0
        Debug_SysPrint("I2CW err4..\r\n");
      #endif
        return FALSE;
    }

    while (num--) {
        iic_sendbyte(*data++);
        if (!iic_waitack()) {
            iic_stop();
          #if DEBUG_RADIO > 0
            Debug_SysPrint("I2CW err5..\r\n");
          #endif
            return FALSE;
        }
    }

    iic_stop();
     
    iic_delay_cnt(2000);
     
    return TRUE;
}

/**************************************************************************************************
**  函数名称:  dal_iic_readbyte
**  功能描述:  读一个byte 
**  输入参数:  None
**  返回参数:  无
**************************************************************************************************/
INT8U dal_iic_readbyte(INT8U devaddr, INT8U mode, INT8U devaddrextend, INT8U readaddr, BOOLEAN addextend)
{
    INT8U read;

    if (!iic_start()) {
        return 0xE1;
    }
    
    iic_sendbyte(devaddr);
    if (!iic_waitack()) {
        iic_stop();
        return 0xE2;
    }
    
    iic_sendbyte(mode);
    if (!iic_waitack()) {
        iic_stop();
        return FALSE;
    }

    iic_sendbyte(devaddrextend);
    if (!iic_waitack()) {
        iic_stop();
        return FALSE;
    }
 
    iic_sendbyte(readaddr);                                                /* 设置低起始地址       */
    if (!iic_waitack()) {
        iic_stop();
        return 0xE5;
    }  
  
    if (!iic_start()) return 0xE6;
     
    iic_sendbyte(devaddr | 0x01);
    if (!iic_waitack()) {
        iic_stop();
        return 0xE7;
    }  
     
    read = iic_receivebyte();
     
    iic_noack();
     
    iic_stop();

    iic_delay_cnt(200);
    return read;
}

/**************************************************************************************************
**  函数名称:  dal_iic_readdata
**  功能描述:  连续读N个字节 
**  输入参数:  None
**  返回参数:  无
**************************************************************************************************/
INT16S dal_iic_readdata(INT8U devaddr, INT8U mode, INT8U devaddrextend, INT8U readaddr, INT8U *data, INT16U readnum)
{
    INT16U len;
    INT16U i;
    
    if (!iic_start()) {
        return -1;
    }
    
    iic_sendbyte(devaddr);                                           /* 设置器件地址+段地址  */
    if (!iic_waitack()) {
        iic_stop();
        return -2;
    }

    iic_sendbyte(mode);
    if (!iic_waitack()) {
        iic_stop();
        return 0;
    }

    iic_sendbyte(devaddrextend);
    if (!iic_waitack()) {
        iic_stop();
        return 0;
    }
 
    iic_sendbyte(readaddr);                                          /* 设置低起始地址       */
    if (!iic_waitack()) { 
        iic_stop();  
        return -3; 
    }  
  
    if (!iic_start()) {
        return 0xFFFF;
    }
    
    iic_sendbyte(devaddr | 0X01);
    if (!iic_waitack()) {
        iic_stop();
        return -4;
    }
    
    ClearWatchdog();
    len = 0;

    for (i = 0; i < readnum; i++) {
        *data++ = iic_receivebyte();
        len++;
        if (i < readnum - 1) { 
            iic_ack();
        } else {
            iic_noack();
        }
    }
    
    iic_stop();
    ClearWatchdog();

    iic_delay_cnt(100);
    return len;
}

/**************************** (C) COPYRIGHT 2012  XIAMEN YAXON.LTD **************END OF FILE******/

