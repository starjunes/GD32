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
//#include  "app_can.h"
#define _PUBLIC_FLASHSECTOR             (E_USRPARA_PAGE + 1 - BLOCK_IMAGESECTOR)     /* FLASH扇区号 */
#define MAX_PARA_SIZE  	                500     /* 注意,公共参数结构最大不得超过这个值 */

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

