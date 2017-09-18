/********************************************************************************
**
** 文件名:     at_cmd_tcpip.c
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
static INT8U Handler_Common(INT8U *recvbuf, INT16U recvlen)
{
    if (YX_SearchKeyWord(recvbuf, recvlen, "OK")) {
        return AT_SUCCESS;
    } else {
        return AT_FAILURE;
    }
}

#if GSM_TYPE == GSM_SIM800

/*
********************************************************************************
* Handler:  AT+CIPSEND
********************************************************************************
*/
static INT8U Handler_AT_CIPSEND(INT8U *recvbuf, INT16U recvlen)
{
    if (YX_SearchKeyWord(recvbuf, recvlen, "\r\nSEND FAIL") || YX_SearchKeyWord(recvbuf, recvlen, "\r\nERROR")) {
        return AT_FAILURE;
    } else {
        return AT_SUCCESS;
    }
}

/*
********************************************************************************
* Handler:  AT+CIPCLOSE
********************************************************************************
*/
static INT8U Handler_SocketClose(INT8U *recvbuf, INT16U recvlen)
{
    if (YX_SearchKeyWord(recvbuf, len, "CLOSE OK")) {
        ATCmdAck.ackbuf[0] = YX_SearchDigitalString(recvbuf, len, ',', 1);
        return AT_SUCCESS;
    } else {
        return AT_FAILURE;
    }
}

/*
********************************************************************************
* Handler:  AT+CIPSTATUS? ----- 查询网络状态
********************************************************************************
*/
static INT8U Handler_AT_CIPSTATUS(INT8U *recvbuf, INT16U recvlen)
{
    if (YX_SearchKeyWord(recvbuf, len, "IP INITIAL")) {                    /* 初始化 */
        ATCmdAck.ackbuf[0] = 0;
    } else if (YX_SearchKeyWord(recvbuf, len, "IP START")) {               /* 启动任务 */
        ATCmdAck.ackbuf[0] = 1;
    } else if (YX_SearchKeyWord(recvbuf, len, "IP CONFIG")) {              /* 配置场景 */
        ATCmdAck.ackbuf[0] = 2;
    } else if (YX_SearchKeyWord(recvbuf, len, "IP IND")) {                 /* 接受场景配置 */
        ATCmdAck.ackbuf[0] = 3;
    } else if (YX_SearchKeyWord(recvbuf, len, "IP GPRSACT")) {             /* 场景已激活 */
        ATCmdAck.ackbuf[0] = 4;
    } else if (YX_SearchKeyWord(recvbuf, len, "IP STATUS")) {              /* 获得本地IP地址 */
        ATCmdAck.ackbuf[0] = 5;
    } else if (YX_SearchKeyWord(recvbuf, len, "TCP CONNECTING")) {         /* 与SERVER建立连接阶段 */
        ATCmdAck.ackbuf[0] = 6;
    } else if (YX_SearchKeyWord(recvbuf, len, "IP CLOSE")) {               /* 连接已关闭 */
        ATCmdAck.ackbuf[0] = 7;
    } else if (YX_SearchKeyWord(recvbuf, len, "CONNECT OK")) {             /* 连接建立成功 */
        ATCmdAck.ackbuf[0] = 8;
    } else if (YX_SearchKeyWord(recvbuf, len, "PDP DEACT")) {              /* GPRS已掉线 */
        ATCmdAck.ackbuf[0] = 9;
    } else {
        ATCmdAck.ackbuf[0] = 0xff;
    }
    return AT_SUCCESS;
}

/*
********************************************************************************
* Handler:  AT+CIFSR ----- 获取IP地址
********************************************************************************
*/
static INT8U Handler_GetLocalIp(INT8U *recvbuf, INT16U recvlen)
{
    INT16U recvlen;
    
    ATCmdAck.ackbuf[0] = '\0';
    if (!YX_SearchKeyWord(recvbuf, len, "ERROR")) {
        recvlen = YX_FindCharPos(recvbuf, '\r', 0, len);
        if (recvlen > sizeof(ATCmdAck.ackbuf) - 1) {
            recvlen = sizeof(ATCmdAck.ackbuf) - 1;
        }
        YX_MEMCPY(ATCmdAck.ackbuf, recvlen, recvbuf, recvlen);
        ATCmdAck.ackbuf[recvlen] = '\0';
        return AT_SUCCESS;
    } else {
        return AT_FAILURE;
    }
}

AT_CMD_PARA_T const g_socket_connect_para        =      { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_socket_send_para         =      { ATCMD_BREAK,      4,  2,  0,  Handler_AT_CIPSEND  };
AT_CMD_PARA_T const g_socket_close_para        =      { 0,                4,  1,  1,  Handler_SocketClose };
AT_CMD_PARA_T const g_pdp_deactive_para         =      { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_socket_multi_para          =      { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_dns_getipbyname_para         =      { 0,                10, 1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_dns_setdns_para         =      { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_pdp_setauth_para            =      { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_pdp_active_para           =      { 0,               30,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_pdp_getlocalip_para           =      { 0,                4,  1,  1,  Handler_GetLocalIp    };
AT_CMD_PARA_T const AT_CIPSTATUS_PARA       =      { 0,                4,  1,  1,  Handler_AT_CIPSTATUS};
AT_CMD_PARA_T const g_socket_sethead_para         =      { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_socket_sendprompt_para         =      { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const AT_CIPCSGP_PARA         =      { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_pdp_setdetect_para         =      { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_socket_sendstyle_para        =      { 0,                4,  1,  1,  Handler_Common      };


/*
********************************************************************************
* AT+CIPSTART, 建立TCP连接
********************************************************************************
*/
INT8U AT_CMD_SocketConnect(INT8U *dptr, INT32U maxlen, INT8U ch, INT8U socket_type, char *ip, INT16U port)
{
    STREAM_T wstrm;
    INT8U len;
    char const str_CIPSTART[] = {"AT+CIPSTART="};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    YX_WriteSTR_Strm(&wstrm, (char *)str_CIPSTART);
    
    YX_WriteBYTE_Strm(&wstrm, ch + 0x30);
    
    if (socket_type == SOCKET_TYPE_STREAM) {
        YX_WriteSTR_Strm(&wstrm, ",\"TCP\",\"");
    } else {
        YX_WriteSTR_Strm(&wstrm, ",\"UDP\",\"");
    }
    
    YX_WriteSTR_Strm(&wstrm, ip);
    YX_WriteSTR_Strm(&wstrm, "\",");
    
    len = YX_DecToAscii(YX_GetStrmPtr(&wstrm), port, 0);
    YX_MovStrmPtr(&wstrm, len);
    YX_WriteSTR_Strm(&wstrm, "\r");
    
    return YX_GetStrmLen(&wstrm);
}

/*
********************************************************************************
* AT+CIPCLOSE, 关闭TCP连接
********************************************************************************
*/
INT8U AT_CMD_SocketClose(INT8U *dptr, INT32U maxlen, INT8U ch)
{
    STREAM_T wstrm;
    char const str_CIPCLOSE[] = {"AT+CIPCLOSE="};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    YX_WriteSTR_Strm(&wstrm, (char *)str_CIPCLOSE);
    YX_WriteBYTE_Strm(&wstrm, ch + 0x30);
    YX_WriteSTR_Strm(&wstrm, "\r");
    
    return YX_GetStrmLen(&wstrm);
}

/*
********************************************************************************
* AT+CIPSEND, 发送数据
********************************************************************************
*/
INT16U AT_CMD_SocketSend(INT8U *dptr, INT32U maxlen, INT8U ch, INT8U socket_type, INT8U *dataptr, INT16U datalen)
{
    STREAM_T wstrm;
    INT16U len;
    char   const str_CIPSEND[] = {"AT+CIPSEND="};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    YX_WriteSTR_Strm(&wstrm, (char *)str_CIPSEND);
    
    YX_WriteBYTE_Strm(&wstrm, ch + 0x30);
    YX_WriteSTR_Strm(&wstrm, ",");
    
    len = YX_DecToAscii(YX_GetStrmPtr(&wstrm), datalen, 0);
    YX_MovStrmPtr(&wstrm, len);
    YX_WriteSTR_Strm(&wstrm, "\r");
    
    YX_WriteDATA_Strm(&wstrm, pdata, datalen);
    
    return YX_GetStrmLen(&wstrm);
}

/*
********************************************************************************
* AT+CSTT, 启动任务并设置APN
********************************************************************************
*/
INT8U AT_CMD_SetAuthentication(INT8U *dptr, INT32U maxlen, char *apn, char *username, char *password)
{
    STREAM_T wstrm;
    char const str_CSTT[] = {"AT+CSTT=\""};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    YX_WriteSTR_Strm(&wstrm, (char *)str_CSTT);
    
    if (apn != 0) {
        YX_WriteSTR_Strm(&wstrm, apn);
    }
    YX_WriteSTR_Strm(&wstrm, "\",\"");
    
    if (username != 0) {
        YX_WriteSTR_Strm(&wstrm, username);
    }
    YX_WriteSTR_Strm(&wstrm, "\",\"");
    
    if (password != 0) {
        YX_WriteSTR_Strm(&wstrm, password);
    }
    YX_WriteSTR_Strm(&wstrm, "\"\r");
    
    return YX_GetStrmLen(&wstrm);
}

/*
********************************************************************************
* AT+CIICR, 激活移动场景
********************************************************************************
*/
INT8U AT_CMD_ActivePDPContext(INT8U *dptr, INT32U maxlen)
{
    STREAM_T wstrm;
    char const str_CIICR[] = {"AT+CIICR\r"};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    YX_WriteSTR_Strm(&wstrm, (char *)str_CIICR);

    return YX_GetStrmLen(&wstrm);
}

/*
********************************************************************************
* AT+CIPSHUT, 关闭移动场景
********************************************************************************
*/
INT8U AT_CMD_DeactivePDPContext(INT8U *dptr, INT32U maxlen)
{
    char const str_CIPSHUT[] = {"AT+CIPSHUT\r"};
    
    YX_MEMCPY(dptr, maxlen, str_CIPSHUT, sizeof(str_CIPSHUT) - 1);
    return sizeof(str_CIPSHUT) - 1;
}

/*
********************************************************************************
* AT+CIFSR, 获得本地IP地址
********************************************************************************
*/
INT8U AT_CMD_GetLocalIp(INT8U *dptr, INT32U maxlen)
{
    STREAM_T wstrm;
    char const str_CIFSR[] = {"AT+CIFSR\r"};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    YX_WriteSTR_Strm(&wstrm, (char *)str_CIFSR);
    
    return YX_GetStrmLen(&wstrm);
}

/*
********************************************************************************
* AT+CIPMUX, 设置多路IP
********************************************************************************
*/
INT8U AT_CMD_SetMultiSocket(INT8U *dptr, INT32U maxlen)
{
    STREAM_T wstrm;
    char const str_CIPMUX[] = {"AT+CIPMUX=1\r"};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    YX_WriteSTR_Strm(&wstrm, (char *)str_CIPMUX);
    
    return YX_GetStrmLen(&wstrm);
}

/*
********************************************************************************
* AT+CIPHEAD, 设置接收数据的IP头
********************************************************************************
*/
INT8U AT_CMD_SetIpHead(INT8U *dptr, INT32U maxlen)
{
    char const str_CIPHEAD[] = {"AT+CIPHEAD=1\r"};
    
    YX_MEMCPY(dptr, maxlen, str_CIPHEAD, sizeof(str_CIPHEAD) - 1);
    return sizeof(str_CIPHEAD) - 1;
}

/*
********************************************************************************
* AT+CIPSPRT, 设置在AT+CIPSEND跟发送提示符号>
********************************************************************************
*/
INT8U AT_CMD_SetSocketSendPrompt(INT8U *dptr, INT32U maxlen)
{
    char const str_CIPSPRT[] = {"AT+CIPSPRT=0\r"};
    
    YX_MEMCPY(dptr, maxlen, str_CIPSPRT, sizeof(str_CIPSPRT) - 1);
    return sizeof(str_CIPSPRT) - 1;
}

/*
********************************************************************************
* AT+CIPDPDP, 设置网络状态检测的时间间隔
********************************************************************************
*/
INT8U AT_CMD_SetPdpDetect(INT8U *dptr, INT32U maxlen)
{
    char const str_CIPDPDP[] = {"AT+CIPDPDP=1,5,5\r"};
    
    YX_MEMCPY(dptr, maxlen, str_CIPDPDP, sizeof(str_CIPDPDP) - 1);
    return sizeof(str_CIPDPDP) - 1;
}

/*
********************************************************************************
* AT+CIPQSEND, 设置GPRS数据发送方式
********************************************************************************
*/
INT8U AT_CMD_SetSocketSendStyle(INT8U *dptr, INT32U maxlen)
{
    char const str_CIPQSEND[] = {"AT+CIPQSEND=1\r"};
    
    YX_MEMCPY(dptr, maxlen, str_CIPQSEND, sizeof(str_CIPQSEND) - 1);
    return sizeof(str_CIPQSEND) - 1;
}

/*
********************************************************************************
* AT+CDNSGIP, 根据域名获取IP地址
********************************************************************************
*/
INT8U AT_CMD_GetIpByDomainName(INT8U *dptr, INT32U maxlen, char *domainname)
{
    STREAM_T wstrm;
    char const str_CDNSGIP[] = {"AT+CDNSGIP="};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    YX_WriteSTR_Strm(&wstrm, (char *)str_CDNSGIP);
    YX_WriteSTR_Strm(&wstrm, "\"");
    YX_WriteSTR_Strm(&wstrm, domainname);
    YX_WriteSTR_Strm(&wstrm, "\"\r");
    
    return YX_GetStrmLen(&wstrm);
}

/*
********************************************************************************
* AT+CDNSCFG, 设置域名解析服务器地址
********************************************************************************
*/
INT8U AT_CMD_SetDNS(INT8U *dptr, INT32U maxlen, char *pri_ip, char *sec_ip)
{
    STREAM_T wstrm;
    char const str_CDNSCFG[] = {"AT+CDNSCFG="};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    YX_WriteSTR_Strm(&wstrm, (char *)str_CDNSCFG);
    YX_WriteSTR_Strm(&wstrm, "\"");
    YX_WriteSTR_Strm(&wstrm, pri_ip);
    YX_WriteSTR_Strm(&wstrm, "\",\"");
    YX_WriteSTR_Strm(&wstrm, sec_ip);
    YX_WriteSTR_Strm(&wstrm, "\"\r");
    
    return YX_GetStrmLen(&wstrm);
}

#endif /* end of SIM800V */

//#if GSM_TYPE == GSM_SIM6320
#if 1

/*
********************************************************************************
* Handler:  获取本地IP地址
********************************************************************************
*/
static INT8U Handler_GetLocalIp(INT8U *recvbuf, INT16U recvlen)
{
    INT16U pos, iplen;
    
    YX_MEMSET(&g_at_ack_info, 0, sizeof(g_at_ack_info));
    if (!YX_SearchKeyWord(recvbuf, recvlen, "ERROR")) {
        pos = YX_FindCharPos(recvbuf, ':', 0, recvlen);
        iplen = YX_FindCharPos(recvbuf, '\r', 0, recvlen);
        iplen = iplen - pos - 1;
        if (iplen < sizeof(g_at_ack_info.localip.ip)) {
            YX_MEMCPY(g_at_ack_info.localip.ip, iplen, &recvbuf[pos + 1], iplen);
            return AT_SUCCESS;
        } else {
            return AT_FAILURE;
        }
    } else {
        return AT_FAILURE;
    }
}

AT_CMD_PARA_T const g_socket_sethead_para     = { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_socket_multi_para       = { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_socket_sendprompt_para  = { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_socket_sendstyle_para   = { 0,                4,  1,  1,  Handler_Common      };

AT_CMD_PARA_T const g_socket_connect_para     = { 0,                30, 1,  2,  Handler_Common      };
AT_CMD_PARA_T const g_socket_close_para       = { 0,                30, 1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_socket_query_para       = { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_socket_send_para        = { ATCMD_BREAK,      4,  2,  1,  Handler_Common      };

AT_CMD_PARA_T const g_pdp_setauth_para        = { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_pdp_active_para         = { 0,                30, 1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_pdp_deactive_para       = { 0,                30, 1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_pdp_getlocalip_para     = { 0,                4,  1,  2,  Handler_GetLocalIp  };
AT_CMD_PARA_T const g_pdp_setdetect_para      = { 0,                4,  1,  1,  Handler_Common      };

AT_CMD_PARA_T const g_dns_getipbyname_para    = { 0,                20, 1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_dns_setdns_para         = { 0,                10, 1,  1,  Handler_Common      };

/*
********************************************************************************
* 建立socket连接
********************************************************************************
*/
INT8U AT_CMD_SocketConnect(INT8U *dptr, INT32U maxlen, INT8U ch, INT8U socket_type, char *ip, INT16U port)
{
    STREAM_T wstrm;
    INT8U len;
    char const str_text[] = {"AT+CIPOPEN="};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    YX_WriteSTR_Strm(&wstrm, (char *)str_text);
    
    YX_WriteBYTE_Strm(&wstrm, ch + 0x30);
    
    if (socket_type == 0) {
        YX_WriteSTR_Strm(&wstrm, ",\"TCP\",\"");
    } else {
        YX_WriteSTR_Strm(&wstrm, ",\"UDP\",\"");
    }
    
    YX_WriteSTR_Strm(&wstrm, ip);
    YX_WriteSTR_Strm(&wstrm, "\",");
    
    len = YX_DecToAscii(YX_GetStrmPtr(&wstrm), port, 0);
    YX_MovStrmPtr(&wstrm, len);
    YX_WriteSTR_Strm(&wstrm, "\r");
    
    return YX_GetStrmLen(&wstrm);
}

/*
********************************************************************************
* 关闭socket连接
********************************************************************************
*/
INT8U AT_CMD_SocketClose(INT8U *dptr, INT32U maxlen, INT8U ch)
{
    STREAM_T wstrm;
    char const str_text[] = {"AT+CIPCLOSE="};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    YX_WriteSTR_Strm(&wstrm, (char *)str_text);
    YX_WriteBYTE_Strm(&wstrm, ch + 0x30);
    YX_WriteSTR_Strm(&wstrm, "\r");
    
    return YX_GetStrmLen(&wstrm);
}

/*
********************************************************************************
* 查询socket状态
********************************************************************************
*/
INT8U AT_CMD_QuerySocketStatus(INT8U *dptr, INT32U maxlen, INT8U ch)
{
    STREAM_T wstrm;
    char const str_text[] = {"AT+CIPCLOSE?"};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    YX_WriteSTR_Strm(&wstrm, (char *)str_text);
    YX_WriteSTR_Strm(&wstrm, "\r");
    
    return YX_GetStrmLen(&wstrm);
}

/*
********************************************************************************
* 发送socket数据
********************************************************************************
*/
INT16U AT_CMD_SocketSend(INT8U *dptr, INT32U maxlen, INT8U ch, INT8U socket_type, INT8U *dataptr, INT16U datalen)
{
    STREAM_T wstrm;
    INT16U len;
    char   const str_text[] = {"AT+CIPSEND="};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    YX_WriteSTR_Strm(&wstrm, (char *)str_text);
    
    YX_WriteBYTE_Strm(&wstrm, ch + 0x30);
    YX_WriteSTR_Strm(&wstrm, ",");
    
    len = YX_DecToAscii(YX_GetStrmPtr(&wstrm), datalen, 0);
    YX_MovStrmPtr(&wstrm, len);
    YX_WriteSTR_Strm(&wstrm, "\r");
    
    YX_WriteDATA_Strm(&wstrm, dataptr, datalen);
    
    return YX_GetStrmLen(&wstrm);
}

/*
********************************************************************************
* 设置APN、用户名和密码
********************************************************************************
*/
INT8U AT_CMD_SetAuthentication(INT8U *dptr, INT32U maxlen, INT8U cid, char *apn, char *username, char *password)
{
    INT8U service;
    STREAM_T wstrm;
    char const str_text_user[] = {"AT+CSOCKAUTH=,,\""};
    char const str_text_apn[]  = {"AT+CGSOCKCONT=1,\"IP\",\""};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    service = ADP_NET_GetOperator();
    if (service == NETWORK_OP_CMCC || service == NETWORK_OP_UNICOM) {          /* 移动或者联通采用APN */
        YX_WriteSTR_Strm(&wstrm, (char *)str_text_apn);
        if (apn != 0) {
            YX_WriteSTR_Strm(&wstrm, apn);
        }
    } else {
        YX_WriteSTR_Strm(&wstrm, (char *)str_text_user);
        if (username != 0) {
            YX_WriteSTR_Strm(&wstrm, username);
        }
        YX_WriteSTR_Strm(&wstrm, "\",\"");
    
        if (password != 0) {
            YX_WriteSTR_Strm(&wstrm, password);
        }
    }
    YX_WriteSTR_Strm(&wstrm, "\"\r");
    return YX_GetStrmLen(&wstrm);
}

/*
********************************************************************************
* 激活GPRS上下文(Activate the GPRS PDP context)
********************************************************************************
*/
INT8U AT_CMD_ActivePDPContext(INT8U *dptr, INT32U maxlen)
{
    STREAM_T wstrm;
    //char const str_text[] = {"AT+NETOPEN=,,1\r"};
    char const str_text[] = {"AT+NETOPEN\r"};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    YX_WriteSTR_Strm(&wstrm, (char *)str_text);

    return YX_GetStrmLen(&wstrm);
}

/*
********************************************************************************
* 去活GPRS PDP上下文(Deactivate GPRS PDP context)
********************************************************************************
*/
INT8U AT_CMD_DeactivePDPContext(INT8U *dptr, INT32U maxlen)
{
    STREAM_T wstrm;
    char const str_text[] = {"AT+NETCLOSE\r"};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    YX_WriteSTR_Strm(&wstrm, (char *)str_text);

    return YX_GetStrmLen(&wstrm);
}

/*
********************************************************************************
* 获取本地IP地址
********************************************************************************
*/
INT8U AT_CMD_GetLocalIp(INT8U *dptr, INT32U maxlen)
{
    STREAM_T wstrm;
    char const str_text[] = {"AT+IPADDR\r"};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    YX_WriteSTR_Strm(&wstrm, (char *)str_text);
    
    return YX_GetStrmLen(&wstrm);
}

/*
********************************************************************************
* 设置网络状态检测的时间间隔
********************************************************************************
*/
INT8U AT_CMD_SetPdpDetect(INT8U *dptr, INT32U maxlen)
{
    STREAM_T wstrm;
    char const str_text[] = {"AT\r"};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    YX_WriteSTR_Strm(&wstrm, (char *)str_text);

    return YX_GetStrmLen(&wstrm);
}

/*
********************************************************************************
* 设置接收数据的IP头
********************************************************************************
*/
INT8U AT_CMD_SetIpHead(INT8U *dptr, INT32U maxlen)
{
    INT8U moduletype;
    STREAM_T wstrm;
    char const str_text_sim6320[] = {"AT+CIPCCFG=,,0,1,1\r"};
    char const str_text_m12[] = {"AT+CIPHEAD=1\r"};
    char const str_text[] = {"AT+CIPCCFG=10,0,0,1,1,0,500\r"};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    
    moduletype = ADP_NET_GetModuleType();
    if (moduletype == MODULE_TYPE_SIM6320) {
        YX_WriteSTR_Strm(&wstrm, (char *)str_text_sim6320);
    } else if (moduletype == MODULE_TYPE_M12) {
        YX_WriteSTR_Strm(&wstrm, (char *)str_text_m12);
    } else {
        YX_WriteSTR_Strm(&wstrm, (char *)str_text);
    }
    
    return YX_GetStrmLen(&wstrm);
}

/*
********************************************************************************
* 设置多路socket
********************************************************************************
*/
INT8U AT_CMD_SetMultiSocket(INT8U *dptr, INT32U maxlen)
{
    INT8U moduletype;
    STREAM_T wstrm;
    char const str_text[] = {"AT\r"};
    char const str_text_m12[] = {"AT+CIPMUX=1\r"};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    moduletype = ADP_NET_GetModuleType();
    if (moduletype == MODULE_TYPE_M12) {
        YX_WriteSTR_Strm(&wstrm, (char *)str_text_m12);
    } else {
        YX_WriteSTR_Strm(&wstrm, (char *)str_text);
    }

    return YX_GetStrmLen(&wstrm);
}

/*
********************************************************************************
* 设置在AT+CIPSEND跟发送提示符号>
********************************************************************************
*/
INT8U AT_CMD_SetSocketSendPrompt(INT8U *dptr, INT32U maxlen)
{
    INT8U moduletype;
    STREAM_T wstrm;
    char const str_text[] = {"AT\r"};
    char const str_text_m12[] = {"AT+CIPSPRT=0\r"};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    moduletype = ADP_NET_GetModuleType();
    if (moduletype == MODULE_TYPE_M12) {
        YX_WriteSTR_Strm(&wstrm, (char *)str_text_m12);
    } else {
        YX_WriteSTR_Strm(&wstrm, (char *)str_text);
    }
    return YX_GetStrmLen(&wstrm);
}

/*
********************************************************************************
* 设置socket数据发送方式(快速方式、不等待send ok)
********************************************************************************
*/
INT8U AT_CMD_SetSocketSendStyle(INT8U *dptr, INT32U maxlen)
{
    INT8U moduletype;
    STREAM_T wstrm;
    char const str_text[] = {"AT+CIPCCFG=10,0,0,1,1,0,500\r"};
    char const str_text_sim6320[] = {"AT+CIPCCFG=,,0,1,1\r"};
    char const str_text_m12[] = {"AT+CIPQSEND=1\r"};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    moduletype = ADP_NET_GetModuleType();
    if (moduletype == MODULE_TYPE_SIM6320) {
        YX_WriteSTR_Strm(&wstrm, (char *)str_text_sim6320);
    } else if (moduletype == MODULE_TYPE_M12) {
        YX_WriteSTR_Strm(&wstrm, (char *)str_text_m12);
    } else {
        YX_WriteSTR_Strm(&wstrm, (char *)str_text);
    }
    return YX_GetStrmLen(&wstrm);
}

/*
********************************************************************************
* 根据域名获取IP地址
********************************************************************************
*/
INT8U AT_CMD_GetIpByDomainName(INT8U *dptr, INT32U maxlen, char *domainname)
{
    STREAM_T wstrm;
    char const str_text[] = {"AT+CDNSGIP="};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    YX_WriteSTR_Strm(&wstrm, (char *)str_text);
    YX_WriteSTR_Strm(&wstrm, "\"");
    YX_WriteSTR_Strm(&wstrm, domainname);
    YX_WriteSTR_Strm(&wstrm, "\"\r");
    
    return YX_GetStrmLen(&wstrm);
}

/*
********************************************************************************
* 设置域名解析服务器地址
********************************************************************************
*/
INT8U AT_CMD_SetDNS(INT8U *dptr, INT32U maxlen, char *pri_ip, char *sec_ip)
{
    STREAM_T wstrm;
    char const str_text[] = {"AT+CDNSCFG="};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    YX_WriteSTR_Strm(&wstrm, (char *)str_text);
    YX_WriteSTR_Strm(&wstrm, "\"");
    YX_WriteSTR_Strm(&wstrm, pri_ip);
    YX_WriteSTR_Strm(&wstrm, "\",\"");
    YX_WriteSTR_Strm(&wstrm, sec_ip);
    YX_WriteSTR_Strm(&wstrm, "\"\r");
    
    return YX_GetStrmLen(&wstrm);
}

#endif /* end of SIM6320 */

#endif
