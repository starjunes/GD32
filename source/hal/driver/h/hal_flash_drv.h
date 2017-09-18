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
#ifndef HAL_FLASH_DRV_H
#define HAL_FLASH_DRV_H      1

#include "stm32f0xx.h"
#include "stm32f0xx_conf.h"


#define FLASH_OP_FLAG        0x55                /* 操作标志 */

#define FLASH_BASE_ADDR      FLASH_BASE          /* FLASH基准地址 */
#define FLASH_PAGE_SIZE      2048                /* 页面大小 */
#define FLASH_MAX_PAGES      64                  /* 页面数量 */
#define FLASH_TOTAL_SIZE     (FLASH_PAGE_SIZE * FLASH_MAX_PAGES)/* FLASH空间总大小 */
#define FLASH_END_ADDR       (FLASH_BASE_ADDR + FLASH_TOTAL_SIZE)/* FLASH最大空间地址 */

#define FLASH_HEAD_OFFSET    0x100               /* 头信息偏移量 */

#define FLASH_BOOT_BASE      FLASH_BASE_ADDR     /* BOOT程序基准地址 */
#define FLASH_BOOT_SIZE      (0x8000)            /* BOOT程序大小 */
#define FLASH_BOOT_HEAD_BASE (FLASH_BOOT_BASE + FLASH_HEAD_OFFSET)

#define FLASH_APP_BASE       (FLASH_BOOT_BASE + FLASH_BOOT_SIZE) /* APP程序基准地址 */
#define FLASH_APP_SIZE       (FLASH_TOTAL_SIZE - FLASH_BOOT_SIZE - FLASH_PAGE_SIZE)  /* APP程序大小,预留一个扇区作为升级标志 */
#define FLASH_APP_HEAD_BASE  (FLASH_APP_BASE + FLASH_HEAD_OFFSET)

#define FLASH_FLAG_BASE      (FLASH_APP_BASE + FLASH_APP_SIZE) /* 固件升级置标志地址 */






/*******************************************************************
** 函数名称:   HAL_FLASH_Unlock
** 函数描述:   解锁FLASH擦写操作,打开后才能对FLASH进行擦写操作
** 参数:       无
** 返回:       无
********************************************************************/
void HAL_FLASH_Unlock1(INT8U flag);
#define HAL_FLASH_Unlock() HAL_FLASH_Unlock1(FLASH_OP_FLAG)

/*******************************************************************
** 函数名称:   HAL_FLASH_Lock
** 函数描述:   锁定FLASH擦写操作
** 参数:       无
** 返回:       无
********************************************************************/
void HAL_FLASH_Lock1(INT8U flag);
#define HAL_FLASH_Lock() HAL_FLASH_Lock1(FLASH_OP_FLAG)

/*******************************************************************
** 函数名称:   HAL_FLASH_ErasePage
** 函数描述:   擦除FLASH页面
** 参数:       [in] page: 页面编号,从0开始
** 返回:       成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN HAL_FLASH_ErasePage1(INT32U page, INT8U flag);
#define HAL_FLASH_ErasePage(page) HAL_FLASH_ErasePage1(page, FLASH_OP_FLAG)

/*******************************************************************
** 函数名称:   HAL_FLASH_EraseAllPage
** 函数描述:   擦除所有FLASH页面
** 参数:       无
** 返回:       成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN HAL_FLASH_EraseAllPage1(INT8U flag);
#define HAL_FLASH_EraseAllPage() HAL_FLASH_EraseAllPage1(FLASH_OP_FLAG)

/*******************************************************************
** 函数名称:   HAL_FLASH_WriteHalfWord
** 函数描述:   写入一个16位数据(小端模式)
** 参数:       [in] addr:   写入的地址,绝对地址，2的整数倍，从0开始
**             [in] halfword: 写入的数据
** 返回:       成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN HAL_FLASH_WriteHalfWord1(INT32U addr, INT16U halfword, INT8U flag);
#define HAL_FLASH_WriteHalfWord(addr, halfword) HAL_FLASH_WriteHalfWord1(addr, halfword, FLASH_OP_FLAG)

/*******************************************************************
** 函数名称:   HAL_FLASH_WriteWord
** 函数描述:   写入一个32位数据(小端模式)
** 参数:       [in] addr: 写入的地址,绝对地址，2的整数倍，从0开始
**             [in] word:   写入的数据
** 返回:       成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN HAL_FLASH_WriteWord1(INT32U addr, INT32U word, INT8U flag);
#define HAL_FLASH_WriteWord(addr, word) HAL_FLASH_WriteWord1(addr, word, FLASH_OP_FLAG)

/*******************************************************************
** 函数名称:   HAL_FLASH_WriteData
** 函数描述:   写入一串数据，绝对地址，2的整数倍，从0开始
** 参数:       [in] addr:  写入的地址,绝对地址，2的整数倍，从0开始
**             [in] data:    写入的数据
**             [in] datalen: 写入的数据长度
** 返回:       成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN HAL_FLASH_WriteData1(INT32U addr, INT8U *data, INT32U datalen, INT8U flag);
#define HAL_FLASH_WriteData(addr, data, datalen) HAL_FLASH_WriteData1(addr, data, datalen, FLASH_OP_FLAG)


#endif /* HAL_FLASH_DRV_H */

/**************************** (C) COPYRIGHT 2010 XIAMEN YAXON.LTD ***************END OF FILE******/
