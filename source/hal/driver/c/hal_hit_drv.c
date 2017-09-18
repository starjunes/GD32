/********************************************************************************
**
** 文件名:     hal_hit_drv.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现加速度传感器芯片驱动
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
#include "st_i2c_reg.h"
#include "st_i2c_simu.h"
#include "hal_hit_drv.h"
#include "yx_debug.h"

#if EN_GSENSOR > 0
/*
********************************************************************************
* 定义配置参数
********************************************************************************
*/

#define MAX_MEMORY           512                 /* 存储器最大容量 */
#define BLOCK_SIZE           256                 /* 存储器块大小 */
#define PAGE_SIZE            16                  /* 存储器页面大小 */

#define I2C_COM_HIT          I2C_COM_0           /* 存储器I2C通道号 */


#define DIRECT_Z_DOWN        0


#define DEVICE_ADDR          0x38                /* 从机设备地址 */

#define REGISTER_STATUS      0x00                /* 数据状态 */
#define REGISTER_X_MSB       0x01                /* X轴值 */
#define REGISTER_X_LSB       0x02
#define REGISTER_Y_MSB       0x03                /* Y轴值 */
#define REGISTER_Y_LSB       0x04
#define REGISTER_Z_MSB       0x05                /* Z轴值 */
#define REGISTER_Z_LSB       0x06
#define REGISTER_XYZ_DATA_CFG 0x0E               /* 设置量程和高通滤波器数据输出使能 */

#define REGISTER_PULSE_CFG   0x21                /* Pulse Configuration Register */
#define REGISTER_PULSE_SRC   0x22                /* Pulse Source Register */

#define REGISTER_PULSE_THSX  0x23                /* Pulse Threshold for X, Y & Z Registers */
#define REGISTER_PULSE_THSY  0x24                /* Pulse Threshold for X, Y & Z Registers */
#define REGISTER_PULSE_THSZ  0x25                /* Pulse Threshold for X, Y & Z Registers */

#define REGISTER_PULSE_TMLT  0x26                /* Pulse Time Window 1 Register */
#define REGISTER_PULSE_LTCY  0x27                /* Pulse Latency Timer Register */
#define REGISTER_PULSE_WIND  0x28                /* Second Pulse Time Window Register */

#define REGISTER_CTRL_REG1   0x2A                /* 6-7: Auto-WAKE sample frequency 
                                                  * 5-3: Data rate
                                                  *   2: Reduced noise reduced Maximun range mode
                                                  *   1: Fast Read mode
                                                  *   0: Full Scale selection 0:STANDBY 1: ACTIVE */
#define REGISTER_CTRL_REG2  0x2B                /* System Control 2 Register */
#define REGISTER_CTRL_REG3  0x2C                /* Interrupt Control Register */
#define REGISTER_CTRL_REG4  0x2D                /* Interrupt Enable Register */
#define REGISTER_CTRL_REG5  0x2E                /* Interrupt Configuration Register */
/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/


/*******************************************************************
** 函数名称:   HAL_HIT_Read
** 函数描述:   读取传感器数据
** 参数:       [in]  offset: flash地址偏移量
**             [out] dptr:   读取缓存指针
**             [in]  maxlen: 读取长度
** 返回:       成功返回true,失败返回false
********************************************************************/
BOOLEAN HAL_HIT_Read(INT32U offset, INT8U *dptr, INT32U maxlen)
{
    INT8U control;
    INT32U readlen = 0;
    
    if (!ST_I2C_IsOpen(I2C_COM_HIT)) {
        return false;
    }
    
    OS_ASSERT((offset + maxlen <= MAX_MEMORY), RETURN_FALSE);
    
    control = DEVICE_ADDR + ((offset / BLOCK_SIZE) << 1);
    offset %= BLOCK_SIZE;
    
    if (ST_I2C_SetReadAddr(I2C_COM_HIT, control, offset)) {                    /* 设置读取地址 */
        readlen = ST_I2C_ReadData(I2C_COM_HIT, control | 0x01, dptr, maxlen);
    }
    
    if (readlen == maxlen) {
        return true;
    } else {
        return false;
    }
}

/*******************************************************************
** 函数名称:   HAL_HIT_Write
** 函数描述:   写入数据
** 参数:       [in] offset: flash地址偏移量
**             [in] sptr:   数据指针
**             [in] slen:  数据长度
** 返回:       成功返回true,失败返回false
********************************************************************/
BOOLEAN HAL_HIT_Write(INT32U offset, INT8U *sptr, INT32U slen)
{
    BOOLEAN result;
    INT8U control;
    
    if (!ST_I2C_IsOpen(I2C_COM_HIT)) {
        return false;
    }
    
    control = DEVICE_ADDR + ((offset / BLOCK_SIZE) << 1);                  /* 写入数据 */
    offset %= BLOCK_SIZE;
    
    result = ST_I2C_SendData(I2C_COM_HIT, control, offset, sptr, slen);

    return result;
}

/*******************************************************************
** 函数名称: InitAccelerometerSensor
** 函数描述: 初始化加速度传感器
** 参数:     无
** 返回:     无
********************************************************************/
static BOOLEAN InitAccelerometerSensor(void)
{   
    INT8U  value_ctrl_reg1 = 0, value_xyz_data_cfg = 0, value;
    
    /* 设置CTRL_REG1寄存器 */
    if (!HAL_HIT_Read(REGISTER_CTRL_REG1, &value, 1)) {
        return FALSE;
    }
    
    value_ctrl_reg1 &= 0xFE;                                                   /* 设置为STANDBY模式 */
    value_ctrl_reg1 &= 0xC7;
    value_ctrl_reg1 |= 0x20;                                                   /* 设置data rate 为50hz */
    
    if (!HAL_HIT_Write(REGISTER_CTRL_REG1, &value_ctrl_reg1, 1)) {
        return FALSE;
    }
    
    /* 设置XYZ_DATA_CFG寄存器 */
    if (!HAL_HIT_Read(REGISTER_XYZ_DATA_CFG, &value_xyz_data_cfg, 1)) {
        return FALSE;
    }
    
    value_xyz_data_cfg &= 0xFC;
    value_xyz_data_cfg |= 0x02;                                                /* 设置量程为8g */
    if (!HAL_HIT_Write(REGISTER_XYZ_DATA_CFG, &value_xyz_data_cfg, 1)) {
        return FALSE;
    }
    
    value = 0x10;
    HAL_HIT_Write(REGISTER_CTRL_REG3, &value, 1);                              /* 开启脉冲（碰撞）唤醒，中断低电平触发 */
    value = 0x08;
    HAL_HIT_Write(REGISTER_CTRL_REG4, &value, 1);                              /* 开启脉冲中断 */
    HAL_HIT_Write(REGISTER_CTRL_REG5, &value, 1);                              /* INT1中断管角，脉冲 */
    
    value = 0x55;
    if (!HAL_HIT_Write(REGISTER_PULSE_CFG, &value, 1)) {                       /* 开启XYZ三轴脉冲检测 */
        return FALSE;
    }
    
    value = 0x0A;
    HAL_HIT_Write(REGISTER_PULSE_THSX, &value, 1);                             /* 加速度阈值设为 10*0.063 =0.63g */
    HAL_HIT_Write(REGISTER_PULSE_THSY, &value, 1);
    HAL_HIT_Write(REGISTER_PULSE_THSZ, &value, 1);
    
    value = 0xFF;
    HAL_HIT_Write(REGISTER_PULSE_TMLT, &value, 1);                             /* 这三个寄存还没分清，反正是设置上中断上报间隔 */
    HAL_HIT_Write(REGISTER_PULSE_LTCY, &value, 1);
    HAL_HIT_Write(REGISTER_PULSE_WIND, &value, 1);

    value_ctrl_reg1 |= 0x01;                                                   /* 设置为ACTIVE模式 */
    if (!HAL_HIT_Write(REGISTER_CTRL_REG1, &value_ctrl_reg1, 1)) {
        return FALSE;
    }
    
    return TRUE;
}

/*******************************************************************
** 函数名称: HAL_HIT_InitDrv
** 函数描述: 初始化模块
** 参数:     无
** 返回:     无
********************************************************************/
void HAL_HIT_InitDrv(void)
{
    ST_I2C_OpenCom(I2C_COM_HIT);
    InitAccelerometerSensor();
}

/*******************************************************************
** 函数名称: HAL_HIT_GetValueXYZ
** 函数描述: 获取XYZ轴值
** 参数:     无
** 返回:     成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN HAL_HIT_GetValueXYZ(GSENSOR_XYZ_T *xyz)
{
    INT8U tempbuf[6];
    
    if (HAL_HIT_Read(REGISTER_X_MSB, tempbuf, sizeof(tempbuf))) {
        xyz->x = ((tempbuf[0] << 8) + tempbuf[1]) >> 2;                        /* 将12位寄存器值转换为0.001g单位 */
        xyz->y = ((tempbuf[2] << 8) + tempbuf[3]) >> 2;
        xyz->z = ((tempbuf[4] << 8) + tempbuf[5]) >> 2;
        if (DIRECT_Z_DOWN > 0) {
            xyz->z = -xyz->z;
        }
        
        if ((xyz->x & 0x2000) != 0) {
            xyz->x |= 0xC000;
        }
        
        if ((xyz->y & 0x2000) != 0) {
            xyz->y |= 0xC000;
        }
        
        if ((xyz->z & 0x2000) != 0) {
            xyz->z |= 0xC000;
        }
        
        return true;
    } else {
        return false;
    }
}

#endif

/**************************** (C) COPYRIGHT 2012  XIAMEN YAXON.LTD **************END OF FILE******/

