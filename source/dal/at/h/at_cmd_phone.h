/********************************************************************************
**
** 文件名:     at_cmd_phone.h
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
#ifndef AT_CMD_PHONE_H
#define AT_CMD_PHONE_H       1


#if EN_AT_PHONE > 0

extern AT_CMD_PARA_T const g_phone_hangup_para;
extern AT_CMD_PARA_T const g_phone_pickup_para;
extern AT_CMD_PARA_T const g_phone_ringup_para;
extern AT_CMD_PARA_T const g_phone_ringup_data_para;
extern AT_CMD_PARA_T const g_phone_dtmf_para;
extern AT_CMD_PARA_T const g_phone_in_para;
extern AT_CMD_PARA_T const g_phone_query_para;
extern AT_CMD_PARA_T const g_phone_setspeaker_para;
extern AT_CMD_PARA_T const g_phone_out_para;
extern AT_CMD_PARA_T const g_phone_setchannel_para;
extern AT_CMD_PARA_T const g_phone_setmic_para;
extern AT_CMD_PARA_T const g_phone_setecho_para;
extern AT_CMD_PARA_T const g_phone_setsidetone_para;




/*********************************************************************************
**                                                                               *
**                                                                               *
**                 AT Commands originating from GSM 07.07                        *
**                                                                               *
**                                                                               *
*********************************************************************************/
INT8U AT_CMD_Hangup(INT8U *dptr, INT32U maxlen);
INT8U AT_CMD_Pickup(INT8U *dptr, INT32U maxlen);
INT8U AT_CMD_Ringup(INT8U *dptr, INT32U maxlen, BOOLEAN DataMode, INT8U  *tel, INT8U tellen);
INT8U AT_CMD_SendDtmf(INT8U *dptr, INT32U maxlen, char dtmf);
INT8U AT_CMD_SetIncomingDisplay(INT8U *dptr, INT32U maxlen);
INT8U AT_CMD_QueryPhoneStatus(INT8U *dptr, INT32U maxlen);
INT8U AT_CMD_SetOutgoingDisplay(INT8U *dptr, INT32U maxlen);
INT8U AT_CMD_SetSpkLevel(INT8U *dptr, INT32U maxlen, INT8U ch, INT8U gain);
INT8U AT_CMD_SelectChannel(INT8U *dptr, INT32U maxlen, INT8U ch);
INT8U AT_CMD_SetMicLevel(INT8U *dptr, INT32U maxlen, INT8U ch, INT8U gain);
INT8U AT_CMD_SetEchoCancel(INT8U *dptr, INT32U maxlen, INT8U ch, INT8U gain);
INT8U AT_CMD_SetSideTone(INT8U *dptr, INT32U maxlen, INT8U ch, INT8U gain);

#endif
#endif
