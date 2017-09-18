/********************************************************************************
**
** 文件名:     at_cmd_sms.h
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
#ifndef AT_CMD_SM_H
#define AT_CMD_SM_H          1




/*********************************************************************************
**                                                                               *
**                                                                               *
**                 AT Commands parameters                                        *
**                                                                               *
**                                                                               *
*********************************************************************************/
extern AT_CMD_PARA_T const g_sms_setformat_para;
extern AT_CMD_PARA_T const g_sms_setcsca_para;
extern AT_CMD_PARA_T const g_sms_getcsca_para;
extern AT_CMD_PARA_T const g_sms_setcsms_para;
extern AT_CMD_PARA_T const g_sms_setcscs_para;
extern AT_CMD_PARA_T const g_sms_setcnma_para;
extern AT_CMD_PARA_T const g_sms_setcnmi_para;
extern AT_CMD_PARA_T const g_sms_getcnmi_para;
extern AT_CMD_PARA_T const g_sms_sendhead_para;
extern AT_CMD_PARA_T const g_sms_senddata_para;
extern AT_CMD_PARA_T const g_sms_delete_para;
extern AT_CMD_PARA_T const g_sms_list_para;
extern AT_CMD_PARA_T const g_sms_read_para;
extern AT_CMD_PARA_T const g_sms_cpms_para;



/*********************************************************************************
**                                                                               *
**                                                                               *
**                 AT Commands originating from GSM 07.05 for SMS                *
**                                                                               *
**                                                                               *
*********************************************************************************/
INT8U   AT_CMD_SetSmsFormat(INT8U *dptr, INT32U maxlen);
INT8U   AT_CMD_SetSmscTel(INT8U *dptr, INT32U maxlen, INT8U  *tel, INT8U tellen);
INT8U   AT_CMD_GetSmscTel(INT8U *dptr, INT32U maxlen);
INT8U   AT_CMD_SetMessageService(INT8U *dptr, INT32U maxlen);
INT8U   AT_CMD_SetSmsCharactorSet(INT8U *dptr, INT32U maxlen);
INT8U   AT_CMD_SetSmsAck(INT8U *dptr, INT32U maxlen);
INT8U   AT_CMD_SetSmsIndication(INT8U *dptr, INT32U maxlen);
INT8U   AT_CMD_GetSmsIndication(INT8U *dptr, INT32U maxlen);
INT8U   AT_CMD_SendSm(INT8U *dptr, INT32U maxlen, INT8U pdulen);
INT8U   AT_CMD_DeleteSm(INT8U *dptr, INT32U maxlen, INT8U index);
INT8U   AT_CMD_ReadSm(INT8U *dptr, INT32U maxlen, INT8U index);
INT8U   AT_CMD_ListSm(INT8U *dptr, INT32U maxlen);
INT8U   AT_CMD_SelectSmStorage(INT8U *dptr, INT32U maxlen);

#endif
