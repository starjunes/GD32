/********************************************************************************
**
** 文件名:     dal_flash.c
** 版权所有:   (c) 2014-2020 厦门雅迅网络股份有限公司
** 文件描述:   FLASH驱动函数接口
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
#include  "dal_flash.h"

/*************************************************************************************************/
/*                           获取FLASH绝对地址                                                   */
/*************************************************************************************************/
#define FlashAbsAddr(sector, offset)   	(FLASH_BASE_ADDR + (sector * FLASH_PAGE_SIZE) + offset)
#define FLASH_ER_PRG_TIMEOUT ((uint32_t)0xFFFFFFFF)

/*******************************************************************
** 函数名:     Flash_Unlock
** 函数描述:   解锁FLASH擦除控制器
** 参数:       无
** 返回:       无
********************************************************************/
void Flash_Unlock(void)
{
    fmc_unlock();
}

/*******************************************************************
** 函数名:     Flash_Lock
** 函数描述:   锁定FLASH擦除控制器
** 参数:       无
** 返回:       无
********************************************************************/
void Flash_Lock(void)
{
    fmc_lock();
}

/*******************************************************************
** 函数名:     Flash_ErasePage
** 函数描述:   擦除某页FLASH
** 参数:       [in] sector:  所要擦除的块，从0到最大
** 返回:       擦除成功返回TRUE，失败返回FLASH
********************************************************************/
BOOLEAN Flash_ErasePage(INT16U sector)
{
    INT32U addr;
    
    if (sector >= FLASH_MAX_PAGES) return FALSE; 
    
    addr = FLASH_BASE_ADDR + FLASH_PAGE_SIZE * sector;
    
    if (FMC_READY != fmc_page_erase(addr)) {
        return FALSE;
    } else {
        return TRUE;
    }
}

/*******************************************************************
** 函数名:     Flash_EraseAllPage
** 函数描述:   擦除整片FLASH
** 参数:       无
** 返回:       擦除成功返回TRUE，失败返回FLASH
********************************************************************/
BOOLEAN Flash_EraseAllPage(void)
{
    if (FMC_READY != fmc_mass_erase()) {
        return FALSE;
    } else {
        return TRUE;
    }
}

/*******************************************************************
** 函数名:     Flash_WriteHalfword
** 函数描述:   写一个16位的数据到FLASH中,按字对齐
** 参数:       [in] addr     : 要写入的地址，必须字对齐
**             [in] halfword : 要写的数据
** 返回:       成功返回TRUE，失败返回FLASH
********************************************************************/
BOOLEAN Flash_WriteHalfword(INT32U addr, INT16U halfword)
{
    if (((addr - FLASH_BASE_ADDR)/ FLASH_PAGE_SIZE) >= FLASH_MAX_PAGES) return FALSE;
    if (addr % 2) return FALSE;
    
    if (FMC_READY != fmc_halfword_program(addr, halfword)) {
        return FALSE;
    } else {
        return TRUE;
    }
}

/*******************************************************************
** 函数名:     Flash_WriteWord
** 函数描述:   写一个32位的数据到FLASH中,按字对齐
** 参数:       [in] addr     : 要写入的地址，必须字对齐
**             [in] word     : 要写的数据
** 返回:       成功返回TRUE，失败返回FLASH
********************************************************************/
BOOLEAN Flash_WriteWord(INT32U addr, INT32U word)
{
    if (((addr - FLASH_BASE_ADDR)/ FLASH_PAGE_SIZE) >= FLASH_MAX_PAGES) return FALSE;
    if (addr % 2) return FALSE;
    
    if (FMC_READY != fmc_word_program(addr,word)) {
        return FALSE;
    } else {
        return TRUE;
    }
}

/*******************************************************************
** 函数名:     Flash_WriteData
** 函数描述:   写一串数据(8位)到FLASH中,按字对齐
** 参数:       [in] addr     : 要写入的地址，必须字对齐
**             [in] data     : 要写的数据
**             [in] datalen  : 要写的数据的个数，必须满足2的倍数
** 返回:       成功返回TRUE，失败返回FLASH
********************************************************************/
BOOLEAN Flash_WriteData(INT32U addr, INT8U *data, INT16U datalen)
{     
    INT16U temp;
	INT32U i;
	INT16U volatile *paddr;
	fmc_state_enum volatile status = FMC_READY;

	fmc_flag_clear(FMC_FLAG_BANK0_END);
	fmc_flag_clear(FMC_FLAG_BANK0_WPERR);
	fmc_flag_clear(FMC_FLAG_BANK0_PGERR);
    
    if (((addr - FLASH_BASE_ADDR) / FLASH_PAGE_SIZE) >= FLASH_MAX_PAGES) {
        return false;
    }
    if (addr % 2) {
        return false;
    }
    if (datalen % 2) {
        return false;
    }
    
    paddr = (INT16U *)(addr);
    datalen = datalen / 2;


	status = fmc_bank0_ready_wait(FLASH_ER_PRG_TIMEOUT);
	if(status == FMC_READY){
		FMC_CTL0 |= FMC_CTL0_PG;
		for (i = 0; i < datalen; i++) {
			ClearWatchdog();
        	temp = data[i *2] +(data[i * 2 +1] << 8);
			*paddr++ = temp;
		}
		status = fmc_bank0_ready_wait(FLASH_ER_PRG_TIMEOUT);
		FMC_CTL0 &= ~FMC_CTL0_PG;
	}else{
		return false;
	}
	paddr = (volatile INT16U*)(addr);
	for (i = 0; i < datalen; i++) {
		ClearWatchdog();
    	temp = data[i *2] +(data[i * 2 +1] << 8);
		if(paddr[i] != temp){
			return false;
		}
	}
	
    return true;
}

/*******************************************************************
** 函数名:     Flash_CopyBlock
** 函数描述:   FLASH拷贝整个扇区
** 参数:       [in] addr     : 要写入的地址，必须字对齐
**             [in] data     : 要写的数据
**             [in] datalen  : 要写的数据的个数，必须满足2的倍数
** 返回:       成功返回TRUE，失败返回FLASH
********************************************************************/
BOOLEAN Flash_CopyBlock(INT32U saddr, INT32U daddr,INT16U datalen)
{    
    if (((saddr - FLASH_BASE_ADDR) / FLASH_PAGE_SIZE) >= FLASH_MAX_PAGES) {
        return false;
    }
    if (((daddr - FLASH_BASE_ADDR) / FLASH_PAGE_SIZE) >= FLASH_MAX_PAGES) {
        return false;
    }
    
    if ((saddr % 2) || (daddr % 2)) {
        return false;
    }
    if (datalen % 2) {
        return false;
    }

    return Flash_WriteData(daddr, (INT8U *)saddr, datalen);
}

/*******************************************************************
** 函数名:     Flash_GetPage
** 函数描述:   获得地址对应的FLASH页码
** 参数:       [in] addr: 地址
** 返回:       地址对应的FLASH的页 
********************************************************************/
INT16U Flash_GetPage(INT32U addr)
{
    return ((addr - FLASH_BASE_ADDR) / FLASH_PAGE_SIZE);
}

/*******************************************************************
** 函数名:     GetFlashAbsAddr
** 函数描述:   获取绝对地址
** 参数:       [in] sector: 页数
**             [in] offset: 偏移地址
** 返回:       绝对地址
********************************************************************/
INT32U GetFlashAbsAddr(INT16U sector, INT32U offset) 
{
    return FlashAbsAddr(sector, offset);
}
 


/************************ (C) COPYRIGHT 2010  XIAMEN YAXON.LTD ******************END OF FILE******/
