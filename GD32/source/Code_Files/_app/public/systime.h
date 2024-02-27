/**************************************************************************************************
**                                                                                               **
**  文件名称:  systime.h                                                                         **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2010                                    **
**  创建信息:  jump -- 2011年1月20日                                                             **
**  文件描述:  系统时间管理                                                                      **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/

#ifndef  DEF_SYSTIME
#define  DEF_SYSTIME

#ifdef  SYSTIME_GLOBALS
#define SYSTIME_EXT
#else  
#define SYSTIME_EXT extern
#endif


void  GetSysTime(SYSTIME_T *ptr);
void  ReviseSysTime(INT8U *curtime);
void  InitSysTime(void);
INT8U GetWeekDay(void);
void  GetAsciiTime(ASCII_TIME *asciitime);
void  GetAsciiDate(ASCII_DATE *asciidate);
INT16U GeFatFileTime(void);
INT16U GetFatFileDate(void);
void YX_GetBcdSysTime(BCD_TIME_T *Bcd);
#endif
