/*
********************************************************************************
** 文件名:     yx_protocal_type.h
** 版权所有:   (c) 2017 厦门雅迅网络股份有限公司
** 文件描述:   协议功能号文件
** 创建人：        谢金成，2017.5.18
********************************************************************************
*/
#ifndef  _YX_PROTOCAL_TYPE_H
#define  _YX_PROTOCAL_TYPE_H

/* Exported constants --------------------------------------------------------*/

//#define  MAN_CODE        0x01                    /* 厂商编号 */

#define  DEVICETYPE      0x18                    /* 福田TBOX外设编码0x18 */

/******************************************************************************/
/*                           定义功能命令编码                                                                                                                                       */
/******************************************************************************/
typedef enum {
    /* common */ 
    LINK_REQ                     = 0x01,             /* 上电指示请求 (UP) */
    LINK_REQ_ACK                 = 0x01,             /* 上电指示请求应答 (DOWN) */
    BEAT_REQ                     = 0x02,             /* 链路维护请求 (UP) */
    BEAT_REQ_ACK                 = 0x02,             /* 链路维护请求应答 (DOWN)*/
    POWER_CONTROL_REQ_ACK        = 0x03,             /* 电源控制请求应答 (UP) */
    POWER_CONTROL_REQ            = 0x03,             /* 电源控制请求 (DOWN) */
    VERSION_REQ_ACK              = 0x04,             /* 版本查询请求应答 (UP) */
    VERSION_REQ                  = 0x04,             /* 版本查询请求(DOWN) */  
    RELINK_REQ_ACK               = 0x05,             /* 重新连接请求应答 (UP) */
    RELINK_REQ                   = 0x05,             /* 重新连接请求 (DOWN) */  
    FUNC_QUERY_ACK               = 0x06,             /* 设备功能查询请求应答 (UP)*/
    FUNC_QUERY                   = 0x06,             /* 设备功能查询请求 (DOWN) */
    RESET_REQ_ACK                = 0x07,             /* 复位设备请求应答(UP) */
    RESET_REQ                    = 0x07,             /* 复位设备请求(DOWN) */
	  RTC_SYNC_REQ								 = 0x08,						 /* rtc时钟校准请求 (UP)*/	
		RTC_SYNC_REQ_ACK						 = 0x08,						 /* rtc时钟校准应答 (DOWN)*/

	
    GET_HOSTSTATUS_REQ           = 0X40,             /*获取主机状态请求*/
    GET_HOSTSTATUS_REQ_ACK       = 0X40,             /*获取主机状态请求应答*/
    RETIM_STATUS_REPORT          = 0x41,             /* 调度屏实时状态上报 (UP)  */ 
    RETIM_STATUS_REPORT_AKC      = 0x41,             /* 对调度屏实时状态上报的应答 (DOWN)  */
    CHGMCOMBAUD_REQ              = 0x43,             /* 请求需要改变主串口波特率(UP) */
    CHGMCOMBAUD_REQ_ACK          = 0x43,             /* 请求需要改变主串口波特率应答(DOWN) */ 
    
    EQUIPMENT_PARA_REQ           = 0x46,             /* 查询主机通用参数请求(UP) */
    EQUIPMENT_PARA_REQ_ACK       = 0x46,             /* 查询主机通用参数应答(DOWN) */
    EQUIPMENT_PARA_SET           = 0x47,             /* 设置主机通用参数请求(UP) */
    EQUIPMENT_PARA_SET_ACK       = 0x47,             /* 设置主机通用参数应答(DOWN) */
    CLEAR_DATA_REQ               = 0x48,                /* 主机恢复调度屏数据请求(DOWN) */
    CLEAR_DATA_REQ_ACK           = 0x48,                /* 主机恢复调度屏数据应答(UP) */  
    
    /* 信号检测 */
    SIGFILTER_PARA_CONF          = 0x86,             /* 信号滤波参数设置(DOWN) */
    SIGFILTER_PARA_CONF_ACK      = 0x86,             /* 对信号滤波参数设置指令的应答(UP) */
    SIG_CHG_INFO_ONFF_REQ        = 0x87,             /* IO输出控制请求 (DOWN) */
    SIG_CHG_INFO_ONFF_REQ_ACK    = 0x87,             /* IO输出控制请求应答 (UP)*/
    SIG_CHG_INFO                 = 0x88,             /* 信号状态改变上报通知(UP) */
    SIG_CHG_INFO_ACK             = 0x88,             /* 对信号状态改变上报通知的应答(DOWN) */
    
    //IO_REPORT_REQ                = 0x88,             /* IO输入检测状态上报 (UP) */
    //IO_REPORT_REQ_ACK            = 0x88,             /* IO输入检测状态上报应答 (DOWN) */ 

    
    /* 数据透传 */
    DATA_DELIVER_REQ             = 0xA0,             /* 数据透传上行请求(UP) */
    DATA_DELIVER_REQ_ACK         = 0xA0,             /* 数据透传上行请求的应答(DOWN) */
    DATA_DELIVER_DOWN_ACK        = 0xA1,             /* 数据透传下行请求的应答(UP) */
    DATA_DELIVER_DOWN            = 0xA1,             /* 数据透传下行请求(DOWN) */
    
    /* 无线升级 */
    UPDATE_START_REQ             = 0xA4,             /*无线升级主程序开始请求(DOWN)*/
    UPDATE_START_REQ_ACK         = 0xA4,             /*无线升级主程序开始应答(UP)*/
    UPDATE_DATA_RECV             = 0xA5,             /*无线升级主程序数据传输请求(DOWN)*/
    UPDATE_DATA_RECV_ACK         = 0xA5,             /*对无线升级主程序数据传输请求的应答(UP)*/
    
    PHYCOM_PARA_CONFIG_REQ_ACK   = 0xC3,             /*对物理扩展串口参数设置请求的应答(UP)*/
    PHYCOM_PARA_CONFIG_REQ       = 0xC3,             /*物理扩展串口参数设置请求(DOWN)*/
    PHYCOM_PARA_QUERY_ACK        = 0xC4,             /*对物理扩展串口参数查询请求的应答(UP)*/
    PHYCOM_PARA_QUERY            = 0xC4,             /*物理扩展串口参数查询请求(DOWN)*/
    PHYCOM_POWER_CONTROL_REQ_ACK = 0xC5,             /*对物理扩展串口电源控制请求的应答(UP)*/
    PHYCOM_POWER_CONTROL_REQ     = 0xC5,             /*物理扩展串口电源控制请求(DOWN)*/

    BUS_TYPE_SET_ACK             = 0xD0,             /* 通信总线模式设置应答 (UP) */
    BUS_TYPE_SET                 = 0xD0,             /* 通信总线模式设置 (DOWN) */
    BUS_ONOFF_CTRL_ACK           = 0xD1,             /* 通信总线功能开关控制请求应答 (UP) */
    BUS_ONOFF_CTRL               = 0xD1,             /* 通信总线功能开关控制请求 (DOWN) */
    
    PARA_SET_CAN                 = 0xD3,             /* CAN通信参数设置(DOWN) */
    PARA_SET_CAN_ACK             = 0xD3,             /* CAN通信参数设置应答(UP) */
    WORKMODE_SET_CAN_ACK         = 0xD5,             /* CAN工作模式设置应答(UP) */
    WORKMODE_SET_CAN             = 0xD5,             /* CAN工作模式设置(DOWN) */
    WORKMODE_QUERY_CAN_ACK       = 0xD6,             /* CAN工作模式查询应答(UP) */
    WORKMODE_QUERY_CAN           = 0xD6,             /* CAN工作模式查询(DOWN) */
    FILTER_SET_CAN_ACK           = 0xD7,             /* CAN滤波ID配置应答(UP) */
    FILTER_SET_CAN               = 0xD7,             /* CAN滤波ID配置(DOWN) */
    FILTER_QUERY_CAN_ACK         = 0xD8,             /* CAN滤波ID配置查询应答(UP) */
    FILTER_QUERY_CAN             = 0xD8,             /* CAN滤波ID配置查询(DOWN) */
    DATA_TRANSF_CAN              = 0xD9,             /* 主机数据通过CAN转发(DOWN) */
    DATA_TRANSF_CAN_ACK          = 0xD9,             /* 主机数据通过CAN转发应答(UP) */ 
    DATA_LUCIDLY_CAN             = 0xDA,             /* 外设CAN数据透传(UP) */
    DATA_LUCIDLY_CAN_ACK         = 0xDA,             /* 外设CAN数据透传应答(DOWN) */
    DATA_REPORT_CAN              = 0xDB,             /* 外设CAN数据上报(UP) */
    DATA_REPORT_CAN_ACK          = 0xDB,             /* 外设CAN数据上报应答(DOWN) */
    GET_CAN_DATA_REQ_ACK         = 0xDC,             /* 主机获取CAN数据请求应答(UP) */
    GET_CAN_DATA_REQ             = 0xDC,             /* 主机获取CAM数据请求(DOWN) */
    FILTER_SCREEN_CAN            = 0xDE,             /* CAN滤波器组配置(DOWN) */
    FILTER_SCREEN_CAN_ACK        = 0xDE,             /* CAN滤波器组配置应答(UP) */
    DATA_SEQ_TRANSF_CAN_ACK      = 0xDF,             /* 主机发送CAN数据（扩展）请求应答(UP) */
    DATA_SEQ_TRANSF_CAN          = 0xDF,             /* 主机发送CAN数据（扩展）请求(DOWN) */

    GSENSOR_BD_REQ_ACK           = 0xE0,             /* g-sensor标定请求应答(UP) */
    GSENSOR_BD_REQ               = 0xE0,             /* g-sensor标定请求(DOWN) */
    CRASH_SET_PARA_ACK           = 0xE1,             /* 对碰撞检测模参数设置应答 */
    CRASH_SET_PARA               = 0XE1,             /* 对碰撞检测模参数设置请求 */
    CRASH_REPORT                 = 0xE2,             /* 对碰撞检测模数据上报 */
    CRASH_REPORT_ACK             = 0xE2,             /* 对碰撞检测模数据上报应答 */

    
    AD_SET_REQ_ACK               = 0xE3,             /* AD转换功能设置请求应答 (UP) */
    AD_SET_REQ                   = 0xE3,             /* AD转换功能设置请求 (DOWN) */
    ADC_DATA_GET                 = 0xE4,             /* 车台主动获取AD数据(DOWN) */
    ADC_DATA_GET_ACK             = 0xE4,             /* 车台主动获取AD数据(UP) */
    AD_REPORT_REQ                = 0xE5,             /* AD转换结果上报请求 (UP) */
    AD_REPORT_REQ_ACK            = 0xE5,             /* AD转换结果上报应答 (DOWN) */
    
    GZ_TEST_REQ                  = 0XE6,             /*工装测试请求*/
    GZ_TEST_REQ_ACK              = 0XE6,             /*工装测试请求应答*/


    UPDATA_REPORT_CAN            = 0x23,             /* CAN升级数据上报(UP) */
    UPDATA_REPORT_CAN_ACK        = 0xa3,             /* CAN升级数据上报应答(DOWN) */

    CLIENT_FUNCTION_UP_REQ       = 0xFC,             /* 客户特殊功能请求(UP) */
    CLIENT_FUNCTION_UP_REQ_ACK   = 0xFC,             /* 客户特殊功能请求应答(DOWN) */

    CLIENT_FUNCTION_DOWN_REQ     = 0xFD,             /* 客户特殊功能下行请求(DOWN) */
    CLIENT_FUNCTION_DOWN_REQ_ACK = 0xFD,             /* 客户特殊功能下行请求应答(UP) */

    

}PROTOCOL_COMMAND_E;

#endif/* _YX_PROTOCAL_TYPE_H */
/************************ (C) COPYRIGHT 2010 XIAMEN YAXON.LTD *******************END OF FILE******/

