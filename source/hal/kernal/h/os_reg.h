/******************************************************************************
**
** Filename:     os_reg.h
** Copyright:    
** Description:  该模块主要实现任务和消息的注册信息表管理
**
-------------------------------------------------------------------------------
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#ifndef H_OS_REG
#define H_OS_REG             1


/*
********************************************************************************
* define struct
********************************************************************************
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
********************************************************************************
* 定义统一编号的任务ID
********************************************************************************
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


#define BEGIN_MSG_CFG(_TSK_ID_)    \
                   _TSK_ID_,
                    

#define MSG_CFG(_TSK_ID_, _MSG_ID_, _PROC_)
#define END_MSG_CFG(_TSK_ID_)

typedef enum {
    #include "os_msg_cfg.reg"
    OS_TSK_ID_MAX
} OS_TSK_ID_E;


/*
********************************************************************************
* 定义统一编号的消息ID
********************************************************************************
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


#define BEGIN_MSG_CFG(_TSK_ID_)
                    
#define MSG_CFG(_TSK_ID_, _MSG_ID_, _PROC_) \
                    _MSG_ID_,
                    
#define END_MSG_CFG(_TSK_ID_)

typedef enum {
    MSG_ID_NULL,
    #include "os_msg_cfg.reg"
    MSG_ID_MAX
} OS_MSG_ID_E;

/*******************************************************************
** 函数名:     OS_GetRegTskInfo
** 函数描述:   获取对应任务的注册信息
** 参数:       [in] tskid:统一编号的任务号
** 返回:       成功返回注册表指针，失败返回0
********************************************************************/
OS_TSK_TBL_T *OS_GetRegTskInfo(INT16U tskid);

/*******************************************************************
*   函数名:    OS_GetRegTskMax
*   功能描述:  获取已注册的任务个数
*　 参数:      无
*   返回值:    注册的个数
********************************************************************/
INT8U OS_GetRegTskMax(void);


#endif




