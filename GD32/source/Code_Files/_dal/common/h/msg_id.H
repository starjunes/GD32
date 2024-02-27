/******************** (C) COPYRIGHT 2009 XIAMEN YAXON.LTD **********************
**
** File Name          : mmi_msg_id.h
** Description        : This file define all massage id 
**
********************************************************************************
**           Important Edit History
**==============================================================================
**| 2009/08/05 | Cybero.Chan |  Create 
**
*********************************************************************************/
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef   _MSG_ID_H
#define   _MSG_ID_H

/* Includes ------------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/* queue id definition,the greater of numerical value,the lower of priority*/
typedef enum 
{
    DAL_Q_ID,                              /* device abstract layer msg queue id */                                                      
    CD_Q_ID,                               /* common drivers msg queue id */
    APP_Q_ID,                              /* app msg  queue id */
    Q_MAX_ID  
} Q_ID_E;

/* message id definition */
typedef enum
{
 /* common message id */
    //MSG_TIMER_OVERTIME,
 /* dal message id */
 /* commdrv message id */
    MSG_CD_FILESYS_INIT,
    MSG_CD_FILESYS_FREE,
 /* app message id */
    MSG_APP_PROTOCALDATA_SEND,
    //MSG_APP_EXTDATA_SEND,   
    MSG_MAX_ID
}MSG_ID_E;


#endif /*_MSG_ID_H*/

/******************* (C) COPYRIGHT 2009 XIAMEN YAXON.LTD *********END OF FILE******/
