/******************************************************************************
**
** Filename:     hal_flash_drv.c
** Copyright:    
** Description:  该模块主要实现FLASH存储器读写的驱动管理
**
*******************************************************************************
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#include "yx_include.h"
#include "stm32f10x.h"
#include "stm32f10x_flash.h"
#include "hal_flash_drv.h"

/*
********************************************************************************
* 定义配置参数
********************************************************************************
*/


/*
********************************************************************************
* 数据结构定义
********************************************************************************
*/

/*
********************************************************************************
* define module variants
********************************************************************************
*/
static INT8U s_unlock = 0;

#define FLASH_ER_PRG_TIMEOUT ((uint32_t)0x000B0000)


/*******************************************************************
** 函数名称:   HAL_FLASH_Unlock
** 函数描述:   解锁FLASH擦写操作,打开后才能对FLASH进行擦写操作
** 参数:       无
** 返回:       无
********************************************************************/
void HAL_FLASH_Unlock1(INT8U flag)
{
    OS_ASSERT((flag == FLASH_OP_FLAG), RETURN_VOID);
    
    if (!s_unlock) {
        s_unlock = 1;
        FLASH_Unlock();
    }
}

/*******************************************************************
** 函数名称:   HAL_FLASH_Lock
** 函数描述:   锁定FLASH擦写操作
** 参数:       无
** 返回:       无
********************************************************************/
void HAL_FLASH_Lock1(INT8U flag)
{
    OS_ASSERT((flag == FLASH_OP_FLAG), RETURN_VOID);
    
    if (s_unlock) {
        s_unlock = 0;
        FLASH_Lock();
    }
}

/*******************************************************************
** 函数名称:   HAL_FLASH_ErasePage
** 函数描述:   擦除FLASH页面
** 参数:       [in] page: 页面编号,从0开始
** 返回:       成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN HAL_FLASH_ErasePage1(INT32U page, INT8U flag)
{
    INT32U addr;
    
    OS_ASSERT((flag == FLASH_OP_FLAG), RETURN_FALSE);
    
    if (page >= FLASH_MAX_PAGES) {
        return FALSE;
    }
    
    addr = FLASH_BASE_ADDR + (FLASH_PAGE_SIZE * page);
    if (FLASH_COMPLETE != FLASH_ErasePage(addr)) {
        return FALSE;
    } else {
        return TRUE;
    }
}

/*******************************************************************
** 函数名称:   HAL_FLASH_EraseAllPage
** 函数描述:   擦除所有FLASH页面
** 参数:       无
** 返回:       成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN HAL_FLASH_EraseAllPage1(INT8U flag)
{
    OS_ASSERT((flag == FLASH_OP_FLAG), RETURN_FALSE);
    
    if (FLASH_COMPLETE != FLASH_EraseAllPages()) {
        return FALSE;
    } else {
        return TRUE;
    }
}

/*******************************************************************
** 函数名称:   HAL_FLASH_WriteHalfWord
** 函数描述:   写入一个16位数据(小端模式)
** 参数:       [in] addr: 写入的地址,绝对地址，2的整数倍，从0开始
**             [in] halfword: 写入的数据
** 返回:       成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN HAL_FLASH_WriteHalfWord1(INT32U addr, INT16U halfword, INT8U flag)
{
    OS_ASSERT((flag == FLASH_OP_FLAG), RETURN_FALSE);
    
    if (addr + 2 > FLASH_END_ADDR) {
        return FALSE;
    }
    
    if ((addr % 2) != 0) {
        return FALSE;
    }
    
    if (FLASH_COMPLETE != FLASH_ProgramHalfWord(addr, halfword)) {
        return FALSE;
    } else {
        if (*(volatile INT16U *)addr == halfword) {
            return TRUE;
        } else {
            return false;
        }
    }
}

/*******************************************************************
** 函数名称:   HAL_FLASH_WriteWord
** 函数描述:   写入一个32位数据(小端模式)
** 参数:       [in] addr: 写入的地址,绝对地址，2的整数倍，从0开始
**             [in] word: 写入的数据
** 返回:       成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN HAL_FLASH_WriteWord1(INT32U addr, INT32U word, INT8U flag)
{
    OS_ASSERT((flag == FLASH_OP_FLAG), RETURN_FALSE);
    
    if (addr + 4 > FLASH_END_ADDR) {
        return FALSE;
    }
    
    if ((addr % 2) != 0) {
        return FALSE;
    }
    
    if (FLASH_COMPLETE != FLASH_ProgramWord(addr, word)) {
        return FALSE;
    } else {
        if (*(volatile INT32U *)addr == word) {
            return TRUE;
        } else {
            return false;
        }
    }
}

/*******************************************************************
** 函数名称:   HAL_FLASH_WriteData
** 函数描述:   写入一串数据，绝对地址，2的整数倍，从0开始
** 参数:       [in] addr:  写入的地址,绝对地址，2的整数倍，从0开始
**             [in] data:    写入的数据
**             [in] datalen: 写入的数据长度
** 返回:       成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN HAL_FLASH_WriteData1(INT32U addr, INT8U *data, INT32U datalen, INT8U flag)
{
    INT16U temp;
    INT32U i;
    INT16U volatile *paddr;
    FLASH_Status volatile status = FLASH_COMPLETE;
    
    OS_ASSERT((flag == FLASH_OP_FLAG), RETURN_FALSE);
    
    if (addr + datalen > FLASH_END_ADDR) {
        return FALSE;
    }
    
    if ((addr % 2) != 0) {
        return FALSE;
    }
    
    if ((datalen % 2) != 0) {
        return FALSE;
    }
    
    paddr = (volatile INT16U *)(addr);
    datalen = datalen / 2;
    
    status = FLASH_WaitForLastOperation(FLASH_ER_PRG_TIMEOUT);                 /* Wait for last operation to be completed */
    if (status == FLASH_COMPLETE) {                                            /* if the previous operation is completed, proceed to program the new data */
        FLASH->CR |= FLASH_CR_PG;
        for (i = 0; i < datalen; i++) {
            ClearWatchdog();
            temp = data[i * 2] + (data[i * 2 + 1] << 8);
            *paddr++ = temp;
            
            //status = FLASH_WaitForLastOperation(FLASH_ER_PRG_TIMEOUT);
            //if (status != FLASH_COMPLETE) {
            //    FLASH->CR &= ~FLASH_CR_PG;                                     /* Disable the PG Bit */
            //    return false;
            //}
        }
        
        status = FLASH_WaitForLastOperation(FLASH_ER_PRG_TIMEOUT);
        if (status != FLASH_COMPLETE) {
            FLASH->CR &= ~FLASH_CR_PG;                                         /* Disable the PG Bit */
            return false;
        }
        FLASH->CR &= ~FLASH_CR_PG;                                             /* Disable the PG Bit */
    } else {
        return false;
    }
    
    paddr = (volatile INT16U *)(addr);                                         /* 检查写入数据是否成功 */
    for (i = 0; i < datalen; i++) {
        temp = data[i * 2] + (data[i * 2 + 1] << 8);
        if (paddr[i] != temp) {
            return false;
        }
    }
    
    return true;
}

/************************ (C) COPYRIGHT 2010  XIAMEN YAXON.LTD ******************END OF FILE******/
