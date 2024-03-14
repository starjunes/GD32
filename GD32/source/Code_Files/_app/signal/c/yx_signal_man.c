/*
********************************************************************************
** 文件名:     yx_signal_man.c
** 版权所有:   (c) 2017 厦门雅迅网络股份有限公司
** 文件描述:   管脚信号处理及上报
** 创建人：        谢金成，2017.5.26
********************************************************************************
*/
#include "yx_includes.h"
#include "yx_signal_man.h"
#include "bal_input_drv.h"
#include "bal_output_drv.h"
#include "yx_com_send.h"
#include "port_gpio.h"
#include "yx_power_man.h"
#include "port_gsensor.h"
#include "yx_can_man.h"
#include "yx_com_man.h"
#include "port_adc.h"
#include "bal_gpio_cfg.h"
#include "yx_configure.h"
#include "yx_lock.h"
#include "hal_exrtc_sd2058_drv.h"
#include "port_can.h"

#define IG_ON_MASK    0x01
#define BCALL_MASK    0x02
#define ECALL_MASK    0x04

static INT8U  s_signalhanle_tmr;
static INT8U  s_getsignalstau_tmr;
static BOOLEAN    s_chginfomoff;     /* 信号改变通知功能是否关闭，TRUE关闭，FALSE开启，默认开启 */

#if DEBUG_SLEEP_STATUS > 0
static INT8U  s_accofftime=0;
#endif
static INT8U acc0;//, ecall0, bcall0;
static INT8U acc1;//ecall1, bcall1;

#if EN_TESTCAN > 0
/*******************************************************************************
**  函数名称:  TestCan
**  功能描述:  CAN测试报文发送
**  输入参数:  CAN通道
**  返回参数:  无
*******************************************************************************/
static void TestCan(INT8U channel)
{
    INT8U senddata[13] = {0x00,0x00,0x11,0x11,0x08,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05};

    senddata[2] = 0xaa + channel * 0x11;
    senddata[3] = 0xaa + channel * 0x11;

    CAN_TxData(senddata, false, channel);
}
#endif

/*******************************************************************************
**  函数名称:  SignalLedOut
**  功能描述:  信号灯输出
**  输入参数:  无
**  返回参数:  None
*******************************************************************************/
static INT8U  s_gps_status = 0xFF, s_wifi_status;
static BOOLEAN /*s_gprs_status, */s_canrx_status;
static void SignalLedOut(void)
{
    INT8U  gps_status,wifi_status;
    INT8U /*gprs_status, */canrx_status;
	//CAN灯
    canrx_status = Can_GetRxStat()|0x02;
    if (PORT_GetBusOffStatus(CAN_CHN_1) == TRUE) {
        canrx_status &= 0x02;
    }
    if (PORT_GetBusOffStatus(CAN_CHN_2) == TRUE) {
        canrx_status &= 0x01;
    }
    #if DEBUG_SIGNAL_STATUS > 0
    debug_printf("CAN通信状态:%d\r\n", canrx_status);
    #endif
    if (s_canrx_status != canrx_status) {
        if ((canrx_status & 0x03) == 0x03) {		//两路can通讯正常
            bal_output_InstallPermnentPort(PORT_CANLED, 5, 5);
            #if DEBUG_SIGNAL_STATUS > 0
            debug_printf("CAN通信正常\r\n");
            #endif
        } else if((canrx_status & 0x03) != 0x00){	//一路can通讯正常
            bal_output_InstallPermnentPort(PORT_CANLED, 10, 10);
            #if DEBUG_SIGNAL_STATUS > 0
            debug_printf("CAN通信一路正常\r\n");
            #endif
        }else{										//两路can通讯异常
			bal_output_RemovePermnentPort(PORT_CANLED);
            #if DEBUG_SIGNAL_STATUS > 0
            debug_printf("CAN通信异常\r\n");
			#endif
		}
    }
    s_canrx_status = canrx_status;

    #if 0 /* 网络灯在EC20 */
    gprs_status = GetGPRSStatus();
    #if EN_DEBUG > 1
    debug_printf("网络状态:%d\r\n", gprs_status);
    #endif
    if (s_gprs_status != gprs_status) {
        if (gprs_status) {
            bal_output_InstallPermnentPort(PORT_NETLED, 6, 6);
            #if EN_DEBUG > 1
            debug_printf("有网络\r\n");
            #endif
        } else {
            bal_output_RemovePermnentPort(PORT_NETLED);
            bal_output_CtlPort(PORT_NETLED, 1);
            #if EN_DEBUG > 1
            debug_printf("无网络\r\n");
            #endif
        }
    }
    s_gprs_status = gprs_status;
    #endif
	//GPS灯
    gps_status = GetGPSStatus();
    #if EN_DEBUG > 1
    debug_printf("定位状态:%02X  s_gps_status=%02X\r\n", gps_status,s_gps_status);
    #endif
    if (s_gps_status != gps_status) {
		if ((gps_status & 0xC0) != 0x00) {			
            //bal_output_RemovePermnentPort(PORT_GPSLED);
            //bal_output_CtlPort(PORT_GPSLED, 1);
            bal_output_RemovePermnentPort(PORT_GPSLED);
            #if EN_DEBUG > 1
            debug_printf("定位模块功能关闭或异常\r\n");
            #endif
        } else if ((gps_status & 0x30) == 0x00) {
            bal_output_InstallPermnentPort(PORT_GPSLED, 10, 10);
            #if EN_DEBUG > 1
            debug_printf("定位模块开启但设备未定位成功\r\n");
            #endif
        }  else if ((gps_status & 0x30) != 0x30) {
            bal_output_InstallPermnentPort(PORT_GPSLED, 5, 0);
            #if EN_DEBUG > 1
            debug_printf("定位成功\r\n");
            #endif
        } else {	
        	bal_output_RemovePermnentPort(PORT_GPSLED);
			#if EN_DEBUG > 1
            debug_printf("定位模块功能关闭或异常\r\n");
            #endif
        }
        /*if ((gps_status & 0x30) == 0x00) {			
            //bal_output_RemovePermnentPort(PORT_GPSLED);
            //bal_output_CtlPort(PORT_GPSLED, 1);
            bal_output_InstallPermnentPort(PORT_GPSLED, 10, 10);
            #if EN_DEBUG > 1
            debug_printf("未定位成功\r\n");
            #endif
        } else if ((gps_status & 0x30) == 0x10) {
            bal_output_InstallPermnentPort(PORT_GPSLED, 5, 0);
            #if EN_DEBUG > 1
            debug_printf("2D定位\r\n");
            #endif
        }  else if ((gps_status & 0x30) == 0x20) {
            bal_output_InstallPermnentPort(PORT_GPSLED, 5, 0);
            #if EN_DEBUG > 1
            debug_printf("3D定位\r\n");
            debug_printf("\r\n");
            #endif
        } else {	//定位模块异常或关闭
        	bal_output_InstallPermnentPort(PORT_GPSLED, 0, 1);
			#if EN_DEBUG > 1
            debug_printf("定位模块功能关闭或异常\r\n");
            #endif
        }*/
    }
    s_gps_status = gps_status;

	//WIFI灯
	wifi_status = Getwifistatus();
	if (s_wifi_status != wifi_status) {
        if (wifi_status == 0x01) {			
			bal_output_InstallPermnentPort(PORT_WIFILED, 10, 10);
			 #if EN_DEBUG > 1
            debug_printf("WIFI模块未接入终端\r\n");
            #endif
        } else if(wifi_status == 0x02){	//WIFI模块接入终端  常亮
        	bal_output_InstallPermnentPort(PORT_WIFILED, 5, 0);
			#if EN_DEBUG > 1
            debug_printf("WIFI模块已接入终端\r\n");
            #endif
        }else{
			bal_output_RemovePermnentPort(PORT_WIFILED);
			#if EN_DEBUG > 1
            debug_printf("WIFI模块无效或未工作\r\n");
            #endif
		}
    }
	s_wifi_status = wifi_status;
}

/*******************************************************************************
**  函数名称:  SignalReport
**  功能描述:  传感器信号变动通知函数，按协议帧格式发数据上行通知
**  输入参数:  无
**  返回参数:  None
*******************************************************************************/
static void SignalReport(void)
{
    INT8U  acc_sta, ecall_sta, bcall_sta, powdet_sta, gpsopen_sta, gpsshort_sta, chg_sta;
    INT8U  ack[32], len, num = 0;
    INT8U  volstatus = 0;
    INT8U  ioreportstatus[4] = {0, 0, 0, 0};
	INT32U accpwrad, acclevel;
    #if EN_TESTCAN > 0
    INT8U i;
    for (i = CAN_CHN_1; i < MAX_CAN_CHN; i++) {
        TestCan(i);
    }    
    #endif

    volstatus    = YX_Power_IsUnderVol();
    if(volstatus & 0x80){
    	volstatus = 0x01;    /* 欠压状态 */
    }else{
    	volstatus = 0x00;
    }
    acclevel     = !bal_input_ReadSensorFilterStatus(TYPE_ACC);
	accpwrad	 = PORT_GetADCValue(ADC_ACCPWR);
	if((accpwrad >= ADC_ACC_VALID) || (acclevel == 1)){
		acc_sta = 1;
	}else{
		acc_sta = 0;
	}
    ecall_sta    = bal_input_ReadSensorFilterStatus(TYPE_ECALL);
    bcall_sta    = bal_input_ReadSensorFilterStatus(TYPE_BCALL);
    powdet_sta   = !bal_input_ReadSensorFilterStatus(TYPE_PWRDECT); /* 高有效(低电平表示掉电) */
    //powdet_sta   = !bal_ReadPort_PWRDECT(); /* 高有效(低电平表示掉电) */
    gpsopen_sta  = bal_input_ReadSensorFilterStatus(TYPE_GPSOPEN);
    gpsshort_sta = bal_input_ReadSensorFilterStatus(TYPE_GPSSHORT);
    chg_sta      = !bal_input_ReadSensorFilterStatus(TYPE_CHG);     /* 低有效 */
	#if DEBUG_SIGNAL_STATUS > 0
	debug_printf("acc_sta:%d ecall_sta:%d ecall_sta:%d powdet_sta:%d gpsopen_sta:%d gpsshort_sta:%d\r\n",\
					acc_sta,ecall_sta,ecall_sta,powdet_sta,gpsopen_sta,gpsshort_sta);
	#endif
	#if 0 /* test */
    if (acc_sta) {
        PORT_SetGpioPin(PIN_EC20WK);
    } else {
        PORT_ClearGpioPin(PIN_EC20WK);
    }
    #endif

    /* 信号状态 */
    ioreportstatus[0] = (acc_sta << 7) | (ecall_sta << 5) | (bcall_sta << 4) | (powdet_sta << 3) | (gpsopen_sta << 2) | (gpsshort_sta << 1);
    ioreportstatus[1] = (chg_sta << 7);
    ioreportstatus[2] = 0;
    ioreportstatus[3] = 0;

    YX_MEMSET(ack, 0x00, sizeof(ack));
    len = 0;
    num = 0;
	ack[len++] = 0;                             // 类型总和(最后填充)
	/* 欠压状态 */
	ack[len++] = 0x06;                        //类型
	ack[len++] = 0x01;                        //长度
	ack[len++] = volstatus;                  //欠压状态
    num++;

    /* 安全气囊状态 */
	ack[len++] = 0x07;                        //类型
	ack[len++] = 0x01;                        //长度
	ack[len++] = !PORT_GetGpioPinState(PIN_AIRBAG);  // EN_GZTEST 安全气囊状态
    num++;

    /* 信号状态 */
	ack[len++] = 0x08;                        //类型
	ack[len++] = 0x04;                        //长度
    ack[len++] = ioreportstatus[0];
    ack[len++] = ioreportstatus[1];
    ack[len++] = ioreportstatus[2];
    ack[len++] = ioreportstatus[3];
    num++;

	ack[0] = num;                           // 类型总和
    if (YX_COM_Islink() /*&& YX_GetUpdateStatus()*/){
    	YX_COM_DirSend( RETIM_STATUS_REPORT, ack, len);
    }
    #if DEBUG_SIGNAL_STATUS > 0
    debug_printf("sinal data:");
    printf_hex(ack,len);
	debug_printf("\r\n");
    #endif
    /* ACC(低有效)断开一断时间后进入休眠 */
    #if DEBUG_SLEEP_STATUS > 0
    if(!acc_sta) {
        s_accofftime++;
        //debug_printf("ACC:%d\r\n", s_accofftime);
        if(s_accofftime == 5) {
            //PORT_SetAlarm(100);       //RTC定时设置
            //LightSleep();
            DeepSleep();
            s_accofftime = 0;
        }
    } else {
        s_accofftime = 0;
    }
    #endif

}

/*******************************************************************************
 ** 函数名:    SignalHandleTmr
 ** 函数描述:   定时器处理函数
 ** 参数:       无
 ** 返回:       无
 ******************************************************************************/

static void SignalHandleTmr(void* pdata)
{
    #if 0
	INT16U pulsecout = 0;
    INT32U pulsefreq = 0;

	pulsecout = PORT_PinGetAirbagCount();
	debug_printf("脉冲计数:%d\r\n",pulsecout);
    pulsefreq = PORT_PinGetAirbagFreq();
    debug_printf("脉冲频率:%d\r\n",pulsefreq);
    #endif
	INT8U acc_level;
    acc_level = !bal_input_ReadSensorFilterStatus(TYPE_ACC);
    INT32U accpwrad;
	accpwrad	 = PORT_GetADCValue(ADC_ACCPWR);
	if((accpwrad >= ADC_ACC_VALID) || (acc_level == 1)){
		acc1 = TRUE;
	}else{
		acc1 = FALSE;
	}
	if(acc0 == FALSE){
		if (GetSpeedFlag() == TRUE){
			acc1 = TRUE;
			SetSpeedFlag(FALSE);
		}
	}
    if(acc0 != acc1){
        if(acc1 == TRUE){
			#if DEBUG_LOCK > 0
            debug_printf("ACC  ON\r\n");
            #endif
            ACCON_HandShake();
        }else{
            #if DEBUG_LOCK > 0
            debug_printf("ACC  OFF\r\n");
            #endif
            ACCOFF_HandShake();
			KMS_Hand_Send_Set(TRUE);
			SetSpeedFlag(FALSE);
        }
        acc0 = acc1;
    }

	//debug_printf("SignalHandleTmr\r\n");
    SignalReport();

    SignalLedOut();
}
/*******************************************************************************
 ** 函数名:    GetAccState
 ** 函数描述:   获取ACC状态
 ** 参数:       无
 ** 返回:       无
 ******************************************************************************/
INT8U GetAccState(void)
{
    return acc1;
}

/*******************************************************************************
 ** 函数名:    SignalHandleTmr
 ** 函数描述:   定时器处理函数
 ** 参数:       无
 ** 返回:       无
 ******************************************************************************/
static void GetSignalstatusTmr(void* pdata)
{
    /*-----------------主机状态请求-------------------*/
    INT8U  ack[32], len, num = 0,data[7];
	static INT8U count = 0;
	BOOLEAN ret;
		
   	YX_MEMSET(ack, 0x00, sizeof(ack));
	YX_MEMSET(data, 0x00, sizeof(data));
		
	ret = HAL_sd2058_ReadCalendar(data);
	SendTimeCan(data);

	if((count++ >= 2) && YX_COM_Islink()) { 
		count = 0;
		if(ret) {
		    len = 0;
		    num = 0;
		    ack[len++] = 0; 					// 类型总和(最后填充)
		    /* 欠压状态 */
		    ack[len++] = 0x01;			  //类型
		    ack[len++] = 0x06;				//长度
		    ack[len++] = data[6]; 		//YEAR
		    ack[len++] = data[5]; 	  //MONTH
		    ack[len++] = data[4]; 		//DATE
		    ack[len++] = data[2]; 		//HOUR
		    ack[len++] = data[1]; 		//MINITE
		    ack[len++] = data[0]; 		//TICK
		    num++;
		    ack[0] = num;        	
			YX_COM_DirSend( GET_HOSTSTATUS_REQ, ack, len);
		} else {      	 
		    YX_COM_DirSend( GET_HOSTSTATUS_REQ, NULL, 0);
		}
	} else {
		if (YX_COM_Islink() != TRUE) {
			count = 0;
		}
	}
}
/*******************************************************************************
 ** 函数名:    YX_Signal_Init
 ** 函数描述:   信号量初始化
 ** 参数:       无
 ** 返回:       无
 ******************************************************************************/
void YX_Signal_Init(void)
{
    #if 0
    bal_input_InstallTriggerProc(TYPE_SRS, MASK_TRIGGER, SignalChangeHandle);
    bal_input_InstallTriggerProc(TYPE_BCALL, MASK_TRIGGER, SignalChangeHandle);
    bal_input_InstallTriggerProc(TYPE_ECALL, MASK_TRIGGER, SignalChangeHandle);
    bal_input_InstallTriggerProc(TYPE_CHG, MASK_TRIGGER, SignalChangeHandle);
    #endif
    acc0 = TRUE;//acc0 = bal_input_ReadSensorFilterStatus(TYPE_ACC);
    s_chginfomoff = FALSE;  /*默认开启*/
    s_signalhanle_tmr = OS_InstallTmr(TSK_ID_OPT, 0, SignalHandleTmr);
    OS_StartTmr(s_signalhanle_tmr, SECOND, 1);
    s_getsignalstau_tmr = OS_InstallTmr(TSK_ID_OPT, 0, GetSignalstatusTmr);
    OS_StartTmr(s_getsignalstau_tmr, SECOND, 1);
}


/**************************************************************************************************
**  函数名称:  SigFilterParaConfig_ReqHdl
**  功能描述:  信号滤波参数配置请求处理函数，接收来自上端的配置参数进行配置(0xD6指令，0xd6 <==> 0x56)
**  输入参数:
**  返回参数:
**************************************************************************************************/
void SigFilterParaConfig_ReqHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen)
{
     INT8U   ack[2], port;
     INT16U  lowtime, hightime;

     port = data[0];
     ack[0] = port;
     hightime = bal_chartoshort(&data[1]);
     lowtime  = bal_chartoshort(&data[3]);

     bal_input_SetFilterPara(port, lowtime, hightime);

     ack[1] = 0x01;

     YX_COM_DirSend( SIGFILTER_PARA_CONF_ACK, ack, 2);
}

/**************************************************************************************************
**  函数名称:  SigChgInfomONOFF_ReqHdl
**  功能描述:  信号改变通知功能开启关闭请求处理函数(0xCB指令 0xCB <=====> 0x4B)
**  输入参数:
**  返回参数:
**************************************************************************************************/
void SigChgInfomONOFF_ReqHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen)
{
    INT8U ack[2];

    ack[0] = data[0];

    switch (data[0]) {
        case 0x01:
            s_chginfomoff = FALSE;
            ack[1] = 0x01;
            break;
        case 0x02:
            s_chginfomoff = TRUE;
            ack[1] = 0x01;
            break;
        default :
            ack[1] = 0x02;
            break;
    }
    YX_COM_DirSend( SIG_CHG_INFO_ONFF_REQ_ACK, ack, 2);
}

/**************************************************************************************************
**  函数名称:  SigChgInfom_AckHdl
**  功能描述:  信号改变后，上报通知，车台回复，对该回复暂做空处理(0xCC指令  0xCC<=====>0x4C)
**  输入参数:
**  返回参数:
**************************************************************************************************/
void SigChgInfom_AckHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen)
{

}

/**************************************************************************************************
**  函数名称:  RealTimeStatusReport_AckHdl
**  功能描述:  实时状态上报后，车台回复，对该回复暂做空处理(0x91指令 0x91 <===> 0x11)
**  输入参数:
**  返回参数:
**************************************************************************************************/
void RealTimeStatusReport_AckHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen)
{

}
/**************************************************************************************************
**  函数名称:  RTC_Synchro_Hdl
**  功能描述:  RTC时间设置
**  输入参数:
**  返回参数:
**************************************************************************************************/
void RTC_Synchro_Hdl(INT8U mancode, INT8U command, INT8U *userdata, INT16U userdatalen)
{
	  INT8U buf[7];
	  buf[0] = userdata[5];           /* 秒 */
    buf[1] = userdata[4];           /* 分 */
    buf[2] = userdata[3];           /* 时 */
    buf[3] = 1;                    /* 星期 */
    buf[4] = userdata[2];           /* 日 */
    buf[5] = userdata[1];           /* 月 */
    buf[6] = userdata[0];           /* 年 */
 		#if DEBUG_EX_RTC > 0
		debug_printf("RTC_Synchro_Hdl\r\n");
		printf_hex(buf,7);
	  #endif
    if (HAL_sd2058_SetCalendar(buf)) {
        YX_COM_DirSend( RTC_SYNC_REQ_ACK, NULL,0);
    }
}


