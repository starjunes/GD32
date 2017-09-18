/********************************************************************************
**
** 文件名:     at_cmd_sms.c
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
#include "yx_include.h"
#include "yx_misc.h"
#include "yx_stream.h"
#include "at_drv.h"


#if EN_AT > 0


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
* Handler:  AT+CMGS_PDU 
********************************************************************************
*/
static INT8U Handler_AT_CMGS_PDU(INT8U *recvbuf, INT16U len)
{
    if (YX_SearchKeyWord(recvbuf, len, "ERROR")) {
    //if (!YX_SearchKeyWord(recvbuf, len, "OK")) {
        return AT_FAILURE;
    } else {
        return AT_SUCCESS;
    }
}

/*
********************************************************************************
* Handler:  AT+CSCA?
********************************************************************************
*/
static INT8U Handler_AT_R_CSCA(INT8U *recvbuf, INT16U len)
{
    if (ATCmdAck.numEC == 1) {
        if (YX_SearchKeyWord(recvbuf, len, "ERROR")) {
            return AT_FAILURE;
        } else {
            ATCmdAck.ackbuf[0] = YX_SearchString(&ATCmdAck.ackbuf[1], sizeof(ATCmdAck.ackbuf) - 1, recvbuf, len, '"', 1);
        }
    } else if (ATCmdAck.numEC == 2) {
        return AT_SUCCESS;
    }
    return AT_CONTINUE;
}

/*
********************************************************************************
* Handler:  AT+CSMS
********************************************************************************
*/
static INT8U Handler_AT_CSMS(INT8U *recvbuf, INT16U len)
{
    /*if (ATCmdAck.numEC == 1) {
        if (YX_SearchKeyWord(recvbuf, len, "ERROR")) {
            return AT_FAILURE;
        }
    } else if (ATCmdAck.numEC == 2) {
        if (YX_SearchKeyWord(recvbuf, len, "OK")) {
            return AT_SUCCESS;
        } else {
            return AT_FAILURE;
        }
    }
    return AT_CONTINUE;*/
    
    if (YX_SearchKeyWord(recvbuf, len, "ERROR")) {
        return AT_FAILURE;
    }
    
    if (YX_SearchKeyWord(recvbuf, len, "OK")) {
        return AT_SUCCESS;
    }
    
    return AT_CONTINUE;
}

/*
********************************************************************************
* Handler:  AT+CNMI? ----- Query New SMS message indications
********************************************************************************
*/
static INT8U Handler_AT_R_CNMI(INT8U *recvbuf, INT16U len)
{
    if (ATCmdAck.numEC == 1) {
        if (YX_SearchKeyWord(recvbuf, len, "ERROR")) {
            return AT_FAILURE;
        } else {
            ATCmdAck.ackbuf[0] = YX_SearchDigitalString(recvbuf, len, ',',  1);
            ATCmdAck.ackbuf[1] = YX_SearchDigitalString(recvbuf, len, ',',  2);
            ATCmdAck.ackbuf[2] = YX_SearchDigitalString(recvbuf, len, ',',  3);
            ATCmdAck.ackbuf[3] = YX_SearchDigitalString(recvbuf, len, ',',  4);
            ATCmdAck.ackbuf[4] = YX_SearchDigitalString(recvbuf, len, '\r', 1);
        }
    } else if (ATCmdAck.numEC == 2) {
        return AT_SUCCESS;
    }
    return AT_CONTINUE;
}

/*********************************************************************************
**                                                                               *
**                                                                               *
**                 AT Commands parameters                                        *
**                                                                               *
**                                                                               *
*********************************************************************************/
AT_CMD_PARA_T const g_sms_setformat_para  = { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_sms_setcsca_para    = { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_sms_getcsca_para    = { ATCMD_INSANT,     4,  1,  0,  Handler_AT_R_CSCA   };
AT_CMD_PARA_T const g_sms_setcsms_para    = { ATCMD_INSANT,     4,  1,  0,  Handler_AT_CSMS     };
AT_CMD_PARA_T const g_sms_setcscs_para    = { 0,                4,  1,  0,  Handler_Common     };
AT_CMD_PARA_T const g_sms_setcnma_para    = { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_sms_setcnmi_para    = { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_sms_getcnmi_para    = { ATCMD_INSANT,     4,  1,  0,  Handler_AT_R_CNMI   };
AT_CMD_PARA_T const g_sms_sendhead_para   = { 0,                4,  1,  0,  0                   };
AT_CMD_PARA_T const g_sms_senddata_para   = { 0,               45,  1,  1,  Handler_AT_CMGS_PDU };
AT_CMD_PARA_T const g_sms_delete_para     = { 0,               10,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_sms_list_para       = { 0,               15,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_sms_read_para       = { 0,                5,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_sms_cpms_para       = { 0,                5,  1,  2,  Handler_Common      };


/*
********************************************************************************
* AT+CMGF    Select SMS message format
********************************************************************************
*/
INT8U AT_CMD_SetSmsFormat(INT8U *dptr, INT32U maxlen)
{
    char const str_CMGF[] = {"AT+CMGF=0\r"};
    
    YX_MEMCPY(dptr, maxlen, str_CMGF, sizeof(str_CMGF) - 1);
    return sizeof(str_CMGF) - 1;
}

/*
********************************************************************************
* AT+CSCA    SMS service centre address
********************************************************************************
*/
INT8U AT_CMD_SetSmscTel(INT8U *dptr, INT32U maxlen, INT8U  *tel, INT8U tellen)
{
    char const str_CSCA[] = {"AT+CSCA=\""};
    
    YX_MEMCPY(dptr, maxlen, str_CSCA, sizeof(str_CSCA) - 1);
    dptr   += sizeof(str_CSCA) - 1;
    YX_MEMCPY(dptr, maxlen, tel, tellen);
    dptr   += tellen;
    *dptr++ = '"';
    *dptr   = '\r';
    return ((sizeof(str_CSCA) - 1) + tellen + 2);
}

/*
********************************************************************************
* AT+CSCA?    query SMS service centre address
********************************************************************************
*/
INT8U AT_CMD_GetSmscTel(INT8U *dptr, INT32U maxlen)
{
    char const str_R_CSCA[] = {"AT+CSCA?\r"};
    
    YX_MEMCPY(dptr, maxlen, str_R_CSCA, sizeof(str_R_CSCA) - 1);
    return sizeof(str_R_CSCA) - 1;
}

/*
********************************************************************************
* AT+CSMS    Select Message Service
********************************************************************************
*/
INT8U AT_CMD_SetMessageService(INT8U *dptr, INT32U maxlen)
{
    INT8U moduletype;
    STREAM_T wstrm;
    char const str_CSMS[] = {"AT+CSMS=0\r"};
    char const str_text_at[] = {"AT\r"};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    moduletype = ADP_NET_GetModuleType();
    if (moduletype == MODULE_TYPE_SIM6320) {
        YX_WriteSTR_Strm(&wstrm, (char *)str_text_at);
    } else {
        YX_WriteSTR_Strm(&wstrm, (char *)str_CSMS);
    }
    return YX_GetStrmLen(&wstrm);
}

/*
********************************************************************************
* AT+CSCS    Select TE Character Set
********************************************************************************
*/
INT8U AT_CMD_SetSmsCharactorSet(INT8U *dptr, INT32U maxlen)
{
    char const str_CSCS[] = {"AT+CSCS=\"UCS2\"\r"};
    
    YX_MEMCPY(dptr, maxlen, str_CSCS, sizeof(str_CSCS) - 1);
    return sizeof(str_CSCS) - 1;
}

/*
********************************************************************************
* AT+CNMA
********************************************************************************
*/
INT8U AT_CMD_SetSmsAck(INT8U *dptr, INT32U maxlen)
{
    char const str_CNMA[] = {"AT+CNMA=0\r"};
    
    YX_MEMCPY(dptr, maxlen, str_CNMA, sizeof(str_CNMA) - 1);
    return sizeof(str_CNMA) - 1;
}

/*
********************************************************************************
* AT+CNMI    New SMS message indications
********************************************************************************
*/
INT8U AT_CMD_SetSmsIndication(INT8U *dptr, INT32U maxlen)
{
    //char const str_CNMI[] = {"AT+CNMI=1,2\r"};
    char const str_CNMI[] = {"AT+CNMI=2,1\r"};
    
    YX_MEMCPY(dptr, maxlen, str_CNMI, sizeof(str_CNMI) - 1);
    return sizeof(str_CNMI) - 1;
}

/*
********************************************************************************
* AT+CNMI?    Query New SMS message indications
********************************************************************************
*/
INT8U AT_CMD_GetSmsIndication(INT8U *dptr, INT32U maxlen)
{
    char const str_R_CNMI[] = {"AT+CNMI?\r"};
    
    YX_MEMCPY(dptr, maxlen, str_R_CNMI, sizeof(str_R_CNMI) - 1);
    return sizeof(str_R_CNMI) - 1;
}

/*
********************************************************************************
* AT+CMGS    Send SMS message
********************************************************************************
*/
INT8U AT_CMD_SendSm(INT8U *dptr, INT32U maxlen, INT8U pdulen)
{
    INT8U len;
    char const str_CMGS[] = {"AT+CMGS="};
    
    YX_MEMCPY(dptr, maxlen, str_CMGS, sizeof(str_CMGS) - 1);
    dptr += sizeof(str_CMGS) - 1;
    len  = YX_DecToAscii(dptr, pdulen, 0);
    dptr += len;
    *dptr = '\r';
    return ((sizeof(str_CMGS) - 1) + len + 1);
}

/*
********************************************************************************
* AT+CMGD    Delete SMS Message
********************************************************************************
*/
INT8U AT_CMD_DeleteSm(INT8U *dptr, INT32U maxlen, INT8U index)
{
    INT8U len;
    char const str_CMGD[] = {"AT+CMGD="};
    
    YX_MEMCPY(dptr, maxlen, str_CMGD, sizeof(str_CMGD) - 1);
    dptr += sizeof(str_CMGD) - 1;    
    len  = YX_DecToAscii(dptr, index, 0);
    dptr += len;
    *dptr = '\r';
    return ((sizeof(str_CMGD) - 1) + len + 1);
}

/*
********************************************************************************
* AT+CMGR    Read SMS Message
********************************************************************************
*/
INT8U AT_CMD_ReadSm(INT8U *dptr, INT32U maxlen, INT8U index)
{
    INT8U len;
    char const str_CMGR[] = {"AT+CMGR="};
    
    YX_MEMCPY(dptr, maxlen, str_CMGR, sizeof(str_CMGR) - 1);
    dptr += sizeof(str_CMGR) - 1;
    len  = YX_DecToAscii(dptr, index, 0);
    dptr += len;
    *dptr = '\r';
    return ((sizeof(str_CMGR) - 1) + len + 1);
}

/*
********************************************************************************
* AT+CMGL    List SMS Message
********************************************************************************
*/
INT8U AT_CMD_ListSm(INT8U *dptr, INT32U maxlen)
{
    char const str_CMGL[] = {"AT+CMGL=4\r"};
    
    YX_MEMCPY(dptr, maxlen, str_CMGL, sizeof(str_CMGL) - 1);
    return sizeof(str_CMGL) - 1;
}

/*
********************************************************************************
* AT+CMGL    select Message storage
********************************************************************************
*/
INT8U AT_CMD_SelectSmStorage(INT8U *dptr, INT32U maxlen)
{
    char const str_text[] = {"AT+CPMS=\"SM\",\"SM\",\"SM\"\r"};
    
    YX_MEMCPY(dptr, maxlen, str_text, sizeof(str_text) - 1);
    return sizeof(str_text) - 1;
}


#endif
