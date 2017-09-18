/********************************************************************************
**
** 文件名:     yx_sm_cmd.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现短信命令解析处理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2011/03/09 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "yx_include.h"
#include "yx_misc.h"
#include "yx_stream.h"
#include "dal_pp_drv.h"
#include "at_drv.h"
#include "yx_sleep.h"

/*
********************************************************************************
* 定义模块配置参数
********************************************************************************
*/
#define LEN_CMD_HEAD                    2
#define LEN_CMD_STR                     4


/*
********************************************************************************
* 定义短消息业务的请求数据结构
********************************************************************************
*/
typedef struct {
	char *sstr;
	void (*entryproc)(SM_T *recvsm, INT8U *dataptr, INT8U datalen);
} FUNCSTRENTRY_T;

/*
********************************************************************************
* 定义模块局部变量
********************************************************************************
*/




/*******************************************************************
** 函数名:     HdlCmd_COMMON
** 函数描述:   普通指令
** 参数:       无
** 返回:       无
********************************************************************/
static void HdlCmd_COMMON(SM_T *recvsm, INT8U *dataptr, INT8U datalen)
{
    YX_SLEEP_Wakeup(20, WAKEUP_EVENT_SM);
}

/*
********************************************************************************
* Handler:  HdlCmd_SZMM     设置短消息命令密码
********************************************************************************
*/
static void HdlCmd_SZMM(SM_T *recvsm, INT8U *dataptr, INT8U datalen)
{
    INT8U i;
    SMS_PASSWORD_T smpassword;

    if (datalen != LEN_PASSWORD) {                                             /* 密码长度不正确 */
        return;
    }
    
    for (i = 0; i < datalen; i++) {                                            /* 密码必须由数字组成 */
        if (dataptr[i] < '0' || dataptr[i] > '9') {
            return;
        }
    }
    YX_MEMCPY((INT8U *)smpassword.password, sizeof(smpassword.password), dataptr, datalen);
    DAL_PP_StoreParaByID(PP_ID_SMSPSW, (INT8U *)&smpassword, sizeof(smpassword));
    YX_SLEEP_Wakeup(20, WAKEUP_EVENT_SM);
}

/*
********************************************************************************
* Handler:  HdlCmd_WDCS     配置无线下载参数
********************************************************************************
*/
static void HdlCmd_WDCS(SM_T *recvsm, INT8U *dataptr, INT8U datalen)
{
    YX_SLEEP_Wakeup(40, WAKEUP_EVENT_SM);
}

/*******************************************************************
** 函数名:     HdlCmd_REST
** 函数描述:   复位车台
** 参数:       无
** 返回:       无
********************************************************************/
static void HdlCmd_REST(SM_T *recvsm, INT8U *dataptr, INT8U datalen)
{
    OS_RESET(RESET_EVENT_INITIATE);
}

static FUNCSTRENTRY_T s_functionentry[] = {
     "REST",             HdlCmd_REST    // 复位车台
    ,"SZMM",             HdlCmd_SZMM    // 设置短信命令密码
    ,"WDCS",             HdlCmd_WDCS    // 配置无线下载参数
    ,"WDTR",             HdlCmd_WDCS    // 启动TR无线下载
    ,"WDND",             HdlCmd_WDCS    // 启动120ND无线下载
};

/*******************************************************************
** 函数名:     YX_DetectSmsCmd
** 函数描述:   检测短信命令
** 参数:       [in]:recvsm:短信结构体
** 返回:       解析成功返回true，失败返回false
********************************************************************/
BOOLEAN YX_DetectSmsCmd(SM_T *recvsm)
{
    INT8U *dptr, i, datalen;
    INT8U *dataptr;
    SMS_PASSWORD_T smpassword;
    char super_psw[] = {"000296"};

    if (recvsm->udlen < LEN_CMD_HEAD + LEN_PASSWORD + LEN_CMD_STR) {
        return FALSE;
    }
    dptr = recvsm->ud;
    if ((*dptr++ != '9') || (*dptr++ != '9')) {                                /* 短消息体必须以99开头 */
        return FALSE;
    }
    
    if (YX_MEMCMP(dptr, super_psw, LEN_PASSWORD) != 0) {                       /* 通过密码校验 */
        if (DAL_PP_ReadParaByID(PP_ID_SMSPSW, (INT8U *)&smpassword, sizeof(smpassword))) {
            if (YX_MEMCMP(dptr, (INT8U *)smpassword.password, LEN_PASSWORD) != 0) {
                return FALSE;
            }
        } else {
            return FALSE;
        }
    }
    dptr += LEN_PASSWORD;
    dataptr = dptr + LEN_CMD_STR;
    datalen = recvsm->udlen - LEN_CMD_HEAD - LEN_PASSWORD - LEN_CMD_STR;
    for (i = 0; i < datalen; i++) {                                            /* 兼容以'!'作为命令的结束标志,防止短信尾部被多加很多不可见字符 */
        if (dataptr[i] == '!') {
            datalen = i;
        }
    }
	
	for (i = 0; i < sizeof(s_functionentry) / sizeof(s_functionentry[0]); i++) {/* 查表 */
		if (YX_STRLEN(s_functionentry[i].sstr) != LEN_CMD_STR) {
		    continue;
		}
		
		if (YX_ACmpString(!CASESENSITIVE, dptr, (INT8U *)s_functionentry[i].sstr, LEN_CMD_STR, LEN_CMD_STR) == STR_EQUAL) {
			if (s_functionentry[i].entryproc != 0) {
				s_functionentry[i].entryproc(recvsm, dataptr, datalen);
				return TRUE;
			}
		}
	}
	
	HdlCmd_COMMON(recvsm, dataptr, datalen);
	return true;
}
