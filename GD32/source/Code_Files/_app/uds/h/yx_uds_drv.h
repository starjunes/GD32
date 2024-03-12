/********************************************************************************
**
** 文件名: yx_uds_drv.h
** 版权所有: (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述: 该模块主要实现统一诊断服务功能
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================

*********************************************************************************/
#ifndef YX_UDS_DRV_H
#define YX_UDS_DRV_H           1

#include "dal_can.h"
/*
********************************************************************************
* 宏定义
********************************************************************************
*/

#define FUNC_REQID                  0x18DB33F1                 /* 功能请求ID(MCU接收) */
#define UDS_PHSCL_REQID             0x18DAF6F1                /* 物理请求ID(MCU接收) */
#define UDS_PHSCL_RESPID            0x18DAF1F6                /* 响应ID(MCU发送) */

#define MASK_DEFAUTLT               (MASK_01 + MASK_08)        /* 测试失效+确认DTC */
#define DTC_FORMAT_J2012            0x00                       /* SAEJ2012格式 */
#define DTC_14229                   0x01                       /* ISO14229-1DTC格式 */

#define UNUSE_BYTE_FULL             0x00                       /* 无效数据填充0xAA */

#define P2_SERVER                   50                         /* P2Server = 50ms(单位1ms) */
#define EXT_P2_SERVER               500                        /* P2*Server = 5000ms(单位10ms) */

#define NOT_SESSION_DEFAULT_TIME    500                        /* 持续非默认会话最大时间(单位:10ms) */
#define SUPPRESS_RESP               0x80                       /* 禁止肯定响应 */
#define RESP_ADD                    0x40                       /* 肯定响应附加 */
#define ACCESS_WAIT_TIME            1000                       /* 安全访问等待最大时间(单位:10ms) */
#define RESET_WAIT_TIME             4                          /* 复位等待最大时间(单位:10ms) 要求50ms内执行*/
#define UDS_CAN_CH                  CAN_CHN_2                  /* can通道 */  

/* 流控帧参数 */
#if EN_UDS_TRANS > 0
#define FC_BS                       0
#define FC_STMIN                    0
#define UDS_TRANS_TIMEOUT           300
#else
#define FC_BS                       0
#define FC_STMIN                    2
#define UDS_TRANS_TIMEOUT           15
#endif

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef enum {
    SID_10 = 0x10,      /* Diagnostic Session Control 诊断会话控制 */
    SID_11 = 0x11,      /* Ecu Reset 电控单元复位 */
    SID_14 = 0x14,      /* Clear Diagnostic Information 清除诊断信息 */
    SID_19 = 0x19,      /* Read DTC Information 读取DTC信息 */
    SID_22 = 0x22,      /* Read Data by Identifier 读取数据 */
    SID_27 = 0x27,      /* Security Access 安全访问 */
    SID_28 = 0x28,      /* Communication Control 通信控制 */
    SID_2E = 0x2E,      /* Write Data by Identifier 写入数据 */
    SID_2F = 0x2F,      /* I/O Control by Identifier 输入输出控制 */
    SID_31 = 0x31,      /* Routing Control 例程控制 */
    SID_34 = 0x34,      /* RequestDownload，请求下载 */
    SID_36 = 0x36,      /* TransferData，传输数据 */
    SID_37 = 0x37,      /* RequestTransferExit 请求传输退出 */
    SID_3E = 0x3E,      /* Tester Present 诊断设备在线 */
    SID_85 = 0x85,      /* Control DTC Setting 控制DTC设置 */
    SID_7F = 0x7F,      /* Negative Response Service ID 否定响应服务ID*/
    SID_TYPE_MAX
} SID_TYPE_E;

typedef enum {
    NRC_11 = 0x11,      /* 服务不支持 */
    NRC_12 = 0x12,      /* 子功能不支持 */
    NRC_13 = 0x13,      /* 报文长度错误 */
    NRC_14 = 0x14,      /* 响应过长 */
    NRC_22 = 0x22,      /* 条件未满足 */
    NRC_24 = 0x24,      /* 请求序列错误 */
    NRC_31 = 0x31,      /* 请求超出范围 */
    NRC_33 = 0x33,      /* 安全访问拒绝 */
    NRC_35 = 0x35,      /* 秘钥无效 */
    NRC_36 = 0x36,      /* 超出密钥访问次数限制 */
    NRC_37 = 0x37,      /* 延时时间未到 */
    NRC_71 = 0x71,      /* 数据传输暂停 */
    NRC_72 = 0x72,      /* 一般编程错误 */
    NRC_73 = 0x73,      /* 错误的序列模块计数 */
    NRC_78 = 0x78,      /* 操作还未执行完 */
    NRC_7E = 0x7E,      /* 当前会话不支持该子功能 */
    NRC_7F = 0x7F,      /* 当前会话不支持该服务 */
    NRC_TYPE_MAX
} NRC_TYPE_E;



typedef enum {
    RESET_HARD  = 0x01,      /* 硬件复位 */
    RESET_KEY   = 0x02,      /* 开关键复位 */
    RESET_SOFT  = 0x03,      /* 软件复位 */
    RESET_TYPE_MAX
} RESET_TYPE_E;             /* 复位类型*/

typedef enum {
    REPORT_NUM       = 0x01,        /* 通过状态掩码报告DTC的数 */
    REPORT_DTC       = 0x02,        /* 通过状态掩码报告DTC */
    REPORT_SNAPSHOT  = 0x04,        /* 通过DTC报告Snapshot的记录 */
    REPORT_EXTENDED  = 0x06,        /* 通过DTC报告扩展数据记录 */
    REPORT_SUPPORTED = 0x0A,        /* 报告支持的DTC */
    READDTC_INFO_MAX
} READDTC_INFO_E;               /* 读取故障信息*/

typedef enum {
    REQSEED_1   = 0x01,        /* 扩展安全级LEVEL_1 */
    REQSEED_3   = 0x03,        /* 重编程安全级LEVEL_2 */    
    REQSEED_5   = 0x05,        /* 重编程安全级LEVEL_2 */
    REQSEED_9   = 0x09,        /* 防盗安全级LEVEL_3 */
    REQSEED_TYPE_MAX
} REQSEED_TYPE_E;          /* 安全访问的请求种子类型   */

typedef enum {
    UDS_COMCTR_ENABLE_RXTX  = 0x00,         /* 使能收发 */
    UDS_COMCTR_DISABLE_TX   = 0x01,         /* 使能收禁止发 */
    UDS_COMCTR_DISABLE_RX   = 0x02,         /* 禁止收使能发 */
    UDS_COMCTR_DISABLE_ALL  = 0x03,         /* 禁止收发 */
} UDS_COM_CTR_E;

typedef enum {
    UDS_COMTYPE_APP                    = 0x01,                       /* 常规应用报文 */
    UDS_COMTYPE_NETWORK                = 0x02,                       /* 网络管理报文 */
    UDS_COMTYPE_ALL                    = 0x03,                       /* 常规应用和网络管理报文 */
} UDS_COM_TYPE_E;

typedef enum {
    CTLPARA_RETURNCTL     = 0x00,        /* 归还电控单元控制权 */
    CTLPARA_RESETDEFAULT  = 0x01,        /* 复位至默认值 */
    CTLPARA_FREEZECURSTA  = 0x02,        /* 冻结当前状态 */
    CTLPARA_SHORTTERJMT   = 0x03,        /* 暂时调整 */
    CTLPARA_MAX
} IO_CTLPARA_E;          /* 输入输出控制参数 */

typedef enum {
    ROUTINE_START       = 0x01,         /* 启动例程 */
    ROUTINE_STOP        = 0x02,         /* 停止例程 */
    ROUTINE_REQRESULTS  = 0x03,         /* 请求例程结果 */
    ROUTINE_MAX
} ROUTINE_CTLTYPE_E;                /* 例程控制类型 */
typedef enum {
    SESSION_DEFAULT  = 0x01,      /* 默认会话 */
    SESSION_PROGRAM  = 0x02,      /* 编程会话 */
    SESSION_EXTENDED = 0x03,      /* 扩展诊断会话 */
    SESSION_TYPE_MAX
} SESSION_TYPE_E;               /*会话类型*/

typedef enum {
    MASK_01 = 0x01,     /* 测试失效 */
    MASK_02 = 0x02,     /* 本检测周期测试失效 */
    MASK_04 = 0x04,     /* 等待DTC */
    MASK_08 = 0x08,     /* 确认DTC */
    MASK_10 = 0x10,     /* 上次清零后测试未完成 */
    MASK_20 = 0x20,     /* 上次清零后测试失效 */
    MASK_40 = 0x40,     /* 本检测周期测试未完成 */
    MASK_80 = 0x80      /* 警告指示位请求 */
} DTCSTATUS_MASK_E;

typedef enum {
    OP_SET = 0x01,     /* 设置DTC状态 */
    OP_CLR = 0x02,     /* 清除DTC状态 */
} DTCSTATUS_OP_E;

/*******************************************************************
** 函数名: YX_UDS_CanSendSingle
** 函数描述: 单帧数据
** 参数: [in] data    : 内容
         [in] datalen : 内容长度
** 返回: TRUE:发送成功, FALSE:发送失败
********************************************************************/
BOOLEAN YX_UDS_CanSendSingle(INT8U* data, INT8U datalen);

/*******************************************************************
** 函数名:     YX_UDS_SendMulCanData
** 函数描述:   发送多帧数据
** 参数:       [in] data          需发送的数据
               [in] datalen       发送数据长度
** 返回:       NULL
********************************************************************/
void YX_UDS_SendMulCanData(INT8U *data, INT16U datalen);

/*******************************************************************
** 函数名: YX_EXT_PRO_Send
** 函数描述: 通过标识符将控制请求传给主机模块
** 参数: [in] type: 功能表示
         [in] flow: 流水号
         [in] data: 控制数据
         [in] len: 数据长度
** 返回: NULL
********************************************************************/
void YX_EXT_PRO_Send(INT8U type, INT16U flow, INT8U* data, INT16U len);

void Req_CmnctCtl(INT8U ctltype, INT8U cmncttype);

/*******************************************************************
** 函数名: YX_UDS_NegativeResponse
** 函数描述: 服务否定响应
** 参数: [in] sid: 服务ID
         [in] nrcode: 否定响应码
** 返回: NULL
********************************************************************/
void YX_UDS_NegativeResponse(INT8U sid, INT8U nrcode);

/*******************************************************************
** 函数名: YX_UDS_Recv
** 描述: 接收到诊断请求
** 参数: [in] is_mul_frame : TRUE:多帧， FALSE:单帧
         [in] reqid: 请求ID
         [in] data: 数据内容
         [in] datalen: 数据长度
** 返回: 处理结果(ture/false)
********************************************************************/
BOOLEAN YX_UDS_Recv(BOOLEAN is_mul_frame, INT32U reqid, INT8U *data, INT16U datalen);
/*******************************************************************
** 函数名:      YX_UDS_SingleFrameRecv
** 函数描述:    UDS单帧接收处理
** 参数:        [in] data:              源数据
                [in] len:               源数据长度
                [in] type:              所属系统类型
** 返回:        NULL
********************************************************************/
BOOLEAN YX_UDS_MultFrameRecv(INT32U id, INT8U* data, INT16U len);
/*******************************************************************
** 函数名: YX_UDS_GetSession
** 函数描述: 获取当前会话类型
** 参数: NULL
** 返回: 当前会话类型:@SESSION_TYPE_E
********************************************************************/
SESSION_TYPE_E YX_UDS_GetSession(void);

/*******************************************************************
** 函数名: YX_UDS_GetSession
** 函数描述: 获取当前会话类型
** 参数: NULL
** 返回: 当前安全访问等级
********************************************************************/
INT8U YX_UDS_GetSecurity(void);

/*******************************************************************
** 函数名: YX_UDS_GetDtcEnableSta
** 函数描述: 获取UDS故障码使能状态
** 参数: NULL
** 返回: UDS故障码使能状态
********************************************************************/
BOOLEAN YX_UDS_GetDtcEnableSta(void);

/*******************************************************************
** 函数名: YX_UDS_GetCanSendEnableSta
** 函数描述: can发送使能
** 参数: 无
** 返回: 无
********************************************************************/
BOOLEAN YX_UDS_GetCanSendEnableSta(void);

/*******************************************************************
** 函数名: YX_UDS_GetCanRecvEnableSta
** 函数描述: can接收使能
** 参数: 无
** 返回: 无
********************************************************************/
BOOLEAN YX_UDS_GetCanRecvEnableSta(void);

/*******************************************************************
** 函数名: YX_UDS_Init
** 函数描述: 统一诊断服务初始化
** 参数: 无
** 返回: 无
********************************************************************/
void YX_UDS_Init(void);
/*****************************************************************************
**  函数名:  UDS_SingleFrameHdl
**  函数描述: uds单帧处理
**  参数:    [in] com     : 通道号
**           [in] candata : 报文
**  返回:    0:执行失败 1:执行成功
*****************************************************************************/
BOOLEAN UDS_SingleFrameHdl(INT8U* data, INT8U datalen);
#endif
