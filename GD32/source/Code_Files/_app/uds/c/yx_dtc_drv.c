/*
********************************************************************************
** 文件名:     yx_dtc_drv.c
** 版权所有:   (c) 2017 厦门雅迅网络股份有限公司
** 文件描述:   故障诊断 
** 创建人：    阙存先，2019.12.30
********************************************************************************
*/
#include "yx_includes.h"
#include "bal_stream.h"
#include "yx_dtc_drv.h"
#include "dal_can.h"
#include "yx_uds_drv.h"
#include "bal_input_drv.h"
#include "bal_output_drv.h"
#include "port_adc.h"
#include "bal_pp_drv.h"
#include "port_dm.h"
#include "yx_uds_did.h"
#include "bal_gpio_cfg.h"
#include "port_can.h"
#if EN_UDS > 0

/*
********************************************************************************
* 定义宏
********************************************************************************
*/
#if EN_DEBUG > 0

#undef DEBUG_DTC_BUS_OFF
#define DEBUG_DTC_BUS_OFF   0

#undef DEBUG_DTC_PARA_SAVE 
#define DEBUG_DTC_PARA_SAVE 0

#undef DEBUG_MAIN_PWR
#define DEBUG_MAIN_PWR      0

#undef  DEBUG_DTC
#define DEBUG_DTC           0

#undef  DEBUG_NODE_LOST
#define DEBUG_NODE_LOST     0

#else
#undef DEBUG_DTC_BUS_OFF
#define DEBUG_DTC_BUS_OFF   0

#undef DEBUG_DTC_PARA_SAVE 
#define DEBUG_DTC_PARA_SAVE 0

#undef DEBUG_MAIN_PWR
#define DEBUG_MAIN_PWR      0

#undef  DEBUG_DTC
#define DEBUG_DTC           0

#undef  DEBUG_NODE_LOST
#define DEBUG_NODE_LOST     0

#endif

#define EN_DTC_SYSC         0

#define DTC_CAN_CH0         CAN_CHN_1
#define DTC_CAN_CH1         CAN_CHN_2

#define MAX_UPDATE_DELAY    500  /* 5秒延时保存pp参数 */

#define KL15_EN_TIME        160  /* 1600ms启动诊断(规范标准为1500~2000ms) */

#define MIN_DTC_ID          U000100

//节点丢失相关
#define ID_NODE_LOST_START  U000200                                        // 节点丢失编号最小值(显示码编号)
#define ID_NODE_LOST_END    U014600                                        // 节点丢失编号最大值(显示码编号)
#define MAX_NODE_LOST_NUM   (ID_NODE_LOST_END - ID_NODE_LOST_START + 1)    // 节点丢失总数   

//杂项故障相关
#define ID_MISC_START       B156D12                                        // MISC编号最小值(显示码编号)
#define ID_MISC_END         B157900                                        // MISC编号最大值(显示码编号)
#define MAX_MISC_NUM        (ID_MISC_END - ID_MISC_START + 1)              // MISC总数    

#define MAIN_POW_LOW_19_5V   1736         // 19.5V(实测)
#define MAIN_POW_RECV_20_5V  1817        // 20.5V(实测) 

#define MAIN_POW_CLOSE_CAN_9V    745     // 9V(实测)
#define MAIN_POW_OPEN_CAN_10V    853     // 10V(实测) 

#define MAIN_POW_CLOSE_CAN_36V   3220    // 36V(实测)
#define MAIN_POW_OPEN_CAN_35V    3125    // 35V  (实测) 

#define MAIN_POW_HIGH_30_5V      2730        // 30.5V(实测)

#define BAT_POW_LOW_3V       1500        // 3.0V
#define BAT_POW_RECV_3_3V    1650        // 3.3V

/*
********************************************************************************
* 定义结构
********************************************************************************
*/

/* DTC记录使能条件 */
typedef struct {
    BOOLEAN can0_bus_off_en;
    BOOLEAN can1_bus_off_en;
    BOOLEAN kl15_en;
    BOOLEAN main_pwr_en;
} DTC_EN_T;

typedef struct {
    BOOLEAN is_detect;             // FALSE:节点丢失 TRUE:收到节点
    INT16U  node_lost_time_out;    // 节点丢失超时计数
} NODE_LOST_T;

typedef struct {
    INT32U id;                    // 节点can id
    INT16U timeout;               // 节点丢失超时计数(单位10ms)
    NODE_LOST_T* nodecheck;       // 节点丢失判断结构
} NODE_LOST_OBJ_T;

/* 更新dtc记录结构 */
typedef struct {
    BOOLEAN is_update;
    INT16U  update_delay;
} UPDATA_DTC_REC_T;

static DM1_DTC_T s_dm1_dtc_tab[MAX_DISP_CODE];
static INT8U   s_canonoff_delay = 0;
static BOOLEAN s_canonoff = TRUE;          
/*
********************************************************************************
* 定义配置参数
********************************************************************************
*/
static INT8U s_dtc_tmr;

/* dtc使能结构 */
static DTC_EN_T s_dtc_en;

/* 故障计数 */
static DTC_CNT_T s_dtc_cnt;

/* 欠压、过压状态 */
static BOOLEAN s_main_pow_low_sta = FALSE;
static BOOLEAN s_bat_pow_low_sta = FALSE;

/* 节点丢失结构 */
static NODE_LOST_T s_node_lost[MAX_NODE_LOST_NUM];
static const NODE_LOST_OBJ_T s_nodelost_obj[MAX_NODE_LOST_NUM] = {
    {0x00,       500,   &s_node_lost[0]},    // 所有伙伴ECU超时（J6低配节点超时）  
    {0x18FEF100, 100,   &s_node_lost[1]},    // EMS节点超时
    {0x0CFE6C17, 50,    &s_node_lost[2]},    // IC仪表节点超时    
    {0x18D00021, 100,   &s_node_lost[3]},    // BCM节点超时//
    {0x18FF6003, 50,    &s_node_lost[4]},    // TCU节点超时//
    {0x18F00010, 100,   &s_node_lost[5]},    // Retarder节点超时//
    {0x18F0010B, 100,   &s_node_lost[6]},    // ABS/EBS节点超时//
    {0x18FEF433, 500,   &s_node_lost[7]},    // TPMS节点超时//
    {0x18FE582F, 100,   &s_node_lost[8]},    // ECAS节点超时//
    {0x18FEFCC6, 500,   &s_node_lost[9]},    // GCT节点超时//
    {0x18FC08F4, 100,   &s_node_lost[10]},    // IBS节点超时//
    {0x18FF0241, 100,   &s_node_lost[11]},    // VIST节点USB通讯失效 //
    {0x18FF4FF4, 100,   &s_node_lost[12]},    // gatway节点超时 //
};
static INT8U s_support_dtc[MAX_DISP_CODE]; /* 支持DTC */
static INT8U s_onoff_dtc[MAX_DISP_CODE];   /* 开启检测对应DTC */
/* 杂项故障 */
static INT8U s_misc_flag[MAX_MISC_NUM];

// MPU故障
static INT16U s_mpu_dtc_sig_sta = 0;

/* 更新dtc的pp参数结构 */
static UPDATA_DTC_REC_T s_dtc_update = {FALSE, 0};

static DTC_OBJ_T s_dtc_obj;

static BOOLEAN MainPowerIsLow(INT8U dtc_id);
static BOOLEAN BatPowerIsLow(INT8U dtc_id);
static BOOLEAN BusIsOff(INT8U dtc_id);
static BOOLEAN NodeIsLost(INT8U dtc_id);
static BOOLEAN MiscIsDetect(INT8U dtc_id);

static const DTC_REG_T s_obj_dtc_tbl[] = {
    /* 基础故障类型 */
    {U000100, 0xC00100, (EN_MASK_85_IS_SET | EN_MASK_KL15_ON | EN_MASK_VOL_NORMAL),                      0, 40, BusIsOff ,         1,        1},     // CAN1 BusOff
    {U003700, 0xC03700, (EN_MASK_85_IS_SET | EN_MASK_KL15_ON | EN_MASK_VOL_NORMAL),                      0, 40, BusIsOff ,         1,        1},     // CAN1 BusOff
    {B157216, 0x957216, (EN_MASK_85_IS_SET | EN_MASK_KL15_ON),                                           0, 40, MainPowerIsLow,    200,     200},     // 终端电源电压欠压(<19.5v故障，恢复到20.5v解除故障)
    {B157F00, 0x957F00, (EN_MASK_85_IS_SET | EN_MASK_KL15_ON),                                           0, 40, BatPowerIsLow,     200,     200},    // 备用电池电压低(<3v故障，恢复到3.3解除故障)
    /* 节点丢失故障类型 */
    {U000200, 0xC00200, (EN_MASK_85_IS_SET | EN_MASK_VOL_NORMAL | EN_MASK_NO_BUS_OFF | EN_MASK_KL15_ON), 0, 40, NodeIsLost,        1,         1},     // 所有伙伴ECU超时（J6低配节点超时）  
    {U010000, 0xC10000, (EN_MASK_85_IS_SET | EN_MASK_VOL_NORMAL | EN_MASK_NO_BUS_OFF | EN_MASK_KL15_ON), 0, 40, NodeIsLost,        1,         1},     // EMS节点超时  
    {U015500, 0xC15500, (EN_MASK_85_IS_SET | EN_MASK_VOL_NORMAL | EN_MASK_NO_BUS_OFF | EN_MASK_KL15_ON), 0, 40, NodeIsLost,        1,         1},     // IC仪表节点超时  
			
		{U014000, 0xC14000, (EN_MASK_85_IS_SET | EN_MASK_VOL_NORMAL | EN_MASK_NO_BUS_OFF | EN_MASK_KL15_ON), 0, 40, NodeIsLost,        1,         1},     // BCM节点超时//
		{U010100, 0xC10100, (EN_MASK_85_IS_SET | EN_MASK_VOL_NORMAL | EN_MASK_NO_BUS_OFF | EN_MASK_KL15_ON), 0, 40, NodeIsLost,        1,         1},     // TCU节点超时//
		{U012A00, 0xC12A00, (EN_MASK_85_IS_SET | EN_MASK_VOL_NORMAL | EN_MASK_NO_BUS_OFF | EN_MASK_KL15_ON), 0, 40, NodeIsLost,        1,         1},     // Retarder节点超时//
		{U012200, 0xC12200, (EN_MASK_85_IS_SET | EN_MASK_VOL_NORMAL | EN_MASK_NO_BUS_OFF | EN_MASK_KL15_ON), 0, 40, NodeIsLost,        1,         1},     // ABS/EBS节点超时//
		{U012700, 0xC12700, (EN_MASK_85_IS_SET | EN_MASK_VOL_NORMAL | EN_MASK_NO_BUS_OFF | EN_MASK_KL15_ON), 0, 40, NodeIsLost,        1,         1},     // TPMS节点超时//
		{U013200, 0xC13200, (EN_MASK_85_IS_SET | EN_MASK_VOL_NORMAL | EN_MASK_NO_BUS_OFF | EN_MASK_KL15_ON), 0, 40, NodeIsLost,        1,         1},     // ECAS节点超时//
		{U103C00, 0xD03C00, (EN_MASK_85_IS_SET | EN_MASK_VOL_NORMAL | EN_MASK_NO_BUS_OFF | EN_MASK_KL15_ON), 0, 40, NodeIsLost,        1,         1},     // GCT节点超时//
		{U103B00, 0xD03B00, (EN_MASK_85_IS_SET | EN_MASK_VOL_NORMAL | EN_MASK_NO_BUS_OFF | EN_MASK_KL15_ON), 0, 40, NodeIsLost,        1,         1},     // IBS节点超时//
		{B158000, 0x958000, (EN_MASK_85_IS_SET | EN_MASK_VOL_NORMAL | EN_MASK_NO_BUS_OFF | EN_MASK_KL15_ON), 0, 40, NodeIsLost,        1,         1},     // VIST节点USB通讯失效 //
		{U014600, 0xC14600, (EN_MASK_85_IS_SET | EN_MASK_VOL_NORMAL | EN_MASK_NO_BUS_OFF | EN_MASK_KL15_ON), 0, 40, NodeIsLost, 			 1, 				1}, 		// GATWAY节点超时 //
		/* MPU端故障 */
    {B156D12, 0x956D12, (EN_MASK_85_IS_SET | EN_MASK_VOL_NORMAL | EN_MASK_KL15_ON),                      0, 40, MiscIsDetect,      200,      500},    // GPS模块故障
    {B156F12, 0x956F12, (EN_MASK_85_IS_SET | EN_MASK_VOL_NORMAL | EN_MASK_KL15_ON),                      0, 40, MiscIsDetect,      10,       500},    // 4G模块故障
    {B157112, 0x957112, (EN_MASK_85_IS_SET | EN_MASK_VOL_NORMAL | EN_MASK_KL15_ON),                      0, 40, MiscIsDetect,      10,       500},    // SIM卡故障
    {B157B00, 0x957B00, (EN_MASK_85_IS_SET | EN_MASK_VOL_NORMAL | EN_MASK_KL15_ON),                      0, 40, MiscIsDetect,      1,        10},    // T-box 未定位
    //{B157E00, 0x957E00, (EN_MASK_85_IS_SET | EN_MASK_VOL_NORMAL | EN_MASK_KL15_ON),                      0, 40, MiscIsDetect,      1,        10},    // T-box 专网拨号不成功
    {B157800, 0x957800, (EN_MASK_85_IS_SET | EN_MASK_VOL_NORMAL | EN_MASK_KL15_ON),                      0, 40, MiscIsDetect,      1,        10},    // 国六模块获取发动机VIN失败
    {B157A00, 0x957A00, (EN_MASK_85_IS_SET | EN_MASK_VOL_NORMAL | EN_MASK_KL15_ON),                      0, 40, MiscIsDetect,      1,        10},    // 国六模块获取发动机VIN不一致
		{B157C00, 0x957C00, (EN_MASK_85_IS_SET | EN_MASK_VOL_NORMAL | EN_MASK_KL15_ON), 										 0, 40, MiscIsDetect, 		 1, 			 10}, 	 // 国六模块获取发动机VIN不一致
		{B158200, 0x958200, (EN_MASK_85_IS_SET | EN_MASK_VOL_NORMAL | EN_MASK_KL15_ON),                      0, 40, MiscIsDetect,      1,        10},   // 国六企业平台连接失败
    {B158300, 0x958300, (EN_MASK_85_IS_SET | EN_MASK_VOL_NORMAL | EN_MASK_KL15_ON),                      0, 40, MiscIsDetect,      1,        10},    // 国六企业平台连接失败
		{B157511, 0x957511, (EN_MASK_85_IS_SET | EN_MASK_VOL_NORMAL | EN_MASK_KL15_ON), 										 0, 40, MiscIsDetect, 		 10,			 500},	 // 4G天线短路
		{B157513, 0x957513, (EN_MASK_85_IS_SET | EN_MASK_VOL_NORMAL | EN_MASK_KL15_ON),                      0, 40, MiscIsDetect,      10,       500},   // 4G天线开路
    {B156E11, 0x956E11, (EN_MASK_85_IS_SET | EN_MASK_VOL_NORMAL | EN_MASK_KL15_ON),	                     0, 40, MiscIsDetect, 	   1,		     100},	 // 终端检测gps开路
		{B156E13, 0x956E13, (EN_MASK_85_IS_SET | EN_MASK_VOL_NORMAL | EN_MASK_KL15_ON),		                   0, 40, MiscIsDetect, 	   1,		     100},	 // 终端检测gps短路
    {B157900, 0x957900, (EN_MASK_85_IS_SET /*| EN_MASK_VOL_NORMAL | EN_MASK_KL15_ON*/),                      0, 40, MiscIsDetect,      200,      200},    // 国六模块电源线束断开
};

/*
********************************************************************************
* 定义本地函数
********************************************************************************
*/
static void DTC_Dm1Init(void)
{
    INT8U idx;

    for (idx = 0; idx < MAX_DISP_CODE; idx++) {
        s_dm1_dtc_tab[idx].bitFiled._CM = 0;
        s_dm1_dtc_tab[idx].bitFiled._OC = 127;    // 127表示未知次数
        if ((idx == B157216) || (idx == B156E13)) {
            s_dm1_dtc_tab[idx].bitFiled._FMI = 5;
        } else if((idx == B156E11) || (idx == B157511)) {
            s_dm1_dtc_tab[idx].bitFiled._FMI = 6;
        } else {
            s_dm1_dtc_tab[idx].bitFiled._FMI = 12;
        }
        s_dm1_dtc_tab[idx].bitFiled._SPN_HIG = 0x7;
    }
    
    s_dm1_dtc_tab[B156D12].bitFiled._SPN_LOW = 0xE6FD;
    s_dm1_dtc_tab[B156F12].bitFiled._SPN_LOW = 0xE8FD;
    s_dm1_dtc_tab[B157112].bitFiled._SPN_LOW = 0xEAFD;
    s_dm1_dtc_tab[U000100].bitFiled._SPN_LOW = 0xEBFD;
    s_dm1_dtc_tab[U000200].bitFiled._SPN_LOW = 0xECFD;
    s_dm1_dtc_tab[U010000].bitFiled._SPN_LOW = 0xEDFD;
    s_dm1_dtc_tab[U015500].bitFiled._SPN_LOW = 0xEEFD;
    s_dm1_dtc_tab[B157800].bitFiled._SPN_LOW = 0x05FE;
    s_dm1_dtc_tab[B157900].bitFiled._SPN_LOW = 0x06FE;
    s_dm1_dtc_tab[B157A00].bitFiled._SPN_LOW = 0x07FE;
    s_dm1_dtc_tab[B158300].bitFiled._SPN_LOW = 0x10FE;
    s_dm1_dtc_tab[B157216].bitFiled._SPN_LOW = 0xEAFD;
    s_dm1_dtc_tab[B157F00].bitFiled._SPN_LOW = 0x0CFE;
    //s_dm1_dtc_tab[B157E00].bitFiled._SPN_LOW = 0x0BFE;
    s_dm1_dtc_tab[B157B00].bitFiled._SPN_LOW = 0x08FE; 
    
		s_dm1_dtc_tab[U014000].bitFiled._SPN_LOW = 0xEFFD;    // BCM节点超时//
    s_dm1_dtc_tab[U010100].bitFiled._SPN_LOW = 0xF1FD;    // TCU节点超时//
    s_dm1_dtc_tab[U012A00].bitFiled._SPN_LOW = 0xFBFD;    // Retarder节点超时//
    s_dm1_dtc_tab[U012200].bitFiled._SPN_LOW = 0x00FE;    // ABS/EBS节点超时//
    s_dm1_dtc_tab[U012700].bitFiled._SPN_LOW = 0xF5FD;    // TPMS节点超时//
    s_dm1_dtc_tab[U013200].bitFiled._SPN_LOW = 0xFAFD;    // ECAS节点超时//
    s_dm1_dtc_tab[U103C00].bitFiled._SPN_LOW = 0x01FE;    // GCT节点超时//
    s_dm1_dtc_tab[U103B00].bitFiled._SPN_LOW = 0x04FE;    // IBS节点超时//
    s_dm1_dtc_tab[B158000].bitFiled._SPN_LOW = 0x0DFE;    // VIST节点USB通讯失效 //
    s_dm1_dtc_tab[B157C00].bitFiled._SPN_LOW = 0x09FE;    // wifi 热点(公网)拨号不成功 //
    s_dm1_dtc_tab[B158200].bitFiled._SPN_LOW = 0x0FFE;    // 国六地方平台连接失败  //
    s_dm1_dtc_tab[B157511].bitFiled._SPN_LOW = 0xFCFD;    // 4G天线开路//
    s_dm1_dtc_tab[B157513].bitFiled._SPN_LOW = 0xFCFD;    // 4G天线开路//
    s_dm1_dtc_tab[B156E11].bitFiled._SPN_LOW = 0xE7FD;    // GPS短路//
    s_dm1_dtc_tab[B156E13].bitFiled._SPN_LOW = 0xE7FD;    // GPS开路//

    s_dm1_dtc_tab[U003700].bitFiled._SPN_LOW = 0xEAFD;    // GCAN2 BusOff//
    s_dm1_dtc_tab[U014600].bitFiled._SPN_LOW = 0xFDFD;    // GATWAY节点超时 //
}

/*******************************************************************************
** 函数名:    DTC_CheckEnable
** 函数描述:  是否满足检测使能条件
** 参数:       无
** 返回:       无
******************************************************************************/
static BOOLEAN DTC_CheckEnable(DTC_DISP_CODE_E dtcIdx, EN_MASK_BIT_E condition)
{
    dtcIdx = dtcIdx;
    
    if (condition & EN_MASK_85_IS_SET) {
        if (!YX_UDS_GetDtcEnableSta()) {
            return FALSE; 
        }
    }
    if (condition & EN_MASK_KL15_ON) {
        if (!s_dtc_en.kl15_en) {
             return FALSE; 
         }
    }
    if (condition & EN_MASK_VOL_NORMAL) {
        if (!s_dtc_en.main_pwr_en) {
            return FALSE; 
        }
    }
    if (condition & EN_MASK_NO_BUS_OFF) {
        if (!s_dtc_en.can0_bus_off_en) {
            return FALSE;            
        }
    }   
    
    return TRUE;
}

/*****************************************************************************
**  函数名:  DTC_ResetMiscFlag
**  函数描述: 重置杂项故障标志
**  参数:    无
**  返回:    无
*****************************************************************************/
static void DTC_ResetMiscFlag(void)
{
    YX_MEMSET(&s_misc_flag, 0x00, sizeof(s_misc_flag));   
}

/*****************************************************************************
**  函数名:  DTC_SetMiscFlag
**  函数描述: 设置Misc标志
**  参数:    [in] dtcIdx :
**  返回:    无
*****************************************************************************/
void DTC_ConfigMiscFlag(DTC_DISP_CODE_E dtcIdx)
{
    INT32U idx;
    if ((dtcIdx < ID_MISC_START) || (dtcIdx > ID_MISC_END)) {
        return;
    }
    
    idx = dtcIdx - ID_MISC_START;    
    s_misc_flag[idx] = 0x01;
}

/*****************************************************************************
**  函数名:  DTC_GetMiscFlag
**  函数描述: 获取杂项状态
**  参数:    [in] dtcIdx :
**  返回:    TRUE:故障 FALSE:无故障
*****************************************************************************/
static BOOLEAN DTC_GetMiscFlag(DTC_DISP_CODE_E dtcIdx)
{
    INT32U idx;
    if ((dtcIdx < ID_MISC_START) || (dtcIdx > ID_MISC_END)) {
        return FALSE;
    }
    
    idx = dtcIdx - ID_MISC_START;   
    if (s_misc_flag[idx]) {
        return TRUE;
    }
    return FALSE;    
}

/*****************************************************************************
**  函数名:  DTC_ClearNodeLostCnt
**  函数描述: 清除节点丢失计时
**  参数:    [in] dtc_idx : 节点丢失dtc编号
**  返回:    无
*****************************************************************************/
static void DTC_ClearNodeLostCnt(INT8U dtc_idx)
{
    INT8U idx;
    if ((dtc_idx < ID_NODE_LOST_START) || (dtc_idx > ID_NODE_LOST_END)) {
        return;
    }
    idx = dtc_idx - ID_NODE_LOST_START;
    s_node_lost[idx].is_detect = TRUE;
    s_node_lost[idx].node_lost_time_out = 0;
}    
/*****************************************************************************
**  函数名:  DTC_GetRegTblInfo
**  函数描述: 根据dtc_id获取注册表信息
**  参数:    [in] dtc_id : DTC显示码
**  返回:    注册表信息
*****************************************************************************/
static DTC_REG_T const *DTC_GetRegTblInfo(INT8U dtc_idx)
{
    if (dtc_idx >= MAX_DISP_CODE) {
        return 0;
    } 
    return (DTC_REG_T const *)&s_obj_dtc_tbl[dtc_idx];
}

/*****************************************************************************
**  函数名:  DTC_GetMaxDtcNum
**  函数描述: 获取最大DTC数量
**  参数:    无
**  返回:    INT8U
*****************************************************************************/
static INT8U DTC_GetMaxDtcNum(void)
{
    return MAX_DISP_CODE;
}

INT32U HexDtc2Long(INT8U *dtc_hex)
{
    INT32U dtc_long;
    dtc_long  = (INT32U)dtc_hex[0] << 16;
    dtc_long |= (INT32U)dtc_hex[1] << 8;
    dtc_long |= (INT32U)dtc_hex[2];
    return dtc_long;
}

void LongDtc2hex(INT8U *dtc_hex, INT32U dtc_long)
{
    dtc_hex[0] = (INT8U)(dtc_long >> 16);
    dtc_hex[1] = (INT8U)(dtc_long >> 8);
    dtc_hex[2] = (INT8U)(dtc_long);
}

/*******************************************************************
** 函数名称: Get_DtcCntPtr
** 函数描述: 获取故障计数地址
** 参数:     无
** 返回:     无
********************************************************************/
static DTC_CNT_T *Get_DtcCntPtr(void)
{
    return &s_dtc_cnt;
}

/*******************************************************************
** 函数名: Update_DTC_Data
** 函数描述: DTC保存在flash中
** 参数: 无
** 返回: TRUE:执行成功
********************************************************************/
static BOOLEAN Update_DTC_Data(void)
{
    INT8U i;
    BOOLEAN is_save;
    DTC_OBJ_T dtc_obj;
    DTC_OBJ_T dtc_obj_cmp;

    is_save = FALSE;
    YX_MEMCPY((INT8U *)&dtc_obj, sizeof(dtc_obj), (INT8U *)&s_dtc_obj, sizeof(s_dtc_obj));
    /* 只保存MASK_08状态 */
    for (i = MIN_DTC_ID; i < MAX_DISP_CODE; i++) {
        dtc_obj.dtc[i].status &= MASK_08;
    }
    
    if (!bal_pp_ReadParaByID(UDS_DTC_PARA_, (INT8U *)&dtc_obj_cmp, sizeof(DTC_OBJ_T))) {
        return FALSE;
    }

    /* 比较是否有更新数据 */
    for (i = MIN_DTC_ID; i < MAX_DISP_CODE; i++) {
        if ((dtc_obj.dtc[i].status != dtc_obj_cmp.dtc[i].status) || (dtc_obj.dtc[i].self_clr_cnt != dtc_obj_cmp.dtc[i].self_clr_cnt)) {
            is_save = TRUE;
            break;
        }
    }    

    if (is_save) {
        #if DEBUG_DTC_PARA_SAVE > 0
        debug_printf("<Update_DTC_Data，保存DTC状态到pp参数>\r\n");
        #endif
        return bal_pp_StoreParaByID(UDS_DTC_PARA_, (INT8U *)&dtc_obj, sizeof(DTC_OBJ_T));
    }
    return TRUE;
}

/*******************************************************************************
** 函数名:     DTC_PpUpdateStart
** 函数描述: 启动dtc的pp参数更新延时
** 参数:       无
** 返回:       无
******************************************************************************/
static void DTC_PpUpdateStart(void)
{
    s_dtc_update.is_update = TRUE;
    s_dtc_update.update_delay = MAX_UPDATE_DELAY;
}

/*****************************************************************************
**  函数名   : MainPwrHdl
**  函数描述 : 主电状态
**  参数     : 无
**  返回     : TRUE:正常    FALSE:异常
*****************************************************************************/
static BOOLEAN MainPwrHdl(void)
{
    INT32S value;
    static BOOLEAN LowCheck = FALSE;
    
    value = PORT_GetADCValue(ADC_MAINPWR);

		if(s_canonoff) {
		    if((value < MAIN_POW_CLOSE_CAN_9V) || (value > MAIN_POW_CLOSE_CAN_36V)) {
				    if(s_canonoff_delay++ >= 100) {                                       /* 主电电压持续1s高压或者低压关闭can */
						    s_canonoff_delay = 0;
								s_canonoff = FALSE;
								bal_Pulldown_CAN0STB();
                bal_Pulldown_CAN1STB();
                bal_Pulldown_CAN2STB();
				    }
		    } else {
		        s_canonoff_delay = 0;
		    }
		} else {
		    if((value > MAIN_POW_OPEN_CAN_10V) && (value < MAIN_POW_OPEN_CAN_35V)) {
				    if(s_canonoff_delay++ >= 100) {                                       /* 主电电压恢复1s */
						    s_canonoff_delay = 0;
								s_canonoff = TRUE;
								bal_Pullup_CAN0STB();
                bal_Pullup_CAN1STB();
                bal_Pullup_CAN2STB();
				    }
		    } else {
		        s_canonoff_delay = 0;
		    }
		}
		
    // 低于19.5V异常后，要恢复到20.5v以上才正常
    if (value > MAIN_POW_LOW_19_5V) {
        if (LowCheck) {
            if (value > MAIN_POW_RECV_20_5V) {
                LowCheck = FALSE;
                return TRUE;
            }
        } else {
            return TRUE;
        }
    } else {
        // 小于19.5v
        LowCheck = TRUE;
    }
    
    return FALSE; 
}

/*****************************************************************************
**  函数名   : MainPwrHighHdl
**  函数描述 : 主电电压状态
**  参数     : 无
**  返回     : TRUE:正常    FALSE:异常
*****************************************************************************/
static BOOLEAN MainPwrIsAccess(void)
{
    INT32S value;
    
    value = PORT_GetADCValue(ADC_MAINPWR);
    // 小于19.5V或大于30.5v，不记录故障
    if ((value <= MAIN_POW_LOW_19_5V) || (value >= MAIN_POW_HIGH_30_5V)) {
        return FALSE;
    }
        
    return TRUE; 
}

/*****************************************************************************
**  函数名   : BatPwrHdl
**  函数描述 : 主电状态
**  参数     : 无
**  返回     : TRUE:正常    FALSE:异常
*****************************************************************************/
static BOOLEAN BatPwrHdl(void)
{
    INT32S value;
    static BOOLEAN LowCheck = FALSE;
    
    value = PORT_GetADCValue(ADC_BAKPWR);

    // 低于3V异常后，要恢复到3.3v以上才正常
    if (value > BAT_POW_LOW_3V) {
        if (LowCheck) {
            if (value > BAT_POW_RECV_3_3V) {
                LowCheck = FALSE;
                return TRUE;
            }
        } else {
            return TRUE;
        }
    } else {
        // 小于3v
        LowCheck = TRUE;
    }
    
    return FALSE; 
}


/*****************************************************************************
**  函数名:  MainPowerCheck
**  函数描述: 主电检测
**  参数:    无
**  返回:    无
*****************************************************************************/
static void MainPowerCheck(void) 
{
    static INT8U dtcRestartDelay = 0;
    
    if (MainPwrHdl()) {
        // 电压正常
        s_main_pow_low_sta = TRUE;
    } else {
        // 电压异常
        s_main_pow_low_sta = FALSE;
    }

    if (!MainPwrIsAccess()) {
        dtcRestartDelay = 0;
        s_dtc_en.main_pwr_en = FALSE;        /* 主电不正常状态 */
    } else {
        if (dtcRestartDelay++ >= 70) {       /* 700ms恢复故障检测，规范500~1000ms */ 
            dtcRestartDelay = 0;
            s_dtc_en.main_pwr_en = TRUE;     /* 主电正常状态 */
        }
    }
}
/*****************************************************************************
**  函数名:   CANBusOffCheck
**  函数描述: BusOff检测
**  参数:     无
**  返回:     无
*****************************************************************************/
static void CANBusOffCheck(void) 
{
    s_dtc_en.can0_bus_off_en = !PORT_GetBusOffStatus(DTC_CAN_CH0);
		s_dtc_en.can1_bus_off_en = !PORT_GetBusOffStatus(DTC_CAN_CH1);
}
/*****************************************************************************
**  函数名:  BatPowerCheck
**  函数描述: 主电检测
**  参数:    无
**  返回:    无
*****************************************************************************/
static void BatPowerCheck(void) 
{
    if (BatPwrHdl()) {
        s_bat_pow_low_sta = TRUE;
    } else {
        s_bat_pow_low_sta = FALSE;
    }
}

/*****************************************************************************
**  函数名:  Kl15_Check
**  函数描述: KL15检测
**  参数:    无
**  返回:    无
*****************************************************************************/
static void Kl15_Check(void)
{
    BOOLEAN acc_sta;
    static INT16U kl15_vaild_cnt = 0;
    
    acc_sta	= !bal_input_ReadSensorFilterStatus(TYPE_ACC); /* acc低有效 */
    
    if (acc_sta) {
        /* 持续3秒表示KL15有效 */
        if (kl15_vaild_cnt++ >= KL15_EN_TIME) {
            kl15_vaild_cnt = 0;
            s_dtc_en.kl15_en = TRUE;
        }
    } else {
        s_dtc_en.kl15_en = FALSE;
        kl15_vaild_cnt = 0;
    }
}

/*****************************************************************************
**  函数名:  NodeLostCheck
**  函数描述: 节点丢失检测
**  参数:    无
**  返回:    无
*****************************************************************************/
static void NodeLostCheck(void)
{
    INT8U i;
    
    for (i = 0; i < MAX_NODE_LOST_NUM; i++) {
        if (s_nodelost_obj[i].nodecheck->is_detect) {
            if (++s_nodelost_obj[i].nodecheck->node_lost_time_out > s_nodelost_obj[i].timeout) {
                s_nodelost_obj[i].nodecheck->node_lost_time_out = 0;
                s_nodelost_obj[i].nodecheck->is_detect = FALSE;    //节点丢失标准
            }
        }
    }    
}

/*****************************************************************************
**  函数名:  MiscStatusCheck
**  函数描述: 杂项状态检测
**  参数:    无
**  返回:    无
*****************************************************************************/
static void MiscStatusCheck(void)
{
    // DTC_ResetMiscFlag();

    // // MPU端故障
    // if (s_mpu_dtc_sig_sta & 0x0001) {
    //     DTC_ConfigMiscFlag(B156D12);
    // } 
    if (!bal_input_ReadSensorFilterStatus(TYPE_PWRDECT)) {
        YX_DTC_SetMpuStatus(B157900-ID_MISC_START, TRUE);
    } else {
        YX_DTC_SetMpuStatus(B157900-ID_MISC_START, FALSE);
    } 
		
		/*if (bal_input_ReadSensorFilterStatus(TYPE_GPSSHORT)) {
        YX_DTC_SetMpuStatus(B156E11 - ID_MISC_START, TRUE);
    } else {
        YX_DTC_SetMpuStatus(B156E11 - ID_MISC_START, FALSE);
    } 

		if (bal_input_ReadSensorFilterStatus(TYPE_GPSOPEN)) {
        YX_DTC_SetMpuStatus(B156E13 - ID_MISC_START, TRUE);
    } else {
        YX_DTC_SetMpuStatus(B156E13 - ID_MISC_START, FALSE);
    }*/ 
}

/*******************************************************************************
 ** 函数名:      Update_Dtc_Para
 ** 函数描述: 更新pp参数处理（延时保存）
 ** 参数:       无
 ** 返回:       无
 ******************************************************************************/
static void Update_Dtc_Para(void)
{
    if (s_dtc_update.is_update) {
        if (s_dtc_update.update_delay) {
            s_dtc_update.update_delay--;
            if (s_dtc_update.update_delay == 0) {
                s_dtc_update.is_update = FALSE;
                Update_DTC_Data();    /* 更新pp参数 */
                #if DEBUG_DTC_PARA_SAVE > 0
                debug_printf("<Update_Dtc_Para，保存DTC状态>\r\n");
                #endif
            }
        }
    }
}

/*******************************************************************
** 函数名: Get_DTCNum
** 描述: 获取T-BOX故障码个数
** 参数: [in] statusmake: 状态掩码
         [out] dtcnum: 故障码
** 返回: 故障码个数
********************************************************************/
static INT8U Get_DTCNum(INT8U statusmask)
{
    INT8U i, dtcnum;

    dtcnum = 0;

    for (i = MIN_DTC_ID; i < MAX_DISP_CODE; i++) {
        if ((s_dtc_obj.dtc[i].status & statusmask) != 0) {
            dtcnum++;
        }
    }

    return dtcnum;
}

/*******************************************************************
** 函数名: Set_DTCStatus
** 描述:   设置T-BOX故障码状态
** 参数: [in] dtc: 故障码 见 DTC_DISP_CODE_E
         [in] mask: DTC状态掩码(应为 DTCSTATUS_MASK_E 的组合)
         [in] op: DTC状态改变操作类型(设置or清除)
         [in] info: 是否需要通知主机(TRUE/FALSE)
** 返回: 设置结果(ture/false)
********************************************************************/
static BOOLEAN Set_DTCStatus(DTC_DISP_CODE_E dtc, INT8U mask, DTCSTATUS_OP_E op, BOOLEAN info)
{
    INT8U      i, status;
    DTC_CNT_T* p_dtc_cnt;
    DTC_OBJ_T  dtc_obj;

    if (!YX_UDS_GetDtcEnableSta()) {
        return FALSE;
    }

    if (dtc >= MAX_DISP_CODE) {
        return FALSE;
    }

    p_dtc_cnt = Get_DtcCntPtr();
    if (op == OP_SET) {
        #if DEBUG_DTC > 1
        debug_printf("<set dtc:%d>status(%x) mask(%x)\r\n", dtc, s_dtc_obj.dtc[dtc].status, mask);
        #endif
        if ((s_dtc_obj.dtc[dtc].status & mask) != mask) { /* 当前状态为未设置状态 */
            status = s_dtc_obj.dtc[dtc].status;
            s_dtc_obj.dtc[dtc].status |= mask;
            /* 状态为已确认时才存储,并且所有故障码只存储已确认状态 */
            if (((status & MASK_08) == 0) && ((mask & MASK_08) != 0)) {                  
                s_dtc_obj.dtc[dtc].self_clr_cnt = 0;    /* 重置自清除计数 */
                p_dtc_cnt->self_clr_cnt[dtc] = 0;
                YX_MEMCPY((INT8U *)&dtc_obj, sizeof(dtc_obj), (INT8U *)&s_dtc_obj, sizeof(s_dtc_obj));
                for (i = MIN_DTC_ID; i < MAX_DISP_CODE; i++) {
                    dtc_obj.dtc[i].status &= MASK_08;
                }

                DTC_PpUpdateStart();
            }

            #if DEBUG_DTC > 1
            debug_printf("set dtc(0x%x) mask(%d) op(%d) info(%d) status(%x)\r\n", dtc, mask, op, info, s_dtc_obj.dtc[dtc].status);
            #endif

            /* 故障码状态变化，上报主机 */
            if (info) {
                #if EN_DTC_SYSC > 1
                YX_Report_DTCStatus(dtc);
                #endif
            }
        }
    } else if (op == OP_CLR) {
        #if DEBUG_DTC > 1
        debug_printf("<clr dtc:%d>status(%x) mask(%x)\r\n", dtc, s_dtc_obj.dtc[dtc].status, mask);
        #endif
		if ((s_dtc_obj.dtc[dtc].status & mask) != 0) { /* 当前状态为已设置状态 */
            status = s_dtc_obj.dtc[dtc].status;
            s_dtc_obj.dtc[dtc].status &= ~mask;
            if (((status & MASK_08) != 0) && ((mask & MASK_08) != 0)) {                  /* 状态为已确认时才存储,并且所有故障码只存储已确认状态 */
                s_dtc_obj.dtc[dtc].self_clr_cnt = 0;    /* 重置自清除计数 */
                p_dtc_cnt->self_clr_cnt[dtc] = 0;
                YX_MEMCPY((INT8U *)&dtc_obj, sizeof(dtc_obj), (INT8U *)&s_dtc_obj, sizeof(s_dtc_obj));
                for (i = MIN_DTC_ID; i < MAX_DISP_CODE; i++) {
                    dtc_obj.dtc[i].status &= MASK_08;
                }
                
                DTC_PpUpdateStart();
            }

            #if DEBUG_DTC > 1
            debug_printf("clr dtc(0x%x) mask(%d) op(%d) info(%d) status(%x)\r\n", dtc, mask, op, info, s_dtc_obj.dtc[dtc].status);
            #endif
            if (info) {
				#if EN_DTC_SYSC > 1
                YX_Report_DTCStatus(dtc);
				#endif
            }
        }
    } else {
        return FALSE;
    }
    return TRUE;
}

/*******************************************************************
** 函数名: Clear_DTC
** 描述:   清除T-BOX故障码
** 参数: [in] dtc: 故障码 见 DTC_DISP_CODE_E(0xFF 表示清除所有故障码)
         [in] info: 是否需要通知MCU(TRUE/FALSE)
** 返回: 清除结果(ture/false)
********************************************************************/
static BOOLEAN Clear_DTC(DTC_DISP_CODE_E dtc, BOOLEAN info)
{
    INT8U i;
    DTC_CNT_T* p_dtc_cnt;
    #if DEBUG_DTC > 0
    debug_printf("Clear_DTC dtc:%d info:%d\r\n", dtc, info);
    #endif

    if ((dtc >= MAX_DISP_CODE) && (dtc != 0xFF)) {
        return FALSE;
    }

    if (dtc == 0xFF) {
        p_dtc_cnt = Get_DtcCntPtr();
        YX_MEMSET((INT8U *)p_dtc_cnt, 0x00, sizeof(DTC_CNT_T));
        for (i = MIN_DTC_ID; i < MAX_DISP_CODE; i++) {
            s_dtc_obj.dtc[i].status = 0x00;
            s_dtc_obj.dtc[i].self_clr_cnt = 0;    /* 重置自清除计数 */
            DTC_ClearNodeLostCnt(i);
        }
        s_mpu_dtc_sig_sta = 0;
        DTC_ResetMiscFlag();
        if (info) {
            #if EN_DTC_SYSC > 1
            i = 0x01;
            YX_EXT_PRO_Send(0x02, 0x00, &i, 1);    // 清除所有故障
            #endif
        }
    } else {
        p_dtc_cnt = Get_DtcCntPtr();
        p_dtc_cnt->set[dtc] = 0x00;
        p_dtc_cnt->clr[dtc] = 0x00;
        p_dtc_cnt->self_clr_cnt[dtc] = 0x00;
		
        s_dtc_obj.dtc[dtc].status = 0x00;
        s_dtc_obj.dtc[dtc].self_clr_cnt = 0;    /* 重置自清除计数 */
        DTC_ClearNodeLostCnt(dtc);
        
        if (info) {
            #if EN_DTC_SYSC > 1
            YX_Report_DTCStatus(dtc);
            #endif
        }
    }
    return TRUE;
}

/*******************************************************************
** 函数名: Get_DTC_Status
** 描述:   获取DTC状态位是否有效
** 参数: [in] dtc: 故障码 见 DTC_DISP_CODE_E
         [in] mask: DTC状态掩码(应为 DTCSTATUS_MASK_E 的组合)
** 返回: TRUE:状态位有效
********************************************************************/
static BOOLEAN Get_DTC_Status(DTC_DISP_CODE_E dtc,INT8U mask)
{
    if (dtc >= MAX_DISP_CODE) return FALSE;

	if (s_dtc_obj.dtc[dtc].status & mask) {
        return TRUE;
	} 
	return FALSE;
}

/*******************************************************************************
**  函数名称:  DtcAccStatusChange
**  功能描述:  信号状态变化处理
**  输入参数:  无
**  返回参数:  None
*******************************************************************************/
static void DtcAccStatusChange(INT8U type, INT8U mode)
{
    INT8U i;
    DTC_CNT_T *dtc_cnt;
    DTC_REG_T const *dtc_info;
    
    switch (type) {
        case  TYPE_ACC:
            if(mode == TRIGGER_POSITIVE) {    /* ACC低有效，上电是产生下降沿 */
                dtc_cnt = Get_DtcCntPtr();
                for (i = MIN_DTC_ID; i < DTC_GetMaxDtcNum(); i++) {
                    dtc_info = DTC_GetRegTblInfo(i);
                    if (dtc_info == NULL) return;
                    if (dtc_cnt  == NULL) return;
                
                    if (dtc_info->is_self_clr) {
                        if (Get_DTC_Status((DTC_DISP_CODE_E)i, MASK_08)) {
                            if (++dtc_cnt->self_clr_cnt[i] >= dtc_info->self_clr_times) {
                                dtc_cnt->self_clr_cnt[i] = 0;
                                /* 40次开关acc，自清除 */
                                Set_DTCStatus((DTC_DISP_CODE_E)i, (MASK_01 | MASK_08), OP_CLR, FALSE);
                            } 
                        }
                    }
                }
            }
            break;
        default:
            break;
    }
}

/*****************************************************************************
**  函数名:  DTC_ClearNodeLostStaBit0
**  函数描述: 清除节点丢失状态bit0
**  参数:    [in] dtc_idx : 节点丢失dtc编号
**  返回:    无
*****************************************************************************/
static void DTC_ClearNodeLostStaBit0(INT8U dtc_idx)
{
    if ((dtc_idx < ID_NODE_LOST_START) || (dtc_idx > ID_NODE_LOST_END)) {
        return;
    }
    Set_DTCStatus((DTC_DISP_CODE_E)dtc_idx, MASK_01, OP_CLR, FALSE);
}

/*****************************************************************************
**  函数名:  DtcMapHdl
**  函数描述: 故障表处理
**  参数:    无
**  返回:    无
*****************************************************************************/
static void DtcMapHdl(void)
{
    INT8U i;
    DTC_CNT_T *dtc_cnt;
    DTC_REG_T const *dtc_info;
	
    dtc_cnt = Get_DtcCntPtr();
    for (i = MIN_DTC_ID; i < DTC_GetMaxDtcNum(); i++) {
			  if((s_support_dtc[i] != 0x01) || (s_onoff_dtc[i] != 0x01))   continue;
        dtc_info = DTC_GetRegTblInfo(i);
        if (dtc_info == NULL) return;
        if (dtc_cnt  == NULL) return;
        if (!DTC_CheckEnable((DTC_DISP_CODE_E)i, (EN_MASK_BIT_E)dtc_info->en_mask)) {
            dtc_cnt->set[i] = 0x00;
            dtc_cnt->clr[i] = 0x00;
            DTC_ClearNodeLostCnt(i);
            DTC_ClearNodeLostStaBit0(i);
        } else {
            if (dtc_info->set_cond != NULL) {
                if (dtc_info->set_cond(i) == TRUE) {
                    dtc_cnt->clr[i] = 0;
                    if (dtc_cnt->set[i] < dtc_info->confirm_times) {
                        dtc_cnt->set[i]++;
                        if (dtc_cnt->set[i] == dtc_info->confirm_times) {
                            Set_DTCStatus((DTC_DISP_CODE_E)i, (MASK_01 | MASK_08), OP_SET, FALSE);
                        }
                    } 
                } else {
                    dtc_cnt->set[i] = 0;
                    if (dtc_cnt->clr[i] < dtc_info->recv_times) {
                        dtc_cnt->clr[i]++;
                        if (dtc_cnt->clr[i] == dtc_info->recv_times) {
                            Set_DTCStatus((DTC_DISP_CODE_E)i, MASK_01, OP_CLR, FALSE);
                        }
                    } 				
                }
            }
		}	
	}
}


/*******************************************************************************
 ** 函数名:    DTC_HandleTmr
 ** 函数描述:   定时器处理函数
 ** 参数:       无
 ** 返回:       无
 ******************************************************************************/
static void DTC_HandleTmr(void* pdata)
{ 
	pdata = pdata;
    /* 故障条件检测 */
		CANBusOffCheck();
    MainPowerCheck();
    BatPowerCheck();
    Kl15_Check();
    NodeLostCheck();
    MiscStatusCheck();
    /* 故障记录处理 */
    DtcMapHdl();
    /* 更新pp参数 */
    Update_Dtc_Para();
}

/*****************************************************************************
**  函数名:  MainPowerIsLow
**  函数描述: 主电欠压状态
**  参数:    [in] dtc_id : 显示码
**  返回:    TRUE: 欠压 FALSE: 正常
*****************************************************************************/
static BOOLEAN MainPowerIsLow(INT8U dtc_id)
{
	dtc_id = dtc_id;
    if (s_main_pow_low_sta == FALSE) {
        return TRUE;
    } 
    return FALSE;
}

/*****************************************************************************
**  函数名:  BatPowerIsLow
**  函数描述: 主电过压状态
**  参数:    [in] dtc_id : 显示码
**  返回:    TRUE: 欠压 FALSE: 正常
*****************************************************************************/
static BOOLEAN BatPowerIsLow(INT8U dtc_id)
{
	dtc_id = dtc_id;
    if (!s_bat_pow_low_sta) {
        #if DEBUG_MAIN_PWR > 1
        debug_printf("<备用电池低压>\r\n");
        #endif
        return TRUE;
    } 
    return FALSE;
}

/*****************************************************************************
**  函数名:  BusIsOff
**  函数描述: bus off状态
**  参数:    [in] dtc_id : 显示码
**  返回:    TRUE: bus off FALSE: 正常
*****************************************************************************/
static BOOLEAN BusIsOff(INT8U dtc_id)
{
    //dtc_id = dtc_id;
    if(dtc_id == U000100) {
        if (!s_dtc_en.can0_bus_off_en) {
            return TRUE;
        }
    } else if(dtc_id == U003700) {
        if (!s_dtc_en.can0_bus_off_en) {
            return TRUE;
        }
    }
    return FALSE;
}

/*****************************************************************************
**  函数名:  NodeIsLost
**  函数描述: 节点丢失状态
**  参数:    [in] dtc_id : 显示码，见DTC_DISP_CODE_E
**  返回:    TRUE: 节点丢失 FALSE: 正常
*****************************************************************************/
static BOOLEAN NodeIsLost(INT8U dtc_id)
{
    INT8U idx;
    
    if ((dtc_id < ID_NODE_LOST_START) || (dtc_id > ID_NODE_LOST_END)) {
        return FALSE;
    }
	
    idx = dtc_id - ID_NODE_LOST_START;

    if (!s_nodelost_obj[idx].nodecheck->is_detect) {
        return TRUE;
    }
    
    return FALSE;
}

/*****************************************************************************
**  函数名:  MiscIsDetect
**  函数描述: 杂项故障码检测
**  参数:    [in] dtc_id : 显示码，见DTC_DISP_CODE_E
**  返回:    TRUE: 检测到故障  FALSE: 正常
*****************************************************************************/
static BOOLEAN MiscIsDetect(INT8U dtc_id)
{
    if ((dtc_id < ID_MISC_START) || (dtc_id > ID_MISC_END)) {
        return FALSE;
    }

    if (DTC_GetMiscFlag((DTC_DISP_CODE_E)dtc_id)) {
        return TRUE;
    }

    return FALSE;    
}

/*******************************************************************
** 函数名: UDS_SID14_Response
** 函数描述: 清除诊断信息肯定响应
** 参数: NULL
** 返回: NULL
********************************************************************/
static void UDS_SID14_Response(void)
{
    STREAM_T wstrm;
    INT8U    memptr[8];
    
    bal_InitStrm(&wstrm, memptr, 8);
	
    bal_WriteBYTE_Strm(&wstrm, (SID_14 + RESP_ADD));                             /* 肯定响应标识 */

    #if DEBUG_UDS > 0
    debug_printf("UDS_SID14_Response\r\n");
    #endif
    YX_UDS_CanSendSingle(bal_GetStrmStartPtr(&wstrm), bal_GetStrmLen(&wstrm));
}

/*******************************************************************
** 函数名: UDS_SID19_Response
** 函数描述: 读取DTC信息肯定响应
** 参数: [in] reporttype: 报告类型
         [in] data: 报告类型数据
         [in] len: 数据长度
** 返回: NULL
********************************************************************/
static void UDS_SID19_Response(INT8U reporttype, INT8U *data, INT8U len)
{
    STREAM_T wstrm;
    INT8U   *memptr, memlen;

    #if DEBUG_UDS > 0
    debug_printf("UDS_SID19_Response->reporttype(%x):", reporttype);
    printf_hex(data, len);
    debug_printf("\r\n");
    #endif

    memlen = len + 2;
    memptr = YX_MemMalloc(memlen);
    if (memptr == NULL) {
        return;
    }

    bal_InitStrm(&wstrm, memptr, memlen);

    bal_WriteBYTE_Strm(&wstrm, (SID_19 + RESP_ADD));                             /* 肯定响应标识 */
    bal_WriteBYTE_Strm(&wstrm, reporttype);                                      /* 报告类型 */
    bal_WriteDATA_Strm(&wstrm, data, len);
    
    if (memlen < 8) {
        YX_UDS_CanSendSingle(bal_GetStrmStartPtr(&wstrm),  bal_GetStrmLen(&wstrm));
    } else {
        YX_UDS_SendMulCanData(bal_GetStrmStartPtr(&wstrm), bal_GetStrmLen(&wstrm));
    }
    
    if (memptr != NULL) {
        YX_MemFree(memptr);
    }
}

/*
********************************************************************************
* 定义对外接口
********************************************************************************
*/

/*******************************************************************
** 函数名: YX_DTC_SID14_ClearDiagnosticInformation
** 函数描述: 清除诊断信息
** 参数: [in] dtcdata: DTC组或特定DTC
         [in] len: 数据长度
** 返回: NULL
********************************************************************/
void YX_DTC_SID14_ClearDiagnosticInformation(INT8U *data, INT16U len)
{
    if (len != 3) {
        YX_UDS_NegativeResponse(SID_14, NRC_13);
        return;
    }
    
    if ((data[0] == 0xFF) && (data[1] == 0xFF) && (data[2] == 0xFF)) {
        #if DEBUG_UDS > 0
        debug_printf("SID14 clear all\r\n");
        #endif
        if (Clear_DTC((DTC_DISP_CODE_E)0xFF, TRUE)) {
            YX_UDS_NegativeResponse(SID_14, NRC_78);
            DTC_PpUpdateStart();    
            UDS_SID14_Response();
        } else {
            YX_UDS_NegativeResponse(SID_14, NRC_31);
        }
    } else {
        #if 0
        dtc_code_in = HexDtc2Long(data);

        for (i = MIN_DTC_ID; i < MAX_DISP_CODE; i++) {
            dtc_info = DTC_GetRegTblInfo(i);
            if (dtc_info->dtc_code == dtc_code_in) {
                break;
            }
        }
        if (i < MAX_DISP_CODE) {
            if (Clear_DTC((DTC_DISP_CODE_E)i, TRUE)) {
                YX_UDS_NegativeResponse(SID_14, NRC_78);
                DTC_PpUpdateStart();    
                UDS_SID14_Response();
            } else {
                YX_UDS_NegativeResponse(SID_14, NRC_31);
            }
        } else {
            YX_UDS_NegativeResponse(SID_14, NRC_31);
        }
        #else
        YX_UDS_NegativeResponse(SID_14, NRC_31);
        #endif
    }
}

/*******************************************************************
** 函数名: YX_DTC_SID19_ReadDTCInformation
** 函数描述: 读取DTC信息
** 参数: [in] data: 包含sub的数据
         [in] len: 数据长度
** 返回: NULL
********************************************************************/
void YX_DTC_SID19_ReadDTCInformation(INT8U *data, INT16U len)
{
    INT8U i, reporttype, dtcnum, mask;
    BOOLEAN response;
    STREAM_T* wstrm;
    DTC_REG_T const *dtc_info;
	INT8U dtc_codetemp[3];
    INT8U isSupPosRsp = TRUE;    // FALSE: 不支持禁止肯定应答
	
    if (YX_UDS_GetSession() == SESSION_PROGRAM) {
        YX_UDS_NegativeResponse(SID_19, NRC_7F);
        return;
    }
    
    if (len < 1) {
        YX_UDS_NegativeResponse(SID_19, NRC_13);
        return;
    }

    wstrm = bal_STREAM_GetBufferStream();
    
    response = FALSE;
    reporttype = data[0];
    switch (reporttype) {
        case REPORT_NUM:
        case (REPORT_NUM + SUPPRESS_RESP):
			
            if (len != 2) {
                YX_UDS_NegativeResponse(SID_19, NRC_13);
                break;
            }
            if ((isSupPosRsp == FALSE) || (!(reporttype & SUPPRESS_RESP))) {
                mask = data[1];
                dtcnum = Get_DTCNum(mask);
                bal_WriteBYTE_Strm(wstrm, MASK_DEFAUTLT);
                bal_WriteBYTE_Strm(wstrm, DTC_FORMAT_J2012);
                bal_WriteHWORD_Strm(wstrm, dtcnum);
                response = TRUE;
            }
            break;
        case REPORT_DTC:
        case (REPORT_DTC + SUPPRESS_RESP):
            if (len != 2) {
                YX_UDS_NegativeResponse(SID_19, NRC_13);
                break;
            }
            if ((isSupPosRsp == FALSE) || (!(reporttype & SUPPRESS_RESP))) {
                bal_WriteBYTE_Strm(wstrm, MASK_DEFAUTLT);
                mask = data[1];
                for (i = MIN_DTC_ID; i < MAX_DISP_CODE; i++) {
                    if ((s_dtc_obj.dtc[i].status & mask) != 0) {
                        dtc_info = DTC_GetRegTblInfo(i);
                        
                        LongDtc2hex(dtc_codetemp, dtc_info->dtc_code);
                        
                        bal_WriteDATA_Strm(wstrm, dtc_codetemp, 3);
                        bal_WriteBYTE_Strm(wstrm, s_dtc_obj.dtc[i].status); /* 当前状态 */
                    }
                }
                response = TRUE;
            }
            break;
        #if 0
        case REPORT_SNAPSHOT:
        case (REPORT_SNAPSHOT + SUPPRESS_RESP):
            if (len != 5) {
                YX_UDS_NegativeResponse(SID_19, NRC_13);
                break;
            }
            YX_UDS_NegativeResponse(SID_19, NRC_31);
            break;
        #endif
        case REPORT_SUPPORTED:
        case (REPORT_SUPPORTED + SUPPRESS_RESP):
            if (len != 1) {
                YX_UDS_NegativeResponse(SID_19, NRC_13);
                break;
            }
            if ((isSupPosRsp == FALSE) || (!(reporttype & SUPPRESS_RESP))) {
                bal_WriteBYTE_Strm(wstrm, MASK_DEFAUTLT);
                for (i = MIN_DTC_ID; i < MAX_DISP_CODE; i++) { 
									  if(s_support_dtc[i] != 0x01) continue;
                    dtc_info = DTC_GetRegTblInfo(i);
                    
                    LongDtc2hex(dtc_codetemp, dtc_info->dtc_code);
                    
                    bal_WriteDATA_Strm(wstrm, dtc_codetemp, 3);              
                    bal_WriteBYTE_Strm(wstrm, MASK_DEFAUTLT); /* 所有支持的状态 */
                }
                response = TRUE;
            }
            break;
        default :
            YX_UDS_NegativeResponse(SID_19, NRC_12);
            break;
    }

    if (response) {
        UDS_SID19_Response(reporttype & 0x7F, bal_GetStrmStartPtr(wstrm), bal_GetStrmLen(wstrm));
    }
}

/*******************************************************************************
 **  函数名称:  YX_DTC_NodeIdCheck
 **  功能描述:  通信丢失诊断报文
 **  输入参数:  [in] node_id : 节点丢失id
 **  返回参数:  无
 ******************************************************************************/
void YX_DTC_NodeIdCheck(INT8U ch, INT32U node_id)
{
    INT8U i;  

		for(i = 0; i < MAX_NODE_LOST_NUM; i++) {
		    if(node_id ==  s_nodelost_obj[i].id) {
				    break;
		    }
		}

		if(i == MAX_NODE_LOST_NUM) {
		    return;
		}
		s_nodelost_obj[i].nodecheck->is_detect = TRUE;
    s_nodelost_obj[i].nodecheck->node_lost_time_out = 0;
        
    // 所有伙伴ECU超时
    s_nodelost_obj[0].nodecheck->is_detect = TRUE;
    s_nodelost_obj[0].nodecheck->node_lost_time_out = 0; 
    #if 0
    ch = ch;
    i = 0;
    mask = node_id & 0x000000FF;
    
    if (mask == 0x00) {    /*EMS节点*/
        
        //if (ch != UDS_CAN_CH) return;

        // ems
        i = U010000 - ID_NODE_LOST_START;
        s_nodelost_obj[i].nodecheck->is_detect = TRUE;
        s_nodelost_obj[i].nodecheck->node_lost_time_out = 0;
        
        // 所有伙伴ECU超时
        i = U000200 - ID_NODE_LOST_START;
        s_nodelost_obj[i].nodecheck->is_detect = TRUE;
        s_nodelost_obj[i].nodecheck->node_lost_time_out = 0;           
    } else if (mask == 0x17) {    /*IC节点*/
        // ic
        i = U015500 - ID_NODE_LOST_START;
        s_nodelost_obj[i].nodecheck->is_detect = TRUE;
        s_nodelost_obj[i].nodecheck->node_lost_time_out = 0;
        
        // 所有伙伴ECU超时
        i = U000200 - ID_NODE_LOST_START;
        s_nodelost_obj[i].nodecheck->is_detect = TRUE;
        s_nodelost_obj[i].nodecheck->node_lost_time_out = 0;    
    } else {
        switch (mask) {
            case 0x21:    /*BCM节点*/
            case 0x0B:    /*ABS/EBS节点*/
            case 0x03:    /*AMT/TCU节点*/
            case 0x10:    /*Retarder节点*/
            case 0x2F:    /*ECAS节点*/
            case 0x33:    /*TPMS节点*/
            case 0xC6:    /*GCT节点*/
            case 0xF4:    /*IBS节点*/
                // 所有伙伴ECU超时
                i = U000200 - ID_NODE_LOST_START;
                s_nodelost_obj[i].nodecheck->is_detect = TRUE;
                s_nodelost_obj[i].nodecheck->node_lost_time_out = 0;    
                break;
            default:
                break;
        }
    }
    #endif
    #if DEBUG_DTC > 0
    debug_printf("<**** dtc lost id recv:0x%x ****>\r\n", i, node_id);
    #endif
}

/*******************************************************************************
** 函数名:     YX_DTC_PP_Update_When_Reset
** 函数描述: 11服务复位时dtc的pp参数更新
** 参数:       无
** 返回:       无
******************************************************************************/
void YX_DTC_PP_Update_When_Reset(void)
{
    if (s_dtc_update.is_update) {
        s_dtc_update.is_update = FALSE;
        s_dtc_update.update_delay = 0;
        Update_DTC_Data();    /* 更新pp参数 */

        #if DEBUG_DTC_PARA_SAVE > 0
        debug_printf("<YX_DTC_PP_Update_When_Reset，保存DTC状态>\r\n");
        #endif
    }
}

/*****************************************************************************
**  函数名:  YX_DTC_SetBusOffFlag
**  函数描述: 设置bus off标志
**  参数:    [in] com  :
**           [in] flag :
**  返回:    无
*****************************************************************************/
void YX_DTC_SetBusOffFlag(INT8U com, INT8U flag)
{
    if (com == DTC_CAN_CH0) {
        s_dtc_en.can0_bus_off_en = flag; 
    } else if (com == DTC_CAN_CH1) {
        s_dtc_en.can1_bus_off_en = flag; 
    }    
}

/*****************************************************************************
**  函数名:  YX_DTC_SetMpuStatus
**  函数描述: 主机故障状态
**  参数:    [in] dtcCode : dtc码
**           [in] status  : 状态 
**  返回:    无
*****************************************************************************/
BOOLEAN YX_DTC_SetMpuStatus(INT8U dtcIdx, BOOLEAN status)
{
    if (dtcIdx > ID_MISC_END - ID_MISC_START) {
        return FALSE;
    }
    if (status) {
        s_misc_flag[dtcIdx] = 0x01;
    } else {
        s_misc_flag[dtcIdx] = 0x00;
    }

    return TRUE;
}

/*****************************************************************************
**  函数名:  YX_DTC_GetStatus
**  函数描述  : 获取故障码状态 
**  参数:    [out] dtcBuf  : 
             [out] dtcBufLen : 
**  返回:    TRUE:有故障 FALSE:无故障
*****************************************************************************/
BOOLEAN YX_DTC_GetStatus(INT8U* dtcBuf, INT8U* dtcBufLen)
{
    INT8U idx, bufLen;

    if ((dtcBuf == NULL) || (dtcBufLen == NULL)) return FALSE;

    bufLen = 0;
    *dtcBufLen = 0;
    
    for (idx = 0; idx < MAX_DISP_CODE; idx++) {
        if (s_dtc_obj.dtc[idx].status & MASK_01) {
            YX_MEMCPY(&dtcBuf[bufLen], 4, s_dm1_dtc_tab[idx].DM1_DTC, 4);
            bufLen += 4;
        }        
    }
    *dtcBufLen = bufLen;

    if (bufLen) {
        return TRUE;
    }
    
    return FALSE;
}
/*****************************************************************************
**  函数名:  YX_CAR_SIGNAL_AdapterDTC
**  函数描述  : 获取故障码状态 
**  参数:    [out] dtcBuf  : 
             [out] dtcBufLen : 
**  返回:    TRUE:有故障 FALSE:无故障
*****************************************************************************/
void YX_CarSignal_AdapterDTC(INT8U type, INT8U *data)
{
    #if DEBUG_DTC > 0
	  INT8U i;
		#endif
		
	  YX_MEMSET(&s_support_dtc, 0x01, sizeof(s_support_dtc));
		YX_MEMSET(&s_onoff_dtc,   0x01, sizeof(s_onoff_dtc));
    
    switch(type) {
        case CAR_SIGNAL_J6:           /* J6低配 */
				    s_support_dtc[U003700] = 0x00;
						s_support_dtc[U014600] = 0x00;
        break;       
        case CAR_SIGNAL_J6_MID:	     /* J6中配 */
					  s_support_dtc[U000100] = 0x00;
						s_support_dtc[U010000] = 0x00;
						s_support_dtc[U015500] = 0x00;
						s_support_dtc[U010100] = 0x00;
						s_support_dtc[U012A00] = 0x00;
						s_support_dtc[U012200] = 0x00;
						s_support_dtc[U013200] = 0x00;
        break;
        case CAR_SIGNAL_QINGQI:			 /* 青汽架构 */
						s_support_dtc[U003700] = 0x00;
						s_support_dtc[U014000] = 0x00;
						s_support_dtc[U010100] = 0x00;
						s_support_dtc[U012A00] = 0x00;
						s_support_dtc[U012200] = 0x00;
						s_support_dtc[U012700] = 0x00;
						s_support_dtc[U013200] = 0x00;
						s_support_dtc[U103C00] = 0x00;
						s_support_dtc[U103B00] = 0x00;
						s_support_dtc[B158000] = 0x00;
						s_support_dtc[U014600] = 0x00;
        break;		  
		}

	  if(data[1] == 0x00) {
		    s_onoff_dtc[U010100] = 0x00;
	  }

	  if(data[3] == 0x00) {
		    s_onoff_dtc[U012700] = 0x00;
	  }

		if(data[4] == 0x00) {
		    s_onoff_dtc[U013200] = 0x00;
	  }

		if(data[7] == 0x00) {
		    s_onoff_dtc[U012A00] = 0x00;
	  }

		if(data[8] == 0x00) {
		    s_onoff_dtc[U014000] = 0x00;
	  }
		if(data[9] == 0x00) {
		    s_onoff_dtc[U012200] = 0x00;
	  }
		if(data[10] == 0x00) {
		    s_onoff_dtc[U103C00] = 0x00;
	  }
		if(data[11] == 0x00) {
		    s_onoff_dtc[U103B00] = 0x00;
	  }

		if(data[13] == 0x00) {
		    s_onoff_dtc[U010000] = 0x00;
	  }
		if(data[14] == 0x00) {
		    s_onoff_dtc[U014600] = 0x00;
	  }
		#if DEBUG_DTC > 0
		for(i = 0; i < MAX_DISP_CODE; i++) {
        debug_printf("s_support_dtc[%d] = %d s_onoff_dtc[%d] = %d r\n",i,s_support_dtc[i],i,s_onoff_dtc[i]);
		}
    #endif
}
/*******************************************************************************
** 函数名:     YX_DTC_Init
** 函数描述:   初始化DTC功能
** 参数:       无
** 返回:       无
******************************************************************************/
void YX_DTC_Init(void)
{
    INT8U i;

    DTC_Dm1Init();
    DTC_ResetMiscFlag();
    
    // 初始化故障码结构体
    YX_MEMSET((INT8U *)&s_dtc_obj, 0x00, sizeof(DTC_OBJ_T));
    if (bal_pp_ReadParaByID(UDS_DTC_PARA_, (INT8U *)&s_dtc_obj, sizeof(DTC_OBJ_T)) == FALSE) {
        #if DEBUG_DTC > 0
        debug_printf("<**** 读取DTC的pp参数错误 ****>r\n");
        #endif
    }     
		YX_MEMSET(&s_onoff_dtc,	 0x01, sizeof(s_onoff_dtc));

    // 初始化节点丢失检测结构
    for (i = 0; i < MAX_NODE_LOST_NUM; i++) {
        // 默认节点不丢失状态 
        s_node_lost[i].is_detect = TRUE;    
        s_node_lost[i].node_lost_time_out = 0;
    }

    YX_MEMSET((INT8U*)&s_dtc_en, 0x00, sizeof(DTC_EN_T));
    /* 默认can正常 */
    s_dtc_en.can0_bus_off_en = TRUE;
    s_dtc_en.can1_bus_off_en = TRUE;
	
    s_dtc_tmr = OS_InstallTmr(TSK_ID_OPT, 0, DTC_HandleTmr);
    OS_StartTmr(s_dtc_tmr, _TICK, 1);

    bal_input_InstallTriggerProc(TYPE_ACC, TRIGGER_POSITIVE, DtcAccStatusChange);
}
#endif

