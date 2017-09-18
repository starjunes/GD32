/********************************************************************************
**
** 文件名:     yx_protocol_type.h
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要定义设备与中心通信协议命令宏文件
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2013/09/10 | 王锚华 |  创建第一版本
*********************************************************************************/
#ifndef H_PROTOCOL_TYPE
#define H_PROTOCOL_TYPE      1


/* 协议头 */
typedef struct {
    INT8U   msgid[2];                  /* 协议ID */
    INT8U   msgattrib[2];              /* 消息体属性 */
    INT8U   mytel[6];                  /* 本机号码,BCD[6] */
    INT8U   flowseq[2];                /* 流水号 */
    INT8U   data[1];
} PROTOCOL_HEAD_T;

/* 协议数据收发通道和收发属性 */
typedef struct {
    INT16U attrib;           /* 收发属性,见SM_ATTRIB_*** */
    INT8U  channel;          /* 收发通道，见UDP_USER_E,TCP_USER_E,SM通道则填为0 */
    INT8U  type;             /* 发送协议数据类型,见PTOTOCOL_TYPE_E */
    INT8U  priority;         /* 发送协议数据类型,见PTOTOCOL_PRIO_E */
    char   tel[23];          /* 手机号码, 以'\0'为结束符,短信通道要填,其他不用时填0 */
} PROTOCOL_COM_T;




#define SM_ATTR_SM                  0x1000
#define SM_ATTR_TCP                 0x2000
#define SM_ATTR_UDP                 0x4000
#define SM_ATTR_AUTOSELECT          0x8000
#define SM_ATTR_GPRS_CHAMASK        0xE000
#define SM_ATTR_CHAMASK             0xF000

/*
********************************************************************************
*                  DEFINE SYSTEM FRAME HEAD LEN
********************************************************************************
*/
#define SYSHEAD_LEN                     (sizeof(PROTOCOL_HEAD_T) - 1)
#define SYSTAIL_LEN                     1

/*
********************************************************************************
*                  应答类型
********************************************************************************
*/
#define ACK_SUCCESS          0x00      /* 成功 */
#define ACK_FAIL             0x01      /* 失败 */
#define ACK_ERR              0x02      /* 错误 */

/*
********************************************************************************
*                  DEFINE SYSTEM FRAME ACK TYPE
********************************************************************************
*/
#define _TP_SYSFRAME_ACK     0x01
#define _TP_SYSFRAME_NAK     0x00

/*
********************************************************************************
*                   定义协议数据发送优先级
********************************************************************************
*/
typedef enum {
    PTOTOCOL_PRIO_LOW,       /* 低优先级 */
    PTOTOCOL_PRIO_HIGH,      /* 高优先级 */
    PTOTOCOL_PRIO_MAX
} PTOTOCOL_PRIO_E;

/*
********************************************************************************
*                  定义 TCP/UDP通道的协议数据类型
********************************************************************************
*/
typedef enum {
    PTOTOCOL_TYPE_LOG  = 0x01,         /* 登入协议数据 */
    PTOTOCOL_TYPE_DATA = 0x02,         /* 普通业务数据 */
    PTOTOCOL_TYPE_MAX
} PTOTOCOL_TYPE_E;


/*
********************************************************************************
*                  定义协议扩展类型
********************************************************************************
*/
#define PROTOCOL_EXT_HEAD    0x2000    /* 扩展信息头 */
#define PROTOCOL_EXT_RSA     0x0400    /* 扩展加密 */

/*
************************************************************************************************
                   协议号关键字宏定
************************************************************************************************
*/
/* common 通用和杂项协议*/      
#define    UP_ACK_COMMON                         0x0001                 /* 终端通用应答(UP) */
#define    DN_ACK_COMMON                         0x8001                 /* 平台通用应答(DN) */
    
#define    UP_CMD_HEART                          0x0002                 /* 终端心跳(UP)*/
#define    DN_CMD_RESEND                         0x8003                 /* 补传分包请求(DN) */  
    
#define    UP_CMD_REG                            0x0100                 /* 终端注册(UP) */
#define    DN_ACK_REG                            0x8100                 /* 终端注册应答(DN) */     


#define    UP_CMD_REG_UNREGIST                   0x0003                 /* 终端注销(UP) */
#define    UP_CMD_AULOG                          0x0102                 /* 终端鉴权(UP) */  

#define    DN_CMD_SETPARA                        0x8103                 /* 设置终端参数(DN) */ 
    
#define    DN_CMD_QRYPARA                        0x8104                 /* 查询终端参数(DN) */ 
#define    UP_ACK_QRYPARA                        0x0104                 /* 查询终端参数应答(UP) */  
    
#define    DN_CMD_CONTROLDEV                     0x8105                 /* 终端控制(DN) */             
#define    DN_CMD_QRYPARALIST                    0x8106                 /* 查询指定终端参数(DN) */                    
#define    DN_CMD_MEATTRIB                       0x8107                 /* 查询终端属性(DN) */ 
#define    UP_ACK_MEATTRIB                       0x0107                 /* 查询终端属性应答(UP) */

#define    DN_CMD_CALLBACKTEL                    0x8400                 /* 电话回拨(DN) */   
#define    DN_CMD_SETTELNOTE                     0x8401                 /* 设置电话本(DN) */  
    
#define    DN_CMD_CONTROLCAR                     0x8500                 /* 车辆控制(DN) */   
#define    UP_ACK_CONTROLCAR                     0x0500                 /* 车辆控制应答(UP) */

#define    UP_CMD_CANDATA                        0x0705                  /* CAN 总线数据上传(UP) */

#define    UP_CMD_DEVICERSA                      0x0A00                  /* 终端RSA 公钥(UP) */ 
#define    DN_CMD_CENTERRSA                      0x8A00                  /* 平台RSA 公钥(DN) */ 
    
#define    UP_CMD_TASKRESPOND                    0x0B01                  /* (UP) */   

#define    UP_CMD_SIGNIN                         0x0B03                  /* (UP) */   
#define    UP_CMD_SIGNOUT                        0x0B04                  /* (UP) */   
#define    UP_CMD_TAXDATA                        0x0B05                  /* (UP) */   

#define    UP_CMD_TASKCOMPLETE                   0x0B07                  /* (UP) */   
#define    UP_CMD_TASKCANCEL                     0x0B08                  /* (UP) */               
#define    UP_CMD_EXTRANSMITION                  0x0B10                  /* (UP) */       
#define    UP_ACK_TERMINALCHECK                  0x0B11                  /* (UP) */   
                                    
#define    UP_CMD_TSMDATA                        0x0B40                  /* (UP) */                   
#define    UP_CMD_TSMDATATIMES                   0x0B41                  /* (UP) */   

#define    UP_CMD_TSMDATALIST                    0x0B42                  /* (UP) */   
#define    UP_CMD_TSMDATADETAIL                  0x0B43                  /* (UP) */   
#define    UP_CMD_TSMDATACOLLECT                 0x0B44                  /* (UP) */   

#define    UP_CMD_BLKLSTINFO                     0x0B50                  /* (UP) */   
#define    UP_CMD_BLKLSTDOWN                     0x0B51                  /* (UP) */               
#define    UP_CMD_BLKLSTVER                      0x0B52                  /* (UP) */       
#define    UP_ACK_BLKLSTUDDATA                   0x0B53                  /* (UP) */ 
    
#define    DN_CMD_TAXICALL                       0x8B00                  /* (DN) */                
#define    DN_CMD_TASKRESULT                     0x8B01                  /* (DN) */      
#define    DN_CMD_TASKCANCEL                     0x8B09                  /* (DN) */    
#define    DN_CMD_AFFIRMALARM                    0x8B0A                  /* (DN) */             
#define    DN_CMD_RELIEVEALARM                   0x8B0B                  /* (DN) */            
#define    DN_CMD_EXTRANSMITION                  0x8B10                  /* (DN) */    
#define    DN_CMD_TERMINALCHECK                  0x8B11                  /* (DN) */    
    
#define    DN_CMD_TSMDATAQRY                     0x8B41                  /* (DN) */      
#define    DN_CMD_TSMDATADETAIL                  0x8B43                  /* (DN) */          
#define    DN_CMD_TSMDATACOLLECT                 0x8B44                  /* (DN) */    
               
#define    DN_CMD_BLKLSTINFO                     0x8B50                  /* (DN) */                 
#define    DN_CMD_BLKLSTDOWN                     0x8B51                  /* (DN) */         
#define    DN_CMD_BLKLSTVER                      0x8B52                  /* (DN) */             
#define    DN_CMD_BLKLSTUPDATA                   0x8B53                  /* (DN) */ 

/* INFO 信息服务协议 */ 
#define    DN_CMD_TEXTINFO                       0x8300                 /* 文本信息下发(DN) */ 
#define    DN_CMD_EVENTSET                       0x8301                 /* 事件设置(DN) */ 
#define    UP_CMD_EVENTREPORT                    0x0301                 /* 事件报告(UP) */
#define    DN_CMD_ASK                            0x8302                 /* 提问下发(DN) */ 
#define    UP_ACK_ASK                            0x0302                 /* 提问应答(UP) */
#define    DN_CMD_INFOMENU                       0x8303                 /* 信息点播菜单设置(DN) */
#define    UP_CMD_INFOSWITCH                     0x0303                 /* 信息点播/取消(UP) */     
#define    DN_CMD_INFOSEVER                      0x8304                 /* 信息服务(DN) */ 

#define    DN_CMD_NEAR_MENULIST                  0x8902                  /* 周边信息查询二期已去掉，这里增加0x8902，暂保留该功能 laird(DN) */    
#define    DN_ACK_NEAR_INFOQRY                   0x8901                  /* 周边信息查询二期已去掉，这里增加0x8901，暂保留该功能 laird(DN) */    
#define    UP_CMD_NEAR_INFOQRYREQ                0x0703                  /* 周边信息查询二期已经去掉，这里增加0703，暂保留该功能 laird(UP) */
#define    UP_CMD_EBILL                          0x0701                  /* 电子路单上报请求(UP) */

/* TRANS 透传服务 */    
#define    DN_CMD_TRANSDATA                      0x8900                  /* 数据下行透传(DN) */ 
#define    UP_CMD_TRANSDATA                      0x0900                  /* 调度信息数据上报中心请求(UP)-数据上行透传 */
#define    UP_CMD_TRANSDATA_ZIP                  0x0901                  /* 发送GZIP文件 (UP) */   

/* TRACK 定位服务协议 */    
#define    UP_CMD_GPS_INFO                       0x0200                  /* 位置信息汇报 (UP) */
#define    DN_CMD_POSQRY                         0x8201                  /* 位置信息查询(DN) */  
#define    UP_ACK_POSQRY                        0x0201                  /* 位置信息查询应答(UP) */
#define    DN_CMD_POSMONITOR                     0x8202                  /* 临时位置跟踪控制(DN) */ 
 
#define    UP_CMD_PACKDATA                       0x0704                  /* 定位数据批量上传(UP) */  

/* TR 行驶记录仪协议 */     
#define    DN_CMD_GATHERRECORD                   0x8700                  /* 行驶记录仪数据采集命令(DN) */  
#define    UP_CMD_RUNRECORD                      0x0700                  /* 行驶记录数据上传(UP) */
#define    DN_CMD_SETRRECORD                     0x8701                  /* 行驶记录仪参数下传命令(DN) */                      
#define    DN_CMD_DRIVERINFO                     0x8702                  /* 上报驾驶员身份信息请求(DN) */  

#define    UP_CMD_DRIVERINFO                     0x0702                  /* 驾驶员身份信息采集上报(UP) */
#define    UP_CMD_DRIVERLOG                      0x070A                  /* (自定义签到协议)上传司机签到签退协议(接触式IC卡使用旧协议070a,RFID卡使用新协议0702) (UP) */

                    
/* MEDIA 多媒体协议*/
#define    UP_CMD_PICAUDIO_EVENTREP              0x0800                  /* 多媒体事件信息上传(UP) */
    
#define    UP_CMD_PICAUDIO                       0x0801                  /* 多媒体数据上传(UP) */
#define    DN_ACK_PICAUDIO                       0x8800                  /* 多媒体数据上传应答(DN) */                                  

#define    DN_CMD_CENTERSNAP                     0x8801                  /* 摄像头立即拍摄命令(DN) */
#define    UP_ACK_RTSNAPINDEX                    0x0805                  /* 中心实时抓拍应答(UP) */
    
#define    DN_CMD_CENTERQRYINDEX                 0x8802                  /* 存储多媒体数据检索(DN) */ 
#define    UP_ACK_CENTERQRYINDEX                 0x0802                  /* 存储多媒体数据检索应答 (UP) */

#define    DN_CMD_CENTER_CALLINDEX               0x8803                  /* 存储多媒体数据上传(DN) */    

#define    DN_CMD_STARTRECAUDIO                  0x8804                  /* 录音开始命令(DN) */          
#define    DN_CMD_CENTER_SINGLEINDEX             0x8805                  /* 单条存储多媒体数据检索上传命令(DN) */
 
/* ALARM 报警服务协议，包括区域等设置*/    
#define    DN_ACK_ALARM                          0x8203                  /* 人工确认报警消息(DN) */ 

#define    DN_CMD_SETCIRCLE                      0x8600                  /* 设置圆形区域(DN) */    
#define    DN_CMD_DELCIRCLE                      0x8601                  /* 删除圆形区域(DN) */                     
#define    DN_CMD_SETRECT                        0x8602                  /* 设置矩形区域(DN) */                       
#define    DN_CMD_DELRECT                        0x8603                  /* 删除矩形区域(DN) */   
#define    DN_CMD_SETPOLYRANGE                   0x8604                  /* 设置多边形区域(DN) */                                      
#define    DN_CMD_DELPOLYRANGE                   0x8605                  /* 删除多边形区域(DN) */                                  
#define    DN_CMD_SETROADLINE                    0x8606                  /* 设置路线(DN) */                       
#define    DN_CMD_DELROADLINE                    0x8607                  /* 删除路线(DN) */ 

#if EN_BD_INVITE > 0
#define UP_CMD_TRANSINFO                         0x7F00                  /* 上传终端信息(北斗招标功能) */
#endif


/*
********************************************************************************
*                  DEFINE ALARM ADDED TYPEID
********************************************************************************
*/
#define TAG_ALARM_METER                 0x01
#define TAG_ALARM_OIL                   0x02
#define TAG_ODO_PULSEVT                 0x03
#define TAG_ALARM_ID                    0x04
#define TAG_ALARM_VT                    0x11
#define TAG_ALARM_RANGE                 0x12
#define TAG_ALARM_RUNTIME               0x13
#if EN_ADDSATELITE > 0
#define TAG_GPS_TYPE                    0x20
#define TAG_GPS_USEDID                  0x21
#define TAG_GPS_VIEWID                  0x31
#endif

#define TAG_EXTSIG_STAT                 0x25 
#define TAG_IO_STAT                     0x2A 
#define TAG_ADVALUE                     0x2B 
#define TAG_GSMSIGNAL                   0x30
#define TAG_GNSSNUM                     0x31

#endif






