/******************************************************************************
**
** Filename:     yx_bootmain.c
** Copyright:    
** Description:  该模块主要实现应用程序的引导运行
**
**=============================================================================
**             Revision history
**=============================================================================
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#define GLOBALS_BOOTMAIN     1
#include "yx_include.h"
#include "hal_flash_drv.h"
#include "dal_output_drv.h"
#include "yx_bootmain.h"
#include "yx_version.h"
#include "yx_debug.h"



/*
********************************************************************************
* define config parameters
********************************************************************************
*/


/*
********************************************************************************
* define struct
********************************************************************************
*/



/*
********************************************************************************
* define module variants
********************************************************************************
*/




/*******************************************************************
** 函数名:     YX_EraseFlagFlashRegion
** 函数描述:   擦除固件升级标志FLASH区
** 参数:       无
** 返回:       成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN YX_EraseFlagFlashRegion(void)
{
    INT16U page;
    
    page = (FLASH_FLAG_BASE - FLASH_BASE_ADDR) / FLASH_PAGE_SIZE;
    OS_ASSERT(HAL_FLASH_ErasePage(page), RETURN_FALSE);
    return true;
}

/*******************************************************************
** 函数名:     YX_WriteFlagRegion
** 函数描述:   数据写入固件升级标志FLASH区
** 参数:       [in] offset: 命令编码
**             [in] data:   数据指针
**             [in] datalen:数据长度
** 返回:       成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN YX_WriteFlagRegion(INT32U offset, INT8U *data, INT16U datalen)
{
    INT8U result;
    INT32U paddr;
    
    paddr = FLASH_FLAG_BASE + offset;
    result = HAL_FLASH_WriteData(paddr, data, datalen);                        /* 写入数据，写入不成功则需要再写入一次 */
    if (!result) {
        result = HAL_FLASH_WriteData(paddr, data, datalen);
    }
    OS_ASSERT(result, RETURN_FALSE);
    
    return true;
}

/*******************************************************************
** 函数名:     YX_EraseAppFlashRegion
** 函数描述:   擦除应用程序FLASH区
** 参数:       无
** 返回:       成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN YX_EraseAppFlashRegion(void)
{
    INT16U i, startpage, endpage;
    
    startpage = (FLASH_APP_BASE - FLASH_BASE_ADDR) / FLASH_PAGE_SIZE;
    endpage   = (FLASH_APP_BASE - FLASH_BASE_ADDR + FLASH_APP_SIZE) / FLASH_PAGE_SIZE;
    
    for (i = startpage; i < endpage; i++) {
        ClearWatchdog();
        OS_ASSERT(HAL_FLASH_ErasePage(i), RETURN_FALSE);
    }
    return true;
}

/*******************************************************************
** 函数名:     YX_EraseAppFlashRegionEx
** 函数描述:   擦除应用程序FLASH区
** 参数:       无
** 返回:       成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN YX_EraseAppFlashRegionEx(INT32U offset)
{
    INT16U startpage;
    
    startpage = (FLASH_APP_BASE - FLASH_BASE_ADDR + offset) / FLASH_PAGE_SIZE;
    OS_ASSERT(HAL_FLASH_ErasePage(startpage), RETURN_FALSE);
    return true;
}

/*******************************************************************
** 函数名:     YX_WriteAppCodeRegion
** 函数描述:   数据写入应用程序FLASH区
** 参数:       [in] offset: 命令编码
**             [in] data:   数据指针
**             [in] datalen:数据长度
** 返回:       成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN YX_WriteAppCodeRegion(INT32U offset, INT8U *data, INT16U datalen)
{
    INT8U result;
    INT32U paddr;
    
    paddr = FLASH_APP_BASE + offset;
    result = HAL_FLASH_WriteData(paddr, data, datalen);                        /* 写入数据，写入不成功则需要再写入一次 */
    if (!result) {
        result = HAL_FLASH_WriteData(paddr, data, datalen);
    }
    OS_ASSERT(result, RETURN_FALSE);
    
    return true;
}

/*******************************************************************
** 函数名:     YX_CheckFileHead
** 函数描述:   检测头标志是否有效
** 参数:       [in] app_head: 文件头信息指针
** 返回:       有效返回true，无效返回false
********************************************************************/
BOOLEAN YX_CheckFileHead(APP_HEAD_T *app_head)
{
    if (app_head->fileflag != CODE_FLAG_WIRE && app_head->fileflag != CODE_FLAG_WD) {    /* 判断头标志 */
        #if DEBUG_SYS > 0
        printf_com("APP文件头标志无效(0x%x)\r\n", app_head->fileflag);
        #endif
        return false;
    }
     
    if (YX_MEMCMP(app_head->platflag, CODE_FLAG_VAR, 4) != 0) {                /* 判断平台层信息 */
        #if DEBUG_SYS > 0
        printf_com("平台标识无效\r\n");
        #endif
        return false;
    }
        
    if (YX_MEMCMP(app_head->appflag, CODE_FLAG_APP, 4) != 0) {                      /* app映象文件的标识 */
        #if DEBUG_SYS > 0
        printf_com("APP标识无效\r\n");
        #endif
        return false;
    }
    return true;
}

/*******************************************************************
** 函数名:     YX_CheckAppIsValid
** 函数描述:   检测APP应用层代码是否有效
** 参数:       无
** 返回:       有效返回TRUE,无效返回FALSE
********************************************************************/
BOOLEAN YX_CheckAppIsValid(void)
{
    INT8U *sptr;
    INT16U checksum, checkcrc;
    INT32U fsize, filesize;
    APP_HEAD_T *p_headinfo;
    
    sptr = (INT8U *)FLASH_APP_BASE;
    p_headinfo = (APP_HEAD_T *)(FLASH_APP_HEAD_BASE);
    
    YX_MEMCPY((INT8U *)&checksum, sizeof(checksum), p_headinfo->checksum, sizeof(checksum));
    YX_MEMCPY((INT8U *)&filesize, sizeof(filesize), p_headinfo->filesize, sizeof(filesize));
    
    if (!YX_CheckFileHead(p_headinfo) || filesize > FLASH_APP_SIZE) {          /* 判断文件头是否正确 */
        goto LOAD_WD_FILE;
    } else {
        #if DEBUG_SYS > 0
        printf_com("APP文件头标志有效(0x%x)\r\n", p_headinfo->fileflag);
        #endif
    }

    if (checksum != 0 && filesize != 0) {
        sptr   = (INT8U *)FLASH_APP_HEAD_BASE + T_OFFSET(APP_HEAD_T, res);
        fsize  = filesize - FLASH_HEAD_OFFSET - T_OFFSET(APP_HEAD_T, res);
        
        checkcrc = YX_CheckSum_2U(sptr, fsize);
        if (checksum != checkcrc) {                                            /* 校验和无效 */
            #if DEBUG_SYS > 0
            printf_com("<FLASH区代码校验出错(0x%x)(0x%x)>\r\n", checksum, checkcrc);
            #endif
            
            goto LOAD_WD_FILE;
        } else {
            #if DEBUG_SYS > 0
            printf_com("<FLASH区代码校验成功(0x%x)(0x%x)>\r\n", checksum, checkcrc);
            #endif
        }
    }
    return true;
    
LOAD_WD_FILE:
    return false;
}

/*******************************************************************
** 函数名:     YX_CheckUpdateFlag
** 函数描述:   检测无线下载标志是否有效
** 参数:       无
** 返回:       有效返回TRUE,无效返回FALSE
********************************************************************/
BOOLEAN YX_CheckUpdateFlag(void)
{
    INT32U flag;
    INT8U *ptr;
    
    ptr = (INT8U *)FLASH_FLAG_BASE;
    flag  = ptr[0] << 24;
    flag += ptr[1] << 16;
    flag += ptr[2] << 8;
    flag += ptr[3];
    if (flag == CODE_FLAG_UPDATE) {
        return true;
    } else {
        return false;
    }
}

