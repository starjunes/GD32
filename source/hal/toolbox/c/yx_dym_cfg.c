/******************************************************************************
**
** Filename:     yx_dym_cfg.c
** Copyright:    
** Description:  该模块主要实现动态内存注册信息管理
**
-------------------------------------------------------------------------------
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#include "hal_include.h"
#include "yx_list.h"
#include "yx_dym_cfg.h"


/*
********************************************************************************
* 定义动态内存链表
********************************************************************************
*/
#ifdef DYM_CFG
#undef DYM_CFG
#endif


#define DYM_CFG(_DYM_TYPE_, _NUM_, _SIZE_, _LABEL_) \
               static LIST_T _DYM_TYPE_##FREELIST; \
               static LIST_T _DYM_TYPE_##USEDLIST;

#include "yx_dym_cfg.reg"

/*
********************************************************************************
* 定义动态内存缓存
********************************************************************************
*/
#ifdef DYM_CFG
#undef DYM_CFG
#endif


#define DYM_CFG(_DYM_TYPE_, _NUM_, _SIZE_, _LABEL_) \
               static struct {                    \
                   NODE_T      reserve;             \
                   DYM_HEAD_T head;                \
                   INT32U    buf[(_SIZE_ + 3)/4]; \
                   INT8U     endflag;             \
               } _DYM_TYPE_##BUF[_NUM_];

#include "yx_dym_cfg.reg"
/*
********************************************************************************
* 定义动态内存注册表
********************************************************************************
*/           
#ifdef DYM_CFG
#undef DYM_CFG
#endif

#define DYM_CFG(_DYM_TYPE_, _NUM_, _SIZE_, _LABEL_) \
                {_DYM_TYPE_,                       \
                 sizeof(_DYM_TYPE_##BUF[0].buf),   \
                 sizeof(_DYM_TYPE_##BUF[0]),       \
                 _NUM_,                           \
                 (INT8U *)_DYM_TYPE_##BUF,         \
                 &(_DYM_TYPE_##FREELIST),          \
                 &(_DYM_TYPE_##USEDLIST),          \
                 _LABEL_},


static const DYM_POOL_T s_cfg_tbl[] = {
    #include "yx_dym_cfg.reg"
    {0}
};


/*-------------------------------------------------------------------
** 函数名:     YX_DYM_GetRegPoolInfo
** 函数描述:   获取内存池注册信息
** 参数:       [in] type: 内存池类型
** 返回:       注册信息表指针
-------------------------------------------------------------------*/
DYM_POOL_T const *YX_DYM_GetRegPoolInfo(INT8U type)
{
    return &s_cfg_tbl[type];
}

/*-------------------------------------------------------------------
** 函数名:     YX_DYM_GetRegPoolNum
** 函数描述:   内存池注册信息个数
** 参数:       无
** 返回:       注册信息个数
-------------------------------------------------------------------*/
INT8U YX_DYM_GetRegPoolNum(void)
{
   return DYM_TYPE_MAX;
}
