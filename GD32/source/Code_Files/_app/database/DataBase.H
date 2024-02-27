/**************************************************************************************************
**                                                                                               **
**  文件名称:  DataBase.H                                                                        **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2010                                    **
**  创建信息:  jump -- 2011年1月20日                                                             **
**  文件描述:  数据库底层管理                                                                    **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/

#ifndef DATABASE_H
#define DATABASE_H  1

#include "flashimg.h"
#include "s_flash.h"
/*
********************************************************************************
* define find mode
********************************************************************************
*/
#define FIND_HALFMATCH                          0x01
#define FIND_MATCHCASE                          0x02
#define FIND_REVERSE                            0x04

/*
********************************************************************************
* define order mode
********************************************************************************
*/
#define ORDER_ORDER                             0x80  /* 允许排序 */
#define ORDER_UP                                0x40  /* 是否升序排序 */
#define ORDER_MATCHCASE                         0x20  /* 是否按照大小写排序 */

/*
********************************************************************************
* define field attribute
* 固定长度,如果不定义,则表示长度不固定,前面需要加一个字节来表示长度
********************************************************************************
*/
#define FIELD_FIXED                             0x80

/*
********************************************************************************
* define FIELD_STRUCT
********************************************************************************
*/
/* sizeof(FIELD_STRUCT) = 2 */
typedef struct {
    INT8U       attrib;
    INT8U       len;                    /* 每个数据段的长度不得超过256字节 */
} FIELD_STRUCT;

void PrintImageHead(void);
BOOLEAN checkflashflag(void);
void clearflashflag(void);
void setflashflag(void);

void    DB_Init(INT8U type, INT16U nsector, INT16U offset, BOOLEAN needclear, INT8U maxfield, FIELD_STRUCT *field, INT8U maxrecord, INT8U ordermode, INT8U orderfield);
void    DB_Del(INT8U type, INT16U nsector, INT16U offset);
BOOLEAN DB_IsValid(INT8U type, INT16U nsector, INT16U offset, INT8U maxfield, FIELD_STRUCT *field, INT8U maxrecord, INT8U ordermode, INT8U orderfield);
INT8U   DB_GetItem(INT8U type, INT16U nsector, INT16U offset);
INT8U   DB_GetBlankItem(INT8U type, INT16U nsector, INT16U offset);
INT16U  DB_GetRec(INT8U type, INT16U nsector, INT16U offset, INT8U numrec, INT8U *ptr);
INT8U   DB_GetRecField(INT8U type, INT16U nsector, INT16U offset, INT8U numrec, INT8U fieldnum, INT8U *ptr);
INT8U   DB_FindRec(INT8U type, INT16U nsector, INT16U offset, INT8U findmode, INT8U fieldnum, INT8U len, INT8U *ptr);    /* 返回：如找不到匹配得记录，则返回0xff */
BOOLEAN DB_DelRec(INT8U type, INT16U nsector, INT16U offset, INT8U numrec);
BOOLEAN DB_AddRec(INT8U type, INT16U nsector, INT16U offset, BOOLEAN updateimage, INT8U *recptr);
BOOLEAN DB_ModifyRec(INT8U type, INT16U nsector, INT16U offset, INT8U numrec, INT8U *recptr);
BOOLEAN DB_ModifyRecField(INT8U type, INT16U nsector, INT16U offset, INT8U numrec, INT8U numfield, INT8U len, INT8U *ptr);
void    InitAllRecord(void);
void    WriteDB(INT16U nsector, INT16U offset);

/*
********************************************************************************
*                  DEFINE CONFIG PARAMETERS
********************************************************************************
*/
#if SWITCH_FLASH == FLASH_512K
#define BLOCK_IMAGESECTOR    3
#elif SWITCH_FLASH == FLASH_2M
#define BLOCK_IMAGESECTOR    2
#endif


#ifdef DATABASE_C
__attribute__((section ("DB_IMAGE"))) INT8U flash_image_mem[8448];
#else
extern INT8U   flash_image_mem[8448];
#endif




/*************************************************************************************************/
/*                           UNIMSG                                                              */
/*************************************************************************************************/
#define DB_DASE_SECTOR                      B_USRPARA_PAGE

/* 收到的短消息 */
#define DB_RECVSMS_SECTOR                   DB_DASE_SECTOR
#define DB_RECVSMS_OFFSET                   8L
#define DB_RECVSMS                          IMAGE_RECVSMS, DB_RECVSMS_SECTOR, DB_RECVSMS_OFFSET
/* 已发的短消息 */
#define DB_SENDSMS_SECTOR                   DB_RECVSMS_SECTOR + BLOCK_IMAGESECTOR
#define DB_SENDSMS_OFFSET                   8L
#define DB_SENDSMS                          IMAGE_SENDSMS, DB_SENDSMS_SECTOR, DB_SENDSMS_OFFSET
/* 短信草稿箱 */
#define DB_DRAFTSMS_SECTOR                  DB_SENDSMS_SECTOR + BLOCK_IMAGESECTOR
#define DB_DRAFTSMS_OFFSET                  8L
#define DB_DRAFTSMS                         IMAGE_DRAFTSMS, DB_DRAFTSMS_SECTOR, DB_DRAFTSMS_OFFSET
/* 用于弹出式消息 */
#define DB_BUFFER_SECTOR                    DB_DRAFTSMS_SECTOR + BLOCK_IMAGESECTOR
#define DB_BUFFER_OFFSET                    8L
#define DB_BUFFER                           IMAGE_BUFFER, DB_BUFFER_SECTOR, DB_BUFFER_OFFSET
/* 调度信息 */
#define DB_ATMPMSG_SECTOR                   DB_BUFFER_SECTOR + BLOCK_IMAGESECTOR
#define DB_ATMPMSG_OFFSET                   8L
#define DB_ATMPMSG                          IMAGE_ATMPMSG, DB_ATMPMSG_SECTOR, DB_ATMPMSG_OFFSET
/* 广播信息 */
#define DB_BROADMSG_SECTOR                  DB_ATMPMSG_SECTOR + BLOCK_IMAGESECTOR
#define DB_BROADMSG_OFFSET                  8L
#define DB_BROADMSG                         IMAGE_BROADMSG, DB_BROADMSG_SECTOR, DB_BROADMSG_OFFSET
/* 周边查询 */
#define DB_AROUND_SECTOR                    DB_BROADMSG_SECTOR + BLOCK_IMAGESECTOR
#define DB_AROUND_OFFSET                    8L
#define DB_AROUND                           IMAGE_AROUND, DB_AROUND_SECTOR, DB_AROUND_OFFSET
/* 导航信息 */
#define DB_TRASTATUSMSG_SECTOR              DB_AROUND_SECTOR + BLOCK_IMAGESECTOR
#define DB_TRASTATUSMSG_OFFSET              8L
#define DB_TRASTATUSMSG                     IMAGE_TRASTATUSMSG, DB_TRASTATUSMSG_SECTOR, DB_TRASTATUSMSG_OFFSET
/* 故障信息 */
#define DB_FAULTMSG_SECTOR                  DB_TRASTATUSMSG_SECTOR + BLOCK_IMAGESECTOR
#define DB_FAULTMSG_OFFSET                  8L
#define DB_FAULTMSG                         IMAGE_FAULTMSG, DB_FAULTMSG_SECTOR, DB_FAULTMSG_OFFSET
/* 紧急信息 */
#define DB_EXIGENCY_SECTOR                  DB_FAULTMSG_SECTOR + BLOCK_IMAGESECTOR
#define DB_EXIGENCY_OFFSET                  8L
#define DB_EXIGENCY                         IMAGE_EXIGENCY, DB_EXIGENCY_SECTOR, DB_EXIGENCY_OFFSET

/* 上报的事件信息 */
#define DB_EVENTMSG_SECTOR                  DB_EXIGENCY_SECTOR + BLOCK_IMAGESECTOR
#define DB_EVENTMSG_OFFSET                  8L
#define DB_EVENTMSG                         IMAGE_EVENTMSG, DB_EVENTMSG_SECTOR, DB_EVENTMSG_OFFSET
/* 电子路单 */
#define DB_TRACKMSG_SECTOR                  DB_EVENTMSG_SECTOR + BLOCK_IMAGESECTOR
#define DB_TRACKMSG_OFFSET                  8L
#define DB_TRACKMSG                         IMAGE_TRACKMSG, DB_TRACKMSG_SECTOR, DB_TRACKMSG_OFFSET
/* 中心提问 */
#define DB_QUESTION_SECTOR                  DB_TRACKMSG_SECTOR + BLOCK_IMAGESECTOR
#define DB_QUESTION_OFFSET                  8L
#define DB_QUESTION                         IMAGE_QUESTION, DB_QUESTION_SECTOR, DB_QUESTION_OFFSET

/*************************************************************************************************/
/*                           调度用语,每组最大20条记录                                                      */
/*************************************************************************************************/
/* 广播信息菜单 */
#define DB_DICT1_SECTOR                     DB_QUESTION_SECTOR + BLOCK_IMAGESECTOR
#define DB_DICT1_OFFSET                     8L
#define DB_DICT1                            IMAGE_DICT, DB_DICT1_SECTOR, DB_DICT1_OFFSET
/* 周边查询菜单 */
#define DB_DICT2_SECTOR                     DB_DICT1_SECTOR
#define DB_DICT2_OFFSET                     (DB_DICT1_OFFSET + 1500L)
#define DB_DICT2                            IMAGE_DICT, DB_DICT2_SECTOR, DB_DICT2_OFFSET
/* 事件上报菜单 */
#define DB_DICT3_SECTOR                     DB_DICT1_SECTOR
#define DB_DICT3_OFFSET                     (DB_DICT2_OFFSET + 1500L)
#define DB_DICT3                            IMAGE_DICT, DB_DICT3_SECTOR, DB_DICT3_OFFSET
/* 调度用语4 */
#define DB_DICT4_SECTOR                     DB_DICT1_SECTOR
#define DB_DICT4_OFFSET                     (DB_DICT3_OFFSET + 1500L)
#define DB_DICT4                            IMAGE_DICT, DB_DICT4_SECTOR, DB_DICT4_OFFSET

/* 电话类,均放在同一sector内 */
/*************************************************************************************************/
/*                           通话类, 每组最大20条记录                                                      */
/*************************************************************************************************/
#define DB_TALK_DIAL_SECTOR                 DB_DICT1_SECTOR + BLOCK_IMAGESECTOR
#define DB_TALK_DIAL_OFFSET                 8L
#define DB_TALK_DIAL                        IMAGE_TEL, DB_TALK_DIAL_SECTOR, DB_TALK_DIAL_OFFSET

#define DB_TALK_RECV_SECTOR                 DB_TALK_DIAL_SECTOR
#define DB_TALK_RECV_OFFSET                 (DB_TALK_DIAL_OFFSET + 1000L)
#define DB_TALK_RECV                        IMAGE_TEL, DB_TALK_RECV_SECTOR, DB_TALK_RECV_OFFSET

#define DB_TALK_MISS_SECTOR                 DB_TALK_DIAL_SECTOR
#define DB_TALK_MISS_OFFSET                 (DB_TALK_RECV_OFFSET + 1000L)
#define DB_TALK_MISS                        IMAGE_TEL, DB_TALK_MISS_SECTOR, DB_TALK_MISS_OFFSET

/*************************************************************************************************/
/*                           电话簿,最大50条记录                                                      */
/*************************************************************************************************/
#define DB_TB_SECTOR                        DB_TALK_DIAL_SECTOR
#define DB_TB_OFFSET                        (DB_TALK_MISS_OFFSET + 1000L)
#define DB_TB                               IMAGE_TEL, DB_TB_SECTOR, DB_TB_OFFSET

#endif

