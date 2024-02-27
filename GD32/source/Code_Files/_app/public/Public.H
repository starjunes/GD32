/**************************************************************************************************
**                                                                                               **
**  文件名称:  Public.H                                                                          **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2010                                    **
**  创建信息:  jump -- 2011年1月20日                                                             **
**  文件描述:  系统参数管理                                                                      **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/

#ifndef PUBLIC_H
#define PUBLIC_H                        1

#include  "dal_can.h"
#if EN_UDS > 0
#include "yx_dtc_drv.h"
#endif
//#include  "app_can.h"
#define _PUBLIC_FLASHSECTOR             (E_USRPARA_PAGE + 1 - BLOCK_IMAGESECTOR)     /* FLASH扇区号 */
#define MAX_PARA_SIZE  	                500     /* 注意,公共参数结构最大不得超过这个值 */
#if EN_UDS > 0
#define MAX_DID_NUM              22
/* 故障码信息 */
typedef struct {
    INT8U status;                       /* 故障状态 */
    INT8U self_clr_cnt;                 /* 自清除计数 */
} DTC_T; 

typedef struct {
    DTC_T  dtc[MAX_DISP_CODE];                     /* 与MAX_DISP_CODE大小要一致 */         
} DTC_OBJ_T; 
/* uds错误计数 */
typedef struct {
    INT16U  fault_cnt;
} UDS_ERR_CNT_T;
/* UDS DID数据 */
typedef struct {
    INT8U DID_F182[24];    // 17数据信息标识    
    INT8U DID_F187[24];    // 整车厂零件号（图号）
    INT8U DID_F190[17];    // 车辆标识号 VIN
    INT8U DID_F193[8];     // 硬件版本号 
    INT8U DID_F195[8];     // 软件版本号
    INT8U DID_F19D[4];     // 安装日期
    INT8U DID_F1A1[16];    // 厂家代码
    INT8U DID_1004[15];    // IMSI

    INT8U DID_STATUS[MAX_DID_NUM];
} UDS_DID_DATA_E2ROM_T;
#endif
/*
********************************************************************************
*                  DEFINE PUBLIC PARAMETERS TYPE
********************************************************************************
*/
#define SCLOCKPARA_                     0
#define LOCKRECORD_                     1
#define CALIBRATION_                    2
#define RESETREC_                       3
#define AXISCALPARA_                    4 
#define SCLOCKPARABAK_                  5
#define KMS_LOCK_PARA_                  6
#if EN_UDS > 0
#define UDS_DTC_PARA_                   7
#define UDS_DID_PARA_                   8
#define UDS_ERRCNT_PARA_                9
#endif
#ifdef PUBLIC_C
/*
********************************************************************************
*                  DEFINE PUBLIC INTIALIZE PARAMETERS
********************************************************************************
*/

static ROUTER_STRUCT    const i_TELROUTER     = {0xFF};
static SETCONFIG_STRUCT const i_SETCONFIG     = {6, true, 3};
static BT_PIN_STRUCT    const i_BT_PIN        = {0, "\"123456\"", TRUE};
static CAN_ATTR_T       const i_CAN1          = {TRUE, CAN_250, DEF_FTYPE, DEF_FMAT, CAN_NORMAL_MODE, CAN_UN_SET, CAN_UN_SET, CAN_UN_SET, CAN_FILTER_ON};
static CAN_ATTR_T       const i_CAN2          = {TRUE, CAN_500, DEF_FTYPE, DEF_FMAT, CAN_NORMAL_MODE, CAN_UN_SET, CAN_UN_SET, CAN_UN_SET, CAN_FILTER_ON};
static ICPARA_T         const i_IC            = {0};
static ICCANPARA_T      const i_iccan         = {1,1,1,1,1}; /* 配置卡默认参数发送CAN车速 */
static SCLOCKPARA_T     const i_sclockpara    = {0xff,
                                                 0x02,{0xaa,0xbb,0xcc,0xaa,0xbb,0xcc,0xaa,0xbb,0xcc,0xaa,0xbb,0xcc,0xaa,0xbb,0xcc,0xaa,0xbb,0xcc,0xaa,0xbb},
                                                 0x04,{0xaa,0xbb,0xcc,0xaa,0xbb,0xcc,0xaa,0xbb,0xcc,0xaa,0xbb,0xcc,0xaa,0xbb,0xcc,0xaa,0xbb,0xcc,0xaa,0xbb},
                                                 0x04,{0xaa,0xbb,0xcc,0xaa,0xbb,0xcc,0xaa,0xbb,0xcc,0xaa,0xbb,0xcc,0xaa,0xbb,0xcc,0xaa,0xbb,0xcc,0xaa,0xbb},
                                                 0x04,{0xaa,0xbb,0xcc,0xaa,0xbb,0xcc,0xaa,0xbb,0xcc,0xaa,0xbb,0xcc,0xaa,0xbb,0xcc,0xaa,0xbb,0xcc,0xaa,0xbb},
                                                 0x01,
                                                 0x06,TRUE,
                                                 250, 250, 250};
static LOCK_RECORD      const i_LOCKRECORD    = {0,0,{{{0,0,0,0,0,0},0},{{0,0,0,0,0,0},0},{{0,0,0,0,0,0},0},{{0,0,0,0,0,0},0},
                                                {{0,0,0,0,0,0},0},{{0,0,0,0,0,0},0},{{0,0,0,0,0,0},0},{{0,0,0,0,0,0},0},{{0,0,0,0,0,0},0},{{0,0,0,0,0,0},0}}};
static GSENSOR_CALIBRATION_T  const i_GSENSOR_CALIBRATION   = {0, 0, 0, 0};
static RESETREC_T       const i_RESETREC      = {0, 0, 0, 0, 0, 0};
static CAN_RELAY_T      const i_CANRELAY      = {0};
static EXLOCKPARA_T     const i_EXLOCKPARA    = {0};
static ADASPARA_T       const i_ADASPARA      = {0};
static AXISPARA_T  		const i_axiscalconfig = {false, 2, -41, 997, 4000};
static SCLOCKPARABAK_T  const i_sclockparabak = {0, 0, 0, 0x01, 0x01, 0x02};
static KMS_LOCK_OBJ_T   const i_kms_lock      = {0x00, {0xff, 0xff}};
#if EN_UDS > 0
static DTC_OBJ_T const i_dtc_obj = {
    {
        {0x00, 0x00},    // bus off 
        {0x00, 0x00},    // 欠压
        {0x00, 0x00},    // 过压

        {0x00, 0x00},    // 所有伙伴ECU超时（J6低配节点超时）    
        {0x00, 0x00},    // EMS节点超时  
        {0x00, 0x00},    // IC仪表节点超时  

        {0x00, 0x00},    // GPS模块故障
        {0x00, 0x00},    // 4G模块故障
        {0x00, 0x00},    // SIM卡故障
        {0x00, 0x00},    // T-box 未定位
        {0x00, 0x00},    // T-box 专网拨号不成功
        {0x00, 0x00},    // 国六模块获取发动机VIN失败
        {0x00, 0x00},    // 国六模块获取发动机VIN不一致
        {0x00, 0x00},    // 国六企业平台连接失败
        {0x00, 0x00},    // 国六模块电源线束断开
    },
};
static const UDS_DID_DATA_E2ROM_T     i_uds_did_e2rom_data = {
    /* DID_F182 */
    {'7', '9', '2', '5', '6', '1', '2', '-', 'Y', '1', '8', '5', '6', '-', 'C', '0', '0',  
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    },
    /* DID_F187 */
    {'7', '9', '2', '5', '1', '0', '0', '-', 'Y', '1', '8', '5', '6', '-', 'C', '0', '0', 
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }, 
    /* DID_F190 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
     0x00 
    }, 
    /* DID_F193 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }, 
    /* DID_F195 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }, 
    /* DID_F19D */
    {0x00, 0x00, 0x00, 0x00
    }, 
    /* DID_F1A1 */
    {'L', 'L', 'L', '2', '1', 
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
     
    }, 
    /* DID_1004 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }, 

    /* DID_STATUS */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }, 
};
static UDS_ERR_CNT_T const i_uds_fault_cnt = {0};
#endif
//static VEHICHEFACTOR_T  const i_VEHICHEFACTOR = {0};
/*
********************************************************************************
* define PUBTBL_STRUCT
********************************************************************************
*/
typedef struct {
    void const  *i_ptr;                      /* 指向初始化区 */
    INT8U       len;                         /* 长度 */
} PUBTBL_STRUCT;

static PUBTBL_STRUCT const PubTbl[] = {
            {&i_sclockpara,             sizeof(SCLOCKPARA_T)},
            {&i_LOCKRECORD,             sizeof(LOCK_RECORD)},
            {&i_GSENSOR_CALIBRATION,    sizeof(GSENSOR_CALIBRATION_T)},
            {&i_RESETREC,               sizeof(RESETREC_T)},
            {&i_axiscalconfig,          sizeof(AXISPARA_T)}, 
            {&i_sclockparabak,          sizeof(SCLOCKPARABAK_T)},
            {&i_kms_lock,               sizeof(KMS_LOCK_OBJ_T)},
            #if EN_UDS > 0
            {&i_dtc_obj,                sizeof(DTC_OBJ_T)},
            {&i_uds_did_e2rom_data,     sizeof(UDS_DID_DATA_E2ROM_T)},
            {&i_uds_fault_cnt,          sizeof(UDS_ERR_CNT_T)},
            #endif
          };
#endif

void   *GetPubParaAddr(BOOLEAN isimage, INT8U ParaID);
BOOLEAN GetPubParaImage(void);
void    UpdatePubParaImage(void);
BOOLEAN PubParaValid(INT8U ParaID);
INT8U   ReadPubPara(INT8U ParaID, void *ptr);
void    StorePubPara(INT8U ParaID, void *ptr);
void    ClearPubPara(INT8U ParaID);
void    ResumeAllPubPara(void);
void    InitAllPubPara(void);

/**************************************************************************************************
**  函数名称:  GetDefaulValue
**  功能描述:  获取pp参数默认值
**  输入参数:
**  返回参数:
**************************************************************************************************/
void const * GetDefaulValue(INT8U ParaID);

#if DEBUG_PARA > 0
void Log_All_Para(void);
#endif
void BakParaInquire(void);
void BakParaHdl(INT8U *data, INT8U datalen);
void BakParaReqSingle(INT8U ParaID);
void BakParaReq(INT8U IdNum, INT8U *IdBuf);
void BakParaAllReq(void);

#endif

