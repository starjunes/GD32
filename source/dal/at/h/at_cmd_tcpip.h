/********************************************************************************
**
** 文件名:     at_cmd_tcpip.h
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
#ifndef AT_CMD_TCPIP_H
#define AT_CMD_TCPIP_H       1


#if EN_AT > 0


extern AT_CMD_PARA_T const g_socket_connect_para;
extern AT_CMD_PARA_T const g_socket_close_para;
extern AT_CMD_PARA_T const g_socket_query_para;
extern AT_CMD_PARA_T const g_socket_send_para;

extern AT_CMD_PARA_T const g_pdp_setauth_para;
extern AT_CMD_PARA_T const g_pdp_active_para;
extern AT_CMD_PARA_T const g_pdp_deactive_para;
extern AT_CMD_PARA_T const g_pdp_getlocalip_para;
extern AT_CMD_PARA_T const g_pdp_setdetect_para;

extern AT_CMD_PARA_T const g_socket_sethead_para;
extern AT_CMD_PARA_T const g_socket_multi_para;
extern AT_CMD_PARA_T const g_socket_sendprompt_para;
extern AT_CMD_PARA_T const g_socket_sendstyle_para;

extern AT_CMD_PARA_T const g_dns_getipbyname_para;
extern AT_CMD_PARA_T const g_dns_setdns_para;






INT8U AT_CMD_SocketConnect(INT8U *dptr, INT32U maxlen, INT8U ch, INT8U socket_type, char *ip, INT16U port);
INT8U AT_CMD_SocketClose(INT8U *dptr, INT32U maxlen, INT8U ch);
INT8U AT_CMD_QuerySocketStatus(INT8U *dptr, INT32U maxlen, INT8U ch);
INT16U AT_CMD_SocketSend(INT8U *dptr, INT32U maxlen, INT8U ch, INT8U socket_type, INT8U *dataptr, INT16U datalen);

INT8U AT_CMD_SetAuthentication(INT8U *dptr, INT32U maxlen, INT8U cid, char *apn, char *username, char *password);
INT8U AT_CMD_ActivePDPContext(INT8U *dptr, INT32U maxlen);
INT8U AT_CMD_DeactivePDPContext(INT8U *dptr, INT32U maxlen);
INT8U AT_CMD_GetLocalIp(INT8U *dptr, INT32U maxlen);
INT8U AT_CMD_SetPdpDetect(INT8U *dptr, INT32U maxlen);

INT8U AT_CMD_SetIpHead(INT8U *dptr, INT32U maxlen);
INT8U AT_CMD_SetMultiSocket(INT8U *dptr, INT32U maxlen);
INT8U AT_CMD_SetSocketSendPrompt(INT8U *dptr, INT32U maxlen);
INT8U AT_CMD_SetSocketSendStyle(INT8U *dptr, INT32U maxlen);

INT8U AT_CMD_GetIpByDomainName(INT8U *dptr, INT32U maxlen, char *domainname);
INT8U AT_CMD_SetDNS(INT8U *dptr, INT32U maxlen, char *pri_ip, char *sec_ip);

#endif
#endif
