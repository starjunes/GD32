/********************************************************************************
**
** 文件名:     port_msghdl.c
** 版权所有:   (c) 1998-2017 厦门雅迅网络股份有限公司
** 文件描述:   实现上层应用事件/消息处理入口
**
*********************************************************************************
**                  修改历史记录
**===============================================================================
**| 日期              | 作者        |  修改记录
**===============================================================================
**| 2017/05/12 | 黄运峰    |  创建该文件
********************************************************************************/
#include "yx_includes.h"
#include "port_plat.h"
#include "port_uart.h"
#include "port_gsensor.h"
#include "port_msghdl.h"
#include "port_tsk.h"

/*******************************************************************
** 函数名:     HdlSysMsgEntry
** 函数描述:   上层应用事件消息处理
** 参数:       [in]  tskid:   暂时无用,对应底层任务号
**             [in]  msgid:   消息ID
**             [in]  msg:     消息参数
** 返回:       处理成功返回TRUE, 处理失败返回FALSE 
********************************************************************/
static BOOLEAN HdlSysMsgEntry(INT32U tskid, INT32U msgid, void *msg)
{
		return false;
#if 0
    BOOLEAN ret;
    
    tskid = tskid;
    ret = TRUE;
    switch (msgid) {
        case SHELL_MSG_GSENSOR_EVENT:
            ret = PORT_Hdl_SHELL_MSG_GSENSOR_EVENT(msg);
            break;
        case SHELL_MSG_MCU_WAKEUP:
            ret = PORT_Hdl_SHELL_MSG_WAKE_EVENT(msg);
            break;
        default:
            ret = FALSE;
            break;
    }
    return ret;
#endif
}
/********************************************************************************
**  函数名:     YX_PortLayerInit
**  函数描述:   初始化Port层，初始化常数型变量，安装系统消息处理入口
**  参数:       无
**  返回:       无
********************************************************************************/
void YX_PortLayerInit(void)
{
#if 0
    PORT_InitConstVar();
    PORT_InstallEventHdl(HdlSysMsgEntry);                               // 最先注册消息
#endif
}

//------------------------------------------------------------------------------
/* End of File */

