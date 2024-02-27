/**************************************************************************************************
**                                                                                               **
**  文件名称:  Tools.H                                                                           **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2010                                    **
**  创建信息:  LEON -- 2010年12月1日                                                             **
**  文件描述:  工具函数集                                                                        **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/
#ifndef __TOOLS_H
#define __TOOLS_H

#include "dal_include.h"

#define STR_EQUAL                           0
#define STR_GREAT                           1
#define STR_LESS                            2

#define CASESENSITIVE1                       0

INT8U u_chksum_1(INT8U *ptr, INT16U len);
INT16U u_chksum_2(INT8U *ptr, INT16U len);
INT8U Getchksum(INT8U *dptr, INT16U len);
INT8U Getchksum_n(INT8U *dptr, INT16U len);
INT8U Get_N_Chksum(INT8U *dptr, INT16U len);
INT8U Get_X_Chksum(INT8U *dptr, INT16U len);
BOOLEAN FindProcEntry(INT16U index, const FUNCENTRY_T *funcentry, INT16U num);
BOOLEAN NeedRoundLine(INT8U *charptr, INT8U charlen);
void Seconds2Systime(INT32U seconds, SYSTIME_T *systime);
INT32U Systime2Seconds(SYSTIME_T *systime);
void BubberSort(INT16U *a, INT8U n);
INT8U UpperChar(INT8U ch);
INT8U CmpString(INT8U *ptr1, INT8U *ptr2, INT16U len);
INT8U AcmpString(BOOLEAN matchcase, INT8U *ptr1, INT8U *ptr2, INT16U len1, INT16U len2);
INT16U Assemblebyrule(INT8U *dptr, INT8U *sptr, INT16U len, INT16U maxlen, const ASMRULE_T *rule);
INT16U Deassemblebyrule(INT8U *dptr, INT8U *sptr, INT16U len, INT16U maxlen, const ASMRULE_T *rule);
INT16U Getassemblelenbyrule(INT8U *sptr, INT16U len, const ASMRULE_T *rule);
INT8U HexToChar(INT8U ch);
INT8U AlgToChar(INT8U ch);
INT8U Chartohex(INT8U schar);
void Longtochar(INT8U *dest, INT32U src);
void Chartolong(INT32U *dest, INT8U *src);
void Shorttochar(INT8U *dest, INT16U src);
void Chartoshort(INT16U *dest, INT8U *src);
INT16U SearchDigitalString(INT8U *ptr, INT16U maxlen, INT8U flagchar, INT8U numflag);
BOOLEAN SearchKeyWordFromHead(BOOLEAN matchcase, INT8U *ptr, INT16U len, char *sptr);

INT16U Hex8UtoAlg16U(INT8U Data);
INT32U Hex16UtoAlg32U(INT16U Data);
INT8U Alg16UtoHex8U(INT16U Data);
INT16U Alg32UtoHex16U(INT32U Data);


BOOLEAN IsInRange(INT8U checkedchar,INT8U uplimit,INT8U downlimit);
INT8U  CutRedundantLen(INT8U len,INT8U standardlen);
INT8U HexToAscii(INT8U *dptr, INT8U *sptr, INT8U len);
INT8U HexToAscii1(INT8U *dptr, INT8U sdata);
INT8U AsciiToHex(INT8U *dptr, INT8U *sptr, INT8U len);
INT8U CutFromRight(INT8U* ptr, INT8U dat, INT8U len);
INT8U CutFromLeft(INT8U* ptr, INT8U dat, INT8U datalen);
INT8U DecToAscii(INT8U *dptr, INT32U data1, INT8U reflen);
INT32U AsciiToDec(INT8U *sptr, INT8U len);
BOOLEAN isdatavalid(INT8U *ptr,INT8U len);


#endif

