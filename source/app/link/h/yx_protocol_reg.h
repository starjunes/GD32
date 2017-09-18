/********************************************************************************
**
** 文件名:     yx_protocol_reg.h
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现协议数据发送通道注册信息表管理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2014/02/09 | 叶德焰 |  创建第一版本
*********************************************************************************/
#ifndef PROTOCOL_REG_H
#define PROTOCOL_REG_H       1



/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    INT8U   type;                                  /* 协议类型 */
    INT32U  msgid;                                 /* 协议ID */
    INT32U  tcp;                                   /* tcp */
    INT32U  udp;                                   /* udp */
    INT8U   sm;                                    /* sm通道 */
} PROTOCOL_REG_T;

typedef struct {
    INT8U              type;            /* 协议类型 */
    PROTOCOL_REG_T const *preg;         /* 各个协议类处理注册信息表 */
    INT8U              nreg;            /* 各个协议类的注册处理函数的个数 */
} PROTOCOL_CLASS_T;


/*
********************************************************************************
* 定义所有协议类统一编号
********************************************************************************
*/

#ifdef  BEGIN_PROTOCOL_DEF
#undef  BEGIN_PROTOCOL_DEF
#endif

#ifdef PROTOCOL_DEF
#undef PROTOCOL_DEF
#endif

#ifdef END_PROTOCOL_DEF
#undef END_PROTOCOL_DEF
#endif

#define BEGIN_PROTOCOL_DEF(_TYPE_) \
          _TYPE_,

#define PROTOCOL_DEF(_TYPE_, _MSG_ID_, _TCP_COM_, _UDP_COM_, _SM_)

#define END_PROTOCOL_DEF(_TYPE_)

typedef enum {
    #include "yx_protocol_reg.def"
    PROTOCOL_CLASS_MAX
} PROTOCOL_CLASS_E;

/*
********************************************************************************
* 定义所有协议统一编号
********************************************************************************
*/
#ifdef  BEGIN_PROTOCOL_DEF
#undef  BEGIN_PROTOCOL_DEF
#endif

#ifdef PROTOCOL_DEF
#undef PROTOCOL_DEF
#endif

#ifdef END_PROTOCOL_DEF
#undef END_PROTOCOL_DEF
#endif

#define BEGIN_PROTOCOL_DEF(_TYPE_)

#define PROTOCOL_DEF(_TYPE_, _MSG_ID_, _TCP_COM_, _UDP_COM_, _SM_) \
        _MSG_ID_##D,

#define END_PROTOCOL_DEF(_TYPE_)

typedef enum {
    #include "yx_protocol_reg.def"
    PROTOCOL_ID_MAX
} PROTOCOL_ID_E;


/*******************************************************************
** 函数名:     YX_PROTOCOL_GetRegClassInfo
** 函数描述:   获取对应协议类的注册信息
** 参数:       [in] nclass:统一编号的类编号，见PROTOCOL_CLASS_E
** 返回:       成功返回注册表指针，失败返回0
********************************************************************/
PROTOCOL_CLASS_T const *YX_PROTOCOL_GetRegClassInfo(INT8U nclass);

/***************************************************************
*   函数名:    YX_PROTOCOL_GetRegClassMax
*   功能描述:  获取已注册的协议类个数
*　 参数:      无
*   返回值:    类个数
***************************************************************/ 
INT8U YX_PROTOCOL_GetRegClassMax(void);

/*******************************************************************
** 函数名:     YX_PROTOCOL_GetRegInfo
** 函数描述:   获取对应协议的注册信息
** 参数:       [in] msgid:统一协议编号,见yx_protocol_type.h
** 返回:       成功返回注册表指针，失败返回0
********************************************************************/
PROTOCOL_REG_T const *YX_PROTOCOL_GetRegInfo(INT32U msgid);

/*******************************************************************
*   函数名:    YX_PROTOCOL_GetRegMax
*   功能描述:  获取已注册的个数
*　 参数:      无
*   返回值:    个数
*******************************************************************/
INT8U YX_PROTOCOL_GetRegMax(void);


#endif

