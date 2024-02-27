/**************************************************************************************************
**                                                                                               **
**  文件名称:  s_flash.c                                                                         **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2010                                    **
**  创建信息:  jump -- 2011年1月14日                                                             **
**  文件描述:  串行FLASH驱动模块                                                                 **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/
#include "app_include.h"
#include "dal_include.h"
//#include "lcd.h"
#include "database.h"
#include "tools.h"
#include "s_flash.h"
#include "dal_gpio.h"
#include "dal_pinlist.h"
#if EN_DEBUG > 0
#include "debug_print.h"
#endif


#define     ID_AT45DB041      0X0   //未测
#define     ID_AT45DB161      0X1F260000
#define     ID_MX25L1606E     0Xc8401500



SWITCH_FLASH_T s_switch_flash;

static void SFlash_Port_Config(void)
{
	gpio_pin_remap_config(GPIO_SWJ_SWDPENABLE_REMAP,ENABLE);
    CreateOutputPort(PORT_SFLASH_CS, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, TRUE);    /* s_flash CS 管脚，输出模式 */
    CreateOutputPort(PORT_SFLASH_SCK, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, TRUE);   /* s_flash SCK 管脚，输出模式 */
    CreateOutputPort(PORT_SFLASH_SO, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, TRUE);/* s_flash SO 管脚，输入\输出模式 */
    CreateOutputPort(PORT_SFLASH_SI, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, TRUE);    /* s_flash SI 管脚，输出模式 */
  //  CreateOutputPort(PORT_SFLASH_RES, GPIO_Mode_Out_PP, GPIO_Speed_50MHz, TRUE);   /* s_flash RES 管脚，输出模式 */
}

static void SFlash_Write_Port(INT32U pgrp, INT16U pnum, BOOLEAN ishigh)
{
    WriteOutputPort(pgrp, pnum, ishigh);
}

static BOOLEAN SFlash_Read_SOPort(void)
{
    return ReadInputPort(PORT_SFLASH_SO);
}

/**************************************************************************************************
**  函数名称:  S_Short_Delay
**  功能描述:  短延时至少50ns
**  输入参数:
**  返回参数:  None
**************************************************************************************************/
static void S_Short_Delay(void)
{
    __nop();
    __nop();
    __nop();
    __nop();
	__nop();
    __nop();
    __nop();
    __nop();
}

/**************************************************************************************************
**  函数名称:  S_FLASH_Delay
**  功能描述:  延时us
**  输入参数:  n 微秒数 n = 1 表示1 us
**  返回参数:  None
**************************************************************************************************/
static void S_FLASH_Delay(INT16U n)
{
    INT32U count = 0;
    const INT32U utime = 12 * n;
    do {
        if (++count > utime) {
            return;
        }
    } while(1);
}

/**************************************************************************************************
**  函数名称:  WrByte
**  功能描述:  向AT写一个字节
**  输入参数:
**  返回参数:
**************************************************************************************************/
static void WrByte(INT8U byte)
{
    INT8U  i;
    for (i = 0; i < 8; i++) {
        if ((byte & 0x80) == 0x80) {
            SFlash_Write_Port(PORT_SFLASH_SI, TRUE);//SI_1;
        } else {
            SFlash_Write_Port(PORT_SFLASH_SI, FALSE);//SI_0;
        }
        SFlash_Write_Port(PORT_SFLASH_SCK, FALSE);
        SFlash_Write_Port(PORT_SFLASH_SCK, TRUE);
        byte <<= 1;
    }
}

/**************************************************************************************************
**  函数名称:  RdByte
**  功能描述:  从AT读取一个字节
**  输入参数:
**  返回参数:
**************************************************************************************************/
static INT8U RdByte(void)
{
    INT8U i, Data = 0;

    for (i = 0; i < 8; i++) {
        SFlash_Write_Port(PORT_SFLASH_SCK, FALSE);
        SFlash_Write_Port(PORT_SFLASH_SCK, TRUE);
        if (SFlash_Read_SOPort() == TRUE) {
            Data = (Data << 1) + 1;
        } else {
            Data <<= 1;
        }
    }
    return Data;
}

/**************************************************************************************************
**  函数名称:  StatusRead
**  功能描述:  读出AT45DB041当前状态
**  输入参数:
**  返回参数:  当前状态
**************************************************************************************************/
static INT8U StatusRead(void)
{
    INT8U status;

    SFlash_Write_Port(PORT_SFLASH_SCK, TRUE);//SCK_1;
    SFlash_Write_Port(PORT_SFLASH_CS, TRUE);//STARTOP;
    S_Short_Delay();
    SFlash_Write_Port(PORT_SFLASH_CS, FALSE);//STARTOP;

    if (s_switch_flash.switch_flish == E_MX25L1606E) {
       WrByte(0x05);
    } else {
       WrByte(0x57);
    }

    status = RdByte();
    SFlash_Write_Port(PORT_SFLASH_CS, TRUE);//ENDOP;
    SFlash_Write_Port(PORT_SFLASH_SI, TRUE);//SI_1;
    return status;
}

/**************************************************************************************************
**  函数名称:  FlashIsBusy
**  功能描述:  判断flash是否在忙状态
**  输入参数:
**  返回参数:
**************************************************************************************************/
static BOOLEAN FlashIsBusy(void)
{

    if (s_switch_flash.switch_flish == E_MX25L1606E) {
        if ((StatusRead() & 0x02) == 0x02) {
            return TRUE;
        } else {
            return FALSE;
        }
    } else {
        if ((StatusRead() & 0x80) == 0x80) {
            return false;
        } else {
            return true;
        }
    }
}


/**************************************************************************************************
**  函数名称:  SetCommand
**  功能描述:  往FLASH中写命令
**  输入参数:  扇区号，0～255
**  返回参数:
**************************************************************************************************/
static void SetCommand(INT8U command,INT8U *data,INT8U datalen)
{
    INT8U i = 0;

    SFlash_Write_Port(PORT_SFLASH_SCK, TRUE);//SCK_1;
    SFlash_Write_Port(PORT_SFLASH_CS, TRUE);//STARTOP;
    S_Short_Delay();
    SFlash_Write_Port(PORT_SFLASH_CS, FALSE);//STARTOP;
    WrByte(command);
    for(i = 0; i < datalen; i++) {
       WrByte(data[i]);
    }
    SFlash_Write_Port(PORT_SFLASH_CS, TRUE);//ENDOP;
}


/**************************************************************************************************
**  函数名称:  BlockErase
**  功能描述:  擦除块
**  输入参数:  扇区号，0～255
**  返回参数:
**************************************************************************************************/
void BlockErase(INT16U block)
{
	if (block <= E_FONTS_PAGE) {
        return;
	}
#if EN_CHECKFLASH
	if (!checkflashflag()) {
        clearflashflag();
        return;
    }
#endif
    #if DEBUG_LCD > 0
        Debug_SysPrint("block = %d \r\n", block);              /* 打印信息 */
    #endif

    if (s_switch_flash.switch_flish == E_MX25L1606E) {
       SetCommand(0x06,0,0); // 使能写
    }

    SFlash_Write_Port(PORT_SFLASH_SCK, TRUE);//SCK_1;
    SFlash_Write_Port(PORT_SFLASH_CS, TRUE);//STARTOP;
    S_Short_Delay();
    SFlash_Write_Port(PORT_SFLASH_CS, FALSE);//STARTOP;

    if (s_switch_flash.switch_flish == E_MX25L1606E) {
       WrByte(0x20);
    } else {
       WrByte(0x50);
    }

#if EN_CHECKFLASH
    if (!checkflashflag()) {
        clearflashflag();
        return;
    }
#endif

    if (s_switch_flash.switch_flish == E_MX25L1606E) {
        WrByte((block >> 4) & 0xff);
        WrByte((block << 4) & 0xf0);
    } else if (s_switch_flash.switch_flish == E_AT45DB161) {
        WrByte((block >> 3) & 0x3f);
        WrByte((block << 5) & 0xe0);
    } else if (s_switch_flash.switch_flish == E_AT45DB041) {
        WrByte((block >> 4) & 0x0f);
        WrByte((block << 4) & 0xf0);
    }

    WrByte(0x00);
    SFlash_Write_Port(PORT_SFLASH_CS, TRUE);//ENDOP;
    if (s_switch_flash.switch_flish == E_MX25L1606E) {
       SetCommand(0x04,0,0); // 使能写
    }

    ClearWatchdog();
    while (FlashIsBusy()) {
        S_FLASH_Delay(12);
    }
    ClearWatchdog();
    SFlash_Write_Port(PORT_SFLASH_SI, TRUE);//SI_1;
}
/*
*******************************************************************
设置FLASH芯片型号，通过读FLASH产家ID与设备编号来设定
参数：无
*******************************************************************
*/
static void  Set_FlashType(void)
{
#if SWITCH_FLASH == FLASH_2M
    INT32U Temp = 0, Temp0 = 0, Temp1 = 0, Temp2 = 0;

    SFlash_Write_Port(PORT_SFLASH_SCK, TRUE);//SCK_1;
    SFlash_Write_Port(PORT_SFLASH_CS, TRUE);//STARTOP;
    S_Short_Delay();
    SFlash_Write_Port(PORT_SFLASH_CS, FALSE);//STARTOP;
    S_Short_Delay();

    WrByte(0x9f);

    __nop();
    Temp0 = RdByte();
    Temp1 = RdByte();
    Temp2 = RdByte();

    SFlash_Write_Port(PORT_SFLASH_CS, FALSE);//ENDOP;
    S_Short_Delay();
    SFlash_Write_Port(PORT_SFLASH_CS, TRUE);//ENDOP;
    SFlash_Write_Port(PORT_SFLASH_SI, TRUE);//SI_1;

    Temp = (Temp0 << 24) | (Temp1 << 16) | (Temp2<<8);
    #if EN_DEBUG > 1
    Debug_SysPrint("set_flashtype temp = %08x\r\n",Temp);
    #endif
	//Debug_SysPrint("set_flashtype temp = %08x\r\n",Temp);
    switch(Temp) {
    case ID_AT45DB161:
        s_switch_flash.switch_flish = E_AT45DB161;
        s_switch_flash.flashpage    = 528;
        s_switch_flash.blocksize    = 8;
        s_switch_flash.block_nsize  = 4224;
        s_switch_flash.flashblock   = 512;
        s_switch_flash.pagenum      = 4096;
        s_switch_flash.sizeofflash  = 0x210000;
        s_switch_flash.size_imagesector   =   8448;
        break;
    default :
        s_switch_flash.switch_flish = E_MX25L1606E;
        s_switch_flash.flashpage    = 256;
        s_switch_flash.blocksize    = 16;
        s_switch_flash.block_nsize  = 4096;
        s_switch_flash.flashblock   = 512;
        s_switch_flash.pagenum      = 8192;
        s_switch_flash.sizeofflash  = 0x200000;
        s_switch_flash.size_imagesector   =   8192;
        break;
    }
#elif SWITCH_FLASH == FLASH_512K
    s_switch_flash.switch_flish = E_AT45DB041;
    s_switch_flash.flashpage    = 264;
    s_switch_flash.blocksize    = 8;
    s_switch_flash.block_nsize  = 2112;
    s_switch_flash.flashblock   = 256;
    s_switch_flash.pagenum      = 2048;
    s_switch_flash.sizeofflash  = 0x84000L;
    s_switch_flash.size_imagesector   =   6336;
#endif
}
/**************************************************************************************************
**  函数名称:  InitFlash
**  功能描述:  初始化、复位串行flash，初始化过后flash进入SPI mode3方式
**  输入参数:
**  返回参数:
**************************************************************************************************/
void InitFlash(void)
{
    SFlash_Port_Config();
    SFlash_Write_Port(PORT_SFLASH_CS, TRUE);//STARTOP;
    S_Short_Delay();
    SFlash_Write_Port(PORT_SFLASH_CS, FALSE);//STARTOP;
    SFlash_Write_Port(PORT_SFLASH_SCK, TRUE);//SCK_1;
    SFlash_Write_Port(PORT_SFLASH_SI, TRUE);//SI_1;

    S_FLASH_Delay(3);

    SFlash_Write_Port(PORT_SFLASH_CS, TRUE);//ENDOP;
    Set_FlashType();
}

/**************************************************************************************************
**  函数名称:  PageWrite
**  功能描述:  对AT45DB041进行写操作子函数: (通过缓冲存储器2对主存储器写操作)0x85
**  输入参数:  pInData 指向要写入数据的指针
**          :  length 写入数据数组的大小,小于264
**          :  startPage 将数据写入主存的起始页地址
**          :  startByte 将数据写入主存的起始页地址中的起始字节地址
**  返回参数:
**************************************************************************************************/
void PageWrite(INT8U* pInData, INT16U length, INT16U startpage, INT16U startbyte)
{
    INT16U  i;
    INT32U tmpdata;
#if EN_CHECKFLASH
    if (!checkflashflag()) {
        clearflashflag();
        return;
    }
#endif
    if (startpage < (E_FONTS_PAGE * s_switch_flash.block_nsize / s_switch_flash.flashpage)) {
        return;
    }
    if (s_switch_flash.switch_flish == E_AT45DB161) {
        tmpdata = ((INT32U)(startpage & 0x0fff)) << 10 | (INT32U)(startbyte & 0x03ff);
    } else if (s_switch_flash.switch_flish == E_AT45DB041) {
        tmpdata = ((INT32U)(startpage & 0x07ff)) << 9 | (INT32U)(startbyte & 0x01ff);
    } else {
        tmpdata = ((INT32U)(startpage & 0x01fff)) << 8 | (INT32U)(startbyte & 0x00ff);
    }

    if (s_switch_flash.switch_flish == E_MX25L1606E) {
        SetCommand(0x06,0,0); // 使能写
    }


    SFlash_Write_Port(PORT_SFLASH_SCK, TRUE);//SCK_1;
    SFlash_Write_Port(PORT_SFLASH_CS, TRUE);//STARTOP;
    S_Short_Delay();
    SFlash_Write_Port(PORT_SFLASH_CS, FALSE);//STARTOP;

    if (s_switch_flash.switch_flish == E_MX25L1606E) {
        WrByte(0x02);
    } else {
        WrByte(0x85);
    }

    #if EN_CHECKFLASH
    if(!checkflashflag()){
        clearflashflag();
        return;
    }
    #endif
    WrByte((INT8U)(tmpdata >> 16));              /* page地址高位 */
    WrByte((INT8U)(tmpdata >> 8));               /* page地址低7位和byte地址的第9位 */
    WrByte((INT8U)tmpdata);                      /* byte地址的低8位 */
    for (i = 0; i < length; i++) {               /* 对目的地址写操作,先写低字节后写高字节 */
        WrByte(*(pInData + i));
    }
    SFlash_Write_Port(PORT_SFLASH_CS, TRUE);//ENDOP;

    if (s_switch_flash.switch_flish == E_MX25L1606E) {
        SetCommand(0x04, 0, 0); // 使能写
    }
    ClearWatchdog();
    while (FlashIsBusy()) {
        S_FLASH_Delay(3200);
    }
    ClearWatchdog();
    SFlash_Write_Port(PORT_SFLASH_SI, TRUE);//SI_1;
}

/**************************************************************************************************
**  函数名称:  PageRead
**  功能描述:  读出AT45DB041存储的数据（主存储器页读, 读出字节）
**  输入参数:  InData    输出指针
**          :  length    读出数据数组的大小
**          :  startPage 读出数据的起始页地址
**          :  startByte 读出数据的起始页地址中的起始字节地址
**  返回参数:  Bone
**************************************************************************************************/
void PageRead(INT8U* InData, INT16U length, INT16U startpage, INT16U startbyte)
{
    INT16U i;
    INT32U tmpdata;
    if (s_switch_flash.switch_flish == E_AT45DB161) {
        tmpdata = ((INT32U)(startpage & 0x0fff)) << 10 | (INT32U)(startbyte & 0x03ff);
    } else if (s_switch_flash.switch_flish == E_AT45DB041) {
        tmpdata = ((INT32U)(startpage & 0x07ff)) << 9 | (INT32U)(startbyte & 0x01ff);
    } else {
        tmpdata = ((INT32U)(startpage & 0x01fff)) << 8 | (INT32U)(startbyte & 0x00ff);
    }

    ClearWatchdog();
    SFlash_Write_Port(PORT_SFLASH_SCK, TRUE);//SCK_1;
    SFlash_Write_Port(PORT_SFLASH_CS, TRUE);//STARTOP;
    S_Short_Delay();
    SFlash_Write_Port(PORT_SFLASH_CS, FALSE);//STARTOP;
    //debug_printf("PageRead switch_flish=%d\r\n",s_switch_flash.switch_flish);
    if (s_switch_flash.switch_flish == E_MX25L1606E) {
        WrByte(0x0b);
    } else {
        WrByte(0x68);
    }


    WrByte((INT8U)(tmpdata >> 16));             /* page地址高位 */
    WrByte((INT8U)(tmpdata >> 8));              /* page地址低位 */
    WrByte((INT8U)tmpdata);                     /* byte地址低位 */
    if (s_switch_flash.switch_flish != E_MX25L1606E) {
        for (i = 0; i < 4; i++) {
            WrByte(0x00);                           /* 32位无关码 */
        }
    } else {
        WrByte(0x00);                               /* 32位无关码 */

    }

    for (i = 0; i < length; i++) {
        InData[i] = RdByte();
    }
    SFlash_Write_Port(PORT_SFLASH_CS, TRUE);//ENDOP;
    SFlash_Write_Port(PORT_SFLASH_SI, TRUE);//SI_1;
    ClearWatchdog();
}
/*
*******************************************************************
按绝对地址对AT45DB041进行读操作
参数：InData 输出指针
length  读出数据数组的大小
absaddr 读出数据起始位置的绝对地址
*******************************************************************
*/
void AbsRead(INT8U* InData, INT16U length, INT32U absaddr)
{
    APP_ASSERT(absaddr < s_switch_flash.sizeofflash);
    PageRead(InData, length, (absaddr / s_switch_flash.flashpage), (absaddr % s_switch_flash.flashpage));
}

/********************************************************************************
** 函数名:   SFlashMode
** 函数描述: 串口模式设置
** 参数:     [in] mode： 模式:0xB9为低功耗，0xAB:从低功耗模式退出；0xA3:高性能模式
** 返回:     设置成功返回TRUE，设置失败返回FALSE.
********************************************************************************/
void SFlashMode(INT8U mode) 
{
    SetCommand(mode, 0, 0);
}


