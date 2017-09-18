/********************************************************************************
**
** 文件名:     dal_output_reg.h
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
#ifndef DAL_OUTPUT_REG_H
#define DAL_OUTPUT_REG_H     1



/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    INT8U type;               /* 统一类型编号*/
    INT8U port;               /* 统一IO编号 */
    INT8U pin;                /* 对应GPIO管脚 */
    INT8U level;              /* 初始化电平 */
    void  (*initport)(INT8U port);  /* IO口初始化函数 */
    void  (*pullup)(INT8U port);    /* 输出高电平 */
    void  (*pulldown)(INT8U port);  /* 输出低电平 */
} OUTPUT_IO_T;


/*
********************************************************************************
* 定义所有IO统一编号
********************************************************************************
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
                    
#define OUTPUT_DEF(_TYPE_, _PORT_ID_, _PIN_, _LEVEL_, _INIT_, _PULLUP_, _PULLDOWN_)        \
          _PORT_ID_,
          
#define END_OUTPUT_DEF(_TYPE_)

typedef enum {
    #include "dal_output_reg.def"
    OUTPUT_IO_MAX
} OUTPUT_IO_E;

/*
********************************************************************************
* 定义所有IO类统一编号
********************************************************************************
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
                    
#define OUTPUT_DEF(_TYPE_, _PORT_ID_, _PIN_, _LEVEL_, _INIT_, _PULLUP_, _PULLDOWN_)

#define END_OUTPUT_DEF(_TYPE_)

typedef enum {
    #include "dal_output_reg.def"
    OUTPUT_CLASS_MAX
} OUTPUT_CLASS_E;




/*******************************************************************
** 函数名:     DAL_OUTPUT_GetRegInfo
** 函数描述:   获取对应I/O的注册信息
** 参数:       [in] port:统一编号的I/O
** 返回:       成功返回注册表指针，失败返回0
********************************************************************/
OUTPUT_IO_T const *DAL_OUTPUT_GetRegInfo(INT8U port);

/*******************************************************************
** 函数名:    DAL_OUTPUT_GetIOMax
** 功能描述:  获取I/O口个数
** 参数:      无
** 返回值:    I/O口个数
********************************************************************/
INT8U DAL_OUTPUT_GetIOMax(void);

#endif
