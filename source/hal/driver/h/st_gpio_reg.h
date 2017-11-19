/******************************************************************************
**
** Filename:     st_gpio_reg.h
** Copyright:    
** Description:  该模块主要实现GPIO资源的配置与管理
**
*******************************************************************************
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#ifndef ST_GPIO_CFG_H
#define ST_GPIO_CFG_H
    
/*
********************************************************************************
* define struct
********************************************************************************
*/
typedef struct {
    INT8U port;              /* 端口 */
    INT8U id;                /* 统一GPIO编号*/
    INT16U pin;              /* 管脚PIN编号 */
    INT8U direction;         /* 方向：输入/输出 */
    //INT8U mode;              /* 控制模式：开漏、推挽 M3无需单独设置*/
    //INT8U pupd;              /* 上下拉模式：上下、下拉、无 M3 无需单独设置 */
    INT8U level;             /* 初始化电平 */
    INT8U enable;            /* 初始化使能 */
} GPIO_REG_T;

/* 类注册表信息 */
typedef struct {
    INT8U             port;        /* 端口 */
    INT32U            gpio_base;   /* GPIO寄存器基准地址 */
    INT32U            gpio_rcc;    /* GPIO系统时钟 */
    GPIO_REG_T const *preg;        /* 各个类协议帧注册信息表 */
    INT8U             nreg;        /* 各个类的注册协议帧的个数 */
} GPIO_CLASS_T;

/*
********************************************************************************
* 定义统一GPIO通道编号
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
                 _ID_,
                 
#define END_GPIO_CFG(_PORT_)

typedef enum {
    #include "st_gpio_reg.def"
    GPIO_PIN_MAX
} GPIO_PIN_E;

/*
********************************************************************************
* 定义所有类统一编号
********************************************************************************
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

#define BEGIN_GPIO_CFG(_PORT_, _GPIO_BASE_, _RCC_)  \
               _PORT_,
               
#define GPIO_DEF(_PORT_, _ID_, _EN_, _PIN_, _DIRECTION_, _MODE_, _PU_PD_, _INIT_)
#define END_GPIO_CFG(_PORT_)


typedef enum {
    #include "st_gpio_reg.def"
    GPIO_CLASS_ID_MAX
} GPIO_CLASS_ID_E;


/*******************************************************************
** 函数名:     DAL_GPIO_GetRegClassInfo
** 函数描述:   获取对应类的注册信息
** 参数:       [in] nclass:统一编号的类编号，见GPS_CLASS_ID_E
** 返回:       成功返回注册表指针，失败返回0
********************************************************************/
GPIO_CLASS_T const *DAL_GPIO_GetRegClassInfo(INT8U nclass);

/*******************************************************************
*   函数名:    DAL_GPIO_GetRegClassMax
*   功能描述:  获取已注册的类个数
*　 参数:      无
*   返回值:    类个数
*******************************************************************/ 
INT8U DAL_GPIO_GetRegClassMax(void);

/*******************************************************************
** 函数名:     ST_GPIO_GetCfgTblInfo
** 函数描述:   获取对应GPIO的配置表信息
** 参数:       [in] id: GPIO编号,见GPIO_PIN_E
** 返回:       成功返回配置表指针，失败返回0
********************************************************************/
const GPIO_REG_T *ST_GPIO_GetCfgTblInfo(INT8U id);

/*******************************************************************
** 函数名:    ST_GPIO_GetCfgTblMax
** 函数描述:  获取已注册的GPIO个数
** 参数:      无
** 返回:      注册的个数
********************************************************************/
INT8U ST_GPIO_GetCfgTblMax(void);

#endif

