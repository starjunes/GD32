/********************************************************************************
**
** 文件名:     port_tsk.c
** 版权所有:   (c) 1998-2017 厦门雅迅网络股份有限公司
** 文件描述:   实现应用层与平台层事件/消息发送及处理接口安装
**
*********************************************************************************
**                  修改历史记录
**===============================================================================
**| 日期              | 作者        |  修改记录
**===============================================================================
**| 2017/05/15 | 黄运峰    |  创建该文件
********************************************************************************/
#include "dal_include.h"
//#include "hal_timer.h"
#include "port_tsk.h"
//#include "dal_maintsk.h"
//#include "hal_maintsk.h"

/********************************************************************************
**  函数名:     PORT_InstallEventHdl
**  函数描述:   安装事件处理函数
**  参数:       [in] func:事件处理函数指针
** 返回:        无
********************************************************************************/
void PORT_InstallEventHdl(INT8U (*func)(INT32U tskid, INT32U msgid, void *msg))
{
#if 0
    DAL_InstallEventHdl(func);
#endif
}

/********************************************************************************
** 函数名:     PORT_PostSysMsg
** 函数描述:   往平台发送平台系统消息
** 参数:       [in] msgid:消息ID,见SHELL_MSG_E
**             [in] para: 消息参数,传递给SHELL_MSG_T->para
** 返回:       成功返回TRUE，失败返回FALSE
********************************************************************************/
BOOLEAN PORT_PostSysMsg(INT32U msgid, void *para)
{
		return false;
#if 0
    OS_QMSG_T msg;

    msg.msg_id = MSG_MAIN_APPEVNT;
    msg.mdata1 = msgid;
    msg.mdata2 = (INT32U)para;

    if (OS_Q_Post(g_maintsk_pq, &msg, 0) == OS_RET_TRUE) {
        return TRUE;
    } else {
        return FALSE;
    }
#endif
}

/********************************************************************************
** 函数名:     PORT_InstallCommonHdl
** 函数描述:   向平台安装MSG_MAIN_COMMON消息处理接口
** 参数:       [in] handler：消息回调函数
** 返回:       无
********************************************************************************/
void PORT_InstallCommonHdl(void (*handler)(void))
{
#if 0
    DAL_InstallCommonHdl(handler);
#endif
}

/********************************************************************************
** 函数名:     PORT_PostCommonMsg
** 函数描述:   往平台发送MSG_MAIN_COMMON消息,平台将回调处理
** 参数:       无
** 返回:       成功返回true，失败返回false
********************************************************************************/
BOOLEAN PORT_PostCommonMsg(void)
{
		return false;
#if 0
    OS_QMSG_T msg;

    msg.msg_id = MSG_MAIN_COMMON;
    msg.mdata1 = 0;
    msg.mdata2 = 0;

    if (OS_Q_Post(g_maintsk_pq, &msg, 0) == OS_RET_TRUE) {
        return TRUE;
    } else {
        return FALSE;
    }
#endif
}

//------------------------------------------------------------------------------
/* End of File */
