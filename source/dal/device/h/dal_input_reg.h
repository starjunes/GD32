/********************************************************************************
**
** 文件名:     dal_input_reg.h
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
#ifndef DAL_INPUT_REG_H
#define DAL_INPUT_REG_H      1



/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    INT8U     fatype;             /* IO类的父外设类型 */
    INT8U     port;               /* 统一IO编号 */
    INT8U     pin;                /* 对应GPIO管脚 */
    INT32U    lowtime;            /* 低脉冲滤波时间，单位：毫秒 */
    INT32U    hightime;           /* 高脉冲滤波时间，单位：毫秒 */
    void      (*initport)(INT8U port);  /* IO口初始化函数 */
    BOOLEAN   (*readport)(INT8U port);  /* IO口状态读函数 */
} INPUT_IO_T;

typedef struct {
    INT8U      fatype;            /* IO类的父外设类型 */
    INPUT_IO_T const *pio;        /* 各个IO类的注册IO信息表 */
    INT8U      nio;               /* 各个IO类的注册IO的个数 */
    void       (*setfilterpara)(INT8U port, INT16U ct_low, INT16U ct_high);/* 设置滤波参数函数 */
} INPUT_CLASS_T;


/*
********************************************************************************
* 定义所有IO统一编号
********************************************************************************
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
                    
#define INPUT_DEF(_PE_FATYPE_, _PORT_ID_, _PIN_, _LOW_TIME_, _HIGH_TIME_, _INIT_, _READ_)        \
          _PORT_ID_,
          
#define END_INPUT_DEF(_PE_FATYPE_)

typedef enum {
    #include "dal_input_reg.def"
    INPUT_IO_MAX
} INPUT_IO_E;

/*
********************************************************************************
* 定义所有IO类统一编号
********************************************************************************
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
          _PE_FATYPE_,
                    
#define INPUT_DEF(_PE_FATYPE_, _PORT_ID_, _PIN_, _LOW_TIME_, _HIGH_TIME_, _INIT_, _READ_)

#define END_INPUT_DEF(_PE_FATYPE_)

typedef enum {
    #include "dal_input_reg.def"
    INPUT_CLASS_MAX
} INPUT_CLASS_E;


/*******************************************************************
** 函数名:     DAL_INPUT_GetRegClassInfo
** 函数描述:   获取对应I/O类的注册信息
** 参数:       [in] nclass:统一编号的I/O类
** 返回:       成功返回注册表指针，失败返回0
********************************************************************/
INPUT_CLASS_T const *DAL_INPUT_GetRegClassInfo(INT8U nclass);

/*******************************************************************
** 函数名:    DAL_INPUT_GetRegClassMax
** 功能描述:  获取已注册的I/O类个数
** 参数:      无
** 返回值:    I/O类个数
********************************************************************/
INT8U DAL_INPUT_GetRegClassMax(void);

/*******************************************************************
** 函数名:     DAL_INPUT_GetCfgTblInfo
** 函数描述:   获取对应GPIO的配置表信息
** 参数:       [in] id: GPIO编号,见GPIO_COM_E
** 返回:       成功返回配置表指针，失败返回0
********************************************************************/
INPUT_IO_T const *DAL_INPUT_GetCfgTblInfo(INT8U id);

/*******************************************************************
** 函数名:    DAL_INPUT_GetIOMax
** 功能描述:  获取I/O口个数
** 参数:      无
** 返回值:    I/O口个数
********************************************************************/
INT8U DAL_INPUT_GetIOMax(void);

#endif
