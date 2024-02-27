/**************************************************************************************************
**                                                                                               **
**  文件名称:  main.c                                                                            **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2010                                    **
**  创建信息:  LEON -- 2010年12月2日                                                             **
**  文件描述:  主函数                                                                            **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/
#include  "app_include.h"
#include  "dal_timer.h"
#include  "dal_hard.h"
#include  "man_irq.h"
#include  "appmain.h"
#include  "man_queue.h"
#include  "man_timer.h"
#include  "scanner.h"
#include  "dmemory.h"
#include  "dal_usart.h"
#include  "debug_print.h"
#include  "ramimage.h"
//#include  "usbh_core.h"
//#include  "usbh_usr.h"
//#include  "usbh_msc_core.h"
#include "debug_print.h"
#include "dal_buzzer.h"
#include "yx_main.h"
#include "port_uart.h"
#include "port_plat.h"
#include "cm_backtrace.h"
#include "yx_version.h"

#if EN_DEBUG > 0
static INT8U const s_temphexbuf[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x01,0x02,0x03,0x04
    ,0x05,0x06,0x07,0x08,0x09,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x01,0x02,0x03,0x04,0x05
    ,0x06,0x07,0x08,0x09,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x01,0x02,0x03,0x04,0x05,0x06
    ,0x07,0x08,0x09,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x01,0x02,0x03,0x04,0x05,0x06,0x07
    ,0x08,0x09,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08
    ,0x09,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,};
#endif

static void KernelStart(void)
{
    ClearWatchdog();
    InitTimerMan();               /* initial timer management              */
    InitMsgQman();                /* initial queue management              */
    InitDmemory();                /* initialize the dyamic memory module   */
    InitScanner();                /* initial scan module                   */
}

#if 1
static void Delay_ms(INT16U time)
{
    INT32U  delaycnt;

    delaycnt = 0xA000 * time;                                          /* 约150ms */
    while (delaycnt--) {
    }
}
#endif

int main(void)
{
    #if DEBUG_RAMDATA > 0
    INT8U ramdata[ram_para_MAX];                               /* 用于调试RAM存储数据 */
    #endif
    #if EN_DEBUG > 0
    //RCC_ClocksTypeDef RCC_Clocks;
    #endif
	System_Initiate();
	KernelStart();
	//PORT_InitUart(USART_DEBUG, 115200, 1000 , 1000);
	cm_backtrace_init("ae64m4", "dy-fw",APPDAL_VERSION_STR);
	#if EN_DEBUG > 0
    /* 调试串口初始化不可置于后面，不然要是打开调试开关，其他部分有打印，就会出错 */
    //Debug_Initiate();
    PORT_InitUart(USART_DEBUG, 115200, 1000 , 1000);
    Debug_SysPrint("System Start...\r\n");              /* 打印系统启动信息 */
    Debug_PrintHex(TRUE, (INT8U*)s_temphexbuf, sizeof(s_temphexbuf));/* 测试打印hex */
    
    Debug_SysPrint("<--app start SystemClock:%d PCLK1:%d-->\r\n", rcu_clock_freq_get(CK_SYS), rcu_clock_freq_get(CK_APB1));
    #endif

    #if 1
    ClearWatchdog();
    Delay_ms(150); /* 用于在关闭调试时加入串口初始化与打印输出占用的时间延时,上飞鸿锁车问题修改点 */
    ClearWatchdog();
    #endif

    ClearWatchdog();
    AppStart();
    YX_Main();
	
    #if DEBUG_RAMDATA > 0
    ReadRamImage(ramdata, ram_para_MAX);
    Debug_SysPrint("prvdata0 = %d\r\n", ramdata[0]);
    Debug_SysPrint("prvdata1 = %d\r\n", ramdata[1]);
    Debug_SysPrint("prvdata2 = %d\r\n", ramdata[2]);
    Debug_SysPrint("prvdata3 = %d\r\n", ramdata[3]);
    Debug_SysPrint("prvdata4 = %d\r\n", ramdata[4]);
    Debug_SysPrint("prvdata5 = %d\r\n", ramdata[5]);
    Debug_SysPrint("prvdata6 = %d\r\n", ramdata[6]);
    Debug_SysPrint("prvdata7 = %d\r\n", ramdata[7]);
    #endif

    if(!RamImageValid()){           // 判参数区数据是否是第一次上电随机数 
	  ClearRamImage();             // 随机数，对参数区清零 
    }

	#if DEBUG_RAMDATA > 0
    ReadRamImage(ramdata, ram_para_MAX);
    Debug_SysPrint("data0 = %d\r\n", ramdata[0]);
    Debug_SysPrint("data1 = %d\r\n", ramdata[1]);
    Debug_SysPrint("data2 = %d\r\n", ramdata[2]);
    Debug_SysPrint("data3 = %d\r\n", ramdata[3]);
    Debug_SysPrint("data4 = %d\r\n", ramdata[4]);
    Debug_SysPrint("data5 = %d\r\n", ramdata[5]);
    Debug_SysPrint("data6 = %d\r\n", ramdata[6]);
    Debug_SysPrint("data7 = %d\r\n", ramdata[7]);
	#endif
	
    while (1) {
      #if EN_USB > 0
        USBH_Process(&USB_OTG_Core, &USB_Host);
      #endif
	  	ClearWatchdog();           /* clear watchdog                        */
        SysTimerEntryProc();       /* timer management entry                */
        QMsgDispatch();            /* message dispatch                      */
        ScannerRunning();          /* scan some program entry               */
    }
}

/**************************** (C) COPYRIGHT 2010 XIAMEN YAXON.LTD ******************************END OF FILE******/


