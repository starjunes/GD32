/*
********************************************************************************
** 文件名:     yx_com_man.c
** 版权所有:   (c) 2017 厦门雅迅网络股份有限公司
** 文件描述:   外设驱动基本功能业务，外设状态管理
** 创建人：        谢金成，2017.5.18
********************************************************************************
*/
#include  "yx_includes.h"
#include  "yx_com_send.h"
#include  "yx_com_recv.h"
#include  "yx_com_man.h"
#include  "yx_version.h"
#include  "bal_gpio_cfg.h"
#include  "yx_power_man.h"
#include  "bal_output_drv.h"
#include  "bal_input_drv.h"

/*
 *******************************************************************************
 *宏定义
 *******************************************************************************
 */
#define  INIT_BAUD_921600			921600L           /* 初始波特率 */
#define  DEFAULT_BAUD_115200		115200L
#define  RESUME_BAUD                0x07              /* 恢复波特率，0x07对应115200 */

#define  COM_PERIOD_SENDLINK        1                 /* 上电链接，1秒一次  */
#define  COM_PERIOD_SENDQUREY       10                /* 链路维护，10秒一次 */
#define  COM_PERIOD_LINKMAX         2                 /* 最大链路维护次数 */
#define  COM_TIMES_JUDGERESTART     60                /* 超时重启时长 */
#define  FIRST_COM_LINK             180               /* 首次超时时间 */
#define  OTA_COM_LINK               1800              /* 首次超时时间 30min */
INT8U s_sendlink_tmrid;                               /* 上电注册、链路维护定时器 */
/*
 *******************************************************************************
 * 定义模块局部变量
 *******************************************************************************
 */
static BOOLEAN s_com_linked = false;        /*链接标志  */
static INT8U   s_com_overtime;              /*链接超时  */
static INT32U  s_com_cnt_nolink;            /*计数未连接的时间 */
static INT8U   s_sendlink_cnt = 0;          /*链路协议发送计数器 */
static BOOLEAN s_firstlink = true;			// 是否收到链接请求 true:否  false:是
static DEVICE_CONFIG_T s_device_config;
static BOOLEAN s_is_ota = false;            /* OTA升级标志  */
static INT16U  s_com_cnt_link;              /*计数连接的时间 */


/**********************************************************************************
**  函数名称:  GZ_TestReq_Hdl
**  功能描述:  工装测试请求协议处理
**  输入参数
            :  userdata
            :  userdatalen
**  返回参数:  None
**********************************************************************************/
void GZ_TestReq_Hdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen)
{
    if (datalen != 1) return;
	//debug_printf("GZ_TestReq_Hdl type:%d\r\n",data[0]);
    switch(data[0]) {
        case 0x01:                            // 充电检测
            YX_Charge_Test_Hdl();
            break;
        case 0x02:                            // 放电检测
            YX_Discharge_Test_Hdl();
            break;
        case 0x03:                            // 气压传感器检测
            break;
        case 0x04:                            // 备电内阻检测
            YX_IntRes_Test_Hdl();
            break;
        case 0x05:                            // 双向检测
            YX_WkUpIO_Test_Hdl();
            break;
	   case 0x06:                            // 检测通知
            YX_GZ_Test_Hdl();
            break;            
        default:
            break;
    }
}

/*******************************************************************************
 ** 函数名:     mmi_com_send_tmrproc
 ** 函数描述:   定时扫描入口函数，负责定时1秒上电链接和定时10秒链路维护
 ** 参数:       [in]  timer:             定时器
 **             [in]  pdata:             定时器私有数据指针
 ** 返回:       无
 ******************************************************************************/
static void com_send_tmrproc(void* pdata)
{
	INT8U temp[2];
    INT8U bcall_status, ecall_status;

	if (--s_sendlink_cnt) {
		if ((s_com_linked == true)&&(s_sendlink_cnt % 3 == 0)) {

		}
		return;
	}

    #if DEBUG_COM_LINK > 0
    debug_printf("OTA升级标志: %d\r\n", YX_COM_IsOTA());
    #endif

	if (s_com_linked == false) {
		s_sendlink_cnt = COM_PERIOD_SENDLINK;
		YX_COM_DirSend( LINK_REQ, NULL, 0);
		#if DEBUG_COM_LINK > 0
		    debug_printf("正在发送上电请求\r\n");
            //debug_printf(YX_GetVersion());
		#endif
		s_com_cnt_nolink++;

		/* 判断链接不上若超过60秒，重启设备 */
        bcall_status = bal_input_ReadSensorFilterStatus(TYPE_BCALL);
        ecall_status = bal_input_ReadSensorFilterStatus(TYPE_ECALL);
        #if DEBUG_COM_LINK > 0
		debug_printf("bcall_status:%d ecall_status:%d\r\n", bcall_status, ecall_status);
        #endif

        if((bcall_status == true) && (ecall_status == true)) { /* bcall和ecall为高电平，判断为升级程序，不复位模块 */
            return;
        } else {
            if (YX_COM_IsOTA()) {
                if ((s_com_cnt_nolink >= OTA_COM_LINK)) {
    				s_com_cnt_nolink = 0;
    	            #if DEBUG_COM_LINK > 0
                    debug_printf("<链路30分钟重启>\r\n");
    	            #endif
                    YX_Power_ReStartSimcomPwr();
                    YX_COM_CleanOTA(); /* 清除OTA标志 */
    			}
            } else {
        		if(s_firstlink == false){
        			if ((s_com_cnt_nolink >= COM_TIMES_JUDGERESTART)) {
        				s_com_cnt_nolink = 0;
        	            #if DEBUG_COM_LINK > 0
                            debug_printf("<链路1分钟重启>\r\n");
        	            #endif
        	    		YX_Power_ReStartSimcomPwr();
        			}
        		} else {
        			if ((s_com_cnt_nolink >= FIRST_COM_LINK)) {
        				s_com_cnt_nolink = 0;
        	            #if DEBUG_COM_LINK > 0
                            debug_printf("链路3分钟<重启>\r\n");
        	            #endif
        	    		YX_Power_ReStartSimcomPwr();
        			}
        		}
            }
        }
        s_com_cnt_link = 0;

	} else {
        
        s_com_cnt_link++;
	    if (s_com_cnt_link > 3) {   /* (30 / COM_PERIOD_SENDQUREY) */
            /* 链路连上30秒后再清除OTA标志，防止主机一设置完ota标志还未进入ota，链路还存在，导致标志马上被清除 */
            if (YX_COM_IsOTA()) {
	            YX_COM_CleanOTA();
            }

	    }

		s_sendlink_cnt = COM_PERIOD_SENDQUREY;
		s_com_cnt_nolink = 0;
		temp[0] = 0x00;
		temp[1] = 20;
		YX_COM_DirSend( BEAT_REQ, temp, 2);
		#if DEBUG_COM_LINK > 0
			debug_printf("正在发送心跳连接请求\r\n");
		#endif

		if (s_com_overtime++ >= COM_PERIOD_LINKMAX) { /* 超过30秒链路断开 */
			s_com_linked = false;
		}
	}
}

/*******************************************************************************
 ** 函数名:     HdlMsg_LINK_REQ_ACK
 ** 函数描述:   上电连接请求
 ** 参数:       [in]data:数据指针
 **             [in]datalen:数据长度
 ** 返回:       无
 ******************************************************************************/
void LinkRequstAck_Hdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen)
{
	s_com_linked = true;
	s_firstlink = false;
    s_com_overtime = 0;
}

/*******************************************************************************
 ** 函数名:     HdlMsg_BEAT_REQ_ACK
 ** 函数描述:   mmi_com链路维护应答
 ** 参数:       [in]data:数据指针
 **             [in]datalen:数据长度
 ** 返回:       无
 ******************************************************************************/
void BeatRequstAck_Hdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen)
{
	s_com_overtime = 0;
}

/*******************************************************************************
 ** 函数名:     HdlMsg_VERSION_REQ
 ** 函数描述:   版本查询协议处理
 ** 参数:       [in]data:数据指针
 **             [in]datalen:数据长度
 ** 返回:       无
 ******************************************************************************/
void VersionNumberReqHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen)
{
    char *get_vernum;
    INT8U ack[50] = {0};
	ack[0] = data[0];
	if (ack[0] == 0x01) {
	    get_vernum = YX_GetVersion();
	} else if (ack[0] == 0x02) {
		get_vernum = YX_GetClientVersion();
	}
	ack[1] = strlen(get_vernum);
	memcpy(&ack[2],get_vernum,strlen(get_vernum));
	YX_COM_DirSend( VERSION_REQ_ACK, ack, strlen(get_vernum)+2);
}
/**************************************************************************************************
**  函数名称:  DeviceConfigurationQueryHdl
**  功能描述:  设备功能查询处理函数--车台发指令查询设备具有的功能
**  输入参数:
**  返回参数:
**************************************************************************************************/
void DeviceConfigurationQueryHdl(INT8U mancode, INT8U command, INT8U *data, INT16U datalen)
{
    INT8U temp[4];

    s_device_config.config_tags.printer              = 0;
    s_device_config.config_tags.cardreader           = 1;
    s_device_config.config_tags.CAN_Comm             = 1;
    s_device_config.config_tags.oilcheck             = 0;
    s_device_config.config_tags.usarttransmit        = 1;
    s_device_config.config_tags.udisk                = 0;
    s_device_config.config_tags.update_udisk         = 0;
    s_device_config.config_tags.update_wireless      = 1;

    s_device_config.config_tags.signergather         = 0;
    s_device_config.config_tags.turnonside           = 0;
    s_device_config.config_tags.hitcheck             = 0;
    s_device_config.config_tags.tts                  = 0;
    s_device_config.config_tags.mileagestat          = 1;
    s_device_config.config_tags.rfid                 = 0;
    s_device_config.config_tags.alarm                = 0;

    bal_longtochar(temp, s_device_config.configw);
    YX_COM_DirSend( FUNC_QUERY_ACK, temp, 4);
}
#if 0

/**************************************************************************************************
**  函数名称:  ReqChgMainComBaud
**  功能描述:  向车台请求需要更改主串口波特率
**  输入参数:  baud : 0x01~0x07
**  返回参数:  None
**************************************************************************************************/
BOOLEAN ReqChgMainComBaud(INT8U baud, FUNC_CHGBAUD_TYPE_E func)
{

    INT8U baudtype;

    baudtype = baud;
    s_func_chgbaud = func;
    APP_ASSERT((baudtype >= 0x01) && (baudtype <= 0x07));
    if (baudtype != RESUME_BAUD) {  /* 请求更改的波特率不是默认的波特率，则需加以下判断 */
        if (FALSE == s_chgbauded) { /* 只有波特率处在默认波特率才允许向上请求，否则不允许向上请求 */
            YX_COM_DirSend( CHGMCOMBAUD_REQ, &baudtype, 1);
            return TRUE;
        } else {
            return FALSE;
        }
    } else {
        YX_COM_DirSend( CHGMCOMBAUD_REQ, &baudtype, 1);
        return TRUE;
    }

}
#endif

/**************************************************************************************************
**  函数名称:  Relink_RequestHdl
**  功能描述:  重新连接处理函
**  输入参数:  version      :
**          :  command      :
**          :  userdata     :
**          :  userdatalen  :
**  返回参数:  None
**************************************************************************************************/
void Relink_RequestHdl(INT8U mancode, INT8U command, INT8U *data, INT16U datalen)
{

    if (TRUE == s_com_linked) {
        s_com_linked = FALSE;
        s_sendlink_cnt = COM_PERIOD_SENDLINK;/*定时器重新配置上电1s*/
    }
    #if DEBUG_COM_LINK > 0
        debug_printf("收到链路重连请求\r\n");
    #endif
    YX_COM_DirSend( RELINK_REQ_ACK, 0, 0);

}

/**************************************************************************************************
**  函数名称:  ReqChgMainComBaudAck_Hdl
**  功能描述:  请求改变主串口波特率后车台给应答指令后改变主串口波特率，置链接标志为FALSE
**  输入参数:  version     :  版本号
**          :  command     :  命令号
**          :  userdata    :  命令数据
**          :  userdatalen :  数据长度
**  返回参数:  None
**************************************************************************************************/
void ReqChgMainComBaudAck_Hdl(INT8U mancode, INT8U command,  INT8U *data, INT16U datalen)
{

    INT8U baudvalue;
    INT8U allowchg;
    INT32U s_com_baudvalue;


    baudvalue = data[0];
    allowchg  = data[1];

    if(0x02 == allowchg) {
        /* 不允许更改主串口波特率 */
        return;
    } else if(0x01 == allowchg) {
        #if DEBUG_COM_LINK > 0
          debug_printf("change baud baudvalue = %d\r\n", baudvalue);
        #endif
        /* 允许改变波特率 */
        switch(baudvalue) {
            case 01:
                s_com_baudvalue = BAUD_2400;
                break;
            case 02:
                s_com_baudvalue = BAUD_4800;
                break;
            case 03:
                s_com_baudvalue = BAUD_9600;
                break;
            case 04:
                s_com_baudvalue = BAUD_19200;
                break;
            case 05:
                s_com_baudvalue = BAUD_38400;
                break;
            case 06:
                s_com_baudvalue = BAUD_57600;
                break;
            case 07:
                s_com_baudvalue = BAUD_115200;
                break;
            default :
                break;
        }
        if (TRUE == s_com_linked) {
            s_com_linked = FALSE;
        }
        PORT_InitUart(MAIN_COM, s_com_baudvalue, MAIN_COM_RBUF_SIZE , MAIN_COM_TBUF_SIZE);

    }
}

/*******************************************************************************
 ** 函数名:    YX_COM_Init
 ** 函数描述:   mmi_com驱动模块初始化
 ** 参数:       无
 ** 返回:       无
 ******************************************************************************/
void YX_COM_Init(void)
{
	INT32U s_com_baudvalue;
	#if 1
	s_com_baudvalue = INIT_BAUD_921600;
	#else
	s_com_baudvalue = DEFAULT_BAUD_115200;
	#endif
	PORT_InitUart(MAIN_COM, s_com_baudvalue, MAIN_COM_RBUF_SIZE , MAIN_COM_TBUF_SIZE);
	YX_ComRecv_Init();
	YX_ComSend_Init();

	s_com_overtime = 0;
	s_com_linked = false;
	s_sendlink_cnt = COM_PERIOD_SENDLINK;

    #if DEBUG_COM_LINK > 0
        s_sendlink_tmrid = OS_InstallTmr(TSK_ID_OPT, 0, com_send_tmrproc);
        OS_StartTmr(s_sendlink_tmrid, SECOND, 1);
    #else
        s_sendlink_tmrid = OS_InstallTmr(TSK_ID_OPT, 0, com_send_tmrproc);
        OS_StartTmr(s_sendlink_tmrid, SECOND, 1);
    #endif

    //bal_output_RemovePermnentPort(PORT_GPSLED);
    //bal_output_CtlPort(PORT_GPSLED, 1);
    bal_output_InstallPermnentPort(PORT_GPSLED, 0, 10);
    //bal_output_RemovePermnentPort(PORT_NETLED);
    //bal_output_CtlPort(PORT_NETLED, 1);
    //bal_output_RemovePermnentPort(PORT_CANLED);
    //bal_output_CtlPort(PORT_CANLED, 1);
    //bal_output_InstallPermnentPort(PORT_CANLED, 60, 60);
}

/*******************************************************************************
 ** 函数名:    YX_COM_Islink
 ** 函数描述:   获取链接状态
 ** 参数:       无
 ** 返回:       true:  已连接   / false: 链路异常
 ******************************************************************************/
BOOLEAN YX_COM_Islink(void)
{
	return s_com_linked;
}

/*******************************************************************************
 ** 函数名:    YX_COM_Cleanlink
 ** 函数描述:   断开心跳
 ** 参数:      无
 ** 返回:      无
 ******************************************************************************/
void YX_COM_Cleanlink(void)
{
	 s_com_linked = false;
}

/*******************************************************************************
 ** 函数名:    YX_COM_SetOTA
 ** 函数描述:  设置OTA升级标志
 ** 参数:      无
 ** 返回:      无
 ******************************************************************************/
void YX_COM_SetOTA(void)
{
	s_is_ota = true;
}

/*******************************************************************************
 ** 函数名:    YX_COM_CleanOTA
 ** 函数描述:  清除OTA升级标志
 ** 参数:      无
 ** 返回:      无
 ******************************************************************************/
void YX_COM_CleanOTA(void)
{
	s_is_ota = false;
}

/*******************************************************************************
 ** 函数名:    YX_COM_IsOTA
 ** 函数描述:  获取OTA状态
 ** 参数:      无
 ** 返回:      true:  OTA进行中   / false:
 ******************************************************************************/
BOOLEAN YX_COM_IsOTA(void)
{
	return s_is_ota;
}

