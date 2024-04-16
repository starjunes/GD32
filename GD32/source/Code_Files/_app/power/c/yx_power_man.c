/*
********************************************************************************
** 文件名:     yx_power_man.c
** 版权所有:   (c) 2017 厦门雅迅网络股份有限公司
** 文件描述:   电源管理
** 创建人：        谢金成，2017.5.26
********************************************************************************
*/
#include "yx_includes.h"
#include "bal_input_drv.h"
#include "bal_output_drv.h"
#include "yx_com_send.h"
#include "port_adc.h"
#include "port_gpio.h"
#include "bal_gpio_cfg.h"
#include "port_gsensor.h"
#include "yx_com_man.h"
#include "yx_hit_man.h"
#include "yx_can_man.h"
#include "port_timer.h"
#include "yx_enc_fun.h"
#include "port_sflash.h"
#include "yx_power_man.h"

/*************************************************************************************
*主电ADC阈值定义               
*************************************************************************************/
#if 1
/* 判断12V还是24V的阈值,17V */
#define Z_ADC_THR_PWR_TYPE           1450    /* 17V作为12V和24V判断阈值 */

#define Z_ADC_24V_LOW_POW            1450    /* 17.0V,24主电供电时的主电欠压 */
#define Z_ADC_24V_RECOVERY           1540    /* 18.0V,24主电供电时的主电恢复 */
#define ZH_ADC_24V_RECV_FROM_BAT     1227    /* 14.5V,24备电供电时的主电恢复值 */
//#define ZH_ADC_24V_RECV_FROM_BAT     1689    /* 16.0V,24备电供电时的主电恢复值 */

#define Z_ADC_12V_LOW_POW            692     /* 8.5V  12主电供电时的主电欠压 */
#define Z_ADC_12V_RECOVERY           740     /* 9.0V  12主电供电时的主电恢复 */
#define ZH_ADC_12V_RECV_FROM_BAT     425     /* 5.5V  12备电供电时的主电恢复值 */
//#define ZH_ADC_12V_RECV_FROM_BAT     630     /* 6.5V  12备电供电时的主电恢复值 */

#define ZH_ADC_7V                    290     /* 4.0V 备电供电时的主电小于7.5V认为主电断开 */
#endif


/*************************************************************************************
*备电ADC阈值定义               
*************************************************************************************/
//#define B_ADC_43V           2743    // 满电
#define B_ADC_41V           2050    // 4.1伏，开始充电
#define B_ADC_39V           1950    // 3.9伏，健康监测
#define B_ADC_20V           1025    // 2.05v, 有电池

#define BAKLOWPWR           1700    /* 3.4V,备电馈电(大于3.4V) */
#define BAKGSMLOWPWR        1760    /* 3.52v,电池供电情况下,电池低于3.7V,关闭gsm */

#define MAX_CONVTTMIE       5       /* 最高转换时间 */
#define MAX_ADC_PACKET      5       /* 最高采样次数  */

#define PWRDECT_MASK        0x40    //主电源状态
#define VINLOW_MASK         0x80    //欠压状态
#define ISCHARGE            0x01    //充电状态
#define ISBATHEALTH         0x02    //电池健康状态
#define RTCTIME             30      //RTC休眠时间基数
#define BAT_R_CNT           4       // 内阻检测次数，不能超过4次

typedef enum {
    STEP1,        // 断电2000ms
    STEP2,        // 拉低250ms
    STEP3,        // 拉500ms
    STEP4,        // 拉低
    OVER
} GSM_PSTEP_E;
static GSM_PSTEP_E s_simcompowerstep;
static INT8U s_simcompowertmr;
/*******************************************************************************
                          ADC转换参数结构体
*******************************************************************************/
typedef struct {
	BOOLEAN  onoff;                  /* 转换和上报功能开关 */
	BOOLEAN  isset;                  /* 是否配置过的标志 */
	INT8U    packet;                 /* 上报的包数 */
	INT8U    stores;                 /* 已经转换好的包数 */
	INT16U   period;                 /* 转换的时间间隔 */
	INT16U   timecnts;               /* 时间计数，到间隔时间就启动转换 */
	INT32U   data[MAX_ADC_PACKET];   /* 存储转换的数据 */
	INT32U   tmpdata[MAX_CONVTTMIE]; /* 初始采样值 */
} ADC_CONVT_T;

static ADC_CONVT_T s_adc[MAX_ADC_CHAN];

static INT32U s_mainpwrad;
static INT32U s_bakpwerad;
static INT32U s_BatOcv;
static INT32U s_pwrad[2]={0};
static INT32U tempdata=0;
static BOOLEAN s_firstcheck = true;
static BOOLEAN s_batstatus = true;
static INT8U s_powerhandle_tmr;
static INT8U s_adchandle_tmr, s_bathandle_tmr;
static INT8U s_wakeupgsmhandle_tmr;
static BOOLEAN mainpwrvalue = false;
static INT8U s_curmaskstatus = 0;
static INT8U s_gosleephandle_tmr;
static INT8U s_rtcwake_tmr;
static INT8U s_modedata = 0, chargestate = 0;
static INT8U s_checkbat = 0;
static GSM_PSTEP_E s_simcompowerstep;
static GSM_PSTEP_E s_simcomwakupstep;
static INT8U s_simcompowertmr;
static BOOLEAN s_rtcwkup, s_isgsmwkup = true;
static BOOLEAN s_otwkup;
static BOOLEAN s_simcomisleep = false;    // 模块已经休眠，则不能停止休眠定时器
static INT16U s_rtchalftm, s_rtcmintm;
static INT8U s_batocnt = 0;               // 内阻检测定时器
static BOOLEAN s_bacharge = false;
static INT8U  s_gztest = 0;               // bit0 充电，bit1 放电，bit2 内阻
static BOOLEAN s_gzwork = false;
static INT8U s_gztesttmr;
static BOOLEAN s_gztest_flag = false;
static INT16U s_waketime = 0;             // 休眠唤醒时间

/*******************************************************************************
 ** 函数名:     WakeupgsmTmr()
 ** 函数描述:    采用定时器唤醒simcom
 ** 参数:       无
 ** 返回:       无
 ******************************************************************************/
static void WakeupgsmTmr(void *pdata)
{
	switch (s_simcomwakupstep){
		case STEP1:
            #if DEBUG_RTC_SLEEP > 0
            debug_printf("WakeupgsmTmr STEP1\r\n");
            #endif
		    PORT_SetGpioPin(PIN_EC20WK);
            OS_StartTmr(s_wakeupgsmhandle_tmr, SECOND, 1/*MILTICK, 50*/);
            s_simcomwakupstep = STEP2;
            break;
		case STEP2:
            #if DEBUG_RTC_SLEEP > 0
            debug_printf("WakeupgsmTmr STEP2\r\n");
            #endif
			PORT_ClearGpioPin(PIN_EC20WK);
			OS_StopTmr(s_wakeupgsmhandle_tmr);
            s_simcomwakupstep = OVER;
            break;
        default:
            break;
	}
}
/*******************************************************************************
 ** 函数名:     WakeUpGsm
 ** 函数描述:    唤醒simcom
 ** 参数:       无
 ** 返回:       无
 ******************************************************************************/
static void WakeUpGsm (void)
{
    #if DEBUG_RTC_SLEEP > 0
	debug_printf("WakeUpGsm 唤醒simcom\r\n");
	#endif

    PORT_ClearGpioPin(PIN_EC20WK);
    s_simcomwakupstep = STEP1;
    OS_StartTmr(s_wakeupgsmhandle_tmr, SECOND, 1);
}

/*******************************************************************************
 ** 函数名:     GsmStartTmr()
 ** 函数描述:   模块上电流程
 ** 参数:       无
 ** 返回:       无
 ******************************************************************************/
static void SimcomPwrStartTmr(void *pdata)
{
    switch(s_simcompowerstep) {
        case STEP1:
            bal_Pullup_GSM4VIC();
            bal_Pullup_GSMPWR();
            PORT_ClearGpioPin(PIN_GSMPU);  //GSM开机信号拉高
            OS_StartTmr(s_simcompowertmr, MILTICK, 25);
            s_simcompowerstep = STEP2;
            #if DEBUG_RTC_SLEEP > 0
            debug_printf("拉低(硬件拉高)\r\n");
            #endif
            break;
        case STEP2:
            PORT_SetGpioPin(PIN_GSMPU);    //GSM开机信号拉低
            OS_StartTmr(s_simcompowertmr, MILTICK, 250);//新模块拉低时间长
            s_simcompowerstep = STEP3;
            #if DEBUG_RTC_SLEEP > 0
            debug_printf("拉高(硬件拉低)\r\n");
            #endif
            break;
        case STEP3:
            PORT_ClearGpioPin(PIN_GSMPU);  //GSM开机信号拉高
            OS_StopTmr(s_simcompowertmr);
            s_simcompowerstep = OVER;
            #if DEBUG_RTC_SLEEP > 0
            debug_printf("结束(硬件拉高)\r\n");
            #endif
            break;
        default:
            break;
    }
}

/*******************************************************************************
 ** 函数名:     StartSimcomPower()
 ** 函数描述:   模块上电
 ** 参数:       无
 ** 返回:       无
 ******************************************************************************/
/*static  void StartSimcomPower(void)
{
    PORT_SetPinIntMode(PIN_MCUWK, false);
    s_simcompowerstep = STEP1;
    SimcomPwrStartTmr(NULL);
}*/

/*******************************************************************************
 ** 函数名:     YX_Power_ReStartSimcomPwr()
 ** 函数描述:   模块重启
 ** 参数:       无
 ** 返回:       无
 ******************************************************************************/
void YX_Power_ReStartSimcomPwr(void)
{
    bal_Pulldown_GSM4VIC();
    bal_Pulldown_GSMPWR();
    //bal_Pulldown_232CON();
    //bal_Pullup_VBUSCTL();
    PORT_SetPinIntMode(PIN_MCUWK, false);
    s_simcompowerstep = STEP1;
    OS_StartTmr(s_simcompowertmr, SECOND, 2);
}

/*******************************************************************************
**  函数名称:  Hdl_MSG_RTCWAKE
**  功能描述:  RTC消息处理函数
**  输入参数:  无
**  返回参数:  None
*******************************************************************************/
void Hdl_MSG_RTCWAKE(void)
{
    s_rtcwkup = true;
    if(OS_TmrIsRun(s_gosleephandle_tmr) && (!s_simcomisleep)) {
        OS_StopTmr(s_gosleephandle_tmr);
    }
    if(s_rtchalftm != 0) s_rtchalftm--;
    if((s_rtchalftm == 0) && (s_rtcmintm == 0)) {            //判断是不是最后一次RTC唤醒
        if(s_isgsmwkup == false) {                           //防止二次唤醒gsm
            #if DEBUG_RTC_SLEEP > 0
            debug_printf("RTC唤醒GSM\r\n");
            #endif
            WakeUpGsm ();                                    //mcu唤醒simcom
            //YX_COM_Cleanlink();                              //重新连接心跳
            s_isgsmwkup = true;                              //已唤醒gsm
            goto ret;
        }
        goto ret;
    }
    if(s_otwkup != true) {                                   //其他唤醒源在RTC之前唤醒
        OS_StartTmr(s_rtcwake_tmr, SECOND, 20);              //过了20s在进入休眠
    }
    ret:;
}
/*******************************************************************************
**  函数名称:  Hdl_MSG_OTWAKE
**  功能描述:  其他唤醒消息处理函数
**  输入参数:  无
**  返回参数:  None
*******************************************************************************/
void Hdl_MSG_OTWAKE(void)
{
    s_otwkup = true;

    #if DEBUG_RTC_SLEEP > 0
    debug_printf("外部GPIO唤醒\r\n");
    #endif
	if(OS_TmrIsRun(s_gosleephandle_tmr) && (!s_simcomisleep)) {
		OS_StopTmr(s_gosleephandle_tmr);
	}

    if (s_rtcwkup == true) {                               //其他唤醒源在RTC之后唤醒
        OS_StopTmr(s_rtcwake_tmr);
    }

    if(s_isgsmwkup == false) {                             //防止二次唤醒gsm
        WakeUpGsm ();                                      //mcu唤醒simcom
		#if DEBUG_RTC_SLEEP > 0
		debug_printf("其他唤醒源（非RTC）唤醒GSM\r\n");
		#endif
        //YX_COM_Cleanlink();                                //重新连接心跳
        s_isgsmwkup = true;
    }
}
static void Delay_ms(INT16U time)
{
    INT32U  delaycnt;

    delaycnt = 0xA000 * time;                                          /* 约150ms */
    while (delaycnt--) {
    }
}

/*******************************************************************************
**  函数名称:  LightSleep
**  功能描述:  浅睡
**  输入参数:  无
**  返回参数:  None
*******************************************************************************/
void LightSleep(void)
{
	
#if EN_DEBUG > 0
	static INT32U wakecount = 0;
    debug_printf("\r\n<*****进入浅休眠*****>\r\n");
#endif
    //Port_SFlashMode(0xB9);
    ClearWatchdog();
    Enc_PinEnOrDis(FALSE);
    #if EN_CHARGE_DET > 0
    // 使能充电唤醒 
    PORT_SetPinIntMode(PIN_CHGDECT, TRUE);
    #endif

    #if EN_CAN2_WAKE_UP > 0
    PORT_SetPinIntMode(PIN_CAN1RXWK, TRUE);
    #endif
	
    s_rtcwkup = false;
    s_otwkup = false;

    s_isgsmwkup = false;
    s_simcomisleep = false;

    PORT_SleepGsensor();                                 //GSENSOR进入低功耗模式

    bal_Pulldown_5VEXT();
    #if EN_GSEN_WAKE_UP > 0
    bal_Pulldown_GYRPWR();     // 拉低上电六轴传感器 
    #else
    bal_Pullup_GYRPWR();       // 拉高断电六轴传感器 
    #endif
    bal_Pulldown_CHGEN();
    bal_Pulldown_CAN0STB();
    bal_Pulldown_CAN1STB();
    bal_Pulldown_CAN2STB();
    bal_PullDown_EXT3V();
    bal_PullDown_EXT1V8();
  
    //PORT_DisableCan(CAN_CHN_1);
    //PORT_DisableCan(CAN_CHN_2);
    //PORT_DisableCan(CAN_CHN_3);
    //bal_Pullup_GPSBAT();
    bal_Pulldown_PSCTRL(); // 通讯模块电源低功耗模式控制 
    PORT_SetPinIntMode(PIN_MCUWK, TRUE);
    PORT_GpioLowPower(TRUE);
    PORT_ClearGpioIntSta();
    
    PORT_Sleep(PORT_GetIoInitSta, s_waketime);            /* 休眠指令 */                        

    PORT_SetPinIntMode(PIN_MCUWK, FALSE);
    PORT_GpioLowPower(FALSE);
	
    bal_Pullup_PSCTRL();
    
    //PORT_EnableCan(CAN_CHN_1);
    //PORT_EnableCan(CAN_CHN_2);
    //PORT_EnableCan(CAN_CHN_3);
    
    bal_Pullup_EXT3V();
    bal_Pullup_EXT1V8();
    bal_Pullup_5VEXT();
    bal_Pulldown_GYRPWR();     // 拉低上电六轴传感器 
    bal_Pullup_CHGEN();
    bal_Pullup_CAN0STB();
    bal_Pullup_CAN1STB();
    bal_Pullup_CAN2STB();

#if EN_NEW_BAT_CTL > 0
    bal_Pullup_BATSHUT();    // 上电后打开电池供电 
#endif

    #if EN_CHARGE_DET > 0
    // 关闭充电唤醒 
    PORT_SetPinIntMode(PIN_CHGDECT, FALSE);
    #endif

    #if EN_CAN2_WAKE_UP > 0
    PORT_SetPinIntMode(PIN_CAN1RXWK, FALSE);
    #endif
	//bal_Pulldown_GPSBAT();
    WakeUpGsm();
    OS_StartTmr(s_powerhandle_tmr, SECOND, 10);
    //OS_StartTmr(s_gosleephandle_tmr, MINUTE, 1);
    Enc_PinEnOrDis(TRUE);
	Delay_ms(20);//加延时保证串口完全恢复
    #if EN_DEBUG > 0
    debug_printf("\r\n<*****退出浅休眠--%d*****>\r\n",++wakecount);
    #endif
    ClearWatchdog();
    //Port_SFlashMode(0xAB);
}
/*******************************************************************************
**  函数名称:  DeepSleep
**  功能描述:  深睡
**  输入参数:  无
**  返回参数:  None
*******************************************************************************/
void DeepSleep(void)
{
    //Port_SFlashMode(0xB9);
    ClearWatchdog();
    Enc_PinEnOrDis(FALSE);
    s_simcomisleep = false;
    #if EN_CHARGE_DET > 0
    /* 使能充电唤醒 */
    PORT_SetPinIntMode(PIN_CHGDECT, TRUE);
    #endif
    //PORT_HwTimer_DisableIRQ();
    // PORT_SetAlarm(1);           //RTC定时设置
    PORT_SleepGsensor();           //GSENSOR进入低功耗模式
    bal_Pulldown_GSMPWR();         //关闭simcom
    bal_Pulldown_GSM4VIC();
    PORT_ClearGpioPin(PIN_GSMPU);  //GSM开机信号拉低

    //PORT_DisableCan(CAN_CHN_1);
    //PORT_DisableCan(CAN_CHN_2);
    //PORT_DisableCan(CAN_CHN_3);
    bal_Pulldown_PSCTRL(); /* 通讯模块电源低功耗模式控制 */

    PORT_GpioLowPower(TRUE);
    PORT_ClearGpioIntSta();

    PORT_SetPinIntMode(PIN_MCUWK, TRUE);
    
    PORT_Sleep(PORT_GetIoInitSta, s_waketime);

    PORT_SetPinIntMode(PIN_MCUWK, FALSE);
    
    PORT_GpioLowPower(FALSE);

    bal_Pullup_PSCTRL();
    //PORT_EnableCan(CAN_CHN_1);
    //PORT_EnableCan(CAN_CHN_2);
    //PORT_EnableCan(CAN_CHN_3);
    
    YX_Power_ReStartSimcomPwr();
    YX_COM_Cleanlink();            //重新连接心跳
    #if EN_CHARGE_DET > 0
    /* 关闭充电唤醒 */
    PORT_SetPinIntMode(PIN_CHGDECT, FALSE);
    #endif
    #if EN_NEW_BAT_CTL > 0
    bal_Pullup_BATSHUT();    /* 上电后打开电池供电 */
    #endif   
    Enc_PinEnOrDis(TRUE);
    ClearWatchdog();
    //Port_SFlashMode(0xAB);
}

/*******************************************************************************
**  函数名称:  RtceleclctlTmr
**  功能描述:  RTC定时唤醒定时器函数
**  输入参数:  无
**  返回参数:  None
*******************************************************************************/
static void RtcElecCtlTmr(void *pdata)
{
    OS_StopTmr(s_rtcwake_tmr);       //过了20s在进入休眠
    if(s_rtchalftm != 0) {
        PORT_SetAlarm(RTCTIME);
    } else {
        if(s_rtcmintm != 0) {
            PORT_SetAlarm(s_rtcmintm);
            s_rtcmintm = 0;
        }
    }
    #if DEBUG_RTC_SLEEP > 0
    debug_printf("RTC自动进入休眠:%d\r\n", s_rtchalftm);
    #endif
    PORT_Sleep(PORT_GetIoInitSta, 0);
}

/*******************************************************************************
 ** 函数名:    AdConvertDv
 ** 函数描述:   AD值转换为电压值
 ** 参数:       AD值
 ** 返回:       电压值
 ******************************************************************************/
static FP32 AdConvertDv(INT32U Ad) 
{
    return (Ad*2)/10;   //return (Ad+11) * 10 /640;
}

/*******************************************************************************
 ** 函数名:    ReadBatAdcTmr
 ** 函数描述:  计算电池的内阻
 ** 参数:       无
 ** 返回:       无
 ******************************************************************************/
static void ReadBatAdcTmr(void *pdata)
{
    INT8U i,senddata[4];

    /* 内阻缓存累加值，用于算平均值用 */
    INT32U s_R_BatTol = 0;
    
    /* 内阻平均值 */
    INT16U s_R_BatAvr = 0;
    
    /* 备电负载电压值 */
    INT32U s_BatCcv = 0;    
    
    /* 内阻值缓存 */
    static INT16U s_R_Bat[BAT_R_CNT] = {0 , 0 , 0 , 0};

    /* 测试电池负载电压(负载20欧姆电阻) */
    if ((s_batocnt % 2) == 0) {
        /* 测试负载电压 */
        s_BatCcv = PORT_GetADCValue(ADC_BAKPWR);
        /* 关闭负载 */
        PORT_ClearGpioPin(PIN_CCVEN);
        /* 计算内阻，负载电阻20欧姆，内阻=(20 * (OCV - CCV) / CCV) */
        s_R_Bat[s_batocnt/2] = (s_BatOcv - s_BatCcv) * 20000 / (s_BatOcv);    /* 单位:千欧 */
        /* 500ms后测试开路电压 */
        OS_StartTmr(s_bathandle_tmr, MILTICK, 50);

        #if DEBUG_BAT_ADC > 0
        debug_printf_dir("(1)电池内阻检测次数:%d\r\n", s_batocnt/2 + 1);
        debug_printf_dir("(1)电池开路电压AD值:%d\r\n", s_BatOcv);
        debug_printf_dir("(1)电池闭路电压AD值:%d\r\n", s_BatCcv);
        debug_printf_dir("(1)电池内阻:%d\r\n", s_R_Bat[s_batocnt/2]);
        #endif
    } else {
    /* 测试开路电池电压 */    
    
        /* 测试电池开路电压 */
        s_BatOcv = PORT_GetADCValue(ADC_BAKPWR);    
        /* 打开负载 */
        PORT_SetGpioPin(PIN_CCVEN);
        /* 500ms后测试负载电压 */
        OS_StartTmr(s_bathandle_tmr, MILTICK, 50);

        #if DEBUG_BAT_ADC > 0
        debug_printf_dir("(2)电池内阻检测次数:%d\r\n", s_batocnt/2 + 1);
        debug_printf_dir("(2)电池开路电压AD值:%d\r\n", s_BatOcv);
        #endif
    }
        
    /* 计算内阻平均值 */    
    if (s_batocnt++ >= (BAT_R_CNT-1)*2) {
        s_batocnt = 0;
        s_R_BatTol = 0;
        for (i = 0; i < BAT_R_CNT; i++) {
            s_R_BatTol += s_R_Bat[i];
            #if DEBUG_BAT_ADC > 0
            debug_printf_dir("\r\n<内阻 %d 缓存值: %d>\r\n",i, s_R_Bat[i]);
            #endif            
        }
        /* 计算平均内阻 */
        s_R_BatAvr = s_R_BatTol/BAT_R_CNT;

        #if DEBUG_BAT_ADC > 0
        debug_printf_dir("电池内阻平均值:%d\r\n", s_R_BatAvr);
        #endif
        
        /* 停止内阻检测 */
        OS_StopTmr(s_bathandle_tmr);
        
        /* 内阻1000表示正常 */
        if(s_R_BatAvr <= 1000) {
            chargestate &=~ ISBATHEALTH;
            #if DEBUG_BAT_ADC > 0
            debug_printf("电池正常\r\n");
            #endif
        } else {
        /* 内阻超过1000表示不正常 */
            chargestate |= ISBATHEALTH;
            #if DEBUG_BAT_ADC > 0
            debug_printf("电池损坏\r\n");
            #endif
        }
            
        /* 如果工装测试内阻，则上班内阻 */
        if ((s_gztest & 0x04) > 0) {
            s_gztest &= ~0x04;
            senddata[0] = 0x04;
            senddata[1] = 0x02;
            senddata[2] = s_R_BatAvr >> 8;
            senddata[3] = s_R_BatAvr;
            YX_COM_DirSend( GZ_TEST_REQ_ACK, senddata, 4);
        }
        return;
    }
}

/*******************************************************************************
 ** 函数名:    System_Vol_Judge
 ** 函数描述:  24v或12v系统判定
 ** 参数:       无
 ** 返回:       无
 ******************************************************************************/
static void System_Vol_Judge(void)
{
    if(s_mainpwrad > Z_ADC_THR_PWR_TYPE){
        mainpwrvalue = true; //大于18V判断为24V系统
        #if DEBUG_ADC_MAINPWR > 0
        debug_printf("24V系统 \r\n");
        #endif
    } else {
        mainpwrvalue = false;//默认12V系统
        #if DEBUG_ADC_MAINPWR > 0
        debug_printf("12V系统 \r\n");
        #endif
    }
}

/*******************************************************************************
**  函数名称:  SignalChangeHandle
**  功能描述:  信号状态变化处理
**  输入参数:  无
**  返回参数:  None
*******************************************************************************/
static void SignalChangeHandle(INT8U type, INT8U mode)
{
	switch (type) {
		case  TYPE_PWRDECT:
			if(mode == TRIGGER_POSITIVE) {
				s_firstcheck = true;
				OS_StartTmr(s_powerhandle_tmr, SECOND, 5);
			}
			break;
        case  TYPE_ACC:
            if(mode == TRIGGER_NEGATIVE) { /* ACC低有效，上电是产生下降沿 */
                if (s_isgsmwkup == false) {                             //防止二次唤醒gsm
                    WakeUpGsm ();                                      //mcu唤醒simcom
            		#if DEBUG_ADC_MAINPWR > 0
                    debug_printf("ACC出现下降沿，唤醒Gsm\r\n");
                    #endif
                    s_isgsmwkup = true;
                }
			}
			break;
		default:
			break;
	}
}

/*******************************************************************************
 ** 函数名:    Select_MAINPWRorBAT
 ** 函数描述:  主电与备用电池电压检测管理
 ** 参数:       无
 ** 返回:       无
 ******************************************************************************/
static void Select_MAINPWRorBAT(void)//修改主备电切换流程
{
	INT16U i = 0;
	INT32U accpwrad,accpwrval;
    static INT8U rstcout = 0,batlowcnt = 0;
    static INT8U timecout=0, chargefull = 0;
    static INT8U cout=0, chargefullcheck = 0;
    static INT8U v37_cout = 0, v35_cout = 0;
	INT8U acc_level;
    INT8U acc_status, power_status,bcall_status,ecall_status,bat_status;

    OS_StartTmr(s_powerhandle_tmr, SECOND, 5);
	
    #if DEBUG_BAT_ADC > 1
    static INT8U test_ir = 0;
    if(++test_ir > 2){
        test_ir = 0;
        debug_printf_dir("\r\n<启动内阻检测>\r\n");
        YX_IntRes_Test_Hdl();
    }
    
    #endif
	
    if(PORT_GetGpioPinState(PIN_BAT) == RESET){
		if(batlowcnt <5)batlowcnt++;
		bal_Pullup_BATSHUT();
	}else{
		batlowcnt = 0;
	}
	if(batlowcnt > 3){
		bal_Pulldown_BATSHUT();//电池低压15s关闭电池供电
	}
	if ((s_gztest > 0) || (s_gzwork > 0) || (s_batocnt > 0)) return;
    //acc_status   = !bal_input_ReadSensorFilterStatus(TYPE_ACC); /* acc低有效 */
    acc_level = !bal_input_ReadSensorFilterStatus(TYPE_ACC); /* acc低有效 */
    bcall_status = bal_input_ReadSensorFilterStatus(TYPE_BCALL);
    ecall_status = bal_input_ReadSensorFilterStatus(TYPE_ECALL);
    power_status = bal_input_ReadSensorFilterStatus(TYPE_PWRDECT);
	accpwrad	 = PORT_GetADCValue(ADC_ACCPWR);
	if((accpwrad >= ADC_ACC_VALID) || (acc_level == 1)){
		acc_status = 1;
	}else{
		acc_status = 0;
	}
	/* 用于手动关闭备电供电 */
    if((acc_status == false) /*&& (power_status == false)*/ && (bcall_status == true) && (ecall_status == true)) {
    	rstcout++;
    	if(rstcout >= 2) {
    	     PORT_ClearGpioPin(PIN_BATCTL);    /* ECALL、BCALL接地，切断电池，让设备断电，方便启动BOOT，无需拆壳 */
             /* 关闭备电开关 */
             bal_Pulldown_BATSHUT();    /* ECALL、BCALL接地，切断电池，让设备断电，方便启动BOOT，无需拆壳 */
             rstcout = 2;
			 //debug_printf("bcall_status%d ecall_status:%d\r\n",bcall_status,ecall_status);
    	}
    	return;
    }
	rstcout = 0;
    s_mainpwrad = PORT_GetADCValue(ADC_MAINPWR);
    s_bakpwerad = PORT_GetADCValue(ADC_BAKPWR);
	#if DEBUG_ADC_MAINPWR > 0
	char mainbuf[2][20] = {"主电正常","主电欠压"};
	char backbuf[2][20] = {"电池正常","电池欠压"};
	BOOLEAN bakflag;
	if(batlowcnt > 3){
		bakflag = 1;
	}else{
		bakflag = 0;
	}
	#endif
    #if DEBUG_ADC_MAINPWR > 0
    debug_printf("<---------------主电备电管理--------------->\r\n");
    debug_printf("主电AD值:%d \r\n", s_mainpwrad);
    //debug_printf("主电电压值(电压值扩大了10倍):%d \r\n", (INT32U)((s_mainpwrad*3.3*1093.5*10/4096.0/93.5 + 9)*10/10.5));
    debug_printf("主电电压值(电压值扩大了100倍):%d \r\n", (INT32U)(((s_mainpwrad*11.2)/10 + 76)));    
    debug_printf("主电转换成原来s32k的adc值:%d \r\n", (INT32U)(((s_mainpwrad*11.2 + 760)*10.5 - 9000)/94.2239)); 
    debug_printf("备电AD值:%d \r\n",s_bakpwerad);
    debug_printf("备电电压值(电压值扩大了100倍):%d \r\n", (INT32U)AdConvertDv(s_bakpwerad));
    debug_printf("备电转换成原来s32k的adc值:%d \r\n", (INT32U)(s_bakpwerad * 1.28 - 11)); 
	debug_printf("acc_status:%d bcall_status%d ecall_status:%d	accpwrad(电压值扩大了1000倍):%d\r\n",\
					acc_status,bcall_status,ecall_status,accpwrad);
	#endif


    if(s_firstcheck == true) {
        s_firstcheck = false;
        System_Vol_Judge();
    }

    /* 主电有效，acc on状态 */
    if((acc_status == true) && (power_status == true)){
        s_curmaskstatus &=~ PWRDECT_MASK;
        /* 主电DC-DC芯片使能 */
        PORT_SetGpioPin(PIN_DCEN);    
        /* 等待主电供电稳定后关闭备电 */
        i = 0;
        while (!PORT_GetGpioPinState(PIN_PWRDECT)) {   
            if((i++) >= 1000) return;
        }
        /* 关闭备用电池输出 */
        PORT_ClearGpioPin(PIN_BATCTL);    /* 《acc on》，主电供电,不输出 */      
        
        /* 24V系统 */
        if(true == mainpwrvalue) {                     
            if (s_mainpwrad < Z_ADC_24V_LOW_POW) {
                /* 欠压标志置位 */
                s_curmaskstatus |= VINLOW_MASK;
            }
            if (s_mainpwrad > Z_ADC_24V_RECOVERY) {
                s_curmaskstatus &=~ VINLOW_MASK;
            }
        } else {                   
            /* 12V系统 */
        
            if (s_mainpwrad < Z_ADC_12V_LOW_POW) {
                /* 欠压标志置位 */
                s_curmaskstatus |= VINLOW_MASK;        
            }
            if (s_mainpwrad > Z_ADC_12V_RECOVERY) {
                s_curmaskstatus &=~ VINLOW_MASK;
            }
        }
        s_batstatus = true;                           //ACC_OFF时，再次判断主电电压，进入主备电切换流程，保证再次由主电切回备电
        timecout=0;                                   //清空备电切回主电的计数，再次进入备电时从零开始计数
        cout = 0;
        /* 有电池 */
        if(s_bakpwerad > B_ADC_20V) {                 
            #if DEBUG_ADC_MAINPWR > 0
            debug_printf("有接电池\r\n");
            #endif
            if((chargestate & ISCHARGE) == 0) {
                /* 电池电压大于3.9V，进行电池健康检测 */
                if(s_bakpwerad > B_ADC_39V) {
                    /* 先进行电池健康监测 */
					if(s_checkbat == 0) {
						s_checkbat = 1;
						#if DEBUG_ADC_MAINPWR > 0
					    debug_printf("电池电压大于3.9V,进行电池健康检测\r\n");
						#endif
                        /* 关闭电池输出供电 */
                        PORT_ClearGpioPin(PIN_BATCTL);    /* 检测内阻时，备电不供电 */

                        /* 关闭电池充电 */
						//PORT_ClearGpioPin(PIN_CHGEN);
						bal_Pulldown_CHGEN();
                        /* 电池开路电压 */
						s_BatOcv = PORT_GetADCValue(ADC_BAKPWR); 
                        /* 拉高500ms后，测试备电带负载电压 */
						PORT_SetGpioPin(PIN_CCVEN);
						s_batocnt = 0;
						OS_StartTmr(s_bathandle_tmr, MILTICK, 50);
						return;
					}
				}
			}

            if((chargestate & ISCHARGE) == 0) { // 电池不在充电状态
                if(s_bakpwerad < B_ADC_41V) {   //电池电压小于4.1V，10s后进行充电
                    chargefullcheck++;
                    if(chargefullcheck > 2) {
                        chargestate |= ISCHARGE;
                        PORT_ClearGpioPin(PIN_BATCTL);  /* 《acc on》, 开始充电，关闭备电供电 */                        
                        //PORT_SetGpioPin(PIN_CHGEN);
                        bal_Pullup_CHGEN();
                        #if DEBUG_ADC_MAINPWR > 0
                        debug_printf("进行充电\r\n"); 
                        #endif
                    }
                }
            } else { // 电池充电状态
                #if DEBUG_ADC_MAINPWR > 0
                debug_printf("正在充电\r\n");
                #endif
                chargefullcheck++;
                if(chargefullcheck > 10) {
                    chargefullcheck--;                                 //防止溢出
                    if (PORT_GetGpioPinState(PIN_CHGSTAT) && s_bakpwerad > B_ADC_41V) {           //低电平在充电，高电平充满电
                        chargefull++;
                        if(chargefull >2){
                            chargefull = 0;
                            chargefullcheck = 0;
                            chargestate &=~ ISCHARGE;
                            s_checkbat = 0;
                            PORT_ClearGpioPin(PIN_CHGEN);              //停止充电
                            #if DEBUG_ADC_MAINPWR > 0
                            debug_printf("电池充满电\r\n");
                            debug_printf("满电检测：%d\r\n", PORT_GetGpioPinState(PIN_CHGSTAT));
                            #endif
                        }
                    } else {
                        chargefull = 0;
                        #if DEBUG_ADC_MAINPWR > 0
                        //debug_printf("正在充电\r\n");
                        debug_printf("满电检测：%d\r\n", PORT_GetGpioPinState(PIN_CHGSTAT));
                        #endif
                    }
                }
            }
            //}
        } else {                                          //没有电池
            chargestate |= ISBATHEALTH;                   //认为电池不健康
            chargestate &=~ ISCHARGE;
            s_checkbat = 0;
            //PORT_ClearGpioPin(PIN_CHGEN);                 //充电脚拉低
            //PORT_SetGpioPin(PIN_CHGEN);                     //无电池时也充电，防止电压过低而判断为无电池，而无法对电池充电
            bal_Pullup_CHGEN();
            #if DEBUG_ADC_MAINPWR > 0
            debug_printf("没接电池\r\n");
            #endif
            if (!s_bacharge) {
                OS_StartTmr(s_powerhandle_tmr, SECOND, 10);
                s_bacharge = true;
            }
        }
        #if DEBUG_ADC_MAINPWR > 0
        debug_printf("欠压主电源标志：%x\r\n", s_curmaskstatus);
        #endif

        return;
    }

    /* ACC off，且有备电情况 */
    if((acc_status == false) && ((s_bakpwerad > B_ADC_20V))) {
        if((chargestate & ISCHARGE) == 1) {
            //PORT_ClearGpioPin(PIN_CHGEN);          //充电断开
            bal_Pulldown_CHGEN();
    	    chargestate &=~ ISCHARGE;              //充电标志位置零
    	    s_checkbat = 0;
    	    #if DEBUG_ADC_MAINPWR > 0
            debug_printf("acc off 停止充电\r\n");
            #endif
        }

        if(true == mainpwrvalue) {                             //24V系统
            if(true == s_batstatus) {
                #if DEBUG_ADC_MAINPWR > 0
                debug_printf("主电供电\r\n");
                #endif
                /* 主电欠压 */
                if(s_mainpwrad < Z_ADC_24V_LOW_POW) {
                    s_curmaskstatus |= VINLOW_MASK;            //欠压标志置位
                    //主电欠压状态：备电供电，主电断开，充电断开
                    if(s_bakpwerad > BAKLOWPWR) {              //备电正常
                        s_checkbat = 1;                        //备电供电以及休眠时，禁止进行电池健康管理
                        /* 主电欠压，备电正常，切换到备电供电 */
                        PORT_SetGpioPin(PIN_BATCTL);    /* 《acc off》，主电欠压，备电正常时，打开备电 */
                        PORT_ClearGpioPin(PIN_DCEN);           //主电断开
                    } else {                                   //备电欠压
                        PORT_ClearGpioPin(PIN_BATCTL);    /* 主电欠压，备电也欠压，关闭备电设备全断电 */
                        PORT_ClearGpioPin(PIN_DCEN);           //关闭主电
                    }
                    s_batstatus = false;
                    timecout=0;
                    cout=0;
                } else {                                       //主电不欠压
                    //PORT_SetGpioPin(PIN_CHGEN);              //ACC_OFF备电不充电
                }
            } else {                                           //主电恢复
                #if DEBUG_ADC_MAINPWR > 0
                debug_printf("24v主电恢复计时:%d\r\n", timecout);
                #endif
                if(s_bakpwerad > BAKLOWPWR) {                  //备电正常
                    if(s_bakpwerad > BAKGSMLOWPWR) {           //判断一下电池电压是否适合给gsm供电
                        #if DEBUG_ADC_MAINPWR
                            //debug_printf("simcon备电取电\r\n");
                        #endif
                    } else {
                        v37_cout++;
                        if (v37_cout >=2) {
                            v37_cout = 0;
                            #if DEBUG_ADC_MAINPWR
                                //debug_printf("关闭simcon\r\n");
                            #endif
                        }
                    }
                    v35_cout = 0;
                } else {                                       //备电欠压
                    v35_cout++;
                    if (v35_cout >= 2) {
                        v35_cout = 0;
                        #if DEBUG_ADC_MAINPWR > 0
                        debug_printf("关闭S32\r\n");
                        #endif
                        PORT_ClearGpioPin(PIN_BATCTL);    /* 主电欠压，备电也欠压时，断开备电，系统全断电 */                        
                        PORT_ClearGpioPin(PIN_DCEN);               //关闭主电
                    }
                }
                timecout++;
                if(/* (s_gztest_flag) || */ (timecout > 24)) {                                    //等待2分钟，待电容放电
                    #if DEBUG_ADC_MAINPWR > 0
                    debug_printf("判断主电电压:准备切回主电\r\n");
                    #endif
                    if (s_mainpwrad > ZH_ADC_24V_RECV_FROM_BAT) {                    //状态恢复：主电供电，备电断开，充电使能
                        timecout--;
                        cout++;
                        if(cout > 2) {                                 //多判断一次
                            #if DEBUG_ADC_MAINPWR > 0
                            debug_printf("主电供电\r\n");
                            #endif
                            timecout=0;
                            cout = 0;
                            s_curmaskstatus &=~ VINLOW_MASK;           //欠压标志清零
                            //bal_Pullup_GSMPWR();                     //打开simcom电源
                            //bal_Pullup_GSM4VIC();                    //打开simcom,从主电取电
                            PORT_SetGpioPin(PIN_DCEN);
                            i = 0;
                            while (!PORT_GetGpioPinState(PIN_PWRDECT)) {   //等待主电供电稳定后关闭备电
                                if((i++) >= 1000) return;
                            };
                            PORT_ClearGpioPin(PIN_BATCTL);    /* 主电恢复，断开备电 */
                            //PORT_SetGpioPin(PIN_CHGEN);              //ACC_OFF备电不充电
                            s_batstatus = true;
                        }
                    } else {                                           //备电供电时，进行备电的电压检测
                        cout = 0;
                        timecout--;
                    }
                }
            }
        } else {                                               //12V系统
            if(true == s_batstatus) {
                #if DEBUG_ADC_MAINPWR > 0
                debug_printf("12v主电供电\r\n");
                #endif
                if (s_mainpwrad < Z_ADC_12V_LOW_POW) {                 //主电欠压状态：备电供电，主电断开，充电断开
                    s_curmaskstatus |= VINLOW_MASK;            //欠压标志置位
                    if(s_bakpwerad > BAKLOWPWR) {              //备电正常
                        /* 主电欠压，备电正常，切换到备电供电 */
                        PORT_SetGpioPin(PIN_BATCTL);    /* 主电欠压，备电正常，备电供电 */
                        PORT_ClearGpioPin(PIN_DCEN);           //主电断开
                        if(s_bakpwerad > BAKGSMLOWPWR) {       //判断一下电池电压是否适合给gsm供电
                            //bal_Pulldown_GSM4VIC();          //关闭simcom,simcom会直接从备电取电
                            #if DEBUG_ADC_MAINPWR
                                //debug_printf("simcon备电取电\r\n");
                            #endif
                        } else {
                            //bal_Pulldown_GSMPWR();          //备电供电欠压时,关闭simcom
                            #if DEBUG_ADC_MAINPWR
                                //debug_printf("关闭simcon\r\n");
                            #endif
                        }
                    } else {                                   //备电欠压
                        #if DEBUG_ADC_MAINPWR > 0
                        debug_printf("关闭S32\r\n");
                        #endif
                        PORT_ClearGpioPin(PIN_BATCTL);    /* 主电备电都欠压，关闭备电，全部断电 */                        
                        PORT_ClearGpioPin(PIN_DCEN);           //关闭主电
                    }
                    s_batstatus = false;
                    timecout=0;
                    cout=0;
                } else {                                       //主电不欠压
                    //PORT_SetGpioPin(PIN_CHGEN);              //ACC_OFF备电不充电
                }
            } else {                                           //主电恢复
                #if DEBUG_ADC_MAINPWR > 0
                debug_printf("12V主电恢复计时:%d\r\n", timecout);
                #endif
                if(s_bakpwerad > BAKLOWPWR) {                  //备电正常
                    if(s_bakpwerad > BAKGSMLOWPWR) {           //判断一下电池电压是否适合给gsm供电
                        //bal_Pulldown_GSM4VIC();                //关闭simcom,simcom会直接从备电取电
                        #if DEBUG_ADC_MAINPWR
                            //debug_printf("simcon备电取电\r\n");
                        #endif
                    } else {
                        v37_cout++;
                        if (v37_cout >=2) {
                            //bal_Pulldown_GSMPWR();             //备电供电欠压时,关闭simcom
                            v37_cout = 0;
                            #if DEBUG_ADC_MAINPWR
                                //debug_printf("关闭simcon\r\n");
                            #endif
                        }
                    }
                    v35_cout = 0;
                } else {                                       //备电欠压
                    v35_cout++;
                    if (v35_cout >= 2) {
                        v35_cout = 0;
                        #if DEBUG_ADC_MAINPWR > 0
                        debug_printf("关闭S32\r\n");
                        #endif
                        PORT_ClearGpioPin(PIN_BATCTL);    /* 主电恢复正常，关闭备电供电 */
                        PORT_ClearGpioPin(PIN_DCEN);               //关闭主电
                    }
                }
                timecout++;
                if(/* (s_gztest_flag)|| */  (timecout > 24)) {                                    //等待2分钟，待电容放电
                    #if DEBUG_ADC_MAINPWR > 0
                    debug_printf("判断主电电压:准备切回主电\r\n");
                    #endif
                    if (s_mainpwrad > ZH_ADC_12V_RECV_FROM_BAT) {                       //状态恢复：主电供电，备电断开，充电使能
                        timecout--;
                        cout++;
                        if(cout > 2) {                                 //多判断一次
                            #if DEBUG_ADC_MAINPWR > 0
                            debug_printf("主电供电\r\n");
                            #endif
                            timecout=0;
                            cout = 0;
                            s_curmaskstatus &=~ VINLOW_MASK;           //欠压标志清零
                            //bal_Pullup_GSMPWR();                       //打开simcom电源
                            //bal_Pullup_GSM4VIC();                      //打开simcom,从主电取电
                            PORT_SetGpioPin(PIN_DCEN);
                            i = 0;
                            while (!PORT_GetGpioPinState(PIN_PWRDECT)) {   //等待主电供电稳定后关闭备电
                                if((i++) >= 1000) return;
                            };
                            PORT_ClearGpioPin(PIN_BATCTL);    /* 《acc off》，主电供电正常，关闭备电供电 */
                            //PORT_SetGpioPin(PIN_CHGEN);              //ACC_OFF备电不充电
                            s_batstatus = true;
                        }
                    } else {
                        timecout--;
                        cout = 0;
                    }
                }
            }
        }
    }

    //if(power_status){
    if(s_mainpwrad > ZH_ADC_7V) {
        s_curmaskstatus &=~ PWRDECT_MASK;
    }else{
        s_curmaskstatus |= PWRDECT_MASK;
        s_curmaskstatus &=~ VINLOW_MASK;
    }

    #if DEBUG_ADC_MAINPWR > 0
    debug_printf("欠压主电源标志：%x\r\n", s_curmaskstatus);
    #endif
}

/*******************************************************************************
 ** 函数名:    PowerHandleTmr
 ** 函数描述:   定时器处理函数
 ** 参数:       无
 ** 返回:       无
 ******************************************************************************/
static void PowerHandleTmr(void* pdata)
{
	Select_MAINPWRorBAT();
}

/*******************************************************************
 ** 函数名:     ReportAdcTmr
 ** 函数描述:   ADC应用层模块定时函数
 ** 参数:       [in]  timer:             定时器
 **             [in]  pdata:             定时器私有数据指针
 ** 返回:       无
 ********************************************************************/
static void ReportAdcTmr(void* pdata)
{
	INT8U i,j, start, len;
	INT8U ack[12]={0};
    INT32U s_temp;

    #if EN_ADC_TRAN_S32K > 0    /* 转换成s32k的ad值，主机不需要修改计算公式 */
	s_temp = PORT_GetADCValue(ADC_MAINPWR);
    /* 转换成s32k的值 */
    s_pwrad[0] = (INT32U)(((s_temp*11.2 + 760)*10.5 - 9000)/94.2239);
    
	s_temp = PORT_GetADCValue(ADC_BAKPWR);
    /* 转换成s32k的值 */
    s_pwrad[1] = (INT32U)(s_temp * 1.28 - 11);
    
    #else                      /* 新电路ad值，主机需要修改计算公式 */     
    s_pwrad[0] = PORT_GetADCValue(ADC_MAINPWR);
    s_pwrad[1] = PORT_GetADCValue(ADC_BAKPWR);
    #endif
    
	for(i=0; i<2; i++) {
		if (s_adc[i].timecnts >= MAX_CONVTTMIE) {
		    for (j = 0; j < MAX_CONVTTMIE - 1; j++) {
			    s_adc[i].tmpdata[j] = s_adc[i].tmpdata[j + 1];
			}
			s_adc[i].tmpdata[MAX_CONVTTMIE - 1] = s_pwrad[i];
		} else {
		    s_adc[i].tmpdata[s_adc[i].timecnts] = s_pwrad[i];
		}

		s_adc[i].timecnts++;

		if (true == s_adc[i].onoff) {
		    if (s_adc[i].timecnts >= s_adc[i].period) {   /* 转换时间间隔到 */
			    for (j = 0, tempdata = 0; j < s_adc[i].timecnts; j++) {
				    tempdata += s_adc[i].tmpdata[j];
				}
			    if(s_adc[i].timecnts >= MAX_CONVTTMIE) {
                    tempdata /= MAX_CONVTTMIE;
			    } else {
                    tempdata /= s_adc[i].timecnts;
			    }

                s_adc[i].data[s_adc[i].stores++] = tempdata;
                if (s_adc[i].stores >= s_adc[i].packet) { /* 转换的包数达到上报的包数，上报 */
                    len = 1 + 1 + 2 * s_adc[i].stores;
                    ack[0] = i+1;                         /* 通道来源 */
                    ack[1] = s_adc[i].stores;             /* 数据包数 */
                    start = 0x02;
                    for (j = 0; j < s_adc[i].stores; j++) {
                        ack[start] = (INT8U) (s_adc[i].data[j] >> 8);
                        ack[start+1] = (INT8U) (s_adc[i].data[j]);
                        start += 2;
                    }
                    if (YX_COM_Islink()){
                        YX_COM_DirSend( AD_REPORT_REQ, ack, len);
                    }
                    s_adc[i].stores = 0;                 /* 存储条数清零 */
                }
                s_adc[i].timecnts = 0;
            }
        }
    }
}

/*******************************************************************
 ** 函数名:     GoSleepTmr
 ** 函数描述:    休眠条件判断的定时函数
 ** 参数:       无
 ** 返回:       无
 ********************************************************************/
static void GoSleepTmr(void* pdata)
{
    BOOLEAN acc_status, e_callstat, chgstat;
	INT32U accpwrad;
	INT8U acc_level, power_status;


    if (s_gzwork) {
        s_gzwork = false;
        if(OS_TmrIsRun(s_gosleephandle_tmr)) {
            OS_StopTmr(s_gosleephandle_tmr);
        }
        LightSleep();
        return;
    }

	/* acc on则唤醒主机模块 */
    //acc_status = !bal_input_ReadSensorFilterStatus(TYPE_ACC);
    acc_level = !bal_input_ReadSensorFilterStatus(TYPE_ACC);
	power_status = bal_input_ReadSensorFilterStatus(TYPE_PWRDECT);
    accpwrad	 = PORT_GetADCValue(ADC_ACCPWR);
	if((accpwrad >= ADC_ACC_VALID) || (acc_level == 1)){
		acc_status = 1;
	}else{
		acc_status = 0;
	}
	if(acc_status == TRUE) {
		#if DEBUG_RTC_SLEEP > 0
		debug_printf("ACC退出休眠\r\n");
		#endif
		WakeUpGsm ();                                             // mcu唤醒simcom
		if(OS_TmrIsRun(s_gosleephandle_tmr)) {
			OS_StopTmr(s_gosleephandle_tmr);
		}
		return;
	}

    #if 0 /* 江淮4G TBOX无充电信号，有个器件没焊，屏蔽此功能 */
	e_callstat = PORT_GetGpioPinState(PIN_ECALL);
	if(e_callstat == TRUE) {
		#if DEBUG_RTC_SLEEP > 0
		debug_printf("ECALL退出休眠\r\n");
		#endif
		WakeUpGsm ();                                             // mcu唤醒simcom
		if(OS_TmrIsRun(s_gosleephandle_tmr)) {
			OS_StopTmr(s_gosleephandle_tmr);
		}
		return;
	}
    #endif
    
    /* 外部电源充电检测脚有效，则唤醒主机模块 */
    #if EN_CHARGE_DET > 0
	chgstat = PORT_GetGpioPinState(PIN_CHGDECT);
	if(chgstat == FALSE) {
		#if DEBUG_RTC_SLEEP > 0
		debug_printf("充电输入退出休眠\r\n");
		#endif
		WakeUpGsm ();                                             // mcu唤醒simcom
		if(OS_TmrIsRun(s_gosleephandle_tmr)) {
			OS_StopTmr(s_gosleephandle_tmr);
		}
		return;
	}
    #endif

	if(OS_TmrIsRun(s_gosleephandle_tmr)) {
		OS_StopTmr(s_gosleephandle_tmr);
	}

    //在休眠之前判断电源类型，为休眠唤醒进行电池健康管理做准备
    //（底层事件无法读取输出口状态，返回FALSE）
	if(PORT_GetGpioPinState(PIN_BATCTL) || !PORT_GetGpioPinState(PIN_CHGEN)) {
		s_checkbat = 1;                                       //备电供电以及充电时，禁止进行电池健康管理
	} else {
		s_checkbat = 0;                                       //主电供电，休眠唤醒后，进行一次电池健康管理
	}
	if (power_status == TRUE) {
		if (DisinfectProceed() == TRUE) {
			return;
		}
	}
	switch (s_modedata) {
		case 0x00 :
			#if DEBUG_RTC_SLEEP > 0
			debug_printf("进入浅睡模式\r\n");
			#endif
			LightSleep();
			break;
		case 0x01 :
			#if DEBUG_RTC_SLEEP > 0
			debug_printf("进入深睡模式\r\n");
			#endif
			DeepSleep();
			break;
		default:
			break;
	}
}

/*******************************************************************************
 ** 函数名:     GzTestTmr()
 ** 函数描述:   工装测试延时定时器
 ** 参数:       无
 ** 返回:       无
 ******************************************************************************/
static void GzTestTmr(void *pdata)
{
    INT8U senddata[3];

    OS_StopTmr(s_gztesttmr);

    if ((s_gztest & 0x01) > 0) {
        s_gztest &= ~0x01;
        senddata[0] = 0x01;
        senddata[1] = 0x01;
        senddata[2] = !PORT_GetGpioPinState(PIN_CHGSTAT);       // 充电检测
        YX_COM_DirSend( GZ_TEST_REQ_ACK, senddata, 3);
        #if DEBUG_ADC_MAINPWR > 0
        debug_printf("充电检测应答\r\n");
        #endif
    }
    if ((s_gztest & 0x02) > 0) {
        PORT_SetGpioPin(PIN_DCEN);                    //主电DC-DC芯片使能控制
        while(!PORT_GetGpioPinState(PIN_PWRDECT));    //等待主电供电稳定后关闭备电        
        PORT_ClearGpioPin(PIN_BATCTL);    /* 《工装测试:主电放电测试》关闭备电输出 */
        s_gztest &= ~0x02;
        senddata[0] = 0x02;
        senddata[1] = 0x01;
        senddata[2] = 0x01;
        YX_COM_DirSend( GZ_TEST_REQ_ACK, senddata, 3);
        #if DEBUG_ADC_MAINPWR > 0
        debug_printf("放电检测应答\r\n");
        #endif
    }
    if ((s_gztest & 0x04) > 0) {
		s_BatOcv = PORT_GetADCValue(ADC_BAKPWR);           //电池开路电压
		PORT_SetGpioPin(PIN_CCVEN);
        #if DEBUG_BAT_ADC > 0
        debug_printf_dir("<打开备电负载>\r\n");
        #endif
		OS_StartTmr(s_bathandle_tmr, SECOND, 2);
    }
    if ((s_gztest & 0x08) > 0) {
		if(PORT_GetGpioPinState(PIN_MCUWK)){
			senddata[0] = 0x05;
			senddata[1] = 0x01;
			senddata[2] = 0x01;
		} else {
	        senddata[0] = 0x05;
	        senddata[1] = 0x01;
	        senddata[2] = 0x00;
		}
        s_gztest &= ~0x08;
    	PORT_ClearGpioPin(PIN_EC20WK);
        YX_COM_DirSend( GZ_TEST_REQ_ACK, senddata, 3);
    }
}

/*******************************************************************************
 ** 函数名:    YX_Power_IsUnderVol
 ** 函数描述:   获取主电欠压状态
 ** 参数:       无
 ** 返回:       返回状态
 ******************************************************************************/
INT8U YX_Power_IsUnderVol(void)
{
	return s_curmaskstatus;
}
/*******************************************************************************
 ** 函数名:    YX_Power_Init
 ** 函数描述:   信号量初始化
 ** 参数:       无
 ** 返回:       无
 ******************************************************************************/
void YX_Power_Init(void)
{
	memset(&s_adc[0], 0, sizeof( ADC_CONVT_T));
	memset(&s_adc[1], 0, sizeof( ADC_CONVT_T));
	s_adc[0].onoff = false;
	s_adc[1].onoff = false;

	s_adchandle_tmr = OS_InstallTmr(TSK_ID_OPT, 0, ReportAdcTmr);
    OS_StartTmr(s_adchandle_tmr, SECOND, 1);

    s_powerhandle_tmr = OS_InstallTmr(TSK_ID_OPT, 0, PowerHandleTmr);
    OS_StartTmr(s_powerhandle_tmr, SECOND, 5);

	s_gosleephandle_tmr = OS_InstallTmr(TSK_ID_OPT, 0, GoSleepTmr);
	
    s_rtcwake_tmr = OS_InstallTmr(TSK_ID_OPT, 0, RtcElecCtlTmr);

    s_wakeupgsmhandle_tmr = OS_InstallTmr(TSK_ID_OPT, 0, WakeupgsmTmr);

	bal_input_InstallTriggerProc(TYPE_PWRDECT, TRIGGER_POSITIVE, SignalChangeHandle);
	bal_input_InstallTriggerProc(TYPE_ACC, TRIGGER_NEGATIVE, SignalChangeHandle);

    s_simcompowertmr = OS_InstallTmr(TSK_ID_OPT, 0, SimcomPwrStartTmr);

    s_bathandle_tmr = OS_InstallTmr(TSK_ID_OPT, 0, ReadBatAdcTmr);

    s_gztesttmr = OS_InstallTmr(TSK_ID_OPT, 0, GzTestTmr);

    YX_Power_ReStartSimcomPwr();

    // test
    //PORT_SetAlarmRepeat(1, true, 10);
}

/*******************************************************************************
 ** 函数名:     ADCSetReqHdl
 ** 函数描述:   车台设置ADC请求处理函数
 ** 参数:       [in]data:数据指针
 **             [in]datalen:数据长度
 ** 返回:       无
 ******************************************************************************/
void ADCSetReqHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen)
{
	INT8U ack[2],channel;

    channel = data[0]-1;
	ack[0] = data[0];                            //通道
	if (data[0] <= 0x0 || data[0]>0x02) {        // 超过限制，失败
		ack[1] = 0x02;
		YX_COM_DirSend( AD_SET_REQ_ACK, ack, 2);
		return;
	}
	s_adc[channel].period = (INT16U) ((INT16U) (data[1]) << 8);
	s_adc[channel].period |= (INT16U) (data[2]); //转换时间间隔
	if(s_adc[channel].period > MAX_CONVTTMIE){
        s_adc[channel].period = MAX_CONVTTMIE;
    }	
	s_adc[channel].packet = data[3];             // 上报包数
	if (s_adc[channel].packet > MAX_ADC_PACKET) {
		s_adc[channel].packet = MAX_ADC_PACKET;
	}
	s_adc[channel].timecnts = 0;
	s_adc[channel].stores = 0;
	s_adc[channel].isset = true;

	if (0x01 == data[4]) {           /* 开启 */
		s_adc[channel].onoff = true;
	} else if (0x02 == data[4]) {    /* 关闭 */
		s_adc[channel].onoff = false;
	}
	ack[1] = 0x01;
	YX_COM_DirSend( AD_SET_REQ_ACK, ack, 2);
}
/*******************************************************************
 ** 函数名:     ADCDataRepAckHdl
 ** 函数描述:   获取AD数据应答
 ** 参数:       [in]data:数据指针
 **             [in]datalen:数据长度
 ** 返回:       无
 ********************************************************************/
void ADCDataRepAckHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen)
{
    mancode     = mancode;
    command     = command;
    data        = data;
    datalen     = datalen;

}

/**************************************************************************************************
**  函数名称:  GetADCDataHdl
**  功能描述:  车台主动获取AD处理函数
**  输入参数:  version
            :  command
            :  userdata
            :  userdatalen
**  返回参数:  None
**************************************************************************************************/
void GetADCDataHdl(INT8U mancode, INT8U command, INT8U *data, INT16U datalen)
{
    INT8U      ack[4];
    INT32U     tempdata;
    INT16U     adcdata;
    INT8U      num;

    mancode     = mancode;
    command     = command;
    datalen     = datalen;

    ack[0] = data[0];
    num = data[0] - 1;
    if (num >= 0x02) {                                                      /* 超过限制，失败 */
        ack[1] = 0x02;
        YX_COM_DirSend( ADC_DATA_GET_ACK, ack, 2);
        return;
    }

    tempdata = s_adc[num].data[num];
    adcdata = (INT16U)tempdata;
    bal_shorttochar(&ack[2], adcdata);
    ack[1] = 0x01;

    #if DEBUG_ADC_MAINPWR > 0
    debug_printf("AD:%d,CH:%d\r\n",tempdata,num);
    #endif
    YX_COM_DirSend( ADC_DATA_GET_ACK, ack, 4);

}

/*******************************************************************************
**  函数名称:  PowerControl_Hd
**  功能描述:  电源控制处理函数（进入低功耗模式）
**  输入参数:  无
**  返回参数:  None
*******************************************************************************/
void PowerControl_Hdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen)
{
    INT8U  ack = 0x01;

    YX_COM_DirSend(POWER_CONTROL_REQ_ACK, &ack, 1);

    s_simcomisleep = true;

    s_waketime = bal_chartoshort(&data[1]);
	if(s_waketime != 0x0) {
		PORT_SetAlarm(s_waketime);
	}

    s_rtchalftm = 1;
    s_rtcmintm = 0;

	#if DEBUG_RTC_SLEEP > 0
	debug_printf("收到休眠指令(按照GSM设置的时间进行设置)\r\n");
	debug_printf("设置休眠时间:%d\r\n", s_waketime);
	#endif

    s_modedata = data[0];
    OS_StartTmr(s_gosleephandle_tmr, SECOND, 2);
}

/**********************工装测试协议*********************************/

/*******************************************************************
 ** 函数名:     YX_Charge_Test_Hdl
 ** 函数描述:   充电检测处理
 ** 参数:       无
 ** 返回:       无
 ********************************************************************/
void YX_Charge_Test_Hdl(void)
{
    #if DEBUG_ADC_MAINPWR > 0
    debug_printf("备电充电检测\r\n");
    #endif
    s_gzwork = true;
    s_gztest |= 0x01;/* 充电检测 */
    PORT_SetGpioPin(PIN_DCEN);                    //主电DC-DC芯片使能控制
    while(!PORT_GetGpioPinState(PIN_PWRDECT));    //等待主电供电稳定后关闭备电
    PORT_ClearGpioPin(PIN_BATCTL);    /* 《工装测试：备电充电测试》关闭输出 */
    //PORT_SetGpioPin(PIN_CHGEN);                   //使能充电
    bal_Pullup_CHGEN();
    chargestate |= ISCHARGE;
    OS_StartTmr(s_gztesttmr, SECOND, 1);
}

/*******************************************************************
 ** 函数名:     YX_Discharge_Test_Hdl
 ** 函数描述:   放电检测处理
 ** 参数:       无
 ** 返回:       无
 ********************************************************************/
void YX_Discharge_Test_Hdl(void)
{
    #if DEBUG_ADC_MAINPWR > 0
    debug_printf("备电放电检测\r\n");
    #endif
    s_gzwork = true;
    s_gztest |= 0x02;/* 放电检测 */
    PORT_SetGpioPin(PIN_BATCTL);    /* 《工装测试：备电放电测试》打开备电供电 */
    PORT_ClearGpioPin(PIN_DCEN);           //主电断开
    s_batstatus = false;
    OS_StartTmr(s_gztesttmr, SECOND, 1);
}

/*******************************************************************
 ** 函数名:     YX_IntRes_Test_Hdl
 ** 函数描述:   电池内阻检测处理
 ** 参数:       无
 ** 返回:       无
 ********************************************************************/
void YX_IntRes_Test_Hdl(void)
{
    #if DEBUG_BAT_ADC > 0
    debug_printf_dir("备电内阻检测\r\n");
    #endif
    s_gzwork = true;
    s_gztest |= 0x04;/* 内阻检测 */
    chargestate &=~ ISCHARGE;
    PORT_SetGpioPin(PIN_DCEN);                    //主电DC-DC芯片使能控制
    while(!PORT_GetGpioPinState(PIN_PWRDECT));    //等待主电供电稳定后关闭备电
    PORT_ClearGpioPin(PIN_BATCTL);    /* 《工装测试:内阻检测》，关闭备电供电 */
    s_batocnt = 0;
    #if DEBUG_BAT_ADC > 0
    debug_printf_dir("<关闭备电充电>\r\n");
    #endif
    //PORT_ClearGpioPin(PIN_CHGEN);                 //停止充电
    bal_Pulldown_CHGEN();
    OS_StartTmr(s_gztesttmr, SECOND, 1);
}

/******************************************************************************
 ** 函数名:     YX_WkUpIO_Test_Hdl
 ** 函数描述:   双向IO检查处理
 ** 参数:       无
 ** 返回:       无
 ******************************************************************************/
void YX_WkUpIO_Test_Hdl(void)
{
    INT8U senddata[3];

	#if DEBUG_ADC_MAINPWR > 0
    debug_printf("双向IO检测\r\n");
	#endif

	if(!PORT_GetGpioPinState(PIN_MCUWK)){
		s_gzwork = true;
		s_gztest |= 0x08;/* 双向IO检测 */
		PORT_SetGpioPin(PIN_EC20WK);
	    OS_StartTmr(s_gztesttmr, SECOND, 1);
	} else {
        senddata[0] = 0x05;
        senddata[1] = 0x01;
        senddata[2] = 0x00;
        YX_COM_DirSend( GZ_TEST_REQ_ACK, senddata, 3);
	}
}

/******************************************************************************
 ** 函数名:     YX_GZ_Test_Hdl
 ** 函数描述:   生产工装检测通知处理
 ** 参数:       无
 ** 返回:       无
 ******************************************************************************/
void YX_GZ_Test_Hdl(void)
{
    INT8U senddata[3];

	#if DEBUG_ADC_MAINPWR
	debug_printf("生产工装检测通知\r\n");
	#endif
    s_gzwork = false;
    s_gztest = 0;
    s_batocnt = 0;
    s_gztest_flag = true;

    senddata[0] = 0x06;
    senddata[1] = 0x01;
    senddata[2] = 0x01;
    YX_COM_DirSend(GZ_TEST_REQ_ACK, senddata, 3);
}

