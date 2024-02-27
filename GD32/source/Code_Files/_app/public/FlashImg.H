/**************************************************************************************************
**                                                                                               **
**  文件名称:  FlashImg.H                                                                        **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2010                                    **
**  创建信息:  jump -- 2011年1月20日                                                             **
**  文件描述:  串行FLASH镜像缓冲区管理                                                           **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/

#ifndef FLASHIMG_H
#define FLASHIMG_H                          1
#define IMAGE_BUFFER        0X30    /* 弹出式菜单 */
#define IMAGE_PUBLIC        0X31
#define IMAGE_RECVSMS       0X32
#define IMAGE_SENDSMS       0X33
#define IMAGE_DRAFTSMS      0X34
#define IMAGE_BROADMSG      0X35
#define IMAGE_ATMPMSG       0X36
#define IMAGE_AROUND        0X37
#define IMAGE_EVENTMSG      0X38
#define IMAGE_TRACKMSG      0X39
#define IMAGE_QUESTION      0X3A
#define IMAGE_DICT          0X3B
#define IMAGE_TEL           0X3C
//#define IMAGE_NOTICMSG      0X3D
//#define IMAGE_PUBNOTICMSG   0X3E
#define IMAGE_TRASTATUSMSG  0X3F
#define IMAGE_FAULTMSG      0X40
#define IMAGE_EXIGENCY      0X41
//#define IMAGE_ATMPDEVMSG    0X42
/*
********************************************************************************
* define SIZE_FLASHID
********************************************************************************
*/
#define SIZE_FLASHID                        8


void    UpdateImageFlash(INT16U nSector, INT8U *sptr);
BOOLEAN ImageFlashValid(INT16U nSector);
BOOLEAN InitImageSram(INT16U nSector, INT8U type, INT8U *sptr);

#endif

