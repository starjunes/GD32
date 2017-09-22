/********************************************************************************
**
** 文件名:     dal_pp_misc.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现公共参数结构体定义和参数定义
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2011/11/15 | 叶德焰 |  创建第一版本
*********************************************************************************/
#ifndef DAL_PP_MISC_H
#define DAL_PP_MISC_H   1

/* 复位记录 */
#define MAX_RESET_RECORD               2  /* 最近复位记录数 */

typedef struct {
    SYSTIME_T  systime;             /* 复位时刻 */
    char       file[11];            /* 复位文件名 */
    INT16U     line;                /* 复位行号 */
} RESET_T;

typedef struct {
    INT32U  rst_total;                  /* number of all reset */
    INT16U  rst_int;                    /* number of reset by internal process */
    INT16U  rst_err;                    /* number of reset by error exit */
    INT16U  rst_ext;                    /* nunber of reset by external watchdog */
    RESET_T rst_record[MAX_RESET_RECORD];/* the extra reset record */
} RESET_RECORD_T;

/* 鉴权码 */
typedef struct {
    INT8U len;
    INT8U authcode[30];
} AUTH_CODE_T;

/* 业务协议类型 */
typedef enum {
    PROTOCOL_TYPE_JTB = 0x11,   /* 交通部通用版协议 */
    PROTOCOL_TYPE_YXT = 0x21,   /* 雅迅千里眼通用版协议 */
    PROTOCOL_TYPE_YXG = 0x31,   /* 雅迅千里眼公开版协议 */
    PROTOCOL_TYPE_MAX
} PROTOCOL_TYPE_E;

/* 通讯参数 */
#define IP_GROUP_NUM         1
#define MAX_DOMAINNAME_LEN   31

typedef struct {
    INT8U  isvalid;                              /* 该组参数是否有效 */
    char   ip[MAX_DOMAINNAME_LEN];               /* ip或域名 */
    INT16U port;                                 /* 端口 */
} GPRS_GROUP_T;

typedef struct {
    INT8U  protocol;                             /* 协议类型,见 PROTOCOL_TYPE_E */
    INT8U  istcp;                                /* 参数类型:是否为TCP参数 */
    char   apn[21];                              /* apn */
    char   username[20];                         /* 用户名 */
    char   password[20];                         /* 密码 */
    GPRS_GROUP_T ippara[IP_GROUP_NUM];
} GPRS_PARA_T;

/* 链路维护参数 */
#define DEF_HEART_PERIOD            60
#define DEF_HEART_WAITTIME          60
#define DEF_HEART_RSENDCNT          6

typedef struct {
    INT16U period;                               /* 终端心跳发送间隔，秒 */
    INT16U tcp_waittime;                         /* TCP消息应答超时时间，秒 */
    INT16U tcp_max;                              /* 消息重传次数 */
} LINK_PARA_T;

/* 手机号 */
typedef struct {
	INT8U tellen;                                /* 号码长度 */
	INT8U tel[15];                               /* 号码 */
} TEL_T, SMSTEL_T;

/* 车辆信息 */
#define MAX_VIN_LEN          17                  /* 车辆识别代码最大长度 */
#define MAX_VCODE_LEN        12                  /* 车牌号码最大长度 */
#define MAX_VBRAND_LEN       12                  /* 车牌分类最大长度 */

typedef struct {
    INT8U  province[2];                          /* 省域ID */
    INT8U  city[2];                              /* 市域ID*/
    
    //INT32U factor;                               /* 车辆特征系数 */
    INT8U  colour;                               /* 车辆颜色 */
    
    INT8U  l_vin;   
    INT8U  vin[MAX_VIN_LEN];                     /* 车辆识别代码 */
    
    INT8U  l_vcode;
    INT8U  vcode[MAX_VCODE_LEN];                 /* 车牌号码 */
    
    //INT8U  l_vbrand;
    //INT8U  vbrand[MAX_VBRAND_LEN];               /* 车牌分类 */
} VEHICLE_INFO_T;

/* 注册信息 */
#define MAX_INSTALLTIME_LEN  6                   /* 记录仪安装时间长度 */
#define MAX_TRID_LEN         15                  /* 记录仪编号 */

typedef struct {
    INT8U manufacturerid[5];                     /* 制造商ID */
    INT8U devtype[20];                           /* 设备型号 */
    INT8U devid[7];                              /* 设备ID */
    //INT8U installtime[MAX_INSTALLTIME_LEN];      /* 记录仪安装时间,BCD格式 */
    //INT8U trid[MAX_TRID_LEN];                    /* 记录仪编号 */
} DEVICE_INFO_T;

/* 重力加速度传感器标定 */
typedef struct {
    INT8U  isvalid;          /* 是否已标定过,TRUE-已标定,FALSE-未标定 */
    INT16S x;                /* x轴重力加速度,单位0.001g */
    INT16S y;                /* y轴重力加速度,单位0.001g */
    INT16S z;                /* z轴重力加速度,单位0.001g */
} GSENSOR_CALIBRATION_T;

/* 省电网络状态 */
typedef enum {
    SLEEP_NETWORK_OFFLINE = 0x00,         /* 关闭所有网络,包括网络和GPRS */
    SLEEP_NETWORK_GSM,                    /* GSM在线模式,可收发短信 */
    SLEEP_NETWORK_GPRS,                   /* GPRS在线模式 */
    SLEEP_NETWORK_MAX
} SLEEP_NETWORK_E;

/* 省电参数 */
typedef struct {
    INT8U  onoff;             /* 省电开关, TRUE-打开省电功能,FALSE-关闭省电功能 */
    INT8U  networkmode;       /* 省电时网络工作模式, 见 SLEEP_NETWORK_E */
    INT8U  delay;             /* ACC OFF后进入省电延时时间,单位:分钟,默认为20分钟,0表示立即 */
    //INT16U period;            /* 定时唤醒时间间隔,为0表示不定时唤醒, 单位:分钟 */
    //INT16U duration;          /* 定时唤醒时长,单位:秒 */
} SLEEP_PARA_T;

/* 主动上报参数 */
typedef struct {
    INT16U  period;          /* 主动上报周期,单位：秒,0-表示无需主动上报 */
} AUTOREPT_PARA_T;

/* GPS位置信息数据 */
typedef struct {
    SYSTIME_T systime;       /* 系统时间 */
    INT8U flag;              /* 状态字, 表示经纬度方向等。Bit0-经度方向，0-为东经，1-为西经;Bit1-纬度方向，0-为北纬，1-为南纬, bit2-表示定位有效性, bit3-表示上传定位标志 */
    INT8U longitude[4];      /* 经度,度+分+小数分 */
    INT8U latitude[4];       /* 纬度,度+分+小数分 */
    INT16S altitude;         /* 高程:单位:米 */
    INT32U odometer;         /* 里程:单位:米 */
} GPS_DATA_T;

/* 设备ID */
typedef struct {
    INT8U idlen;             /* 设备ID长度 */
    INT8U id[32];            /* 设备ID */
} DEVICE_ID_T;

/* 短信命令密码 */
#define LEN_PASSWORD         6
typedef struct {
    INT8U password[LEN_PASSWORD];
} SMS_PASSWORD_T;

/* 日志标志 */
typedef struct {
    BOOLEAN flag;           /* 是否开启日志标志 */
} LOG_FLAG_T;

    
/*
********************************************************************************
*                        默认PP参数定义
********************************************************************************
*/
#ifdef GLOBALS_PP_REG
//static TEL_T                    i_mytel             = {11, "13459034578"};


static GPRS_PARA_T              i_gprspara            = 
                               {
                                true, PROTOCOL_TYPE_JTB, "cmnet", "card", "card",
                                false, "", 0 /*,
                                false, "", 0*/
                               };

static SMS_PASSWORD_T           i_sms_password = {{'1', '2', '3', '4', '5', '6'}};

static LOG_FLAG_T               i_log_flag = {0};





#endif


#endif
