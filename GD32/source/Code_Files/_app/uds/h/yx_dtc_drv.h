/*
********************************************************************************
** 文件名:     yx_dtc_drv.h
** 版权所有:   (c) 2017 厦门雅迅网络股份有限公司
** 文件描述:   故障诊断
** 创建人：    阙存先，2019.12.30
********************************************************************************
*/
#ifndef YX_DTC_DRV_H_
#define YX_DTC_DRV_H_
    
/*
********************************************************************************
* 定义结构
********************************************************************************
*/
typedef enum {
    EN_MASK_85_IS_SET      = 0x01,       /* 85服务设置了DTC使能 */   
    EN_MASK_KL15_ON        = 0x02,       /* KL15 ON(ACC ON) */
    EN_MASK_VOL_NORMAL     = 0x04,       /* 12V电源范围满足9~16V; 24V电源范围满足18~32V */    
    EN_MASK_NO_BUS_OFF     = 0x08,       /* 无bus off */
    EN_MASK_DIG_CONFIG     = 0x10,       /* did 0xF110配置使能 */
} EN_MASK_BIT_E;

/* 显示码 */
typedef enum {
    U000100,    // CAN1 BusOff//
    U003700,    // CAN2 BusOff//
    B157216,    // 终端电源电压欠压//
    B157F00,    // 备用电池电压低//
    
    U000200,    // 所有伙伴ECU超时//
    U010000,    // EMS节点超时//
    U015500,    // IC仪表节点超时//
    U014000,    // BCM节点超时//
    U010100,    // TCU节点超时//
    U012A00,    // Retarder节点超时//
    U012200,    // ABS/EBS节点超时//
    U012700,    // TPMS节点超时//
    U013200,    // ECAS节点超时//
    U103C00,    // GCT节点超时//
    U103B00,    // IBS节点超时//
    B158000,    // VIST节点USB通讯失效 //
    U014600,    // GATWAY节点超时
    B156D12,    // GPS模块故障//
    B156F12,    // 4G模块故障//
    B157112,    // SIM卡故障//
    B157B00,    // T-box 未定位 //
    //B157E00,    // T-box 专网拨号不成功
    B157800,    // 国六模块获取发动机VIN失败 //
    B157A00,    // 国六模块获取发动机VIN不一致 //
    B157C00,    // wifi 热点(公网)拨号不成功 //
    B158200,    // 国六地方平台连接失败  //
    B158300,    // 国六企业平台连接失败  //
    B157513,    // 4G天线开路//
    B157900,    // 国六模块电源线束断开//
    B156E11,    // GPS短路//
    B156E13,    // GPS开路//

    MAX_DISP_CODE
} DTC_DISP_CODE_E;

typedef struct {
    INT16U set[MAX_DISP_CODE];             /* 故障成熟计数 */
    INT16U clr[MAX_DISP_CODE];             /* 故障清除计数 */
    INT16U self_clr_cnt[MAX_DISP_CODE];    /* 自清除计数 */
} DTC_CNT_T;

typedef struct {
    INT32U display_code;                  // DTC显示码
    INT32U dtc_code;                      // DTC故障码
    INT8U  en_mask;                       // DTC使能条件(见EN_MASK_BIT_E)
    BOOLEAN is_self_clr;                  // 是否自清除
    INT8U   self_clr_times;               // 自清除次数(单位1)
    BOOLEAN (*set_cond)(INT8U dtc_id);    // 判断条件
    INT16U  confirm_times;                // 确定时间(单位10ms)或次数(单位1)
    INT16U  recv_times;                   // 恢复时间(单位10ms)或次数(单位1)
    //BOOLEAN isSaveHistory;                // 是否保存历史故
} DTC_REG_T;

/* DM1故障码 */
typedef union {
    INT8U DM1_DTC[4];
    struct {
        unsigned _SPN_LOW : 16;   // 可疑参数编号SPN低字节
        unsigned _SPN_HIG : 3;    // 可疑参数编号SPN高3位
        unsigned _FMI     : 5;    // 故障模式标志
        unsigned _CM      : 1;    // 可以参数编号的转化方式(默认0)
        unsigned _OC      : 7;    // 发生次数
    } bitFiled;
} DM1_DTC_T;

/*******************************************************************
** 函数名: YX_DTC_SID14_ClearDiagnosticInformation
** 函数描述: 清除诊断信息
** 参数: [in] dtcdata: DTC组或特定DTC
         [in] len: 数据长度
** 返回: NULL
********************************************************************/
void YX_DTC_SID14_ClearDiagnosticInformation(INT8U *data, INT16U len);

/*******************************************************************
** 函数名: YX_DTC_SID19_ReadDTCInformation
** 函数描述: 读取DTC信息
** 参数: [in] data: 包含sub的数据
         [in] len: 数据长度
** 返回: NULL
********************************************************************/
void YX_DTC_SID19_ReadDTCInformation(INT8U *data, INT16U len);

/*******************************************************************************
 **  函数名称:  YX_DTC_NodeIdCheck
 **  功能描述:  通信丢失诊断报文
 **  输入参数:  [in] node_id : 节点丢失id
 **  返回参数:  无
 ******************************************************************************/
void YX_DTC_NodeIdCheck(INT8U ch, INT32U node_id);

/*******************************************************************************
** 函数名:     YX_DTC_PP_Update_When_Reset
** 函数描述: 11服务复位时dtc的pp参数更新
** 参数:       无
** 返回:       无
******************************************************************************/
void YX_DTC_PP_Update_When_Reset(void);

/*****************************************************************************
**  函数名:  YX_DTC_SetBusOffFlag
**  函数描述: 设置bus off标志
**  参数:    [in] com  :
**           [in] flag :
**  返回:    无
*****************************************************************************/
void YX_DTC_SetBusOffFlag(INT8U com, INT8U flag);

/*****************************************************************************
**  函数名:  YX_DTC_SetMpuStatus
**  函数描述: 主机故障状态
**  参数:    [in] dtcCode : dtc码
**           [in] status  : 状态 
**  返回:    无
*****************************************************************************/
BOOLEAN YX_DTC_SetMpuStatus(INT8U dtcCode, BOOLEAN status);

/*****************************************************************************
**  函数名:  YX_DTC_GetStatus
**  函数描述  : 获取故障码状态 
**  参数:    [out] dtcBuf  : 
             [out] dtcBufLen : 
**  返回:    TRUE:有故障 FALSE:无故障
*****************************************************************************/
BOOLEAN YX_DTC_GetStatus(INT8U* dtcBuf, INT8U* dtcBufLen);
/*****************************************************************************
**  函数名:  YX_CAR_SIGNAL_AdapterDTC
**  函数描述  : 获取故障码状态 
**  参数:    [out] dtcBuf  : 
             [out] dtcBufLen : 
**  返回:    TRUE:有故障 FALSE:无故障
*****************************************************************************/
void YX_CarSignal_AdapterDTC(INT8U type,INT8U *data);
/*******************************************************************************
 ** 函数名:    YX_DTC_Init
 ** 函数描述:   信号量初始化
 ** 参数:       无
 ** 返回:       无
 ******************************************************************************/
void YX_DTC_Init(void);
    
#endif

