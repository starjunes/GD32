/********************************************************************************
**
** 文件名:     hal_sflash_drv.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现外部扩展串行flash存储芯片读写驱动
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2014/04/09 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "hal_include.h"
#include "stm32f0xx.h"
#include "st_gpio_drv.h"
#include "st_i2c_reg.h"
#include "st_i2c_simu.h"
#include "yx_debug.h"


/*
********************************************************************************
* 定义配置参数
********************************************************************************
*/

#define MAX_MEMORY           1024                 /* 存储器最大容量 */
#define BLOCK_SIZE           256                 /* 存储器块大小 */
#define PAGE_SIZE            16                  /* 存储器页面大小 */

#define I2C_COM_FLASH        I2C_COM_0           /* 存储器I2C通道号 */




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
** 函数名称: HAL_SFLASH_InitDrv
** 函数描述: 初始化模块
** 参数:     无
** 返回:     无
********************************************************************/
void HAL_SFLASH_InitDrv(void)
{
    ST_I2C_OpenCom(I2C_COM_FLASH);
}

/*******************************************************************
** 函数名称:   HAL_SFLASH_Read
** 函数描述:   读取FLASH数据
** 参数:       [in]  offset: flash地址偏移量
**             [out] dptr:   读取缓存指针
**             [in]  maxlen: 读取长度
** 返回:       成功返回true,失败返回false
********************************************************************/
BOOLEAN HAL_SFLASH_Read(INT32U offset, INT8U *dptr, INT32U maxlen)
{
    INT8U control;
    INT32U readlen = 0;
    
    if (!ST_I2C_IsOpen(I2C_COM_FLASH)) {
        return false;
    }
    
    OS_ASSERT((offset + maxlen <= MAX_MEMORY), RETURN_FALSE);
    
    control = 0xA0 + ((offset / BLOCK_SIZE) << 1);
    offset %= BLOCK_SIZE;
    
    if (ST_I2C_SetReadAddr(I2C_COM_FLASH, control, offset)) {                  /* 设置读取地址 */
        readlen = ST_I2C_ReadData(I2C_COM_FLASH, control | 0x01, dptr, maxlen);
    }
    
    if (readlen == maxlen) {
        return true;
    } else {
        return false;
    }
}

/*******************************************************************
** 函数名称:   HAL_SFLASH_Write
** 函数描述:   写入FLASH数据
** 参数:       [in] offset: flash地址偏移量
**             [in] sptr:   数据指针
**             [in] slen:  数据长度
** 返回:       成功返回true,失败返回false
********************************************************************/
BOOLEAN HAL_SFLASH_Write(INT32U offset, INT8U *sptr, INT32U slen)
{
    BOOLEAN result;
    INT8U control;
    INT8U tempbuf[PAGE_SIZE];
    INT16U startbyte, writelen, ct_wait;
    
    if (!ST_I2C_IsOpen(I2C_COM_FLASH)) {
        return false;
    }
    
    OS_ASSERT((offset + slen <= MAX_MEMORY), RETURN_FALSE);
    
    startbyte = offset % PAGE_SIZE;         						           /* 计算页面中的起始字节位置 */
    if (startbyte != 0) {                                                      /* 写入起始非对齐字节 */
        result = HAL_SFLASH_Read(offset - startbyte, tempbuf, sizeof(tempbuf));
        OS_ASSERT((result != 0), RETURN_FALSE);
        
        if (slen >= PAGE_SIZE - startbyte) {
            writelen = PAGE_SIZE - startbyte;
        } else {
            writelen = slen;
        }
        
        YX_MEMCPY(&tempbuf[startbyte], writelen, sptr, writelen);              /* 填充数据 */
        offset -= startbyte;
        control = 0xA0 + ((offset / BLOCK_SIZE) << 1);                         /* 写入数据 */
        result = ST_I2C_SendData(I2C_COM_FLASH, control, offset % BLOCK_SIZE, tempbuf, sizeof(tempbuf));
        OS_ASSERT((result != 0), RETURN_FALSE);
        
        ct_wait = 0;
        while (++ct_wait < 0x100) {                                            /* 判断是否已写入成功 */
            ClearWatchdog();
            result = HAL_SFLASH_Read(offset, tempbuf, sizeof(tempbuf));
            if (result) {
                break;
            }
        }
        OS_ASSERT((result != 0), RETURN_FALSE);
        OS_ASSERT((YX_MEMCMP(&tempbuf[startbyte], sptr, writelen) == 0), RETURN_FALSE);
        
        sptr += writelen;
        slen -= writelen;
        offset += PAGE_SIZE;
    }
    
    for (;;) {                                                                 /* 写入剩余字节 */
        if (slen == 0) {
            break;
        }
        
        if (slen >= PAGE_SIZE) {
            writelen = PAGE_SIZE;
        } else {
            writelen = slen;
            
            result = HAL_SFLASH_Read(offset, tempbuf, sizeof(tempbuf));
            OS_ASSERT((result != 0), RETURN_FALSE);
        }
            
        YX_MEMCPY(tempbuf, writelen, sptr, writelen);
        control = 0xA0 + ((offset / BLOCK_SIZE) << 1);                         /* 写入数据 */
        result = ST_I2C_SendData(I2C_COM_FLASH, control, offset % BLOCK_SIZE, tempbuf, sizeof(tempbuf));
        OS_ASSERT((result != 0), RETURN_FALSE);
        
        ct_wait = 0;
        while (++ct_wait < 0x100) {                                            /* 判断是否已写入成功 */
            ClearWatchdog();
            result = HAL_SFLASH_Read(offset, tempbuf, sizeof(tempbuf));
            if (result) {
                break;
            }
        }
        OS_ASSERT((result != 0), RETURN_FALSE);
        OS_ASSERT((YX_MEMCMP(tempbuf, sptr, writelen) == 0), RETURN_FALSE);
        
        sptr += writelen;
        slen -= writelen;
        offset += writelen;
    }
    
    return result;
}






/**************************** (C) COPYRIGHT 2012  XIAMEN YAXON.LTD **************END OF FILE******/

