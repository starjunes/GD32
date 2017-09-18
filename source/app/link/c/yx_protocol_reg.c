/********************************************************************************
**
** 文件名:     yx_protocol_reg.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现协议数据发送通道注册信息表管理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2014/02/09 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "yx_includes.h"
#include "yx_protocol_type.h"
#include "yx_jt_linkman.h"
#include "yx_protocol_reg.h"


/*
****************************************************************
*   定义协议发送注册表
****************************************************************
*/

#ifdef BEGIN_PROTOCOL_DEF
#undef BEGIN_PROTOCOL_DEF
#endif

#ifdef PROTOCOL_DEF
#undef PROTOCOL_DEF
#endif

#ifdef END_PROTOCOL_DEF
#undef END_PROTOCOL_DEF
#endif

#if EN_FREECONST > 0  /* EN_FREECONST > 0 */

#define BEGIN_PROTOCOL_DEF(_TYPE_)
#define PROTOCOL_DEF(_TYPE_, _MSG_ID_, _TCP_COM_, _UDP_COM_, _SM_)                 \
                   {_TYPE_, _MSG_ID_, _TCP_COM_, _UDP_COM_, _SM_},
                 
#define END_PROTOCOL_DEF(_TYPE_)

static const PROTOCOL_REG_T s_protocol_tbl[] = {
                   #include "yx_protocol_reg.def"
                   {0}
                   };

#else /* EN_FREECONST == 0 */

static PROTOCOL_REG_T s_protocol_tbl[PROTOCOL_ID_MAX];

#endif /* end EN_FREECONST */

/*
****************************************************************
*   定义类的注册表
****************************************************************
*/
#define BEGIN_PROTOCOL_DEF(_TYPE_) \
          _TYPE_##BEGIN,

#define PROTOCOL_DEF(_TYPE_, _MSG_ID_, _TCP_COM_, _UDP_COM_, _SM_)       \
          _MSG_ID_##DEF,

#define END_PROTOCOL_DEF(_TYPE_) \
          _TYPE_##END,

typedef enum {
    #include "yx_protocol_reg.def"
    PROTOCOL_DEF_MAX
} PROTOCOL_DEF_E;

#ifdef BEGIN_PROTOCOL_DEF
#undef BEGIN_PROTOCOL_DEF
#endif

#ifdef PROTOCOL_DEF
#undef PROTOCOL_DEF
#endif

#ifdef END_PROTOCOL_DEF
#undef END_PROTOCOL_DEF
#endif

#if EN_FREECONST > 0  /* EN_FREECONST > 0 */
#define BEGIN_PROTOCOL_DEF(_TYPE_)                                                  \
                   {_TYPE_,                                                      \
                   (PROTOCOL_REG_T const *)&s_protocol_tbl[_TYPE_##BEGIN - 2 * _TYPE_], \
                   _TYPE_##END - _TYPE_##BEGIN - 1      \
                   },
                   
#define PROTOCOL_DEF(_TYPE_, _MSG_ID_, _TCP_COM_, _UDP_COM_, _SM_)
#define END_PROTOCOL_DEF(_TYPE_)

static const PROTOCOL_CLASS_T s_class_tbl[] = {
    #include "yx_protocol_reg.def"
    {0}
};

#else /* EN_FREECONST == 0 */

static PROTOCOL_CLASS_T s_class_tbl[PROTOCOL_CLASS_MAX];

#endif /* end EN_FREECONST */

/*
****************************************************************
*   手动初始化
****************************************************************
*/
#if EN_FREECONST == 0
DECLARE_GPS_FUN_INIT_CONSTVAR(GPS_PROTOCOL_REG_STATICDAT_ID)
{
    /* 初始化协议处理注册表 */
#ifdef BEGIN_PROTOCOL_DEF
#undef BEGIN_PROTOCOL_DEF
#endif

#ifdef PROTOCOL_DEF
#undef PROTOCOL_DEF
#endif

#ifdef END_PROTOCOL_DEF
#undef END_PROTOCOL_DEF
#endif

#define BEGIN_PROTOCOL_DEF(_TYPE_)
                   
#define PROTOCOL_DEF(_TYPE_, _MSG_ID_, _TCP_COM_, _UDP_COM_, _SM_)                 \
                   s_protocol_tbl[_MSG_ID_##D].type    = _TYPE_;  \
                   s_protocol_tbl[_MSG_ID_##D].msgid      = _MSG_ID_;    \
                   s_protocol_tbl[_MSG_ID_##D].tcp     = _TCP_COM_;    \
                   s_protocol_tbl[_MSG_ID_##D].udp     = _UDP_COM_;    \
                   s_protocol_tbl[_MSG_ID_##D].sm      = _SM_;
                   
#define END_PROTOCOL_DEF(_TYPE_)

#include "yx_protocol_reg.def"

    /* 初始化类的注册表 */
#ifdef BEGIN_PROTOCOL_DEF
#undef BEGIN_PROTOCOL_DEF
#endif

#ifdef PROTOCOL_DEF
#undef PROTOCOL_DEF
#endif

#ifdef END_PROTOCOL_DEF
#undef END_PROTOCOL_DEF
#endif

#define BEGIN_PROTOCOL_DEF(_TYPE_)                                                                              \
                   s_class_tbl[_TYPE_].type  = _TYPE_;                                                       \
                   s_class_tbl[_TYPE_].preg  = (PROTOCOL_REG_T const *)&s_protocol_tbl[_TYPE_##BEGIN - 2 * _TYPE_]; \
                   s_class_tbl[_TYPE_].nreg  = _TYPE_##END - _TYPE_##BEGIN - 1;
                   
#define PROTOCOL_DEF(_TYPE_, _MSG_ID_, _TCP_COM_, _UDP_COM_, _SM_)
#define END_PROTOCOL_DEF(_TYPE_)

#include "yx_protocol_reg.def"

}
#endif /* end EN_FREECONST == 0 */

/*******************************************************************
** 函数名:     YX_PROTOCOL_GetRegClassInfo
** 函数描述:   获取对应协议类的注册信息
** 参数:       [in] nclass:统一编号的类编号，见PROTOCOL_CLASS_E
** 返回:       成功返回注册表指针，失败返回0
********************************************************************/
PROTOCOL_CLASS_T const *YX_PROTOCOL_GetRegClassInfo(INT8U nclass)
{
    if (nclass >= PROTOCOL_CLASS_MAX) {
        return 0;
    }

    return (PROTOCOL_CLASS_T const *)(&s_class_tbl[nclass]);
}

/***************************************************************
*   函数名:    YX_PROTOCOL_GetRegClassMax
*   功能描述:  获取已注册的协议类个数
*　 参数:      无
*   返回值:    类个数
***************************************************************/ 
INT8U YX_PROTOCOL_GetRegClassMax(void)
{
    return PROTOCOL_CLASS_MAX;
}

/*******************************************************************
** 函数名:     YX_PROTOCOL_GetRegInfo
** 函数描述:   获取对应协议的注册信息
** 参数:       [in] msgid:统一协议编号,见yx_protocol_type.h
** 返回:       成功返回注册表指针，失败返回0
********************************************************************/
PROTOCOL_REG_T const *YX_PROTOCOL_GetRegInfo(INT32U msgid)
{
    INT8U i;
    
    for (i = 0; i < PROTOCOL_ID_MAX; i++) {
        if (s_protocol_tbl[i].msgid == msgid) {
            return (PROTOCOL_REG_T const *)&s_protocol_tbl[i];
        }
    }
    
    return (PROTOCOL_REG_T const *)0;
}

/*******************************************************************
*   函数名:    YX_PROTOCOL_GetRegMax
*   功能描述:  获取已注册的个数
*　 参数:      无
*   返回值:    个数
*******************************************************************/
INT8U YX_PROTOCOL_GetRegMax(void)
{
    return PROTOCOL_ID_MAX;
}






