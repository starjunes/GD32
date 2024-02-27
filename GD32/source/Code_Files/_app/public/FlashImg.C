/**************************************************************************************************
**                                                                                               **
**  文件名称:  FlashImg.C                                                                        **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2010                                    **
**  创建信息:  jump -- 2011年1月20日                                                             **
**  文件描述:  串行FLASH镜像缓冲区管理                                                           **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/

#define  FLASHIMG_GLOBALS
#include "app_include.h"
#include "dal_include.h"
#include "s_flash.h"
#include "flashimg.h"
#include "database.h"
static INT8U   ImageType = 0;            /* 用来指示内存映射类型 */

static INT8U const  FlashID[SIZE_FLASHID] = {0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77};
static INT8U   FlashCheck[SIZE_FLASHID];

/**************************************************************************************************
**  函数名称:  ImageFlashValid
**  功能描述:  校验闪存内数据是否有效（校验码位于前8个字节）
**  输入参数:
**  返回参数:
**************************************************************************************************/
BOOLEAN ImageFlashValid(INT16U nSector)
{
    INT8U i;

    PageRead(FlashCheck, SIZE_FLASHID, nSector * s_switch_flash.blocksize, 0);
    for (i = 0; i < SIZE_FLASHID; i++) {
        if (FlashCheck[i] != FlashID[i]) {
            return FALSE;
        }
    }
    return TRUE;
}

/**************************************************************************************************
**  函数名称:  UpdateImageFlash
**  功能描述:   更新闪存内数据。
**  输入参数:  从nSector开始到nSector+1结束,占用2个block
**  返回参数:
**************************************************************************************************/
void UpdateImageFlash(INT16U nSector, INT8U *sptr)
{
    INT8U i;

    ClearWatchdog();

    if (nSector <= E_FONTS_PAGE) {
        return;
    }

    #if EN_CHECKFLASH
    //setflashflag();
    #endif
    for (i = 0; i < BLOCK_IMAGESECTOR; i++) {
        BlockErase(nSector + i);
    }
    PageWrite((INT8U*)FlashID, SIZE_FLASHID, nSector * s_switch_flash.blocksize, 0);
    PageWrite(sptr + SIZE_FLASHID, s_switch_flash.flashpage - SIZE_FLASHID, nSector * s_switch_flash.blocksize, SIZE_FLASHID);
    for (i = 1; i < s_switch_flash.blocksize * BLOCK_IMAGESECTOR; i++) {
        PageWrite(sptr + s_switch_flash.flashpage * i, s_switch_flash.flashpage, nSector * s_switch_flash.blocksize + i, 0);
    }
    #if EN_CHECKFLASH
    //clearflashflag();
    #endif

    ClearWatchdog();
}

/**************************************************************************************************
**  函数名称:  InitImageSram
**  功能描述:  将闪存内数据读到指定位置，增加一个type,用来判断当前IMAGE类型,以节约读取次数
**  输入参数: 从nSector开始到nSector+1结束,占用2个block
**  返回参数:
**************************************************************************************************/
BOOLEAN InitImageSram(INT16U nSector, INT8U type, INT8U *sptr)
{
    if (ImageFlashValid(nSector)) {
        if (ImageType != type) {
            AbsRead(sptr, BLOCK_IMAGESECTOR*s_switch_flash.block_nsize, (INT32U)nSector * s_switch_flash.block_nsize);
            ImageType = type;
        }
        return TRUE;
    } else {
        return FALSE;
    }
}

