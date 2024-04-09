/********************************************************************************
**
** 文件名:     yx_uds_did.c
** 版权所有:   (c) 2007-2021厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现UDS的DID功能
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2021/06/07 | cym    |  创建第一版本
*********************************************************************************/
#include "yx_includes.h"
#include "bal_stream.h"
#include "yx_dtc_drv.h"
#include "dal_can.h"
#include "yx_uds_drv.h"
#include "bal_input_drv.h"
#include "bal_pp_drv.h"
#include "port_dm.h"
#include "yx_dtc_drv.h"
#include "bal_gpio_cfg.h"
#include "yx_com_man.h"
#include "port_adc.h"
#include "port_gpio.h"
#include "yx_uds_did.h"
#include "yx_com_send.h"
#include "yx_can_man.h"
#include "hal_exrtc_sd2058_drv.h"
#include "public.h"
#include "appmain.h"
#if EN_UDS > 0
/*
********************************************************************************
* 定义宏
********************************************************************************
*/
#if EN_DEBUG > 1
#define DEBUG_DID         1
#else
#define DEBUG_DID         0
#endif

#define DELAY_SAVE_TIMEOUT    5

/*
********************************************************************************
* 定义模块结构
********************************************************************************
*/


/* did值范围 */
typedef struct {
    INT8U min;
    INT8U max;
} DID_VALUE_T;

typedef struct {
    INT16U code;                 // did号
    INT8U  rw;                   // did读写属性
    INT8U  len;                  // 数据长度
    INT8U* data;                 // 数据内容
    DID_DATA_TYPE_E datatype;    // 数据类型 
    BOOLEAN (*didReadFun) (INT8U* data, INT8U dataLen);
    BOOLEAN (*didWriteFun) (INT8U* data, INT8U dataLen);
} UDS_DID_OBJ_T;

typedef struct {
    INT8U totalDis[4];        // 总里程,0x18FEE017（5-8字节）
    INT8U tolalFuelCon[4];    // 总油耗,0x18FEE900（5-8字节）
    INT8U fuelRate[2];        // 瞬时燃油消耗率,0x18FEF200（1-2字节）
    INT8U vehSpeed[2];        // 仪表车速,0x0CFE6C17（7-8字节）
    INT8U engSpeed[2];        // 发动机转速,0x0CF00400（4-5字节）
} CAN_SIGNAL_T;

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
typedef struct {
		INT8U DID_1007[7]; 	   /* 当前时间 */
    #if 0
    INT8U DID_F184[7];    /* 刷写日期 */
    INT8U DID_F1A0[64];   /* 软件版本号 */
    #endif

    INT8U DID_1009[2];    // GPS定位状态
    INT8U DID_1010[2];    // 4G工作状态
    INT8U DID_1015[1];    // 本地存储器使用率
    INT8U DID_102D[4];    // 总里程
    INT8U DID_102E[4];    // 总油耗
    INT8U DID_102F[2];    // 瞬时燃油消耗率
    INT8U DID_1030[2];    // 仪表车速
    INT8U DID_1031[2];    // 发动机转速
    INT8U DID_1036[1];    // TBOX备用电池电压
    INT8U DID_103A[64];   // mcu版本号
    INT8U DID_103B[4];    // AI总里程/积分总里程
    INT8U DID_103C[4];    // AI总油耗/积分总油耗
    INT8U DID_103D[1];    // TSP连接状态  
} UDS_DID_LOCAL_T;

static UDS_DID_LOCAL_T    s_uds_did_local;
static UDS_DID_DATA_E2ROM_T s_uds_did_e2rom_data;

static INT8U s_did_tmr;
static INT8U s_delay_save_to_flash;

static CAN_SIGNAL_T      s_can_signal;
static CAR_SIGNAL_TYPE_E s_car_signal;
static INT16U s_vehSpeed = 0;
static INT8U  s_vehSpeed_over = 0;
static const INT8U s_did_103a[64] = {
     'V',  '4',  '.',  '3',  '5',  'Y',  0x00,0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00,
};
/*
********************************************************************************
* 定义本地接口
********************************************************************************
*/
#if 0
static BOOLEAN DID_ReadF1A0(INT8U* pData, INT8U didLen)
{
    INT8U i, *ver;

    if ((pData == NULL) || (didLen == 0)) {
        return FALSE;
    }
    
    YX_MEMSET(pData, 0x00, didLen);
    ver = (INT8U*)YX_GetVersion();
    for (i = 0; i < didLen; i++) {
        if (ver[i] != 0x00) {
            pData[i] = ver[i];
        } else {
            break;
        }
    }   

    return TRUE;
}
#endif
static BOOLEAN DID_Read1009(INT8U* pData, INT8U didLen)
{
    if ((pData == NULL) || (didLen == 0)) {
        return FALSE;
    }
     

    return TRUE;
}

static BOOLEAN DID_Read1010(INT8U* pData, INT8U didLen)
{
    if ((pData == NULL) || (didLen == 0)) {
        return FALSE;
    }
     

    return TRUE;
}

static BOOLEAN DID_Read1015(INT8U* pData, INT8U didLen)
{
    if ((pData == NULL) || (didLen == 0)) {
        return FALSE;
    }
     
    return TRUE;
}

static BOOLEAN DID_Read102D(INT8U* pData, INT8U didLen)
{
    if ((pData == NULL) || (didLen == 0)) {
        return FALSE;
    }
     
    pData[0] = s_can_signal.totalDis[0];
    pData[1] = s_can_signal.totalDis[1];
    pData[2] = s_can_signal.totalDis[2];
    pData[3] = s_can_signal.totalDis[3];

    return TRUE;
}

static BOOLEAN DID_Read102E(INT8U* pData, INT8U didLen)
{
    if ((pData == NULL) || (didLen == 0)) {
        return FALSE;
    }
     
    pData[0] = s_can_signal.tolalFuelCon[0];
    pData[1] = s_can_signal.tolalFuelCon[1];
    pData[2] = s_can_signal.tolalFuelCon[2];
    pData[3] = s_can_signal.tolalFuelCon[3];

    return TRUE;
}

static BOOLEAN DID_Read102F(INT8U* pData, INT8U didLen)
{
    if ((pData == NULL) || (didLen == 0)) {
        return FALSE;
    }
     
    pData[0] = s_can_signal.fuelRate[0];
    pData[1] = s_can_signal.fuelRate[1];

    return TRUE;
}

static BOOLEAN DID_Read1030(INT8U* pData, INT8U didLen)
{
    if ((pData == NULL) || (didLen == 0)) {
        return FALSE;
    }
     
    pData[0] = s_can_signal.vehSpeed[0];
    pData[1] = s_can_signal.vehSpeed[1];

    return TRUE;
}
static BOOLEAN DID_Read1031(INT8U* pData, INT8U didLen)
{
    if ((pData == NULL) || (didLen == 0)) {
        return FALSE;
    }
     
    pData[0] = s_can_signal.engSpeed[0];
    pData[1] = s_can_signal.engSpeed[1];
    return TRUE;
}

static BOOLEAN DID_Read1036(INT8U* pData, INT8U didLen)
{
    INT32S val;
    if ((pData == NULL) || (didLen == 0)) {
        return FALSE;
    }

    val = PORT_GetADCValue(ADC_BAKPWR);
    if (val < 500) {
        pData[0] = 0xFF;
    } else {
        val *= 2;
        val /= 100;
        pData[0] = (INT8U)val;
    }

    return TRUE;
}
static BOOLEAN DID_Read103B(INT8U* pData, INT8U didLen)
{
    if ((pData == NULL) || (didLen == 0)) {
        return FALSE;
    }
     

    return TRUE;
}
static BOOLEAN DID_Read103C(INT8U* pData, INT8U didLen)
{
    if ((pData == NULL) || (didLen == 0)) {
        return FALSE;
    }
     

    return TRUE;
}
static BOOLEAN DID_Read103D(INT8U* pData, INT8U didLen)
{
    if ((pData == NULL) || (didLen == 0)) {
        return FALSE;
    }
     

    return TRUE;
}
static BOOLEAN DID_Read1007(INT8U* pData, INT8U didLen)
{
    INT8U data[7];
		 
		if ((pData == NULL) || (didLen == 0)) {
       return FALSE;
    }
               
    if(HAL_sd2058_ReadCalendar(data)) {      
        pData[0] = 20; 	        //YEAR
        pData[1] = data[6]; 		//YEAR
        pData[2] = data[5]; 	  //MONTH
        pData[3] = data[4]; 		//DATE
        pData[4] = data[2]; 		//HOUR
        pData[5] = data[1]; 		//MINITE
        pData[6] = data[0]; 		//TICK
    } else { 
        pData[0] = 20; 	        //YEAR
        pData[1] = 24; 		//YEAR
        pData[2] = 03; 	  //MONTH
        pData[3] = 01; 		//DATE
        pData[4] = 13; 		//HOUR
        pData[5] = 43; 		//MINITE
        pData[6] = 58; 		//TICK
        return FALSE;
    }

    return TRUE;
}

static const UDS_DID_OBJ_T s_uds_did_obj[MAX_DID_NUM] = {
	  #if 0
    {0xF184, DID_RW,  sizeof(s_uds_did_local.DID_F184), s_uds_did_local.DID_F184, DID_DATA_TYPE_HEX , NULL,         NULL}, 
    {0xF1A0, DID_RO,  sizeof(s_uds_did_local.DID_F1A0), s_uds_did_local.DID_F1A0, DID_DATA_TYPE_HEX , DID_ReadF1A0, NULL},
    #endif
    {0x1009, DID_RO,  sizeof(s_uds_did_local.DID_1009), s_uds_did_local.DID_1009, DID_DATA_TYPE_HEX , DID_Read1009, NULL},
    {0x1010, DID_RO,  sizeof(s_uds_did_local.DID_1010), s_uds_did_local.DID_1010, DID_DATA_TYPE_HEX , DID_Read1010, NULL},
    {0x1015, DID_RO,  sizeof(s_uds_did_local.DID_1015), s_uds_did_local.DID_1015, DID_DATA_TYPE_HEX , DID_Read1015, NULL},
    {0x102D, DID_RO,  sizeof(s_uds_did_local.DID_102D), s_uds_did_local.DID_102D, DID_DATA_TYPE_HEX , DID_Read102D, NULL},
    {0x102E, DID_RO,  sizeof(s_uds_did_local.DID_102E), s_uds_did_local.DID_102E, DID_DATA_TYPE_HEX , DID_Read102E, NULL},
    {0x102F, DID_RO,  sizeof(s_uds_did_local.DID_102F), s_uds_did_local.DID_102F, DID_DATA_TYPE_HEX , DID_Read102F, NULL},
    {0x1030, DID_RO,  sizeof(s_uds_did_local.DID_1030), s_uds_did_local.DID_1030, DID_DATA_TYPE_HEX , DID_Read1030, NULL},
    {0x1031, DID_RO,  sizeof(s_uds_did_local.DID_1031), s_uds_did_local.DID_1031, DID_DATA_TYPE_HEX , DID_Read1031, NULL},
    {0x1036, DID_RO,  sizeof(s_uds_did_local.DID_1036), s_uds_did_local.DID_1036, DID_DATA_TYPE_HEX , DID_Read1036, NULL},
    {0x103A, DID_RO,  sizeof(s_uds_did_local.DID_103A), s_uds_did_local.DID_103A, DID_DATA_TYPE_ASCII,NULL,         NULL},
    {0x103B, DID_RO,  sizeof(s_uds_did_local.DID_103B), s_uds_did_local.DID_103B, DID_DATA_TYPE_HEX , DID_Read103B, NULL},
    {0x103C, DID_RO,  sizeof(s_uds_did_local.DID_103C), s_uds_did_local.DID_103C, DID_DATA_TYPE_HEX , DID_Read103C, NULL},
    {0x103D, DID_RO,  sizeof(s_uds_did_local.DID_103D), s_uds_did_local.DID_103D, DID_DATA_TYPE_HEX , DID_Read103D, NULL},
    
		{0x1007, DID_RW,  sizeof(s_uds_did_local.DID_1007),      s_uds_did_local.DID_1007,      DID_DATA_TYPE_BCD,   DID_Read1007, NULL}, 
    // 存在pp参数中
    {0x0100, DID_RW,  sizeof(s_uds_did_e2rom_data.DID_0100), s_uds_did_e2rom_data.DID_0100, DID_DATA_TYPE_HEX,   NULL, NULL}, 
    {0x0110, DID_RW,  sizeof(s_uds_did_e2rom_data.DID_0110), s_uds_did_e2rom_data.DID_0110, DID_DATA_TYPE_HEX,   NULL, NULL},  
		{0x1002, DID_RW,	sizeof(s_uds_did_e2rom_data.DID_1002), s_uds_did_e2rom_data.DID_1002, DID_DATA_TYPE_ASCII, NULL, NULL},
		{0x1003, DID_RW,	sizeof(s_uds_did_e2rom_data.DID_1003), s_uds_did_e2rom_data.DID_1003, DID_DATA_TYPE_ASCII, NULL, NULL},
	  {0x1004, DID_RO,	sizeof(s_uds_did_e2rom_data.DID_1004), s_uds_did_e2rom_data.DID_1004, DID_DATA_TYPE_ASCII, NULL, NULL},
		{0x1028, DID_RW,	sizeof(s_uds_did_e2rom_data.DID_1028), s_uds_did_e2rom_data.DID_1028, DID_DATA_TYPE_ASCII, NULL, NULL},
		{0x102A, DID_RW,	sizeof(s_uds_did_e2rom_data.DID_102A), s_uds_did_e2rom_data.DID_102A, DID_DATA_TYPE_HEX,	 NULL, NULL},
		{0x102B, DID_RW,	sizeof(s_uds_did_e2rom_data.DID_102B), s_uds_did_e2rom_data.DID_102B, DID_DATA_TYPE_HEX,	 NULL, NULL},
		{0x102C, DID_RW,	sizeof(s_uds_did_e2rom_data.DID_102C), s_uds_did_e2rom_data.DID_102C, DID_DATA_TYPE_HEX,	 NULL, NULL},

		{0x1035, DID_RW,  sizeof(s_uds_did_e2rom_data.DID_1035), s_uds_did_e2rom_data.DID_1035, DID_DATA_TYPE_HEX,   NULL, NULL},	
		{0x2100, DID_RO,	sizeof(s_uds_did_e2rom_data.DID_2100), s_uds_did_e2rom_data.DID_2100, DID_DATA_TYPE_ASCII, NULL, NULL}, 
		{0x3102, DID_RO,	sizeof(s_uds_did_e2rom_data.DID_3102), s_uds_did_e2rom_data.DID_3102, DID_DATA_TYPE_HEX,	 NULL, NULL}, 
		{0x3103, DID_RO,	sizeof(s_uds_did_e2rom_data.DID_3103), s_uds_did_e2rom_data.DID_3103, DID_DATA_TYPE_HEX,	 NULL, NULL}, 
		{0xF182, DID_RO,  sizeof(s_uds_did_e2rom_data.DID_F182), s_uds_did_e2rom_data.DID_F182, DID_DATA_TYPE_ASCII, NULL, NULL},   
    {0xF187, DID_RO,  sizeof(s_uds_did_e2rom_data.DID_F187), s_uds_did_e2rom_data.DID_F187, DID_DATA_TYPE_ASCII, NULL, NULL},   
    {0xF190, DID_RW,  sizeof(s_uds_did_e2rom_data.DID_F190), s_uds_did_e2rom_data.DID_F190, DID_DATA_TYPE_ASCII, NULL, NULL}, 
    {0xF192, DID_RO,  sizeof(s_uds_did_e2rom_data.DID_F192), s_uds_did_e2rom_data.DID_F192, DID_DATA_TYPE_ASCII, NULL, NULL},
    {0xF193, DID_RO,  sizeof(s_uds_did_e2rom_data.DID_F193), s_uds_did_e2rom_data.DID_F193, DID_DATA_TYPE_ASCII, NULL, NULL},
    {0xF194, DID_RO,  sizeof(s_uds_did_e2rom_data.DID_F194), s_uds_did_e2rom_data.DID_F194, DID_DATA_TYPE_ASCII, NULL, NULL},  
    {0xF195, DID_RO,  sizeof(s_uds_did_e2rom_data.DID_F195), s_uds_did_e2rom_data.DID_F195, DID_DATA_TYPE_ASCII, NULL, NULL}, 
    {0xF198, DID_RW,  sizeof(s_uds_did_e2rom_data.DID_F198), s_uds_did_e2rom_data.DID_F198, DID_DATA_TYPE_ASCII, NULL, NULL}, 
    {0xF199, DID_RO,  sizeof(s_uds_did_e2rom_data.DID_F199), s_uds_did_e2rom_data.DID_F199, DID_DATA_TYPE_BCD,   NULL, NULL},  
    {0xF19D, DID_RW,  sizeof(s_uds_did_e2rom_data.DID_F19D), s_uds_did_e2rom_data.DID_F19D, DID_DATA_TYPE_BCD,   NULL, NULL},   
    {0xF1A1, DID_RO,  sizeof(s_uds_did_e2rom_data.DID_F1A1), s_uds_did_e2rom_data.DID_F1A1, DID_DATA_TYPE_ASCII, NULL, NULL},     
};
INT8U s_did_status[MAX_DID_NUM]= {0x00};
static INT8U s_did_num = MAX_DID_NUM;

/*******************************************************************************
** 函数名:    DID_DataUpdate
** 函数描述:   启动更新pp参数延时
** 参数:       无
** 返回:       无
******************************************************************************/
static void DID_DataUpdate(void)
{
    s_delay_save_to_flash = DELAY_SAVE_TIMEOUT;
}

/*******************************************************************************
** 函数名:    DID_DataUpdateDelay
** 函数描述:   启动更新pp参数延时
** 参数:       无
** 返回:       无
******************************************************************************/
static void DID_DataUpdateDelay(void)
{
    s_delay_save_to_flash = DELAY_SAVE_TIMEOUT;
}

/*******************************************************************************
 ** 函数名:    DTC_HandleTmr
 ** 函数描述:   定时器处理函数
 ** 参数:       无
 ** 返回:       无
 ******************************************************************************/
static void DID_HandleTmr(void* pdata)
{
    static INT8U s_delay_send_did = 0;
    INT8U i = 0, isNeedSaveToFlash;
    STREAM_T* wstrm;

    OS_StartTmr(s_did_tmr, _SECOND, 1);
    pdata = pdata;
		
		if(s_vehSpeed > 0) {                            /* 速度报文丢失1s恢复速度0 */
        if(s_vehSpeed_over++ >= 3) {
				    s_vehSpeed_over = 0;
						s_vehSpeed = 0;
        }
		}
		
    if (s_delay_save_to_flash) {
        s_delay_save_to_flash--;
        if (s_delay_save_to_flash == 0) {
            if (bal_pp_StoreParaByID(UDS_DID_PARA_, (INT8U *)&s_uds_did_e2rom_data, sizeof(UDS_DID_DATA_E2ROM_T))) {
                #if DEBUG_DID > 0
                debug_printf("主机UDS pp参数延时保存成功\r\n");
                #endif
            } else {
                #if DEBUG_DID > 0
                debug_printf("主机UDS pp参数延时保存失败\r\n");
                #endif              
            }
        }
    }

    if (YX_COM_Islink()) {
        if (s_delay_send_did < 4) {
            s_delay_send_did++;
            if (s_delay_send_did == 4) {
                s_delay_send_did = 0xff;
                isNeedSaveToFlash = FALSE;
                for (i = 0; i < MAX_DID_NUM; i++) {
                    if (s_did_status[i] == 1) {
                        s_did_status[i] = 0;
												if(i != 9){
                            isNeedSaveToFlash = TRUE;
												}
                        /* 上报到屏 */
                        wstrm = bal_STREAM_GetBufferStream();
                        bal_WriteHWORD_Strm(wstrm, CLIENT_CODE);//CLIENT_CODE
                        bal_WriteBYTE_Strm(wstrm, 0x20);
                        bal_WriteHWORD_Strm(wstrm, s_uds_did_obj[i].code);
                        bal_WriteBYTE_Strm(wstrm, (INT8U)s_uds_did_obj[i].len);
                        bal_WriteDATA_Strm(wstrm, s_uds_did_obj[i].data, s_uds_did_obj[i].len);
                        YX_COM_DirSend(CLIENT_FUNCTION_UP_REQ, bal_GetStrmStartPtr(wstrm), bal_GetStrmLen(wstrm));
                    }
                }
                if (isNeedSaveToFlash) {
                    isNeedSaveToFlash = FALSE;
                    // 更新状态到PP参数
                    bal_pp_StoreParaByID(UDS_DID_PARA_, (INT8U *)&s_uds_did_e2rom_data, sizeof(UDS_DID_DATA_E2ROM_T));
                }
                
            }
        }
    } else {
        s_delay_send_did = 0;
    }
}

#if 0
static void HdlMsg_DN_EXT_PRO(INT8U cmd, INT8U *data, INT16U datalen)
{
    INT8U    type;
    INT16U   flow_num;
    INT16U   len;
    INT16U   did;
    INT8U    ack[10];
    STREAM_T rstrm;

    if (cmd != UP_PE_ACK_EXT_PRO) {
        return;
    }

    YX_InitStrm(&rstrm, data, datalen);

    type     = YX_ReadBYTE_Strm(&rstrm);
    flow_num = YX_ReadHWORD_Strm(&rstrm);
    len      = YX_ReadHWORD_Strm(&rstrm);

    switch (type) {

        case 0x01:

            break;


        case 0x02:

            break;
            
         case 0x03:

            break;  
            
        case 0x04:
        
            break;
        
        
        case 0x05:
            did = YX_ReadHWORD_Strm(&rstrm);
            YX_HWToBigEndMode(ack, did);
            ack[2] = YX_UDS_DID_Down(did, (INT8U*)YX_GetStrmPtr(&rstrm), len - 2); 
            YX_EXT_PRO_Send(0x05, flow_num, ack, 3);
            break;
            
         case 0x06:

            break;  
    }
}

static FUNCENTRY_MMI_T s_functionentry[] = {
    {UP_PE_ACK_EXT_PRO,    HdlMsg_DN_EXT_PRO},
};



/*******************************************************************
** 函数名: YX_EXT_PRO_Send
** 函数描述: 通过标识符将控制请求传给主机模块
** 参数: [in] type: 功能表示
         [in] flow: 流水号
         [in] data: 控制数据
         [in] len: 数据长度
** 返回: NULL
********************************************************************/
void YX_EXT_PRO_Send(INT8U type, INT16U flow, INT8U* data, INT16U len)
{
    INT8U *memptr;
    INT16U memlen;
    STREAM_T wstrm;
    INT16U flow_tmp;

    static INT16U flow_num = 0;
    if (type == 0x05) {
        flow_tmp = flow;        
    } else {
       flow_num++; 
       flow_tmp = flow_num;  
    }

    memlen = len + 5;
    memptr = YX_DYM_Alloc(memlen);
    if (memptr == NULL) {
        return;
    }
    YX_InitStrm(&wstrm, memptr, memlen);
    YX_WriteBYTE_Strm(&wstrm,  type);            /* 协议命令 */
    YX_WriteHWORD_Strm(&wstrm, flow_tmp);        /* 流水号 */
    YX_WriteHWORD_Strm(&wstrm, len);             /* 数据长度 */
    YX_WriteDATA_Strm(&wstrm,  data, len);       /* 数据内容 */
    YX_COM_DirSend(DN_PE_CMD_EXT_PRO, YX_GetStrmStartPtr(&wstrm), YX_GetStrmLen(&wstrm));

    if (memptr != NULL) {
        YX_DYM_Free(memptr);
    }
}
#endif
/*******************************************************************
** 函数名: Jude_Diddata_Validity
** 函数描述: 判断DID数据合法性
** 参数: [in] did: 数据标识符
         [in] data: 写入数据
         [in] len: 数据长度
** 返回: TRUE:合法数据 FALSE: 不合法
********************************************************************/
static BOOLEAN Jude_Diddata_Validity(INT16U did, INT8U *data, INT8U len)
{
    INT8U j;
    INT16U phy_range;
    
    switch (did) {
        case 0x0110:
            for (j = 0; j < len; j++) {
                if (data[j] > 0x01) {
                    return FALSE;
                }
            } 					 
        break;
        case 0x0100:
            for (j = 0; j < len - 1; j++) {
                if (data[j] > 0x01) {
                    return FALSE;
                }
            }
            if(data[len - 1] > 0x0E) {
                return FALSE;	
            }
        break;
        /*case 0x1002:
        case 0x1003:
            for (j = 0; j < len; j++) {
                if (((data[j] > 0x39) && (data[j] < 0x41)) || ((data[j] > 0x5A) && (data[j] < 0x61))) {
                    return FALSE;
                }
            }
        break;*/
        case 0x102A:
            for (j = 0; j < len; j++) {
                switch (j) {
                    case 0x00:
                        if(data[j] > 18) {
                            return FALSE;
                        }
                    break;

                    case 0x01:
                        if(data[j] > 6) {
                            return FALSE;
                        }
                    break;

                    case 0x02:
                    case 0x08:
                    case 0x09:
                    case 0x13:
                    case 0x15:
                    case 0x17:
                    case 0x18:
                        if(data[j] > 2) {
                            return FALSE;
                        }
                    break;
                    case 0x03:
                    case 0x0C:
                        if(data[j] > 7) {
                            return FALSE;
                        }
                    break;
                    case 0x04:
                    case 0x11:
                        if(data[j] > 4) {
                            return FALSE;
                        }
                    break;
                    case 0x05:
                    case 0x06:
                    case 0x10:
                    case 0x14:
                    case 0x16:
                    case 0x19:
                        if(data[j] > 1) {
                            return FALSE;
                        }
                    break;
                    case 0x07:
                    case 0x0B:
                    case 0x0D:
                    case 0x0E:
                    case 0x0F:
                    case 0x12:
                        if(data[j] > 3) {
                            return FALSE;
                        }
                    break;
                    case 0x0A:
                        if(data[j] > 5) {
                            return FALSE;
                        }
                    break;
                    default:
                    break;
                }
        } 
        break;
        case 0x102B:
            for (j = 0; j < len; j++) {
                if (data[j] > 0x0A) {
                    return FALSE;
                }
            } 					 	 
        break;
        case 0x102C:
            for (j = 0; j < len; j++) {
                if (data[j] > 0x04) {
                   return FALSE;
                }
            } 						 
        break;
        case 0x1035:
        phy_range = (data[0] << 8) + data[1];
        if (phy_range > 5000) {
        return FALSE;
        }				 
        break;
        
        default:
        break;
    }
    return true;
}
/*******************************************************************
** 函数名: UDS_SID22_Response
** 函数描述: 通过标识符读数据肯定响应
** 参数: [in] did: 数据标识符
         [in] data: 肯定响应数据
         [in] len: 数据长度
** 返回: NULL
********************************************************************/
static void UDS_DID102A_Handle(void)
{
    if((s_uds_did_e2rom_data.DID_102A[18] == 0x00) && (s_car_signal != CAR_SIGNAL_QINGQI)) {     /* 无VIST */
		    Control_IdleWarmup();
    }
}
/*******************************************************************
** 函数名: UDS_SID22_Response
** 函数描述: 通过标识符读数据肯定响应
** 参数: [in] did: 数据标识符
         [in] data: 肯定响应数据
         [in] len: 数据长度
** 返回: NULL
********************************************************************/
static void UDS_SID22_Response(INT16U did, INT8U *data, INT8U len)
{
    STREAM_T wstrm;
    INT8U *memptr, memlen;

    memlen = len + 3;
    memptr = YX_MemMalloc(memlen);
    if (memptr == NULL) {
        return;
    }

    bal_InitStrm(&wstrm, memptr, memlen);

    bal_WriteBYTE_Strm(&wstrm, (SID_22 + RESP_ADD));                             /* 肯定响应标识 */
    bal_WriteHWORD_Strm(&wstrm, did);                                            /* 数据标识符 */
    bal_WriteDATA_Strm(&wstrm, data, len);

    #if DEBUG_DID > 0
    debug_printf("UDS_SID22_Response->did(%x),data len:(%d)\r\n", did, len);
    printf_hex(data, len);
    debug_printf("\r\n");
    #endif
    
    #if DEBUG_DID > 0
    printf_hex(bal_GetStrmStartPtr(&wstrm), bal_GetStrmLen(&wstrm));
    debug_printf("\r\n");
    #endif

    if (memlen < 8) {
        YX_UDS_CanSendSingle(bal_GetStrmStartPtr(&wstrm),bal_GetStrmLen(&wstrm));
    } else {
        YX_UDS_SendMulCanData(bal_GetStrmStartPtr(&wstrm), bal_GetStrmLen(&wstrm));
    }

    if (memptr != NULL) {
        YX_MemFree(memptr);
    }
}

/*******************************************************************
** 函数名: UDS_SID2E_Response
** 函数描述: 通过标识符写数据肯定响应
** 参数: [in] did: 数据标识符
** 返回: NULL
********************************************************************/
static void UDS_SID2E_Response(INT16U did)
{
    STREAM_T wstrm;
    INT8U memptr[8];
    
    bal_InitStrm(&wstrm, memptr, 8);

    bal_WriteBYTE_Strm(&wstrm, (SID_2E + RESP_ADD));                             /* 肯定响应标识 */
    bal_WriteHWORD_Strm(&wstrm, did);                                            /* 数据标识符 */

    #if DEBUG_DID > 0
    debug_printf("UDS_SID2E_Response->did(%x)", did);
    #endif

    YX_UDS_CanSendSingle(bal_GetStrmStartPtr(&wstrm),bal_GetStrmLen(&wstrm));
}

/*****************************************************************************
**  函数名:  DID_IsSelfDid
**  函数描述: 自定义did
**  参数:    [in] did :
**  返回:    0:执行失败 1:执行成功
*****************************************************************************/
static BOOLEAN DID_IsSelfDid(INT16U did)
{
    did = did;
    return FALSE;
}

/*
********************************************************************************
* 定义对外接口
********************************************************************************
*/

/*******************************************************************
** 函数名: YX_UDS_DID_SID22_ReadDataByIdentifier
** 函数描述: 通过标识符读数据
** 参数: [in] did: 数据标识
** 返回: NULL
********************************************************************/
void YX_UDS_DID_SID22_ReadDataByIdentifier(INT8U* data, INT16U len)
{
    INT16U did;
    INT8U i, didLen, *pData;
    INT8U j, num;
    STREAM_T *wstrm;
    INT8U memlen = 0;

    num = len / 2;

    if (num != 1) {
        wstrm = bal_STREAM_GetBufferStream();
        bal_WriteBYTE_Strm(wstrm, (SID_22 + RESP_ADD));                             /* 肯定响应标识 */
    }

    for (j = 0; j < num; j++) {
        did = (INT16U)YX_BigEndModeToHWord(data+j*2);

    // 判断是否为自定义did
    if (DID_IsSelfDid(did)) {
        return;
    }
    
    for (i = 0; i < s_did_num; i++) {
        if (s_uds_did_obj[i].code == did) {
            // 判断是否可读
            if (!(s_uds_did_obj[i].rw & DID_RO)) {
                YX_UDS_NegativeResponse(SID_22, NRC_31);
                return;        
            }            
            break;
        }
    }

    if (i == s_did_num) {
        YX_UDS_NegativeResponse(SID_22, NRC_31);
        return;
    }
    
    didLen = s_uds_did_obj[i].len;
    pData  = s_uds_did_obj[i].data;

    if (s_uds_did_obj[i].didReadFun != NULL) {
        s_uds_did_obj[i].didReadFun(pData, didLen);
    }
    
    if (num == 1) {
        UDS_SID22_Response(did, pData, didLen);
    } else {
            bal_WriteHWORD_Strm(wstrm, did);                                            /* 数据标识符 */
            bal_WriteDATA_Strm(wstrm, pData, didLen);
            memlen += (2 + didLen);
            if (j == num - 1) {
                if (memlen < 8) {
                    YX_UDS_CanSendSingle(bal_GetStrmStartPtr(wstrm),  bal_GetStrmLen(wstrm));
                } else {
                    YX_UDS_SendMulCanData(bal_GetStrmStartPtr(wstrm), bal_GetStrmLen(wstrm));
                }
            }
        }
    }

}

/*******************************************************************
** 函数名: YX_UDS_DID_SID2E_WriteDataByIdentifier
** 函数描述: 通过标识符写数据
** 参数: [in] did: 数据标识
         [in] data: 待写数据
         [in] len: 数据长度
** 返回: NULL
********************************************************************/
void YX_UDS_DID_SID2E_WriteDataByIdentifier(INT8U *data, INT8U len)
{
    INT8U* p_did;
    INT16U did, didLen;
    INT8U i, j;
    STREAM_T* wstrm;
    BOOLEAN is_save_pp;
    INT8U level;

    did = (INT16U)YX_BigEndModeToHWord(data);
    level = YX_UDS_GetSecurity();
    
    // 会话判断
    if (YX_UDS_GetSession() == SESSION_PROGRAM) {
        if (did != 0xF184) {
            YX_UDS_NegativeResponse(SID_2E, NRC_31);
            return;
        }
    } else if (YX_UDS_GetSession() == SESSION_EXTENDED) {
        ;
    } else {
        YX_UDS_NegativeResponse(SID_2E, NRC_7F);
        return;
    }

    if (len < 2) {
        YX_UDS_NegativeResponse(SID_2E, NRC_13);
        return;
    }

    // 安全等级判断
    if (level == (REQSEED_1 + 1)) {
        ;
    } else if (level == (REQSEED_3 + 1)) {
        if (did != 0xF184) {
            YX_UDS_NegativeResponse(SID_2E, NRC_31);
            return;   
        }
    } else if (level == (REQSEED_5 + 1)) {
        if (did != 0xF184) {
            YX_UDS_NegativeResponse(SID_2E, NRC_31);
            return;   
        }
    } else {
        YX_UDS_NegativeResponse(SID_2E, NRC_33);
        return;
    }
    
    #if DEBUG_DID > 0
    debug_printf("<**** 2e did:0x%x ****>\r\n",did);
    printf_hex(data, len);
    #endif  
    
    didLen = len - 2;
    for (i = 0; i < s_did_num; i++) {
        if (s_uds_did_obj[i].code == did) {
            // 判断是否可写
            if (!(s_uds_did_obj[i].rw & DID_WO)) {
                YX_UDS_NegativeResponse(SID_2E, NRC_31);
                return;        
            }    
            break;
        }
    }
    
    if (i == s_did_num) {
        YX_UDS_NegativeResponse(SID_2E, NRC_31);
        return;
    }

    
    if (didLen != s_uds_did_obj[i].len) {
        YX_UDS_NegativeResponse(SID_2E, NRC_13);
        return;
    }    

    // data前2字节为did
    p_did = &data[2];
    switch (s_uds_did_obj[i].datatype) {
        case DID_DATA_TYPE_ASCII:
            /* 判断是否为ASIIC范围 */
            for (j = 0; j < didLen; j++) {
                if (p_did[j] > 0x7F) {
                    YX_UDS_NegativeResponse(SID_2E, NRC_31);
                    return;
                }
            }            
            break;
            
        case DID_DATA_TYPE_BCD:
            /* 判断是否为ASIIC范围 */
            for (j = 0; j < didLen; j++) {
                if (((p_did[j] & 0x0F) > 0x09) || ((p_did[j] & 0xF0) > 0x90)) {
                    YX_UDS_NegativeResponse(SID_2E, NRC_31);
                    return;
                }
            }              
            break;
        case DID_DATA_TYPE_HEX:

            break;
        default:
            break;
    }

    if(!Jude_Diddata_Validity(did, p_did, didLen)) {
        YX_UDS_NegativeResponse(SID_2E, NRC_31);
        return;
    }
    is_save_pp = FALSE;

    /* 检查是否有更新数据 */
    for (j = 0; j < didLen; j++) {
        if (p_did[j] != s_uds_did_obj[i].data[j]) {
            /* 保存到pp参数 */
            is_save_pp = TRUE;
            break;
        }
    }

    if (is_save_pp == TRUE) {
        YX_UDS_NegativeResponse(SID_2E, NRC_78);
        
         memcpy(s_uds_did_obj[i].data, p_did, didLen);
				 if(did == 0x0100) {
				     YX_Set_CarSignal(s_uds_did_obj[i].data);	
						 UDS_DID102A_Handle();
				 }
				 if(did == 0x102A) {
				     UDS_DID102A_Handle();
				 }
         if (did != 0xF184) {
            DID_DataUpdateDelay();
            #if DEBUG_DID > 0
            debug_printf("<**** 保存2E服务下发的did(0x%x)到pp参数延时启动 ****>\r\n",did);
            #endif            
        }
    }    
    UDS_SID2E_Response(did); 

    
    if (YX_COM_Islink()) {
        /* 上报到屏 */
        wstrm = bal_STREAM_GetBufferStream();
        bal_WriteHWORD_Strm(wstrm, CLIENT_CODE);//CLIENT_CODE
        bal_WriteBYTE_Strm(wstrm, 0x20);
        bal_WriteHWORD_Strm(wstrm, did);
        bal_WriteBYTE_Strm(wstrm, (INT8U)didLen);
        bal_WriteDATA_Strm(wstrm, p_did, didLen);
        YX_COM_DirSend(CLIENT_FUNCTION_UP_REQ, bal_GetStrmStartPtr(wstrm), bal_GetStrmLen(wstrm));

    } else {
        s_did_status[i] = 1;
        DID_DataUpdateDelay();
    }
    
    #if DEBUG_DID > 0
    debug_printf("\r\n<**** 2E服务上报MPU did(0x%x),len(%d) ****>\r\n", did, didLen);
    printf_hex(p_did, didLen);
    #endif        
}

/*******************************************************************
** 函数名: YX_UDS_DID_Down
** 函数描述: 主机UDS数据同步
** 参数: [in] did  : did号
         [in] data : 数据内容
         [in] len  : 数据长度
** 返回: 无
********************************************************************/
BOOLEAN YX_UDS_DID_Down(INT16U did, INT8U *data, INT8U len)
{
    INT8U i, j;
    BOOLEAN savepp;

    for (i = 0; i < s_did_num; i++) {
        if (s_uds_did_obj[i].code == did) {
            break;
        }
    }
    
    if (i == s_did_num) { 
        #if DEBUG_DID > 0
        debug_printf("<***** YX_UDS_DID_Down:did(0x%x)不存在 *****>\r\n",did);
        #endif
        return FALSE;
    }

    #if DEBUG_DID > 0
    debug_printf("<***** YX_UDS_DID_Down:0x%x,len(%d) *****>\r\n",did, len);
    #endif    
    
    if (len != s_uds_did_obj[i].len) {
        #if DEBUG_DID > 0
        debug_printf("<***** YX_UDS_DID_Down:长度不对 *****>\r\n");
        #endif        
        return FALSE;
    }

    savepp = FALSE;

    for (j = 0; j < len; j++) {
        if (s_uds_did_obj[i].data[j] != data[j]) {
            savepp = TRUE;
            break;
        }
    }

    if (savepp) {
        #if DEBUG_DID > 0
        debug_printf("<**** (2)保存主机下发的did到pp参数延时启动 ****>\r\n");
        #endif
        if (s_did_status[i] == 0) {         
            /* did == 0xF1A1 ||did == 0xF182 || did == 0xF187 为MCU默认配置，无需主机同步 */
            if ((did == 0xF190) || (did == 0xF193) || (did == 0xF195) || (did == 0xF19D) || (did == 0x1004) ||
							  (did == 0x1002) || (did == 0x1003) || (did == 0x1028) || (did == 0x1009) || (did == 0x1010) || 
							  (did == 0x1015) || (did == 0x3102) || (did == 0x3103) || (did == 0xF194) || (did == 0x2100) ||
							  (did == 0xF192) || (did == 0xF199)) {
							  YX_MEMCPY(s_uds_did_obj[i].data,len, data, len);
                DID_DataUpdate();
            }
        }

    }

    return TRUE;
}

/*******************************************************************
** 函数名: YX_UDS_DID_SaveDataToFlash
** 函数描述:DID数据立即保存到pp参数
** 参数: 无
** 返回: 无
********************************************************************/
void YX_UDS_DID_SaveDataToFlash(void)
{
    if (s_delay_save_to_flash) {
        s_delay_save_to_flash = 0;      
        if (bal_pp_StoreParaByID(UDS_DID_PARA_, (INT8U *)&s_uds_did_e2rom_data, sizeof(UDS_DID_DATA_E2ROM_T))) {
            #if DEBUG_DID > 0
            debug_printf("主机UDS pp参数保存成功\r\n");
            #endif
        } else {
            #if DEBUG_DID > 0
            debug_printf("主机UDS pp参数保存失败\r\n");
            #endif              
        }        
    }
}

/*****************************************************************************
**  函数名:  YX_UDS_DID_UpdataCanData
**  函数描述: 更新can报文数据
**  参数:    [in] canId :
**           [in] data  :    intel格式报文内容
**           [in] len   :
**  返回:    无
*****************************************************************************/
void YX_UDS_DID_UpdataCanData(INT32U canId, INT8U* data, INT8U len)
{
    if (len != 8) return;

    if (canId == 0x18FEE017) {          // 总里程
        s_can_signal.totalDis[0] = data[4];
        s_can_signal.totalDis[1] = data[5];
        s_can_signal.totalDis[2] = data[6];
        s_can_signal.totalDis[3] = data[7];
        
    } else if (canId == 0x18FEE900) {   // 总油耗
        s_can_signal.tolalFuelCon[0] = data[4];
        s_can_signal.tolalFuelCon[1] = data[5];
        s_can_signal.tolalFuelCon[2] = data[6];
        s_can_signal.tolalFuelCon[3] = data[7];

    } else if (canId == 0x18FEF200) {   // 瞬时燃油消耗率
        s_can_signal.fuelRate[0] = data[0];
        s_can_signal.fuelRate[1] = data[1];
        
    } else if (canId == 0x0CFE6C17) {   // 仪表车速
        s_can_signal.vehSpeed[0] = data[6];
        s_can_signal.vehSpeed[1] = data[7];
				s_vehSpeed = (data[6] << 8) + data[7]; 
				s_vehSpeed_over = 0;
    } else if (canId == 0x0CF00400) {   // 发动机转速
        s_can_signal.engSpeed[0] = data[3];
        s_can_signal.engSpeed[1] = data[4];
        
    }
}

/*****************************************************************************
**  函数名:  YX_UDS_DID_DataReset
**  函数描述: 初始化did数据
**  参数:    无
**  返回:    无
*****************************************************************************/
void YX_UDS_DID_DataReset(void)
{
    YX_MEMSET(s_uds_did_local.DID_102D, 0xFF, sizeof(s_uds_did_local.DID_102D));
    YX_MEMSET(s_uds_did_local.DID_102E, 0xFF, sizeof(s_uds_did_local.DID_102E));
    YX_MEMSET(s_uds_did_local.DID_1030, 0xFF, sizeof(s_uds_did_local.DID_1030));
    YX_MEMSET(s_uds_did_local.DID_1031, 0xFF, sizeof(s_uds_did_local.DID_1031));
    YX_MEMSET(s_uds_did_local.DID_1036, 0xFF, sizeof(s_uds_did_local.DID_1036));
    
    YX_MEMSET(&s_can_signal, 0xFF, sizeof(s_can_signal));
}
/*****************************************************************************
**  函数名:  YX_Set_CarSignal(
**  函数描述:根据did数据配置车型
**  参数:    无
**  返回:    无
*****************************************************************************/
void YX_Set_CarSignal(INT8U *data)
{
    INT8U type;

		type = data[15];
    if((type == 0x02) || (type == 0x07)) {
		    s_car_signal = CAR_SIGNAL_J6;
    } else if((type == 0x03) || (type == 0x08)) {
        s_car_signal = CAR_SIGNAL_J6_MID;
    } else if((type == 0x0C) || (type == 0x0D)) {
        s_car_signal = CAR_SIGNAL_QINGQI;
    } else {
         s_car_signal = CAR_SIGNAL_J6;
    }
		YX_CarSignal_AdapterDTC(s_car_signal,data);
}
#if 0
/*****************************************************************************
**  函数名:  YX_Get_CarSignal(
**  函数描述:获取车型
**  参数:    无
**  返回:    无
*****************************************************************************/
CAR_SIGNAL_TYPE_E YX_Set_CarSignal(void)
{
    return  s_car_signal; 
}
#endif
/*****************************************************************************
**  函数名:   YX_Get_EMSType
**  函数描述: 获取EMS类型
**  参数:     无
**  返回:     无
*****************************************************************************/
INT8U YX_Get_EMSType(void)
{
    return s_uds_did_e2rom_data.DID_102A[0];
}
/*****************************************************************************
**  函数名:   YX_Get_VehSpeed
**  函数描述: 获取车速
**  参数:     无
**  返回:     无
*****************************************************************************/
INT16U YX_Get_VehSpeed(void)
{
    return s_vehSpeed;
}
/*******************************************************************
** 函数名: YX_UDS_DID_Init
** 函数描述:DID数据初始化
** 参数: 无
** 返回: 无
********************************************************************/
void YX_UDS_DID_Init(void)
{
    //char* img_inf;

    YX_MEMSET((INT8U*)&s_can_signal, 0x00, sizeof(s_can_signal));
    YX_MEMSET((INT8U *)&s_uds_did_local, 0x00, sizeof(UDS_DID_LOCAL_T));
    
    YX_UDS_DID_DataReset();
    
    //img_inf = YX_GetVersionDate();
    memcpy(s_uds_did_local.DID_103A, s_did_103a, sizeof(s_uds_did_local.DID_103A));
    s_did_status[9] = 1;
    s_delay_save_to_flash = 0;
    if (!bal_pp_ReadParaByID(UDS_DID_PARA_, (INT8U *)&s_uds_did_e2rom_data, sizeof(UDS_DID_DATA_E2ROM_T))) {
        #if DEBUG_DID > 0
        debug_printf("<<YX_UDS_DID_Init 读取主机UDS参数失败>>\r\n");
        #endif
    } 
    YX_Set_CarSignal(s_uds_did_e2rom_data.DID_0100);
		UDS_DID102A_Handle();
    s_did_tmr = OS_InstallTmr(TSK_ID_OPT, 0, DID_HandleTmr);
    OS_StartTmr(s_did_tmr, _SECOND, 1);

    // num = sizeof(s_functionentry) / sizeof(s_functionentry[0]);
    // for (i = 0; i < num; i++) {
    //     YX_COM_RecvRegister(s_functionentry[i].cmd, s_functionentry[i].entryproc);
    // }
}

#endif

