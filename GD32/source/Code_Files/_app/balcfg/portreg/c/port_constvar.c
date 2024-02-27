/********************************************************************************
**
** 文件名:     port_constvar.c
** 版权所有:   (c) 1998-2017 厦门雅迅网络股份有限公司
** 文件描述:   初始化CONST变量
** 创建人：        陈从华
** 注意:       该文件与平台无关
*********************************************************************************
**                  修改历史记录
**===============================================================================
**| 日期              | 作者        |  修改记录
**===============================================================================
**| 2017/05/12 | 黄运峰    |  规范化，修改优化
********************************************************************************/
#define GLOBALS_PORT_CONSTVAR    1
#include "yx_includes.h"
#include "bal_tools.h"
#include "port_constvar.h"


#if EN_FREECONST > 0

/********************************************************************************
** 函数名:     PORT_InitConstVar
** 函数描述:   初始化各模块的CONST变量
** 参数:       无
** 返回:       无
********************************************************************************/
void PORT_InitConstVar(void)
{
    ;
}

/********************************************************************************
** 函数名:     PORT_DeinitConstVar
** 函数描述:   模块去初始化函数
** 参数:       无
** 返回:       无
********************************************************************************/
void PORT_DeinitConstVar(void)
{
    ;
}
#else           /* #if EN_FREECONST > 0 */

//#include "..\\..\\main\\h\\gps_config.h"

#ifdef  GPS_CONSTVAR_ID_DEF
#undef  GPS_CONSTVAR_ID_DEF
#endif

#define GPS_CONSTVAR_ID_DEF(_CONSTVAR_ID_)                      \
        extern NAME_GPS_FUN_INIT_CONSTVAR(_CONSTVAR_ID_)(void);

#include "bal_constvar_id.def"
#include "app_constvar_id.def"



/********************************************************************************
** 函数名:     PORT_InitConstVar
** 函数描述:   初始化各模块的CONST变量
** 参数:       无
** 返回:       无
********************************************************************************/
void PORT_InitConstVar(void)
{
#ifdef  GPS_CONSTVAR_ID_DEF
#undef  GPS_CONSTVAR_ID_DEF
#endif

#define GPS_CONSTVAR_ID_DEF(_CONSTVAR_ID_)                      \
        NAME_GPS_FUN_INIT_CONSTVAR(_CONSTVAR_ID_)();
    
    #include "bal_constvar_id.def"
    #include "app_constvar_id.def"
}

/********************************************************************************
** 函数名:     PORT_DeinitConstVar
** 函数描述:   模块去初始化函数
** 参数:       无
** 返回:       无
********************************************************************************/
void PORT_DeinitConstVar(void)
{
    ;
}
#endif          /* end of EN_FREECONST */

//------------------------------------------------------------------------------
/* End of File */
