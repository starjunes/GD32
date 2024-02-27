/********************************************************************************
**
** 文件名:     dal_simi2c.c
** 版权所有:   (c) 2014-2020 厦门雅迅网络股份有限公司
** 文件描述:   模拟I2C接口
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
#include  "dal_simi2c.h"
#include  "dal_pinlist.h"
#include  "dal_gpio.h"
#define u8 uint8_t
/*************************************************************************************************/
/*                           宏定义                                                              */
/*************************************************************************************************/

#define SCL_H         GPIO_BOP(GPIOD) = GPIO_Pin_10
#define SCL_L         GPIO_BC(GPIOD)  = GPIO_Pin_10
    
#define SDA_H         GPIO_BOP(GPIOD) = GPIO_Pin_11
#define SDA_L         GPIO_BC(GPIOD)  = GPIO_Pin_11

#define SCL_read      GPIO_ISTAT(GPIOD)  & GPIO_Pin_10
#define SDA_read      GPIO_ISTAT(GPIOD)  & GPIO_Pin_11

/**************************************************************************************************
**  函数名称:  Delay
**  功能描述:  延时函数
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
static void Delay(INT32U ticks)
{
    while(ticks--);
}
/**************************************************************************************************
**  函数名称:  I2C_delay
**  功能描述:  延时函数
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
void I2C_delay(void)
{
   INT8U i = 40; //150 //这里可以优化速度 ，经测试最低到5还能写入
   while(i--);
}

/**************************************************************************************************
**  函数名称:  I2C_Start
**  功能描述:  发送启始位
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
BOOLEAN I2C_Start(void)
{
    SDA_H;
    SCL_H;
    I2C_delay();
    if (!SDA_read) {
        return FALSE; //SDA线为低电平则总线忙,退出
    }
    SDA_L;
    I2C_delay();
    if (SDA_read) {
        return FALSE; //SDA线为高电平则总线出错,退出
    }
    SDA_L;
    I2C_delay();
    return TRUE;
}
/**************************************************************************************************
**  函数名称:  I2C_Stop
**  功能描述:  发送停止位
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
void I2C_Stop(void)
{
    SCL_L;
    I2C_delay();
    SDA_L;
    I2C_delay();
    SCL_H;
    I2C_delay();
    SDA_H;
    I2C_delay();
}
/**************************************************************************************************
**  函数名称:  I2C_Ack
**  功能描述:  发送应答位
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
void I2C_Ack(void)
{
    SCL_L;
    I2C_delay();
    SDA_L;
    I2C_delay();
    SCL_H;
    I2C_delay();
    SCL_L;
    I2C_delay();
}
/**************************************************************************************************
**  函数名称:  I2C_NoAck
**  功能描述:  不发送应答位
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
void I2C_NoAck(void)
{
    SCL_L;
    I2C_delay();
    SDA_H;
    I2C_delay();
    SCL_H;
    I2C_delay();
    SCL_L;
    I2C_delay();
}
/**************************************************************************************************
**  函数名称:  I2C_WaitAck
**  功能描述:  等待应答
**  输入参数:  None
**  返回参数:  返回为:=1有ACK,=0无ACK
**************************************************************************************************/
BOOLEAN I2C_WaitAck(void)
{
    SCL_L;
    I2C_delay();
    SDA_H;
    I2C_delay();
    SCL_H;
    I2C_delay();
    if (SDA_read) {
        SCL_L;
        return FALSE;
    }
    SCL_L;
    return TRUE;
}
/**************************************************************************************************
**  函数名称:  I2C_SendByte
**  功能描述:  发送一个byte ,数据从高位到低位
**  输入参数:  None
**  返回参数:  无
**************************************************************************************************/
void I2C_SendByte(INT8U SendByte)
{
    u8 i = 8;

    while (i--) {
        SCL_L;
        I2C_delay();
        if (SendByte & 0x80) {
            SDA_H;
        } else {
            SDA_L;
        }
        SendByte<<=1;
        I2C_delay();
        SCL_H;
        I2C_delay();
    }

    SCL_L;
}
/**************************************************************************************************
**  函数名称:  I2C_ReceiveByte
**  功能描述:  接收一个byte ,数据从高位到低位
**  输入参数:  None
**  返回参数:  无
**************************************************************************************************/
INT8U I2C_ReceiveByte(void)
{
    u8 i = 8;
    u8 ReceiveByte = 0;

    SDA_H;
    while(i--) {
        ReceiveByte <<= 1;
        SCL_L;
        I2C_delay();
        SCL_H;
        I2C_delay();
        if (SDA_read) {
            ReceiveByte |= 0x01;
        }
    }

#if 0

    for (i = 0; i < 8; i ++) {
        SCL_H;
        I2C_delay();
        ReceiveByte<<=1;
        if(SDA_read)
        {
          ReceiveByte|=0x01;
        }
        SCL_L;
        I2C_delay();
    }
  #endif
    SCL_L;
    return ReceiveByte;
}



/**************************************************************************************************
**  函数名称:  I2C_WriteByte
**  功能描述:  写一个字节到指定设备的指定地址中
**  输入参数:  None
**  返回参数:  无
**************************************************************************************************/
BOOLEAN I2C_WriteByte(INT8U devaddr, INT8U writeaddr, INT8U value, BOOLEAN addextend)
{
    if (!I2C_Start()) {
        return 0xE1;
    }
    I2C_SendByte(0xA0);
    if (!I2C_WaitAck()) {
        I2C_Stop();
        return FALSE;
    }

    if (addextend) {
        I2C_SendByte(devaddr);                                       /* 设置器件地址+段地址  */
        if (!I2C_WaitAck()) {
            I2C_Stop();
            return FALSE;
        }
    }


    I2C_SendByte(writeaddr);                                         /* 设置段内地址       */
    if (!I2C_WaitAck()) {
        I2C_Stop();
        return FALSE;
    }

    I2C_SendByte(value);
    if (!I2C_WaitAck()) {
        I2C_Stop();
        return FALSE;
    }

    I2C_Stop();

    Delay(2000);

    return TRUE;
}

/**************************************************************************************************
**  函数名称:  I2C_WriteNumByte
**  功能描述:  写一个字节到指定设备的指定地址中
**  输入参数:  None
**  返回参数:  无
**************************************************************************************************/
BOOLEAN I2C_WriteNumByte(INT8U devaddr, INT8U writeaddr, INT8U *data, INT8U num, BOOLEAN addextend)
{
    INT8U len;

    #if DEBUG_ICCARD > 0
        Debug_SysPrint("I2C_WriteNumByte addr:%d  num:%d\r\n", writeaddr, num);
        Debug_PrintHex(TRUE, data, num);
    #endif


    while (num) {
        len = 1;//8 - (writeaddr % 8);                                   /* 页写，每8个字节为1页 */
        if (num > len) {
            num -= len;
        } else {
            num = 0;
        }
        if (!I2C_Start()) return FALSE;

        I2C_SendByte(0xA0);
        if (!I2C_WaitAck()) {
            I2C_Stop();
            return FALSE;
        }

        if (addextend) {
            I2C_SendByte(devaddr);                                   /* 设置器件地址+段地址  */
            if (!I2C_WaitAck()) {
                I2C_Stop();
                return FALSE;
            }
        }

        I2C_SendByte(writeaddr);                                      /* 设置段内地址       */
        if (!I2C_WaitAck()) {
            I2C_Stop();
            return FALSE;
        }
        writeaddr += len;

        while (len--) {
            I2C_SendByte(*data ++);
            if (!I2C_WaitAck()) {
                I2C_Stop();
                return FALSE;
            }
        }

        I2C_Stop();

        Delay(20000);
    }

    return TRUE;
}

/**************************************************************************************************
**  函数名称:  I2C_ReadByte
**  功能描述:  读一个byte
**  输入参数:  None
**  返回参数:  无
**************************************************************************************************/
INT8U I2C_ReadByte(INT8U devaddr, INT8U readaddr, BOOLEAN addextend)
{
    INT8U read = 5;

    if (!I2C_Start()) {
        return 0xE1;
    }

    I2C_SendByte(0xA0);
    if (!I2C_WaitAck()) {
        I2C_Stop();
        return 0xE2;
    }
 //    if (!I2C_Start()) return 0xE3;
    if (addextend) {
    I2C_SendByte(devaddr);                                           /* 设置器件地址+段地址  */
        if (!I2C_WaitAck()) {
            I2C_Stop();
            return 0xE4;
        }
    }

    I2C_SendByte(readaddr);                                          /* 设置低起始地址       */
    if (!I2C_WaitAck()) {
        I2C_Stop();
        return 0xE5;
    }

    if (!I2C_Start()) return 0xE6;

    I2C_SendByte(0xA1);
    if (!I2C_WaitAck()) {
        I2C_Stop();
        return 0xE7;
    }

    read = I2C_ReceiveByte();

    I2C_NoAck();

    I2C_Stop();

    Delay(200);
    return read;
}

/**************************************************************************************************
**  函数名称:  I2C_ReadWord
**  功能描述:  从指定备的指定地址中读一个字节
**  输入参数:  None
**  返回参数:  无
**************************************************************************************************/
INT32U I2C_ReadWord(INT8U devaddr, INT8U readaddr, BOOLEAN addextend)
{
	INT8U  bytes[4];
	INT32U aword = 0;

	if (!I2C_Start()) return 0;

    I2C_SendByte(0xA0);
    if (!I2C_WaitAck()) {
        I2C_Stop();
        return 0;
    }

    if (addextend) {
        I2C_SendByte(devaddr);                                       /* 设置器件地址+段地址  */
        if (!I2C_WaitAck()) {
            I2C_Stop();
            return 0;
        }
    }

    I2C_SendByte(readaddr);                                          /* 设置低起始地址       */
    if (!I2C_WaitAck()) {
        I2C_Stop();
        return 0;
    }

    if (!I2C_Start()) return 0xFF;

    I2C_SendByte(0xA1);
    if (!I2C_WaitAck()) {
        I2C_Stop();
        return 0;
    }

    ClearWatchdog();

    bytes[0] = I2C_ReceiveByte();
    I2C_Ack();

    bytes[1] = I2C_ReceiveByte();
    I2C_Ack();

    bytes[2] = I2C_ReceiveByte();
    I2C_Ack();

    bytes[3] = I2C_ReceiveByte();
    I2C_NoAck();

    I2C_Stop();


    aword = (INT32U)(bytes[3] << 24) | (INT32U)(bytes[2] << 16) | (INT32U)(bytes[1] << 8) | (INT32U)bytes[0];

	return aword;
}

/**************************************************************************************************
**  函数名称:  I2C_ReadNumByte
**  功能描述:  连续读N个字节
**  输入参数:  None
**  返回参数:  无
**************************************************************************************************/
INT16U I2C_ReadNumByte(INT8U devaddr, INT8U readaddr, INT8U *data, INT16U readnum, BOOLEAN addextend)
{
    INT16U len;
    INT16U i;
    INT8U *ptr;

    //#if DEBUG_ICCARD > 0
     //   Debug_SysPrint("I2C_ReadNumByte\r\n");
    //#endif

    if (!I2C_Start()) {
        return 0xFF;
    }

    I2C_SendByte(0xA0);
    if (!I2C_WaitAck()) {
        I2C_Stop();
        return 0xFF;
    }

    if (addextend) {
        I2C_SendByte(devaddr);                                       /* 设置器件地址+段地址  */
        if (!I2C_WaitAck()) {
            I2C_Stop();
            return 0xFF;
        }
    }

    I2C_SendByte(readaddr);                                          /* 设置低起始地址       */
    if (!I2C_WaitAck()) {
        I2C_Stop();
        return 0xFF;
    }

    if (!I2C_Start()) return 0xFF;

    I2C_SendByte(0xA1);
    if (!I2C_WaitAck()) {
        I2C_Stop();
        return 0xFF;
    }

    ClearWatchdog();
    len = 0;

    ptr = data;
    i = 0;
    if (readnum) {
        for (; i < readnum; i++) {
            ptr[i] = I2C_ReceiveByte();
            if (i < readnum - 1) {
                I2C_Ack();
            } else {
                I2C_NoAck();
            }
        }
    }
    I2C_Stop();
    ClearWatchdog();

    Delay(100);
    len = i;
    while (len > 0) {
        if (ptr[len - 1] != 0) {
           break;
        }
        len--;
    }
    while (len > 0) {
        len--;
        if (ptr[len] != 0xff) {
            if (len == 126) {
                return len + 2;                                      /* 防止部标校验码部分为0xff */
            } else {
                return len + 1;
            }
        }
    }
    return 0;
}


/**************************** (C) COPYRIGHT 2012  XIAMEN YAXON.LTD **************END OF FILE******/

