/**************************************************************************************************
**                                                                                               **
**  文件名称:  appmain.h                                                                         **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2010                                    **
**  创建信息:  Lantu.Cai -- 2010年12月6日                                                        **
**  文件描述:  应用程序头文件                                                                    **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/

#ifndef  _APPMAIN_H
#define  _APPMAIN_H

#include  "app_include.h"
#include  "man_queue.h"
#include  "yx_version.h"

#define  SIZE_APPMSG_BUF         50

#define  CLIENT_CODE             0x0918


/* Exported macro ------------------------------------------------------------*/
/*************************************************************************************************/
/*                             总线类型枚举                                                      */
/*************************************************************************************************/
typedef enum {
    BUS_NONE = 0x00,                                                      /* 总线类型：无        */
    BUS_CAN  = 0x01,                                                      /* 总线类型：CAN       */
    BUS_485  = 0x02,                                                      /* 总线类型：485       */
    BUS_BOTH = 0x03                                                       /* 总线类型：两种都有  */
} BUS_TYPE_E;

/*************************************************************************************************/
/*                           总线复位类型枚举                                                    */
/*************************************************************************************************/
typedef enum {
    BUS_HARD_RESET = 0x01,
    BUS_SOFT_RESET = 0x02
} BUS_RESET_E;


#ifdef  APPMAIN_G
#define APPMAIN_EXT
#else
#define APPMAIN_EXT  extern
#endif


APPMAIN_EXT  QCB_T   *g_appQ;
APPMAIN_EXT  QMSG_T   g_appMsg;

void Hdl_MSG_COM_SEND(void);
//void Hdl_MSG_OPT_GSEN_HIT(INT16U tskid, INT16U msgid, INT32U lpara, INT32U hpara);
//void Hdl_MSG_OPT_GSEN_MOTION(INT16U tskid, INT16U msgid, INT32U lpara, INT32U hpara);
void Hdl_MSG_RTCWAKE(void);
void Hdl_MSG_OTWAKE(void);
void AppStart(void);
void Turnoff_ReStartDevice(void);
BUS_TYPE_E GetBusType(INT8U channel);
void Call_Circulate(INT16U m);

#endif /* _APPMAIN_H */
/************************ (C) COPYRIGHT 2010 XIAMEN YAXON.LTD *******************END OF FILE******/

