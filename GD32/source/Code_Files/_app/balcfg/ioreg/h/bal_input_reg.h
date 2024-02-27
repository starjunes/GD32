/********************************************************************************
**
** 文件名:     bal_input_reg.h
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
#ifndef BAL_INPUTREG_H
#define BAL_INPUTREG_H      1
/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
/* 外设类型 */
typedef enum {
    PE_TYPE_NOUSED    = 0x00,     // 主板IO
    PE_TYPE_MAX
} PE_TYPE_E;

typedef struct {
    INT8U     fatype;             /* IO类的父外设类型 */
    INT8U     port;               /* 统一IO编号 */
    INT32U    lowtime;            /* 低脉冲滤波时间，单位：毫秒 */
    INT32U    hightime;           /* 高脉冲滤波时间，单位：毫秒 */
    void      (*initport)(void);  /* IO口初始化函数 */
    BOOLEAN   (*readport)(void);  /* IO口状态读函数 */
} INPUT_IO_T;

typedef struct {
    INT8U      fatype;            /* IO类的父外设类型 */
    INPUT_IO_T const *pio;        /* 各个IO类的注册IO信息表 */
    INT8U      nio;               /* 各个IO类的注册IO的个数 */
    void       (*setfilterpara)(INT8U port, INT16U ct_low, INT16U ct_high);/* 设置滤波参数函数 */
} INPUT_CLASS_T;

/*
*********************************************************************************
* 定义所有IO统一编号
*********************************************************************************
*/
#ifdef  BEGIN_INPUT_DEF
#undef  BEGIN_INPUT_DEF
#endif

#ifdef INPUT_DEF
#undef INPUT_DEF
#endif

#ifdef END_INPUT_DEF
#undef END_INPUT_DEF
#endif


#define BEGIN_INPUT_DEF(_PE_FATYPE_, _SET_FILTER_)
#if EN_FREECONST > 0
#define INPUT_DEF(_PE_FATYPE_, _PORT_ID_, _LOW_TIME_, _HIGH_TIME_, _INIT_, _READ_)        \
          _PORT_ID_,
#else
#define INPUT_DEF(_PE_FATYPE_, _PORT_ID_, _LOW_TIME_, _HIGH_TIME_, _INIT_, _READ_)        \
          _PORT_ID_##_ENUM,
#endif
#define END_INPUT_DEF(_PE_FATYPE_)

typedef enum {
    #include "bal_input_reg.def"
    INPUT_IO_MAX
} INPUT_IO_E;

/* 定义统一编号 */
#ifdef BEGIN_INPUT_DEF
#undef BEGIN_INPUT_DEF
#endif

#ifdef INPUT_DEF
#undef INPUT_DEF
#endif

#ifdef END_INPUT_DEF
#undef END_INPUT_DEF
#endif

#if EN_FREECONST == 0
#ifdef GLOBALS_INPUT_REG
#define BEGIN_INPUT_DEF(_PE_FATYPE_, _SET_FILTER_)
#define INPUT_DEF(_PE_FATYPE_, _PORT_ID_, _LOW_TIME_, _HIGH_TIME_, _INIT_, _READ_) \
               INT8U const _PORT_ID_ = _PORT_ID_##_ENUM;
                  
#define END_INPUT_DEF(_PE_FATYPE_)

#include "bal_input_reg.def"
#else

#define BEGIN_INPUT_DEF(_PE_FATYPE_, _SET_FILTER_)
#define INPUT_DEF(_PE_FATYPE_, _PORT_ID_, _LOW_TIME_, _HIGH_TIME_, _INIT_, _READ_) \
               extern INT8U const _PORT_ID_;
                  
#define END_INPUT_DEF(_PE_FATYPE_)

#include "bal_input_reg.def"
#endif
#endif /* #if EN_FREECONST == 0 */
/*
*********************************************************************************
* 定义所有IO类统一编号
*********************************************************************************
*/
#ifdef  BEGIN_INPUT_DEF
#undef  BEGIN_INPUT_DEF
#endif

#ifdef INPUT_DEF
#undef INPUT_DEF
#endif

#ifdef END_INPUT_DEF
#undef END_INPUT_DEF
#endif

#define BEGIN_INPUT_DEF(_PE_FATYPE_, _SET_FILTER_) \
          _PE_FATYPE_##CLASS,
                    
#define INPUT_DEF(_PE_FATYPE_, _PORT_ID_, _LOW_TIME_, _HIGH_TIME_, _INIT_, _READ_)

#define END_INPUT_DEF(_PE_FATYPE_)

typedef enum {
    #include "bal_input_reg.def"
    INPUT_CLASS_MAX
} INPUT_CLASS_E;


/********************************************************************************
** 函数名:     bal_input_GetRegClassInfo
** 函数描述:   获取对应I/O类的注册信息
** 参数:       [in] nclass:统一编号的I/O类
** 返回:       成功返回注册表指针，失败返回0
********************************************************************************/
INPUT_CLASS_T const *bal_input_GetRegClassInfo(INT8U nclass);

/********************************************************************************
*   函数名:    bal_input_GetRegClassMax
*   功能描述:  获取已注册的I/O类个数
*　 参数:      无
*   返回值:    I/O类个数
********************************************************************************/
INT8U bal_input_GetRegClassMax(void);

/********************************************************************************
*   函数名:    bal_input_GetIOMax
*   功能描述:  获取I/O口个数
*　 参数:      无
*   返回值:    I/O口个数
********************************************************************************/
INT8U bal_input_GetIOMax(void);

//-------------------------------------------------------------------------------
#endif /* BAL_INPUTREG_H */

