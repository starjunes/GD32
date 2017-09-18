/******************************************************************************
**
** Filename:     os_reg.c
** Copyright:    
** Description:  该模块主要实现任务和消息的注册信息表管理
**
-------------------------------------------------------------------------------
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#define  GLOBALS_OS_REG   1
#include "yx_include.h"
#include "os_include.h"
#include "os_reg.h"



/*
****************************************************************
*   定义各个函数申明
****************************************************************
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

#include "os_tsk_cfg.reg"

/* 消息处理函数声明 */
#ifdef BEGIN_MSG_CFG
#undef BEGIN_MSG_CFG
#endif

#ifdef MSG_CFG
#undef MSG_CFG
#endif

#ifdef END_MSG_CFG
#undef END_MSG_CFG
#endif

#define BEGIN_MSG_CFG(_TSK_ID_)

#define MSG_CFG(_TSK_ID_, _MSG_ID_, _PROC_)                    \
                   void _PROC_(INT16U tskid, INT16U msgid, INT32U lpara, INT32U hpara);
                   
#define END_MSG_CFG(_TSK_ID_)

#include "os_msg_cfg.reg"


/*
****************************************************************
*   定义各个任务的消息的注册表
****************************************************************
*/

#ifdef BEGIN_MSG_CFG
#undef BEGIN_MSG_CFG
#endif

#ifdef MSG_CFG
#undef MSG_CFG
#endif

#ifdef END_MSG_CFG
#undef END_MSG_CFG
#endif

#define BEGIN_MSG_CFG(_TSK_ID_)                         \
                   static const OS_MSG_TBL_T _TSK_ID_##MSG_TBL[] = {
                    
#define MSG_CFG(_TSK_ID_, _MSG_ID_, _PROC_)             \
                   {_TSK_ID_, _MSG_ID_, _PROC_},
                 
#define END_MSG_CFG(_TSK_ID_)                           \
                   {0}};

#include "os_msg_cfg.reg"

/*
****************************************************************
*   定义任务的初始化函数的注册表
****************************************************************
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

#define BEGIN_TSK_DEF(_TSK_ID_)         \
                   static const OS_INIT_TBL_T _TSK_ID_##INIT_TBL[] = {
                    
#define TSK_INIT_DEF(_TSK_ID_, _INIT_)        \
                   {_INIT_},
                 
#define END_TSK_DEF(_TSK_ID_)           \
                   {0}};

#include "os_tsk_cfg.reg"

/*
****************************************************************
*   定义任务的注册表
****************************************************************
*/
#ifdef BEGIN_MSG_CFG
#undef BEGIN_MSG_CFG
#endif

#ifdef MSG_CFG
#undef MSG_CFG
#endif

#ifdef END_MSG_CFG
#undef END_MSG_CFG
#endif


#define BEGIN_MSG_CFG(_TSK_ID_)                                             \
                   {_TSK_ID_,                                               \
                   (OS_MSG_TBL_T *)_TSK_ID_##MSG_TBL,                       \
                   sizeof(_TSK_ID_##MSG_TBL) / sizeof(OS_MSG_TBL_T) - 1,    \
                   (OS_INIT_TBL_T *)_TSK_ID_##INIT_TBL,                     \
                   sizeof(_TSK_ID_##INIT_TBL) / sizeof(OS_INIT_TBL_T) - 1   \
                   },
                   
#define MSG_CFG(_TSK_ID_, _MSG_ID_, _PROC_)
#define END_MSG_CFG(_TSK_ID_)

static const OS_TSK_TBL_T s_tsk_tbl[] = {
    #include "os_msg_cfg.reg"
    {0}
};



/*******************************************************************
** 函数名:     OS_GetRegTskInfo
** 函数描述:   获取对应任务的注册信息
** 参数:       [in] tskid:任务编号
** 返回:       成功返回注册表指针，失败返回0
********************************************************************/
OS_TSK_TBL_T *OS_GetRegTskInfo(INT16U tskid)
{
    if (tskid >= OS_TSK_ID_MAX) {
        return 0;
    }

    return (OS_TSK_TBL_T *)(&s_tsk_tbl[tskid]);
}

/*******************************************************************
*   函数名:    OS_GetRegTskMax
*   功能描述:  获取已注册的任务个数
*　 参数:      无
*   返回值:    注册的个数
********************************************************************/
INT8U OS_GetRegTskMax(void)
{
    return OS_TSK_ID_MAX;
}




