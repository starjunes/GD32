/*
********************************************************************************
** 文件名:     yx_com_send.h
** 版权所有:   (c) 2017 厦门雅迅网络股份有限公司
** 文件描述:   协议数据处理接口（发送）
** 创建人：        谢金成，2017.5.18
********************************************************************************
*/
#ifndef  _YX_COM_SEND_H
#define  _YX_COM_SEND_H

#include  "yx_protocal_type.h"
/******************************************************************************/
/*                发送的命令的属性，需要应答(NEED_ACK),不需要应答(NORMAL)                  */
/******************************************************************************/
#define  MAN_CODE        0x01                             /* 厂商编号 */
#define  DEV_CODE        0x0F                             /* 120R外设编码0x02 */

typedef enum {
    NEED_ACK,
    NORMAL
}SEND_ATTRIB_E;

/******************************************************************************/
/*                                 接口函数                                                                                                                                    */
/******************************************************************************/

void YX_ConfirmSend(INT8U command);
void YX_COM_Send(INT8U attrib, INT8U command, INT8U *userdata, INT16U userdatalen);
void YX_COM_DirSend(INT8U command, INT8U *userdata, INT16U userdatalen);
void YX_ComSend_Init(void);
void YX_ComSend_ListInit(void);

#endif/* _YX_PROTOCALDATA_SEND_H */
/************************ (C) COPYRIGHT 2010 XIAMEN YAXON.LTD *******************END OF FILE******/
