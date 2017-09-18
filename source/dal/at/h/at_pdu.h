/********************************************************************************
**
** 文件名:     at_pdu.h
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现短信编码转换
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2011/01/15 | 叶德焰 |  修改接口函数
*********************************************************************************/
#ifndef AT_PDU_H
#define AT_PDU_H            1



/*
********************************************************************************
* define sm code type
********************************************************************************
*/
typedef enum {
    SM_DCS_BIT7 = 0x00,
    SM_DCS_BIT8 = 0x04,
    SM_DCS_UCS2 = 0x08,
    SM_DCS_MAX
} SM_DCS_E;

#define SHELL_SMS_LEN 160

typedef struct {
    INT8U   pdutype;                    /* Protocal data unit type */
    INT8U   dcs;                        /* Data coding scheme */
    INT8U   pid;                        /* Protocal Identifier */
    INT8U   timezone;                   /* 时区, 本地时间与格林尼治之间的时差, 单位: 1/4小时 */
    struct {                            /* 短消息到达短信中心的日期 */
        INT8U year;
        INT8U month;
        INT8U day;
    } date;
    struct {                           /* 短消息到达短信中心的时间 */
        INT8U hour;
        INT8U minute;
        INT8U second;
    } time;
    INT8U   scalen;                       /* Service Center address length */
    INT8U   sca[22];                    /* Service Center address */
    INT8U   oalen;                        /* Originator address length */
    INT8U   oa[22];                     /* Originator address */
    INT8U   udlen;                        /* User data length */
    INT8U   ud[SHELL_SMS_LEN];          /* User data */
} SM_T;

/*******************************************************************
** 函数名:     AT_SMS_ParsePduData
** 函数描述:   解析PDU数据格式短信
** 参数:       [out] sm: 解析后的短信内容
**             [in]  orgdata: 待解析PDU数据
**             [in]  orgdata: 待解析PDU数据长度
**             [in]  pdulen:  实际PDU数据长度
** 返回:       成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN AT_SMS_ParsePduData(SM_T *sm, INT8U *orgdata, INT16U orglen, INT16U pdulen);

/*******************************************************************
** 函数名:     AT_SMS_AssemblePduData
** 函数描述:   按照pdu格式要求对待发送的短消息进行组帧
** 参数:       [in]  dcs:     短消息编码格式
**             [in]  telptr:  接收方手机号码
**             [in]  tellen:  接收方手机号码长度
**             [in]  dataptr: 短消息内容缓冲区
**             [in]  datalen: 短消息内容长度
**             [out] dptr:    返回存放组帧后的pdu格式数据的缓冲区
**             [in]  dlen:    缓冲区最大长度
** 返回:       成功返回组帧后长度,失败返回0
********************************************************************/
INT16U AT_SMS_AssemblePduData(INT8U dcs, INT8U *telptr, INT8U tellen, INT8U *dataptr, INT32U datalen, INT8U *dptr, INT32U dlen);

INT8U  GsmCodeToASCII(INT8U incode);
INT8U  ASCIIToGsmCode(INT8U incode);


#endif
