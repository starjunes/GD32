/********************************************************************************
**
** 文件名:     at_cmd_common.h
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现手机模块AT指令组帧和解析
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2014/06/15 | 叶德焰 |  创建第一版本
*********************************************************************************/
#ifndef AT_CMD_COMMON_H
#define AT_CMD_COMMON_H      1



#ifndef ATCMD_C
#define _ATCMD_EXT                      extern
#else
#define _ATCMD_EXT
#endif


/*
********************************************************************************
* define config parameters
********************************************************************************
*/
#define SIZE_ATCMDACK                   64

/*
********************************************************************************
* define command execution result
********************************************************************************
*/
#define AT_SUCCESS                      0
#define AT_FAILURE                      1
#define AT_OVERTIME                     2
#define AT_ABANDON                      3
#define AT_CONTINUE                     0xff

/*
********************************************************************************
* define command status
********************************************************************************
*/
#define ATCMD_EXIST                     0x01
#define ATCMD_READY                     0x02
#define ATCMD_SENDING                   0x04
#define ATCMD_BREAK                     0x08
#define ATCMD_INSANT                    0x20
#define ATCMD_WAIT                      0x80

/*
********************************************************************************
* define ATCMDACK_STRUCT
********************************************************************************
*/
typedef struct {
    INT8U       numEC;
    INT8U       ackbuf[SIZE_ATCMDACK];
} ATCMDACK_STRUCT;

/* 本地IP地址信息 */
typedef struct {
    char ip[20];
} LOCAL_IP_T;

/* 模块版本信息 */
typedef struct {
    INT8U moduletype;        /* 模块型号,见 MODULE_TYPE_E */
} MODULE_VERSION_T;

/* 电信运营商 */
typedef struct {
    INT8U operatortype;      /* 网络运营商,见 NETWORK_OP_E */
} NETWORK_OP_T;

typedef union {
    LOCAL_IP_T localip;
    MODULE_VERSION_T moduleversion;
    NETWORK_OP_T networkoperator;
} AT_CMD_ACK_INFO_U;

/*
********************************************************************************
* define AT_CMD_PARA_T
********************************************************************************
*/
typedef struct {
    INT8U status;
    INT8U overtime;                   /* 溢出时间，单位：秒 */
    INT8U nsEC;
    INT8U naEC;
    INT8U (*handler)(INT8U *ptr, INT16U len);
} AT_CMD_PARA_T;

_ATCMD_EXT ATCMDACK_STRUCT ATCmdAck;
_ATCMD_EXT AT_CMD_ACK_INFO_U g_at_ack_info;


#include "at_cmd_tcpip.h"
#include "at_cmd_phone.h"
#include "at_cmd_sms.h"

/*********************************************************************************
**                                                                               *
**                                                                               *
**                 AT Commands parameters                                        *
**                                                                               *
**                                                                               *
*********************************************************************************/

/*
********************************************************************************
* Standard V.25ter AT Commands
********************************************************************************
*/
extern AT_CMD_PARA_T const g_at_test_para;
extern AT_CMD_PARA_T const g_getmoduleinfo_para;
extern AT_CMD_PARA_T const AT_CALM_PARA;
extern AT_CMD_PARA_T const AT_AND_D_PARA;
extern AT_CMD_PARA_T const AT_IPR_PARA;

/*
********************************************************************************
* AT Commands originating from GSM 07.07
********************************************************************************
*/
extern AT_CMD_PARA_T const AT_CFUN_PARA;
extern AT_CMD_PARA_T const AT_CSQ_PARA;
extern AT_CMD_PARA_T const AT_R_CREG_PARA;
extern AT_CMD_PARA_T const AT_R_COPS_PARA;
extern AT_CMD_PARA_T const AT_CPBS_PARA;
extern AT_CMD_PARA_T const AT_CPBW_PARA;
extern AT_CMD_PARA_T const AT_CPBR_PARA;
extern AT_CMD_PARA_T const AT_CCLK_PARA;
extern AT_CMD_PARA_T const AT_R_CCLK_PARA;
extern AT_CMD_PARA_T const AT_R_CIMI_PARA;
extern AT_CMD_PARA_T const AT_R_IMEI_PARA;
extern AT_CMD_PARA_T const AT_IMEI_PARA;
extern AT_CMD_PARA_T const AT_R_ICCID_PARA;
extern AT_CMD_PARA_T const AT_CPIN_PARA;
extern AT_CMD_PARA_T const AT_R_CPIN_PARA;
extern AT_CMD_PARA_T const AT_CLCK_PARA;
extern AT_CMD_PARA_T const AT_CPWD_PARA;


/*
********************************************************************************
* AT Commands originating from GSM 07.05 for SMS
********************************************************************************
*/
extern AT_CMD_PARA_T const AT_ESCAPE_PARA;

/*
********************************************************************************
* defined AT Commands for enhanced functions
********************************************************************************
*/
#if GSM_TYPE == GSM_SIMCOM
extern AT_CMD_PARA_T const AT_CSMINS_PARA;
extern AT_CMD_PARA_T const AT_CRSL_PARA;
extern AT_CMD_PARA_T const AT_QSCLK_PARA;
extern AT_CMD_PARA_T const AT_SET_QENG_PARA;
extern AT_CMD_PARA_T const AT_Q_QENG_PARA;
extern AT_CMD_PARA_T const AT_CTTS_PARA;
extern AT_CMD_PARA_T const AT_CTTSPARAM_PARA;
#endif          /* end of GSM_SIMCOM */



/*********************************************************************************
**                                                                               *
**                                                                               *
**                 Standard V.25ter AT Commands                                  *
**                                                                               *
**                                                                               *
*********************************************************************************/
INT8U   AT_CMD_Test(INT8U *dptr, INT32U maxlen);
INT8U   AT_CMD_GetModuleInfo(INT8U *dptr, INT32U maxlen);
INT8U   AT_CALM(INT8U *dptr, INT32U maxlen);
INT8U   AT_AND_D(INT8U *dptr, INT32U maxlen);
INT8U   AT_IPR(INT8U *dptr, INT32U maxlen, char *rate);


/*********************************************************************************
**                                                                               *
**                                                                               *
**                 AT Commands originating from GSM 07.07                        *
**                                                                               *
**                                                                               *
*********************************************************************************/
INT8U   AT_CFUN(INT8U *dptr, INT32U maxlen, INT8U fun);
INT8U   AT_CSQ(INT8U *dptr, INT32U maxlen);
INT8U   AT_R_CREG(INT8U *dptr, INT32U maxlen);
INT8U   AT_R_COPS(INT8U *dptr, INT32U maxlen);
INT8U   AT_CPBS(INT8U *dptr, INT32U maxlen);
INT8U   AT_CPBR(INT8U *dptr, INT32U maxlen, INT8U index1, INT8U index2);
INT8U   AT_CPBW(INT8U *dptr, INT32U maxlen, INT8U index, INT8U *tel, INT8U tellen, INT8U *text, INT8U textlen);
INT8U   AT_CCLK(INT8U *dptr, INT32U maxlen, DATE_T  *date, TIME_T  *time);
INT8U   AT_R_CCLK(INT8U *dptr, INT32U maxlen);
INT8U   AT_CMD_GetIMSI(INT8U *dptr, INT32U maxlen);
INT8U   AT_CMD_GetIMEI(INT8U *dptr, INT32U maxlen);
INT8U   AT_IMEI(INT8U *dptr, INT32U maxlen, INT8U *imei, INT8U imeilen);
INT8U   AT_CMD_GetICCID(INT8U *dptr, INT32U maxlen);
INT8U   AT_CPIN(INT8U *dptr, INT32U maxlen, char *password);
INT8U   AT_R_CPIN(INT8U *dptr, INT32U maxlen);
INT8U   AT_CLCK(INT8U *dptr, INT32U maxlen, INT8U lock, char *password);
INT8U   AT_CPWD(INT8U *dptr, INT32U maxlen, char *opassword, char *npassword);

/*********************************************************************************
**                                                                               *
**                                                                               *
**                 AT Commands originating from GSM 07.05 for SMS                *
**                                                                               *
**                                                                               *
*********************************************************************************/
INT8U   AT_ESCAPE(INT8U *dptr, INT32U maxlen);



/*********************************************************************************
**                                                                               *
**                                                                               *
**                      defined AT Commands for enhanced functions               *
**                                                                               *
**                                                                               *
*********************************************************************************/
#if GSM_TYPE == GSM_SIMCOM
INT8U   AT_CSMINS(INT8U *dptr, INT32U maxlen);                                            /* 查询SIM卡是否已插入 */
INT8U   AT_CRSL(INT8U *dptr, INT32U maxlen);                                              /* 调节来电铃声音量 */

INT8U   AT_QSCLK(INT8U *dptr, INT32U maxlen, INT8U enable);                               /* 省电模式设置 */
INT8U   AT_SET_QENG(INT8U *dptr, INT32U maxlen, INT8U enable);                             /* AT+QENG, 设置基站信息 */
INT8U   AT_Q_QENG(INT8U *dptr, INT32U maxlen);                                             /* AT+QENG, 查询基站信息 */
INT16U AT_CTTS(INT8U *dptr, INT32U maxlen, INT8U *sptr, INT16U len);
INT16U AT_CTTSPARAM(INT8U *dptr, INT32U maxlen, INT8U volume);
#endif              /* end of GSM_SIMCOM */

#endif
