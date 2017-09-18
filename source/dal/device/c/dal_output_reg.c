/********************************************************************************
**
** 文件名:     dal_output_reg.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现I/O输出口注册信息表管理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2012/02/09 | 叶德焰 |  创建第一版本
*********************************************************************************/
#define GLOBALS_OUTPUT_REG    1
#include "yx_include.h"
#include "st_gpio_drv.h"
#include "dal_output_reg.h"

/*
****************************************************************
*   定义各个函数申明
****************************************************************
*/
#ifdef BEGIN_OUTPUT_DEF
#undef BEGIN_OUTPUT_DEF
#endif

#ifdef OUTPUT_DEF
#undef OUTPUT_DEF
#endif

#ifdef END_OUTPUT_DEF
#undef END_OUTPUT_DEF
#endif

#define BEGIN_OUTPUT_DEF(_TYPE_)
#define OUTPUT_DEF(_TYPE_, _PORT_ID_, _PIN_, _LEVEL_, _INIT_, _PULLUP_, _PULLDOWN_)  \
                   void _INIT_(INT8U port);                                         \
                   void _PULLUP_(INT8U port);                                       \
                   void _PULLDOWN_(INT8U port);
                   
#define END_OUTPUT_DEF(_TYPE_)

#include "dal_output_reg.def"


/*
****************************************************************
*   定义IO注册表
****************************************************************
*/

#ifdef BEGIN_OUTPUT_DEF
#undef BEGIN_OUTPUT_DEF
#endif

#ifdef OUTPUT_DEF
#undef OUTPUT_DEF
#endif

#ifdef END_OUTPUT_DEF
#undef END_OUTPUT_DEF
#endif

#if EN_FREECONST > 0  /* EN_FREECONST > 0 */
#define BEGIN_OUTPUT_DEF(_TYPE_)
#define OUTPUT_DEF(_TYPE_, _PORT_ID_, _PIN_, _LEVEL_, _INIT_, _PULLUP_, _PULLDOWN_)      \
                   {_TYPE_, _PORT_ID_, _PIN_, _LEVEL_, _INIT_, _PULLUP_, _PULLDOWN_},
                 
#define END_OUTPUT_DEF(_TYPE_)
                   
static const OUTPUT_IO_T s_io_regtbl[] = {
    #include "dal_output_reg.def"
    {0}
    };

#else /* EN_FREECONST == 0 */

static OUTPUT_IO_T s_io_regtbl[OUTPUT_IO_MAX];

#endif /* end EN_FREECONST */

/*
****************************************************************
*   手动初始化
****************************************************************
*/
#if EN_FREECONST == 0
DECLARE_GPS_FUN_INIT_CONSTVAR(GPS_OUTPUT_REG_STATICDAT_ID)
{
    /* IO注册表 */
#ifdef BEGIN_OUTPUT_DEF
#undef BEGIN_OUTPUT_DEF
#endif

#ifdef OUTPUT_DEF
#undef OUTPUT_DEF
#endif

#ifdef END_OUTPUT_DEF
#undef END_OUTPUT_DEF
#endif

#define BEGIN_OUTPUT_DEF(_TYPE_)

#define OUTPUT_DEF(_TYPE_, _PORT_ID_, _PIN_, _LEVEL_, _INIT_, _PULLUP_, _PULLDOWN_) \
                   s_io_regtbl[_PORT_ID_].type     = _TYPE_;                 \
                   s_io_regtbl[_PORT_ID_].port     = _PORT_ID_;              \
                   s_io_regtbl[_PORT_ID_].pin      = _PIN_;                  \
                   s_io_regtbl[_PORT_ID_].level    = _LEVEL_;                \
                   s_io_regtbl[_PORT_ID_].initport = _INIT_;                 \
                   s_io_regtbl[_PORT_ID_].pullup   = _PULLUP_;               \
                   s_io_regtbl[_PORT_ID_].pulldown = _PULLDOWN_;
                   
#define END_OUTPUT_DEF(_TYPE_)

#include "dal_output_reg.def"
}
#endif /* end EN_FREECONST == 0 */

/*******************************************************************
** 函数名:     DAL_OUTPUT_GetRegInfo
** 函数描述:   获取对应I/O的注册信息
** 参数:       [in] port:统一编号的I/O
** 返回:       成功返回注册表指针，失败返回0
********************************************************************/
OUTPUT_IO_T const *DAL_OUTPUT_GetRegInfo(INT8U port)
{
    if (port >= OUTPUT_IO_MAX) {
        return 0;
    }

    return (OUTPUT_IO_T const *)(&s_io_regtbl[port]);
}

/*******************************************************************
** 函数名:    DAL_OUTPUT_GetIOMax
** 功能描述:  获取I/O口个数
** 参数:      无
** 返回值:    I/O口个数
********************************************************************/
INT8U DAL_OUTPUT_GetIOMax(void)
{
    return OUTPUT_IO_MAX;
}




