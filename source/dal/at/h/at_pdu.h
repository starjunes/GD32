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
#ifndef PDUMODE_H
#define PDUMODE_H            1



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
    INT8U   PDUtype;                    /* Protocal data unit type */
    INT8U   DCS;                        /* Data coding scheme */
    INT8U   PID;                        /* Protocal Identifier */
    INT8U   timezone;                   /* time zone */
    INT8U   SCTS[12];                   /* Service center time stamp */
    INT8U   SCAL;                       /* Service Center address length */
    INT8U   SCA[22];                    /* Service Center address */
    INT8U   OAL;                        /* Originator address length */
    INT8U   OA[22];                     /* Originator address */
    INT8U   UDL;                        /* User data length */
    INT8U   UD[SHELL_SMS_LEN];          /* User data */
} SM_T;

BOOLEAN ParsePDUData(SM_T *desSM, INT8U *orgdata, INT16U orglen, INT16U pdulen);
INT16U AssemblePDUData(INT8U DCS, INT8U *telptr, INT8U tellen, INT8U *dataptr, INT32U datalen, INT8U *dptr, INT32U dlen);

INT8U  GsmCodeToASCII(INT8U incode);
INT8U  ASCIIToGsmCode(INT8U incode);


#endif
