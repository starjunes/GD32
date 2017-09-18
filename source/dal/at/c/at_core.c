/********************************************************************************
**
** 文件名:     at_core.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现手机模块初始化配置
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2014/06/15 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "yx_include.h"
#include "yx_misc.h"
#include "yx_stream.h"
#include "at_drv.h"



#if EN_AT > 0
/*
********************************************************************************
* define config parameters
********************************************************************************
*/
#define MAX_TRY                                 8
#define MAX_RECOVERY                            10

#define _OPEN                0x01

/*
********************************************************************************
* define s_dcb.status
********************************************************************************
*/

#define NETWORK_NotRegistered               0
#define NETWORK_HomeNetwork                 1
#define NETWORK_NotRegistered_Searching     2
#define NETWORK_Denied                      3
#define NETWORK_Unknown                     4
#define NETWORK_Roaming                     5


/*
********************************************************************************
* define s_dcb.flag
********************************************************************************
*/
#define _ACKSM                                   0x01

/*
********************************************************************************
* define initialization step
********************************************************************************
*/
typedef enum {
    FLAG_ESC,
    FLAG_TEST,
    FLAG_SETDTR,
    FLAG_QVER,
    //FLAG_CALM,
    FLAG_CFUN,
    FLAG_CSMS,
    FLAG_CMGF,
    FLAG_CNMI,
    FLAG_CPMS,
    //FLAG_COLP,
    FLAG_CLIP,
    FLAG_CLCC,
    //FLAG_CPBS,
    //FLAG_QIMEI,
    //FLAG_QICCID,
    //FLAG_QIMSI,
    //FLAG_CLCK,
    //FLAG_CPWD,
    //FLAG_CPIN,
    FLAG_QSIM,
    FLAG_QNET,
    FLAG_QCSQ,
    FLAG_QOSP,
    FLAG_CIPDPDP,
    FLAG_CIPSPRT,
    FLAG_CIPQSEND,
    FLAG_MAX
} FLAG_E;

#define FLAG_MASK            ((1 << FLAG_MAX) - 1)         /* 掩码 */

/*
********************************************************************************
* define timer period
********************************************************************************
*/
#define PERIOD_SCAN          _SECOND, 1
#define MAX_WAIT             3

typedef struct {
    INT8U status;
    INT32U flag;
    INT8U ct_try;
    INT8U ct_wait;
    INT8U ct_recovery;
    INT8U csq;
    INT8U cfun;
    //INT8U simlock;
    //INT8U islock;
    INT8U netstatus;
    INT8U simstatus;
    INT8U service;
    INT8U moduletype;
    //char  password[7];
    //char  opassword[7];
    //char  npassword[7];
    //void (*inform_setpinfunction)(INT8U result);
    //void (*inform_unlockpin)(INT8U result);
    //void (*inform_modifypinpassword)(INT8U result);
} ACB_T;

/*
********************************************************************************
* define module variants
********************************************************************************
*/
static ACB_T s_dcb;
static INT8U s_waittmr;
//static IMSI_T s_imsi;
//static IMEI_T s_imei;
//static ICCID_T s_iccid;
//static GSM_VER_T s_gsm_ver;


/*******************************************************************
** 函数名:     ResetGSM
** 函数描述:   复位GSM模块
** 参数:       无
** 返回:       无
********************************************************************/
static void ResetGSM(void)
{
    if ((s_dcb.status & _OPEN) != 0) {
        #if DEBUG_AT > 0
        printf_com("<restart init gsm>\r\n");
        #endif
        
        YX_SEND_SetEcho(false);
        YX_SEND_AbandonATCmd();

        s_dcb.ct_try      = 0;
        s_dcb.ct_recovery = 0;
    
        //s_dcb.flag = (1 << FLAG_TEST);
        s_dcb.flag = FLAG_MASK;
        AT_POWER_PowerReset();
	}
}

/*******************************************************************
** 函数名:     Proc_ESC
** 函数描述:   发送退出当前AT指令
** 参数:       无
** 返回:       无
********************************************************************/
static void Informer_ESC(INT8U result)
{
    s_dcb.flag &= (~(1 << FLAG_ESC));
    s_dcb.flag |= (1 << FLAG_TEST);
}

static void Proc_ESC(void)
{
    INT32U len;
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    len = AT_ESCAPE(YX_GetStrmStartPtr(wstrm), YX_GetStrmMaxLen(wstrm));
    AT_SEND_SendCmd(&AT_ESCAPE_PARA, YX_GetStrmStartPtr(wstrm), len, Informer_ESC);
}

/*******************************************************************
** 函数名:     InitProc_TEST
** 函数描述:   AT测试命令
** 参数:       无
** 返回:       无
********************************************************************/
static void Informer_TEST(INT8U result)
{
    if (result == AT_SUCCESS) {
        s_dcb.ct_recovery = 0;
        s_dcb.flag &= (~(1 << FLAG_TEST));
    } else {
        if (++s_dcb.ct_recovery > MAX_RECOVERY) {
            #if DEBUG_AT > 0
            printf_com("<多次发送AT没有收到回应，复位GSM模块>\r\n");
            #endif
            
            s_dcb.ct_recovery = 0;
            
            ResetGSM();
        } else {
            s_dcb.ct_wait = MAX_WAIT;
        }
    }
}

static void InitProc_TEST(void)
{
    INT32U len;
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    len = AT_CMD_Test(YX_GetStrmStartPtr(wstrm), YX_GetStrmMaxLen(wstrm));
    AT_SEND_SendCmd(&g_at_test_para, YX_GetStrmStartPtr(wstrm), len, Informer_TEST);
}

#if 0
/*******************************************************************
** 函数名:     InitProc_CLCK
** 函数描述:   PIN码功能设置
** 参数:       无
** 返回:       无
********************************************************************/
static void Informer_CLCK(INT8U result)
{
    if (result == AT_SUCCESS) {
        s_dcb.ct_try = 0;
        s_dcb.flag &= (~(1 << FLAG_CLCK));
        
        if (s_dcb.inform_setpinfunction != 0) {                                /* 设置结果回调 */
            s_dcb.inform_setpinfunction(true);
        }
    } else {
        if (++s_dcb.ct_try >= 1) {
            #if DEBUG_AT > 0
            printf_com("<多次设置PIN码功能失败，复位GSM模块>\r\n");
            #endif
            
            s_dcb.ct_try = 0;
            s_dcb.flag &= (~(1 << FLAG_CLCK));
            if (s_dcb.simlock == 0) {                                          /* 解锁未成功，需要复位 */
                //OS_ASSERT(0, RETURN_VOID);
            } else {                                                           /* 设锁未成功，不需要复位 */
                s_dcb.simlock = 0;
            }
            
            if (s_dcb.inform_setpinfunction != 0) {                            /* 设置结果回调 */
                s_dcb.inform_setpinfunction(false);
            }
        } else {
            s_dcb.ct_wait = MAX_WAIT;
        }
    }
}

static void InitProc_CLCK(void)
{
    INT32U len;
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    if (s_dcb.simlock) {
        len = AT_CLCK(YX_GetStrmStartPtr(wstrm), YX_GetStrmMaxLen(wstrm), 1, s_dcb.password);
    } else {
        len = AT_CLCK(YX_GetStrmStartPtr(wstrm), YX_GetStrmMaxLen(wstrm), 0, s_dcb.password);
    }
    AT_SEND_SendCmd(&AT_CLCK_PARA, YX_GetStrmStartPtr(wstrm), len, Informer_CLCK);
}

/*******************************************************************
** 函数名:     InitProc_CPWD
** 函数描述:   PIN码功能设置
** 参数:       无
** 返回:       无
********************************************************************/
static void Informer_CPWD(INT8U result)
{
    if (result == AT_SUCCESS) {
        s_dcb.ct_try = 0;
        s_dcb.flag &= (~(1 << FLAG_CPWD));
        
        if (s_dcb.inform_modifypinpassword != 0) {                             /* 设置结果回调 */
            s_dcb.inform_modifypinpassword(true);
        }
    } else {
        if (++s_dcb.ct_try >= 1) {
            #if DEBUG_AT > 0
            printf_com("<多次修改PIN码功能失败，复位GSM模块>\r\n");
            #endif
            
            s_dcb.ct_try = 0;
            s_dcb.flag &= (~(1 << FLAG_CPWD));
            
            if (s_dcb.inform_modifypinpassword != 0) {                         /* 设置结果回调 */
                s_dcb.inform_modifypinpassword(false);
            }
        } else {
            s_dcb.ct_wait = MAX_WAIT;
        }
    }
}

static void InitProc_CPWD(void)
{
    INT32U len;
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    len = AT_CPWD(YX_GetStrmStartPtr(wstrm), YX_GetStrmMaxLen(wstrm), s_dcb.opassword, s_dcb.npassword);
    AT_SEND_SendCmd(&AT_CPWD_PARA, YX_GetStrmStartPtr(wstrm), len, Informer_CPWD);
}

/*******************************************************************
** 函数名:     InitProc_CPIN
** 函数描述:   解PIN码
** 参数:       无
** 返回:       无
********************************************************************/
static void Informer_CPIN(INT8U result)
{
    if (result == AT_SUCCESS) {
        s_dcb.ct_try = 0;
        s_dcb.flag &= (~(1 << FLAG_CPIN));
        
        if (s_dcb.inform_unlockpin != 0) {                                     /* 设置结果回调 */
            s_dcb.inform_unlockpin(true);
        }
    } else {
        if (++s_dcb.ct_try >= 1) {
            #if DEBUG_AT > 0
            printf_com("<多次解PIN码功能失败，复位GSM模块>\r\n");
            #endif
            
            s_dcb.ct_try = 0;
            s_dcb.flag &= (~(1 << FLAG_CPIN));
            //OS_ASSERT(0, RETURN_VOID);
            
            if (s_dcb.inform_unlockpin != 0) {                                 /* 设置结果回调 */
                s_dcb.inform_unlockpin(false);
            }
        } else {
            s_dcb.ct_wait = MAX_WAIT;
        }
    }
}

static void InitProc_CPIN(void)
{
    INT32U len;
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    len = AT_CPIN(YX_GetStrmStartPtr(wstrm), YX_GetStrmMaxLen(wstrm), s_dcb.password);
    AT_SEND_SendCmd(&AT_CPIN_PARA, YX_GetStrmStartPtr(wstrm), len, Informer_CPIN);
}
#endif

/*******************************************************************
** 函数名:     InitProc_SETDTR
** 函数描述:   设置DTR管脚应用
** 参数:       无
** 返回:       无
********************************************************************/
static void Informer_SETDTR(INT8U result)
{
    s_dcb.flag &= (~(1 << FLAG_SETDTR));
}

static void InitProc_SETDTR(void)
{
    INT32U len;
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    len = AT_AND_D(YX_GetStrmStartPtr(wstrm), YX_GetStrmMaxLen(wstrm));
    AT_SEND_SendCmd(&AT_AND_D_PARA, YX_GetStrmStartPtr(wstrm), len, Informer_SETDTR);
}

/*******************************************************************
** 函数名:     InitProc_QVER
** 函数描述:   查询模块版本号
** 参数:       无
** 返回:       无
********************************************************************/
static void Informer_QVER(INT8U result)
{
    if (result == AT_SUCCESS) {
        #if DEBUG_AT > 0
        printf_com("<手机模块型号(%d)>\r\n", g_at_ack_info.moduleversion.moduletype);
        #endif
        
        s_dcb.moduletype = g_at_ack_info.moduleversion.moduletype;
        s_dcb.ct_try = 0;
        s_dcb.flag &= (~(1 << FLAG_QVER));
    } else {
        if (++s_dcb.ct_try > MAX_TRY) {
            s_dcb.ct_try = 0;
            ResetGSM();
            //s_dcb.flag &= (~(1 << FLAG_CFUN));
        }
        
        s_dcb.ct_wait = MAX_WAIT;
    }
}

static void InitProc_QVER(void)
{
    INT32U len;
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    len = AT_CMD_GetModuleInfo(YX_GetStrmStartPtr(wstrm), YX_GetStrmMaxLen(wstrm));
    AT_SEND_SendCmd(&g_getmoduleinfo_para, YX_GetStrmStartPtr(wstrm), len, Informer_QVER);
}

#if 0
/*******************************************************************
** 函数名:     InitProc_CALM
** 函数描述:   设置来电铃音
** 参数:       无
** 返回:       无
********************************************************************/
static void Informer_CALM(INT8U result)
{
    if (result == AT_SUCCESS) {
        ;
    }
    
    s_dcb.flag &= (~(1 << FLAG_CALM));
}

static void InitProc_CALM(void)
{
    INT32U len;
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    len = AT_CALM(YX_GetStrmStartPtr(wstrm), YX_GetStrmMaxLen(wstrm));
    AT_SEND_SendCmd(&AT_CALM_PARA, YX_GetStrmStartPtr(wstrm), len, Informer_CALM);
}
#endif

/*******************************************************************
** 函数名:     InitProc_CFUN
** 函数描述:   设置cfun
** 参数:       无
** 返回:       无
********************************************************************/
static void Informer_CFUN(INT8U result)
{
    if (result == AT_SUCCESS) {
        s_dcb.ct_try = 0;
        s_dcb.flag &= (~(1 << FLAG_CFUN));
    } else {
        if (++s_dcb.ct_try > MAX_TRY) {
            s_dcb.ct_try = 0;
            ResetGSM();
            //s_dcb.flag &= (~(1 << FLAG_CFUN));
        }
        
        s_dcb.ct_wait = MAX_WAIT;
    }
}

static void InitProc_CFUN(void)
{
    INT32U len;
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    if (s_dcb.cfun == NETWORK_MODE_WORK) {
        len = AT_CFUN(YX_GetStrmStartPtr(wstrm), YX_GetStrmMaxLen(wstrm), '1');
    } else {
        len = AT_CFUN(YX_GetStrmStartPtr(wstrm), YX_GetStrmMaxLen(wstrm), '4');
    }

    AT_SEND_SendCmd(&AT_CFUN_PARA, YX_GetStrmStartPtr(wstrm), len, Informer_CFUN);
}

/*******************************************************************
** 函数名:     InitProc_CSMS
** 函数描述:   设置短信服务模式
** 参数:       无
** 返回:       无
********************************************************************/
static void Informer_CSMS(INT8U result)
{
    if (result == AT_SUCCESS) {
        s_dcb.ct_try = 0;
        s_dcb.flag &= (~(1 << FLAG_CSMS));
    } else {
        if (++s_dcb.ct_try > MAX_TRY) {
            s_dcb.ct_try = 0;
            //ResetGSM();
            s_dcb.flag &= (~(1 << FLAG_CSMS));
        }
        s_dcb.ct_wait = MAX_WAIT;
    }
}

static void InitProc_CSMS(void)
{
    INT32U len;
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    len = AT_CMD_SetMessageService(YX_GetStrmStartPtr(wstrm), YX_GetStrmMaxLen(wstrm));
    AT_SEND_SendCmd(&g_sms_setcsms_para, YX_GetStrmStartPtr(wstrm), len, Informer_CSMS);
}

/*******************************************************************
** 函数名:     InitProc_CMGF
** 函数描述:   设置短信为PDU格式
** 参数:       无
** 返回:       无
********************************************************************/
static void Informer_CMGF(INT8U result)
{
    if (result == AT_SUCCESS) {
        s_dcb.ct_try = 0;
        s_dcb.flag &= (~(1 << FLAG_CMGF));
    } else {
        if (++s_dcb.ct_try > MAX_TRY) {
            s_dcb.ct_try = 0;
            ResetGSM();
            //s_dcb.flag &= (~(1 << FLAG_CMGF));
        }
        s_dcb.ct_wait = MAX_WAIT;
    }
}

static void InitProc_CMGF(void)
{
    INT32U len;
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    len = AT_CMD_SetSmsFormat(YX_GetStrmStartPtr(wstrm), YX_GetStrmMaxLen(wstrm));
    AT_SEND_SendCmd(&g_sms_setformat_para, YX_GetStrmStartPtr(wstrm), len, Informer_CMGF);
}

/*******************************************************************
** 函数名:     InitProc_CNMI
** 函数描述:   设置新短信指示模式
** 参数:       无
** 返回:       无
********************************************************************/
static void Informer_CNMI(INT8U result)
{
    if (result == AT_SUCCESS) {
        s_dcb.ct_try = 0;
        s_dcb.flag &= (~(1 << FLAG_CNMI));
    } else {
        if (++s_dcb.ct_try > MAX_TRY) {
            s_dcb.ct_try = 0;
            ResetGSM();
            //s_dcb.flag &= (~(1 << FLAG_CNMI));
        }
        s_dcb.ct_wait = MAX_WAIT;
    }
}

static void InitProc_CNMI(void)
{
    INT32U len;
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    len = AT_CMD_SetSmsIndication(YX_GetStrmStartPtr(wstrm), YX_GetStrmMaxLen(wstrm));
    AT_SEND_SendCmd(&g_sms_setcnmi_para, YX_GetStrmStartPtr(wstrm), len, Informer_CNMI);
}

/*******************************************************************
** 函数名:     InitProc_CPMS
** 函数描述:   设置新短信存储方式
** 参数:       无
** 返回:       无
********************************************************************/
static void Informer_CPMS(INT8U result)
{
    if (result == AT_SUCCESS) {
        s_dcb.ct_try = 0;
        s_dcb.flag &= (~(1 << FLAG_CPMS));
    } else {
        if (++s_dcb.ct_try > MAX_TRY) {
            s_dcb.ct_try = 0;
            //ResetGSM();
            s_dcb.flag &= (~(1 << FLAG_CPMS));
        }
        s_dcb.ct_wait = MAX_WAIT;
    }
}

static void InitProc_CPMS(void)
{
    INT32U len;
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    len = AT_CMD_SelectSmStorage(YX_GetStrmStartPtr(wstrm), YX_GetStrmMaxLen(wstrm));
    AT_SEND_SendCmd(&g_sms_cpms_para, YX_GetStrmStartPtr(wstrm), len, Informer_CPMS);
}

#if 0
/*******************************************************************
** 函数名:     InitProc_COLP
** 函数描述:   设置拨打电话应答模式
** 参数:       无
** 返回:       无
********************************************************************/
static void Informer_COLP(INT8U result)
{
    if (result == AT_SUCCESS) {
        s_dcb.ct_try = 0;
        s_dcb.flag &= (~(1 << FLAG_COLP));
    } else {
        if (++s_dcb.ct_try > MAX_TRY) {
            s_dcb.ct_try = 0;
            //ResetGSM();
            s_dcb.flag &= (~(1 << FLAG_COLP));
        }
        s_dcb.ct_wait = MAX_WAIT;
    }
}

static void InitProc_COLP(void)
{
    INT32U len;
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    len = AT_CMD_SetOutgoingDisplay(YX_GetStrmStartPtr(wstrm), YX_GetStrmMaxLen(wstrm));
    AT_SEND_SendCmd(&g_phone_out_para, YX_GetStrmStartPtr(wstrm), len, Informer_COLP);
}
#endif

/*******************************************************************
** 函数名:     InitProc_CLIP
** 函数描述:   选择来电显示
** 参数:       无
** 返回:       无
********************************************************************/
static void Informer_CLIP(INT8U result)
{
    if (result == AT_SUCCESS) {
        s_dcb.flag &= (~(1 << FLAG_CLIP));
    } else {
        if (++s_dcb.ct_try > MAX_TRY) {
            s_dcb.ct_try = 0;
            ResetGSM();
            //s_dcb.flag &= (~(1 << FLAG_CLIP));
        }
        s_dcb.ct_wait = MAX_WAIT;
    }
}

static void InitProc_CLIP(void)
{
    INT32U len;
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    len = AT_CMD_SetIncomingDisplay(YX_GetStrmStartPtr(wstrm), YX_GetStrmMaxLen(wstrm));
    AT_SEND_SendCmd(&g_phone_in_para, YX_GetStrmStartPtr(wstrm), len, Informer_CLIP);
}

/*******************************************************************
** 函数名:     InitProc_CLCC
** 函数描述:   电话状态选择
** 参数:       无
** 返回:       无
********************************************************************/
static void Informer_CLCC(INT8U result)
{
    if (result == AT_SUCCESS) {
        s_dcb.flag &= (~(1 << FLAG_CLCC));
    } else {
        if (++s_dcb.ct_try > MAX_TRY) {
            s_dcb.ct_try = 0;
            ResetGSM();
            //s_dcb.flag &= (~(1 << FLAG_CLCC));
        }
        s_dcb.ct_wait = MAX_WAIT;
    }
}

static void InitProc_CLCC(void)
{
    INT32U len;
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    len = AT_CMD_QueryPhoneStatus(YX_GetStrmStartPtr(wstrm), YX_GetStrmMaxLen(wstrm));
    AT_SEND_SendCmd(&g_phone_query_para, YX_GetStrmStartPtr(wstrm), len, Informer_CLCC);
}

#if 0
/*******************************************************************
** 函数名:     InitProc_CPBS
** 函数描述:   选择电话本存储区
** 参数:       无
** 返回:       无
********************************************************************/
static void Informer_CPBS(INT8U result)
{
    s_dcb.flag &= (~(1 << FLAG_CPBS));
}

static void InitProc_CPBS(void)
{
    INT32U len;
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    len = AT_CPBS(YX_GetStrmStartPtr(wstrm), YX_GetStrmMaxLen(wstrm));
    AT_SEND_SendCmd(&AT_CPBS_PARA, YX_GetStrmStartPtr(wstrm), len, Informer_CPBS);
}

/*******************************************************************
** 函数名:     InitProc_QIMEI
** 函数描述:   查询IMEI
** 参数:       无
** 返回:       无
********************************************************************/
static void Informer_QIMEI(INT8U result)
{
    if (result == AT_SUCCESS) {
	    if (YX_ACmpString(true, s_imei.imei, &ATCmdAck.ackbuf[1], s_imei.len, ATCmdAck.ackbuf[0]) == STR_EQUAL) {
            s_dcb.flag &= (~(1 << FLAG_QIMEI));
		} else {                                                               /* 这里为了可靠性的原因，读取2次一致才表示有效 */
		    s_imei.len = ATCmdAck.ackbuf[0];
		    if (s_imei.len > sizeof(s_imei.imei)) {
		        s_imei.len = sizeof(s_imei.imei);
		    }
		    YX_MEMCPY(s_imei.imei, sizeof(s_imei.imei), &ATCmdAck.ackbuf[1], s_imei.len);
		}
	} else {
        if (++s_dcb.ct_try > MAX_TRY) {
            s_dcb.ct_try = 0;
            ResetGSM();
            s_imei.len = 0;
            //s_dcb.flag &= (~(1 << FLAG_QIMEI));
        }
        s_dcb.ct_wait = MAX_WAIT;
    }
}


static void InitProc_QIMEI(void)
{
    INT32U len;
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    len = AT_CMD_GetIMEI(YX_GetStrmStartPtr(wstrm), YX_GetStrmMaxLen(wstrm));
    AT_SEND_SendCmd(&AT_R_IMEI_PARA, YX_GetStrmStartPtr(wstrm), len, Informer_QIMEI);
}

/*******************************************************************
** 函数名:     InitProc_QICCID
** 函数描述:   查询IMEI
** 参数:       无
** 返回:       无
********************************************************************/
static void Informer_QICCID(INT8U result)
{
    if (result == AT_SUCCESS) {
	    if (YX_ACmpString(true, s_iccid.iccid, &ATCmdAck.ackbuf[1], s_iccid.len, ATCmdAck.ackbuf[0]) == STR_EQUAL) {
            s_dcb.flag &= (~(1 << FLAG_QICCID));
		} else {                                                               /* 这里为了可靠性的原因，读取2次一致才表示有效 */
		    s_iccid.len = ATCmdAck.ackbuf[0];
		    if (s_iccid.len > sizeof(s_iccid.iccid)) {
		        s_iccid.len = sizeof(s_iccid.iccid);
		    }
		    YX_MEMCPY(s_iccid.iccid, sizeof(s_iccid.iccid), &ATCmdAck.ackbuf[1], s_iccid.len);
		}
	} else {
        if (++s_dcb.ct_try > MAX_TRY) {
            s_dcb.ct_try = 0;
            ResetGSM();
            s_iccid.len = 0;
            //s_dcb.flag &= (~(1 << FLAG_QICCID));
        }
        s_dcb.ct_wait = MAX_WAIT;
    }
}


static void InitProc_QICCID(void)
{
    INT32U len;
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    len = AT_CMD_GetICCID(YX_GetStrmStartPtr(wstrm), YX_GetStrmMaxLen(wstrm));
    AT_SEND_SendCmd(&AT_R_ICCID_PARA, YX_GetStrmStartPtr(wstrm), len, Informer_QICCID);
}

/*******************************************************************
** 函数名:     InitProc_QIMSI
** 函数描述:   查询IMSI
** 参数:       无
** 返回:       无
********************************************************************/
static void Informer_QIMSI(INT8U result)
{
    if (result == AT_SUCCESS) {
	    if (YX_ACmpString(true, s_imsi.imsi, &ATCmdAck.ackbuf[1], s_imsi.len, ATCmdAck.ackbuf[0]) == STR_EQUAL) {
            s_dcb.flag &= (~(1 << FLAG_QIMSI));
		} else {                                                               /* 这里为了可靠性的原因，读取2次一致才表示有效 */
		    s_imsi.len = ATCmdAck.ackbuf[0];
		    if (s_imsi.len > sizeof(s_imsi.imsi)) {
		        s_imsi.len = sizeof(s_imsi.imsi);
		    }
		    YX_MEMCPY(s_imsi.imsi, sizeof(s_imsi.imsi), &ATCmdAck.ackbuf[1], s_imsi.len);
		}
	} else {
        if (++s_dcb.ct_try > MAX_TRY) {
            s_dcb.ct_try = 0;
            ResetGSM();
            s_imsi.len = 0;
            //s_dcb.flag &= (~(1 << FLAG_QIMSI));
        }
        s_dcb.ct_wait = MAX_WAIT;
    }
}

static void InitProc_QIMSI(void)
{
    INT32U len;
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    len = AT_CMD_GetIMSI(YX_GetStrmStartPtr(wstrm), YX_GetStrmMaxLen(wstrm));
    AT_SEND_SendCmd(&AT_R_CIMI_PARA, YX_GetStrmStartPtr(wstrm), len, Informer_QIMSI);
}
#endif

/*******************************************************************
** 函数名:     Proc_QSIM
** 函数描述:   查询SIM卡状态
** 参数:       无
** 返回:       无
********************************************************************/
static void Informer_QSIM(INT8U result)
{
    if (result == AT_SUCCESS) {
        s_dcb.simstatus = SIM_NORMAL;
	} else if (result == AT_FAILURE) {
	    s_dcb.simstatus = SIM_NOT_INSERTED;
	    s_dcb.netstatus = NETWORK_STATE_NOT_REGISTERED;
	    //s_dcb.csq       = 0;
	}
	
	s_dcb.flag &= (~(1 << FLAG_QSIM));
}

static void Proc_QSIM(void)
{
    INT32U len;
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    len = AT_R_CPIN(YX_GetStrmStartPtr(wstrm), YX_GetStrmMaxLen(wstrm));
    AT_SEND_SendCmd(&AT_R_CPIN_PARA, YX_GetStrmStartPtr(wstrm), len, Informer_QSIM);
    //len = AT_CMD_GetSmsIndication(YX_GetStrmStartPtr(wstrm), YX_GetStrmMaxLen(wstrm));
    //AT_SEND_SendCmd(&g_sms_getcnmi_para, YX_GetStrmStartPtr(wstrm), len, Informer_QSIM);
    //len = AT_CSMINS(YX_GetStrmStartPtr(wstrm), YX_GetStrmMaxLen(wstrm));
    //AT_SEND_SendCmd(&AT_CSMINS_PARA, YX_GetStrmStartPtr(wstrm), len, Informer_QSIM);
    
}

/*******************************************************************
** 函数名:     Proc_QNET
** 函数描述:   查询网络
** 参数:       无
** 返回:       无
********************************************************************/
static void Informer_QNET(INT8U result)
{
    INT8U networkregistration;
    
    if (result == AT_SUCCESS) {
        networkregistration = ATCmdAck.ackbuf[1];
        switch (networkregistration) 
        {
        case NETWORK_HomeNetwork:
            s_dcb.netstatus = NETWORK_STATE_HOME;
            break;
        case NETWORK_Roaming:
            s_dcb.netstatus = NETWORK_STATE_ROAMING;
            break;
        case NETWORK_NotRegistered:
            s_dcb.netstatus = NETWORK_STATE_NOT_REGISTERED;
            //s_dcb.csq = 0;
            break;
        case NETWORK_NotRegistered_Searching:
            s_dcb.netstatus = NETWORK_STATE_SEARCHING;
            //s_dcb.csq = 0;
            break;
        case NETWORK_Denied:
            s_dcb.netstatus = NETWORK_STATE_REG_DENIED;
            //s_dcb.csq = 0;
            break;
        case NETWORK_Unknown:
            s_dcb.netstatus = NETWORK_STATE_UNKNOWN;
            //s_dcb.csq = 0;
            break;
        default:
            s_dcb.netstatus = NETWORK_STATE_UNKNOWN;
            //s_dcb.csq = 0;
            break;
        }
	} else {
	    s_dcb.netstatus = NETWORK_STATE_NOT_REGISTERED;
	}
    s_dcb.flag &= (~(1 << FLAG_QNET));
}

static void Proc_QNET(void)
{
    INT32U len;
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    len = AT_R_CREG(YX_GetStrmStartPtr(wstrm), YX_GetStrmMaxLen(wstrm));
    AT_SEND_SendCmd(&AT_R_CREG_PARA, YX_GetStrmStartPtr(wstrm), len, Informer_QNET);
}

/*******************************************************************
** 函数名:     Proc_QCSQ
** 函数描述:   查询信号
** 参数:       无
** 返回:       无
********************************************************************/
static void Informer_QCSQ(INT8U result)
{
    if (result == AT_SUCCESS) {
        s_dcb.csq = ATCmdAck.ackbuf[0];
        if (s_dcb.csq == 99) {
            s_dcb.csq = 0;
        }
        
        //if (s_dcb.netstatus != NETWORK_STATE_HOME && s_dcb.netstatus != NETWORK_STATE_ROAMING) {
        //    s_dcb.csq = 0;
        //}
	}
	s_dcb.flag &= (~(1 << FLAG_QCSQ));
}

static void Proc_QCSQ(void)
{
    INT32U len;
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    len = AT_CSQ(YX_GetStrmStartPtr(wstrm), YX_GetStrmMaxLen(wstrm));
    AT_SEND_SendCmd(&AT_CSQ_PARA, YX_GetStrmStartPtr(wstrm), len, Informer_QCSQ);
}
/*******************************************************************
** 函数名:     Proc_QOSP
** 函数描述:   查询运营商
** 参数:       无
** 返回:       无
********************************************************************/
static void Informer_QOSP(INT8U result)
{
    if (result == AT_SUCCESS) {
        #if DEBUG_AT > 0
        printf_com("<网络运营商(%d)>\r\n", g_at_ack_info.networkoperator.operatortype);
        #endif
        
        s_dcb.service = g_at_ack_info.networkoperator.operatortype;
    } else {
        //if (++s_dcb.ct_try > MAX_TRY) {
        //    s_dcb.ct_try = 0;
        //    ResetGSM();
            //s_dcb.flag &= (~(1 << FLAG_QOSP));
        //}
        //s_dcb.ct_wait = MAX_WAIT;
    }
    s_dcb.flag &= (~(1 << FLAG_QOSP));
}

static void Proc_QOSP(void)
{
    INT32U len;
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    len = AT_R_COPS(YX_GetStrmStartPtr(wstrm), YX_GetStrmMaxLen(wstrm));
    AT_SEND_SendCmd(&AT_R_COPS_PARA, YX_GetStrmStartPtr(wstrm), len, Informer_QOSP);
}

/*******************************************************************
** 函数名:     Proc_CIPDPDP
** 函数描述:   设置GPRS网络检测时间
** 参数:       无
** 返回:       无
********************************************************************/
static void Informer_CIPDPDP(INT8U result)
{
    if (result == AT_SUCCESS) {
        s_dcb.flag &= (~(1 << FLAG_CIPDPDP));
    } else {
        if (++s_dcb.ct_try > MAX_TRY) {
            s_dcb.ct_try = 0;
            ResetGSM();
            //s_dcb.flag &= (~(1 << FLAG_CIPDPDP));
        }
        s_dcb.ct_wait = MAX_WAIT;
    }
}

static void Proc_CIPDPDP(void)
{
    INT32U len;
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    len = AT_CMD_SetPdpDetect(YX_GetStrmStartPtr(wstrm), YX_GetStrmMaxLen(wstrm));
    AT_SEND_SendCmd(&g_pdp_setdetect_para, YX_GetStrmStartPtr(wstrm), len, Informer_CIPDPDP);
}

/*******************************************************************
** 函数名:     Proc_CIPSPRT
** 函数描述:   设置GPRS网络检测时间
** 参数:       无
** 返回:       无
********************************************************************/
static void Informer_CIPSPRT(INT8U result)
{
    if (result == AT_SUCCESS) {
        s_dcb.flag &= (~(1 << FLAG_CIPSPRT));
    } else {
        if (++s_dcb.ct_try > MAX_TRY) {
            s_dcb.ct_try = 0;
            ResetGSM();
            //s_dcb.flag &= (~(1 << FLAG_CIPSPRT));
        }
        s_dcb.ct_wait = MAX_WAIT;
    }
}

static void Proc_CIPSPRT(void)
{
    INT32U len;
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    len = AT_CMD_SetSocketSendPrompt(YX_GetStrmStartPtr(wstrm), YX_GetStrmMaxLen(wstrm));
    AT_SEND_SendCmd(&g_socket_sendprompt_para, YX_GetStrmStartPtr(wstrm), len, Informer_CIPSPRT);
}

/*******************************************************************
** 函数名:     Proc_CIPQSEND
** 函数描述:   设置GPRS数据传输模式
** 参数:       无
** 返回:       无
********************************************************************/
static void Informer_CIPQSEND(INT8U result)
{
    if (result == AT_SUCCESS) {
        s_dcb.flag &= (~(1 << FLAG_CIPQSEND));
    } else {
        if (++s_dcb.ct_try > MAX_TRY) {
            s_dcb.ct_try = 0;
            ResetGSM();
            //s_dcb.flag &= (~(1 << FLAG_CIPQSEND));
        }
        s_dcb.ct_wait = MAX_WAIT;
    }
}

static void Proc_CIPQSEND(void)
{
    INT32U len;
    STREAM_T *wstrm;
    
    wstrm = YX_STREAM_GetBufferStream();
    len = AT_CMD_SetSocketSendStyle(YX_GetStrmStartPtr(wstrm), YX_GetStrmMaxLen(wstrm));
    AT_SEND_SendCmd(&g_socket_sendstyle_para, YX_GetStrmStartPtr(wstrm), len, Informer_CIPQSEND);
}

/*******************************************************************
** 函数名:     Proc_init
** 函数描述:   AT命令发送
** 参数:       无
** 返回:       无
********************************************************************/
static void Proc_init(void)
{
    if ((s_dcb.status & _OPEN) == 0 || (s_dcb.ct_wait > 0) || !AT_SEND_CanSendATCmd()) {
        return;
    }
    /* 退出当前AT指令 */
    if ((s_dcb.flag & (1 << FLAG_ESC)) != 0) {
        Proc_ESC();
        return;
    }
    /* 测试指令 */
    if ((s_dcb.flag & (1 << FLAG_TEST)) != 0) {
        InitProc_TEST();
        return;
    }

    /* 查询模块版本号 */
    if ((s_dcb.flag & (1 << FLAG_QVER)) != 0) {
        InitProc_QVER();
        return;
    }
#if 0
    /* 查询ICCID */
    if ((s_dcb.flag & (1 << FLAG_QICCID)) != 0) {
        InitProc_QICCID();
        return;
    }
    /* 设置PIN码功能 */
    if ((s_dcb.flag & (1 << FLAG_CLCK)) != 0) {
        InitProc_CLCK();
        return;
    }
    
    /* 修改PIN密码 */
    if ((s_dcb.flag & (1 << FLAG_CPWD)) != 0) {
        InitProc_CPWD();
        return;
    }
    
    /* 解锁PIN码功能 */
    if ((s_dcb.flag & (1 << FLAG_CPIN)) != 0) {
        InitProc_CPIN();
        return;
    }
    
    if (s_dcb.islock != 0) {                                                   /* 需要输入pin码，其他指令都无法执行 */
        return;
    }
    /* 设置关闭来电铃声 */
    if ((s_dcb.flag & (1 << FLAG_CALM)) != 0) {
        InitProc_CALM();
        return;
    }
#endif

    /* 设置DTR */
    if ((s_dcb.flag & (1 << FLAG_SETDTR)) != 0) {
        InitProc_SETDTR();
        return;
    }
    /* 设置网络 */
    if ((s_dcb.flag & (1 << FLAG_CFUN)) != 0) {
        InitProc_CFUN();
        return;
    }
    /* 查询sim卡 */
    if ((s_dcb.flag & (1 << FLAG_QSIM)) != 0) {
        Proc_QSIM();
        return;
    }
    /* 查询网络状态 */
    if ((s_dcb.flag & (1 << FLAG_QNET)) != 0) {
        Proc_QNET();
        return;
    }
    /* 查询信号强度 */
    if ((s_dcb.flag & (1 << FLAG_QCSQ)) != 0) {
        Proc_QCSQ();
        return;
    }
    /* 查询网络运营商 */
    if ((s_dcb.flag & (1 << FLAG_QOSP)) != 0) {
        Proc_QOSP();
        return;
    }

    /* 设置GPRS网络检测时间 */
    if ((s_dcb.flag & (1 << FLAG_CIPDPDP)) != 0) {
        Proc_CIPDPDP();
        return;
    }
    /* 设置GPRS回行符 */
    if ((s_dcb.flag & (1 << FLAG_CIPSPRT)) != 0) {
        Proc_CIPSPRT();
        return;
    }
    
    /* 设置GPRS数据传输模式 */
    if ((s_dcb.flag & (1 << FLAG_CIPQSEND)) != 0) {
        Proc_CIPQSEND();
        return;
    }
    
    /* 设置短信存储方式 */
    if ((s_dcb.flag & (1 << FLAG_CPMS)) != 0) {
        InitProc_CPMS();
        return;
    }
    
    if ((s_dcb.flag & (1 << FLAG_CSMS)) != 0) {
        InitProc_CSMS();
        return;
    }
    
    if ((s_dcb.flag & (1 << FLAG_CMGF)) != 0) {
        InitProc_CMGF();
        return;
    }
    
    if ((s_dcb.flag & (1 << FLAG_CNMI)) != 0) {
        InitProc_CNMI();
        return;
    }
    
#if 0
    if ((s_dcb.flag & (1 << FLAG_COLP)) != 0) {
        InitProc_COLP();
        return;
    }
#endif
    
    if ((s_dcb.flag & (1 << FLAG_CLIP)) != 0) {
        InitProc_CLIP();
        return;
    }
    
    if ((s_dcb.flag & (1 << FLAG_CLCC)) != 0) {
        InitProc_CLCC();
        return;
    }
    
#if 0
    if ((s_dcb.flag & (1 << FLAG_CPBS)) != 0) {
        InitProc_CPBS();
        return;
    }
    /* 查询IMEI */
    if ((s_dcb.flag & (1 << FLAG_QIMEI)) != 0) {
        InitProc_QIMEI();
        return;
    }
    
    /* 查询IMSI */
    if ((s_dcb.flag & (1 << FLAG_QIMSI)) != 0) {
        InitProc_QIMSI();
        return;
    }
#endif
}

#if 0
static void Informer_acksm(INT8U result)
{
    result = result;
    s_dcb.flag &= (~_ACKSM);
}

static void Proc_acksm(void)
{
    INT32U len;
    
    if (!AT_SEND_CanSendATCmd()) {
        return;
    }
    len = AT_CMD_SetSmsAck(YX_GetStrmStartPtr(wstrm), YX_GetStrmMaxLen(wstrm));
    AT_SEND_SendCmd(&g_sms_setcnma_para, YX_GetStrmStartPtr(wstrm), len, Informer_acksm);
}
#endif



/*******************************************************************
** 函数名:     WaitTmrProc
** 函数描述:   初始化延时定时器
** 参数:       [in] pdata:定时器特征值
** 返回:       无
********************************************************************/
static void WaitTmrProc(void *pdata)
{
    if (s_dcb.ct_wait > 0) {
        s_dcb.ct_wait--;
    }
}

/*******************************************************************
** 函数名:     AT_CORE_Init
** 函数描述:   模块初始化
** 参数:       无
** 返回:       无
********************************************************************/
void AT_CORE_Init(void)
{
    YX_MEMSET(&s_dcb, 0, sizeof(s_dcb));
    
    s_dcb.cfun = NETWORK_MODE_WORK;
    //s_dcb.flag = FLAG_MASK;
    
    s_waittmr = OS_CreateTmr(TSK_ID_DAL, 0, WaitTmrProc);
    OS_StartTmr(s_waittmr, PERIOD_SCAN);
}

/*******************************************************************
** 函数名:     AT_CORE_Open
** 函数描述:   打开模块初始化功能
** 参数:       无
** 返回:       无
********************************************************************/
void AT_CORE_Open(void)
{
    if ((s_dcb.status & _OPEN) == 0) {
        s_dcb.status = _OPEN;

        ResetGSM();
    }
}

/*******************************************************************
** 函数名:     AT_CORE_Close
** 函数描述:   关闭模块初始化功能
** 参数:       无
** 返回:       无
********************************************************************/
void AT_CORE_Close(void)
{
    if ((s_dcb.status & _OPEN) != 0) {
        s_dcb.status = 0;
        s_dcb.simstatus = SIM_NOT_INSERTED;
        s_dcb.netstatus = NETWORK_STATE_NOT_REGISTERED;
        s_dcb.csq = 0;
        s_dcb.cfun = NETWORK_MODE_WORK;
        s_dcb.flag = 0;
    }
}

/*******************************************************************
** 函数名:     AT_CORE_SendEntry
** 函数描述:   AT发送入口
** 参数:       无
** 返回:       无
********************************************************************/
void AT_CORE_SendEntry(void)
{
    if (s_dcb.flag != 0) {
        Proc_init();
    }
    
#if 0
    if (s_dcb.flag & _ACKSM) {
        Proc_acksm();
    }
#endif
}

/*******************************************************************
** 函数名:     AT_CORE_HaveHighTask
** 函数描述:   是否AT需要发送
** 参数:       无
** 返回:       有返回true，否则返回false
********************************************************************/
BOOLEAN AT_CORE_HaveHighTask(void)
{
    if ((s_dcb.flag & (FLAG_TEST | FLAG_ESC)) != 0) {
        return true;
    } else {
        return false;
    }
}

/*******************************************************************
** 函数名:     AT_CORE_HaveLowTask
** 函数描述:   是否AT需要发送
** 参数:       无
** 返回:       有返回true，否则返回false
********************************************************************/
BOOLEAN AT_CORE_HaveLowTask(void)
{
    if ((s_dcb.flag & FLAG_MASK) != 0) {
        return true;
    } else {
        return false;
    }
}

/*******************************************************************
** 函数名:     AT_CORE_RecoveryATCmd
** 函数描述:   AT测试
** 参数:       无
** 返回:       无
********************************************************************/
void AT_CORE_RecoveryATCmd(void)
{
    if ((s_dcb.flag & (1 << FLAG_TEST)) == 0) {
        s_dcb.ct_recovery = 0;
        s_dcb.flag |= (1 << FLAG_TEST);
    }
}

/*******************************************************************
** 函数名:     AT_CORE_EscapeATCmd
** 函数描述:   退出当前AT指令
** 参数:       无
** 返回:       无
********************************************************************/
void AT_CORE_EscapeATCmd(void)
{
    s_dcb.flag |= (1 << FLAG_ESC);
    YX_SEND_AbandonATCmd();
}

#if 0
/*******************************************************************
** 函数名:     ADP_NET_GetIMEI
** 函数描述:   获取IMEI号
** 参数:       [in] ptr：   缓存指针
**             [in] maxlen：缓存最大长度
** 返回:       返回IMEI长度，失败返回0
********************************************************************/
INT8U ADP_NET_GetIMEI(INT8U *ptr, INT16U maxlen)
{
    if (s_imei.len > maxlen) {
        return 0;
    } else {
        YX_MEMCPY(ptr, maxlen, s_imei.imei, s_imei.len);
        return s_imei.len;
    }
}

/*******************************************************************
** 函数名:     ADP_NET_GetIMSI
** 函数描述:   获取IMSI号
** 参数:       [in] ptr：   缓存指针
**             [in] maxlen：缓存最大长度
** 返回:       返回IMSI长度，失败返回0
********************************************************************/
INT8U ADP_NET_GetIMSI(INT8U *ptr, INT16U maxlen)
{
    if (s_imsi.len > maxlen) {
        return 0;
    } else {
        YX_MEMCPY(ptr, maxlen, s_imsi.imsi, s_imsi.len);
        return s_imsi.len;
    }
}

/*******************************************************************
** 函数名:     ADP_NET_GetICCID
** 函数描述:   获取SIM卡的ICCID号
** 参数:       [in] ptr：   缓存指针
**             [in] maxlen：缓存最大长度
** 返回:       返回IMSI长度，失败返回0
********************************************************************/
INT8U ADP_NET_GetICCID(INT8U *ptr, INT16U maxlen)
{
    if (s_iccid.len > maxlen) {
        return 0;
    } else {
        YX_MEMCPY(ptr, maxlen, s_iccid.iccid, s_iccid.len);
        return s_iccid.len;
    }
}

/*******************************************************************
** 函数名:     ADP_NET_SetNetworkMode
** 函数描述:   设置网络开关功能
** 参数:       [in] mode:网络模式,见NETWORK_MODE_E
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN ADP_NET_SetNetworkMode(INT8U mode)
{
    if (s_dcb.cfun != mode) {
        s_dcb.cfun = mode;
        s_dcb.flag |= (1 << FLAG_CFUN);
    }
    return true;
}

/*******************************************************************
** 函数名:     ADP_NET_SetSimPinFunction
** 函数描述:   设置pin码功能
** 参数:       [in] onoff:   true-打开pin码功能，false-关掉pin码功能
**             [in] password:PIN密码，以'\0'为结束符,密码为4位
**             [in] fp:      设置结果回调，result：true-设置成功，false-设置失败
** 返回:       调用成功返回true，失败返回false，调用失败时无fp回调
********************************************************************/
BOOLEAN ADP_NET_SetSimPinFuction(INT8U onoff, char *password, void (*fp)(INT8U result))
{
    INT32U len;
    
    len = YX_STRLEN(password);
    if (len >= sizeof(s_dcb.password) || len != 4) {
        return false;
    }
    
    if (s_dcb.simlock != onoff) {
        s_dcb.inform_setpinfunction = fp;
        s_dcb.simlock = onoff;
        s_dcb.flag |= (1 << FLAG_CLCK);
        YX_STRCPY(s_dcb.password, password);
        return true;
    } else {
        return false;
    }
}

/*******************************************************************
** 函数名:     ADP_NET_ModifyPinPassword
** 函数描述:   修改pin密码，必须在已开启pin码功能的情况下
** 参数:       [in] opassword:旧PIN密码，以'\0'为结束符,密码为4位
**             [in] npassword:新PIN密码，以'\0'为结束符,密码为4位
**             [in] fp:      设置结果回调，result：true-设置成功，false-设置失败
** 返回:       调用成功返回true，失败返回false，调用失败时无fp回调
********************************************************************/
BOOLEAN ADP_NET_ModifyPinPassword(char *opassword, char *npassword, void (*fp)(INT8U result))
{
    INT8U len1, len2;
    
    len1 = YX_STRLEN(opassword);
    len2 = YX_STRLEN(npassword);
    
    if (len1 >= sizeof(s_dcb.opassword) || len1 != 4) {
        return false;
    }
    
    if (len2 >= sizeof(s_dcb.npassword) || len2 != 4) {
        return false;
    }
    
    if (s_dcb.simlock) {                                                       /* 在已开启pin码功能的前提下 */
        s_dcb.inform_modifypinpassword = fp;
        s_dcb.flag |= (1 << FLAG_CPWD);
        YX_STRCPY(s_dcb.opassword, opassword);
        YX_STRCPY(s_dcb.npassword, npassword);
        return true;
    } else {
        return false;
    }
}

/*******************************************************************
** 函数名:     ADP_NET_UnlockSimPinCode
** 函数描述:   解SIM卡pin码
** 参数:       [in] password:PIN密码，以'\0'为结束符,密码为4位
**             [in] fp:      解码结果回调，result：true-解码成功，false-解码失败
** 返回:       调用成功返回true，失败返回false，调用失败时无fp回调
********************************************************************/
BOOLEAN ADP_NET_UnlockSimPinCode(char *password, void (*fp)(INT8U result))
{
    INT32U len;
    
    len = YX_STRLEN(password);
    if (len >= sizeof(s_dcb.password) || len != 4) {
        return false;
    }
    
    s_dcb.inform_unlockpin = fp;
    s_dcb.flag |= (1 << FLAG_CPIN);
    YX_STRCPY(s_dcb.password, password);
    return true;
}

/*******************************************************************
** 函数名:     ADP_NET_SimPinFuctionIsOn
** 函数描述:   pin码功能是否已开启
** 参数:       无
** 返回:       开启返回true，未开启返回false
********************************************************************/
BOOLEAN ADP_NET_SimPinFuctionIsOn(void)
{
    return s_dcb.simlock;
}

/*******************************************************************
** 函数名:     ADP_NET_SimPinCodeIsLock
** 函数描述:   pin码是否未解锁
** 参数:       无
** 返回:       是返回true，否返回false
********************************************************************/
BOOLEAN ADP_NET_SimPinCodeIsLock(void)
{
    return s_dcb.islock;
}

/*******************************************************************
** 函数名:     ADP_NET_InformSimPinCodeStatus
** 函数描述:   通知当前PIN码状态
** 参数:       [in] lock:   true-pin码已锁，false-无pin码功能
** 返回:       是返回true，否返回false
********************************************************************/
BOOLEAN ADP_NET_InformSimPinCodeStatus(INT8U lock)
{
    if (lock) {                                                                /* pin码锁住需要解锁 */
        s_dcb.islock = lock;
        s_dcb.simlock = lock;
    } else {                                                                   /* pin码已解锁 */
        s_dcb.islock = lock;
    }
    return true;
}
#endif

/*******************************************************************
** 函数名:     ADP_NET_GetNetworkState
** 函数描述:   获取网络状态
** 参数:       [out] simcard: sim卡状态，见SIM_STATE_E
**             [out] creg:    网络注册状态，见NETWORK_STATE_E
**             [out] cgreg：  网络注册状态，见NETWORK_STATE_E
**             [out] rssi：   接收的信号强度指示，单位：dBm.
**             [out] ber：    位错误率Bit error rate，单位：dBm.
** 返回:       成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN ADP_NET_GetNetworkState(INT8U *simcard, INT8U *creg, INT8U *cgreg, INT8U *rssi, INT8U *ber)
{
    s_dcb.flag |= ((1 << FLAG_QSIM) | (1 << FLAG_QNET) | (1 << FLAG_QCSQ) | (1 << FLAG_QOSP));
    
    *simcard = s_dcb.simstatus;
    *creg    = s_dcb.netstatus;
    if ((s_dcb.netstatus == NETWORK_STATE_HOME) || (s_dcb.netstatus == NETWORK_STATE_ROAMING)) {
        *cgreg   = NETWORK_STATE_REGISTERED;
    } else {
        *cgreg   = NETWORK_STATE_NOT_REGISTERED;
    }
    *ber     = 0;
    *rssi    = s_dcb.csq;
    
    return true;
}

/*******************************************************************
** 函数名:     ADP_NET_GetOperator
** 函数描述:   获取网络运营商
** 参数:       无
** 返回:       运营商id,见 NETWORK_OP_E
********************************************************************/
INT8U ADP_NET_GetOperator(void)
{
    return s_dcb.service;
}

/*******************************************************************
** 函数名:     ADP_NET_GetModuleType
** 函数描述:   获取模块型号
** 参数:       无
** 返回:       返回模块型号,见 MODULE_TYPE_E
********************************************************************/
INT8U ADP_NET_GetModuleType(void)
{
    return s_dcb.moduletype;
}

#endif









