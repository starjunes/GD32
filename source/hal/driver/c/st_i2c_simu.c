/********************************************************************************
**
** 文件名:     st_i2c_simu.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现模拟I2C接口驱动
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2014/04/09 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "hal_include.h"
#include "stm32f10x.h"
#include "st_gpio_drv.h"
#include "st_i2c_reg.h"
#include "st_i2c_simu.h"
#include "yx_debug.h"

//#if EN_I2C_SIMU > 0

/*
********************************************************************************
* 定义配置参数
********************************************************************************
*/

/* 定义管脚 */
#define _SENDING             0x01
#define _OPEN                0x80


#define Pullup_SCL(com)         GPIO_SetBits((GPIO_TypeDef *)ST_I2C_GetRegTblInfo(com)->gpio_base, 1 << (ST_I2C_GetRegTblInfo(com)->pin_scl))
#define Pulldown_SCL(com)       GPIO_ResetBits((GPIO_TypeDef *)ST_I2C_GetRegTblInfo(com)->gpio_base, 1 << (ST_I2C_GetRegTblInfo(com)->pin_scl))
    
#define Pullup_SDA(com)         GPIO_SetBits((GPIO_TypeDef *)ST_I2C_GetRegTblInfo(com)->gpio_base, 1 << (ST_I2C_GetRegTblInfo(com)->pin_sda))
#define Pulldown_SDA(com)       GPIO_ResetBits((GPIO_TypeDef *)ST_I2C_GetRegTblInfo(com)->gpio_base, 1 << (ST_I2C_GetRegTblInfo(com)->pin_sda))
#define ReadPin_SDA(com)        GPIO_ReadInputDataBit((GPIO_TypeDef *)ST_I2C_GetRegTblInfo(com)->gpio_base, 1 << (ST_I2C_GetRegTblInfo(com)->pin_sda))


/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    INT8U       status;
} I2C_T;

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static I2C_T s_i2c[I2C_COM_MAX];



/*******************************************************************
** 函数名称: I2C_PinsConfig
** 函数描述: 管脚配置
** 参数:     [in] pinfo: 配置表
** 返回:     无
********************************************************************/
static void I2C_PinsConfig(const I2C_REG_T *pinfo)
{
    GPIO_InitTypeDef gpio_initstruct;
    
    if (pinfo == 0) {
        return;
    }
    
    //RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);                             /* 开启系统时钟 */
    
    /* Configure gpio SCL */
    gpio_initstruct.GPIO_Pin   = (INT16U)(1 << pinfo->pin_scl);
    gpio_initstruct.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_initstruct.GPIO_Mode  = GPIO_Mode_Out_OD;
    //gpio_initstruct.GPIO_OType = GPIO_OType_OD;
    //gpio_initstruct.GPIO_PuPd  = GPIO_PuPd_UP;
    
    GPIO_Init((GPIO_TypeDef *)pinfo->gpio_base, &gpio_initstruct);
    
    /* Configure gpio SDA */
    gpio_initstruct.GPIO_Pin   = (INT16U)(1 << pinfo->pin_sda);
    gpio_initstruct.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_initstruct.GPIO_Mode  = GPIO_Mode_Out_OD;
    //gpio_initstruct.GPIO_OType = GPIO_OType_OD;
    //gpio_initstruct.GPIO_PuPd  = GPIO_PuPd_UP;
    
    GPIO_Init((GPIO_TypeDef *)pinfo->gpio_base, &gpio_initstruct);
}

/*******************************************************************
** 函数名:     I2C_delay
** 函数描述:   I2C延时一段时间
** 参数:       [in] period: 延时时间,单位：us
** 返回:       无
********************************************************************/
static void I2C_delay(void)
{
    /* 这里根据具体情况,可以优化速度 */
    
    #if 1
    INT8U i = 20;
    
    while (i--) {
        ;
    }
    #endif
}

/*******************************************************************
** 函数名:     I2C_Start
** 函数描述:   发送启始位
** 参数:       [in] com: 通道,见I2C_COM_E
** 返回:       成功返回TRUE, 否则返回FALSE
********************************************************************/
static BOOLEAN I2C_Start(INT8U com)
{
    Pullup_SDA(com);
    Pullup_SCL(com);
    I2C_delay();
    
    if (!ReadPin_SDA(com)) {                                                   /* SDA线为低电平则总线忙,退出 */
        return FALSE;
    }
    
    Pulldown_SDA(com);
    I2C_delay();
    if (ReadPin_SDA(com)) {                                                    /* SDA线为高电平则总线出错,退出 */
        return FALSE; 
    }
    
    Pulldown_SCL(com);
    I2C_delay();
    
    return TRUE;
}

/*******************************************************************
** 函数名:     I2C_Stop
** 函数描述:   发送停止位
** 参数:       [in] com: 通道,见I2C_COM_E
** 返回:       成功返回TRUE, 否则返回FALSE
********************************************************************/
static BOOLEAN I2C_Stop(INT8U com)
{
    Pulldown_SDA(com);
    I2C_delay();
    Pullup_SCL(com);
    I2C_delay();
    Pullup_SDA(com);
    I2C_delay();
    
    return true;
}

/*******************************************************************
** 函数名:     I2C_Ack
** 函数描述:   发送应答位
** 参数:       [in] com: 通道,见I2C_COM_E
** 返回:       成功返回TRUE, 否则返回FALSE
********************************************************************/
static BOOLEAN I2C_Ack(INT8U com)
{
    Pulldown_SCL(com);
    I2C_delay();
    Pulldown_SDA(com);
    I2C_delay();
    Pullup_SCL(com);
    I2C_delay();
    Pulldown_SCL(com);
    I2C_delay();
    
    Pullup_SDA(com);                                                           /* 释放总线 */
    return true;
}

/*******************************************************************
** 函数名:     I2C_NoAck
** 函数描述:   不发送应答位
** 参数:       [in] com: 通道,见I2C_COM_E
** 返回:       成功返回TRUE, 否则返回FALSE
********************************************************************/
static BOOLEAN I2C_NoAck(INT8U com)
{
    Pulldown_SCL(com);
    //I2C_delay();
    Pullup_SDA(com);
    I2C_delay();
    Pullup_SCL(com);
    I2C_delay();
    Pulldown_SCL(com);
    I2C_delay();
    
    return true;
}

/*******************************************************************
** 函数名:     I2C_WaitAck
** 函数描述:   等待应答
** 参数:       [in] com: 通道,见I2C_COM_E
** 返回:       成功返回TRUE, 否则返回FALSE
********************************************************************/
static BOOLEAN I2C_WaitAck(INT8U com)
{
    Pulldown_SCL(com);
    I2C_delay();
    Pullup_SDA(com);
    I2C_delay();
    Pullup_SCL(com);
    I2C_delay();
    
    if (ReadPin_SDA(com)) {                 
        Pulldown_SCL(com);
        I2C_delay();
        return FALSE;
    } else {                                                                   /* 正确应答位,低电平 */
        Pulldown_SCL(com);
        I2C_delay();
        return TRUE;
    }
}

/*******************************************************************
** 函数名:     I2C_SendByte
** 函数描述:   发送一个字节数据,数据从高位到低位
** 参数:       [in] com:      通道,见I2C_COM_E
**             [in] sendbyte: 待发送字节
** 返回:       无
********************************************************************/
static void I2C_SendByte(INT8U com, INT8U sendbyte)
{
    INT8U i;
    
    for (i = 0; i < 8; i++) {
        Pulldown_SCL(com);
        if (sendbyte & 0x80) {
            Pullup_SDA(com);
        } else {
            Pulldown_SDA(com);
        }
        I2C_delay();
        sendbyte <<= 1;
        Pullup_SCL(com);
        I2C_delay();
    }
    
    Pulldown_SCL(com);
    Pullup_SDA(com);                                                           /* 释放总线 */
}

/*******************************************************************
** 函数名:     I2C_ReadByte
** 函数描述:   读取一个字节数据,数据从高位到低位
** 参数:       [in] com:      通道,见I2C_COM_E
** 返回:       返回字节数据
********************************************************************/
static INT8U I2C_ReadByte(INT8U com)
{
    INT8U i;
    INT8U rbyte = 0;

    Pullup_SDA(com);
    for (i = 0; i < 8; i++) {
        rbyte <<= 1;
        Pulldown_SCL(com);
        I2C_delay();
        Pullup_SCL(com);
        I2C_delay();
        if (ReadPin_SDA(com)) {
            rbyte |= 1;
        }
    }

    Pulldown_SCL(com);
    return rbyte;
}


/*******************************************************************
** 函数名称: ST_I2C_InitDrv
** 函数描述: 初始化模块
** 参数:     无
** 返回:     无
********************************************************************/
void ST_I2C_InitDrv(void)
{
    YX_MEMSET(&s_i2c, 0, sizeof(s_i2c));
}

/*******************************************************************
** 函数名称:   ST_I2C_IsOpen
** 函数描述:   I2C总线是否已打开
** 参数:       [in] com: 通道编号,见I2C_COM_E
** 返回:       是返回true,否返回false
********************************************************************/
BOOLEAN ST_I2C_IsOpen(INT8U com)
{
    OS_ASSERT((com < I2C_COM_MAX), RETURN_FALSE);
    
    if ((s_i2c[com].status & _OPEN) != 0) {
        return true;
    } else {
        return false;
    }
}

/*******************************************************************
** 函数名称:   ST_I2C_OpenCom
** 函数描述:   打开I2C总线并初始化
** 参数:       [in] com: 通道编号,见I2C_COM_E
** 返回:       成功返回true,失败返回false
********************************************************************/
BOOLEAN ST_I2C_OpenCom(INT8U com)
{
    const I2C_REG_T *pinfo;
    
    OS_ASSERT((com < I2C_COM_MAX), RETURN_FALSE);
    
    pinfo = ST_I2C_GetRegTblInfo(com);
    if (pinfo->enable == 0) {
        return false;
    }
    
    I2C_PinsConfig(pinfo);                                                         /* configure the rx & tx pins */
    s_i2c[com].status = _OPEN;
    return true;
}

/*******************************************************************
** 函数名:     ST_I2C_CloseCom
** 函数描述:   关闭I2C
** 参数:       [in] com: 通道编号,见I2C_COM_E
** 返回:       成功返回true, 失败返回false
********************************************************************/
BOOLEAN ST_I2C_CloseCom(INT8U com)
{
    const I2C_REG_T *pinfo;
    
    OS_ASSERT((com < I2C_COM_MAX), RETURN_FALSE);
    
    pinfo = ST_I2C_GetRegTblInfo(com);
    if (pinfo->enable == 0) {
        return false;
    }
    
    s_i2c[com].status &= ~_OPEN;
    return true;
}
    
/*******************************************************************
** 函数名:     ST_I2C_SendData
** 函数描述:   发送一串数据,单字节无地址对齐限制
** 参数:       [in] com:       通道,见I2C_COM_E
**             [in] devaddr:   设备地址
**             [in] writeaddr: 数据的起始地址
**             [in] sptr:      数据指针
**             [in] slen:      数据长度
** 返回:       成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN ST_I2C_SendData(INT8U com, INT16U devaddr, INT16U writeaddr, INT8U *sptr, INT16U slen)
{
    OS_ASSERT((com < I2C_COM_MAX), RETURN_FALSE);
    
    if ((s_i2c[com].status & _OPEN) == 0) {
        return false;
    }
    
    if (sptr == 0 || slen == 0) {
        return false;
    }
    
    if (!I2C_Start(com)) {
        return FALSE;
    }
     
    I2C_SendByte(com, devaddr);                                                /* 发送设备地址 */
    if (!I2C_WaitAck(com)) {
        I2C_Stop(com);
        return FALSE; 
    }
 
    I2C_SendByte(com, writeaddr);                                              /* 设置写入地址 */
    if (!I2C_WaitAck(com)) {
        I2C_Stop(com);
        return FALSE;
    }

    while (slen--) {
        I2C_SendByte(com, *sptr++);
        if (!I2C_WaitAck(com)) {
            I2C_Stop(com);
            return FALSE;
        }
    }

    I2C_Stop(com);
    return TRUE;
}

/*******************************************************************
** 函数名:     ST_I2C_SetReadAddr
** 函数描述:   设置读取数据地址
** 参数:       [in]  com:       通道,见I2C_COM_E
**             [in]  devaddr:   设备地址
**             [in]  readaddr:  数据的起始地址
** 返回:       成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN ST_I2C_SetReadAddr(INT8U com, INT16U devaddr, INT16U readaddr)
{
    OS_ASSERT((com < I2C_COM_MAX), RETURN_FALSE);
    
    if ((s_i2c[com].status & _OPEN) == 0) {
        return FALSE;
    }
    
    if (!I2C_Start(com)) {
        return FALSE;
    }

    I2C_SendByte(com, devaddr);                                                /* 发送设备地址 */
    if (!I2C_WaitAck(com)) {
        I2C_Stop(com);
        return FALSE;
    }

    I2C_SendByte(com, readaddr);                                               /* 设置读取地址 */
    if (!I2C_WaitAck(com)) {
        I2C_Stop(com);
        return FALSE;
    }
    
    return true;
}

/*******************************************************************
** 函数名:     ST_I2C_ReadData
** 函数描述:   读取一串字节数据,在这之前必须调用ST_I2C_SetReadAddr
** 参数:       [in]  com:       通道,见I2C_COM_E
**             [in]  devaddr:   设备地址
**             [out] dptr:      返回读取数据
**             [out] maxlen:    读取数据长度
** 返回:       返回读取的字节数
********************************************************************/
INT16U ST_I2C_ReadData(INT8U com, INT16U devaddr, INT8U *dptr, INT16U maxlen)
{
    INT16U i, len;
    
    OS_ASSERT((com < I2C_COM_MAX), RETURN_FALSE);
    
    if ((s_i2c[com].status & _OPEN) == 0) {
        return 0;
    }
    
    if (dptr == 0 || maxlen == 0) {
        return 0;
    }
    
    if (!I2C_Start(com)) {
        return 0;
    }
    
    I2C_SendByte(com, devaddr);
    if (!I2C_WaitAck(com)) {
        I2C_Stop(com);
        return 0;
    }
    
    len = 0;
    for (i = 0; i < maxlen - 1; i++) {
        *dptr++ = I2C_ReadByte(com);
         len++;
         I2C_Ack(com);
    }
    *dptr++ = I2C_ReadByte(com);
    len++;
    I2C_NoAck(com);
    I2C_Stop(com);
    
    return len;
}

//#endif






/**************************** (C) COPYRIGHT 2012  XIAMEN YAXON.LTD **************END OF FILE******/

