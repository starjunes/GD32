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


/* 手机号 */
typedef struct {
	INT8U tellen;                                /* 号码长度 */
	INT8U tel[15];                               /* 号码 */
} TEL_T, SMSTEL_T;

/* 省电参数 */
typedef struct {
    INT8U  onoff;             /* 省电开关, TRUE-打开省电功能,FALSE-关闭省电功能 */
    INT8U  networkmode;       /* 省电时网络工作模式, 见 SLEEP_NETWORK_E */
    INT8U  delay;             /* ACC OFF后进入省电延时时间,单位:分钟,默认为20分钟,0表示立即 */
    //INT16U period;            /* 定时唤醒时间间隔,为0表示不定时唤醒, 单位:分钟 */
    //INT16U duration;          /* 定时唤醒时长,单位:秒 */
} SLEEP_PARA_T;


/* 设备ID */
typedef struct {
    INT8U idlen;             /* 设备ID长度 */
    INT8U id[32];            /* 设备ID */
} DEVICE_ID_T;


/* 日志标志 */
typedef struct {
    BOOLEAN flag;           /* 是否开启日志标志 */
} LOG_FLAG_T;
/* 存储主机复位记录的状态 */
typedef struct {
    INT8U status;
}HOST_RESET_STATUS_T;

/* can适配信息 */
typedef struct {
    BOOLEAN can1;       /* can1是否能接收数据 */
    INT8U   can2_high;  /* can2 高引脚编号 */
    INT8U   cam2_low;   /* can2低引脚编号 */
} CAN_ADAPTER_INTO_T;


/* can配置信息 */

typedef struct {
    INT8U isvaild;
    INT8U  mode;       /* 工作模式,见 CAN_WORK_MODE_E */
    INT8U  idtype;     /* 帧ID类型,见 CAN_ID_TYPE_E */
    INT32U baud;       /* 波特率 */
} CAN_CFG_INFO_T;


/* can滤波信息 */
typedef struct {
    INT8U isvaild;      /* 是否有效 */
    INT8U filtertype;   /* 滤波类型 */
    INT8U idtype;       /* id类型 */
    INT8U idnum;        /* id数量 */
    union {
        struct {
            INT32U canid[56];
        } list;

        struct {
            INT32U id[14];
            INT32U mask[14];
        } mask;
    }filter;    
} CAN_FILTER_INFO_T;


/*
********************************************************************************
*                        默认PP参数定义
********************************************************************************
*/
#ifdef GLOBALS_PP_REG
//static TEL_T                    i_mytel             = {11, "13459034578"};


static LOG_FLAG_T               i_log_flag = {0};

static CAN_ADAPTER_INTO_T        i_can_adapter_info = {0, 0, 0};

static CAN_CFG_INFO_T            i_can_cfg_info = {TRUE, 0, 1, 250000};

#endif


#endif
