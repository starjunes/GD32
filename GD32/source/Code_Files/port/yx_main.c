/********************************************************************************
**
** 文件名:     yx_main.c
** 版权所有:   (c) 1998-2017 厦门雅迅网络股份有限公司
** 文件描述:   上层主应用程序的初始化入口
**
*********************************************************************************
**                  修改历史记录
**===============================================================================
**| 日期              | 作者        |  修改记录
**===============================================================================
**| 2017/05/15 | 黄运峰    |  创建该文件
********************************************************************************/
#include "yx_includes.h"
#include "port_msghdl.h"
#include "port_gpio.h"
//#include "hal_sysctl.h"
#include "build_file.h"
#include "bal_gpio_cfg.h"
/*
*********************************************************************************
*   定义配置参数
*********************************************************************************
*/
#define TBOX_APP_VERSION  0x00001000       // app当前匹配的平台版本

/********************************************************************************
** 函数名:     Tsk_Init
** 函数描述:   初始化函数
** 参数:       无
** 返回:       无
********************************************************************************/
void Tsk_Init(void)
{
    INT8U i, j, ntsk, ninit;
    OS_TSK_TBL_T *ptsk;
    OS_INIT_TBL_T *pinit;
        
    ntsk = OS_GetRegTskMax();
    for (i = 0; i < ntsk; i++) {
        ptsk = OS_GetRegTskInfo(i);
        pinit = ptsk->pinit;
        ninit = ptsk->ninit;
        
        for (j = 0; j < ninit; j++) {
            if (pinit->init != 0) {
                pinit->init();
            }
            pinit++;
            bal_ClearWatchdog();
        }
    }
}

/********************************************************************************
** 函数名:     YX_Main
** 函数描述:   上层主应用程序的初始化入口
** 参数:       无
** 返回:       无
********************************************************************************/
void YX_Main(void)
{
	char *psn_str ;

    PORT_GpioInit();
    YX_PortLayerInit();                                     // 平台相关port初始化接口
	OS_InitDiag();											// 诊断模块初始化
	//OS_InitTmr();											// 定时器初始化
	//OS_InitMsgSched();										// 消息队列初始化
    Tsk_Init();
	mmi_resetcnt_init();
    debug_printf_dir("\r\n <**** MCU版本号:%s ****>\r\n",YX_GetVersion());
    debug_printf_dir("\r\n <**** 编译日期:%s, %s ****>\r\n",YX_GetVersionDate(), YX_GetVersionTime());
    #if EN_DEBUG > 0
    //debug_printf_dir("\r\n< (1)编译日期:%s,%s >\r\n",BUILD_DATE,BUILD_TIME);
    //debug_printf_dir("\r\n< (1)编译日期:%s,%s >\r\n",YX_GetVersionDate(),YX_GetVersionTime());
    //debug_printf_dir("\r\n <复位原因:0x%x> \r\n",PORT_GetResetReason());
    #endif 
    
}

//------------------------------------------------------------------------------
/* End of File */

