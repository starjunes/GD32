/******************************************************************************
**
** 文件名   : yx_uds_transfer.c
** 版权所有 : (c) 2005-2021 厦门雅迅网络股份有限公司
** 文件描述 : uds诊断传输服务 
** 创建人   : cym  ,  2021年7月22日 
**
 ******************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者    |  修改记录
**===============================================================================
**| 2021 /7/22 | cym     |  创建该文件
**
*********************************************************************************/
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
#include "yx_uds_drv.h"
#include "bal_gpio_cfg.h"
#include "port_can.h"
#if EN_UDS_TRANS > 0

/*
********************************************************************************
* 定义配置参数
********************************************************************************
*/      
#define PROGRAME_SIZE          32

#define MAX_DRIVER_SIZE        (4 * 1024)
#define MAX_MCU_UPDATE_SIZE    (192 * 1024)

/* 一次传输大小 */
#define UDS_TRANSFER_BLOCK     512
/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
/* 历程控制类型 */
typedef enum {
    ROUTINE_CTR_STAR   = 0x01,                                                  /* 启动 */
    ROUTINE_CTR_STOP   = 0x02,                                                  /* 结束 */
    ROUTIN_CTRE_RESULT = 0x03,                                                  /* 查询结果 */
} UDS_ROUTINE_CTR_E;

/* 历程ID */
typedef enum {
    ROUTINE_CHECKPROG     = 0x0202,                                             /* 数据校验(程序一致性) */
    ROUTINE_ERASEMEMORY   = 0xFF00,                                             /* 擦除内存 */
    ROUTINE_CHECKSOFTWARE = 0xFF01,                                             /* 逻辑块依赖性检查 */
    ROUTINE_CONDITION     = 0xFF02,                                             /* 编程预条件检查 */
} UDS_ROUTINE_ID_E;

/* 升级对象 */
typedef enum {
    UDS_UPDATE_NONE,
    UDS_UPDATE_DREVER,                                                          /* 下载FLASH DRIVER */       
    UDS_UPDATE_MCU,                                                             /* 下载MCU */
    UDS_UPDATE_MPU,                                                             /* 下载MPU */
} UDS_UPDATE_TYYE_E;

/* 传输状态 */
typedef enum {
    UDS_TRANSFER_IDLE,                                                          /* 空闲 */
    UDS_TRANSFER_UP,                                                            /* 上传 */
    UDS_TRANSFER_DOWN,                                                          /* 下载 */
} UDS_TRANSFER_E;

typedef struct {
    INT8U  type;        /* 升级对象，见UDS_UPDATE_TYYE_E */
    INT8U  status;      /* 传输状态，见UDS_TRANSFER_E */
    INT8U  err_cnt;     /* 错误计数 */
    INT32U crc;         /* crc32校验值 */
    INT32U mem_addr;    /* 刷写地址 */
    INT32U mem_size;    /* 刷写数据大小 */
    INT32U tran_idx;    /* 传输序号 */
    INT32U tran_len;    /* 已传输的长度计数 */
} UDS_TRANS_OBJ_T;

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static UDS_TRANS_OBJ_T uds_tran_obj;

/*
********************************************************************************
* 定义本地接口
********************************************************************************
*/

/*******************************************************************
** 函数名:    UDS_Transfer_Error
** 函数描述:  UDS诊断上传/下载服务出错处理
** 参数:      无
** 返回:      无
********************************************************************/
static void UDS_Transfer_Error(void)
{
    if (++uds_tran_obj.err_cnt >= 3) {
        uds_tran_obj.status = UDS_TRANSFER_IDLE;
        uds_tran_obj.err_cnt = 0;
    }
}

/*******************************************************************
** 函数名:    UDS_Update_CRC32
** 函数描述:  下载数据校验码算法
** 参数:      [in] ptr              数据指针
**            [in] len              数据长度
** 返回:      校验值
********************************************************************/
static void UDS_Update_CRC32(INT8U *ptr, INT32U len) 
{
    INT8U i;

    while (len) {
        uds_tran_obj.crc ^= *ptr++;
        for (i = 0; i < 8; i++) {
            if ((uds_tran_obj.crc & 1) != 0) {
                uds_tran_obj.crc >>= 1;
                uds_tran_obj.crc ^= 0xEDB88320;
            } else {
                uds_tran_obj.crc >>= 1;
            }
        }
        len--;
    }
}

/*******************************************************************
** 函数名:    UDS_Update_Saveimageparam
** 功能描述:  向flash写入要备份的参数信息的函数
** 参数:      无
** 返回:      无
********************************************************************/
static BOOLEAN UDS_Update_Saveimageparam(void)
{
    INT8U	*data_ptr;
    INT16U  check;

    IMG_INF_T* img_inf;
    img_inf = GetImgInfObj();

    /* 计算累加和 */
    data_ptr = (INT8U *)UPDATE_MUC_BACK_ADDR_BEGIN;
    
    img_inf->flag       = 0x55aa;
    img_inf->appsize    = uds_tran_obj.mem_size;
    
    ClearWatchdog();
    check = YX_CheckSum_2U((INT8U*)data_ptr, img_inf->appsize);
    ClearWatchdog();   
    img_inf->check_sum  = check;


    #if DEBUG_TRANS > 0
    debug_printf("<**** 1.保存升级参数:check_sum(0x%x), file_len(%d) ****>\r\n", img_inf->check_sum, img_inf->appsize);
    #endif
    return TRUE;
}

/*******************************************************************
** 函数名:    UDS_Flash_ProgramData
** 函数描述:  刷写FLASH
** 参数:      [in] startaddr : 起始地址
**            [in] sdata     : 待写入数据缓存指针
**            [in] dlen      : 待写入数据长度(字节)，必须4字节的整数倍
** 返回:      操作结果
********************************************************************/
static BOOLEAN UDS_Flash_ProgramData(INT32U startaddr, INT8U *sdata, INT32U dlen)
{
    BOOLEAN  rt;
    INT16U   cnt, program_times;
    INT16U   left_data;
    INT8U    temp[32];
    INT32U   cur_addr;
    STREAM_T rstrm;
    
    program_times = dlen / PROGRAME_SIZE;    /* 总共分成多少份PROGRAME_SIZE大小的块 */
    left_data     = dlen % PROGRAME_SIZE;    /* 剩余多少字节 */
    
    YX_InitStrm(&rstrm, sdata, dlen); 
    
    OS_ENTER_CRITICAL();
    cur_addr = startaddr;
    /* 写入数据 */
    for (cnt = 0; cnt < program_times; cnt++) {
        ClearWatchdog();
        YX_ReadDATA_Strm(&rstrm, temp, PROGRAME_SIZE);
        
        /* 每次写入PROGRAME_SIZE字节 */
        rt = HAL_FLASH_WriteData(FLASH_TYPE_CODE, cur_addr, (INT32U)temp, PROGRAME_SIZE);
        if (rt == FALSE) {
            OS_EXIT_CRITICAL();
            #if DEBUG_UPDATE > 0
            debug_printf("<**** 1.(uds-> 0x36)写入升级升级错误,addr:0x%x ****>\r\n",cur_addr);
            #endif   
            return FALSE;
        }
        cur_addr += PROGRAME_SIZE;
    }

    YX_MEMSET(temp, 0xFF, sizeof(temp));

    /* 写入剩余数据 */
    if (left_data) {
        ClearWatchdog();

        YX_ReadDATA_Strm(&rstrm, temp, left_data);
        /* 每次写入PROGRAME_SIZE字节 */
        rt = HAL_FLASH_WriteData(FLASH_TYPE_CODE, cur_addr, (INT32U)temp, PROGRAME_SIZE);
        if (rt == FALSE) {
            OS_EXIT_CRITICAL();
            #if DEBUG_UPDATE > 0
            debug_printf("<**** 2.(uds-> 0x36)写入升级升级错误,addr:0x%x ****>\r\n",cur_addr);
            #endif              
            return FALSE;
        }
        cur_addr += left_data;
    }
    OS_EXIT_CRITICAL();    
    return TRUE;
}

/*****************************************************************************
**  函数名:  UpdateEraseBackSector
**  函数描述: 擦除备份区(MID空间的sector 6、sector 7用于升级程序备份)
**  参数:    无
**  返回:    FALSE:执行失败 TRUE:执行成功
*****************************************************************************/
static BOOLEAN UDS_EraseBackSctor(void)
{
    ClearWatchdog();
    OS_ENTER_CRITICAL();
    /* UnLock MID空间 */
    if (HAL_FLASH_LockSet(FLASH_TYPE_CODE, FALSE, FLASH_ADDR_MID) == FALSE) {
        OS_EXIT_CRITICAL();
        return FALSE;
    } 
    if (HAL_FLASH_BlankCheck(FLASH_TYPE_CODE, CODE_FLASH_BLOCK_6) == FALSE) {
        ClearWatchdog();
        if (HAL_FLASH_Erase(FLASH_TYPE_CODE, CODE_FLASH_BLOCK_6) == FALSE) {
            /* Lock MID空间 */
            HAL_FLASH_LockSet(FLASH_TYPE_CODE, TRUE, FLASH_ADDR_MID);
            OS_EXIT_CRITICAL();
            return FALSE;
        }
    }
    ClearWatchdog();
    if (HAL_FLASH_BlankCheck(FLASH_TYPE_CODE, CODE_FLASH_BLOCK_7) == FALSE) {
        ClearWatchdog();
        if (HAL_FLASH_Erase(FLASH_TYPE_CODE, CODE_FLASH_BLOCK_7) == FALSE) {
            /* Lock MID空间 */
            HAL_FLASH_LockSet(FLASH_TYPE_CODE, TRUE, FLASH_ADDR_MID);
            OS_EXIT_CRITICAL();
            return FALSE;
        }
    }

    ClearWatchdog();  
    OS_EXIT_CRITICAL();
    return TRUE;
}

/*******************************************************************
** 函数名   : UDS_SID31_Response
** 函数描述 : 历程控制肯定响应 
** 参数     : [in] routine_ctl_type: 历程控制类型, 见UDS_ROUTINE_CTR_E
**            [in] routine_id      : 历程控制类型, 见UDS_ROUTINE_ID_E
**            [in] result          : 结果,TRUE / FALSE
** 返回     : 无
********************************************************************/
static void UDS_SID31_Response(UDS_ROUTINE_CTR_E routine_ctl_type, UDS_ROUTINE_ID_E routine_id, INT8U result)
{
    INT16U   rt_id;
    INT8U    memptr[8];
    STREAM_T wstrm;
    
    YX_InitStrm(&wstrm, memptr, 8);
    /* 肯定响应标识*/
    YX_WriteBYTE_Strm(&wstrm, (SID_31 + RESP_ADD));   
    /* 历程控制类型 */
    YX_WriteBYTE_Strm(&wstrm, routine_ctl_type);                                
    /* 历程ID */
    rt_id = routine_id;
    YX_WriteBYTE_Strm(&wstrm, rt_id >> 8);                                    
    YX_WriteBYTE_Strm(&wstrm, rt_id & 0xFF);  

    /* 历程控制结果 */
    YX_WriteBYTE_Strm(&wstrm, result);
    
    YX_UDS_CanSendSingle(YX_GetStrmStartPtr(&wstrm),YX_GetStrmLen(&wstrm));
}

/*******************************************************************
** 函数名   : UDS_SID34_Response
** 函数描述 : 请求下载肯定响应
** 参数     : 无
** 返回     : 无
********************************************************************/
static void UDS_SID34_Response(void)
{
    INT8U    memptr[8];
    STREAM_T wstrm;
    
    YX_InitStrm(&wstrm, memptr, 8);
    /* 肯定响应标识*/
    YX_WriteBYTE_Strm(&wstrm, (SID_34 + RESP_ADD));   
    /* 传输大小字段长度(0x20高4位，即2byte) */
    YX_WriteBYTE_Strm(&wstrm, 0x20);
    /* 传输大小 */
    YX_WriteHWORD_Strm(&wstrm, UDS_TRANSFER_BLOCK + 2);

    YX_UDS_CanSendSingle(YX_GetStrmStartPtr(&wstrm),YX_GetStrmLen(&wstrm));
}

/*******************************************************************
** 函数名   : UDS_SID36_Response
** 函数描述 : 传输数据肯定响应
** 参数     : [in] index : 传输序号
** 返回     : 无
********************************************************************/
static void UDS_SID36_Response(INT8U index)
{
    INT8U    memptr[8];
    STREAM_T wstrm;
    
    YX_InitStrm(&wstrm, memptr, 8);
    /* 肯定响应标识*/
    YX_WriteBYTE_Strm(&wstrm, (SID_36 + RESP_ADD));   
    /* 传输序号 */
    YX_WriteBYTE_Strm(&wstrm, index);
    YX_UDS_CanSendSingle(YX_GetStrmStartPtr(&wstrm),YX_GetStrmLen(&wstrm));
}

/*******************************************************************
** 函数名   : UDS_SID37_Response
** 函数描述 : 退出传输数据肯定响应
** 参数     : 无
** 返回     : 无
********************************************************************/
static void UDS_SID37_Response(void)
{
    INT8U    memptr[8];
    STREAM_T wstrm;
    
    YX_InitStrm(&wstrm, memptr, 8);
    /* 肯定响应标识*/
    YX_WriteBYTE_Strm(&wstrm, (SID_37 + RESP_ADD));   
    YX_UDS_CanSendSingle(YX_GetStrmStartPtr(&wstrm),YX_GetStrmLen(&wstrm));
}

/*
********************************************************************************
* 定义对外接口
********************************************************************************
*/

/*****************************************************************************
**  函数名:  YX_UDS_RoutineControl
**  函数描述: 历程控制服务(0x31)
**  参数:    [in] sid  : 历程控制sid
**           [in] data : 数据
**           [in] len  : 数据长度
**  返回:    无
*****************************************************************************/
void YX_UDS_RoutineControl(INT8U sid, INT8U* data, INT16U len)
{
    INT8U             len1, len2, tmp;
    INT32U            check;
    BOOLEAN           result;
    BOOLEAN           is_pos_ack;
    STREAM_T          rstm;
    UDS_ROUTINE_CTR_E rt_type;
    UDS_ROUTINE_ID_E  rt_id;
    INT32U            mem_addr, mem_size;
    
    YX_InitStrm(&rstm, data, len);    

    /* 是否禁止肯定应答 */
    is_pos_ack = TRUE;
    if (sid & SUPPRESS_RESP) {
        /* 禁止肯定应答 */
        is_pos_ack = FALSE;
        sid &= 0x7F;
    }

    /* 历程控制类型 */
    rt_type = YX_ReadBYTE_Strm(&rstm);

    /* 历程ID */
    rt_id   = YX_ReadHWORD_Strm(&rstm);

    if (rt_type == ROUTINE_CTR_STAR) {

        switch (rt_id) {
            case ROUTINE_CHECKPROG :        /* 数据校验程序一致性(0x0202) */
                if (YX_UDS_GetSession() != SESSION_PROGRAM) {
                   YX_UDS_NegativeResponse(sid, NRC_7F);
                   return;
                }
                if (YX_UDS_GetSecurity() != 0x03) {
                    YX_UDS_NegativeResponse(sid, NRC_33);
                    return;
                }
                if (len != 7) {
                    YX_UDS_NegativeResponse(sid, NRC_13);
                    return;
                }
                
                check = YX_ReadLONG_Strm(&rstm);
                
                if (uds_tran_obj.type == UDS_UPDATE_DREVER) {
                    #if 0
                    if (is_pos_ack) {
                        YX_UDS_NegativeResponse(sid, NRC_78);
                    }
                    #endif
                    /* 实际flash driver不可重编程，默认应答成功 */
                    result = TRUE;   
                    #if DEBUG_TRANS > 0
                    debug_printf("<**** (0x31 -> 0x0202)flash driver ok ****>\r\n");
                    #endif                        
                } else if (uds_tran_obj.type == UDS_UPDATE_MCU) {
                    #if 0
                    if (is_pos_ack) {
                        YX_UDS_NegativeResponse(sid, NRC_78);
                    }               
                    #endif
                    
                    if (check == (uds_tran_obj.crc ^ 0xFFFFFFFF)) {
                        uds_tran_obj.crc = 0xFFFFFFFF;
                        result = TRUE;  
                        #if DEBUG_TRANS > 0
                        debug_printf("<**** (0x31 -> 0x0202)mcu app ok, check:0x%x ****>\r\n",check);
                        #endif                           
                    } else {
                        result = FALSE;  
                        #if DEBUG_TRANS > 0
                        debug_printf("<**** (0x31 -> 0x0202)mcu app err, check:0x%x, crc32:0x%x ****>\r\n",check, uds_tran_obj.crc ^ 0xFFFFFFFF);
                        #endif                               
                    }
                } else {
                    /* MPU升级 */
                    YX_UDS_NegativeResponse(sid, NRC_78);
                    return;
                }

                break;
            case ROUTINE_ERASEMEMORY :      /* 擦除内存(0xFF00) */
                if (YX_UDS_GetSession() != SESSION_PROGRAM) {
                    YX_UDS_NegativeResponse(sid, NRC_7F);
                    return;
                }
                
                if (YX_UDS_GetSecurity() != 0x03) {
                    YX_UDS_NegativeResponse(sid, NRC_33);
                    return;
                }
                
                tmp = YX_ReadBYTE_Strm(&rstm);
                /* 内存大小字段长度 */
                len2 = tmp >> 4;
                /* 内存地址字段长度 */
                len1 = tmp & 0x0F;

                /* 获取内存地址 */
                if (len1 == 1) {
                    mem_addr = YX_ReadBYTE_Strm(&rstm);
                } else if (len1 == 2) {
                    mem_addr = YX_ReadHWORD_Strm(&rstm);
                } else if (len1 == 3) {
                    mem_addr  = (INT32U)(YX_ReadBYTE_Strm(&rstm)) << 16;
                    mem_addr |= (INT32U)(YX_ReadBYTE_Strm(&rstm)) << 8;
                    mem_addr |= (INT32U)(YX_ReadBYTE_Strm(&rstm));  
                } else if (len1 == 4) {
                    mem_addr = YX_ReadLONG_Strm(&rstm);
                } else {
                    YX_UDS_NegativeResponse(sid, NRC_13);
                    return;
                }

                /* 获取内存大小 */
                if (len2 == 1) {
                    mem_size = YX_ReadBYTE_Strm(&rstm);
                } else if (len2 == 2) {
                    mem_size = YX_ReadHWORD_Strm(&rstm);
                } else if (len2 == 3) {
                    mem_size  = (INT32U)(YX_ReadBYTE_Strm(&rstm)) << 16;
                    mem_size |= (INT32U)(YX_ReadBYTE_Strm(&rstm)) << 8;
                    mem_size |= (INT32U)(YX_ReadBYTE_Strm(&rstm));
                } else if (len2 == 4) {
                    mem_size = YX_ReadLONG_Strm(&rstm);
                } else {
                    YX_UDS_NegativeResponse(sid, NRC_13);
                    return;
                }
                
                if (mem_size <= MAX_MCU_UPDATE_SIZE) {
                    uds_tran_obj.type = UDS_UPDATE_MCU;
                } else {
                    uds_tran_obj.type = UDS_UPDATE_MPU;
                }
                
                if (uds_tran_obj.type == UDS_UPDATE_MPU) {
                    YX_UDS_NegativeResponse(sid, NRC_78);
                    return;
                }
                
                if (tmp == 0x00) {
                    YX_UDS_NegativeResponse(sid, NRC_31);
                    return;
                }
                
                if (len != 4 + len1 + len2) {
                    YX_UDS_NegativeResponse(sid, NRC_13);
                    return;
                }
                
                if (is_pos_ack) {
                    YX_UDS_NegativeResponse(sid, NRC_78);
                }
                
                if (uds_tran_obj.type == UDS_UPDATE_MCU) {
                    /* 擦除备份区 */
                    if (UDS_EraseBackSctor() == TRUE) {
                        result = 0x01;
                        #if DEBUG_TRANS > 0
                        debug_printf("<**** (0x31)擦除FLASH成功 ****>\r\n");
                        #endif                        
                    } else {
                        result = 0x00;
                        #if DEBUG_TRANS > 0
                        debug_printf("<**** (0x31)擦除FLASH失败 ****>\r\n");
                        #endif                          
                    }
                } else {
                    result = 0x01;
                }
                /* 校验值初始化 */
                uds_tran_obj.crc = 0xFFFFFFFF;                                    

                break;       
            case ROUTINE_CHECKSOFTWARE :    /* 逻辑块依赖性检查(0xFF01) */
                if (YX_UDS_GetSession() != SESSION_PROGRAM) {
                   YX_UDS_NegativeResponse(sid, NRC_7F);
                   return;
                }
                
                if (YX_UDS_GetSecurity() != 0x03) {
                    YX_UDS_NegativeResponse(sid, NRC_33);
                    return;
                }
                
                if (len != 3) {
                    YX_UDS_NegativeResponse(sid, NRC_13);
                    return;
                }
                
                if (uds_tran_obj.type == UDS_UPDATE_MPU) {
                    YX_UDS_NegativeResponse(sid, NRC_78);
                    return;
                }
                
                if (is_pos_ack) {
                    YX_UDS_NegativeResponse(sid, NRC_78);
                }
                
                if (uds_tran_obj.type == UDS_UPDATE_DREVER) {
                    result = 0x01;
                } else if (uds_tran_obj.type == UDS_UPDATE_MCU) {
                    /* 保存升级参数到flash */
                    if (UDS_Update_Saveimageparam()) {
                        result = 0x01;
                        #if DEBUG_TRANS > 0
                        debug_printf("<**** (0x31 -> 0xFF01)保存参数ok ****>\r\n");
                        #endif                          
                    } else {
                        result = 0x00;
                        #if DEBUG_TRANS > 0
                        debug_printf("<**** (0x31 -> 0xFF01)保存参数err ****>\r\n");
                        #endif                           
                    }
                    /* 锁住flash */    
                    HAL_FLASH_LockSet(FLASH_TYPE_CODE, TRUE, FLASH_ADDR_MID);    
                } else {
                    result = 0x00;
                }

                break; 
            case ROUTINE_CONDITION :        /* 编程预条件检查(0xFF02) */
                if (YX_UDS_GetSession() != SESSION_EXTENDED) {
                    YX_UDS_NegativeResponse(sid, NRC_7F);
                    return;
                }
                #if 0
                if (YX_UDS_GetSecurity() != 0x02 && YX_UDS_GetSecurity() != 0x03) {
                    YX_UDS_NegativeResponse(sid, NRC_33);
                    return;
                }
                #endif
                if (len != 3) {
                    YX_UDS_NegativeResponse(sid, NRC_13);
                    return;
                }
                result = 0x00;
                uds_tran_obj.crc = 0xFFFFFFFF;

                break; 
            default:
                YX_UDS_NegativeResponse(sid, NRC_31);
                return;
        } 
        
        /* 肯定应答 */
        if (is_pos_ack) {
            is_pos_ack = FALSE;
            UDS_SID31_Response(rt_type, rt_id, result);
        }          
    } else if (rt_type == ROUTINE_CTR_STOP) {
        YX_UDS_NegativeResponse(sid, NRC_12);
        return;
    } else if (rt_type == ROUTIN_CTRE_RESULT) {
        YX_UDS_NegativeResponse(sid, NRC_12);
        return;
    } else {
        YX_UDS_NegativeResponse(sid, NRC_12);
        return;
    }
}

/*****************************************************************************
**  函数名:  YX_UDS_ReqDownload
**  函数描述: 请求下载服务(0x34)
**  参数:    [in] sid  : 请求下载sid
**           [in] data : 数据
**           [in] len  : 数据长度
**  返回:    无
*****************************************************************************/
void YX_UDS_ReqDownload(INT8U sid, INT8U* data, INT16U len)
{
    INT8U     sub_fun;
    INT8U     tmp, len1, len2;
    BOOLEAN   is_pos_ack;
    STREAM_T  rstm;

    if (YX_UDS_GetSession() != SESSION_PROGRAM) {
       YX_UDS_NegativeResponse(sid, NRC_7F);
       return;
    }
    if (YX_UDS_GetSecurity() != 0x03) {
        YX_UDS_NegativeResponse(sid, NRC_33);
        return;
    }
    
    YX_InitStrm(&rstm, data, len); 

    /* 是否禁止肯定应答 */
    is_pos_ack = TRUE;
    if (sid & SUPPRESS_RESP) {
        /* 禁止肯定应答 */
        is_pos_ack = FALSE;
        sid &= 0x7F;
    }

    /* 子功能 */
    sub_fun = YX_ReadBYTE_Strm(&rstm);
    sub_fun = sub_fun;
    /* 参数长度 */
    tmp     = YX_ReadBYTE_Strm(&rstm);
    len2    = tmp >> 4;
    len1    = tmp & 0x0F;

    #if DEBUG_TRANS > 0
    debug_printf("<**** (0x34)->(len:%d, len1:%d, len2:%d) ****>\r\n",len, len1, len2);
    #endif
    
    if (len != (2 + len1 + len2)) {
        YX_UDS_NegativeResponse(sid, NRC_13);
        return;
    } 

    /* 获取内存地址 */
    if (len1 == 1) {
        uds_tran_obj.mem_addr = YX_ReadBYTE_Strm(&rstm);
    } else if (len1 == 2) {
        uds_tran_obj.mem_addr = YX_ReadHWORD_Strm(&rstm);
    } else if (len1 == 3) {
        uds_tran_obj.mem_addr  = (INT32U)(YX_ReadBYTE_Strm(&rstm)) << 16;
        uds_tran_obj.mem_addr |= (INT32U)(YX_ReadBYTE_Strm(&rstm)) << 8;
        uds_tran_obj.mem_addr |= (INT32U)(YX_ReadBYTE_Strm(&rstm));  
    } else if (len1 == 4) {
        uds_tran_obj.mem_addr = YX_ReadLONG_Strm(&rstm);
    } else {
        if (uds_tran_obj.type != UDS_UPDATE_MPU) {
            YX_UDS_NegativeResponse(sid, NRC_13);
            return;
        }
    }
    
    /* 获取内存大小 */
    if (len2 == 1) {
        uds_tran_obj.mem_size = YX_ReadBYTE_Strm(&rstm);
    } else if (len2 == 2) {
        uds_tran_obj.mem_size = YX_ReadHWORD_Strm(&rstm);
    } else if (len2 == 3) {
        uds_tran_obj.mem_size  = (INT32U)(YX_ReadBYTE_Strm(&rstm)) << 16;
        uds_tran_obj.mem_size |= (INT32U)(YX_ReadBYTE_Strm(&rstm)) << 8;
        uds_tran_obj.mem_size |= (INT32U)(YX_ReadBYTE_Strm(&rstm));
    } else if (len2 == 4) {
        uds_tran_obj.mem_size = YX_ReadLONG_Strm(&rstm);
    } else {
        if (uds_tran_obj.type != UDS_UPDATE_MPU) {
            YX_UDS_NegativeResponse(sid, NRC_13);
            return;
        }
    }
    
    #if DEBUG_TRANS > 0
    debug_printf("<**** (0x34)刷写addr(0x%x), size(%d) ****>\r\n",uds_tran_obj.mem_addr, uds_tran_obj.mem_size);
    #endif 

    if (uds_tran_obj.mem_size <= MAX_DRIVER_SIZE) {
        uds_tran_obj.type = UDS_UPDATE_DREVER;
        #if DEBUG_TRANS > 0
        debug_printf("<**** (0x34)刷写flash driver: ****>\r\n");
        #endif        
    } else if (uds_tran_obj.mem_size <= MAX_MCU_UPDATE_SIZE) {
        uds_tran_obj.type = UDS_UPDATE_MCU;
        #if DEBUG_TRANS > 0
        debug_printf("<**** (0x34)刷写mcu app ****>\r\n");
        #endif         
    } else {
        uds_tran_obj.type = UDS_UPDATE_MPU;
        #if DEBUG_TRANS > 0
        debug_printf("<**** (0x34)刷写mpu app ****>\r\n");
        #endif         
    }    

    if (uds_tran_obj.type == UDS_UPDATE_MPU) {                                  /* 下载MPU请求 */
        YX_UDS_NegativeResponse(sid, NRC_78);
        uds_tran_obj.status = UDS_TRANSFER_DOWN;
        return;
    } else {
        if (uds_tran_obj.mem_addr % 4) {
            YX_UDS_NegativeResponse(sid, NRC_13);
            return;
        }
    }  

    uds_tran_obj.mem_addr = UPDATE_MUC_BACK_ADDR_BEGIN;
    /* 对长度和地址做判断 */
    if (uds_tran_obj.type == UDS_UPDATE_MCU) {
        if ((uds_tran_obj.mem_addr >= UPDATE_MUC_BACK_ADDR_BEGIN) && 
            ((uds_tran_obj.mem_addr + uds_tran_obj.mem_size) < (UPDATE_MUC_BACK_ADDR_BEGIN + MAX_MCU_UPDATE_SIZE))) {
            if (is_pos_ack) {
                is_pos_ack = FALSE;
                UDS_SID34_Response();
                #if DEBUG_TRANS > 0
                debug_printf("<**** (0x34)1.地址合法 ****>\r\n");
                #endif                  
            }
            #if DEBUG_TRANS > 0
            debug_printf("<**** (0x34)2.地址合法 ****>\r\n");
            #endif     
        } else {
            YX_UDS_NegativeResponse(sid, NRC_31);
            #if DEBUG_TRANS > 0
            debug_printf("<**** (0x34)地址不合法 ****>\r\n");
            #endif                
            return;
        }
    }else {
        if (is_pos_ack) {
            is_pos_ack = FALSE;
            UDS_SID34_Response();                
        }
    }

    uds_tran_obj.status   = UDS_TRANSFER_DOWN;
    uds_tran_obj.tran_idx = 1;
    uds_tran_obj.tran_len = 0;
}

/*****************************************************************************
**  函数名:  YX_UDS_TransData
**  函数描述: 传输数据服务(0x36)
**  参数:    [in] sid  : 传输数据sid
**           [in] data : 数据
**           [in] len  : 数据长度
**  返回:    无
*****************************************************************************/
void YX_UDS_TransData(INT8U sid, INT8U* data, INT16U len)
{
    INT8U     index;
    BOOLEAN   is_pos_ack;
    INT32U    dlen;
    INT32U    startaddr;

    if (YX_UDS_GetSession() != SESSION_PROGRAM) {
       YX_UDS_NegativeResponse(sid, NRC_7F);
       return;
    }
    if (YX_UDS_GetSecurity() != 0x03) {
        YX_UDS_NegativeResponse(sid, NRC_33);
        return;
    }
    
    index = data[0];
    
    /* 是否禁止肯定应答 */
    is_pos_ack = TRUE;
    if (sid & SUPPRESS_RESP) {
        /* 禁止肯定应答 */
        is_pos_ack = FALSE;
        sid &= 0x7F;
    }
    #if DEBUG_TRANS > 0
    debug_printf("<**** (0x36)序号:(%d) (%d) ****>\r\n",index, uds_tran_obj.tran_idx);
    #endif  

    if (uds_tran_obj.status == UDS_TRANSFER_DOWN) {                             /* 下载流程 */
        if (uds_tran_obj.type == UDS_UPDATE_MPU) {

            YX_UDS_NegativeResponse(sid, NRC_78);
            return;
        }
        
        if (uds_tran_obj.tran_len >= uds_tran_obj.mem_size) {
            UDS_Transfer_Error();
            YX_UDS_NegativeResponse(sid, NRC_24);
            #if DEBUG_TRANS > 0
            debug_printf("<**** (0x36)长度无效(NRC_24) ****>\r\n");
            #endif               
            return;
        }
        
        if ((len - 1 > UDS_TRANSFER_BLOCK) || (len < 3)) {
            UDS_Transfer_Error();
            YX_UDS_NegativeResponse(sid, NRC_13);
            #if DEBUG_TRANS > 0
            debug_printf("<**** (0x36)长度无效(NRC_13) ****>\r\n");
            #endif              
            return;
        }
        
        if (index != uds_tran_obj.tran_idx) {
            UDS_Transfer_Error();
            YX_UDS_NegativeResponse(sid, NRC_73);
            #if DEBUG_TRANS > 0
            debug_printf("<**** (0x36)序号无效(NRC_73) ****>\r\n");
            #endif              
            return;
        }
        
        YX_UDS_NegativeResponse(sid, NRC_78);
        uds_tran_obj.tran_idx++;
        if (uds_tran_obj.tran_idx > 255) {
            uds_tran_obj.tran_idx = 0;
        }

        uds_tran_obj.mem_addr = UPDATE_MUC_BACK_ADDR_BEGIN;
        if ((uds_tran_obj.tran_len + len - 1) <= uds_tran_obj.mem_size) {
            /* 传输长度 */                                                                    /* 数据处理&para[1], len - 1 */
            dlen = len - 1;
            /* 写入地址 */
            startaddr = uds_tran_obj.mem_addr + uds_tran_obj.tran_len;
            
            #if DEBUG_TRANS > 0
            debug_printf("<**** (0x36)刷写地址:(0x%x), len(%d) ****>\r\n",startaddr, dlen);
            #endif  

            if (uds_tran_obj.type== UDS_UPDATE_MCU) {
                if (UDS_Flash_ProgramData(startaddr, &data[1], dlen) == FALSE) {
                    YX_UDS_NegativeResponse(sid, NRC_72);
                    return;
                }
            }
            
            uds_tran_obj.tran_len += dlen;
            UDS_Update_CRC32(&data[1], dlen);
            /* 肯定应答 */
            if (is_pos_ack) {
                is_pos_ack = FALSE;
                UDS_SID36_Response(index);
            }            
        } else {
            YX_UDS_NegativeResponse(sid, NRC_71);
        }
    } else {
        UDS_Transfer_Error();
        YX_UDS_NegativeResponse(sid, NRC_24);
    }    

}

/*****************************************************************************
**  函数名:  YX_UDS_ReqTransExit
**  函数描述: 请求传输退出服务(0x37)
**  参数:    [in] sid  : 请求传输退出sid
**  返回:    无
*****************************************************************************/
void YX_UDS_ReqTransExit(INT8U sid)
{
    BOOLEAN   is_pos_ack;

    if (YX_UDS_GetSession() != SESSION_PROGRAM) {
       YX_UDS_NegativeResponse(sid, NRC_7F);
       return;
    }
    if (YX_UDS_GetSecurity() != 0x03) {
        YX_UDS_NegativeResponse(sid, NRC_33);
        return;
    }

    /* 是否禁止肯定应答 */
    is_pos_ack = TRUE;
    if (sid & SUPPRESS_RESP) {
        /* 禁止肯定应答 */
        is_pos_ack = FALSE;
        sid &= 0x7F;
    }

    if (uds_tran_obj.type == UDS_UPDATE_MPU) {

        YX_UDS_NegativeResponse(sid, NRC_78);
        uds_tran_obj.status = UDS_TRANSFER_IDLE;
        return;
    }

    if (uds_tran_obj.status == UDS_TRANSFER_DOWN) {                             /* 结束下载流程 */
        uds_tran_obj.status = UDS_TRANSFER_IDLE;

        if (uds_tran_obj.tran_len == uds_tran_obj.mem_size) {
            if (is_pos_ack) {
                is_pos_ack = FALSE;
                UDS_SID37_Response();
            }
        } else {
            YX_UDS_NegativeResponse(sid, NRC_24);
        }
    } else if (uds_tran_obj.status == UDS_TRANSFER_UP) {                        /* 结束上传流程 */
        uds_tran_obj.status = UDS_TRANSFER_IDLE;
        if (is_pos_ack) {
            is_pos_ack = FALSE;
            UDS_SID37_Response();
        }
    } else {
        YX_UDS_NegativeResponse(sid, NRC_24);
    }
}

/*****************************************************************************
**  函数名:  YX_UDS_TransferInit
**  函数描述: uds诊断传输初始化
**  参数:    无
**  返回:    无
*****************************************************************************/
void YX_UDS_TransferInit(void)
{
    uds_tran_obj.type     = UDS_UPDATE_NONE;
    uds_tran_obj.crc      = 0xFFFFFFFF;
    uds_tran_obj.mem_addr = 0;
    uds_tran_obj.mem_size = 0;
    uds_tran_obj.tran_idx = 0;
    uds_tran_obj.tran_len = 0;
    uds_tran_obj.status   = UDS_TRANSFER_IDLE;
    uds_tran_obj.err_cnt  = 0;
}

#endif

