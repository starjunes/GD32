/**************************************************************************************************
**                                                                                               **
**  文件名称:  Dal_Structs.H                                                                     **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2010                                    **
**  创建信息:  LEON -- 2010年12月1日                                                             **
**  文件描述:  全局结构体定义头文件                                                              **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/
#ifndef __DAL_STRUCTS_H
#define __DAL_STRUCTS_H

#include "gd32f30x_def.h"


typedef union {
	INT16U     hword;
#ifdef __BIG_ENDIAN
	struct {
    	INT8U  high;
    	INT8U  low;
    } bytes;
#else  // LITTLE ENDIAN
	struct {
    	INT8U  low;
    	INT8U  high;
    } bytes;
#endif
} WORD_UNION;

typedef enum
{
    ADAS_OPEN = 0x01,                       
    ADAS_CLOSE = 0x02,                            
} ADAS_STATUS_E;

typedef union {
    INT16U      hword;
    struct {
       INT8U    low;
       INT8U    high;
    } bytes;
} HWORD_UNION;

typedef struct {
    INT8U       year;
    INT8U       month;
    INT8U       day;
} DATE_T;

typedef struct {
    INT8U       hour;
    INT8U       minute;
    INT8U       second;
} TIME_T;

typedef struct {
    DATE_T      date;
    TIME_T      time;
} SYSTIME_T;

typedef struct {
    INT16U      index;
    void     (* entryproc)(void);
} FUNCENTRY_T;

typedef struct {
	char	    *sstr;                                          /* 字符串标识 */
	void        (* entryproc)(void);                            /* 对应的处理函数 */
} FUNCSTRENTRY_T;

typedef struct {
    INT8U       c_flags;                /* c_convert0 + c_convert1 = c_flags */
    INT8U       c_convert0;             /* c_convert0 + c_convert2 = c_convert0 */
    INT8U       c_convert1;
    INT8U       c_convert2;
} ASMRULE_T;

typedef struct
{
     /* HBIT = 01A000      A: VALID BIT,   0 ----- VALID    1 ----- INVALID*/
     INT8U       hbit;
     INT8U       latitude[4];
     INT8U       longitude[4];
     INT8U       vector;
     INT8U       direction;
} GPS_STRUCT;

typedef struct
{
	INT8U	link_status;//连接状态
	INT8U	reserved[4];//预留
}WIFI_STATUS;

typedef struct
{
   INT8U ac_year[2];
   INT8U ac_month[2];
   INT8U ac_day[2];
} ASCII_DATE;

typedef struct
{
   INT8U  ac_hour[2];
   INT8U  ac_minute[2];
   INT8U  ac_second[2];
} ASCII_TIME;

typedef struct {
    INT8U  second;
    INT8U  minute;
} CLOCKTIME_STRUCT;

typedef struct {
    BOOLEAN udiskinsert;                         /* U盘是否接入 */
    INT8U   udiskinterface;                      /* U盘总线接口规范 */
} para_struct;

typedef struct {
    INT8U     bright;       //液晶对比度
    BOOLEAN   keysound;     //按键音有无
    INT8U     light;        //背光持续时间
} SETCONFIG_STRUCT;

typedef struct {
    INT8U   router;
} ROUTER_STRUCT;             //电话本流水号

typedef struct {
    INT8U     flag;
    char      code[9];
    BOOLEAN   BT_power;
} BT_PIN_STRUCT;             //蓝牙PIN码

typedef struct {
    BOOLEAN   CAN_power;    //CAN功能总开关
    INT8U     buadrate;
    INT8U     type;
    BOOLEAN   test_flag;    //测试功能
    INT8U     ID_num;
    INT32U    ID[14];
} CAN_STRUCT;               //CAN参数


/*************************************************************************************************/
/*                           IC卡模块属性结构体                                                  */
/*************************************************************************************************/
typedef struct {
    INT8U       type;                                                /* IC卡类型 0-未定，1-4442，2-24cXX */
} ICPARA_T;

/*************************************************************************************************/
/*                           LDW功能是否开启关闭                                                  */
/*************************************************************************************************/
typedef struct {
    INT8U       ldw_ison;                                                /* LDW功能是否开启关闭 */
    INT8U       fcw_ison;                                                /* FCW功能是否开启关闭 */
} ADASPARA_T;

/*************************************************************************************************/
/*                           IC卡中豪华版与标准版设置结构体                                                  */
/*************************************************************************************************/
typedef struct {
    INT8U       cantimeen;                                           /* 往CAN总线发送时间使能 */
    INT8U       candistnen;                                          /* 往CAN总线发送里程使能 */
    INT8U       canspeeden;                                          /* 往CAN总线发送车速使能 */
    INT8U       candistnresen;                                       /* 往CAN总线小计里程复位使能 */
    INT8U       canoilen;                                            /* 往CAN总线发送油耗使能 */
} ICCANPARA_T;


#define  DEF_CANBAUD       CAN_250
#define  DEF_FTYPE         _FRAME_EXT
#define  DEF_FMAT          _FRAME_DATA


/*************************************************************************************************/
/*                           上柴锁车参数结构体                                                  */
/*************************************************************************************************/
typedef struct {
    INT8U       ecutype;                                             /* ECU类型，决定开机采用何种握手方式 */
    INT8U       firmcodelen;                                         /* 固定密钥长度 */
    INT8U       firmcode[20];                                        /* 固定密钥 */
    INT8U       srlnumberlen;                                        /* GPSID长度 */
    INT8U       serialnumber[20];                                    /* GPSID */
	INT8U		securityKeylen;										 /* securityKey长度 */
	INT8U		securityKey[20];									 /* securityKey */
	INT8U		deratespeedlen;										 /* deratespeed长度 */
	INT8U		deratespeed[20];									 /* deratespeed */
    INT8U       unbindstat;                                          /* 解绑成功标志 0:绑定 1:解绑*/
    INT8U       limitfunction;                                       /* 锁车类型，包含绑定，解绑，限制，锁车等 */
    BOOLEAN     configsuccess;                                       /* 是否配置成功 */
	INT16U		canbaud[3];											 /* can初始化波特率 */
} SCLOCKPARA_T;
/*************************************************************************************************/
/*                           锁车日志结构体                                                 */
/*************************************************************************************************/
typedef struct{
    INT8U Year;
    INT8U Mon;
    INT8U Day;
    INT8U Hour;
    INT8U Min;
    INT8U Sec;
} BCD_TIME_T;



typedef struct {
    BCD_TIME_T locktime;     //锁车时间
    INT8U      locktype;     //锁车类型
} LOCK_PARA;


typedef struct {
    INT8U       storenum;         //存储日志数目
    INT8U       storeadd;         //最新存储位置，覆盖最旧
    LOCK_PARA   lockrecord[10];   //日志数据，最大10个日志。
} LOCK_RECORD;

/*************************************************************************************************/
/*                           CAN总线数据转发                                                     */
/*************************************************************************************************/
typedef struct {
    INT8U       source;                                              /* 源通道 */
    INT8U       destination;                                         /* 目标通道 */
    INT32U      id;                                                  /* 滤波的ID */
    INT32U      screeid;                                             /* 屏蔽ID */
} RELAY_T;

typedef struct {
    INT8U       num;
    RELAY_T     relay[10];
} CAN_RELAY_T;

/*************************************************************************************************/
/*                           扩展锁车参数结构体                                                  */
/*************************************************************************************************/
typedef struct {
    INT8U       wctype;                                              /* 潍柴发动机类型(自适应) */
} EXLOCKPARA_T;


/*************************************************************************************************/
/*                           定义与升级更新程序有关的参数、结构体等                              */
/*************************************************************************************************/
#define MAX_VERSION_LEN      30                                      /* program version length */
#define UP_FILENAME_LEN      30                                      /* file name length */
#define MAX_TELNUM_LEN       16                                      /* telephone number length */

#define STEP_REQ             0x01                                    /* step:request filelist */
#define STEP_LOADING         0x02                                    /* step:loading */
#define STEP_LOADED          0x03                                    /* step:loaded */
#define STEP_REPLACED        0x04                                    /* step:replaced */

/*************************************************************************************************/
/*                         定义与升级更新程序有关的参数、结构体等                                */
/*************************************************************************************************/
typedef struct {
    BOOLEAN     updated;                        /* flag stand for if update finish */
    INT8U       fname_len;                      /* file name length */
    INT8U       fname[UP_FILENAME_LEN];         /* file name string */
    INT8U       fsize[4];                       /* file data size */
    INT8U       fchksum[4];                     /* file data checksum */
} FILE_INFO_T;

typedef struct {
    BOOLEAN     need_update;                    /* flag stand for if need update,TRUE need and FALSE not need */
    INT8U       ser_ip[4];                      /* ip address of update sever  */
    INT8U       ser_port[2];                    /* port of update sever */
    INT8U       verlen;                         /* program version length */
    INT8U       ver[MAX_VERSION_LEN];           /* program version string */
    INT8U       tellen;                         /* telephone number length */
    INT8U       tel[MAX_TELNUM_LEN];            /* telephone number */
    INT8U       filenums;                       /* file numbers which for update */
} UPDATE_BASE_INFO_T;

typedef struct {
    INT8U       step;                           /* update step */
    INT8U       file_id;                        /* file id loading */
    INT32U      fsize_temp;                     /* file data size downloaded */
} DOWNLOAD_CB_T;

typedef struct {
    INT8U               chksum;                 /* total check sum */
    INT8U               reserved;               /* reserved byte for bytes mutiple of 32 */
    DOWNLOAD_CB_T       s_dcb;                  /* download control block */
    UPDATE_BASE_INFO_T  base_info;              /* update base information block */
    FILE_INFO_T         f_info;                 /* update file information */
} UPDATE_MAN_T;

/*************************************************************************************************/
/*                           复位统计                                                            */
/*************************************************************************************************/
#define MAX_REC_EXT          5                                                 /* 最多统计最近5个信息 */
#define FILENAMELEN          16
#define ERR_ID_INT           0xffff                                             /* 内部复位 */
#define ERR_ID_EXT           0xfff0                                             /* 外部复位 */
#define ERR_ID_ERR           0xff00                                             /* 异常错误复位 */
#define ERR_ID_LINK          0xf000                                             /* 链路超时复位 */

typedef struct {
    SYSTIME_T       start_time;                                                /* 启动或者上次复位的时间 */
    INT8U           filename[FILENAMELEN];                                     /* 出错文件名 */
    INT16U          line;                                                      /* 出错行号 */
    INT32U          err_id;                                                    /* 出错代码 */
    //INT32U          mem_peak;                                                  /* 复位时的内存峰值 */
} REC_EXT_T;

typedef struct {
    INT16U      rst_total;                                                     /* 复位总数 */
    INT16U      rst_int;                                                       /* 内部复位 */
    INT16U      rst_ext;                                                       /* 外部复位 */
    INT16U      rst_err;                                                       /* 异常复位 */
    INT16U      rst_link;                                                      /* 链路超时 */
    REC_EXT_T   rst_rec_ext[MAX_REC_EXT];                                      /* the extra reset record */
} RESETREC_T;

/* 重力加速度传感器标定 */
typedef struct {
    INT8U  isvalid;          /* 是否已标定过,TRUE-已标定,FALSE-未标定 */
    INT16S x;                /* x轴重力加速度,单位0.001g */
    INT16S y;                /* y轴重力加速度,单位0.001g */
    INT16S z;                /* z轴重力加速度,单位0.001g */
} GSENSOR_CALIBRATION_T;

typedef struct {
    BOOLEAN  is_cali;
    INT32S   calvalue_x; /* X轴标定值 */
    INT32S   calvalue_y; /* Y轴标定值 */
    INT32S   calvalue_z; /* Z轴标定值 */
    INT16U   setvalue_g; /* 加速度设置阈值 */
} AXISPARA_T;
 
/*************************************************************************************************/
/*                               备份锁车参数结构体                                                  */
/*************************************************************************************************/
typedef struct {
    INT8U       lock_en;                                             /* 锁车使能开关 */
    INT8U       lock_en_bak1;                                        /* 锁车使能开关备份1 */
    INT8U       lock_en_bak2;                                        /* 锁车使能开关备份2 */
    INT8U       unbindstat_bak1;                                     /* 解绑状态备份1 */
    INT8U       unbindstat_bak2;                                     /* 解绑状态备份2 */
    INT8U       chksum;                                              /* 校验和(此校验字节前的所有数据的校验和) */
} SCLOCKPARABAK_T;

typedef struct {
    INT8U kms_cmd;        /* 锁车指令 */
    INT8U rota_speed[2];    /* 转速 */
} KMS_LOCK_OBJ_T; 


/*************************************************************************************************/
/*                             UDS   ECU升级数据保存结构体                                                  */
/*************************************************************************************************/
typedef struct {
    INT8U vin[17];                  /*车身VIN*/
    #if 0
    INT8U vmfspnumer[18];         /*车辆制造商分配零部件号*/
    INT8U ecumanudate[6];          /*ECU生产日期*/
    INT8U ecuserialnum[18];         /*ECU生产日序列号*/
    INT8U carhardnum[18];           /*车辆制造商ECU硬件号*/
    INT8U sysecuhardnum[12];        /*ECU硬件号*/
    INT8U sysecuhardvernum[10];     /*ECU硬件版本号*/
    INT8U sysecusoftnum[12];        /*ECU软件号*/
    INT8U sysecusoftvernum[12];     /*ECU软件版本号*/
    INT8U erotanum[18];             /*排放形式认证号*/
    INT8U enginetype[10];           /*发动机型号*/
    INT8U calibrationdate[6];      /*标定数据更改日期*/
    INT8U calibrationdatanum[18];   /*标定数据号*/
    INT8U calibrationdataver[10];   /*标定数据版本号*/
    INT8U sysecupartnum[12];        /*ECU零部件号*/
    INT8U sysecuerpnum[18];         /*ECU零部件ERP号*/
    #endif
    INT8U ecuverson[15];
    BOOLEAN readflag[2];            /*读标志*/
    BOOLEAN writeflag[2];           /*写标志*/
} ECUDATA_T;

#endif


