/********************************************************************************
**
** 文件名:     os_reg.h
** 版权所有:   (c) 1998-2017 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现任务和消息的注册信息表管理
** 创建人：        叶徳焰
*********************************************************************************
**                  修改历史记录
**===============================================================================
**| 日期              | 作者        |  修改记录
**===============================================================================
**| 2017/05/12 | 黄运峰    |  规范化，修改优化
********************************************************************************/
#ifndef OS_REG_H
#define OS_REG_H    1
#include "yx_includes.h"
/*
*********************************************************************************
* 定义模块数据结构
*********************************************************************************
*/
typedef struct {
    INT16U  tskid;           /* 统一的任务编号 */
    INT16U  msgid;           /* 统一的消息编号 */
    void    (*proc)(INT16U tskid, INT16U msgid, INT32U lpara, INT32U hpara); /* 消息处理函数 */
} OS_MSG_TBL_T;

typedef struct {
    void    (*init)(void);   /* 任务初始化函数 */
} OS_INIT_TBL_T;

typedef struct {
    INT16U         tskid;    /* 统一的任务编号 */
    OS_MSG_TBL_T  *pmsg;     /* 各个任务的消息注册表 */
    INT32U         nmsg;     /* 各个任务的注册消息的个数 */
    
    OS_INIT_TBL_T *pinit;    /* 各个任务的初始化函数的注册表 */
    INT32U         ninit;    /* 各个任务的注册初始化函数的个数 */
} OS_TSK_TBL_T;

/*
*********************************************************************************
* 定义统一编号的任务ID
*********************************************************************************
*/

#ifdef BEGIN_MSG_DEF
#undef BEGIN_MSG_DEF
#endif

#ifdef MSG_DEF
#undef MSG_DEF
#endif

#ifdef END_MSG_DEF
#undef END_MSG_DEF
#endif

#if EN_FREECONST > 0
#define BEGIN_MSG_DEF(_TSK_ID_)    \
                   _TSK_ID_,
#else
#define BEGIN_MSG_DEF(_TSK_ID_)    \
                   _TSK_ID_##_ENUM,
#endif

#define MSG_DEF(_TSK_ID_, _MSG_ID_, _PROC_)
#define END_MSG_DEF(_TSK_ID_)


typedef enum {
    #include "os_msg_reg.def"
    TSK_ID_MAX
} OS_TSK_ID_E;

/* 定义统一编号 */
#ifdef BEGIN_MSG_DEF
#undef BEGIN_MSG_DEF
#endif

#ifdef MSG_DEF
#undef MSG_DEF
#endif

#ifdef END_MSG_DEF
#undef END_MSG_DEF
#endif

#if EN_FREECONST == 0
#ifdef GLOBALS_OS_REG
#define BEGIN_MSG_DEF(_TSK_ID_) \
               INT8U const _TSK_ID_ = _TSK_ID_##_ENUM;
               
#define MSG_DEF(_TSK_ID_, _MSG_ID_, _PROC_)
                  
#define END_MSG_DEF(_TSK_ID_)

#include "os_msg_reg.def"

#else

#define BEGIN_MSG_DEF(_TSK_ID_) \
               extern INT8U const _TSK_ID_;
               
#define MSG_DEF(_TSK_ID_, _MSG_ID_, _PROC_)
                  
#define END_MSG_DEF(_TSK_ID_)

#include "os_msg_reg.def"
#endif
#endif  /* #if EN_FREECONST == 0 */

/*
*********************************************************************************
*                               DEFINE TASKS PRIORITY
*********************************************************************************
*/
#define PRIO_COMMONTASK      TSK_ID_COMMON
#define PRIO_OPTTASK         TSK_ID_OPT

#define MAX_TASK_NUM         TSK_ID_MAX

/*
*********************************************************************************
* 定义统一编号的消息ID
*********************************************************************************
*/

#ifdef BEGIN_MSG_DEF
#undef BEGIN_MSG_DEF
#endif

#ifdef MSG_DEF
#undef MSG_DEF
#endif

#ifdef END_MSG_DEF
#undef END_MSG_DEF
#endif


#define BEGIN_MSG_DEF(_TSK_ID_)
#if EN_FREECONST > 0
#define MSG_DEF(_TSK_ID_, _MSG_ID_, _PROC_) \
                    _MSG_ID_,
#else
#define MSG_DEF(_TSK_ID_, _MSG_ID_, _PROC_) \
                    _MSG_ID_##_ENUM,
#endif
#define END_MSG_DEF(_TSK_ID_)

typedef enum {
    MSG_ID_NULL,
    #include "os_msg_reg.def"
    MSG_ID_MAX
} OS_MSG_ID_E;

/* 定义统一编号 */
#ifdef BEGIN_MSG_DEF
#undef BEGIN_MSG_DEF
#endif

#ifdef MSG_DEF
#undef MSG_DEF
#endif

#ifdef END_MSG_DEF
#undef END_MSG_DEF
#endif

#if EN_FREECONST == 0
#ifdef GLOBALS_OS_REG
#define BEGIN_MSG_DEF(_TSK_ID_)
#define MSG_DEF(_TSK_ID_, _MSG_ID_, _PROC_) \
               INT8U const _MSG_ID_ = _MSG_ID_##_ENUM;
                  
#define END_MSG_DEF(_TSK_ID_)

#include "os_msg_reg.def"
#else

#define BEGIN_MSG_DEF(_TSK_ID_)
#define MSG_DEF(_TSK_ID_, _MSG_ID_, _PROC_) \
               extern INT8U const _MSG_ID_;
                  
#define END_MSG_DEF(_TSK_ID_)

#include "os_msg_reg.def"
#endif
#endif /* #if EN_FREECONST == 0 */

/********************************************************************************
** 函数名:     OS_GetRegTskInfo
** 函数描述:   获取对应任务的注册信息
** 参数:       [in] tskid:统一编号的任务号
** 返回:       成功返回注册表指针，失败返回0
********************************************************************************/
OS_TSK_TBL_T *OS_GetRegTskInfo(INT16U tskid);

/********************************************************************************
*   函数名:    OS_GetRegTskMax
*   功能描述:  获取已注册的任务个数
*　 参数:      无
*   返回值:    注册的个数
********************************************************************************/
INT8U OS_GetRegTskMax(void);

//-------------------------------------------------------------------------------
#endif /* OS_REG_H */




