/********************************************************************************
**
** 文件名:     dal_input_reg.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现I/O口传感器注册信息表管理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2010/08/09 | 叶德焰 |  创建第一版本
*********************************************************************************/
#define GLOBALS_INPUT_REG    1
#include "yx_include.h"
#include "st_gpio_drv.h"
#include "dal_input_drv.h"

/*
****************************************************************
*   定义各个函数申明
****************************************************************
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

#define INPUT_DEF(_PE_FATYPE_, _PORT_ID_, _PIN_, _LOW_TIME_, _HIGH_TIME_, _INIT_, _READ_)  \
                   void _INIT_(INT8U port);                                  \
                   BOOLEAN _READ_(INT8U port);
                   
#define END_INPUT_DEF(_PE_FATYPE_)

#include "dal_input_reg.def"

/*
****************************************************************
*   定义IO注册表
****************************************************************
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

#define BEGIN_INPUT_DEF(_PE_FATYPE_, _SET_FILTER_)    \
                   _PE_FATYPE_##BEGIN,
                   
#define INPUT_DEF(_PE_FATYPE_, _PORT_ID_, _PIN_, _LOW_TIME_, _HIGH_TIME_, _INIT_, _READ_)      \
                   _PORT_ID_##DEF,

#define END_INPUT_DEF(_PE_FATYPE_)                                         \
                   _PE_FATYPE_##END,

typedef enum {
    #include "dal_input_reg.def"
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

#if EN_FREECONST > 0  /* EN_FREECONST > 0 */
#define BEGIN_INPUT_DEF(_PE_FATYPE_, _SET_FILTER_)

#define INPUT_DEF(_PE_FATYPE_, _PORT_ID_, _PIN_, _LOW_TIME_, _HIGH_TIME_, _INIT_, _READ_)      \
                   {_PE_FATYPE_, _PORT_ID_, _PIN_, _LOW_TIME_, _HIGH_TIME_, _INIT_, _READ_},

#define END_INPUT_DEF(_PE_FATYPE_)


static const INPUT_IO_T s_input_tbl[] = {
    #include "dal_input_reg.def"
    {0}
};

#else /* EN_FREECONST == 0 */

static INPUT_IO_T s_input_tbl[INPUT_IO_MAX];

#endif /* end EN_FREECONST */
/*
****************************************************************
*   定义IO类的注册表
****************************************************************
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
                   (INPUT_IO_T const *)&s_input_tbl[_PE_FATYPE_##BEGIN - 2 * _PE_FATYPE_],   \
                   _PE_FATYPE_##END - _PE_FATYPE_##BEGIN - 1,                                \
                   _SET_FILTER_ \
                   },
                   
#define INPUT_DEF(_PE_FATYPE_, _PORT_ID_, _PIN_, _LOW_TIME_, _HIGH_TIME_, _INIT_, _READ_)
#define END_INPUT_DEF(_PE_FATYPE_)

static const INPUT_CLASS_T s_class_tbl[] = {
    #include "dal_input_reg.def"
    {0, 0, 0, 0}
};

#else /* EN_FREECONST == 0 */

static INPUT_CLASS_T s_class_tbl[INPUT_CLASS_MAX];

#endif /* end EN_FREECONST */

/*
****************************************************************
*   手动初始化
****************************************************************
*/
#if EN_FREECONST == 0
DECLARE_GPS_FUN_INIT_CONSTVAR(GPS_INPUT_REG_STATICDAT_ID)
{
    INT32U i;
    
    i = 0;
    
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
     
#define BEGIN_INPUT_DEF(_PE_FATYPE_, _SET_FILTER_)
#define INPUT_DEF(_PE_FATYPE_, _PORT_ID_, _PIN_, _LOW_TIME_, _HIGH_TIME_, _INIT_, _READ_) \
                   s_input_tbl[i].fatype   = _PE_FATYPE_;                    \
                   s_input_tbl[i].port     = _PORT_ID_;                      \
                   s_input_tbl[i].pin      = _PIN_;                          \
                   s_input_tbl[i].lowtime  = _LOW_TIME_;                     \
                   s_input_tbl[i].hightime = _HIGH_TIME_;                    \
                   s_input_tbl[i].initport = _INIT_;                         \
                   s_input_tbl[i].readport = _READ_;                         \
                   i++;
                   
#define END_INPUT_DEF(_PE_FATYPE_)

#include "dal_input_reg.def"
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
                  s_class_tbl[_PE_FATYPE_].fatype = _PE_FATYPE_;                                     \
                  s_class_tbl[_PE_FATYPE_].pio = (INPUT_IO_T const *)&s_input_tbl[_PE_FATYPE_##BEGIN - 2 * _PE_FATYPE_];              \
                  s_class_tbl[_PE_FATYPE_].nio = _PE_FATYPE_##END - _PE_FATYPE_##BEGIN - 1; \
                  s_class_tbl[_PE_FATYPE_].setfilterpara = _SET_FILTER_;

#define INPUT_DEF(_PE_FATYPE_, _PORT_ID_, _PIN_, _LOW_TIME_, _HIGH_TIME_, _INIT_, _READ_)
#define END_INPUT_DEF(_PE_FATYPE_)

#include "dal_input_reg.def"
}
#endif /* end EN_FREECONST == 0 */




/*******************************************************************
** 函数名:     DAL_INPUT_GetRegClassInfo
** 函数描述:   获取对应I/O类的注册信息
** 参数:       [in] nclass:统一编号的I/O类
** 返回:       成功返回注册表指针，失败返回0
********************************************************************/
INPUT_CLASS_T const *DAL_INPUT_GetRegClassInfo(INT8U nclass)
{
    if (nclass >= INPUT_CLASS_MAX) {
        return 0;
    }

    return (&s_class_tbl[nclass]);
}

/*******************************************************************
** 函数名:    DAL_INPUT_GetRegClassMax
** 功能描述:  获取已注册的I/O类个数
** 参数:      无
** 返回值:    I/O类个数
********************************************************************/
INT8U DAL_INPUT_GetRegClassMax(void)
{
    return INPUT_CLASS_MAX;
}

/*******************************************************************
** 函数名:     DAL_INPUT_GetCfgTblInfo
** 函数描述:   获取对应GPIO的配置表信息
** 参数:       [in] id: GPIO编号,见GPIO_COM_E
** 返回:       成功返回配置表指针，失败返回0
********************************************************************/
INPUT_IO_T const *DAL_INPUT_GetCfgTblInfo(INT8U id)
{
    if (id >= INPUT_IO_MAX) {
        return 0;
    }
    return (INPUT_IO_T const *)&s_input_tbl[id];
}

/*******************************************************************
** 函数名:    DAL_INPUT_GetIOMax
** 功能描述:  获取I/O口个数
** 参数:      无
** 返回值:    I/O口个数
********************************************************************/
INT8U DAL_INPUT_GetIOMax(void)
{
    return INPUT_IO_MAX;
}




