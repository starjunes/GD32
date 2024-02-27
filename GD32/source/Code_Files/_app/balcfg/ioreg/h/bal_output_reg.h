/********************************************************************************
**
** 文件名:     bal_output_reg.h
** 版权所有:   (c) 1998-2017 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现I/O输出口注册信息表管理
**
*********************************************************************************
**                  修改历史记录
**===============================================================================
**| 日期              | 作者        |  修改记录
**===============================================================================
**| 2017/05/12 | 黄运峰    |  移植、修改、规范化
********************************************************************************/
#ifndef BAL_OUTPUTREG_H
#define BAL_OUTPUTREG_H    1

/*
*********************************************************************************
* 定义模块数据结构
*********************************************************************************
*/
typedef struct {
    INT8U type;               /* 统一类型编号*/
    INT8U port;               /* 统一IO编号 */
    INT8U level;              /* 初始化电平 */
    void  (*initport)(void);  /* IO口初始化函数 */
    void  (*pullup)(void);    /* 输出高电平 */
    void  (*pulldown)(void);  /* 输出低电平 */
} OUTPUT_IO_T;

/*
*********************************************************************************
* 定义所有IO统一编号
*********************************************************************************
*/

#ifdef  BEGIN_OUTPUT_DEF
#undef  BEGIN_OUTPUT_DEF
#endif

#ifdef OUTPUT_DEF
#undef OUTPUT_DEF
#endif

#ifdef END_OUTPUT_DEF
#undef END_OUTPUT_DEF
#endif


#define BEGIN_OUTPUT_DEF(_TYPE_)
#if EN_FREECONST > 0
#define OUTPUT_DEF(_TYPE_, _PORT_ID_, _LEVEL_, _INIT_, _PULLUP_, _PULLDOWN_)    \
          _PORT_ID_,
#else
#define OUTPUT_DEF(_TYPE_, _PORT_ID_, _LEVEL_, _INIT_, _PULLUP_, _PULLDOWN_)    \
          _PORT_ID_##_ENUM,
#endif
#define END_OUTPUT_DEF(_TYPE_)

typedef enum {
    #include "bal_output_reg.def"
    OUTPUT_IO_MAX
} OUTPUT_IO_E;

/* 定义统一编号 */
#ifdef BEGIN_OUTPUT_DEF
#undef BEGIN_OUTPUT_DEF
#endif

#ifdef OUTPUT_DEF
#undef OUTPUT_DEF
#endif

#ifdef END_OUTPUT_DEF
#undef END_OUTPUT_DEF
#endif

#if EN_FREECONST == 0
#ifdef GLOBALS_OUTPUT_REG
#define BEGIN_OUTPUT_DEF(_TYPE_)
#define OUTPUT_DEF(_TYPE_, _PORT_ID_, _LEVEL_, _INIT_, _PULLUP_, _PULLDOWN_) \
               INT8U const _PORT_ID_ = _PORT_ID_##_ENUM;
                  
#define END_OUTPUT_DEF(_TYPE_)

#include "bal_output_reg.def"
#else

#define BEGIN_OUTPUT_DEF(_TYPE_)
#define OUTPUT_DEF(_TYPE_, _PORT_ID_, _LEVEL_, _INIT_, _PULLUP_, _PULLDOWN_) \
               extern INT8U const _PORT_ID_;
                  
#define END_OUTPUT_DEF(_TYPE_)

#include "bal_output_reg.def"
#endif
#endif /* #if EN_FREECONST == 0 */
/*
*********************************************************************************
* 定义所有IO类统一编号
*********************************************************************************
*/

#ifdef  BEGIN_OUTPUT_DEF
#undef  BEGIN_OUTPUT_DEF
#endif

#ifdef OUTPUT_DEF
#undef OUTPUT_DEF
#endif

#ifdef END_OUTPUT_DEF
#undef END_OUTPUT_DEF
#endif


#define BEGIN_OUTPUT_DEF(_TYPE_) \
          _TYPE_,
                    
#define OUTPUT_DEF(_TYPE_, _PORT_ID_, _LEVEL_, _INIT_, _PULLUP_, _PULLDOWN_)

#define END_OUTPUT_DEF(_TYPE_)

typedef enum {
    #include "bal_output_reg.def"
    OUTPUT_CLASS_MAX
} OUTPUT_CLASS_E;

/********************************************************************************
** 函数名:     bal_output_GetRegInfo
** 函数描述:   获取对应I/O的注册信息
** 参数:       [in] port:统一编号的I/O
** 返回:       成功返回注册表指针，失败返回0
********************************************************************************/
OUTPUT_IO_T const *bal_output_GetRegInfo(INT8U port);

/********************************************************************************
*   函数名:    bal_output_GetIOMax
*   功能描述:  获取I/O口个数
*　 参数:      无
*   返回值:    I/O口个数
********************************************************************************/
INT8U bal_output_GetIOMax(void);

//-------------------------------------------------------------------------------
#endif /* BAL_OUTPUTREG_H */
