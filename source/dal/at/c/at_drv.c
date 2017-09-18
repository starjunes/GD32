/********************************************************************************
**
** 文件名:     at_drv.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现手机模块驱动管理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2014/06/15 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "yx_include.h"
#include "yx_misc.h"
#include "dal_input_drv.h"
#include "at_drv.h"
#include "yx_sm_cmd.h"

#if EN_AT > 0

/*
********************************************************************************
* 定义模块配置参数
********************************************************************************
*/


/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/


/*******************************************************************
** 函数名:     Callback_RecvSmIndex
** 函数描述:   接收到短信
** 参数:       [in] sm: 短信内容
** 返回:       无
********************************************************************/
static void Callback_RecvSmIndex(INT16U index)
{
    AT_Q_ReadSmByIndex(index, 0);
}

/*******************************************************************
** 函数名:     Callback_RecvsSm
** 函数描述:   接收到短信
** 参数:       [in] sm: 短信内容
** 返回:       无
********************************************************************/
static void Callback_RecvsSm(SM_T *sm)
{
    #if DEBUG_AT > 0
    printf_com("<receive short message>\r\n");
    printf_com("time is: ");
    printf_hex(&sm->date.year, 6);
    printf_com("\r\n");
    printf_com("sms  is(%d): ", sm->scalen);
    printf_raw(sm->sca, sm->scalen);
    printf_com("\r\n");
    printf_com("tel  is(%d): ", sm->oalen);
    printf_raw(sm->oa, sm->oalen);
    printf_com("\r\n"); 
    
    printf_com("data is(%d): ", sm->udlen);
    printf_raw(sm->ud, sm->udlen);
    printf_com(">\r\n");
    printf_com("hex is(%d): ", sm->udlen);
    printf_hex(sm->ud, sm->udlen);
    printf_com(">\r\n");
    #endif
    
    YX_DetectSmsCmd(sm);
}

/*******************************************************************
** 函数名:     AT_DRV_Open
** 函数描述:   打开驱动功能
** 参数:       无
** 返回:       无
********************************************************************/
void AT_DRV_Open(void)
{
    AT_COM_PullUp();                                                           /* 打开电源 */
    AT_SEND_Open();                                                            /* 打开发送驱动 */
    AT_RECV_Open();                                                            /* 打开数据接收解析功能 */
    AT_CORE_Open();                                                            /* 打开模块初始化 */
}

/*******************************************************************
** 函数名:     AT_DRV_Close
** 函数描述:   关闭驱动功能
** 参数:       无
** 返回:       无
********************************************************************/
void AT_DRV_Close(void)
{
    AT_Q_ClearPhoneQueue();                                                    /* 清除电话队列 */
    AT_Q_ClearTcpipQueue();                                                    /* 清除TCPIP队列 */
    AT_Q_ClearSmsQueue();                                                      /* 清除短信队列 */
    AT_Q_ClearSetQueue();                                                      /* 清除设置指令队列 */
    
    AT_GPRS_ClosePdpContext();                                                 /* 关闭GPRS上下文 */
    AT_SOCKET_CloseAllSocket();                                                /* 关闭所有socket */
    
    AT_COM_PullDown();                                                         /* 关闭电源 */
    AT_SEND_Close();                                                           /* 关闭发送驱动 */
    AT_RECV_Close();                                                           /* 关闭数据接收解析 */
    AT_CORE_Close();                                                           /* 关闭模块初始化 */
}

/*******************************************************************
** 函数名:     AT_DRV_Init
** 函数描述:   初始化函数
** 参数:       无
** 返回:       无
********************************************************************/
void AT_DRV_Init(void)
{
    AT_SMS_CALLBACK_T callback;
    
    AT_POWER_Init();
    AT_COM_Init();
    AT_SEND_Init();
    AT_RECV_Init();
    
    /* 初始化队列 */
#if EN_AT_PHONE > 0
    AT_Q_InitPhone();
#endif
    AT_Q_InitSet();
    AT_Q_InitSms();
    AT_Q_InitTcpip();
    
    /* 初始化数据解析 */
    AT_URC_InitCommon();
    AT_URC_InitPhone();
    AT_URC_InitSms();
    AT_URC_InitTcpip();
    
    AT_CORE_Init();
    //AT_OTHER_Init();
    AT_GPRS_InitDrv();
    AT_SOCKET_InitDrv();
    
    callback.callback_recvsmindex = Callback_RecvSmIndex;
    callback.callback_recvsm = Callback_RecvsSm;
    AT_URC_RegistSmsHandler(&callback);
}

#endif

