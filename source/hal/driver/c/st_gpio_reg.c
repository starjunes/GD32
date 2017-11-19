/******************************************************************************
**
** Filename:     st_gpio_reg.c
** Copyright:    
** Description:  该模块主要实现GPIO资源的配置管理
**
*******************************************************************************
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#include "hal_include.h"
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "st_gpio_reg.h"

/*
********************************************************************************
* 定义各个GPIO的配置信息
********************************************************************************
*/
#ifdef BEGIN_GPIO_CFG
#undef BEGIN_GPIO_CFG
#endif

#ifdef END_GPIO_CFG
#undef END_GPIO_CFG
#endif

#ifdef GPIO_DEF
#undef GPIO_DEF
#endif

#define BEGIN_GPIO_CFG(_PORT_, _GPIO_BASE_, _RCC_)

#define GPIO_DEF(_PORT_, _ID_, _EN_, _PIN_, _DIRECTION_, _MODE_, _PU_PD_, _INIT_) \
   {(INT8U)_PORT_, (INT8U)_ID_, _PIN_, (INT8U)_DIRECTION_, /*(INT8U)_MODE_, (INT8U)_PU_PD_,*/ (INT8U)_INIT_, (INT8U)_EN_},

#define END_GPIO_CFG(_PORT_)


static const GPIO_REG_T s_gpio_tbl[] = {
    #include "st_gpio_reg.def"
    {0}
};

/*
****************************************************************
*   定义类的注册表
****************************************************************
*/
#ifdef BEGIN_GPIO_CFG
#undef BEGIN_GPIO_CFG
#endif

#ifdef GPIO_DEF
#undef GPIO_DEF
#endif

#ifdef END_GPIO_CFG
#undef END_GPIO_CFG
#endif

#define BEGIN_GPIO_CFG(_PORT_, _GPIO_BASE_, _RCC_) \
          _PORT_##BEGIN,


#define GPIO_DEF(_PORT_, _ID_, _EN_, _PIN_, _DIRECTION_, _MODE_, _PU_PD_, _INIT_)       \
          _PORT_##_ID_##DEF,

#define END_GPIO_CFG(_PORT_) \
          _PORT_##END,

typedef enum {
    #include "st_gpio_reg.def"
    GPIO_DEF_MAX
} GPIO_DEF_E;

#ifdef BEGIN_GPIO_CFG
#undef BEGIN_GPIO_CFG
#endif

#ifdef GPIO_DEF
#undef GPIO_DEF
#endif

#ifdef END_GPIO_CFG
#undef END_GPIO_CFG
#endif

#define BEGIN_GPIO_CFG(_PORT_, _GPIO_BASE_, _RCC_)                                \
                   {(INT8U)_PORT_,                                                       \
                   (INT32U)_GPIO_BASE_,                                           \
                   (INT32U)_RCC_,                                                 \
                   (GPIO_REG_T const *)&s_gpio_tbl[_PORT_##BEGIN - 2 * _PORT_],   \
                   _PORT_##END - _PORT_##BEGIN - 1                                \
                   },
                   
#define GPIO_DEF(_PORT_, _ID_, _EN_, _PIN_, _DIRECTION_, _MODE_, _PU_PD_, _INIT_)
#define END_GPIO_CFG(_PORT_)

static const GPIO_CLASS_T s_class_tbl[] = {
    #include "st_gpio_reg.def"
    {0}
};



/*******************************************************************
** 函数名:     DAL_GPIO_GetRegClassInfo
** 函数描述:   获取对应类的注册信息
** 参数:       [in] nclass:统一编号的类编号，见GPS_CLASS_ID_E
** 返回:       成功返回注册表指针，失败返回0
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) GPIO_CLASS_T const *DAL_GPIO_GetRegClassInfo(INT8U nclass)
{
    if (nclass >= GPIO_CLASS_ID_MAX) {
        return 0;
    }

    return (GPIO_CLASS_T const *)(&s_class_tbl[nclass]);
}

/*******************************************************************
*   函数名:    DAL_GPIO_GetRegClassMax
*   功能描述:  获取已注册的类个数
*　 参数:      无
*   返回值:    类个数
*******************************************************************/ 
__attribute__ ((section ("IRQ_HANDLE"))) INT8U DAL_GPIO_GetRegClassMax(void)
{
    return GPIO_CLASS_ID_MAX;
}

/*******************************************************************
** 函数名:     ST_GPIO_GetCfgTblInfo
** 函数描述:   获取对应GPIO的配置表信息
** 参数:       [in] id: GPIO编号,见GPIO_COM_E
** 返回:       成功返回配置表指针，失败返回0
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) const GPIO_REG_T *ST_GPIO_GetCfgTblInfo(INT8U id)
{
    if (id >= GPIO_PIN_MAX) {
        return 0;
    }
    return &s_gpio_tbl[id];
}

/*******************************************************************
** 函数名:    ST_GPIO_GetCfgTblMax
** 函数描述:  获取已注册的GPIO个数
** 参数:      无
** 返回:      注册的个数
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) INT8U ST_GPIO_GetCfgTblMax(void)
{
    return GPIO_PIN_MAX;
}
