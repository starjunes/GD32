/**************************************************************************************************
**                                                                                               **
**  文件名称:  Public.C                                                                          **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2010                                    **
**  创建信息:  jump -- 2011年1月20日                                                             **
**  文件描述:  系统参数管理                                                                      **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/

#define  PUBLIC_C
#include "app_include.h"
#include "dal_include.h"
#include "tools.h"
#include "s_flash.h"
#include "flashimg.h"
#include "public.h"
#include "database.h"
#include "dal_flash.h"
#include "debug_print.h"
//#include "protocaldata_send.h"
#include "dmemory.h"
//#include "pm_core.h"
//#include "app_can.h"

#define INT_PUBLIC_FLASHSECTOR          B_PARA2_PAGE     /* FLASH扇区号 */
#define InternalFlashAbsAddr(offset)   	(FLASH_BASE_ADDR + (INT_PUBLIC_FLASHSECTOR * FLASH_PAGE_SIZE) + offset)

static INT8U   InternalPubPara_buf[MAX_PARA_SIZE];
static INT8U const  FlashCheck[SIZE_FLASHID] = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x29};

/* 只能从ram中读取 */
/**************************************************************************************************
**  函数名称:  GetPubParaAddr
**  功能描述:  获取参数在片外数据库中的地址
**  输入参数:
**  返回参数:
**************************************************************************************************/
void   *GetPubParaAddr(BOOLEAN isimage, INT8U ParaID)
{
    INT8U  i;
    INT16U offset;

    offset = SIZE_FLASHID;
    for (i = 0; i < ParaID; i++) {
        offset += (PubTbl[i].len + 1);
    }
    if (isimage) {
        return (void *)(flash_image_mem + offset + 1);
    } else {
        AbsRead(InternalPubPara_buf + offset, PubTbl[ParaID].len + 1, (INT32U)_PUBLIC_FLASHSECTOR * s_switch_flash.block_nsize + offset);
        return InternalPubPara_buf + offset + 1;              /* 因为起始位置为校验和 */
    }
}

/**************************************************************************************************
**  函数名称:  GetInternalPubParaAddr
**  功能描述:  获取参数在片内数据库中的地址
**  输入参数:
**  返回参数:
**************************************************************************************************/
static INT8U *GetInternalPubParaAddr(INT8U ParaID)
{
    INT8U  i;
    INT16U offset;

    offset = SIZE_FLASHID;
    for (i = 0; i < ParaID; i++) {
        offset += (PubTbl[i].len + 1);
    }
    return InternalPubPara_buf + offset + 1;              /* 因为起始位置为校验和 */
}

/**************************************************************************************************
**  函数名称:  InitPubPara
**  功能描述:  初始化片外指定参数
**  输入参数:
**  返回参数:
**************************************************************************************************/
static void InitPubPara(INT8U ParaID)
{
    INT8U   *ptr;

    if (ParaID >= sizeof(PubTbl) / sizeof(PubTbl[0])) {
        return;
    }
    if (PubTbl[ParaID].i_ptr != 0) {
        ptr = GetPubParaAddr(TRUE, ParaID);
        memcpy(ptr, PubTbl[ParaID].i_ptr, PubTbl[ParaID].len);
        *(ptr - 1) = Getchksum_n(ptr, PubTbl[ParaID].len);
    }
}

/**************************************************************************************************
**  函数名称:  InitInternalPubPara
**  功能描述:  初始化片内指定参数
**  输入参数:
**  返回参数:
**************************************************************************************************/
static void InitInternalPubPara(INT8U ParaID)
{
    INT8U   *ptr;

    if (ParaID >= sizeof(PubTbl) / sizeof(PubTbl[0])) {
        return;
    }
    if (PubTbl[ParaID].i_ptr != 0) {
        ptr = GetInternalPubParaAddr(ParaID);
        memcpy(ptr, PubTbl[ParaID].i_ptr, PubTbl[ParaID].len);
        *(ptr - 1) = Getchksum_n(ptr, PubTbl[ParaID].len);
    }
}

/**************************************************************************************************
**  函数名称:  GetPubParaImage
**  功能描述:  读取片外FLASH参数数据到镜像区
**  输入参数:
**  返回参数:
**************************************************************************************************/
BOOLEAN GetPubParaImage(void)
{
    BOOLEAN result;

    result = InitImageSram(_PUBLIC_FLASHSECTOR, IMAGE_PUBLIC, flash_image_mem);
    #if DEBUG_PARA > 1
    Debug_SysPrint("GetPubParaImage:");
    Debug_PrintHex(TRUE, flash_image_mem, 100);
    #endif
    return result;
}

/**************************************************************************************************
**  函数名称:  UpdatePubParaImage
**  功能描述:  将参数数据写入片外FLASH
**  输入参数:
**  返回参数:
**************************************************************************************************/
void UpdatePubParaImage(void)
{
    UpdateImageFlash(_PUBLIC_FLASHSECTOR, flash_image_mem);
}

/**************************************************************************************************
**  函数名称:  UpdateInternalPubParaImage
**  功能描述:  将参数数据写入片内FLASH
**  输入参数:
**  返回参数:
**************************************************************************************************/
void UpdateInternalPubParaImage(void)
{
    Flash_Unlock();
    Flash_ErasePage(INT_PUBLIC_FLASHSECTOR);
    Flash_WriteData(InternalFlashAbsAddr(0), InternalPubPara_buf, MAX_PARA_SIZE);
    Flash_Lock();
}

/**************************************************************************************************
**  函数名称:  PubParaValid
**  功能描述:  检查片外参数合法性
**  输入参数:
**  返回参数:
**************************************************************************************************/
BOOLEAN PubParaValid(INT8U ParaID)
{
    INT8U   *ptr;

    if (ParaID >= sizeof(PubTbl) / sizeof(PubTbl[0])) {
        return FALSE;
    }
    ptr = GetPubParaAddr(FALSE, ParaID);
    if (*(ptr - 1) == Getchksum_n(ptr, PubTbl[ParaID].len)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/**************************************************************************************************
**  函数名称:  InternalPubParaValid
**  功能描述:  检查片内参数合法性
**  输入参数:
**  返回参数:
**************************************************************************************************/
BOOLEAN InternalPubParaValid(INT8U ParaID)
{
    INT8U   *ptr;

    if (ParaID >= sizeof(PubTbl) / sizeof(PubTbl[0])) {
        return FALSE;
    }
    ptr = GetInternalPubParaAddr(ParaID);
    if (*(ptr - 1) == Getchksum_n(ptr, PubTbl[ParaID].len)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/**************************************************************************************************
**  函数名称:  ReadPubPara
**  功能描述:  读取指定参数
**  输入参数:
**  返回参数:
**************************************************************************************************/
INT8U ReadPubPara(INT8U ParaID, void *ptr)
{
    if (ParaID >= sizeof(PubTbl) / sizeof(PubTbl[0])) {
        return 0;
    }
    memcpy(ptr, GetInternalPubParaAddr(ParaID), PubTbl[ParaID].len);
    return PubTbl[ParaID].len;
}

/**************************************************************************************************
**  函数名称:  StorePubPara
**  功能描述:  存储指定参数
**  输入参数:
**  返回参数:
**************************************************************************************************/
void StorePubPara(INT8U ParaID, void  *ptr)
{
    INT8U *imageptr;

    if (ParaID >= sizeof(PubTbl) / sizeof(PubTbl[0])) {
        return;
    }
    imageptr  = GetInternalPubParaAddr(ParaID);
    memcpy(imageptr, ptr, PubTbl[ParaID].len);
    *(imageptr - 1) = Getchksum_n(imageptr, PubTbl[ParaID].len);
    UpdateInternalPubParaImage();                                               /* 存片内 */

    GetPubParaImage();
    imageptr  = GetPubParaAddr(TRUE, ParaID);
    memcpy(imageptr, ptr, PubTbl[ParaID].len);
    *(imageptr - 1) = Getchksum_n(imageptr, PubTbl[ParaID].len);
    UpdatePubParaImage();                                                       /* 同时存片外 */
}

/**************************************************************************************************
**  函数名称:  ResumeAllPubPara
**  功能描述:  恢复片内片外参数出厂设置
**  输入参数:
**  返回参数:
**************************************************************************************************/
void ResumeAllPubPara(void)
{
    INT8U i;

    for (i = 0; i < sizeof(PubTbl) / sizeof(PubTbl[0]); i++) {
        InitPubPara(i);
        InitInternalPubPara(i);
    } 
    UpdatePubParaImage();
    UpdateInternalPubParaImage();
}

/**************************************************************************************************
**  函数名称:  GetDefaulValue
**  功能描述:  获取pp参数默认值
**  输入参数:
**  返回参数:
**************************************************************************************************/
void const * GetDefaulValue(INT8U ParaID)
{
    return PubTbl[ParaID].i_ptr;
}

/**************************************************************************************************
**  函数名称:  InitAllPubPara
**  功能描述:  初始化参数区
**  输入参数:
**  返回参数:
**************************************************************************************************/
void InitAllPubPara(void)
{
    INT8U   i;
    BOOLEAN image;
    INT8U   *addr;
    INT16U  offset;
    BOOLEAN valid1;                                                  /* 片内FLASH有效标志 */
    BOOLEAN valid2;                                                  /* 片外FLASH有效标志 */
    INT32U  delaycnt;

    offset = SIZE_FLASHID;
    for (i = 0; i < sizeof(PubTbl) / sizeof(PubTbl[0]); i++) {
        offset += (PubTbl[i].len + 1);
    }
    #if EN_DEBUG > 0
    Debug_SysPrint("总参数大小:%d\r\n", offset);
    #endif
    DAL_ASSERT(offset <= MAX_PARA_SIZE);

  #if EN_CHECKFLASH
    setflashflag();
  #endif

    valid1 = FALSE;
    valid2 = FALSE;
    for (i = 0; i < 3; i++) {
        if (valid1 == FALSE) {                                       /* 有效无需再去读取数据 */
            addr = (INT8U*)GetFlashAbsAddr(INT_PUBLIC_FLASHSECTOR, 0);
            memcpy(InternalPubPara_buf, addr, MAX_PARA_SIZE);
            #if DEBUG_PARA > 0
            Debug_SysPrint("GetFlashAbsAddr:");
            Debug_PrintHex(TRUE, InternalPubPara_buf, offset);
            #endif
        }
        if (CmpString(InternalPubPara_buf, (INT8U*)FlashCheck, SIZE_FLASHID) != STR_EQUAL) {
            if (ImageFlashValid(_PUBLIC_FLASHSECTOR)) {              /* 如果片外flash有效片内无效 */
                valid2 = TRUE;
                if (i >= 1) {                                        /* 从片外拷到片内 */
                    GetPubParaImage();
                    memcpy(InternalPubPara_buf, FlashCheck, SIZE_FLASHID);
                    memcpy(&InternalPubPara_buf[SIZE_FLASHID], &flash_image_mem[SIZE_FLASHID], offset - SIZE_FLASHID);
                    UpdateInternalPubParaImage();
                    #if EN_DEBUG > 1
                    Debug_SysPrint("从片外FLASH拷到片内!\r\n");
                    Debug_PrintHex(TRUE, InternalPubPara_buf, offset);
                    #endif
                    break;
                }
            }
        } else {
            valid1 = TRUE;
            if (valid2 == FALSE && !ImageFlashValid(_PUBLIC_FLASHSECTOR)) {/* 如果片内flash有效片外无效 */
                if (i == 2) {                                        /* 从片内拷到片外 */
                    memcpy(&flash_image_mem[SIZE_FLASHID], &InternalPubPara_buf[SIZE_FLASHID], offset - SIZE_FLASHID);
                    UpdatePubParaImage();
                    #if EN_DEBUG > 1
                    Debug_SysPrint("从片内拷到片外!\r\n");
                    Debug_PrintHex(TRUE, flash_image_mem, offset);
                    #endif
                    break;
                }
            } else {
                valid2 = TRUE;
                break;
            }
        }

        ClearWatchdog();
        delaycnt = 1800000;                                          /* 约150ms */
        while (delaycnt--) {
        }
        ClearWatchdog();
    }
    if (i >= 3) {                                                    /* 连续3次未读取成功，恢复出厂设置 */
		memcpy(InternalPubPara_buf, FlashCheck, SIZE_FLASHID);
        ResumeAllPubPara();
        #if EN_DEBUG >= 1
        Debug_SysPrint("多次未读取成功，恢复出厂设置!\r\n");
        Debug_PrintHex(TRUE, InternalPubPara_buf, offset);
        Debug_PrintHex(TRUE, flash_image_mem, offset);
        #endif
    } else {
        image = FALSE;
        for (i = 0; i < sizeof(PubTbl)/sizeof(PubTbl[0]); i++) {
			delaycnt = 1800000;                                          /* 约150ms */
	        while (delaycnt--) {
	        }
			ClearWatchdog();
			//debug_printf("i = %d PubParaValid=%d\r\n",i,PubParaValid(i));
            if (PubTbl[i].i_ptr != 0 && !PubParaValid(i)) {          /* 单个片外参数无效 */
                #if EN_DEBUG > 1
                Debug_SysPrint("单个片外参数无效! %d\r\n", i);
                #endif
			    if (!image) {
                    image = TRUE;
                    GetPubParaImage();
                }
                if (InternalPubParaValid(i)) {                       /* 如果片内有效从片内拷贝 */
                    addr = GetPubParaAddr(TRUE, i);
                    memcpy(addr, GetInternalPubParaAddr(i), PubTbl[i].len);
                    *(addr - 1) = Getchksum_n(addr, PubTbl[i].len);
                    #if DEBUG_PARA > 0
                    Debug_SysPrint("如果片内有效从片内拷贝\r\n");
                    #endif
                } else {                                             /* 恢复单个参数默认设置 */
                    InitPubPara(i);
                    #if DEBUG_PARA > 0
                    Debug_SysPrint("恢复单个参数默认设置\r\n");
                    #endif
                }
            }
			//debug_printf("i = %d image = %d  len=%d\r\n",i,image,sizeof(PubTbl)/sizeof(PubTbl[0]));
		}
        if (image) {
            UpdatePubParaImage();
        }


        addr = (INT8U*)GetFlashAbsAddr(INT_PUBLIC_FLASHSECTOR, 0);
        memcpy(InternalPubPara_buf, addr, MAX_PARA_SIZE);

        image = FALSE;
        for (i = 0; i < sizeof(PubTbl) / sizeof(PubTbl[0]); i++) {
            if (PubTbl[i].i_ptr != 0 && !InternalPubParaValid(i)) {  /* 单个片内参数无效 */
                #if EN_DEBUG > 1
                Debug_SysPrint("单个片内参数无效! %d\r\n", i);
                #endif
                image = TRUE;
                if (PubParaValid(i)) {                               /* 这个函数校验同时已读到RAM区 */
                    InitInternalPubPara(i);
                    #if DEBUG_PARA > 0
                    Debug_SysPrint("恢复单个参数默认设置\r\n");
                    #endif
                }
            }
        }
		
        if (image) {
			#if EN_DEBUG > 0
				debug_printf("UpdateInternalPubParaImage\r\n");
			#endif
            UpdateInternalPubParaImage();
        }
    }

  #if EN_CHECKFLASH
    clearflashflag();
  #endif
}

#if DEBUG_PARA > 0
INT16U ReadPubParaEx(INT8U ParaID, void *ptr)
{
    if (ParaID >= sizeof(PubTbl) / sizeof(PubTbl[0])) {
        return 0;
    }
    memcpy(ptr, GetPubParaAddr(false, ParaID), PubTbl[ParaID].len);
    return PubTbl[ParaID].len;
}

void Log_All_Para(void)
{
    #if 0
    INT8U i;
    SETCONFIG_STRUCT  setconfig;
    ROUTER_STRUCT TelBookRou;
    BT_PIN_STRUCT btconfig;
    CAN_ATTR_T can1attrib, can2attrib;
    ICPARA_T     icconfig;
    ICCANPARA_T    iccan;
    GSENSOR_CALIBRATION_T calibration;
    INT8U len = 0;
    INT8U *ptr;


    //INT8U data[31] = {0x55,0x02,0x02,0x0b,0xda,0x00,0x22,0x41,0x42,0x43,0x44,0x45,0x46,0x22,0x00,0x01,
    //                            0x03,0x0c,0x05,0x00,0x00,0xfa,0x00,0x04,0x00,0x00,0x01,0x02,0x01,0x02,0x01};
    //BakParaHdl(data, 31);



    Debug_SysPrint("sizeof(PubTbl):%d  sizeof(PubTbl[0]):%d\r\n", sizeof(PubTbl), sizeof(PubTbl[0]));
    Debug_SysPrint("can:%d %d %d %d %d %d %d\r\n", sizeof(CAN_BAUD_E), sizeof(FRAMETYPE_E),
                                sizeof(FRAMEFMAT_E), sizeof(CAN_TEST_MODE_E), sizeof(CAN_PARA_SET_E),
                                sizeof(CAN_WORKMODE_E), sizeof(CAN_FILTERCTRL_E));
    for (i = 0; i < sizeof(PubTbl) / sizeof(PubTbl[0]); i++) {
        Debug_SysPrint("para%d_len:%x\r\n", i, PubTbl[i].len);
    }

    len = ReadPubPara(SETCONFIG_, &setconfig);
    Debug_SysPrint("SETCONFIG_ len:%d\r\n", len);
    len = ReadPubPara(TELBOOKROUTER_, &TelBookRou);
    Debug_SysPrint("TELBOOKROUTER_ len:%d\r\n", len);
    len = ReadPubPara(BT_PIN_, &btconfig);
    Debug_SysPrint("BT_PIN_ len:%d\r\n", len);
    len = ReadPubPara(CAN1_, &can1attrib);
    Debug_SysPrint("CAN1_ len:%d\r\n", len);
    len = ReadPubPara(CAN2_, &can2attrib);
    Debug_SysPrint("CAN2_ len:%d\r\n", len);
    len = ReadPubPara(IC_, &icconfig);
    Debug_SysPrint("IC_ len:%d\r\n", len);
    len = ReadPubPara(ICCANPARA_, &iccan);
    Debug_SysPrint("ICCANPARA_ len:%d\r\n", len);
    len = ReadPubPara(CALIBRATION_, &calibration);
    Debug_SysPrint("CALIBRATION_ len:%d\r\n", len);
    ptr = (INT8U *)&calibration;
    for (i = 0; i< len; i++) {
        Debug_SysPrint(" %x ", *ptr++);
    }
    Debug_SysPrint("\r\n");

    #if DEBUG_PARA > 0
    Debug_SysPrint("\r\n falsh(in) setconfig:%x %x %x\r\n", setconfig.bright, setconfig.keysound, setconfig.light);
    Debug_SysPrint("TelBookRou:%x\r\n", TelBookRou.router);
    Debug_SysPrint("btconfig: %x", btconfig.flag);
    for(i = 0; i < 9; i++) {
        Debug_SysPrint(" %x", btconfig.code[i]);
    }
    Debug_SysPrint(" %x\r\n", btconfig.BT_power);
    Debug_SysPrint("can1attrib:%x %x %x %x %x %x %x %x %x %x\r\n",
        can1attrib.onoff, can1attrib.baud, can1attrib.type, can1attrib.fmat,
        can1attrib.test_mode, can1attrib.comm_effect, can1attrib.mode, can1attrib.mode_effect,
        can1attrib.mode_effect, can1attrib.filteronoff);
    Debug_SysPrint("can2attrib:%x %x %x %x %x %x %x %x %x %x\r\n",
        can2attrib.onoff, can2attrib.baud, can2attrib.type, can2attrib.fmat,
        can2attrib.test_mode, can2attrib.comm_effect, can2attrib.mode, can2attrib.mode_effect,
        can2attrib.mode_effect, can2attrib.filteronoff);
    Debug_SysPrint("icconfig:%x\r\n", icconfig.type);
    Debug_SysPrint("iccan:%d %x %x %x %x\r\n", iccan.cantimeen, iccan.candistnen, iccan.canspeeden, iccan.candistnresen, iccan.canoilen);
    Debug_SysPrint("calibration:%x %x %x %x\r\n", calibration.isvalid, calibration.x, calibration.y, calibration.z);
    #endif

    len = ReadPubParaEx(SETCONFIG_, &setconfig);
    Debug_SysPrint("EX SETCONFIG_ len:%d\r\n", len);
    len = ReadPubParaEx(TELBOOKROUTER_, &TelBookRou);
    Debug_SysPrint("EX TELBOOKROUTER_ len:%d\r\n", len);
    len = ReadPubParaEx(BT_PIN_, &btconfig);
    Debug_SysPrint("EX BT_PIN_ len:%d\r\n", len);
    len = ReadPubParaEx(CAN1_, &can1attrib);
    Debug_SysPrint("EX CAN1_ len:%d\r\n", len);
    len = ReadPubParaEx(CAN2_, &can2attrib);
    Debug_SysPrint("EX CAN2_ len:%d\r\n", len);
    len = ReadPubParaEx(IC_, &icconfig);
    Debug_SysPrint("EX IC_ len:%d\r\n", len);
    len = ReadPubParaEx(ICCANPARA_, &iccan);
    Debug_SysPrint("EX ICCANPARA_ len:%d\r\n", len);
    len = ReadPubParaEx(CALIBRATION_, &calibration);
    Debug_SysPrint("EX CALIBRATION_ len:%d\r\n", len);

    #if DEBUG_PARA > 0
    Debug_SysPrint("\r\n falsh(ex) setconfig:%x %x %x\r\n", setconfig.bright, setconfig.keysound, setconfig.light);
    Debug_SysPrint("TelBookRou:%x\r\n", TelBookRou.router);
    Debug_SysPrint("btconfig: %x", btconfig.flag);
    for(i = 0; i < 9; i++) {
        Debug_SysPrint(" %x", btconfig.code[i]);
    }
    Debug_SysPrint(" %d\r\n", btconfig.BT_power);
    Debug_SysPrint("can1attrib:%x %x %x %x %x %x %x %x %x %x\r\n",
        can1attrib.onoff, can1attrib.baud, can1attrib.type, can1attrib.fmat,
        can1attrib.test_mode, can1attrib.comm_effect, can1attrib.mode, can1attrib.mode_effect,
        can1attrib.mode_effect, can1attrib.filteronoff);
    Debug_SysPrint("can2attrib:%x %x %x %x %x %x %x %x %x %x\r\n",
        can2attrib.onoff, can2attrib.baud, can2attrib.type, can2attrib.fmat,
        can2attrib.test_mode, can2attrib.comm_effect, can2attrib.mode, can2attrib.mode_effect,
        can2attrib.mode_effect, can2attrib.filteronoff);
    Debug_SysPrint("icconfig:%x\r\n", icconfig.type);
    Debug_SysPrint("iccan:%d %x %x %x %x\r\n", iccan.cantimeen, iccan.candistnen, iccan.canspeeden, iccan.candistnresen, iccan.canoilen);
    Debug_SysPrint("calibration:%x %x %x %x\r\n", calibration.isvalid, calibration.x, calibration.y, calibration.z);
    #endif
    #endif
}
#endif

