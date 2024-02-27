/**************************************************************************************************
**                                                                                               **
**  文件名称:  s_flash.h                                                                         **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2010                                    **
**  创建信息:  jump -- 2011年1月14日                                                             **
**  文件描述:  串行FLASH驱动模块                                                                 **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/




#ifndef _S_FLASH_H
#define _S_FLASH_H 1


#if 0
#if SWITCH_FLASH == AT45DB161
#define s_switch_flash.flashpage           528//264
#define BLOCKSIZE           8              /* 1 BLOCK = 8 PAGE */
#define BLOCK_NSIZE         4224//2112     /* 1 BLOCK = FLASHPAGE*BLOCKSIZE */
#define FLASHBLOCK          512//256
#define PAGENUM             4096//2048     /* BLOCKSIZE*FLASHBLOCK */
#define SIZEOFFLASH         0x210000L      /* 528*4096 */
#elif SWITCH_FLASH == MX25L1606E
#define FLASHPAGE           256                                                /* 页大小*/
#define BLOCKSIZE           16        /* 1 BLOCK = 16 PAGE */
#define BLOCK_NSIZE         4096     /* 1 BLOCK = FLASHPAGE*BLOCKSIZE */
#define FLASHBLOCK          512
#define PAGENUM             8192     /* BLOCKSIZE*FLASHBLOCK */
#define SIZEOFFLASH         0x200000L /* 264*2048,512+16 KB */
#else
#define FLASHPAGE           264
#define BLOCKSIZE           8        /* 1 BLOCK = 8 PAGE */
#define BLOCK_NSIZE         2112     /* 1 BLOCK = FLASHPAGE*BLOCKSIZE */
#define FLASHBLOCK          256
#define PAGENUM             2048     /* BLOCKSIZE*FLASHBLOCK */
#define SIZEOFFLASH         0x84000L /* 264*2048,512+16 KB */
#endif
#endif
/*************************************************************************************************/
/*                             FLASH芯片属性                                                      */
/*************************************************************************************************/
typedef enum {
    E_AT45DB041      = 0x00,                                
    E_AT45DB161      = 0x01,                                
    E_MX25L1606E     = 0x02,                               
} SWITCH_FLASH_E;

/*************************************************************************************************/
/*                             FLASH芯片属性                                                      */
/*************************************************************************************************/
typedef struct {
    SWITCH_FLASH_E     switch_flish;
    INT8U              blocksize;
    INT16U             flashpage;
    INT16U             block_nsize;
    INT16U             flashblock;
    INT16U             pagenum;
    INT16U             size_imagesector;
    INT32U             sizeofflash;
}SWITCH_FLASH_T;
extern SWITCH_FLASH_T s_switch_flash;
void InitFlash(void); 

void BlockErase(INT16U block);
void PageWrite(INT8U* pInData, INT16U length, INT16U startpage, INT16U startbyte);
void PageRead(INT8U* InData, INT16U length, INT16U startpage, INT16U startbyte);
void AbsWrite(INT8U* pInData, INT16U length, INT32U absaddr);
void AbsWrite_Byte(INT32U absaddr, INT8U byte);
void AbsRead(INT8U* InData, INT16U length, INT32U absaddr);
/********************************************************************************
** 函数名:   SFlashMode
** 函数描述: 串口模式设置
** 参数:     [in] mode： 模式:0xB9为低功耗，0xAB:从低功耗模式退出；0xA3:高性能模式
** 返回:     设置成功返回TRUE，设置失败返回FALSE.
********************************************************************************/
void SFlashMode(INT8U mode);

#endif


