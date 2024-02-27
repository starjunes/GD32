/********************************************************************************
**
** 文件名:     bal_input_reg.c
** 版权所有:   (c) 1998-2017 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现I/O口传感器注册信息表管理
**
*********************************************************************************
**                  修改历史记录
**===============================================================================
**| 日期              | 作者        |  修改记录
**===============================================================================
**| 2017/05/12 | 黄运峰    |  移植、修改、规范化
********************************************************************************/
#define GLOBALS_INPUT_REG    1
#include "yx_includes.h"
#include "bal_input_reg.h"

/*
*********************************************************************************
*   定义各个函数声明
*********************************************************************************
*/
#ifdef BEGIN_INPUT_DEF
#undef BEGIN_INPUT_DEF
#endif

#ifdef INPUT_DEF
#undef INPUT_DEF
#endif

#ifdef END_INPUT_DEF
#undef END_INPUT_DEF
#endif

#define BEGIN_INPUT_DEF(_PE_FATYPE_, _SET_FILTER_) \
                   void _SET_FILTER_(INT8U port, INT16U ct_low, INT16U ct_high);

#define INPUT_DEF(_PE_FATYPE_, _PORT_ID_, _LOW_TIME_, _HIGH_TIME_, _INIT_, _READ_)  \
                   void _INIT_(void);                                  \
                   BOOLEAN _READ_(void);
                   
#define END_INPUT_DEF(_PE_FATYPE_)

#include "bal_input_reg.def"


/*
*********************************************************************************
*   定义IO注册表
*********************************************************************************
*/

#ifdef BEGIN_INPUT_DEF
#undef BEGIN_INPUT_DEF
#endif

#ifdef INPUT_DEF
#undef INPUT_DEF
#endif

#ifdef END_INPUT_DEF
#undef END_INPUT_DEF
#endif

#if EN_FREECONST > 0  /* EN_FREECONST > 0 */
#define BEGIN_INPUT_DEF(_PE_FATYPE_, _SET_FILTER_)    \
                   static const INPUT_IO_T _PE_FATYPE_##_TBL[] = {
                    
#define INPUT_DEF(_PE_FATYPE_, _PORT_ID_, _LOW_TIME_, _HIGH_TIME_, _INIT_, _READ_)      \
                   {_PE_FATYPE_, _PORT_ID_, _LOW_TIME_, _HIGH_TIME_, _INIT_, _READ_},
                 
#define END_INPUT_DEF(_PE_FATYPE_)                                         \
                   {0, 0, 0, 0, 0}};

#include "bal_input_reg.def"

#else /* EN_FREECONST == 0 */

#define BEGIN_INPUT_DEF(_PE_FATYPE_, _SET_FILTER_)    \
                   _PE_FATYPE_##BEGIN,
                   
#define INPUT_DEF(_PE_FATYPE_, _PORT_ID_, _LOW_TIME_, _HIGH_TIME_, _INIT_, _READ_)      \
                   _PORT_ID_##DEF,

#define END_INPUT_DEF(_PE_FATYPE_)                                         \
                   _PE_FATYPE_##END,

typedef enum {
    #include "bal_input_reg.def"
    INPUT_DEF_MAX
} INPUT_DEF_E;

#ifdef BEGIN_INPUT_DEF
#undef BEGIN_INPUT_DEF
#endif

#ifdef INPUT_DEF
#undef INPUT_DEF
#endif

#ifdef END_INPUT_DEF
#undef END_INPUT_DEF
#endif

#define BEGIN_INPUT_DEF(_PE_FATYPE_, _SET_FILTER_)    \
                   static INPUT_IO_T _PE_FATYPE_##_TBL[_PE_FATYPE_##END - _PE_FATYPE_##BEGIN];
                   
#define INPUT_DEF(_PE_FATYPE_, _PORT_ID_, _LOW_TIME_, _HIGH_TIME_, _INIT_, _READ_)
#define END_INPUT_DEF(_PE_FATYPE_)

#include "bal_input_reg.def"

#endif /* end EN_FREECONST */
/*
*********************************************************************************
*   定义IO类的注册表
*********************************************************************************
*/
#ifdef BEGIN_INPUT_DEF
#undef BEGIN_INPUT_DEF
#endif

#ifdef INPUT_DEF
#undef INPUT_DEF
#endif

#ifdef END_INPUT_DEF
#undef END_INPUT_DEF
#endif

#if EN_FREECONST > 0  /* EN_FREECONST > 0 */
#define BEGIN_INPUT_DEF(_PE_FATYPE_, _SET_FILTER_)    \
                   {_PE_FATYPE_, \
                   (INPUT_IO_T const *)_PE_FATYPE_##_TBL,                                \
                   sizeof(_PE_FATYPE_##_TBL) / sizeof(INPUT_IO_T) - 1,                   \
                   _SET_FILTER_ \
                   },
                   
#define INPUT_DEF(_PE_FATYPE_, _PORT_ID_, _LOW_TIME_, _HIGH_TIME_, _INIT_, _READ_)
#define END_INPUT_DEF(_PE_FATYPE_)

static const INPUT_CLASS_T s_class_tbl[] = {
    #include "bal_input_reg.def"
    {0, 0, 0, 0}
};

#else /* EN_FREECONST == 0 */

static INPUT_CLASS_T s_class_tbl[INPUT_CLASS_MAX];

#endif /* end EN_FREECONST */

/*
*********************************************************************************
*   手动初始化
*********************************************************************************
*/
#if EN_FREECONST == 0
DECLARE_GPS_FUN_INIT_CONSTVAR(GPS_INPUT_REG_STATICDAT_ID)
{
    INT32U i;
    
    /* IO注册表 */
#ifdef BEGIN_INPUT_DEF
#undef BEGIN_INPUT_DEF
#endif

#ifdef INPUT_DEF
#undef INPUT_DEF
#endif

#ifdef END_INPUT_DEF
#undef END_INPUT_DEF
#endif

#define BEGIN_INPUT_DEF(_PE_FATYPE_, _SET_FILTER_) \
                   i = 0;

#define INPUT_DEF(_PE_FATYPE_, _PORT_ID_, _LOW_TIME_, _HIGH_TIME_, _INIT_, _READ_) \
                   _PE_FATYPE_##_TBL[i].fatype   = _PE_FATYPE_;                    \
                   _PE_FATYPE_##_TBL[i].port     = _PORT_ID_;                      \
                   _PE_FATYPE_##_TBL[i].lowtime  = _LOW_TIME_;                     \
                   _PE_FATYPE_##_TBL[i].hightime = _HIGH_TIME_;                    \
                   _PE_FATYPE_##_TBL[i].initport = _INIT_;                         \
                   _PE_FATYPE_##_TBL[i].readport = _READ_;                         \
                   i++;
                   
#define END_INPUT_DEF(_PE_FATYPE_)

#include "bal_input_reg.def"
    /* IO类的注册表 */
#ifdef BEGIN_INPUT_DEF
#undef BEGIN_INPUT_DEF
#endif

#ifdef INPUT_DEF
#undef INPUT_DEF
#endif

#ifdef END_INPUT_DEF
#undef END_INPUT_DEF
#endif

#define BEGIN_INPUT_DEF(_PE_FATYPE_, _SET_FILTER_) \
                  s_class_tbl[_PE_FATYPE_##CLASS].fatype = _PE_FATYPE_;                                     \
                  s_class_tbl[_PE_FATYPE_##CLASS].pio = (INPUT_IO_T const *)_PE_FATYPE_##_TBL;              \
                  s_class_tbl[_PE_FATYPE_##CLASS].nio = sizeof(_PE_FATYPE_##_TBL) / sizeof(INPUT_IO_T) - 1; \
                  s_class_tbl[_PE_FATYPE_##CLASS].setfilterpara = _SET_FILTER_;

#define INPUT_DEF(_PE_FATYPE_, _PORT_ID_, _LOW_TIME_, _HIGH_TIME_, _INIT_, _READ_)
#define END_INPUT_DEF(_PE_FATYPE_)

#include "bal_input_reg.def"
}
#endif /* end EN_FREECONST == 0 */

/********************************************************************************
** 函数名:     bal_input_GetRegClassInfo
** 函数描述:   获取对应I/O类的注册信息
** 参数:       [in] nclass:统一编号的I/O类
** 返回:       成功返回注册表指针，失败返回0
********************************************************************************/
INPUT_CLASS_T const *bal_input_GetRegClassInfo(INT8U nclass)
{
    if (nclass >= INPUT_CLASS_MAX) {
        return 0;
    }

    return (&s_class_tbl[nclass]);
}

/********************************************************************************
*   函数名:    bal_input_GetRegClassMax
*   功能描述:  获取已注册的I/O类个数
*　 参数:      无
*   返回值:    I/O类个数
********************************************************************************/
INT8U bal_input_GetRegClassMax(void)
{
    return INPUT_CLASS_MAX;
}

/********************************************************************************
*   函数名:    bal_input_GetIOMax
*   功能描述:  获取I/O口个数
*　 参数:      无
*   返回值:    I/O口个数
********************************************************************************/
INT8U bal_input_GetIOMax(void)
{
    return INPUT_IO_MAX;
}

//------------------------------------------------------------------------------
/* End of File */



