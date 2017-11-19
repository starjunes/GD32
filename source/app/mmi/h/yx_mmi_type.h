/********************************************************************************
**
** 文件名:     yx_mmi_ptype.h
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   MMI协议命令宏文件
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2014/05/02 | 叶德焰 |  创建第一版本
*********************************************************************************/
#ifndef H_YX_MMI_PTYPE
#define H_YX_MMI_PTYPE          1


/* 命令处理结构体 */
typedef struct {
    INT8U cmd;
    void  (*entryproc)(INT8U cmd, INT8U *data, INT16U datalen);
} FUNCENTRY_MMI_T;

#define PE_TYPE_YXMMI      0x19      /* 外设编号 */
#define COM_VER_MMI        0x01      /* 通信协议版本 */

#define PE_ACK_MMI         0x01
#define PE_NAK_MMI         0x02

/* 复位类型 */
typedef enum {
    MMI_RESET_EVENT_NULL = 0,
    MMI_RESET_EVENT_NORMAL,         /* 常规复位,如主动复位 */
    MMI_RESET_EVENT_ERROR,          /* 异常复位，如出现ASSERT等异常问题 */
    MMI_RESET_EVENT_WDG,            /* 看门狗异常复位 */
    MMI_RESET_EVENT_SLEEP,          /* 进入省电，关闭主机 */
    MMI_RESET_EVENT_UPDATE,         /* 程序升级复位，如启动外设程序升级，外设程序升级完毕 */
    MMI_RESET_EVENT_POWERDOWN,      /* 主电源断电 */
    MMI_RESET_EVENT_POWERUP,        /* 主电源上电 利用新协议0x0A上报*/
    MMI_RESET_EVENT_WAKEUP,         /* 休眠唤醒   利用新协议0x0A上报*/
    MMI_RESET_EVENT_MAX
} MMI_RESET_EVENT_E;

/*************************************************************************************************/
/*                           定义功能命令编码                                                    */
/*************************************************************************************************/
typedef enum {
    /* 通用管理协议 */ 
    UP_PE_CMD_LINK_REQ                  = 0x01,                /* 上电指示请求 (UP) */
    DN_PE_ACK_LINK_REQ                  = 0x01,                /* 上电指示请求应答 (DOWN) */
    
    UP_PE_CMD_BEAT_REQ                  = 0x02,                /* 链路维护请求 (UP) */
    DN_PE_ACK_BEAT_REQ                  = 0x02,                /* 链路维护请求应答 (DOWN)*/
    
    DN_PE_CMD_VERSION_REQ               = 0x04,                /* 版本查询请求(DOWN) */
    UP_PE_ACK_VERSION_REQ               = 0x04,                /* 版本查询请求应答 (UP) */ 
    
    DN_PE_CMD_RESET_INFORM              = 0x05,                /* 主机即将复位通知请求 (DOWN) */
    UP_PE_ACK_RESET_INFORM              = 0x05,                /* 主机即将复位通知请求应答 (UP) */
    
    UP_PE_CMD_PE_RESET_INFORM           = 0x06,                /* 外设通知主机，外设即将复位通知请求 (UP) */
    DN_PE_ACK_PE_RESET_INFORM           = 0x06,                /* 外设通知主机，外设即将复位通知请求 (DOWN)*/
    
    DN_PE_CMD_RESTART_REQ               = 0x07,                /* 复位重启请求 (DOWN) */
    UP_PE_ACK_RESTART_REQ               = 0x07,                /* 复位重启请求应答 (UP) */
    
    UP_PE_CMD_HOST_RESET_INFORM         = 0x08,                /* 外设通知主机，外设即将关闭或重启主机通知请求 (UP) */
    DN_PE_ACK_HOST_RESET_INFORM         = 0x08,                /* 外设通知主机，外设即将关闭或重启主机通知请求应答 (DOWN)*/
    
    DN_PE_CMD_GET_RESET_REC             = 0x09,                /* 复位记录查询请求 (DOWN) */
    UP_PE_ACK_GET_RESET_REC             = 0x09,                /* 复位记录查询请求应答 (UP) */

    UP_PE_CMD_HOST_RESET_INFORM_NEW     = 0x0A,                /* 外设通知主机，外设即将关闭或重启主机通知请求 (UP) (新 增加休眠唤醒和上电状态上报)*/
    DN_PE_ACK_HOST_RESET_INFORM_NEW     = 0x0A,                /* 外设通知主机，外设即将关闭或重启主机通知请求应答 (DOWN)(新 增加休眠唤醒和上电状态上报)*/
 
    
    /* 基础业务协议 */
    DN_PE_CMD_CTL_GPIO                  = 0x41,                /* 控制GPIO输出 (DOWN) */
    UP_PE_ACK_CTL_GPIO                  = 0x41,                /* 控制GPIO输出应答 (UP) */
    
    DN_PE_CMD_CLEAR_WATCHDOG            = 0x42,                /* 看门狗喂狗 (DOWN) */
    UP_PE_ACK_CLEAR_WATCHDOG            = 0x42,                /* 看门狗喂狗应答 (UP) */
    
    DN_PE_CMD_SET_REALCLOCK             = 0x43,                /* 设置实时时钟 (DOWN) */
    UP_PE_ACK_SET_REALCLOCK             = 0x43,                /* 设置实时时钟应答 (UP) */
    
    UP_PE_CMD_REPORT_REALCLOCK          = 0x44,                /* 上报实时时钟 (DOWN) */
    DN_PE_ACK_REPORT_REALCLOCK          = 0x44,                /* 上报实时时钟应答 (UP) */
    
    DN_PE_CMD_READ_REALCLOCK            = 0x45,                /* 读取实时时钟 (DOWN) */
    UP_PE_ACK_READ_REALCLOCK            = 0x45,                /* 读取实时时钟应答 (UP) */
    
    DN_PE_CMD_HOST_SET_PARA             = 0x46,                /* 设置通用参数 (DOWN) */
    UP_PE_ACK_HOST_SET_PARA             = 0x46,                /* 设置通用参数应答 (UP) */
    
    UP_PE_CMD_SLAVE_GET_PARA            = 0x47,                /* 从机查询通用参数 */
    DN_PE_ACK_SLAVE_GET_PARA            = 0x47,                /* 从机查询通用参数应答 */
    
    DN_PE_CMD_CTL_FUNCTION              = 0x48,                /* 功能控制 */
    UP_PE_ACK_CTL_FUNCTION              = 0x48,                /* 功能控制应答 */
    
    DN_PE_CMD_HOST_GET_PARA             = 0x49,                /* 主机查询通用参数 */
    UP_PE_ACK_HOST_GET_PARA             = 0x49,                /* 主机查询通用参数应答 */
    
    /* 行驶记录仪业务协议 */
    DN_PE_CMD_GET_ICCARD_INFO           = 0x61,                /* 读卡请求(DOWN) */
    UP_PE_ACK_GET_ICCARD_INFO           = 0x61,                /* 读卡请求应答 (UP) */
    
    UP_PE_CMD_REPORT_ICCARD_INFO        = 0x62,                /* 主动上报IC卡信息(UP) */
    DN_PE_ACK_REPORT_ICCARD_INFO        = 0x62,                /* 主动上报IC卡信息应答(DOWN) */
    
    UP_PE_CMD_REPORT_ICCARD_DATA        = 0x63,                /* 主动上报IC卡原始数据(UP) */
    DN_PE_ACK_REPORT_ICCARD_DATA        = 0x63,                /* 主动上报IC卡原始数据应答(DOWN) */
    
    DN_PE_CMD_WRITE_ICCARD              = 0x64,                /* 写IC卡数据 (DOWN) */
    UP_PE_ACK_WRITE_ICCARD              = 0x64,                /* 写IC卡数据应答 (UP) */
    
    /* 传感器信号协议 */
    UP_PE_CMD_REPORT_SENSOR_STATUS      = 0x71,                /* 主动上报GPIO传感器状态(UP) */
    DN_PE_ACK_REPORT_SENSOR_STATUS      = 0x71,                /* 主动上报GPIO传感器状态应答(DOWN) */
    DN_PE_CMD_SET_SENSOR_FILTER         = 0x72,                /* 设置GPIO滤波参数(DOWN) */
    UP_PE_ACK_SET_SENSOR_FILTER         = 0x72,                /* 设置GPIO滤波参数应答 (UP) */
    DN_PE_CMD_SET_SENSOR_PARA           = 0x73,                /* 设置GPIO传感器参数(DOWN) */
    UP_PE_ACK_SET_SENSOR_PARA           = 0x73,                /* 设置GPIO传感器参数应答 (UP) */
    
    UP_PE_CMD_REPORT_ODOPULSE           = 0x74,                /* 主动上报里程脉冲(UP) */
    DN_PE_ACK_REPORT_ODOPULSE           = 0x74,                /* 主动上报里程脉冲应答(DOWN) */
    DN_PE_CMD_SET_ODOPULSE_PARA         = 0x75,                /* 设置里程脉冲参数(DOWN) */
    UP_PE_ACK_SET_ODOPULSE_PARA         = 0x75,                /* 设置里程脉冲参数应答 (UP) */
    
    DN_PE_CMD_SET_AD_PARA               = 0x78,                /* 设置AD采集参数(DOWN) */
    UP_PE_ACK_SET_AD_PARA               = 0x78,                /* 设置AD采集参数应答 (UP) */
    DN_PE_CMD_GET_AD                    = 0x79,                /* 获取AD值(DOWN) */
    UP_PE_ACK_GET_AD                    = 0x79,                /* 获取AD值应答 (UP) */
    UP_PE_CMD_REPORT_AD                 = 0x7A,                /* 主动上报AD值(UP) */
    DN_PE_ACK_REPORT_AD                 = 0x7A,                /* 主动上报AD值应答(DOWN) */
    
    UP_PE_CMD_REPORT_KEY                = 0x7B,                /* 主动上报按键值(UP) */
    DN_PE_ACK_REPORT_KEY                = 0x7B,                /* 主动上报按键值应答(DOWN) */
    
    /* 扩展串口业务 */
    DN_PE_CMD_SET_UART_PARA             = 0x81,                /* 设置扩展串口参数 (DOWN) */
    UP_PE_ACK_SET_UART_PARA             = 0x81,                /* 设置扩展串口参数应答 (UP) */
    
    DN_PE_CMD_GET_UART_PARA             = 0x82,                /* 获取扩展串口参数 (DOWN) */
    UP_PE_ACK_GET_UART_PARA             = 0x82,                /* 获取扩展串口参数应答 (UP) */
    
    DN_PE_CMD_CTL_UART_POWER            = 0x83,                /* 控制扩展串口电源 (DOWN) */
    UP_PE_ACK_CTL_UART_POWER            = 0x83,                /* 控制扩展串口电源应答 (UP) */
    
    UP_PE_CMD_UART_DATA_SEND            = 0x84,                 /* UART数据上行透传请求(UP) */
    DN_PE_ACK_UART_DATA_SEND            = 0x84,                 /* UART数据上行透传请求应答(DOWN) */
    DN_PE_CMD_UART_DATA_SEND            = 0x85,                 /* UART数据下行透传请求 (DOWN) */
    UP_PE_ACK_UART_DATA_SEND            = 0x85,                 /* UART数据下行透传请求应答(UP) */
    
    
    UP_PE_CMD_RETIM_STATUS_REPORT       = 0x41,                /* 调度屏实时状态上报 (UP)  */ 
    DN_PE_ACK_RETIM_STATUS_REPORT       = 0x41,                /* 对调度屏实时状态上报的应答 (DOWN)  */
    

    UP_PE_CMD_EQUIPMENT_STATUS          = 0x44,                /* 主机自检(UP) */
    DN_PE_ACK_EQUIPMENT_STATUS          = 0x44,                /* 主机自检应答(DOWN) */

    DN_PE_CMD_EQUIPMENT_CLRDB_REQ       = 0x48,                /* 恢复的数据区请求(DOWN) */
    UP_PE_ACK_EQUIPMENT_CLRDB_REQ       = 0x48,                /* 恢复的数据区应答(UP) */
    UP_PE_CMD_EQUIPMENT_STATUS_GET      = 0x49,                /* 开机请求主机状态请求 */
    DN_PE_ACK_EQUIPMENT_STATUS_GET      = 0x49,                /* 开机请求主机状态应答 */

    DN_PE_CMD_EQUIPMENT_STATUS_CHANGE   = 0x4A,                /* 主机状态切换告知(DOWN) */
    UP_PE_ACK_EQUIPMENT_STATUS_CHANGE   = 0x4A,                /* 主机状态切换应答(UP) */

    DN_PE_CMD_START_OR_STOP_LOG         = 0x4B,                /* 开启或关闭日志请求(DOWN) */
    UP_PE_ACK_START_OR_STOP_LOG         = 0x4B,                /* 开启或关闭日志应答(UP) */



    /* 行驶记录 */
    DN_PE_CMD_DRIVER_LOGIN_STA          = 0x89,                 /* 司机登录状况告知(DOWN) */
    UP_PE_ACK_DRIVER_LOGIN_STA          = 0x89,                 /* 司机登录状况应答(UP) */
    UP_PE_CMD_DRIVER_LOGIN_REQ          = 0x8a,                 /* 司机刷卡结果告知(UP) */
    DN_PE_ACK_DRIVER_LOGIN_REQ          = 0x8a,                 /* 司机刷卡结果应答(DOWN) */
    UP_PE_CMD_CAR_CHECK_REQ             = 0x8b,                 /* 车辆特征系数查询、设置请求(UP) */
    DN_PE_ACK_CAR_CHECK_REQ             = 0x8b,                 /* 车辆特征系数查询、设置应答(DOWN) */
    UP_PE_CMD_SPEED_AVERAGE_REQ         = 0x8c,                 /* 15分钟平均速度(UP) */
    DN_PE_ACK_SPEED_AVERAGE_REQ         = 0x8c,                 /* 15分钟平均速度应答(DOWN) */
    UP_PE_CMD_DRIVE_RECORD_REQ          = 0x8d,                 /* 近两天连续驾驶记录请求(UP) */
    DN_PE_ACK_DRIVE_RECORD_REQ          = 0x8d,                 /* 近两天连续驾驶记录应答(DOWN) */
    UP_PE_CMD_DRIVER_LOGIN_STA_REQ      = 0x8e,                 /* 司机登录状况请求(UP) */
    DN_PE_ACK_DRIVER_LOGIN_STA_REQ      = 0x8e,                 /* 司机登录状况告知(DOWN) */
    
    DN_PE_CMD_COMBUS_MODE_SET           = 0xD0,                /* 通信总线模式设置请求(DOWN) */
    UP_PE_ACK_COMBUS_MODE_SET           = 0xD0,                /* 通信总线模式设置请求应答(UP) */
    DN_PE_CMD_COMBUS_ONOFF_SET          = 0xD1,                /* 通信总线开关设置请求(DOWN) */
    UP_PE_ACK_COMBUS_ONOFF_SET          = 0xD1,                /* 通信总线开关设置请求应答(UP) */
    DN_PE_CMD_COMBUS_RESET              = 0xD2,                /* 通信总线复位请求(DOWN) */
    UP_PE_ACK_COMBUS_RESET              = 0xD2,                /* 通信总线复位请求应答(UP) */


    UP_PE_CMD_TRANSMITION_UP            = 0xA0,                 /* 数据透传上行协议 */ 
    DN_PE_ACK_TRANSMITION_UP            = 0xA0,                 /* 数据透传上行应答 */ 
    UP_PE_ACK_TRANSMITION_DOWN          = 0xA1,                 /* 数据透传下行应答 */    
    DN_PE_CMD_TRANSMITION_DOWN          = 0xA1,                 /* 数据透传下行协议 */ 
    

    /* 无线下载 */
    DN_PE_CMD_WDOWN_REQ                 = 0xA4,                 /* 无线升级主程序开始请求(DOWN) */
    UP_PE_ACK_WDOWN_REQ                 = 0xA4,                 /* 无线升级主程序开始请求的应答 (UP) */
    DN_PE_CMD_WDOWN_DATA_SEND           = 0xA5,                 /* 无线升级主程序数据传输请求(DOWN) */
    UP_PE_ACK_WDOWN_DATA_SEND           = 0xA5,                 /* 无线升级主程序数据传输请求的应答 (UP) */


    DN_PE_CMD_RPEXT_PARA_SET            = 0xC3,                 /* 物理扩展串口参数设置请求(DOWN) */
    UP_PE_ACK_RPEXT_PARA_SET            = 0xC3,                 /* 物理扩展串口参数设置请求应答(UP) */
    DN_PE_CMD_PEXT_PARA_QUER           = 0xC4,                 /* 物理扩展串口参数查询请求(DOWN) */
    UP_PE_ACK_RPEXT_PARA_QUER           = 0xC4,                 /* 物理扩展串口参数查询请求应答(UP) */
    DN_PE_CMD_PEXT_POWER_SET           = 0xC5,                 /* 物理扩展串口电源设置请求(DOWN) */
    UP_PE_ACK_RPEXT_POWER_SET           = 0xC5,                 /* 物理扩展串口电源设置请求应答(UP) */
    

    //UP_PE_CMD_TRIG_WIRELESS_REQ         = 0xF9,                 /* 指示车台远程升级请求(UP) */
    //DN_PE_ACK_TRIG_WIRELESS_REQ         = 0xF9                /* 指示车台远程升级请求应答(DOWN) */
    
    /* CAN通讯 */
    DN_PE_CMD_CAN_TRANS_DATA            = 0x90,                /* CAN业务协议透传请求(DOWN) */
    UP_PE_CMD_CAN_TRANS_DATA            = 0x90,                /* CAN业务协议透传请求(UP) */
    
    DN_PE_CMD_CAN_SET_PARA              = 0x91,                /* CAN通信参数设置请求(DOWN) */
    UP_PE_ACK_CAN_SET_PARA              = 0x91,                /* CAN通信参数设置请求的应答(UP) */
    
    DN_PE_CMD_CAN_CLOSE                 = 0x92,                /* CAN通信关闭请求(DOWN) */
    UP_PE_ACK_CAN_CLOSE                 = 0x92,                /* CAN通信关闭请求的应答(UP) */
    
    DN_PE_CMD_CAN_RESET                 = 0x93,                /* CAN通信总线复位请求(DOWN) */
    UP_PE_ACK_CAN_RESET                 = 0x93,                /* CAN通信总线复位请求的应答(UP) */
    
    DN_PE_CMD_CAN_SET_FILTER_ID_LIST    = 0x94,                /* CAN滤波ID设置,列表式(DOWN) */
    UP_PE_ACK_CAN_SET_FILTER_ID_LIST    = 0x94,                /* CAN滤波ID设置,列表式的应答(UP) */
    
    DN_PE_CMD_CAN_SET_FILTER_ID_MASK    = 0x95,                /* CAN滤波ID设置,掩码式(DOWN) */
    UP_PE_ACK_CAN_SET_FILTER_ID_MASK    = 0x95,                /* CAN滤波ID设置,掩码式的应答(UP) */
    
    UP_PE_CMD_CAN_DATA_REPORT           = 0x98,                /* 主动上报CAN数据请求(UP) */
    DN_PE_ACK_CAN_DATA_REPORT           = 0x98,                /* 主动上报CAN数据请求的应答(DOWN)*/
    
    DN_PE_CMD_CAN_SEND_DATA             = 0x99,                /* 发送CAN数据请求(UP) */
    UP_PE_ACK_CAN_SEND_DATA             = 0x99,                /* 发送CAN数据请求的应答(DOWN)*/
    
    UP_PE_CMD_CAN_BUS_STATUS_REPORT     = 0x9A,                /* 主动上报CAN总线状态?UP) */
    DN_PE_ACK_CAN_BUS_STATUS_REPORT     = 0x9A,                /* 主动上报CAN总线状态的应答(DOWN) */
    

    /* 碰撞侧翻 */
    DN_PE_CMD_HITCK_DMC_START           = 0xA1,                 /* 启动碰撞检测标定请求(DOWN) */
    UP_PE_ACK_HITCK_DMC_START           = 0xA1,                 /* 启动碰撞检测标定应答(UP) */
    
    DN_PE_CMD_HITCK_DMC_STOP            = 0xA2,                 /* 停止碰撞检测标定请求(DOWN) */
    UP_PE_ACK_HITCK_DMC_STOP            = 0xA2,                 /* 停止碰撞检测标定应答(UP) */
    
    DN_PE_CMD_HITCK_PARA_SET            = 0xA3,                 /* 碰撞检测参数设置请求(DOWN) */
    UP_PE_ACK_HITCK_PARA_SET            = 0xA3,                 /* 碰撞检测参数设置应答(UP) */
    
    UP_PE_CMD_HITCK_REPORT              = 0xA4,                 /* 碰撞检测信号上报(UP) */
    DN_PE_ACK_HITCK_REPORT              = 0xA4,                 /* 碰撞检测信号上报应答(DOWN) */
    
    /* GPS模块 */
    DN_PE_CMD_SET_GPS_UART              = 0xB1,                 /* 设置GPS串口通信参数请求 (DOWN) */
    UP_PE_ACK_SET_GPS_UART              = 0xB1,                 /* 设置GPS串口通信参数请求应答(UP) */
    DN_PE_CMD_CTL_GPS_POWER             = 0xB2,                 /* 控制GPS模块电源请求 (DOWN) */
    UP_PE_ACK_CTL_GPS_POWER             = 0xB2,                 /* 控制GPS模块电源请求应答(UP) */
    UP_PE_CMD_GPS_DATA_SEND             = 0xB3,                 /* GPS数据上行透传请求(UP) */
    DN_PE_ACK_GPS_DATA_SEND             = 0xB3,                 /* GPS数据上行透传请求应答(DOWN) */
    DN_PE_CMD_GPS_DATA_SEND             = 0xB4,                 /* GPS数据下行透传请求 (DOWN) */
    UP_PE_ACK_GPS_DATA_SEND             = 0xB4,                 /* GPS数据下行透传请求应答(UP) */
    
    /* 固件升级 */
    UP_PE_CMD_FIRMWARE_UPDATE_REQ       = 0xE1,                 /* 从机请求固件升级(UP) */
    DN_PE_ACK_FIRMWARE_UPDATE_REQ       = 0xE1,                 /* 从机请求固件升级应答(DOWN) */
    
    DN_PE_CMD_FIRMWARE_UPDATE_REQ       = 0xE2,                 /* 主机下发固件升级请求 (DOWN) */
    UP_PE_ACK_FIRMWARE_UPDATE_REQ       = 0xE2,                 /* 主机下发固件升级请求应答(UP) */
    
    UP_PE_CMD_FIRMWARE_DATA_REQ         = 0xE3,                 /* 从机请求固件数据(UP) */
    DN_PE_ACK_FIRMWARE_DATA_REQ         = 0xE3,                 /* 从机请求固件数据应答(DOWN) */
    
    UP_PE_CMD_INFORM_UPDATE_RESULT      = 0xE4,                 /* 固件更新结果通知(UP) */
    DN_PE_ACK_INFORM_UPDATE_RESULT      = 0xE4,                 /* 固件更新结果通知应答(DOWN) */
    
    PROTOCOL_COMMAND_MAX
} PROTOCOL_COMMAND_E;
 
/* 通用参数定义 */
typedef enum {
    /* 基本参数类 */
    PARA_BASE_START,
    PARA_MYTEL            = 0x01,                /* 本机号码 */
    PARA_SMSC             = 0x02,                /* 短信服务中心号码 */
    PARA_ALARMTEL         = 0x03,                /* 报警号码 */
    
    PARA_SERVER1_MAIN     = 0x04,                /* 第一服务器通信参数（主） */
    PARA_SERVER1_BACK     = 0x05,                /* 第一服务器通信参数（副） */
    PARA_SERVER1_ATTRIB   = 0x06,                /* 第一服务器通信属性 */
    PARA_SERVER1_AUTHCODE = 0x07,                /* 第一服务器鉴权码 */
    PARA_SERVER1_LINK     = 0x08,                /* 第一服务器链路维护参数 */
    PARA_SERVER2_MAIN     = 0x09,                /* 第二服务器通信参数（主） */
    PARA_SERVER2_BACK     = 0x0A,                /* 第二服务器通信参数（副） */
    PARA_SERVER2_ATTRIB   = 0x0B,                /* 第二服务器通信属性 */
    PARA_SERVER2_AUTHCODE = 0x0C,                /* 第二服务器鉴权码 */
    PARA_SERVER2_LINK     = 0x0D,                /* 第二服务器链路维护参数 */
    
    PARA_VEHICHE_PROVINCE = 0x0E,                /* 车辆归属地 */
    PARA_VEHICHE_CODE     = 0x0F,                /* 车牌号 */
    PARA_VEHICHE_COLOUR   = 0x10,                /* 车辆颜色 */
    PARA_VEHICHE_BRAND    = 0x11,                /* 车辆分类 */
    PARA_VEHICHE_VIN      = 0x12,                /* 车辆VIN */
    
    PARA_DEVICEINFO       = 0x13,                /* 设备信息 */
    PARA_SLEEP            = 0x14,                /* 省电参数 */
    PARA_AUTOREPT         = 0x15,                /* 主动上报参数 */
    PARA_DEVICEID         = 0x16,                /* 设备ID */
    PARA_SMSPSW           = 0x17,                /* 短信命令密码 */
    PARA_BASE_END,
    
    /* 数据类 */
    PARA_DATA_START = 0x80,
    PARA_DATA_GPS         = 0x81,                /* GPS数据 */
    PARA_DATA_END,
    
    PARA_MAX
} PARA_E;

#endif
