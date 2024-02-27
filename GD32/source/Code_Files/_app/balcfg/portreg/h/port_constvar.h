/********************************************************************************
**
** 文件名:     port_constvar.h
** 版权所有:   (c) 1998-2017 厦门雅迅网络股份有限公司
** 文件描述:   CONST变量的相关宏定义
** 创建人：        陈从华
** 注意:       该文件与平台无关
*********************************************************************************
**                  修改历史记录
**===============================================================================
**| 日期              | 作者        |  修改记录
**===============================================================================
**| 2017/05/12 | 黄运峰    |  规范化，修改优化
********************************************************************************/
#ifndef H_PORT_CONSTVAR
#define H_PORT_CONSTVAR

#ifdef GLOBALS_PORT_CONSTVAR

#ifdef  GPS_CONSTVAR_ID_DEF
#undef  GPS_CONSTVAR_ID_DEF
#endif

#define GPS_CONSTVAR_ID_DEF(_CONSTVAR_ID_)  \
    INT8U _CONSTVAR_ID_##_VAR;
    
#include "bal_constvar_id.def"
#include "app_constvar_id.def"

#else

#ifdef  GPS_CONSTVAR_ID_DEF
#undef  GPS_CONSTVAR_ID_DEF
#endif

#define GPS_CONSTVAR_ID_DEF(_CONSTVAR_ID_)  \
    extern INT8U _CONSTVAR_ID_##_VAR;
    
#include "bal_constvar_id.def"
#include "app_constvar_id.def"

#endif

/*
********************************************************************************
* 定义初始化CONST变量函数名
********************************************************************************
*/
#define NAME_GPS_FUN_INIT_CONSTVAR(_CONSTVAR_ID_)       GPS_FUN_INIT_CONSTVAR_##_CONSTVAR_ID_

/*
********************************************************************************
* 定义初始化化CONST变量函数
********************************************************************************
*/
#define DECLARE_GPS_FUN_INIT_CONSTVAR(_CONSTVAR_ID_)  \
        void _CONSTVAR_ID_##FUNC(void)                \
        {                                             \
            _CONSTVAR_ID_##_VAR = 0;                  \
        }                                             \
        void NAME_GPS_FUN_INIT_CONSTVAR(_CONSTVAR_ID_)(void)


/*******************************************************************
** 函数名:     PORT_InitConstVar
** 函数描述:   初始化各模块的CONST变量
** 参数:       无
** 返回:       无
********************************************************************/
void PORT_InitConstVar(void);

/*******************************************************************
** 函数名:     PORT_DeinitConstVar
** 函数描述:   模块去初始化函数
** 参数:       无
** 返回:       无
********************************************************************/
void PORT_DeinitConstVar(void);

#endif          /* end of H_PORT_CONSTVAR */


