/********************************************************************************
**
** 文件名:     yx_version.c
** 版权所有:   (c) 1998-2017 厦门雅迅网络股份有限公司
** 文件描述:   该模块程序版本号查询功能
**
*********************************************************************************
**                  修改历史记录
**===============================================================================
**| 日期              | 作者        |  修改记录
**===============================================================================
**| 2017/05/15 | 黄运峰    |  移植、修改、规范化
********************************************************************************/
#include "yx_includes.h"
#include "yx_version.h"
#include "bal_tools.h"
#include "build_file.h"
#include "yx_uds_did.h"
/*
********************************************************************************
*                   模块数据类型、变量及宏定义
********************************************************************************
*/
static char s_ful_version_str[MAX_VERSION_LEN + 1];
static char s_version_buf[MAX_VERSION_LEN];

static BOOLEAN s_ver_assembled = FALSE;

/********************************************************************************
*    函 数 名： YX_GetVersionDate
*    功能说明： 获取生成当前版本的日期信息
*    输入参数： 无
*    返回:      日期信息字符串指针
********************************************************************************/
char* YX_GetVersionDate(void)
{
    return __DATE__;
}

/********************************************************************************
*    函 数 名： YX_GetVersionTime
*    功能说明： 获取生成当前版本的时间信息
*    输入参数： 无
*    返回:     时间信息字符串指针
********************************************************************************/
char* YX_GetVersionTime(void)
{
    return __TIME__;
}
/********************************************************************************
*    函 数 名： YX_GetClientVersion
*    功能说明： 获取客户版本号
*    输入参数： 无
*    返回:      版本号字符串指针
********************************************************************************/
char *YX_GetClientVersion(void)
{
    char p_ver[11];
		char *tmpptr;
		INT8U pos;
		
    tmpptr = (char*)CLIENT_VERSION_STR;
		pos = strlen(CLIENT_VERSION_STR);
    memcpy(p_ver, tmpptr, pos);
		
    if(YX_Get_Did0110()) {
			 tmpptr = (char*)QINGDAO;
    } else {
       tmpptr = (char*)CHANGCHUN; 
    }
		strcat(p_ver, tmpptr);

		return p_ver;
}

/********************************************************************************
*    函 数 名： YX_GetVersion
*    功能说明： 获取当前程序版本号(含APP/DAL层和HAL层)
*    输入参数： 无
*    返回:      版本号字符串指针
********************************************************************************/
char *YX_GetVersion(void)
{
    char *p_ver, *tmpptr1, *tmpptr2;
    INT8U pos1, pos2, len;

    //if (!s_ver_assembled) {
        memset(s_ful_version_str, 0, sizeof(s_ful_version_str));
        tmpptr1 = APPDAL_VERSION_STR;
        //pos2 = bal_FindCharPos((INT8U *)tmpptr1, '-', 2, strlen(tmpptr1))+1;/*提取c0918002-110r7s32k*/
        pos2 = strlen(APPDAL_VERSION_STR);
        memcpy(s_ful_version_str, tmpptr1, pos2);
        p_ver = s_ful_version_str + pos2;

        tmpptr2 = PORT_GetHalVersion();
        pos1 = bal_FindCharPos((INT8U *)tmpptr2, '-', 1, strlen(tmpptr2));
        pos2 = bal_FindCharPos((INT8U *)tmpptr2, '-', 2, strlen(tmpptr2));
        len = pos2 - pos1;
        memcpy(p_ver, tmpptr2 + pos1, len);
        p_ver += len;
        tmpptr2 += pos2;

        //len = strlen(tmpptr1);
        //memcpy(p_ver, tmpptr1, len);
       // p_ver += len;

        strcat(p_ver, tmpptr2);

        s_ver_assembled = TRUE;
  //  }

    return s_ful_version_str;
}

/********************************************************************************
*    函 数 名： YX_GetVersion_EXT
*    功能说明： 获取当前程序版本号，加上日期和时间显示
*    输入参数： 无
*    返回:     版本号字符串指针
********************************************************************************/
char* YX_GetVersion_EXT(void)
{
    INT8U len;
    char *p_str;

    memset(s_version_buf, 0, sizeof(s_version_buf));

    strcpy(s_version_buf, YX_GetVersion());
    len = strlen(YX_GetVersion());
    s_version_buf[len++] = '\n';

    p_str = YX_GetVersionDate();
    if ((len + strlen(p_str)) >= sizeof(s_version_buf) - 2)  goto ret;
    strcpy(s_version_buf + len, p_str);
    len += strlen(p_str);
    s_version_buf[len++] = '\n';

    p_str = YX_GetVersionTime();
    if ((len + strlen(p_str)) >= sizeof(s_version_buf) - 2)  goto ret;
    strcpy(s_version_buf + len, p_str);
    len += strlen(p_str);
    s_version_buf[len++] = '\n';

    /* DAL库版本 */
    p_str = bal_GetDalLibVersion();
    if ((len + strlen(p_str)) >= sizeof(s_version_buf) - 2)  goto ret;
    strcpy(s_version_buf + len, p_str);
    len += strlen(p_str);

ret:
    return s_version_buf;
}

//------------------------------------------------------------------------------
/* End of File */
