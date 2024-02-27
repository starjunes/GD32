/********************************************************************************
**
** 文件名: yx_uds_drv.c
** 版权所有: (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述: 该模块主要实现统一诊断服务功能
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================

*********************************************************************************/
#include "yx_includes.h"
#include "bal_stream.h"
#include "yx_dtc_drv.h"
#include "dal_can.h"
#include "yx_uds_drv.h"
#include "bal_input_drv.h"
#include "bal_pp_drv.h"
#include "port_dm.h"
#include "yx_uds_drv.h"
#include "bal_gpio_cfg.h"
#include "yx_com_man.h"
#include "yx_uds_did.h"
#include "port_plat.h"
#include "bal_tools.h"
#if EN_UDS > 0
#if EN_UDS_TRANS > 0
#include "yx_uds_transfer.h"
#endif

#include "yx_can_man.h"
/*
********************************************************************************
* 宏定义
********************************************************************************
*/
#define PERIOD_TICK                 1
#define UDS_PERIOD                  _TICK, PERIOD_TICK                

#define UDS_27_NRC_78               0    /* 1:错误计数需要回NRC78后保存pp参数再应答    0:立即应答 */ 

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/

typedef struct {
    INT8U session_type;                 /* 当前诊断会话类别 */
    BOOLEAN en_session;                 /* 维持非默认会话计数使能 */
    INT16U session_cnt;                 /* 维持非默认会话计数 */
    INT8U access_status;                /* 安全访问状态 0x0:锁定 0x1:请求种子 0x2:解锁(级别1) 0x3:解锁(级别2) 0x4:解锁(级别3) */
    INT16U access_fault_cnt;            /* 安全访问失败计数 */
    INT16U access_fault_cnt_pre;        /* 安全访问失败计数前一个值 */
    BOOLEAN en_access_wait;             /* 安全访问等待计数使能 */
    INT16U access_wait_cnt;             /* 安全访问等待计数 */
    INT8U access_seed[4];               /* 安全访问种子 */
    INT8U access_key[4];                /* 安全访问密钥 */
    BOOLEAN en_setdtc;                  /* 设置故障码使能 */
    INT8U reset_type;                   /* 复位类型 */
    INT16U reset_cnt;                   /* 复位等待计数 */
} UDS_MODULE_STATUS_T; 

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/

static INT32U              s_reqid;
static INT32U              s_rand_cnt = 0;
static BOOLEAN             s_can_recv_sw = TRUE;

static INT8U               s_udstmr;
static UDS_MODULE_STATUS_T s_uds_module;
static UDS_ERR_CNT_T       s_uds_fault_cnt = {0};

#if UDS_27_NRC_78 == 0
#define UDS_TIMEOUT        500
static INT16U              s_uds_timeout = 0;
#endif
/*
********************************************************************************
* 定义本地接口
********************************************************************************
*/	

/*****************************************************************************
**  函数名:  UdsStartTimeout
**  函数描述: 启动超时
**  参数:    无
**  返回:    无
*****************************************************************************/
static void UdsStartTimeout(void)
{
    s_uds_timeout = UDS_TIMEOUT;
}

/*****************************************************************************
**  函数名:  Update_UDS_Fault_Cnt
**  函数描述: 更新错误计数
**  参数:    [in] fault_cnt : 新错误计数值
**  返回:    无
*****************************************************************************/
static void Update_UDS_Fault_Cnt(void)
{
    if (s_uds_module.access_fault_cnt_pre != s_uds_module.access_fault_cnt) {
        s_uds_module.access_fault_cnt_pre = s_uds_module.access_fault_cnt;
        s_uds_fault_cnt.fault_cnt = s_uds_module.access_fault_cnt;
        bal_pp_StoreParaByID(UDS_ERRCNT_PARA_, (INT8U *)&s_uds_fault_cnt, sizeof(UDS_ERR_CNT_T));
    }
}

/*******************************************************************
** 函数名: Get_Rand
** 函数描述: 获得随机数
** 参数: [in] len: 随机数长度
         [out] random: 随机数
** 返回: 无
********************************************************************/
static void Get_Rand(INT8U len, INT8U *random)
{
    INT32U rand_tmp;
    len = len;
    rand_tmp = s_rand_cnt;  
    if ((rand_tmp == 0) || (rand_tmp == 0xFFFFFFFF)) {
        rand_tmp = 0x01346595;
    }
    YX_DWToLitEndMode(random, rand_tmp);
    s_rand_cnt++;
}
/*******************************************************************
** 函数名: Get_SecurityKey
** 函数描述: 获得安全访问密钥
** 参数: [in] accesstype : 访问等级，见@ REQSEED_TYPE_E
         [in] seed_val   : seed
**       [out] key       : 输出key值
** 返回: 无
********************************************************************/
static void Get_SecurityKey(INT8U accesstype, INT8U* seedval, INT8U* keyval)
{
    INT8U i;
    INT32U seed, key, algorithmask;

    seed = 0;
    key  = 0;
    if (accesstype == REQSEED_1) {
        algorithmask = 0x1DB7E8D9;
    } else if (accesstype == REQSEED_5) {
        algorithmask = 0x2DD3BE5D;
    } else {
        algorithmask = 0x37313533;
    }
    
    seed = (INT32U)(seedval[0] << 24) + (INT32U)(seedval[1] << 16) + (INT32U)(seedval[2] << 8) + (INT32U)(seedval[3]);

    if (!((seed == 0) || (seed == 0xFFFFFFFF))) {
        for(i = 0; i < 35; i++) {
            if(seed & 0x80000000) {
                seed = seed << 1;
                seed = seed ^ algorithmask;
            } else {
                seed = seed << 1;
            }
        }
        key = seed;
    }
    keyval[3] = key;
    keyval[2] = key >> 8;
    keyval[1] = key >> 16;
    keyval[0] = key >> 24;   
}
/*******************************************************************
** 函数名: Req_CmnctCtl
** 函数描述: 请求通信控制
** 参数: [in] ctltype: 控制类型 见 CMNCT_CTLTYPE_E
         [in] cmncttype: 通信类型
** 返回: 无
********************************************************************/
void Req_CmnctCtl(INT8U ctltype, INT8U cmncttype)
{
    #if DEBUG_UDS > 0
    debug_printf("Req_CmnctCtl ctltype:%d  cmncttype:%d\r\n", ctltype, cmncttype);
    #endif
    switch (cmncttype) {
        case UDS_COMTYPE_APP:
            if (ctltype & 0x01) {
                HAL_CAN_SendIdAccessSet(CAN_CHN_1, TRUE, UDS_PHSCL_RESPID, 0);
                HAL_CAN_SendIdAccessSet(CAN_CHN_2, TRUE, UDS_PHSCL_RESPID, 0);
                #if DEBUG_UDS > 0
                debug_printf("常规应用报文 禁止发送\r\n");
                #endif
            } else {
                HAL_CAN_SendIdAccessSet(CAN_CHN_1, FALSE, UDS_PHSCL_RESPID, 0);
                HAL_CAN_SendIdAccessSet(CAN_CHN_2, FALSE, UDS_PHSCL_RESPID, 0);
                
                dal_CAN_Reset_PeriodSendPeriod();
                #if DEBUG_UDS > 0
                debug_printf("常规应用报文 允许发送\r\n");
                #endif
            }
            if (ctltype & 0x02) {
                s_can_recv_sw = FALSE;
                #if DEBUG_UDS > 0
                debug_printf("常规应用报文 禁止接收\r\n");
                #endif
            } else {
                s_can_recv_sw = TRUE;
                #if DEBUG_UDS > 0
                debug_printf("常规应用报文 允许接收\r\n");
                #endif
            }
            break;
        #if 0
        case UDS_COMTYPE_NETWORK:
            if (ctltype & 0x01) {
                PORT_Nm_Silent();
                #if DEBUG_UDS > 0
                debug_printf("网络管理报文 禁止发送\r\n");
                #endif
            } else {
                PORT_Nm_Talk();
                #if DEBUG_UDS > 0
                debug_printf("网络管理报文 允许发送\r\n");
                #endif
            }
            if (ctltype & 0x02) {
                PORT_Nm_ReceiveSwitch(FALSE);
                #if DEBUG_UDS > 0
                debug_printf("网络管理报文 禁止接收\r\n");
                #endif
            } else {
                PORT_Nm_ReceiveSwitch(TRUE);
                #if DEBUG_UDS > 0
                debug_printf("网络管理报文 允许接收\r\n");
                #endif
            }
            break;
		#endif	
        case UDS_COMTYPE_ALL:
            if (ctltype & 0x01) {
                HAL_CAN_SendIdAccessSet(CAN_CHN_1, TRUE, UDS_PHSCL_RESPID, 0);
                HAL_CAN_SendIdAccessSet(CAN_CHN_2, TRUE, UDS_PHSCL_RESPID, 0);

                //PORT_Nm_Silent();
                #if DEBUG_UDS > 0
                debug_printf("常规应用和网络管理报文 禁止发送\r\n");
                #endif
            } else {
                HAL_CAN_SendIdAccessSet(CAN_CHN_1, FALSE, UDS_PHSCL_RESPID, 0);
                HAL_CAN_SendIdAccessSet(CAN_CHN_2, FALSE, UDS_PHSCL_RESPID, 0);
                dal_CAN_Reset_PeriodSendPeriod();
                //PORT_Nm_Talk();
                #if DEBUG_UDS > 0
                debug_printf("常规应用和网络管理报文 允许发送\r\n");
                #endif
            }
            if (ctltype & 0x02) {
                s_can_recv_sw = FALSE;
                //PORT_Nm_ReceiveSwitch(FALSE);
                #if DEBUG_UDS > 0
                debug_printf("常规应用和网络管理报文 禁止接收\r\n");
                #endif
            } else {
                s_can_recv_sw = TRUE;
                //PORT_Nm_ReceiveSwitch(TRUE);
                #if DEBUG_UDS > 0
                debug_printf("常规应用和网络管理报文 允许接收\r\n");
                #endif
            }
            break;
        
        default:
            break;
    }
}

/*******************************************************************
** 函数名: UDS_Reset_ServiceStatus
** 函数描述: 重置UDS服务状态
** 参数: 无
** 返回: 无
********************************************************************/
static void UDS_Reset_ServiceStatus(void)
{
    /* 复位27服务:安全访问闭锁,关闭等待计数器,等待计数器清零,失败计数器减1或重置为1 */
    s_uds_module.access_status = 0x00;
    //s_uds_module.en_access_wait = FALSE;
    s_uds_module.access_wait_cnt = 0;

    #if DEBUG_UDS > 0
    debug_printf("<uds重置，27服务，wait:%x,status:%x,fault_cnt:%x,access_wait_cnt:%x>\r\n",s_uds_module.en_access_wait, s_uds_module.access_status, 
                                    s_uds_module.access_fault_cnt, s_uds_module.access_wait_cnt);
    #endif      

    /* 复位28服务:CAN使能收发 */
    // 允许发送
    HAL_CAN_SendIdAccessSet(CAN_CHN_1, FALSE, UDS_PHSCL_RESPID, 0);
    HAL_CAN_SendIdAccessSet(CAN_CHN_2, FALSE, UDS_PHSCL_RESPID, 0);
    // 允许接收
    s_can_recv_sw = TRUE;

    /* 复位85服务:打开DTC设置 */
    s_uds_module.en_setdtc = TRUE;


    /* 复位2F服务:暂无 */
}

/*******************************************************************
** 函数名: UDS_SID10_Response
** 函数描述: 诊断会话控制肯定响应
** 参数: [in] sessiontype: 会话类别
** 返回: NULL
********************************************************************/
static void UDS_SID10_Response(INT8U sessiontype)
{
    INT16U p2_temp;
    INT8U memptr[8];
    STREAM_T wstrm;
    bal_InitStrm(&wstrm, memptr, 8);

    bal_WriteBYTE_Strm(&wstrm, (SID_10 + RESP_ADD));                             /* 肯定响应标识*/
    bal_WriteBYTE_Strm(&wstrm, sessiontype);                                     /* 会话类别*/

    /* P2Server */
    p2_temp = P2_SERVER;
    bal_WriteBYTE_Strm(&wstrm, p2_temp >> 8);                                    /* P2:请求响应间隔,高字节 */
    bal_WriteBYTE_Strm(&wstrm, p2_temp & 0xFF);                                  /* P2:请求响应间隔,低字节 */

    /* P2*Server */
    p2_temp = EXT_P2_SERVER;
    bal_WriteBYTE_Strm(&wstrm, p2_temp >> 8);                                    /* P2*:增强超时时间间隔,高字节 */
    bal_WriteBYTE_Strm(&wstrm, p2_temp & 0xFF);                                  /* P2*:增强超时时间间隔,低字节 */

    #if DEBUG_UDS > 0
    debug_printf("UDS_SID10_Response->sessiontype(%x)\r\n", sessiontype);
    #endif

    YX_UDS_CanSendSingle(bal_GetStrmStartPtr(&wstrm),bal_GetStrmLen(&wstrm));
}

/*******************************************************************
** 函数名: UDS_SID10_DiagnosticSessionControl
** 函数描述: 诊断会话控制
** 参数: [in] sessiontype: 会话类别
** 返回: NULL
********************************************************************/
static void UDS_SID10_DiagnosticSessionControl(INT8U sessiontype)
{
    BOOLEAN response;
    //IMG_INF_T* img_inf;
    
    response = FALSE;

    switch (sessiontype) {
        case SESSION_DEFAULT:
        case (SESSION_DEFAULT + SUPPRESS_RESP):
            if (s_uds_module.session_type != SESSION_DEFAULT) {
                UDS_Reset_ServiceStatus();
            }
            s_uds_module.session_type = SESSION_DEFAULT;
            s_uds_module.session_cnt = 0;
            s_uds_module.en_session = FALSE;
            if (sessiontype == SESSION_DEFAULT) {
                response = TRUE;
            }
            break;
        case SESSION_PROGRAM:
        case (SESSION_PROGRAM + SUPPRESS_RESP):
            /* 编程会话在功能请求不支持 */
            if ((s_reqid == FUNC_REQID) || (s_uds_module.session_type == SESSION_DEFAULT)) {
                YX_UDS_NegativeResponse(SID_10, NRC_7E);
                return;
            } else {
                /* 进入boot模式，关闭常规报文处理 */
                //Req_CmnctCtl(UDS_COMCTR_DISABLE_ALL, UDS_COMTYPE_ALL);
                s_uds_module.session_type = SESSION_PROGRAM;
                s_uds_module.session_cnt = 0;
                s_uds_module.access_fault_cnt = 0;
                s_uds_module.en_access_wait = FALSE;
                s_uds_module.access_wait_cnt = 0;
                s_uds_module.access_status = 0;
                s_uds_module.en_session = TRUE;

                #if DEBUG_UDS > 0
                debug_printf("设置当前模式  s_uds_module.session_type(%d)\r\n", s_uds_module.session_type);
                #endif
				
				// YX_UDS_NegativeResponse(SID_10, NRC_78);
				// Update_UDS_Fault_Cnt(s_uds_module.access_fault_cnt);

                // img_inf = GetImgInfObj();
                // /* 设置boot升级标志 */
                // img_inf->boot = 0x5aa5;
                
                if (!OS_TmrIsRun(s_udstmr)) {
                    OS_StartTmr(s_udstmr, UDS_PERIOD);
                }
                if (sessiontype == SESSION_PROGRAM) {
                    response = TRUE;
                }
                /* 启动硬件复位 */
                // s_uds_module.reset_cnt = 0;
                // s_uds_module.reset_type = RESET_HARD;
            }
            break;
        case SESSION_EXTENDED:
        case (SESSION_EXTENDED + SUPPRESS_RESP):
            if (s_uds_module.session_type == SESSION_PROGRAM) {
                YX_UDS_NegativeResponse(SID_10, NRC_7E);
                return;
            } 
            if (s_uds_module.session_type != SESSION_DEFAULT) {
                s_uds_module.access_status = 0x00;
            }
            s_uds_module.session_type = SESSION_EXTENDED;
            s_uds_module.session_cnt = 0;
            s_uds_module.en_session = TRUE;
            if (!OS_TmrIsRun(s_udstmr)) {
                OS_StartTmr(s_udstmr, UDS_PERIOD);
            }
            if (sessiontype == SESSION_EXTENDED) {
                response = TRUE;
            }
            break;
        default :
            YX_UDS_NegativeResponse(SID_10, NRC_12);
            break;
    }

    if (response) {
        UDS_SID10_Response(sessiontype & (~SUPPRESS_RESP));
    }
}

/*******************************************************************
** 函数名: UDS_SID11_Response
** 函数描述: 电控单元复位肯定响应
** 参数: [in] resettype: 复位类别
** 返回: NULL
********************************************************************/
static void UDS_SID11_Response(INT8U resettype)
{
    INT8U memptr[8];
    STREAM_T wstrm;
    bal_InitStrm(&wstrm, memptr, 8);

    bal_WriteBYTE_Strm(&wstrm, (SID_11 + RESP_ADD));                             /* 肯定响应标识 */
    bal_WriteBYTE_Strm(&wstrm, resettype);                                       /* 复位类型 */

    #if DEBUG_UDS > 0
    debug_printf("UDS_SID11_Response->resettype(%x)\r\n", resettype);
    #endif
    YX_UDS_CanSendSingle(bal_GetStrmStartPtr(&wstrm),bal_GetStrmLen(&wstrm));
}

/*******************************************************************
** 函数名: UDS_SID11_EcuReset
** 函数描述: 电控单元复位
** 参数: [in] resettype: 复位类别
** 返回: NULL
********************************************************************/
static void UDS_SID11_EcuReset(INT8U resettype)
{
    BOOLEAN response;

    response = FALSE;

    switch (resettype) {
        case RESET_HARD:
        case (RESET_HARD + SUPPRESS_RESP):
        case RESET_SOFT:
        case (RESET_SOFT + SUPPRESS_RESP):
            if ((s_uds_module.session_type == SESSION_DEFAULT)) {
                YX_UDS_NegativeResponse(SID_11, NRC_7F);
                return;
            }

            s_uds_module.reset_type = (resettype & (~SUPPRESS_RESP));
            s_uds_module.reset_cnt = 0;
            if (s_uds_module.reset_type == RESET_HARD) {
                YX_UDS_NegativeResponse(SID_11, NRC_78);
                /* 复位前先更新dtc的pp参数 */
                YX_DTC_PP_Update_When_Reset();  

                /* 复位前先更新did的pp参数 */
                YX_UDS_DID_SaveDataToFlash();

                /* 复位前保存错误计数 */
                Update_UDS_Fault_Cnt();
            }
            if (!OS_TmrIsRun(s_udstmr)) {
                OS_StartTmr(s_udstmr, UDS_PERIOD);
            }
            if ((resettype == RESET_HARD) || (resettype == RESET_SOFT)) {
                response = TRUE;
            }

            break;
        default :
            YX_UDS_NegativeResponse(SID_11, NRC_12);
            break;
    }

    if (response) {
        UDS_SID11_Response(resettype);
    }
}

/*******************************************************************
** 函数名: UDS_SID27_Response
** 函数描述: 安全访问肯定响应
** 参数: [in] accesstype: 访问类型
         [in] seed: 种子
         [in] len: 种子长度(正常为4字节)
** 返回: NULL
********************************************************************/
static void UDS_SID27_Response(INT8U accesstype, INT8U *seed, INT8U len)
{
    INT8U memptr[8];
    STREAM_T wstrm;

    bal_InitStrm(&wstrm, memptr, 8);

    bal_WriteBYTE_Strm(&wstrm, (SID_27 + RESP_ADD));                             /* 肯定响应标识 */
    bal_WriteBYTE_Strm(&wstrm, accesstype);                                      /* 访问类型 */
    if (len == 4) {
        bal_WriteDATA_Strm(&wstrm, seed, len);
    }

    #if DEBUG_UDS > 0
    debug_printf("UDS_SID27_Response->accesstype(%x):", accesstype);
    if (len == 4) {
        printf_hex(seed, len);
    }
    debug_printf("\r\n");
    #endif

    YX_UDS_CanSendSingle(bal_GetStrmStartPtr(&wstrm),bal_GetStrmLen(&wstrm));
}

/*****************************************************************************
**  函数名:  UDS_SID27_Level
**  函数描述: 安全等级处理
**  参数:    [in] accesstype :
**           [in] data       :
**           [in] len        :
**  返回:    无
*****************************************************************************/
static void UDS_SID27_Level(INT8U accesstype, INT8U* data, INT8U len)
{
   
    INT8U status;
    INT8U accesskey[4];

    switch (accesstype) {
        case REQSEED_1:
        case REQSEED_5: 
        //case (REQSEED_1 + SUPPRESS_RESP):
        //case (REQSEED_5 + SUPPRESS_RESP):
            if (accesstype == REQSEED_1) {
                if (s_uds_module.session_type == SESSION_DEFAULT) {
                    YX_UDS_NegativeResponse(SID_27, NRC_7E);
                    return;
                }
                status = 0x02;    /* 表示level1 */
            } else {
                if (s_uds_module.session_type != SESSION_PROGRAM) {
                    YX_UDS_NegativeResponse(SID_27, NRC_7E);
                    return;
                }
                status = 0x03;    /* 表示level2 */
            }

            /* 长度校验 */
            if(len != 1){
                YX_UDS_NegativeResponse(SID_27, NRC_13);
                break;
            }
            #if DEBUG_UDS > 0
            debug_printf("<(1)27服务，wait:%x,status:%x,fault_cnt:%x,access_wait_cnt:%x>\r\n",s_uds_module.en_access_wait, s_uds_module.access_status, 
                                                                                                s_uds_module.access_fault_cnt, s_uds_module.access_wait_cnt);
            #endif
            /* 延时阶段 */
            if (s_uds_module.en_access_wait) {
                YX_UDS_NegativeResponse(SID_27, NRC_37);
                break;
            }
            if (s_uds_module.access_status == 0x01) {
                /* 连续请求种子，错误计数加1，大于等于4次后，应答NRC_36 */
                s_uds_module.access_fault_cnt++;

                if (s_uds_module.access_fault_cnt > 3) {
                    s_uds_module.en_access_wait = TRUE;
                    s_uds_module.access_wait_cnt = 0;
                    s_uds_module.access_fault_cnt = 3;    /* 防止溢出 */
                    #if UDS_27_NRC_78 > 0
                    YX_UDS_NegativeResponse(SID_27, NRC_78);
                    Update_UDS_Fault_Cnt();
                    #endif
                    if (!OS_TmrIsRun(s_udstmr)) {
                        OS_StartTmr(s_udstmr, UDS_PERIOD);
                    }
                    YX_UDS_NegativeResponse(SID_27, NRC_36);
                } else {
                    #if UDS_27_NRC_78 > 0
                    YX_UDS_NegativeResponse(SID_27, NRC_78);
                    Update_UDS_Fault_Cnt();     
                    #endif
                    UDS_SID27_Response(accesstype, s_uds_module.access_seed, 4);
                }
                
                #if DEBUG_UDS > 0
                debug_printf("<(2)27服务，wait:%x,status:%x,fault_cnt:%x,access_wait_cnt:%x>\r\n",s_uds_module.en_access_wait, s_uds_module.access_status, 
                                                    s_uds_module.access_fault_cnt, s_uds_module.access_wait_cnt);
                #endif
            } else if (s_uds_module.access_status == status) {
                YX_MEMSET(s_uds_module.access_seed, 0x00, sizeof(s_uds_module.access_seed));
                UDS_SID27_Response(accesstype, s_uds_module.access_seed, 4);
                #if DEBUG_UDS > 0
                debug_printf("<(3)27服务，wait:%x,status:%x,fault_cnt:%x,access_wait_cnt:%x>\r\n",s_uds_module.en_access_wait, s_uds_module.access_status, 
                                                    s_uds_module.access_fault_cnt, s_uds_module.access_wait_cnt);
                #endif                
            } else {
                /* 随机数计算可能比较耗时，添加78否定应答 */
                //YX_UDS_NegativeResponse(SID_27, NRC_78);
                s_uds_module.access_status = 0x01;
                YX_MEMSET(s_uds_module.access_seed, 0x00, sizeof(s_uds_module.access_seed));
                for (;;) {
                    #if DEBUG_UDS > 1
                    s_uds_module.access_seed[0] = 0x2b;
                    s_uds_module.access_seed[1] = 0x3a;
                    s_uds_module.access_seed[2] = 0x7a;
                    s_uds_module.access_seed[3] = 0x44;
                    #else
                    Get_Rand(sizeof(s_uds_module.access_seed), s_uds_module.access_seed);
                    #endif			
                    #if DEBUG_UDS > 0
                    debug_printf("access_seed: ");
                    printf_hex(s_uds_module.access_seed, 4);
                    debug_printf("\r\n");
                    #endif
                    if (((s_uds_module.access_seed[0] != 0x00) || (s_uds_module.access_seed[1] != 0x00) || (s_uds_module.access_seed[2] != 0x00) || (s_uds_module.access_seed[3] != 0x00))
                        && ((s_uds_module.access_seed[0] != 0xFF) || (s_uds_module.access_seed[1] != 0xFF) || (s_uds_module.access_seed[2] != 0xFF) || (s_uds_module.access_seed[3] != 0xFF))) {
                        break;
                    }
                }

                UDS_SID27_Response(accesstype, s_uds_module.access_seed, 4);
                
                /* 根据seed算出key值 */
                Get_SecurityKey(accesstype, s_uds_module.access_seed, s_uds_module.access_key);
                
                #if DEBUG_UDS > 1 
                s_uds_module.access_key[0] = 0x44;
                s_uds_module.access_key[1] = 0x33;
                s_uds_module.access_key[2] = 0x22;
                s_uds_module.access_key[3] = 0x11;                
                #endif 
                
                #if DEBUG_UDS > 0
                debug_printf("UDS_SID27_SecurityAccess:");
                debug_printf("\r\n seed:");
                printf_hex(s_uds_module.access_seed, 4);
                debug_printf("\r\n key:");
                printf_hex(s_uds_module.access_key, 4);                
                debug_printf("\r\n");
                #endif
                #if DEBUG_UDS > 0
                debug_printf("<(4)27服务，wait:%x,status:%x,fault_cnt:%x,access_wait_cnt:%x>\r\n",s_uds_module.en_access_wait, s_uds_module.access_status, 
                                                    s_uds_module.access_fault_cnt, s_uds_module.access_wait_cnt);
                #endif                
            }
            break;
        case (REQSEED_1 + 1):
        case (REQSEED_5 + 1):    
        //case (REQSEED_1 + SUPPRESS_RESP + 1):
        //case (REQSEED_5 + SUPPRESS_RESP + 1):
            if (accesstype == (REQSEED_1 + 1)) {
                if (s_uds_module.session_type == SESSION_DEFAULT) {
                    YX_UDS_NegativeResponse(SID_27, NRC_7E);
                    return;
                }
                status = 0x02;    /* 表示level1 */
            } else {
                if (s_uds_module.session_type != SESSION_PROGRAM) {
                    YX_UDS_NegativeResponse(SID_27, NRC_7E);
                    return;
                }
                status = 0x03;    /* 表示level2 */
            }

            if (len != 5) {
                YX_UDS_NegativeResponse(SID_27, NRC_13);
                break;
            }
            
            if (s_uds_module.en_access_wait) {
                YX_UDS_NegativeResponse(SID_27, NRC_24);
                break;
            }
            
            if (s_uds_module.access_status == 0x01) {
                 memcpy(accesskey,&data[1],4);
                
                if (STR_EQUAL == bal_ACmpString(CASESENSITIVE, s_uds_module.access_key, accesskey, 4, 4)) {//比较密钥
                    
                    /* 成功后，错误计数清0 */
                    s_uds_module.access_fault_cnt = 0;
                    #if UDS_27_NRC_78 > 0
                    YX_UDS_NegativeResponse(SID_27, NRC_78);
                    Update_UDS_Fault_Cnt(); 
                    #endif
                    
                    s_uds_module.access_status = status; /* 解锁级别 */
                    /* 成功后,停止延迟机制 */
                    s_uds_module.access_wait_cnt = 0;
                    s_uds_module.en_access_wait = FALSE;
					UDS_SID27_Response(accesstype, NULL, 0);
                } else {
                    s_uds_module.access_fault_cnt++;
                    /* 无效密钥，回到锁定状态 */
                    s_uds_module.access_status = 0;
                    /* 无效key为3次后，应答NCR_36 */
                    if (s_uds_module.access_fault_cnt >= 3) {
                        /* 启动延迟机制 */
                        s_uds_module.en_access_wait = TRUE;
                        s_uds_module.access_wait_cnt = 0;
                        s_uds_module.access_fault_cnt = 3;    /* 防止溢出 */
                        #if UDS_27_NRC_78 > 0
                        YX_UDS_NegativeResponse(SID_27, NRC_78);
                        Update_UDS_Fault_Cnt();
                        #endif
                        if (!OS_TmrIsRun(s_udstmr)) {
                            OS_StartTmr(s_udstmr, UDS_PERIOD);
                        }
                        YX_UDS_NegativeResponse(SID_27, NRC_36);
                    } else {
                        #if UDS_27_NRC_78 > 0
                        YX_UDS_NegativeResponse(SID_27, NRC_78);
                        Update_UDS_Fault_Cnt();; 	
                        #endif
                        YX_UDS_NegativeResponse(SID_27, NRC_35);
                    }
                }
                #if DEBUG_UDS > 0
                debug_printf("<(5)27服务，wait:%x,status:%x,fault_cnt:%x,access_wait_cnt:%x>\r\n",s_uds_module.en_access_wait, s_uds_module.access_status, 
                                                    s_uds_module.access_fault_cnt, s_uds_module.access_wait_cnt);
                #endif
            } else {
                YX_UDS_NegativeResponse(SID_27, NRC_24);
                break;
            }
            break;
        default:
            break;
    }    
}

/*******************************************************************
** 函数名: UDS_SID27_SecurityAccess
** 函数描述: 安全访问
** 参数: [in] data: 包含sub的数据
         [in] len: 数据长度
** 返回: NULL
********************************************************************/
static void UDS_SID27_SecurityAccess(INT8U *data, INT8U len)
{
    if ((data[0] == REQSEED_1) || (data[0] == (REQSEED_1 + 1))/*  || (data[0] == REQSEED_5) || (data[0] == (REQSEED_5 + 1)) */) { 
        if (s_uds_module.session_type == SESSION_DEFAULT) {
            YX_UDS_NegativeResponse(SID_27, NRC_7F);
            return;
        }

        UDS_SID27_Level(data[0], data, len);
    } else {
        YX_UDS_NegativeResponse(SID_27, NRC_12);
    }
}

/*******************************************************************
** 函数名: UDS_SID28_Response
** 函数描述: 通信控制肯定响应
** 参数: [in] ctltype: 控制类型 见CMNCT_CTLTYPE_E
** 返回: NULL
********************************************************************/
static void UDS_SID28_Response(INT8U ctltype)
{
    INT8U memptr[8];
    STREAM_T wstrm;
    bal_InitStrm(&wstrm, memptr, 8);

    bal_WriteBYTE_Strm(&wstrm, (SID_28 + RESP_ADD));                             /* 肯定响应标识 */
    bal_WriteBYTE_Strm(&wstrm, ctltype);                                         /* 控制类型 */

    #if DEBUG_UDS > 0
    debug_printf("UDS_SID28_Response->ctltype(%x)\r\n", ctltype);
    #endif
	
    YX_UDS_CanSendSingle(bal_GetStrmStartPtr(&wstrm),bal_GetStrmLen(&wstrm));
}

/*******************************************************************
** 函数名: UDS_SID28_CommunicationControl
** 函数描述: 通信控制
** 参数: [in] ctltype: 控制类型 见CMNCT_CTLTYPE_E
         [in] cmncttype: 通信类型
** 返回: NULL
********************************************************************/
static void UDS_SID28_CommunicationControl(INT8U ctltype, INT8U cmncttype)
{
    if (s_uds_module.session_type != SESSION_EXTENDED) {
        YX_UDS_NegativeResponse(SID_28, NRC_7F);
        return;
    }
    switch (ctltype) {
        case UDS_COMCTR_ENABLE_RXTX:
        case (UDS_COMCTR_ENABLE_RXTX + SUPPRESS_RESP):
        case UDS_COMCTR_DISABLE_ALL:
        case (UDS_COMCTR_DISABLE_ALL + SUPPRESS_RESP):
            if ((cmncttype != 0x01) && (cmncttype != 0x03)) {      /* 不支持单独对网络管理报文的控制 */
                YX_UDS_NegativeResponse(SID_28, NRC_31);
                return;
            }

            Req_CmnctCtl((ctltype & (~SUPPRESS_RESP)), cmncttype);
            if ((ctltype == UDS_COMCTR_ENABLE_RXTX) || (ctltype == UDS_COMCTR_DISABLE_ALL)) {
                UDS_SID28_Response(ctltype);
            }
            break;
        default :
            YX_UDS_NegativeResponse(SID_28, NRC_12);
            break;
    }
}

/*******************************************************************
** 函数名: UDS_SID3E_Response
** 函数描述: 诊断设备在线肯定响应
** 参数: [in] sub: 子功能
** 返回: NULL
********************************************************************/
static void UDS_SID3E_Response(INT8U sub)
{
    INT8U sdata[8];
    STREAM_T wstrm;

    bal_InitStrm(&wstrm, sdata, 8);

    bal_WriteBYTE_Strm(&wstrm, (SID_3E + RESP_ADD));                             /* 肯定响应标识 */
    bal_WriteBYTE_Strm(&wstrm, sub);                                             /* 子功能 */

    #if DEBUG_UDS > 0
    debug_printf("UDS_SID3E_Response->sub(%x)\r\n", sub);
    #endif

    YX_UDS_CanSendSingle(bal_GetStrmStartPtr(&wstrm), bal_GetStrmLen(&wstrm));
}

/*******************************************************************
** 函数名: UDS_SID3E_TesterPresent
** 函数描述: 诊断设备在线
** 参数: [in] sub: 子功能
** 返回: NULL
********************************************************************/
static void UDS_SID3E_TesterPresent(INT8U sub)
{
    switch (sub) {
        case 0x00:               /* Zero子功能 */
        case (0x00 + SUPPRESS_RESP):
            s_uds_module.session_cnt = 0;
            if (sub == 0x00) {
                UDS_SID3E_Response(sub);
            }
            break;
        default :
            YX_UDS_NegativeResponse(SID_3E, NRC_12);
            break;
    }
}

/*******************************************************************
** 函数名: UDS_SID85_Response
** 函数描述: 控制DTC设置肯定响应
** 参数: [in] settype: 设置类型(开/关)
** 返回: NULL
********************************************************************/
static void UDS_SID85_Response(INT8U settype)
{
    INT8U    sdata[8];
    STREAM_T wstrm; 

    bal_InitStrm(&wstrm, sdata, 8);

    bal_WriteBYTE_Strm(&wstrm, (SID_85 + RESP_ADD));                             /* 肯定响应标识 */
    bal_WriteBYTE_Strm(&wstrm, settype);                                             /* 子功能 */

    #if DEBUG_UDS > 0
    debug_printf("UDS_SID85_Response->settype(%x)\r\n", settype);
    #endif

    YX_UDS_CanSendSingle(bal_GetStrmStartPtr(&wstrm), bal_GetStrmLen(&wstrm)); 
}

/*******************************************************************
** 函数名: UDS_SID3E_ControlDTCSetting
** 函数描述: 控制DTC设置
** 参数: [in] settype: 设置类型(开/关)
** 返回: NULL
********************************************************************/
static void UDS_SID85_ControlDTCSetting(INT8U settype)
{
    if (s_uds_module.session_type != SESSION_EXTENDED) {
        YX_UDS_NegativeResponse(SID_85, NRC_7F);
        return;
    }

    switch (settype) {
        case 0x01:                      /* 打开DTC设置 */
        case (0x01 + SUPPRESS_RESP):
			s_uds_module.en_setdtc = TRUE;
            if (settype == 0x01) {
                UDS_SID85_Response(settype);
            }
            break;
        case 0x02:                      /* 关闭DTC设置 */
        case (0x02 + SUPPRESS_RESP):
			s_uds_module.en_setdtc = FALSE;
            if (settype == 0x02) {
                UDS_SID85_Response(settype);
            }
            break;
        default :
            YX_UDS_NegativeResponse(SID_85, NRC_12);
            break;
    }
}

/*******************************************************************
** 函数名: UDSTmrProc
** 函数描述: 统一诊断服务定时器
** 参数: [in] pdata: 定时器特征值
** 返回: 无
********************************************************************/
static void UDSTmrProc(void *pdata)
{
    OS_StartTmr(s_udstmr, UDS_PERIOD);
    pdata = pdata;
    /* 通过累计模拟随机数 */
    s_rand_cnt++;

    if (s_uds_module.en_session) {
        if (++s_uds_module.session_cnt >= NOT_SESSION_DEFAULT_TIME) {
            s_uds_module.session_cnt = 0;
            s_uds_module.en_session = FALSE;
            s_uds_module.session_type = SESSION_DEFAULT;
            UDS_Reset_ServiceStatus();
        }
    }

    if (s_uds_module.en_access_wait) {
        if (++s_uds_module.access_wait_cnt >= ACCESS_WAIT_TIME) {
            s_uds_module.access_wait_cnt = 0;
            s_uds_module.en_access_wait = FALSE;
            s_uds_module.access_status = 0x00;
            if (s_uds_module.access_fault_cnt != 0) {
                s_uds_module.access_fault_cnt--;
                Update_UDS_Fault_Cnt();
            }
        }
    }

    #if UDS_27_NRC_78 == 0
    // 超时保存
    if (s_uds_timeout) {
        s_uds_timeout--;
        if (!s_uds_timeout) {
            Update_UDS_Fault_Cnt();
        }
    }
    #endif
    
    if ((s_uds_module.reset_type == RESET_HARD) || (s_uds_module.reset_type == RESET_SOFT)) {
        if (++s_uds_module.reset_cnt >= RESET_WAIT_TIME) {
            s_uds_module.reset_cnt = 0;
            if (s_uds_module.reset_type == RESET_HARD) {
                /* MCU复位 */
                PORT_ResetCPU();
            } else {
                /* 应用软件复位，MCU本身没复位 */
                 /* 软件复位:会话模式回到默认模式和安全范围等级回到lock */
                s_uds_module.session_cnt = 0;
                s_uds_module.en_session = FALSE;
                s_uds_module.session_type = SESSION_DEFAULT;
                UDS_Reset_ServiceStatus();       
				/* 复位后，如果安全访问错误计数大于等于3，开启10秒超时 */
                if (s_uds_module.access_fault_cnt >= 3) {
                    /* 安全访问等待计数使能 */
                    s_uds_module.en_access_wait = TRUE;
                    s_uds_module.access_wait_cnt = 0;
                }               
            }
            s_uds_module.reset_type = 0;
        }
    }
    
}

/*******************************************************************
** 函数名:      UDS_CanDataHdl
** 函数描述:    UDS数据处理
** 参数:        [in] id  : can id 
                [in] data: 源数据
                [in] len : 源数据长度
** 返回:        是否为uds数据
********************************************************************/
static BOOLEAN UDS_CanDataHdl(INT32U id, INT8U* data, INT8U len) 
{
    //INT8U i;
    //CAN_PACKET_SND_MANAGER_T* sendpack;

    id = id;
    
    /* uds帧长度必须为8 */
    if (len != 8) {
        return TRUE; 
    }
    
    /* uds PCItype服务最高4位不超过0x30 */
    if ((data[0] & 0xF0) > 0x30) {
        #if DEBUG_UDS > 0
        debug_printf("< uds错误PCI >\r\n");
        #endif  
        return TRUE;
    }

    /* 错误单帧处理 */
    if ((data[0] & 0xF0) == 0x00) {
        /* 单帧有效长度为1~7 */
        if ((data[0] == 0) || (data[0] > 7)) {
            #if DEBUG_UDS > 0
            debug_printf("< uds错误单帧长度 >\r\n");
            #endif
            return TRUE;
        }
    }    

    /* 发送多帧数据时，如果接收到其他帧(SF、FF、FC)，不做处理 */
    /*sendpack = YX_CAN_GetSndManager();
    for(i = 0; i < MAX_PACKE_SND_NUM; i++) {
        if(!sendpack[i].isuse) {
            continue;
        }
        if ((sendpack[i].canid == UDS_PHSCL_RESPID) && (sendpack[i].sendcontinue)) {
            #if DEBUG_UDS > 0
            debug_printf("< (1)正在发送 >\r\n");
            #endif              
            return TRUE;
        }
    }*/
    
    /* 单帧处理 */
    if (YX_UDS_Recv(FALSE, id, data, (INT16U)len)) {
        #if DEBUG_UDS > 0
        debug_printf("< 单帧处理data[0]:0x%x >\r\n",data[0]);
        #endif         
        return TRUE;
    }

    return FALSE;
}

/*****************************************************************************
**  函数名:  UDS_SingleFrameHdl
**  函数描述: uds单帧处理
**  参数:    [in] com     : 通道号
**           [in] candata : 报文
**  返回:    0:执行失败 1:执行成功
*****************************************************************************/
BOOLEAN UDS_SingleFrameHdl(INT8U* data, INT8U datalen)
{
    CAN_DATA_HANDLE_T* CAN_msg;
		INT32U id;

		CAN_msg = (CAN_DATA_HANDLE_T*)data;

		id = bal_chartolong(CAN_msg->id);
    // did报文数据
    YX_UDS_DID_UpdataCanData(id, CAN_msg->databuf, CAN_msg->len);
    // DTC节点丢失记录
    YX_DTC_NodeIdCheck(CAN_msg->channel, id);

    if (CAN_msg->channel != UDS_CAN_CH) {
        return FALSE;
    }  

    if ((id != UDS_PHSCL_REQID) && (id != FUNC_REQID)) {
        return FALSE;
    }

    return UDS_CanDataHdl(id, CAN_msg->databuf, CAN_msg->len);
}
/*******************************************************************
** 函数名:      YX_UDS_SingleFrameRecv
** 函数描述:    UDS单帧接收处理
** 参数:        [in] data:              源数据
                [in] len:               源数据长度
                [in] type:              所属系统类型
** 返回:        NULL
********************************************************************/
BOOLEAN YX_UDS_MultFrameRecv(INT32U id, INT8U* data, INT16U len) 
{
    // 只有物理寻址有多帧数据
    if (id != UDS_PHSCL_REQID) return FALSE;

    return YX_UDS_Recv(TRUE, id, data, len);
}
/*****************************************************************************
**  函数名:  Uds_Fault_Cnt_Init
**  函数描述: 27服务错误计数
**  参数:    无
**  返回:    无
*****************************************************************************/
static void Uds_27Server_Fault_Cnt_Init(void)
{
    YX_MEMSET((INT8U *)&s_uds_fault_cnt, 0x00, sizeof(UDS_ERR_CNT_T));
    
    if (!bal_pp_ReadParaByID(UDS_ERRCNT_PARA_, (INT8U *)&s_uds_fault_cnt, sizeof(UDS_ERR_CNT_T))) {
        #if DEBUG_UDS > 0
        debug_printf("<Uds_27Server_Fault_Cnt_Init():%d>\r\n",s_uds_module.access_fault_cnt);
        #endif
    } else {
        /* 读取成功 */
        s_uds_module.access_fault_cnt = s_uds_fault_cnt.fault_cnt;
    }

    if (s_uds_module.access_fault_cnt != 0) {
        /* 安全访问等待计数使能 */
        s_uds_module.en_access_wait = TRUE;
        s_uds_module.access_wait_cnt = 0;
    }
    
    s_uds_module.access_fault_cnt_pre = s_uds_module.access_fault_cnt;
}


/*******************************************************************
** 函数名: YX_UDS_CanSendSingle
** 函数描述: 单帧数据
** 参数: [in] data    : 内容
         [in] datalen : 内容长度
** 返回: TRUE:发送成功, FALSE:发送失败
********************************************************************/
BOOLEAN YX_UDS_CanSendSingle(INT8U* data, INT8U datalen)
{
    CAN_DATA_SEND_T sdata;

    /* 填充 */
    memset(sdata.Data, UNUSE_BYTE_FULL, 8);
    
    sdata.can_DLC = 8;
    sdata.can_id  = UDS_PHSCL_RESPID;
		sdata.period  = 0xffff;

    if (sdata.can_id <= 0x7ff) {
        sdata.can_IDE   = FMAT_STD;
    } else {
        sdata.can_IDE   = FMAT_EXT;
    }
    sdata.channel = UDS_CAN_CH;
		sdata.Data[0] = datalen;
    memcpy(&sdata.Data[1], data, datalen);
    
    return PORT_CanSend(&sdata);
}

/*******************************************************************
** 函数名:     YX_UDS_SendMulCanData
** 函数描述:   发送多帧数据
** 参数:       [in] data          需发送的数据
               [in] datalen       发送数据长度
** 返回:       NULL
********************************************************************/
void YX_UDS_SendMulCanData(INT8U *data, INT16U datalen)
{
    YX_MMI_UDS_CanSendMul(UDS_CAN_CH, data, datalen);
}
/*******************************************************************
** 函数名: YX_UDS_NegativeResponse
** 函数描述: 服务否定响应
** 参数: [in] sid: 服务ID
         [in] nrcode: 否定响应码
** 返回: NULL
********************************************************************/
void YX_UDS_NegativeResponse(INT8U sid, INT8U nrcode)
{
    INT8U memptr[8];
    STREAM_T wstrm;
  
    if ((s_reqid == FUNC_REQID) && ((nrcode == NRC_11) || (nrcode == NRC_12) || (nrcode == NRC_31) || (nrcode == NRC_7E) || (nrcode == NRC_7F))) {
        return; /* 功能寻址，对于NRC为0x11/0x12/0x31/7E/7F的5种情况，ECU不响应 */
    }

    bal_InitStrm(&wstrm, memptr, 8);

    bal_WriteBYTE_Strm(&wstrm, SID_7F);                                          /* 通过标识符写数据的否定响应标识*/
    bal_WriteBYTE_Strm(&wstrm, sid);                                             /* 服务标识*/
    bal_WriteBYTE_Strm(&wstrm, nrcode);                                          /* 否定响应码*/

    #if DEBUG_UDS > 0
    debug_printf("YX_UDS_NegativeResponse->can id(0x%x),sid(%x),nrcode(%x)\r\n",s_reqid, sid, nrcode);
    #endif
    YX_UDS_CanSendSingle(bal_GetStrmStartPtr(&wstrm),bal_GetStrmLen(&wstrm));
}

/*******************************************************************
** 函数名: YX_UDS_Recv
** 描述: 接收到诊断请求
** 参数: [in] is_mul_frame : TRUE:多帧， FALSE:单帧
         [in] reqid: 请求ID
         [in] data: 数据内容
         [in] datalen: 数据长度
** 返回: 处理结果(ture/false)
********************************************************************/
BOOLEAN YX_UDS_Recv(BOOLEAN is_mul_frame, INT32U reqid, INT8U *data, INT16U datalen)
{
    INT8U* p_data;
    INT8U  sid;
    //INT16U did;
    INT16U  len;
    //BOOLEAN acc_sta;
    BOOLEAN nrc_22_ack;
	
    #if DEBUG_UDS > 0
    debug_printf("<收到UDS诊断数据(CANID:%x):", reqid);
    printf_hex(data, datalen);
    debug_printf(">\r\n");
    #endif

    if (((reqid != FUNC_REQID) && (reqid != UDS_PHSCL_REQID)) || (data == NULL)) {
        #if DEBUG_UDS > 0
        debug_printf("<YX_UDS_Recv->ERR>\r\n");
        #endif
        return false;
    }

    /* 单帧的data[0]高位为0 */
    if ((!is_mul_frame) && (datalen <= 8)) {
        if (((data[0] & 0xf0) == 0x10) || ((data[0] & 0xf0) == 0x20) || ((data[0] & 0xf0) == 0x30)) {
            return false;
        } 
    }

    if (s_uds_module.en_session) {
        s_uds_module.session_cnt = 0; /* 在扩展模式，在S3server超时前有诊断请求，ECU不应该跳转到默认会话 */
    }

    s_reqid = reqid;
    
    if (is_mul_frame) {
        len = datalen;
        /* 多帧data[0]为sid */
        sid = data[0];
        /* 实际内容 */
        p_data = &data[1];
    } else {
        /* 单帧data[0]为数据长度,data[1]为sid */
        len = data[0];
        sid = data[1];
        /* 实际内容 */
        p_data = &data[2];
    }

    nrc_22_ack = FALSE;
    
    UdsStartTimeout();    

    #if 0
    /* MCU与主机未通讯上，应答NRC_22 */
    if (!YX_COM_IsLink()) {
        nrc_22_ack = TRUE;
    }
    #endif

	if (reqid == UDS_PHSCL_REQID) {
        switch (sid) {
            case SID_10:
                if (nrc_22_ack) {
                    YX_UDS_NegativeResponse(sid, NRC_22);
                    break;
                }	                
                if (len != 2) {
                    YX_UDS_NegativeResponse(sid, NRC_13);
                    break;
                }
                UDS_SID10_DiagnosticSessionControl(p_data[0]);
                break;
            case SID_11:
                if (nrc_22_ack) {
                    YX_UDS_NegativeResponse(sid, NRC_22);
                    break;
                }				
                if (len != 2) {
                    YX_UDS_NegativeResponse(sid, NRC_13);
                    break;
                }
                UDS_SID11_EcuReset(p_data[0]);
                break;
            case SID_14:
                if (nrc_22_ack) {
                    YX_UDS_NegativeResponse(sid, NRC_22);
                    break;
                }				
                if (len != 4) {
                    YX_UDS_NegativeResponse(sid, NRC_13);
                    break;
                }
                YX_DTC_SID14_ClearDiagnosticInformation(p_data, (len - 1));
                break;
            case SID_19:
                if (nrc_22_ack) {
                    YX_UDS_NegativeResponse(sid, NRC_22);
                    break;
                }	                
                if (len < 2) {
                    YX_UDS_NegativeResponse(sid, NRC_13);
                    break;
                }
                YX_DTC_SID19_ReadDTCInformation(p_data, (len - 1));
                break;
            case SID_22:
                if (nrc_22_ack) {
                    YX_UDS_NegativeResponse(sid, NRC_22);
                    break;
                }				
                if ((len < 3) || (len%2 != 1)) {
                    YX_UDS_NegativeResponse(sid, NRC_13);
                    break;
                }
                //did = (INT16U)YX_BigEndModeToHWord(p_data);
                YX_UDS_DID_SID22_ReadDataByIdentifier(p_data, len - 1);
                break;
            case SID_27:
                if (nrc_22_ack) {
                    YX_UDS_NegativeResponse(sid, NRC_22);
                    break;
                }				
                if (len < 2) {
                    YX_UDS_NegativeResponse(sid, NRC_13);
                    break;
                }
                UDS_SID27_SecurityAccess(p_data, (len - 1));
                break;
            case SID_28:
                if (nrc_22_ack) {
                    YX_UDS_NegativeResponse(sid, NRC_22);
                    break;
                }				
                if (len != 3) {
                    YX_UDS_NegativeResponse(sid, NRC_13);
                    break;
                }
                UDS_SID28_CommunicationControl(p_data[0], p_data[1]);
                break;
            case SID_2E:
                if (nrc_22_ack) {
                    YX_UDS_NegativeResponse(sid, NRC_22);
                    break;
                }				
                if (len < 3) {
                    YX_UDS_NegativeResponse(sid, NRC_13);
                    break;
                }
                //did = (INT16U)YX_BigEndModeToHWord(p_data);
                //YX_UDS_DID_SID2E_WriteDataByIdentifier(did, &p_data[2], (len - 3));
                YX_UDS_DID_SID2E_WriteDataByIdentifier(p_data, (len - 1));
                break;
            case SID_3E:
                if (len != 2) {
                    YX_UDS_NegativeResponse(sid, NRC_13);
                    break;
                }
                UDS_SID3E_TesterPresent(p_data[0]);
                break;
            case SID_85:
                if (nrc_22_ack) {
                    YX_UDS_NegativeResponse(sid, NRC_22);
                    break;
                }				
                if (len != 2) {
                    YX_UDS_NegativeResponse(sid, NRC_13);
                    break;
                }
                UDS_SID85_ControlDTCSetting(p_data[0]);
                break;
                
          #if EN_UDS_TRANS > 0
            case SID_31:
                YX_UDS_RoutineControl(sid, p_data, len - 1);
                break;
                
            case SID_34:
                YX_UDS_ReqDownload(sid, p_data, len - 1);
                break;   
                
            case SID_36:
                YX_UDS_TransData(sid, p_data, len - 1);
                break;
                
            case SID_37:
                YX_UDS_ReqTransExit(sid);
                break;                  
          #endif
                
            default:
                YX_UDS_NegativeResponse(sid, NRC_11);
                break;
            }
    } else if (reqid == FUNC_REQID) {
        switch (sid) {
            case SID_10:
                if (nrc_22_ack) {
                    YX_UDS_NegativeResponse(sid, NRC_22);
                    break;
                }	                
                if (len != 2) {
                    YX_UDS_NegativeResponse(sid, NRC_13);
                    break;
                }
                UDS_SID10_DiagnosticSessionControl(p_data[0]);
                break;
            case SID_11:
                if (nrc_22_ack) {
                    YX_UDS_NegativeResponse(sid, NRC_22);
                    break;
                }				
                if (len != 2) {
                    YX_UDS_NegativeResponse(sid, NRC_13);
                    break;
                }
                UDS_SID11_EcuReset(p_data[0]);
                break;
            case SID_14:
                if (nrc_22_ack) {
                    YX_UDS_NegativeResponse(sid, NRC_22);
                    break;
                }				
                if (len != 4) {
                    YX_UDS_NegativeResponse(sid, NRC_13);
                    break;
                }
                YX_DTC_SID14_ClearDiagnosticInformation(p_data, (len - 1));
                break;
            case SID_19:
                if (nrc_22_ack) {
                    YX_UDS_NegativeResponse(sid, NRC_22);
                    break;
                }	                
                if (len < 2) {
                    YX_UDS_NegativeResponse(sid, NRC_13);
                    break;
                }
                YX_DTC_SID19_ReadDTCInformation(p_data, (len - 1));
                break;
            case SID_22:
                if (nrc_22_ack) {
                    YX_UDS_NegativeResponse(sid, NRC_22);
                    break;
                }				
                if ((len < 3) || (len%2 != 1)) {
                    YX_UDS_NegativeResponse(sid, NRC_13);
                    break;
                }
                //did = (INT16U)YX_BigEndModeToHWord(p_data);
                YX_UDS_DID_SID22_ReadDataByIdentifier(p_data, len - 1);
                break;
            case SID_28:
                if (nrc_22_ack) {
                    YX_UDS_NegativeResponse(sid, NRC_22);
                    break;
                }				
                if (len != 3) {
                    YX_UDS_NegativeResponse(sid, NRC_13);
                    break;
                }
                UDS_SID28_CommunicationControl(p_data[0], p_data[1]);
                break;
            case SID_3E:
                if (len != 2) {
                    YX_UDS_NegativeResponse(sid, NRC_13);
                    break;
                }
                UDS_SID3E_TesterPresent(p_data[0]);
                break;
            case SID_85:
                if (nrc_22_ack) {
                    YX_UDS_NegativeResponse(sid, NRC_22);
                    break;
                }				
                if (len != 2) {
                    YX_UDS_NegativeResponse(sid, NRC_13);
                    break;
                }
                UDS_SID85_ControlDTCSetting(p_data[0]);
                break;
            default:
                YX_UDS_NegativeResponse(sid, NRC_11);
                break;
        }
    }

    return true;
}

/*******************************************************************
** 函数名: YX_UDS_GetSession
** 函数描述: 获取当前会话类型
** 参数: NULL
** 返回: 当前会话类型:@SESSION_TYPE_E
********************************************************************/
SESSION_TYPE_E YX_UDS_GetSession(void)
{
    return (SESSION_TYPE_E)s_uds_module.session_type;
}

/*******************************************************************
** 函数名: YX_UDS_GetSession
** 函数描述: 获取当前安全等级
** 参数: NULL
** 返回: 当前安全访问等级
********************************************************************/
INT8U YX_UDS_GetSecurity(void)
{
    return s_uds_module.access_status;
}

/*******************************************************************
** 函数名: YX_UDS_GetDtcEnableSta
** 函数描述: 获取UDS故障码使能状态
** 参数: NULL
** 返回: UDS故障码使能状态
********************************************************************/
BOOLEAN YX_UDS_GetDtcEnableSta(void)
{
    return s_uds_module.en_setdtc;
}

/*******************************************************************
** 函数名: YX_UDS_GetCanRecvEnableSta
** 函数描述: can接收使能
** 参数: 无
** 返回: 无
********************************************************************/
BOOLEAN YX_UDS_GetCanRecvEnableSta(void)
{
    return s_can_recv_sw;
}

/*******************************************************************
** 函数名: YX_UDS_Init
** 函数描述: 统一诊断服务初始化
** 参数: 无
** 返回: 无
********************************************************************/
void YX_UDS_Init(void)
{
    //INT8U i;
	//INT32U uds_can_id_list[2]    = {UDS_PHSCL_REQID,  UDS_PHSCL_RESPID};
    //INT32U uds_can_fc_id_list[2] = {UDS_PHSCL_RESPID, UDS_PHSCL_REQID};
    
    #if DEBUG_UDS > 0
    debug_printf("<<YX_UDS_Init>>\r\n");
    #endif

    s_rand_cnt = 0;
    
    YX_MEMSET((INT8U *)&s_uds_module, 0x00, sizeof(UDS_MODULE_STATUS_T));
    
    s_uds_module.session_type = SESSION_DEFAULT;
    s_uds_module.en_setdtc = TRUE;
	
    YX_DTC_Init();
    YX_UDS_DID_Init();
    Uds_27Server_Fault_Cnt_Init();
	  #if EN_UDS_TRANS > 0
    YX_UDS_TransferInit();
    #endif
    s_udstmr = OS_InstallTmr(TSK_ID_OPT, (void*)0, UDSTmrProc);
    OS_StartTmr(s_udstmr, UDS_PERIOD);    
    
    //YX_CAN_UDS_ONFF(UDS_CAN_CH, TRUE);

    //for (i = 0; i < 2; i++) {
    //    YX_CAN_SetUDS_List(UDS_CAN_CH, i, uds_can_id_list[i], uds_can_fc_id_list[i]);
    //}  

    /* 单帧解析，必须放在多帧解析注册前面 */
    //YX_CAN_InstallIntReceiveProc(UDS_CAN_CH, UDS_SingleFrameHdl);
    //YX_CAN_InstallIntReceiveProc(CAN_COM_0,  UDS_SingleFrameHdl);
    //PORT_SetCanFilter(UDS_CAN_CH, 1,0x18DB33F1, 0xFFFE3AFF);
}
#endif

