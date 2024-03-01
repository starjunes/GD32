/*
********************************************************************************
** 文件名:     yx_protocal_hdl.h
** 版权所有:   (c) 2017 厦门雅迅网络股份有限公司
** 文件描述:   协议命令对应的功能函数头文件
** 创建人：        谢金成，2017.5.18
********************************************************************************
*/
#ifndef  _YX_PROTOCAL_HDL_H
#define  _YX_PROTOCAL_HDL_H

void LinkRequstAck_Hdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen);
void BeatRequstAck_Hdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen);
void PowerControl_Hdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen);
void VersionNumberReqHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen);
void Relink_RequestHdl(INT8U mancode, INT8U command, INT8U *data, INT16U datalen);
void DeviceConfigurationQueryHdl(INT8U mancode, INT8U command, INT8U *data, INT16U datalen);

/******************************************************************************/
/*                           复位设备                                                                                                    */
/******************************************************************************/
void ResetReq_Hdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen);

void RTC_Synchro_Hdl(INT8U mancode, INT8U command, INT8U *userdata, INT16U userdatalen);
void EquipmentStatusReq(INT8U mancode, INT8U command, INT8U *data, INT16U datalen);
void RealTimeStatusReport_AckHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen);

void ReqChgMainComBaudAck_Hdl(INT8U mancode, INT8U command,  INT8U *data, INT16U datalen);

void GprsSetAns(INT8U mancode, INT8U command, INT8U *data, INT16U datalen);
void GprsQueAns(INT8U mancode, INT8U command,INT8U *data, INT16U datalen);

void SigFilterParaConfig_ReqHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen);
void SigChgInfomONOFF_ReqHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen);
void SigChgInfom_AckHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen);

/******************************************************************************/
/*                           数据透传                                                       */
/******************************************************************************/
void EXUsartParaSetReqHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen);
void EXUsartParaQueryHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen);
void EXUsartPowerCtrlHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen);
void ExUartData_Down_Hdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen);
void ExUartData_ReqAck_Hdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen);

/******************************************************************************/
/*                           无线升级                                                                                                    */
/******************************************************************************/
void StartUpdateReq_Hdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen);       
void UpdateDadaRecv_Hdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen); 


/******************************************************************************/
/*                           CAN通信                                                                                                     */
/******************************************************************************/
void BusTypeSetReqHdl(INT8U mancode,  INT8U command, INT8U *data, INT16U datalen);
void BusOnOffReqHdl(INT8U mancode,INT8U command, INT8U *data, INT16U datalen);

void CANCommParaSetReqHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen);
void CANWorkModeSetHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen);
void CANWorkModeQueryHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen);
void CANFilterIDSetReqHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen);
void CANFilterIDQueryHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen);
void CANDataTransReqHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen);
void CANDataReportAckHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen);
void CANScreenIDSetReqHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen);
void CANDataLucidlyAckHdl(INT8U mancode, INT8U command, INT8U *data, INT16U datalen);
void GetCANDataReqHdl(INT8U mancode, INT8U command, INT8U *data, INT16U datalen);


/******************************************************************************/
/*                           碰撞检测                                                                   */
/******************************************************************************/
void CrashBDHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen);
void CrashSetParaHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen);
void CrashReportAckHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen);


/******************************************************************************/
/*                           ADC采集                                                      */
/******************************************************************************/
void ADCSetReqHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen);
void GetADCDataHdl(INT8U mancode, INT8U command, INT8U *data, INT16U datalen);
void ADCDataRepAckHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen);

/******************************************************************************/
/*                         工装检测                                                     */
/******************************************************************************/

void GZ_TestReq_Hdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen);


/******************************************************************************/
/*                              客户特殊协议处理                                                       */
/******************************************************************************/

void Client_FuntionUpAck_Hdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen);
void Client_FuntionDown_Hdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen);









#if 0

void DeviceConfigurationQueryHdl(INT8U *userdata, INT16U userdatalen);
void ReStartReq_Hdl(INT8U *userdata, INT16U userdatalen);
void ReqChgMainComBaudAck_Hdl(INT8U *userdata, INT16U userdatalen);     /*0x93*/
void EquipmentParaReqAck_Hdl(INT8U *userdata, INT16U userdatalen);      /* 0x49 */

void RealTimeStatusReport_AckHdl(INT8U *userdata, INT16U userdatalen);  /*0x91*/
void SigChgInfomONOFF_ReqHdl(INT8U *userdata, INT16U userdatalen);      /*0xcb*/
void SigChgInfom_AckHdl(INT8U *userdata, INT16U userdatalen);           /*0xcc*/
void SigFilterParaConfig_ReqHdl(INT8U *userdata, INT16U userdatalen);   /*0xd6*/

#endif



#if EN_EXPDATA > 0
void ReqNeedExportData_AckHdl(INT8U *userdata, INT16U userdatalen);     /*0xd3*/
void StartDataExportAck_Hdl(INT8U *userdata, INT16U userdatalen);       /*0xd4*/
void DataRecv_IndHdl(INT8U *userdata, INT16U userdatalen);              /*0xd5*/
#endif



#if EN_CAN > 0
void CANWMConfig_ReqHdl(INT8U *userdata, INT16U userdatalen);           /*0xd0*/
void CANCommOnOff_ReqHdl(INT8U *userdata, INT16U userdatalen);          /*0xd1*/
void CANDataTrans_ReqHdl(INT8U *userdata, INT16U userdatalen);          /*0xd2*/
void CANDataRecvInfor_AckHdl(INT8U *userdata, INT16U userdatalen);      /*0xd3*/
void CANDataUpload_AckHdl(INT8U *userdata, INT16U userdatalen);
#endif


#if EN_USB > 0
void MediaDel_Ack(INT8U *userdata, INT16U userdatalen);
void MediaMark_Ack(INT8U *userdata, INT16U userdatalen);
void MediaRev_Ack(INT8U *userdata, INT16U userdatalen);
void MediaExport_Ack(INT8U *userdata, INT16U userdatalen);
void MediaTransmit_Hdl(INT8U *userdata, INT16U userdatalen);
void MediaExport_Result(INT8U *userdata, INT16U userdatalen);
#endif


#if EN_USBUPDATE > 0
void FileCreate_Result(INT8U *userdata, INT16U userdatalen);
void FileTransmit_Result(INT8U *userdata, INT16U userdatalen);
#endif





/******************************************************************************/
/*                           CAN通信                                                                                                                                                        */
/******************************************************************************/
#if 0//EN_CAN > 0
void EquipmentStatusReq(INT8U *userdata, INT16U userdatalen);
void DispMessage_Hdl(INT8U *userdata, INT16U userdatalen);
void EquipmentStatus(INT8U *userdata, INT16U userdatalen);
void Register_Ack(INT8U *userdata, INT16U userdatalen);
void ClearDataRequest(INT8U *userdata, INT16U userdatalen);

/******************************************************************************/
/*                           总线配置                                                                                                                                                         */
/******************************************************************************/
void BusTypeSetReqHdl(INT8U *userdata, INT16U userdatalen);
void BusOnOffReqHdl(INT8U *userdata, INT16U userdatalen);
void BusResetReqHdl(INT8U *userdata, INT16U userdatalen);

void CANCommParaSetReqHdl(INT8U *userdata, INT16U userdatalen);
void CANCommParaQueryHdl(INT8U *userdata, INT16U userdatalen);
void CANWorkModeSetHdl(INT8U *userdata, INT16U userdatalen);
void CANWorkModeQueryHdl(INT8U *userdata, INT16U userdatalen);
void CANFilterIDSetReqHdl(INT8U *userdata, INT16U userdatalen);
void CANFilterIDQueryHdl(INT8U *userdata, INT16U userdatalen); 
void CANDataTransReqHdl(INT8U *userdata, INT16U userdatalen);
void CANDataLucidlyAckHdl(INT8U *userdata, INT16U userdatalen);
void CANDataReportAckHdl(INT8U *userdata, INT16U userdatalen);
void GetCANDataReqHdl(INT8U *userdata, INT16U userdatalen);
//void CANResceCtrlHdl(INT8U mancode, INT8U command, INT8U *userdata, INT16U userdatalen);
void CANScreenIDSetReqHdl(INT8U *userdata, INT16U userdatalen);
void TestFunctionReqHdl(INT8U *userdata, INT16U userdatalen);

void ClientFunctionAckHdl(INT8U *userdata, INT16U userdatalen);
void ClientFunctionSetHdl(INT8U *userdata, INT16U userdatalen);

/******************************************************************************/
/*                            其他                                                                               */
/******************************************************************************/
void Client_FuntionDown_Hdl(INT8U *data, INT16U datalen);

#endif


/**************************************************************************************************
**  函数名称:  ClearDataRequest
**  功能描述:  处理主机清空调度屏数据请求
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
void ClearDataRequest(INT8U mancode, INT8U command, INT8U *data, INT16U userdatalen);



#endif/* _YX_PROTOCAL_HDL_H */
/************************ (C) COPYRIGHT 2010 XIAMEN YAXON.LTD *******************END OF FILE******/
