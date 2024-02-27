/********************************************************************************
**
** 文件名:     os_reg.c
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
#define  GLOBALS_OS_REG   1
#include "yx_includes.h"
#include "os_reg.h"
/*
*********************************************************************************
*   定义各个函数声明
*********************************************************************************
*/
/* 初始化函数声明 */
#ifdef BEGIN_TSK_DEF
#undef BEGIN_TSK_DEF
#endif

#ifdef TSK_INIT_DEF
#undef TSK_INIT_DEF
#endif

#ifdef END_TSK_DEF
#undef END_TSK_DEF
#endif

#define BEGIN_TSK_DEF(_TSK_ID_)
#define TSK_INIT_DEF(_TSK_ID_, _INIT_)   \
                      void _INIT_(void);
                      
#define END_TSK_DEF(_TSK_ID_)

#include "os_tsk_reg.def"

/* 消息处理函数声明 */
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

#define MSG_DEF(_TSK_ID_, _MSG_ID_, _PROC_)                    \
                   void _PROC_(INT16U tskid, INT16U msgid, INT32U lpara, INT32U hpara);
                   
#define END_MSG_DEF(_TSK_ID_)

#include "os_msg_reg.def"

/*
*********************************************************************************
*   定义各个任务的消息的注册表
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


#define BEGIN_MSG_DEF(_TSK_ID_)    \
                   _TSK_ID_##BEGIN,
                    
#define MSG_DEF(_TSK_ID_, _MSG_ID_, _PROC_) \
                   _MSG_ID_##DEF,
                   
#define END_MSG_DEF(_TSK_ID_)     \
                   _TSK_ID_##END,

typedef enum {
    #include "os_msg_reg.def"
     OS_DEF1_ID_MAX
} OS_DEF1_ID_E;

#ifdef BEGIN_MSG_DEF
#undef BEGIN_MSG_DEF
#endif

#ifdef MSG_DEF
#undef MSG_DEF
#endif

#ifdef END_MSG_DEF
#undef END_MSG_DEF
#endif

#if EN_FREECONST > 0  /* EN_FREECONST > 0 */
#define BEGIN_MSG_DEF(_TSK_ID_)                         \
                   static const OS_MSG_TBL_T _TSK_ID_##MSG_TBL[] = {
                    
#define MSG_DEF(_TSK_ID_, _MSG_ID_, _PROC_)             \
                   {_TSK_ID_, _MSG_ID_, _PROC_},
                 
#define END_MSG_DEF(_TSK_ID_)                           \
                   {0}};

#include "os_msg_reg.def"

#else /* EN_FREECONST == 0 */

#define BEGIN_MSG_DEF(_TSK_ID_)                          \
                   static OS_MSG_TBL_T _TSK_ID_##MSG_TBL[_TSK_ID_##END - _TSK_ID_##BEGIN];
                   
#define MSG_DEF(_TSK_ID_, _MSG_ID_, _PROC_)
#define END_MSG_DEF(_TSK_ID_)

#include "os_msg_reg.def"
#endif /* end EN_FREECONST */

/*
*********************************************************************************
*   定义任务的初始化函数的注册表
*********************************************************************************
*/
#ifdef BEGIN_TSK_DEF
#undef BEGIN_TSK_DEF
#endif

#ifdef TSK_INIT_DEF
#undef TSK_INIT_DEF
#endif

#ifdef END_TSK_DEF
#undef END_TSK_DEF
#endif


#define BEGIN_TSK_DEF(_TSK_ID_)    \
                   _TSK_ID_##BG,
                    
#define TSK_INIT_DEF(_TSK_ID_, _INIT_)        \
                   _INIT_##DEF,
                   
#define END_TSK_DEF(_TSK_ID_)     \
                   _TSK_ID_##ED,

typedef enum {
    #include "os_tsk_reg.def"
     OS_DEF2_ID_MAX
} OS_DEF2_ID_E;


#ifdef BEGIN_TSK_DEF
#undef BEGIN_TSK_DEF
#endif

#ifdef TSK_INIT_DEF
#undef TSK_INIT_DEF
#endif

#ifdef END_TSK_DEF
#undef END_TSK_DEF
#endif

#if EN_FREECONST > 0  /* EN_FREECONST > 0 */
#define BEGIN_TSK_DEF(_TSK_ID_)         \
                   static const OS_INIT_TBL_T _TSK_ID_##INIT_TBL[] = {
                    
#define TSK_INIT_DEF(_TSK_ID_, _INIT_)        \
                   {_INIT_},
                 
#define END_TSK_DEF(_TSK_ID_)           \
                   {0}};

#include "os_tsk_reg.def"

#else /* EN_FREECONST == 0 */

#define BEGIN_TSK_DEF(_TSK_ID_)                          \
                   static OS_INIT_TBL_T _TSK_ID_##INIT_TBL[_TSK_ID_##ED - _TSK_ID_##BG];
                   
#define TSK_INIT_DEF(_TSK_ID_, _INIT_)
#define END_TSK_DEF(_TSK_ID_)

#include "os_tsk_reg.def"

#endif /* end EN_FREECONST */

/*
*********************************************************************************
*   定义任务的注册表
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

#if EN_FREECONST > 0  /* EN_FREECONST > 0 */
#define BEGIN_MSG_DEF(_TSK_ID_)                                             \
                   {_TSK_ID_,                                               \
                   (OS_MSG_TBL_T *)_TSK_ID_##MSG_TBL,                       \
                   sizeof(_TSK_ID_##MSG_TBL) / sizeof(OS_MSG_TBL_T) - 1,    \
                   (OS_INIT_TBL_T *)_TSK_ID_##INIT_TBL,                     \
                   sizeof(_TSK_ID_##INIT_TBL) / sizeof(OS_INIT_TBL_T) - 1   \
                   },
#define MSG_DEF(_TSK_ID_, _MSG_ID_, _PROC_)
#define END_MSG_DEF(_TSK_ID_)

static const OS_TSK_TBL_T s_tsk_tbl[] = {
    #include "os_msg_reg.def"
    {0}
};
#else /* EN_FREECONST == 0 */

static OS_TSK_TBL_T s_tsk_tbl[TSK_ID_MAX];

#endif /* end EN_FREECONST */


/*
*********************************************************************************
*   手动初始化
*********************************************************************************
*/
#if EN_FREECONST == 0
DECLARE_GPS_FUN_INIT_CONSTVAR(GPS_OS_REG_STATICDAT_ID)
{
    INT32U i;
    
    /* 初始化消息列表 */
#ifdef BEGIN_MSG_DEF
#undef BEGIN_MSG_DEF
#endif

#ifdef MSG_DEF
#undef MSG_DEF
#endif

#ifdef END_MSG_DEF
#undef END_MSG_DEF
#endif

#define BEGIN_MSG_DEF(_TSK_ID_)                                   \
                   i = 0;
                   
#define MSG_DEF(_TSK_ID_, _MSG_ID_, _PROC_)                       \
                   _TSK_ID_##MSG_TBL[i].tskid = _TSK_ID_;         \
                   _TSK_ID_##MSG_TBL[i].msgid = _MSG_ID_;         \
                   _TSK_ID_##MSG_TBL[i].proc = _PROC_;            \
                   i++;
                   
                 
#define END_MSG_DEF(_TSK_ID_)

#include "os_msg_reg.def"

    /* 初始化各任务的初始化函数列表 */
#ifdef BEGIN_TSK_DEF
#undef BEGIN_TSK_DEF
#endif

#ifdef TSK_INIT_DEF
#undef TSK_INIT_DEF
#endif

#ifdef END_TSK_DEF
#undef END_TSK_DEF
#endif

#define BEGIN_TSK_DEF(_TSK_ID_)    \
                   i = 0;
                   
#define TSK_INIT_DEF(_TSK_ID_, _INIT_)   \
                   _TSK_ID_##INIT_TBL[i].init = _INIT_; \
                   i++;
                   
#define END_TSK_DEF(_TSK_ID_)

#include "os_tsk_reg.def"

 /* 初始化任务列表 */
#ifdef BEGIN_MSG_DEF
#undef BEGIN_MSG_DEF
#endif

#ifdef MSG_DEF
#undef MSG_DEF
#endif

#ifdef END_MSG_DEF
#undef END_MSG_DEF
#endif

#define BEGIN_MSG_DEF(_TSK_ID_)                                                                      \
                  s_tsk_tbl[_TSK_ID_].tskid = _TSK_ID_;                                              \
                  s_tsk_tbl[_TSK_ID_].pmsg = (OS_MSG_TBL_T *)_TSK_ID_##MSG_TBL;                      \
                  s_tsk_tbl[_TSK_ID_].nmsg = sizeof(_TSK_ID_##MSG_TBL) / sizeof(OS_MSG_TBL_T) - 1;   \
                  s_tsk_tbl[_TSK_ID_].pinit = (OS_INIT_TBL_T *)_TSK_ID_##INIT_TBL;                   \
                  s_tsk_tbl[_TSK_ID_].ninit = sizeof(_TSK_ID_##INIT_TBL) / sizeof(OS_INIT_TBL_T) - 1;
                  
#define MSG_DEF(_TSK_ID_, _MSG_ID_, _PROC_)
#define END_MSG_DEF(_TSK_ID_)

#include "os_msg_reg.def"
}
#endif /* end EN_FREECONST == 0 */



/********************************************************************************
** 函数名:     OS_GetRegTskInfo
** 函数描述:   获取对应任务的注册信息
** 参数:       [in] tskid:统一编号的任务号
** 返回:       成功返回注册表指针，失败返回0
********************************************************************************/
OS_TSK_TBL_T *OS_GetRegTskInfo(INT16U tskid)
{
    if (tskid >= TSK_ID_MAX) {
        return 0;
    }

    return (OS_TSK_TBL_T *)(&s_tsk_tbl[tskid]);
}

/********************************************************************************
*   函数名:    OS_GetRegTskMax
*   功能描述:  获取已注册的任务个数
*　 参数:      无
*   返回值:    注册的个数
********************************************************************************/
INT8U OS_GetRegTskMax(void)
{
    return TSK_ID_MAX;
}

//------------------------------------------------------------------------------
/* End of File */



