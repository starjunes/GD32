/********************************************************************************
** 文件名:     port_iflash.c
** 版权所有:   (c) 2017 厦门雅迅网络股份有限公司
** 文件描述:   MCU内部flash接口
** 创建人：        谢金成，2017.12.8
********************************************************************************/
#include "hal_includes.h"
#include "port_iflash.h"
#include "hal_iflash_drv.h"
/********************************************************************************
**  函数名称:  PORT_GetIflashPara
**  功能描述:  获取指定类型flash的参数，起始地址和大小
**  输入参数:  [in] f_type： flash的类型，codeflash或dataflash
**            [out] bs_addr：用来返回起始地址
**            [out] f_size： 用来返回大小
**  返回参数:  TRUE，执行成功； FALSE，执行失败
********************************************************************************/
BOOLEAN PORT_GetIflashPara(INT8U f_type, INT32U *bs_addr, INT32U *f_size)
{
	return hal_iflash_GetFlashPara(f_type, bs_addr, f_size);
}

/********************************************************************************
**  函数名称:  PORT_EraseChip
**  功能描述:  擦除全片内部flash(包括codeflash、dataflash以及两个块对应的IFR区域)
**  输入参数:  无
**  返回参数:  TRUE，执行成功； FALSE，执行失败
********************************************************************************/
BOOLEAN PORT_EraseChip(void)
{
	return hal_iflash_EraseChip();
}

/********************************************************************************
**  函数名称:  PORT_EraseIflashBlock
**  功能描述:  擦除flash一个块区(codeflash或dataflash)
**  输入参数:  [in] startaddr: 要擦除的块区的起始地址
**  返回参数:  TRUE，执行成功； FALSE，执行失败
********************************************************************************/
BOOLEAN PORT_EraseIflashBlock(INT32U startaddr)
{
	return hal_iflash_EraseBlock(startaddr);
}

/********************************************************************************
**  函数名称:  PORT_ChkIflashErzSect
**  功能描述:  检验一个扇区是否已擦除过
**  输入参数:  [in] startaddr: 待检验扇区起始地址
**  返回参数:  TRUE，执行成功； FALSE，执行失败
********************************************************************************/
BOOLEAN PORT_ChkIflashErzSect(INT32U startaddr)
{
	return hal_iflash_ChkErzSect(startaddr);
}

/********************************************************************************
**  函数名称:  PORT_EraseIflashSector
**  功能描述:  擦除flash一个扇区(对于cflash是4KB，对于dflash是2KB)
**  输入参数:  [in] startaddr: 要擦除的扇区起始地址
**             [in] sctnum：擦除的扇区个数
**             [in] needchk：是否需要校验
**  返回参数:  TRUE，执行成功； FALSE，执行失败
********************************************************************************/
BOOLEAN PORT_EraseIflashSector(INT32U startaddr, INT32U sctnum, BOOLEAN needchk)
{
	return hal_iflash_EraseSector(startaddr, sctnum, needchk);
}

/********************************************************************************
**  函数名称:  PORT_ProgramIflashData
**  功能描述:  向flash写入一批数据
**  输入参数:  [in] startaddr: 写入地址，必须8字节对齐
**             [in] sdata：待写入数据缓存指针
**             [in] dlen：待写入数据长度(字节)，必须8的整数倍
**             [in] needchk：是否需要校验
**  返回参数:  TRUE，执行成功； FALSE，执行失败
********************************************************************************/
BOOLEAN PORT_ProgramIflashData(INT32U startaddr, INT8U *sdata, INT32U dlen, BOOLEAN needchk)
{
	return hal_iflash_ProgramData(startaddr, sdata, dlen, needchk);
}

