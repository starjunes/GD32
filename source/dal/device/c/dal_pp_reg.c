/********************************************************************************
**
** 文件名:     dal_pp_reg.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现公共参数文件存储驱动注册信息表管理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2011/11/15 | 叶德焰 |  创建第一版本
*********************************************************************************/
#define  GLOBALS_PP_REG   1

#include "yx_include.h"
#include "yx_misc.h"
#include "dal_pp_reg.h"
#include "dal_pp_misc.h"
#include "dal_pp_drv.h"


/*
****************************************************************
*   定义映射内存
****************************************************************
*/

#ifdef BEGIN_PP_DEF
#undef BEGIN_PP_DEF
#endif

#ifdef PP_DEF
#undef PP_DEF
#endif

#ifdef END_PP_DEF
#undef END_PP_DEF
#endif

#define BEGIN_PP_DEF(_PP_TYPE_, _DLY_, _DESC_)  \
        INT8U _PP_TYPE_##_MEM_BUF[0

#define PP_DEF(_PP_TYPE_, _PP_ID_, _LEN_, _BAK_, _I_PTR_) \
        + _LEN_ + sizeof(PP_HEAD_T) + 1
#define END_PP_DEF(_PP_TYPE_) \
        ];

#include "dal_pp_reg.def"
/*
****************************************************************
*   定义公共参数文件注册表
****************************************************************
*/
#ifdef BEGIN_PP_DEF
#undef BEGIN_PP_DEF
#endif

#ifdef PP_DEF
#undef PP_DEF
#endif

#ifdef END_PP_DEF
#undef END_PP_DEF
#endif


#define BEGIN_PP_DEF(_PP_TYPE_, _DLY_, _DESC_)

#define PP_DEF(_PP_TYPE_, _PP_ID_, _LEN_, _BAK_, _I_PTR_) \
               {_PP_TYPE_, _PP_ID_, _LEN_, _BAK_, _I_PTR_},

#define END_PP_DEF(_PP_TYPE_)

static const PP_REG_T s_pp_regtbl[] = {
    #include "dal_pp_reg.def"
    {0}
};


/*
****************************************************************
*   定义类的注册表
****************************************************************
*/
#ifdef BEGIN_PP_DEF
#undef BEGIN_PP_DEF
#endif

#ifdef PP_DEF
#undef PP_DEF
#endif

#ifdef END_PP_DEF
#undef END_PP_DEF
#endif

#define BEGIN_PP_DEF(_PP_TYPE_, _DLY_, _DESC_) \
          _PP_TYPE_##BEGIN,


#define PP_DEF(_PP_TYPE_, _PP_ID_, _LEN_, _BAK_, _I_PTR_)       \
          _PP_ID_##DEF,

#define END_PP_DEF(_PP_TYPE_) \
          _PP_TYPE_##END,

typedef enum {
    #include "dal_pp_reg.def"
    PP_DEF_MAX
} PP_DEF_E;

#ifdef BEGIN_PP_DEF
#undef BEGIN_PP_DEF
#endif

#ifdef PP_DEF
#undef PP_DEF
#endif

#ifdef END_PP_DEF
#undef END_PP_DEF
#endif

#define BEGIN_PP_DEF(_PP_TYPE_, _DLY_, _DESC_)                                                            \
                   {_PP_TYPE_, _DLY_, /*_DESC_,*/ sizeof(_PP_TYPE_##_MEM_BUF),  (INT8U *)_PP_TYPE_##_MEM_BUF, \
                   (PP_REG_T const *)&s_pp_regtbl[_PP_TYPE_##BEGIN - 2 * _PP_TYPE_],                      \
                   _PP_TYPE_##END - _PP_TYPE_##BEGIN - 1                                                  \
                   },
                   
#define PP_DEF(_PP_TYPE_, _PP_ID_, _LEN_, _BAK_, _I_PTR_)
#define END_PP_DEF(_PP_TYPE_)

static const PP_CLASS_T s_class_tbl[] = {
    #include "dal_pp_reg.def"
    {0}
};




/*******************************************************************
** 函数名:     DAL_PP_GetRegClassInfo
** 函数描述:   获取对应公共参数类的注册信息
** 参数:       [in] nclass:统一编号的类编号，见PP_CLASS_ID_E
** 返回:       成功返回注册表指针，失败返回0
********************************************************************/
PP_CLASS_T const *DAL_PP_GetRegClassInfo(INT8U nclass)
{
    if (nclass >= PP_CLASS_ID_MAX) {
        return 0;
    }

    return (PP_CLASS_T const *)(&s_class_tbl[nclass]);
}

/*******************************************************************
*   函数名:    DAL_PP_GetRegClassMax
*   功能描述:  获取已注册的公共参数类个数
*　 参数:      无
*   返回值:    类个数
*******************************************************************/ 
INT8U DAL_PP_GetRegClassMax(void)
{
    return PP_CLASS_ID_MAX;
}

/*******************************************************************
** 函数名:     DAL_PP_GetRegInfo
** 函数描述:   获取对应公共参数的注册信息
** 参数:       [in] id:统一编号的参数编号，见PP_ID_E
** 返回:       成功返回注册表指针，失败返回0
********************************************************************/
PP_REG_T const *DAL_PP_GetRegInfo(INT8U id)
{
    if (id >= PP_ID_MAX) {
        return 0;
    }
    return (PP_REG_T const *)&s_pp_regtbl[id];
}

/*******************************************************************
*   函数名:    DAL_PP_GetRegPPMax
*   功能描述:  获取已注册的公共参数个数
*　 参数:      无
*   返回值:    个数
*******************************************************************/
INT8U DAL_PP_GetRegPPMax(void)
{
    return PP_ID_MAX;
}
