/********************************************************************************
**
** 文件名:     yx_version.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块定义版本号
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2012/02/09 | 叶德焰 |  创建第一版本
*********************************************************************************/
#define GLOBALS_VERSION      1
#include "yx_include.h"
#include "hal_flash_drv.h"
#include "yx_bootmain.h"

/*
********************************************************************************
* 定义模块配置参数
********************************************************************************
*/
#define MAX_VERSION_LEN      50
#define YX_VERSION_STR       "c0226001-110rv01m3-701_T06"

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/

typedef struct {
    char *info;
} VER_CODE_INFO_T;


/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static char s_version[MAX_VERSION_LEN];
VER_CODE_INFO_T const g_code_time = {__DATE__" "__TIME__};



/*******************************************************************
** 函数名:     YX_GetVersion
** 函数描述:   获取版本
** 参数:       无
** 返回:       返回库版本指针
********************************************************************/
char *YX_GetVersion(void)
{
    /* boot还未做,先屏蔽 */

    INT32U pos, entry_info;
    char const *version;
    APP_HEAD_T *p_head;
    CODE_INFO_T *pinfo;
 
    YX_STRCPY(s_version, YX_VERSION_STR);

    p_head = (APP_HEAD_T *)(FLASH_BOOT_HEAD_BASE);                             /* 头信息 */
    memcpy(&entry_info, p_head->entry, sizeof(entry_info));
    pinfo = (CODE_INFO_T *)(FLASH_BOOT_HEAD_BASE + entry_info + T_OFFSET(APP_HEAD_T, entry));/* 固件信息 */
    version = (char *)(pinfo->version);
    if ((INT32U)version > FLASH_BOOT_BASE && (INT32U)version < FLASH_BOOT_BASE + FLASH_BOOT_SIZE) {                                                  /* ????? */
        pos = YX_FindCharPos((INT8U *)version, '-', 1, YX_STRLEN(version));
        YX_STRCAT(s_version, &version[pos]);
    } else {
        YX_STRCAT(s_version, "-x.x");
    }

    
    return s_version;
}



