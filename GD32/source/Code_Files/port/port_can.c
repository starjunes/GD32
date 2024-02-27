/********************************************************************************
** 文件名:     port_can.c
** 版权所有:   (c) 2017 厦门雅迅网络股份有限公司
** 文件描述:   can收发器接口
** 创建人：        谢金成，2017.5.9
*********************************************************************************
**                  修改历史记录
**===============================================================================
**| 日期              | 作者        |  修改记录
**===============================================================================
**| 2017/05/12 | 黄运峰    |  代码规范化，修改优化
********************************************************************************/
#include "port_can.h"
#include "dal_can.h"
#include "dal_infrared.h"
#include "tools.h"
#include "dal_Infrared.H"
/********************************************************************************
**  函数名:     PORT_InitCan
**  函数描述:   CAN初始化dal层的CAN收发器
**  参数:       无
**  返回:       无
********************************************************************************/
void PORT_InitCan(void)
{
    Dal_CAN_Init();
}

/********************************************************************************
**  函数名:     PORT_OpenCan
**  函数描述:   CAN初始化
**  参数:       [in] chn:通道号
**              [in] baud:波特率
**              [in] fmat: can帧格式(标准帧/扩展帧)
**  返回:       成功返回true，失败返回false
********************************************************************************/
BOOLEAN PORT_OpenCan(CAN_CHN_E chn, APPCAN_BAUD_E baud, INT8U fmat)
{
    CAN_ATTR_T CAN_InitStruct;

    if (fmat == FMAT_EXT) {
        CAN_InitStruct.type = _FRAME_EXT;
    } else if (fmat == FMAT_STD) {
        CAN_InitStruct.type = _FRAME_STD;
    } else {
        return false;
    }
    CAN_InitStruct.fmat = _FRAME_DATA;
    CAN_InitStruct.test_mode = CAN_NORMAL_MODE;
    if (baud == CAN_BAUD_10K) {
        CAN_InitStruct.baud = CAN_10;
    } else if (baud == CAN_BAUD_20K) {
        CAN_InitStruct.baud = CAN_20;
    } else if (baud == CAN_BAUD_50K) {
        CAN_InitStruct.baud = CAN_50;
    } else if (baud == CAN_BAUD_100K) {
        CAN_InitStruct.baud = CAN_100;
    } else if (baud == CAN_BAUD_125K) {
        CAN_InitStruct.baud = CAN_125;
    } else if (baud == CAN_BAUD_250K) {
        CAN_InitStruct.baud = CAN_250;
    } else if (baud == CAN_BAUD_500K) {
        CAN_InitStruct.baud = CAN_500;
    } else if (baud == CAN_BAUD_1000K) {
        CAN_InitStruct.baud = CAN_1000;
    } else {
        return false;
    }
    CAN_CommParaSet(&CAN_InitStruct, chn);
    //CAN_OnOFFCtrl(true, chn);
    return true;
}

/********************************************************************************
**  函数名:     PORT_CanEnable
**  函数描述:   使能CAN
**  参数:       [in] chn：通道号
**  返回:       无
********************************************************************************/
void PORT_CanEnable(CAN_CHN_E chn, BOOLEAN enble)
{
	if (!enble) {
	    CAN_OnOFFCtrl(false, chn);
	} else {
	    CAN_OnOFFCtrl(true, chn);
	}
}

/********************************************************************************
**  函数名:     PORT_CloseCan
**  函数描述:   关闭CAN
**  参数:       [in] chn：通道号
**  返回:       无
********************************************************************************/
void PORT_SetCanbaud(CAN_CHN_E chn, APPCAN_BAUD_E baud)
{
    //hal_can_SetBaud(chn, baud);
}
/********************************************************************************
**  函数名:     PORT_CanSend
**  函数描述:   CAN数据发送处理
**  参数:       [in] sdata: port层can数据结构体指针，应包括端口，ID，帧格式，周期参数，数据长度，数据内容等
**              注意:如果周期有效同时数据长度为0，则为远程帧。
**  返回:       发送结果
********************************************************************************/
BOOLEAN PORT_CanSend(CAN_DATA_SEND_T *sdata)
{
	//return dal_can_Send((CAN_SENDDATA_T *)sdata);
	INT8U data[13] = {0x00,0x00,0x00,0x00,0x08,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
	//debug_printf("PORT_CanSend period%x\r\n",sdata->period);
    Longtochar(data, sdata->can_id);
    data[4] = sdata->can_DLC;
    memcpy(&data[5], sdata->Data, sdata->can_DLC);
	if (sdata->period == 0xffff) {
	    return DAL_CAN_TxData(data, FALSE, sdata->channel);
	} else if (sdata->period == 0) {
	    return Dal_StopCANMsg_Period(sdata->can_id, sdata->channel);
	} else {
	    return Dal_SetCANMsg_Period(sdata->can_id, &data[4], sdata->period, sdata->channel);
	}
}

/********************************************************************************
**  函数名:     PORT_ClearCanFilter
**  函数描述:   清空CAN滤波组
**  参数:       [in] chn：通道号
**  返回:       无
********************************************************************************/
void PORT_ClearCanFilter(CAN_CHN_E chn)
{
    INT8U i;
    
	for (i = 0; i < MAX_RXIDOBJ; i++) {                      /* 清除所有配置过的ID滤波对象 */
        CAN_ClearFilterPara(i, chn);                         /* 清除消息对象 */
    }
}

/********************************************************************************
**  函数名:     PORT_SetCanFilter
**  函数描述:   CAN滤波设置
**  参数:       [in] chn:通道号
**              [in] fmat: 帧格式，0：标准帧，1：扩展帧
**              [in] filtid: 滤波id
**              [in] maskid: 掩码id
**  返回:       设置结果
********************************************************************************/
BOOLEAN PORT_SetCanFilter(CAN_CHN_E chn, INT8U fmat, INT32U filtid, INT32U maskid)
{
	return CAN_RxFilterConfig(filtid, maskid, CAN_FILTER_ON, chn);
}

/********************************************************************************
**  函数名:     PORT_AddCanID
**  函数描述:   CAN增加接收ID
**  参数:       [in] chn:通道号
**              [in] fmat: 帧格式，0：标准帧，1：扩展帧
**              [in] id: 具体id值
**  返回:       发送结果
********************************************************************************/
BOOLEAN PORT_AddCanID(CAN_CHN_E chn, INT8U fmat, INT32U id)
{
	return CAN_RxFilterConfig(id, 0xffffffff, CAN_FILTER_ON, chn);
}

/********************************************************************************
**  函数名:     PORT_RegCanCallbakFunc
**  函数描述:   CAN回调函数注册
**  参数:
**             [in] handle: 回调函数
**  返回:      无
********************************************************************************/
void PORT_RegCanCallbakFunc(CAN_CALLBAK_HDL handle)
{
    //dal_can_lbrepregist((CAN_CALLBAK_FP)handle);
    Dal_CANLBRepReg(LB_ANALYZE, handle);
}

void PORT_CanSeqCFSendCallbakFunc(CAN_SEQCFSEND_HDL handle)
{
    dal_CanSeqCFSendCallbakFunc(handle);
}

#if EN_CAN_ERR_INT > 0
/********************************************************************************
** 函数名:     PORT_GetBusOffFlag
** 函数描述:   查询bus off 状态
** 参数:       [in] ch:can通道
** 返回:       TRUE bus off
********************************************************************************/
BOOLEAN PORT_GetBusOffFlag(INT8U ch)
{
    return hal_can_getbusoffflag(ch);
}

/********************************************************************************
** 函数名:     PORT_ClrBusOffFlag
** 函数描述:   查询bus off 状态
** 参数:       [in] ch:can通道
** 返回:       TRUE bus off
********************************************************************************/
void PORT_ClrBusOffFlag(INT8U ch)
{
    hal_can_clrbusoffflag(ch);
}

/********************************************************************************
** 函数名:     PORT_GetTxErrCnt
** 函数描述:   查询Transmit Error Counter
** 参数:       [in] ch:can通道
** 返回:       TRUE bus off
********************************************************************************/
INT8U PORT_GetTxErrCnt(INT8U ch)
{
    return hal_can_gettxerrcnt(ch);
}

/********************************************************************************
** 函数名:     PORT_GetESR1
** 函数描述:   查询ESR1寄存器状态
** 参数:       [in] ch:can通道
** 返回:
********************************************************************************/
INT32U PORT_GetESR1(INT8U ch)
{
    return hal_can_getESR1(ch);
}

/********************************************************************************
** 函数名:     PORT_BusOffManual
** 函数描述:   设置手动清除bus off 状态
** 参数:       [in] ch:can通道
** 返回:
********************************************************************************/
void PORT_BusOffManual(INT8U ch)
{
    //hal_can_busoffmanual();
}

/*****************************************************************************
**  函数名:  Port_CanGetErrStatus
**  函数描述: 获取can通道错误状态
**  参数:    [in] ch : 通道号
**  返回:    错误状态值
*****************************************************************************/
INT32U Port_CanGetErrStatus(INT8U ch)
{
    return hal_can_get_err_status(ch);
}

/*****************************************************************************
**  函数名:  Port_CanClrErrStatus
**  函数描述: 清除can通道错误状态
**  参数:    [in] ch : 通道号
**  返回:    无
*****************************************************************************/
void Port_CanClrErrStatus(INT8U ch)
{
    hal_can_clr_err_status(ch);
}
#endif
//------------------------------------------------------------------------------
/* End of File */


