/********************************************************************************
**
** 文件名:     at_cmd_common.c
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
#define  ATCMD_C                                1

#include "yx_include.h"
#include "yx_misc.h"
#include "yx_stream.h"
#include "at_drv.h"
#include "at_q_phone.h"
#include "at_socket_drv.h"

#if EN_AT > 0



/*********************************************************************************
**                                                                               *
**                                                                               *
**                 AT Commands handler function                                  *
**                                                                               *
**                                                                               *
*********************************************************************************/

/*
********************************************************************************
* Handler:  Common AT Commands
********************************************************************************
*/
static INT8U Handler_Common(INT8U *recvbuf, INT16U len)
{
    if (YX_SearchKeyWord(recvbuf, len, "OK")) {
        return AT_SUCCESS;
    } else {
        return AT_FAILURE;
    }
}

/*
********************************************************************************
* Handler:  ATI
********************************************************************************
*/
static INT8U Handler_AT_ATI(INT8U *recvbuf, INT16U len)
{
    if (YX_SearchKeyWord(recvbuf, len, "ERROR")) {
        return AT_FAILURE;
    }
    
    if (YX_SearchKeyWord(recvbuf, len, "SIM7100CE")) {
        g_at_ack_info.moduleversion.moduletype = MODULE_TYPE_SIM7100CE;
        return AT_SUCCESS;
    }else if (YX_SearchKeyWord(recvbuf, len, "SIM7100C")) {
        g_at_ack_info.moduleversion.moduletype = MODULE_TYPE_SIM7100C;
        return AT_SUCCESS;
    } else if (YX_SearchKeyWord(recvbuf, len, "SIM6320")) {
        g_at_ack_info.moduleversion.moduletype = MODULE_TYPE_SIM6320;
        return AT_SUCCESS;
    } else if (YX_SearchKeyWord(recvbuf, len, "SIM5360")) {
        g_at_ack_info.moduleversion.moduletype = MODULE_TYPE_SIM5360;
        return AT_SUCCESS;
    } else if (YX_SearchKeyWord(recvbuf, len, "M12")) {
        g_at_ack_info.moduleversion.moduletype = MODULE_TYPE_M12;
        return AT_SUCCESS;
    } else if (YX_SearchKeyWord(recvbuf, len, "OK")) {
        g_at_ack_info.moduleversion.moduletype = MODULE_TYPE_MAX;
        return AT_SUCCESS;
    }
    return AT_CONTINUE;
}

/*
********************************************************************************
* Handler:  AT+CSQ ------ Signal Quality
********************************************************************************
*/
static INT8U Handler_AT_CSQ(INT8U *recvbuf, INT16U len)
{
    if (YX_SearchKeyWord(recvbuf, len, "+CSQ")) {
        ATCmdAck.ackbuf[0] = YX_SearchDigitalString(recvbuf, len, ',',  1);
        ATCmdAck.ackbuf[1] = YX_SearchDigitalString(recvbuf, len, '\r', 1);
        return AT_SUCCESS;
    } else {
        return AT_FAILURE;
    }
}

/*
********************************************************************************
* Handler:  AT+CREG? ------ NetWork Registration
********************************************************************************
*/
static INT8U Handler_AT_R_CREG(INT8U *recvbuf, INT16U len)
{
    INT8U temp;
    
    if (YX_SearchKeyWord(recvbuf, len, "+CREG")) {
        ATCmdAck.ackbuf[0] = YX_SearchDigitalString(recvbuf, len, ',', 1);
        temp               = YX_SearchDigitalString(recvbuf, len, ',', 2);
        if (temp == 0xff) {
            ATCmdAck.ackbuf[1] = YX_SearchDigitalString(recvbuf, len, '\r', 1);
            ATCmdAck.ackbuf[2] = 0xff;
            ATCmdAck.ackbuf[3] = 0xff;
        } else {
            ATCmdAck.ackbuf[1] = temp;
            ATCmdAck.ackbuf[2] = YX_SearchDigitalString(recvbuf, len, ',',  3);
            ATCmdAck.ackbuf[3] = YX_SearchDigitalString(recvbuf, len, '\r', 1);
        }
        return AT_SUCCESS;
    } else {
        return AT_FAILURE;
    }
}
/*
********************************************************************************
* Handler:  AT+COPS? ------ NetWork operator
********************************************************************************
*/
static INT8U Handler_AT_R_COPS(INT8U *recvbuf, INT16U len)
{
    if (YX_SearchKeyWord(recvbuf, len, "ERROR")) {
        return AT_FAILURE;
	}
    if (YX_SearchKeyWord(recvbuf, len, "MOBILE")) {                            /* 中国移动 */
        g_at_ack_info.networkoperator.operatortype = NETWORK_OP_CMCC;
	    return AT_SUCCESS;
	} else if (YX_SearchKeyWord(recvbuf, len, "CMCC")) {
	    g_at_ack_info.networkoperator.operatortype = NETWORK_OP_CMCC;
	    return AT_SUCCESS;
    } else if (YX_SearchKeyWord(recvbuf, len, "46000")) {
	    g_at_ack_info.networkoperator.operatortype = NETWORK_OP_CMCC;
	    return AT_SUCCESS;
	} else if (YX_SearchKeyWord(recvbuf, len, "46002")) {
	    g_at_ack_info.networkoperator.operatortype = NETWORK_OP_CMCC;
	    return AT_SUCCESS;
	} else if (YX_SearchKeyWord(recvbuf, len, "UNICOM")) {                     /* 中国联通 */
	    g_at_ack_info.networkoperator.operatortype = NETWORK_OP_UNICOM;
	    return AT_SUCCESS;
	} else if (YX_SearchKeyWord(recvbuf, len, "CU-GSM")) {
	    g_at_ack_info.networkoperator.operatortype = NETWORK_OP_UNICOM;
	    return AT_SUCCESS;
	} else if (YX_SearchKeyWord(recvbuf, len, "46001")) {
	    g_at_ack_info.networkoperator.operatortype = NETWORK_OP_UNICOM;
	    return AT_SUCCESS;
	} else if (YX_SearchKeyWord(recvbuf, len, "46003")) {                      /* 中国电信 */
	    g_at_ack_info.networkoperator.operatortype = NETWORK_OP_CT;
	    return AT_SUCCESS;
	} else if (YX_SearchKeyWord(recvbuf, len, "TELECOM")) {
	    g_at_ack_info.networkoperator.operatortype = NETWORK_OP_CT;
	    return AT_SUCCESS;
	} else if (YX_SearchKeyWord(recvbuf, len, "OK")) {
	    g_at_ack_info.networkoperator.operatortype = NETWORK_OP_MAX;
	    return AT_SUCCESS;
	}
	return AT_FAILURE;
}

/*
********************************************************************************
* Handler:  AT+CCLK?
********************************************************************************
*/
static INT8U Handler_AT_R_CCLK(INT8U *recvbuf, INT16U len)
{
    if (YX_SearchKeyWord(recvbuf, len, "ERROR")) {
        return AT_FAILURE;
    } else {
        ATCmdAck.ackbuf[0] = YX_SearchDigitalString(recvbuf, len, '/', 1);
        ATCmdAck.ackbuf[1] = YX_SearchDigitalString(recvbuf, len, '/', 2);
        ATCmdAck.ackbuf[2] = YX_SearchDigitalString(recvbuf, len, ',', 1);
        ATCmdAck.ackbuf[3] = YX_SearchDigitalString(recvbuf, len, ':', 2);
        ATCmdAck.ackbuf[4] = YX_SearchDigitalString(recvbuf, len, ':', 3);
        ATCmdAck.ackbuf[5] = YX_SearchDigitalString(recvbuf, len, '+', 2);
        return AT_SUCCESS;
    }
}
/*
********************************************************************************
* Handler:  AT+CIMI
********************************************************************************
*/
static INT8U Handler_AT_R_CIMI(INT8U *recvbuf, INT16U len)
{
    INT8U pos;

	if (ATCmdAck.numEC == 1) {
        if (YX_SearchKeyWord(recvbuf, len, "ERROR")) {
            return AT_FAILURE;
        } else {
		    pos = YX_FindCharPos(recvbuf, 0x0d, 0, len);
			if (pos >= 15 && pos <= 20) {
			    ATCmdAck.ackbuf[0] = pos;
				YX_MEMCPY(&ATCmdAck.ackbuf[1], pos, recvbuf, pos);
			} else {
			    return AT_FAILURE;
			}
		}
    } else if (ATCmdAck.numEC == 2) {
        if (YX_SearchKeyWord(recvbuf, len, "OK")) {
            return AT_SUCCESS;
        } else {
            return AT_FAILURE;
        }
    }
    return AT_CONTINUE;
}

/*
********************************************************************************
* Handler:  AT+GSN
********************************************************************************
*/
static INT8U Handler_AT_R_IMEI(INT8U *recvbuf, INT16U len)
{
    INT8U pos;

	if (ATCmdAck.numEC == 1) {
        if (YX_SearchKeyWord(recvbuf, len, "ERROR")) {
            return AT_FAILURE;
        } else {
		    pos = YX_FindCharPos(recvbuf, 0x0d, 0, len);
			if (pos >= 15 && pos <= 20) {
			    ATCmdAck.ackbuf[0] = pos;
				YX_MEMCPY(&ATCmdAck.ackbuf[1], pos, recvbuf, pos);
			} else {
			    return AT_FAILURE;
			}
		}
    } else if (ATCmdAck.numEC == 2) {
        if (YX_SearchKeyWord(recvbuf, len, "OK")) {
            return AT_SUCCESS;
        } else {
            return AT_FAILURE;
        }
    }
    return AT_CONTINUE;
}
/*
********************************************************************************
* Handler:  AT+ICCID
********************************************************************************
*/
static INT8U Handler_AT_R_ICCID(INT8U *recvbuf, INT16U len)
{
    INT8U pos;

	if (ATCmdAck.numEC == 1) {
        if (YX_SearchKeyWord(recvbuf, len, "ERROR")) {
            return AT_FAILURE;
        } else {
		    pos = YX_FindCharPos(recvbuf, 0x0d, 0, len);
			if (pos >= 15 && pos <= 30) {
			    ATCmdAck.ackbuf[0] = pos;
				YX_MEMCPY(&ATCmdAck.ackbuf[1], pos, recvbuf, pos);
			} else {
			    return AT_FAILURE;
			}
		}
    } else if (ATCmdAck.numEC == 2) {
        if (YX_SearchKeyWord(recvbuf, len, "OK")) {
            return AT_SUCCESS;
        } else {
            return AT_FAILURE;
        }
    }
    return AT_CONTINUE;
}

/*
********************************************************************************
* Handler:  AT+CEREG? ------ NetWork Registration
********************************************************************************
*/
static INT8U Handler_AT_R_CEREG(INT8U *recvbuf, INT16U len)
{
    INT8U temp;
    
    if (YX_SearchKeyWord(recvbuf, len, "+CEREG")) {
        ATCmdAck.ackbuf[0] = YX_SearchDigitalString(recvbuf, len, ',', 1);
        temp               = YX_SearchDigitalString(recvbuf, len, ',', 2);
        if (temp == 0xff) {
            ATCmdAck.ackbuf[1] = YX_SearchDigitalString(recvbuf, len, '\r', 1);
            ATCmdAck.ackbuf[2] = 0xff;
            ATCmdAck.ackbuf[3] = 0xff;
        } else {
            ATCmdAck.ackbuf[1] = temp;
            ATCmdAck.ackbuf[2] = YX_SearchDigitalString(recvbuf, len, ',',  3);
            ATCmdAck.ackbuf[3] = YX_SearchDigitalString(recvbuf, len, '\r', 1);
        }
        return AT_SUCCESS;
    } else {
        return AT_FAILURE;
    }
}

#if GSM_TYPE == GSM_SIMCOM


/*
********************************************************************************
* Handler:  AT+CSMINS ----- 查询SIM卡是否已插入
********************************************************************************
*/
static INT8U Handler_AT_CSMINS(INT8U *recvbuf, INT16U len)
{
    
    if (YX_SearchDigitalString(recvbuf, len, '\r',  1) == 1) {
        return AT_SUCCESS;
    } else {
        return AT_FAILURE;
    }
}
#endif

/*
********************************************************************************
* Handler:  查询SIM卡是否已插入
********************************************************************************
*/
/*static INT8U Handler_QuerySimcard(INT8U *recvbuf, INT16U len)
{
    if (YX_SearchKeyWord(recvbuf, len, "OK")) {
        if (YX_SearchKeyWord(recvbuf, len, "READY")) {
            return AT_SUCCESS;
        }
    }
    return AT_FAILURE;
}*/


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
AT_CMD_PARA_T const g_at_test_para       = { 0,                2,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_getmoduleinfo_para = { ATCMD_INSANT,     4,  1,  0,  Handler_AT_ATI      };
AT_CMD_PARA_T const AT_CALM_PARA         = { 0,                4,  1,  0,  Handler_Common      };
AT_CMD_PARA_T const AT_AND_D_PARA        = { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const AT_IPR_PARA          = { 0,                4,  1,  1,  Handler_Common      };


/*
********************************************************************************
* AT Commands originating from GSM 07.07
********************************************************************************
*/
AT_CMD_PARA_T const AT_CFUN_PARA            =      { 0,                4,  1,  1,  Handler_Common};
AT_CMD_PARA_T const AT_CSQ_PARA             =      { 0,                4,  1,  2,  Handler_AT_CSQ      };
AT_CMD_PARA_T const AT_R_CREG_PARA          =      { ATCMD_INSANT,     4,  1,  0,  Handler_AT_R_CREG   };
AT_CMD_PARA_T const AT_R_COPS_PARA          =      { 0,                4,  1,  2,  Handler_AT_R_COPS   };
AT_CMD_PARA_T const AT_CPBS_PARA            =      { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const AT_CPBW_PARA            =      { 0,               10,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const AT_CPBR_PARA            =      { 0,               15,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const AT_CCLK_PARA            =      { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const AT_R_CCLK_PARA          =      { 0,                4,  1,  2,  Handler_AT_R_CCLK   };
AT_CMD_PARA_T const AT_R_CIMI_PARA          =      { ATCMD_INSANT,     4,  1,  2,  Handler_AT_R_CIMI   };
AT_CMD_PARA_T const AT_R_IMEI_PARA          =      { ATCMD_INSANT,     4,  1,  2,  Handler_AT_R_IMEI   };
AT_CMD_PARA_T const AT_IMEI_PARA            =      { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const AT_R_ICCID_PARA         =      { ATCMD_INSANT,     4,  1,  2,  Handler_AT_R_ICCID  };
AT_CMD_PARA_T const AT_CPIN_PARA            =      { 0,                4,  1,  1,  Handler_Common   };
AT_CMD_PARA_T const AT_R_CPIN_PARA          =      { 0,                4,  1,  1,  Handler_Common   };
AT_CMD_PARA_T const AT_CLCK_PARA            =      { 0,                4,  1,  1,  Handler_Common   };
AT_CMD_PARA_T const AT_CPWD_PARA            =      { 0,                4,  1,  1,  Handler_Common   };
AT_CMD_PARA_T const AT_R_CEREG_PARA         =      { ATCMD_INSANT,     4,  1,  0,  Handler_AT_R_CEREG  };
AT_CMD_PARA_T const AT_CNMP_PARA            =      { 0,                4,  1,  1,  Handler_Common      };


/*
********************************************************************************
* AT Commands originating from GSM 07.05 for SMS
********************************************************************************
*/
AT_CMD_PARA_T const AT_ESCAPE_PARA          =      { 0,                4,  1,  1,  Handler_Common      };

/*
********************************************************************************
* defined AT Commands for enhanced functions
********************************************************************************
*/
#if GSM_TYPE == GSM_SIMCOM
AT_CMD_PARA_T const AT_CSMINS_PARA          =      { 0,                4,  1,  1,  Handler_AT_CSMINS   };
AT_CMD_PARA_T const AT_CRSL_PARA            =      { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const AT_QSCLK_PARA           =      { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const AT_SET_QENG_PARA        =      { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const AT_Q_QENG_PARA          =      { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const AT_CTTS_PARA            =      { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const AT_CTTSPARAM_PARA       =      { 0,                4,  1,  1,  Handler_Common      };
#endif

#if GSM_TYPE == GSM_BENQ
AT_CMD_PARA_T const AT_NOSLEEP_PARA         =      { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const AT_VOLUME_PARA          =      { 0,                4,  1,  2,  Handler_Common      };
AT_CMD_PARA_T const AT_VOLUME1_PARA         =      { 0,                4,  1,  2,  Handler_Common      };
AT_CMD_PARA_T const AT_AUPATH_PARA          =      { 0,                4,  1,  2,  Handler_Common      };
AT_CMD_PARA_T const AT_AUAEC_PARA           =      { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const AT_AULEVELMAX_PARA      =      { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const AT_CGPCO_PARA           =      { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const AT_PKTSIZE_PARA         =      { 0,                4,  1,  2,  Handler_Common      };
AT_CMD_PARA_T const AT_TIMEOUT_PARA         =      { 0,                4,  1,  2,  Handler_Common      };
AT_CMD_PARA_T const AT_DESTINFO_PARA        =      { 0,                4,  1,  2,  Handler_Common      };
AT_CMD_PARA_T const AT_CGDCONT_PARA         =      { 0,                4,  1,  1,  Handler_Common      };
#endif


/*********************************************************************************
**                                                                               *
**                                                                               *
**                 Standard V.25ter AT Commands                                  *
**                                                                               *
**                                                                               *
*********************************************************************************/

/*
********************************************************************************
* AT    Test communication
********************************************************************************
*/
INT8U AT_CMD_Test(INT8U *dptr, INT32U maxlen)
{
    STREAM_T wstrm;
    char const str_text[] = {"ATE0\r"};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    YX_WriteSTR_Strm(&wstrm, (char *)str_text);
    return YX_GetStrmLen(&wstrm);
}

/*
********************************************************************************
* ATI   display product identification information
********************************************************************************
*/
INT8U AT_CMD_GetModuleInfo(INT8U *dptr, INT32U maxlen)
{
    STREAM_T wstrm;
    char const str_text[] = {"ATI\r"};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    YX_WriteSTR_Strm(&wstrm, (char *)str_text);
    return YX_GetStrmLen(&wstrm);
}
/*
********************************************************************************
* CGMR  获取手机模块软件版本号
********************************************************************************
*/
INT8U AT_CMD_GetModuleVerInfo(INT8U *dptr, INT32U maxlen)
{
    STREAM_T wstrm;
    char const str_text[] = {"AT+CGMR\r"};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    YX_WriteSTR_Strm(&wstrm, (char *)str_text);
    return YX_GetStrmLen(&wstrm);
}
/*
********************************************************************************
* CALM   display product identification information
********************************************************************************
*/
INT8U AT_CALM(INT8U *dptr, INT32U maxlen)
{
    char const str_CALM[] = {"AT+CALM=1\r"};
    
    YX_MEMCPY(dptr, maxlen, str_CALM, sizeof(str_CALM) - 1);
    return sizeof(str_CALM) - 1;
}

/*
********************************************************************************
* AT&D    set DTR function mode
********************************************************************************
*/
INT8U AT_AND_D(INT8U *dptr, INT32U maxlen)
{
    char const str_AT_AND_D[] = {"AT&D2\r"};
    
    YX_MEMCPY(dptr, maxlen, str_AT_AND_D, sizeof(str_AT_AND_D) - 1);
    return sizeof(str_AT_AND_D) - 1;
}

/*
********************************************************************************
* AT+IPR    Set fixed local rate
********************************************************************************
*/
INT8U AT_IPR(INT8U *dptr, INT32U maxlen, char *rate)
{
    INT8U len;
    char const str_IPR[] = {"AT+IPR="};
    
    YX_MEMCPY(dptr, maxlen, str_IPR, sizeof(str_IPR) - 1);
    dptr += sizeof(str_IPR) - 1;
    len  = YX_STRLEN(rate);
    YX_MEMCPY(dptr, maxlen, rate, len);
    dptr += len;
    *dptr = '\r';
    return ((sizeof(str_IPR) - 1) + len + 1);
}

/*********************************************************************************
**                                                                               *
**                                                                               *
**                 AT Commands originating from GSM 07.07                        *
**                                                                               *
**                                                                               *
*********************************************************************************/

/*
********************************************************************************
* AT+CFUN
********************************************************************************
*/
INT8U AT_CFUN(INT8U *dptr, INT32U maxlen, INT8U fun)
{
    INT8U len;
    
    char const str_CFUN[] = {"AT+CFUN="};
    
    YX_MEMCPY(dptr, maxlen, str_CFUN, sizeof(str_CFUN) - 1);
    len = sizeof(str_CFUN) - 1;
    dptr += sizeof(str_CFUN) - 1;
    *dptr++ = fun;
    len++;
    *dptr++ = '\r';
    len++;
    
    return len;
}

/*
********************************************************************************
* AT+CSQ    Signal Quality
********************************************************************************
*/
INT8U AT_CSQ(INT8U *dptr, INT32U maxlen)
{
    char const str_CSQ[] = {"AT+CSQ\r"};
    
    YX_MEMCPY(dptr, maxlen, str_CSQ, sizeof(str_CSQ) - 1);
    return sizeof(str_CSQ) - 1;
}

/*
********************************************************************************
* AT+CREG?    NetWork Registration Query
********************************************************************************
*/
INT8U AT_R_CREG(INT8U *dptr, INT32U maxlen)
{
    char const str_R_CREG[] = {"AT+CREG?\r"};
    
    YX_MEMCPY(dptr, maxlen, str_R_CREG, sizeof(str_R_CREG) - 1);
    return sizeof(str_R_CREG) - 1;
}

/*
********************************************************************************
* AT+COPS?    NetWork operator
********************************************************************************
*/
INT8U AT_R_COPS(INT8U *dptr, INT32U maxlen)
{
    char const str_R_COPS[] = {"AT+COPS?\r"};
    
    YX_MEMCPY(dptr, maxlen, str_R_COPS, sizeof(str_R_COPS) - 1);
    return sizeof(str_R_COPS) - 1;
}

/*
********************************************************************************
* AT+CPBS="SM"
********************************************************************************
*/
INT8U AT_CPBS(INT8U *dptr, INT32U maxlen)
{
    char const str_CPBS[] = {"AT+CPBS=\"SM\"\r"};
    
    YX_MEMCPY(dptr, maxlen, str_CPBS, sizeof(str_CPBS) - 1);
    return sizeof(str_CPBS) - 1;
}

/*
********************************************************************************
* AT+CPBR
********************************************************************************
*/
INT8U AT_CPBR(INT8U *dptr, INT32U maxlen, INT8U index1, INT8U index2)
{
    INT8U len1, len2;
    char const str_CPBR[] = {"AT+CPBR="};
    
    YX_MEMCPY(dptr, maxlen, str_CPBR, sizeof(str_CPBR) - 1);
    dptr   += sizeof(str_CPBR) - 1;
    len1   = YX_DecToAscii(dptr, index1, 0);
    dptr   += len1;
    *dptr++ = ',';
    len2   = YX_DecToAscii(dptr, index2, 0);
    dptr   += len2;
    *dptr   = '\r';
    return ((sizeof(str_CPBR) - 1) + len1 + 1 + len2 + 1);
}

/*
********************************************************************************
* AT+CPBW
********************************************************************************
*/
INT8U AT_CPBW(INT8U *dptr, INT32U maxlen, INT8U index, INT8U *tel, INT8U tellen, INT8U *text, INT8U textlen)
{
    INT8U len;
    char const str_CPBW[] = {"AT+CPBW="};

    if (tellen > 20) {                              /* SIM300C模块要求电话号码长度不超过20个字节 */
        tellen = 20;
    }
    if (textlen > 14) {                             /* SIM300C模块要求文本信息不超过14个字节 */
        textlen = 14;
    }
    YX_MEMCPY(dptr, maxlen, str_CPBW, sizeof(str_CPBW) - 1);
    dptr   += sizeof(str_CPBW) - 1;
    len    = YX_DecToAscii(dptr, index, 0);
    dptr   += len;
    *dptr++ = ',';
    *dptr++ = '"';
    YX_MEMCPY(dptr, maxlen, tel, tellen);
    dptr   += tellen;
    *dptr++ = '"';
    *dptr++ = ',';
    *dptr++ = ',';
    *dptr++ = '"';
    YX_MEMCPY(dptr, maxlen, text, textlen);
    dptr   += textlen;
    *dptr++ = '"';
    *dptr   = '\r';
    return (sizeof(str_CPBW) - 1) + len + 2 + tellen + 4 + textlen + 2;
}

/*
********************************************************************************
* AT+CCLK
********************************************************************************
*/
INT8U AT_CCLK(INT8U *dptr, INT32U maxlen, DATE_T  *date, TIME_T  *time)
{
    INT8U t_len, len;
    char const str_CCLK[] = {"AT+CCLK="};
    
    t_len  = 0;
    YX_MEMCPY(dptr, maxlen, str_CCLK, sizeof(str_CCLK) - 1);
    dptr   += sizeof(str_CCLK) - 1;
    t_len += sizeof(str_CCLK) - 1;
    *dptr++ = '"';
    len    = YX_DecToAscii(dptr, date->year, 2);
    dptr   += len;
    *dptr++ = '/';
    t_len += len + 2;
    len    = YX_DecToAscii(dptr, date->month, 2);
    dptr   += len;
    *dptr++ = '/';
    t_len += len + 1;
    len    = YX_DecToAscii(dptr, date->day, 2);
    dptr   += len;
    *dptr++ = ',';
    t_len += len + 1;
    len    = YX_DecToAscii(dptr, time->hour, 2);
    dptr   += len;
    *dptr++ = ':';
    t_len += len + 1;
    len    = YX_DecToAscii(dptr, time->minute, 2);
    dptr   += len;
    *dptr++ = ':';
    t_len += len + 1;
    len    = YX_DecToAscii(dptr, time->second, 2);
    dptr   += len;
    YX_MEMCPY(dptr, maxlen, "+00\"\r", sizeof("+00\"\r") - 1);
    t_len += len + sizeof("+00\"\r") - 1;
    return t_len;
}

/*
********************************************************************************
* AT+CCLK?
********************************************************************************
*/
INT8U AT_R_CCLK(INT8U *dptr, INT32U maxlen)
{
    char const str_CCLK[] = {"AT+CCLK?\r"};
    
    YX_MEMCPY(dptr, maxlen, str_CCLK, sizeof(str_CCLK));
    return sizeof(str_CCLK) - 1;
}
/*
********************************************************************************
* AT+CIMI
********************************************************************************
*/
INT8U AT_CMD_GetIMSI(INT8U *dptr, INT32U maxlen)
{
    char const str_CIMI[] = {"AT+CIMI\r"};
    
    YX_MEMCPY(dptr, maxlen, str_CIMI, sizeof(str_CIMI));
    return sizeof(str_CIMI) - 1;
}

/*
********************************************************************************
* AT+GSN
********************************************************************************
*/
INT8U AT_CMD_GetIMEI(INT8U *dptr, INT32U maxlen)
{
    char const str_GSN[] = {"AT+GSN\r"};
    
    YX_MEMCPY(dptr, maxlen, str_GSN, sizeof(str_GSN));
    return sizeof(str_GSN) - 1;
}

/*
********************************************************************************
* AT+IMEI
********************************************************************************
*/
INT8U AT_IMEI(INT8U *dptr, INT32U maxlen, INT8U *imei, INT8U imeilen)
{
    char const str_EGMR[] = {"AT+EGMR=1,7,\""};
    
    YX_MEMCPY(dptr, maxlen, str_EGMR, sizeof(str_EGMR) - 1);
    dptr   += sizeof(str_EGMR) - 1;
    YX_MEMCPY(dptr, maxlen, imei, imeilen);
    dptr   += imeilen;
    *dptr++ = '"';
    *dptr   = '\r';
    return ((sizeof(str_EGMR) - 1) + imeilen + 2);
}
/*
********************************************************************************
* AT+ICCID
********************************************************************************
*/
INT8U AT_CMD_GetICCID(INT8U *dptr, INT32U maxlen)
{
    char const str_ICCID[] = {"AT+CCID\r"};
    
    YX_MEMCPY(dptr, maxlen, str_ICCID, sizeof(str_ICCID));
    return sizeof(str_ICCID) - 1;
}
/*
********************************************************************************
* AT+CPIN
********************************************************************************
*/
INT8U AT_CPIN(INT8U *dptr, INT32U maxlen, char *password)
{
    STREAM_T wstrm;
    char const str_CPIN[] = {"AT+CPIN="};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    YX_WriteSTR_Strm(&wstrm, (char *)str_CPIN);
    YX_WriteSTR_Strm(&wstrm, "\"");
    YX_WriteSTR_Strm(&wstrm, password);
    YX_WriteSTR_Strm(&wstrm, "\"\r");
    
    return YX_GetStrmLen(&wstrm);
}

/*
********************************************************************************
* AT+CPIN?
********************************************************************************
*/
INT8U AT_R_CPIN(INT8U *dptr, INT32U maxlen)
{
    STREAM_T wstrm;
    char const str_R_CPIN[] = {"AT+CPIN?\r"};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    YX_WriteSTR_Strm(&wstrm, (char *)str_R_CPIN);
    
    return YX_GetStrmLen(&wstrm);
}

/*
********************************************************************************
* AT+CLCK
********************************************************************************
*/
INT8U AT_CLCK(INT8U *dptr, INT32U maxlen, INT8U lock, char *password)
{
    STREAM_T wstrm;
    char const str_CLCK[] = {"AT+CLCK=\"SC\","};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    YX_WriteSTR_Strm(&wstrm, (char *)str_CLCK);
    if (lock == 0) {
        YX_WriteSTR_Strm(&wstrm, "0,\"");
    } else {
        YX_WriteSTR_Strm(&wstrm, "1,\"");
    }
    
    YX_WriteSTR_Strm(&wstrm, password);
    YX_WriteSTR_Strm(&wstrm, "\"\r");
    
    return YX_GetStrmLen(&wstrm);
}

/*
********************************************************************************
* AT+CPWD
********************************************************************************
*/
INT8U AT_CPWD(INT8U *dptr, INT32U maxlen, char *opassword, char *npassword)
{
    STREAM_T wstrm;
    char const str_cpwd[] = {"AT+CPWD=\"SC\",\""};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    YX_WriteSTR_Strm(&wstrm, (char *)str_cpwd);
    YX_WriteSTR_Strm(&wstrm, opassword);
    YX_WriteSTR_Strm(&wstrm, "\",\"");
    YX_WriteSTR_Strm(&wstrm, npassword);
    YX_WriteSTR_Strm(&wstrm, "\"\r");
    
    return YX_GetStrmLen(&wstrm);
}

/*
********************************************************************************
* ESCAPE    Abort message
********************************************************************************
*/
INT8U AT_ESCAPE(INT8U *dptr, INT32U maxlen)
{
    *dptr = 0x1b;
    return 1;
}

/*
********************************************************************************
* AT+CEREG?    NetWork Registration Query
********************************************************************************
*/
INT8U AT_R_CEREG(INT8U *dptr, INT32U maxlen)
{
    char const str_R_CEREG[] = {"AT+CEREG?\r"};
    
    YX_MEMCPY(dptr, maxlen, str_R_CEREG, sizeof(str_R_CEREG) - 1);
    return sizeof(str_R_CEREG) - 1;
}


/*********************************************************************************
**                                                                               *
**                                                                               *
**                      defined AT Commands for enhanced functions               *
**                                                                               *
**                                                                               *
*********************************************************************************/

/*
********************************************************************************
* AT+CSMINS?, 查询SIM卡是否已插入
********************************************************************************
*/
INT8U AT_CSMINS(INT8U *dptr, INT32U maxlen)
{
    char const str_CSMINS[] = {"AT+CSMINS?\r"};
    
    YX_MEMCPY(dptr, maxlen, str_CSMINS, sizeof(str_CSMINS) - 1);
    return sizeof(str_CSMINS) - 1;
}

/*
********************************************************************************
* AT+CRSL, 调节来电振铃音量
********************************************************************************
*/
INT8U AT_CRSL(INT8U *dptr, INT32U maxlen)
{
    char const str_CRSL[] = {"AT+CRSL=0\r"};
    
    YX_MEMCPY(dptr, maxlen, str_CRSL, sizeof(str_CRSL) - 1);
    return sizeof(str_CRSL) - 1;
}
/*
********************************************************************************
* AT+QSCLK, 省电模式设置
********************************************************************************
*/
INT8U AT_QSCLK(INT8U *dptr, INT32U maxlen, INT8U enable)
{
    char const str_EN_CRSL[]  = {"AT+QSCLK=1\r"};
    char const str_DIS_CRSL[] = {"AT+QSCLK=0\r"};
    
    if (enable) {
        YX_MEMCPY(dptr, maxlen, str_EN_CRSL, sizeof(str_EN_CRSL) - 1);
    } else {
        YX_MEMCPY(dptr, maxlen, str_DIS_CRSL, sizeof(str_DIS_CRSL) - 1);
    }
    return sizeof(str_EN_CRSL) - 1;
}
/*
********************************************************************************
* AT+QENG, 设置基站信息
********************************************************************************
*/
INT8U AT_SET_QENG(INT8U *dptr, INT32U maxlen, INT8U enable)
{
    char const str_EN_QENG[] = {"AT+QENG=1,1\r"};
    char const str_DIS_QENG[] = {"AT+QENG=0,1\r"};
    
    if (enable) {
        YX_MEMCPY(dptr, maxlen, str_EN_QENG, sizeof(str_EN_QENG) - 1);
    } else {
        YX_MEMCPY(dptr, maxlen, str_DIS_QENG, sizeof(str_DIS_QENG) - 1);
    }
    return sizeof(str_EN_QENG) - 1;
}

/*
********************************************************************************
* AT+QENG, 查询基站信息
********************************************************************************
*/
INT8U AT_Q_QENG(INT8U *dptr, INT32U maxlen)
{
    char const str_Q_QENG[] = {"AT+QENG?\r"};
    
    YX_MEMCPY(dptr, maxlen, str_Q_QENG, sizeof(str_Q_QENG) - 1);
    return sizeof(str_Q_QENG) - 1;
}

/*
********************************************************************************
* AT+CTTS, 播放TTS
********************************************************************************
*/
INT16U AT_CTTS(INT8U *dptr, INT32U maxlen, INT8U *sptr, INT16U len)
{
    STREAM_T wstrm;
    char const str_CTTS_PLAY[] = {"AT+CTTS=2,\""};
    char const str_CTTS_STOP[] = {"AT+CTTS=0\r"};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    if (len > 0) {
        YX_WriteSTR_Strm(&wstrm, (char *)str_CTTS_PLAY);
        YX_WriteDATA_Strm(&wstrm, sptr, len);
        YX_WriteSTR_Strm(&wstrm, "\"\r");
    } else {
        YX_WriteSTR_Strm(&wstrm, (char *)str_CTTS_STOP);
    }
    
    return YX_GetStrmLen(&wstrm);
}

/*
********************************************************************************
* AT+CTTSPARAM, 设置TTS参数
********************************************************************************
*/
INT16U AT_CTTSPARAM(INT8U *dptr, INT32U maxlen, INT8U volume)
{
    STREAM_T wstrm;
    /* TTS音量，模块音量，播放模式，语调，语速，通道*/
    char const str_para[] = {"AT+CTTSPARAM=50,"};
    char const gain_tts[11][4] = {
                                        {"1"},  {"10"}, {"15"}, {"20"}, {"22"}
                                       ,{"25"}, {"30"}, {"35"}, {"50"}, {"90"}
                                       ,{"95"}};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    YX_WriteSTR_Strm(&wstrm, (char *)str_para);
    YX_WriteSTR_Strm(&wstrm, (char *)(gain_tts[volume]));
    //len = YX_DecToAscii(YX_GetStrmPtr(&wstrm), datalen, 0);
    YX_WriteSTR_Strm(&wstrm, ",0,50,55,1\r");
    
    return YX_GetStrmLen(&wstrm);
}

#endif
