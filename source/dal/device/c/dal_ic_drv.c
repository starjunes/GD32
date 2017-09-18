/********************************************************************************
**
** 文件名:     yx_ic_drv.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现IC卡刷卡功能管理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2014/04/09 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "yx_include.h"
#include "yx_stream.h"
#include "st_gpio_drv.h"
#include "hal_ic_drv.h"
#include "dal_input_drv.h"
#include "dal_ic_drv.h"
#include "yx_mmi_drv.h"
#include "yx_debug.h"

#if EN_ICCARD > 0

/*
********************************************************************************
* 定义配置参数
********************************************************************************
*/
#define PERIOD_DELAY         _SECOND, 1

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static INT8U s_delaytmr;
static DRIVER_T s_driverinfo;
static void (*s_callback_iccard)(DRIVER_T *info, INT8U *data, INT32U datalen) = 0;
#if EN_ICCARD_PASER == 0
static INT8U s_readbuf[260];
#endif


#if EN_ICCARD_PASER > 0
/*******************************************************************
** 函数名:     CheckDataValid
** 函数描述:   校验IC卡中的数据是否有效
** 参数:       [in] data: 数据指针
**             [in] len:  数据长度
** 返回:       有效返回TRUE, 无效返回FALSE
********************************************************************/
static BOOLEAN CheckDataValid(INT8U *data, INT16U len)
{
    INT8U check01, check02, rlen, tr2012, tr2003;
    INT32U i, temp;
    STREAM_T rstrm;
    
    tr2012 = 0;
    tr2003 = 0;
    YX_MEMSET(&s_driverinfo, 0, sizeof(s_driverinfo));
    
    check01 = YX_ChkSum_XOR(&data[0], 127);
    check02 = YX_ChkSum_XOR(&data[32], 95);
    if ((check01 == data[127]) || (check02 == data[127])) {                    /* 2012行驶记录仪标准 */
        YX_InitStrm(&rstrm, data, len);
        
        YX_MovStrmPtr(&rstrm, 32);
        YX_ReadDATA_Strm(&rstrm, s_driverinfo.driverlicense, sizeof(s_driverinfo.driverlicense));/* 驾驶证号 */
        s_driverinfo.driverlicenselen = YX_DecodeStuffDataLen(s_driverinfo.driverlicense, sizeof(s_driverinfo.driverlicense), 0x00);
        
        s_driverinfo.date.year  = YX_ReadBYTE_Strm(&rstrm);
        s_driverinfo.date.month = YX_ReadBYTE_Strm(&rstrm);
        s_driverinfo.date.day   = YX_ReadBYTE_Strm(&rstrm);
        
        YX_ReadDATA_Strm(&rstrm, s_driverinfo.qualification, sizeof(s_driverinfo.qualification));/* 从业资格证 */
        s_driverinfo.qualificationlen = YX_DecodeStuffDataLen(s_driverinfo.qualification, sizeof(s_driverinfo.qualification), 0x00);
        
        tr2012 = true;
    }
    
    if (tr2012 && len > 134) {                                                 /* 2012行驶记录仪扩展段 */
        for (i = 134; i < len; i++) {
            if (data[i] == 0xff) {
                break;
            }
        }
        
        check01 = YX_GetChkSum(&data[130], i - 130);
        check02 = YX_GetNChkSum(&data[130], i - 130);
        if ((check01 == data[128]) && (check02 == data[129])) {
            tr2003 = true;
        }
    }
    
    if (!tr2012 && len > 38) {                                                 /* 2003行驶记录仪 */
        for (i = 38; i < len; i++) {
            if (data[i] == 0xff) {
                break;
            }
        }
        check01 = YX_GetChkSum(&data[34], i - 34);
        check02 = YX_GetNChkSum(&data[34], i - 34);
        if ((check01 == data[32]) && (check02 == data[33])) {
            tr2003 = true;
        }
    }
    
    if (tr2003) {
        YX_InitStrm(&rstrm, data, len);
        
        if (tr2012) {
            YX_MovStrmPtr(&rstrm, 130);
        } else {
            YX_MovStrmPtr(&rstrm, 34);
        }
        
        temp = 0;
        rlen = YX_ReadBYTE_Strm(&rstrm);                                        /* 司机代码 */
        if (rlen > 3) {
            return false;
        }
        if (rlen > 0) {
            for (i = 0; i < rlen; i++) {
                temp  <<= 8;
                temp  |= YX_ReadBYTE_Strm(&rstrm);
            }
            s_driverinfo.driveridlen = YX_DecToAscii(s_driverinfo.driverid, temp, 0);
        }
        
        s_driverinfo.passwordlen = YX_ReadBYTE_Strm(&rstrm);                   /* 登入密码 */
        if (s_driverinfo.passwordlen > sizeof(s_driverinfo.password)) {
            return false;
        }
        YX_ReadDATA_Strm(&rstrm, s_driverinfo.password, s_driverinfo.passwordlen);
        
        s_driverinfo.namelen = YX_ReadBYTE_Strm(&rstrm);                       /* 司机姓名 */
        if (s_driverinfo.namelen > sizeof(s_driverinfo.name)) {
            return false;
        }
        YX_ReadDATA_Strm(&rstrm, s_driverinfo.name, s_driverinfo.namelen);
        
        s_driverinfo.identitylen = YX_ReadBYTE_Strm(&rstrm);                   /* 身份证号 */
        if (s_driverinfo.identitylen > sizeof(s_driverinfo.identity)) {
            return false;
        }
        YX_ReadDATA_Strm(&rstrm, s_driverinfo.identity, s_driverinfo.identitylen);
        
        s_driverinfo.institutionlen = YX_ReadBYTE_Strm(&rstrm);                /* 发证机关 */
        if (s_driverinfo.institutionlen > sizeof(s_driverinfo.institution)) {
            return false;
        }
        YX_ReadDATA_Strm(&rstrm, s_driverinfo.institution, s_driverinfo.institutionlen);
    }
    
    return true;
}
#endif

/*******************************************************************
** 函数名:     Read_SLE4442
** 函数描述:   读取SLE4442卡信息
**             [out] dptr：  数据指针
**             [in]  maxlen：数据长度
** 返回:       成功返回TRUE, 失败返回FALSE
********************************************************************/
static BOOLEAN Read_SLE4442(INT8U *dptr, INT16U maxlen)
{
    if (HAL_IC_ReadSLE4442(0, dptr, maxlen) == maxlen) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/*******************************************************************
** 函数名:     Read_24CXX
** 函数描述:   读取24CXX卡数据(TRUE表示24c64接口;FALSE表示24c08接口)
**             [out] dptr：  数据指针
**             [in]  maxlen：数据长度
** 返回:       成功返回TRUE, 失败返回FALSE
********************************************************************/
static BOOLEAN Read_24CXX(INT8U *dptr, INT16U maxlen)
{
    if (HAL_IC_Read24CXX(0, dptr, maxlen) == maxlen) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/*******************************************************************
** 函数名:     ReadCardInfo
** 函数描述:   读取卡信息
**             无
** 返回:       无
********************************************************************/
static void ReadCardInfo(void)
{
#if EN_ICCARD_PASER > 0
    INT8U status, type;
    INT8U readbuf[260];
    
    if (DAL_INPUT_ReadFilterStatus(IPT_ICDECT)) {
        #if DEBUG_ICCARD > 0
        printf_com("<检测到IC卡插入>\r\n");
        #endif
        
        if (Read_SLE4442(&readbuf[4], 256)) {                                  /* 读取是否为4442卡 */
            #if DEBUG_ICCARD > 0
            printf_com("<读取SLE4442卡(%d)>\r\n", sizeof(readbuf));
            //printf_hex(readbuf, 64);
            //while (!printf_hex(&readbuf[64], 64));
            //while (!printf_hex(&readbuf[128], 64));
            //while (!printf_hex(&readbuf[192], 64));
            //printf_com(">\r\n");
            #endif
            
            if (CheckDataValid(&readbuf[4], 256)) {
                status = IC_STATUS_SUCC;
                type   = IC_TYPE_SLE4442;
            } else {
                status = IC_STATUS_ERROR;
                type   = IC_TYPE_UNKNOW;
            }
        } else if (Read_24CXX(&readbuf[4], 256)) {                             /* 读取是否为24CXX卡 */
            #if DEBUG_ICCARD > 0
            printf_com("<读取24CXX系列卡(%d)>\r\n", sizeof(readbuf));
            //printf_hex(readbuf, 64);
            //while (!printf_hex(&readbuf[64], 64));
            //while (!printf_hex(&readbuf[128], 64));
            //while (!printf_hex(&readbuf[192], 64));
            //printf_com(">\r\n");
            #endif
            
            if (CheckDataValid(&readbuf[4], 256)) {
                status = IC_STATUS_SUCC;
                type   = IC_TYPE_24CXX;
            } else {
                status = IC_STATUS_ERROR;
                type   = IC_TYPE_UNKNOW;
            }
        } else {
            status = IC_STATUS_ERROR;
            type   = IC_TYPE_UNKNOW;
        }
        
        if (status == IC_STATUS_SUCC) {
            readbuf[0] = 0x00;
            readbuf[1] = 0x00;
            readbuf[2] = 0x01;
            readbuf[3] = 0x00;
        }
    } else {
        #if DEBUG_ICCARD > 0
        printf_com("<检测到IC卡拔出>\r\n");
        #endif
        
        status = IC_STATUS_NOCARD;
        type   = IC_TYPE_UNKNOW;
    }
    
    if (status != s_driverinfo.status) {
        s_driverinfo.status = status;
        s_driverinfo.type   = type;
        if (s_callback_iccard != 0) {
            s_callback_iccard(&s_driverinfo, readbuf, sizeof(readbuf));
        }
    }
#else
    INT8U status, type;
    
    if (DAL_INPUT_ReadFilterStatus(IPT_ICDECT)) {
        #if DEBUG_ICCARD > 0
        printf_com("<检测到IC卡插入>\r\n");
        #endif
        
        if (Read_SLE4442(&s_readbuf[4], 256)) {                                  /* 读取是否为4442卡 */
            #if DEBUG_ICCARD > 0
            printf_com("<读取SLE4442卡(%d)>\r\n", sizeof(s_readbuf));
            //printf_hex(readbuf, 64);
            //while (!printf_hex(&readbuf[64], 64));
            //while (!printf_hex(&readbuf[128], 64));
            //while (!printf_hex(&readbuf[192], 64));
            //printf_com(">\r\n");
            #endif
            
            status = IC_STATUS_SUCC;
            type   = IC_TYPE_SLE4442;
        } else if (Read_24CXX(&s_readbuf[4], 256)) {                             /* 读取是否为24CXX卡 */
            #if DEBUG_ICCARD > 0
            printf_com("<读取24CXX系列卡(%d)>\r\n", sizeof(s_readbuf));
            //printf_hex(readbuf, 64);
            //while (!printf_hex(&readbuf[64], 64));
            //while (!printf_hex(&readbuf[128], 64));
            //while (!printf_hex(&readbuf[192], 64));
            //printf_com(">\r\n");
            #endif
            
            status = IC_STATUS_SUCC;
            type   = IC_TYPE_24CXX;
        } else {
            #if DEBUG_ICCARD > 0
            printf_com("<读取IC卡失败>\r\n");
            #endif
            status = IC_STATUS_ERROR;
            type   = IC_TYPE_UNKNOW;
        }
        
        if (status == IC_STATUS_SUCC) {
            s_readbuf[0] = 0x00;
            s_readbuf[1] = 0x00;
            s_readbuf[2] = 0x01;
            s_readbuf[3] = 0x00;
        }
    } else {
        #if DEBUG_ICCARD > 0
        printf_com("<检测到IC卡拔出>\r\n");
        #endif
        
        status = IC_STATUS_NOCARD;
        type   = IC_TYPE_UNKNOW;
    }
    
    if (status != s_driverinfo.status) {
        s_driverinfo.status = status;
        s_driverinfo.type   = type;
        if (s_callback_iccard != 0) {
            s_callback_iccard(&s_driverinfo, s_readbuf, sizeof(s_readbuf));
        }
    }
#endif
}

/*******************************************************************
** 函数名:     DelayTmrProc
** 函数描述:   定时器
** 参数:       [in] index:定时器标识
** 返回:       无
********************************************************************/
static void DelayTmrProc(void *index)
{
    OS_StopTmr(s_delaytmr);
    
    ReadCardInfo();
}

/*******************************************************************
** 函数名:     SignalChangeInformer
** 函数描述:   信号跳变通知处理
** 参数:       [in] port: 输入口编号，见INPUT_IO_E
**             [in] mode: 信号跳变触发模式,INPUT_TRIG_E
** 返回:       无
********************************************************************/
static void SignalChangeInformer(INT8U port, INT8U mode)
{
    OS_ASSERT((port == IPT_ICDECT), RETURN_VOID);
    
    OS_StopTmr(s_delaytmr);
    ReadCardInfo();
}

/*******************************************************************
** 函数名:     DAL_IC_InitDrv
** 函数描述:   模块初始化
** 参数:       无
** 返回:       无
********************************************************************/
void DAL_IC_InitDrv(void)
{
    YX_MEMSET(&s_driverinfo, 0, sizeof(s_driverinfo));
    s_driverinfo.status = IC_STATUS_NOCARD;
    
    s_delaytmr = OS_CreateTmr(TSK_ID_DAL, 0, DelayTmrProc);
    OS_StartTmr(s_delaytmr, PERIOD_DELAY);
    
    DAL_INPUT_InstallTriggerProc(IPT_ICDECT, INPUT_TRIG_POSITIVE | INPUT_TRIG_NEGATIVE, SignalChangeInformer);
}

/*******************************************************************
** 函数名:     DAL_IC_RegistIccardProc
** 函数描述:   注册IC卡事件通知函数,最大可注册个数为 MAX_REG_ICCARD
** 参数:       [in] handler: 通知器
** 返回:       成功返回TRUE, 失败返回FALSE
********************************************************************/
BOOLEAN DAL_IC_RegistIccardProc(void (*handler)(DRIVER_T *info, INT8U *data, INT32U datalen))
{
    s_callback_iccard = handler;
    return TRUE;
}

/*******************************************************************
** 函数名:     DAL_IC_GetDriverInfo
** 函数描述:   获取驾驶员刷卡信息
** 参数:       无
** 返回:       成功返回信息
********************************************************************/
DRIVER_T *DAL_IC_GetDriverInfo(void)
{
    return &s_driverinfo;
}

/*******************************************************************
** 函数名:     DAL_IC_WriteData
** 函数描述:   写卡数据
** 参数:       [in] offset: 偏移地址
**             [in] sptr：  数据指针
**             [in] slen:   数据长度
** 返回:       写入数据长度
********************************************************************/
INT32U DAL_IC_WriteData(INT32U offset, INT8U *sptr, INT16U slen)
{
    return HAL_IC_WriteData(offset, sptr, slen);
}

/*******************************************************************
** 函数名:     DAL_IC_RereadIccardData
** 函数描述:   重新读取IC卡中的数据
** 参数:       无
** 返回:       无
********************************************************************/
void DAL_IC_RereadIccardData(void)
{
    ReadCardInfo();
}

#if EN_ICCARD_PASER == 0
/*******************************************************************
** 函数名:     DAL_IC_GetData
** 函数描述:   获取IC卡原始数据
** 参数:       无
** 返回:       成功返回数据指针
********************************************************************/
INT8U *DAL_IC_GetData(void)
{
    return s_readbuf;
}
#endif


#endif

/************************** (C) COPYRIGHT 2010 XIAMEN YAXON.LTD *******************END OF FILE******/


