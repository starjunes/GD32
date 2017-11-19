/******************************************************************************
**
** Filename:     yx_bootmain.h
** Copyright:    
** Description:  该模块主要实现应用程序的引导运行
**
**=============================================================================
**             Revision history
**=============================================================================
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#ifndef YX_BOOTMAIN_H
#define YX_BOOTMAIN_H        1


#define CODE_FLAG_UPDATE     0x55AA5AA5      /* 无线下载请求标志 */
#define CODE_FLAG_VAR        "STM3"          /* 平台信息 */
#define CODE_FLAG_PLAT       "PLAT"          /* 平台层标志 */
#define CODE_FLAG_HAL        "HAL0"          /* HAL层标志 */
#define CODE_FLAG_APP        "APP0"          /* APP层标志 */

#define CODE_FLAG_WIRE       0x55            /* 有线下载标志，与上层保持一致 */
#define CODE_FLAG_WD         0xAA            /* 无线下载标志，与上层保持一致 */
#define MAX_WDFILE_SIZE      0x000E0000      /* 无线下载文件的最大长度,与上层保持一致 */
#define WD_SEGSIZE           512             /* 无线下载分段大小,与上层保持一致 */


/* APP头信息结构 */
typedef struct {
    INT8U fileflag;                    /* 文件头标志 */
    INT8U checksum[2];                 /* 文件校验和 */
    INT8U filesize[4];                 /* 文件长度：从文件头标志开始算 */
    INT8U res;                         /* 填充字节 */
    //INT8U date[3];                     /* 生成日期 */
    //INT8U time[3];                     /* 生成时间 */
    INT8U platflag[4];                 /* 平台标志 */
    INT8U appflag[4];                  /* 分层标识：APP */
    INT8U time[4];                      /* 编译时间入口地址 */
    INT8U entry[4];                    /* 入口地址 */
} APP_HEAD_T;

/* BOOT信息 */
typedef struct {
    char *version;
} CODE_INFO_T;


#ifdef GLOBALS_BOOTMAIN
CODE_INFO_T const g_code_info = {YX_VERSION_STR};

#else
extern CODE_INFO_T const g_code_info;
#endif



/*******************************************************************
** 函数名:     YX_EraseFlagFlashRegion
** 函数描述:   擦除固件升级标志FLASH区
** 参数:       无
** 返回:       成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN YX_EraseFlagFlashRegion(void);

/*******************************************************************
** 函数名:     YX_WriteFlagRegion
** 函数描述:   数据写入固件升级标志FLASH区
** 参数:       [in] offset: 命令编码
**             [in] data:   数据指针
**             [in] datalen:数据长度
** 返回:       成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN YX_WriteFlagRegion(INT32U offset, INT8U *data, INT16U datalen);

/*******************************************************************
** 函数名:     YX_EraseAppFlashRegion
** 函数描述:   擦除应用程序FLASH区
** 参数:       无
** 返回:       成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN YX_EraseAppFlashRegion(void);

/*******************************************************************
** 函数名:     YX_EraseAppFlashRegionEx
** 函数描述:   擦除应用程序FLASH区
** 参数:       无
** 返回:       成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN YX_EraseAppFlashRegionEx(INT32U offset);

/*******************************************************************
** 函数名:     YX_WriteAppCodeRegion
** 函数描述:   数据写入应用程序FLASH区
** 参数:       [in] offset: 命令编码
**             [in] data:   数据指针
**             [in] datalen:数据长度
** 返回:       成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN YX_WriteAppCodeRegion(INT32U offset, INT8U *data, INT16U datalen);

/*******************************************************************
** 函数名:     YX_CheckFileHead
** 函数描述:   检测头标志是否有效
** 参数:       [in] app_head: 文件头信息指针
** 返回:       有效返回true，无效返回false
********************************************************************/
BOOLEAN YX_CheckFileHead(APP_HEAD_T *app_head);

/*******************************************************************
** 函数名:     YX_CheckAppIsValid
** 函数描述:   检测APP应用层代码是否有效
** 参数:       无
** 返回:       有效返回TRUE,无效返回FALSE
********************************************************************/
BOOLEAN YX_CheckAppIsValid(void);

/*******************************************************************
** 函数名:     YX_CheckUpdateFlag
** 函数描述:   检测无线下载标志是否有效
** 参数:       无
** 返回:       有效返回TRUE,无效返回FALSE
********************************************************************/
BOOLEAN YX_CheckUpdateFlag(void);

#endif
