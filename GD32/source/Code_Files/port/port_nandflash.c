/********************************************************************************
**
** 文件名:     port_nandflash.c
** 版权所有:   (c) 1998-2017 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要封装nandflash存储访问接口
**
*********************************************************************************
**                  修改历史记录
**===============================================================================
**| 日期              | 作者        |  修改记录
**===============================================================================
**| 2017/08/04 | 黄运峰    |  创建该文件
********************************************************************************/
//#include "hal_includes.h"
#include "port_nandflash.h"
#include "hal_sflash_drv.h"

#define PAGE_ADDR_BITS  6
#define BLK_ADDR_BITS   11
#define SFLS_IDX        0

// 以下3个参数与具体nandflash芯片有关
#define ATTR_OFFSET     4096    // 属性段相对数据段的偏移量
#define DATA_SEG_LEN    512
#define SPARE_SEG_LEN   16

/********************************************************************************
** 函数名:   PORT_EraseBlock
** 函数描述: 擦除指定块，对于nandflash一次只能擦除1个BLOCK
** 参数:     [in] blknum：要擦除的block序号（0~2047）
** 返回:     执行结果代码
********************************************************************************/
ERR_TYPE_E PORT_EraseBlock(INT32U blkNum)
{
    INT32U rowAddr;

    rowAddr = blkNum << PAGE_ADDR_BITS;
    return hal_sfm_EraseBlock(SFLS_IDX, rowAddr);
}
/********************************************************************************
** 函数名:   PORT_WriteFlashRec
** 函数描述: 向nandflash指定地址写入一条记录数据
** 参数:     [in] blkNum：block序号(0~2047)
**           [in] pageNum：page序号(0~63)
**           [in] recNum：记录序号(0~3)
**           [in] attrBuf：待写入属性段缓存指针
**           [in] dataBuf：待写入数据段缓存指针
** 返回:     执行结果代码
********************************************************************************/
ERR_TYPE_E PORT_WriteFlashRec(INT32U blkNum, INT32U pageNum, INT8U recNum, INT8U *attrBuf, INT8U *dataBuf)
{
    INT32U rowAddr, colAddr;
    ERR_TYPE_E ret;

    if (recNum >= MAX_REC_IDX) return ERR_PARA;
    if (attrBuf == NULL || dataBuf == NULL) return ERR_PARA;
    rowAddr = (blkNum << PAGE_ADDR_BITS) + pageNum;
    colAddr = recNum * REC_DATA_LEN;
    ret = hal_sfm_Write(SFLS_IDX, rowAddr, colAddr, dataBuf, REC_DATA_LEN, attrBuf, REC_ATTR_LEN);

    return ret;
}
/********************************************************************************
** 函数名:   PORT_ReadFlashRec
** 函数描述: 从nandflash指定地址读出一条记录数据
** 参数:     [in] blkNum：block序号(0~2047)
**           [in] pageNum：page序号(0~63)
**           [in] recNum：记录序号(0~3)
**           [in] attrBuf：存放读取的属性段数据缓存指针
**           [in] dataBuf：存放读取的数据段数据缓存指针
** 返回:     执行结果代码
********************************************************************************/
ERR_TYPE_E PORT_ReadFlashRec(INT32U blkNum, INT32U pageNum, INT8U recNum, INT8U *attrBuf, INT8U *dataBuf)
{
    INT32U rowAddr, colAddr;
    ERR_TYPE_E ret;

    if (recNum >= MAX_REC_IDX) return ERR_PARA;
    if (attrBuf == NULL || dataBuf == NULL) return ERR_PARA;
    rowAddr = (blkNum << PAGE_ADDR_BITS) + pageNum;
    colAddr = recNum * REC_DATA_LEN;
    ret = hal_sfm_Read(SFLS_IDX, rowAddr, colAddr, dataBuf, REC_DATA_LEN);
    if (ret != ERR_NONE) return ret;
    colAddr = ATTR_OFFSET + colAddr/DATA_SEG_LEN * SPARE_SEG_LEN;
    ret = hal_sfm_Read(SFLS_IDX, rowAddr, colAddr, attrBuf, REC_ATTR_LEN);

    return ret;
}
/********************************************************************************
** 函数名:   PORT_GetFlashPara
** 函数描述: 获取sflash的基本参数
** 参数:     [out] pagesize：页大小
**           [out] blksize：块大小，一个块中包含的页数
**           [out] blknum：块的个数（与总容量有关）
** 返回:     TRUE,获取成功；FALSE，获取失败
********************************************************************************/
BOOLEAN PORT_GetFlashPara(INT32U *pageSize, INT32U *blkSize, INT32U *blkNum)
{
    if (pageSize == NULL || blkSize == NULL || blkNum == NULL) return FALSE;
    return hal_sfm_GetPara(SFLS_IDX, pageSize, blkSize, blkNum);
}

//------------------------------------------------------------------------------
/* End of File */
