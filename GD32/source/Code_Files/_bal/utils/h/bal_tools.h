/****************************************************************
**                                                              *
**  FILE         :  bal_tools.h                                  *
**  COPYRIGHT    :  (c) 2001 .Xiamen Yaxon NetWork CO.LTD       *
**                                                              *
**                                                              *
**  By : CCH 2002.1.15                                          *
****************************************************************/
/**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2008/10/21 | 王世华 |  1.根据g6规范修改函数名字；
**
*********************************************************************************/
#ifndef DEF_TOOLS
#define DEF_TOOLS


#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "string.h"
#include "yx_includes.h"

#define YX_MEMSET               memset
#define YX_SPRINTF              sprintf
#define YX_VSPRINTF             vsnprintf
#define YX_STRLEN               strlen
#define YX_STRCPY               strcpy
#define YX_STRCAT               strcat
/*
********************************************************************************
*                  DEFINE PARAMETERS
********************************************************************************
*/
#define STR_EQUAL                           0
#define STR_GREAT                           1
#define STR_LESS                            2

#define MODE_CODE7                          0
#define MODE_CODE8                          1

#define CASESENSITIVE                       1

#define YX_MEMCPY(DET_PTR, DET_LEN, SRC_PTR, SRC_LEN)   \
do {                                                    \
    OS_ASSERT((DET_LEN) >= (SRC_LEN), RETURN_FALSE);    \
    memcpy(DET_PTR, SRC_PTR, SRC_LEN);                  \
} while(0)
                                                         
/* 定义4字节对齐后的长度 */
#define ALIGN4(n)            ((((n) + 3) / 4) * 4)

/*******************************************************************
** 函数名:     bal_u_chksum_1
** 函数描述:   单字节校验码算法
** 参数:       [in] ptr:      目标数据地址
**             [in] len:      目标数据长度
** 返回:       单字节校验码
********************************************************************/
INT8U bal_u_chksum_1(INT8U *ptr, INT32U len);

/*******************************************************************
** 函数名:     bal_u_chksum_1b
** 函数描述:   单字节校验码算法,数据取反码后再计算校验和
** 参数:       [in] ptr:      目标数据地址
**             [in] len:      目标数据长度
** 返回:       单字节校验码
********************************************************************/
INT8U bal_u_chksum_1b(INT8U *ptr, INT32U len);

/*******************************************************************
** 函数名:     bal_u_chksum_2
** 函数描述:   双字节校验码算法
** 参数:       [in] ptr:      目标数据地址
**             [in] len:      目标数据长度
** 返回:       双字节校验码
********************************************************************/
INT16U bal_u_chksum_2(INT8U *ptr, INT32U len);

INT8U   bal_GetChkSum_IP(INT8U *dptr, INT16U len);   //yyy

void    bal_MovXX(INT8U *dptr, INT8U *sptr, INT16U len);
void    bal_MovStr(INT8U *dptr, char *sptr);
void    bal_InitBuf(INT8U *dptr, INT8U initdata, INT16U len);
INT8U   bal_GetChkSum(INT8U *dptr, INT16U len);
INT8U   bal_GetChkSum_N(INT8U *dptr, INT16U len);
INT8U   bal_GetChkSum_C(INT8U *dptr, INT16U len);
INT16U  bal_GetChkSum_CRC16(INT8U *dptr, INT16U len);
INT8U   bal_GetChkSum_7C(INT8U *dptr, INT16U len);
void    bal_ConvertData(INT8U *dptr, INT16U len, INT8U ddata, INT8U sdata);
BOOLEAN bal_CmpString_Byte(INT8U *ptr, INT16U len, INT8U cb);
INT8U   bal_CmpString(INT8U *ptr1, INT8U *ptr2, INT16U len);
INT8U   bal_ACmpString(BOOLEAN matchcase, INT8U *ptr1, INT8U *ptr2, INT16U len1, INT16U len2);

INT16U  bal_Bit7ToOctet(INT8U *dptr, INT8U *sptr, INT16U len);
INT16U  bal_OctetToBit7(INT8U *dptr, INT8U *sptr, INT16U len, INT8U leftbit);
INT8U   bal_HexToChar(INT8U sbyte);
INT8U   bal_CharToHex(INT8U schar);
INT8U   bal_SemiOctetToChar(INT8U sbyte);
INT8U   bal_CharToSemiOctet(INT8U schar);
INT8U   bal_DecToAscii(INT8U *dptr, INT32U data, INT8U reflen);
INT32U  bal_AsciiToDec(INT8U *sptr, INT8U len);
INT16U  bal_HexToAscii(INT8U *dptr, INT8U *sptr, INT16U len);
INT16U  bal_AsciiToHex(INT8U *dptr, INT8U *sptr, INT16U len);
void    bal_SemiOctetToHex(INT8U *dptr, INT8U *sptr, INT16U len);
INT16U  bal_SemiOctetToAscii(INT8U *dptr, INT8U *sptr, INT16U len);
INT16U  bal_AsciiToSemiOctet(INT8U *dptr, INT8U *sptr, INT16U len);
/*********************************************************************************
** 函数名:     bal_ConvertGmtToLocaltime
** 函数描述:   将格林尼治时间转换成本地时间
** 参数:       [in/out]  date:     日期
**             [in/out]  time:     时间
**             [in]      diftime:  时差
** 返回:       无
*********************************************************************************/
void bal_ConvertGmtToLocaltime(DATE_T *date, TIME_T *time, INT8U diftime);
//#define bal_ConvertGmtToLocaltime    bal_ConvertGmtToLocaltime

/*********************************************************************************
** 函数名:     bal_ConvertLocaltimeToGmt
** 函数描述:   将本地时间转换成格林尼治时间
** 参数:       [in/out]  date:     日期
**             [in/out]  time:     时间
**             [in]      diftime:  时差
** 返回:       无
*********************************************************************************/
void bal_ConvertLocaltimeToGmt(DATE_T *date, TIME_T *time, INT8U diftime);

/*******************************************************************
** 函数名:      bal_ASearchKeyWord
** 函数描述:    搜索字符串，可以区分大小写，在s1中搜索s2
** 参数:        [in] matchcase:true-区分大小写，false-不区分大小写
**              [in] s1：      原字符串
**              [in] len1：    s1长度
**              [in] s2：      要查找的字符串
**              [in] len2：    s2长度
** 返回:        成功返回true，失败返回false
********************************************************************/
BOOLEAN bal_ASearchKeyWord(BOOLEAN matchcase, INT8U *s1, INT16U len1, INT8U *s2, INT16U len2);
BOOLEAN bal_SearchDataInMem(INT8U *bptr, INT16U blen, INT8U *sptr, INT16U slen);
/*******************************************************************
**  函数名:     bal_SearchKeyWord
**  函数描述:   搜索字符串
**  参数:       [in] ptr:    数据指针
**              [in] maxlen: 数据长度
**              [in] sptr:   需要在ptr中查找的字符串，以‘\0’为结束符
**  返回:       成功返回TRUE,失败返回false
********************************************************************/
BOOLEAN bal_SearchKeyWord(INT8U *ptr, INT16U maxlen, char *sptr);
BOOLEAN bal_SearchKeyWordFromHead(INT8U *ptr, INT16U maxlen, char *sptr);
INT16U  bal_SearchDigitalString(INT8U *ptr, INT16U maxlen, INT8U flagchar, INT8U numflag);
INT16U  bal_SearchHexString(INT8U *ptr, INT16U maxlen, INT8U flagchar, INT8U numflag);
INT16U  bal_SearchString(INT8U *dptr, INT16U limitlen, INT8U *sptr, INT16U maxlen, INT8U flagchar, INT8U numflag);

INT16U  bal_FindCharPos(INT8U *sptr, char findchar, INT8U numchar, INT16U maxlen);
void    bal_AddGpsData(INT8U *result, INT8U *data1, INT8U *data2);
BOOLEAN bal_DecGpsData(INT8U *result, INT8U *data1, INT8U *data2);
BOOLEAN bal_ShieldHighBit(INT8U *data);

/*******************************************************************
** 函数名:     bal_CodeX_To_Code6
** 函数描述:   X转6编码
** 参数:       [in]  mode:     编码位数，见MODE_CODE7,MODE_CODE8
**             [out] dptr:     转码后的数据缓存地址
**             [in]  limitlen: 转码后的数据缓存最大长度
**             [in]  sptr:     待转码的数据地址
**             [in]  len:      待转码的数据长度
**             [in]  openpro:  协议类型,true-公开协议,false-雅迅通用协议
** 返回:       成功返回转码后数据长度,失败返回0
********************************************************************/
INT16U bal_CodeX_To_Code6(INT8U mode, INT8U *dptr, INT16U limitlen, INT8U *sptr, INT16U len, BOOLEAN openpro);

/*******************************************************************
** 函数名:     bal_Code6_To_CodeX
** 函数描述:   6转X解码
** 参数:       [in]  mode:     编码位数，见MODE_CODE7,MODE_CODE8
**             [out] dptr:     转码后的数据缓存地址
**             [in]  limitlen: 转码后的数据缓存最大长度
**             [in]  sptr:     待转码的数据地址
**             [in]  len:      待转码的数据长度
**             [in]  openpro:  协议类型,true-公开协议,false-雅迅通用协议
** 返回:       成功返回转码后数据长度,失败返回0
********************************************************************/
INT16U bal_Code6_To_CodeX(INT8U mode, INT8U *dptr, INT16U limitlen, INT8U *sptr, INT16U len, BOOLEAN openpro);

INT16U  bal_DecodeTellen(INT8U *sptr, INT16U len);
INT16U  bal_DecodeTel(INT8U *dptr, INT8U *sptr, INT16U len);
void    bal_EncodeTel(INT8U *dptr, INT8U *sptr, INT16U len, INT16U maxlen);
BOOLEAN bal_FindProcEntry(INT16U index, FUNCENTRY_T *funcentry, INT16U num);

BOOLEAN bal_Add_Thousand(INT16U *add1, INT16U *add2, INT8U len1, INT8U len2);
BOOLEAN bal_Dec_Thousand(INT16U *dec1, INT16U *dec2, INT8U len1, INT8U len2);
void    bal_Mul_Thousand(INT16U *result, INT16U mul1, INT16U mul2);
INT8U   bal_ThousandToAscii(INT8U *ptr, INT16U opdata);

INT16U  bal_AssembleByRules(INT8U *dptr, INT8U *sptr, INT16U len, ASMRULE_T *rule);
INT16U  bal_DeassembleByRules(INT8U *dptr, INT8U *sptr, INT16U len, ASMRULE_T *rule);

BOOLEAN bal_DateValid(DATE_T *date);
BOOLEAN bal_TimeValid(TIME_T *time);
BOOLEAN bal_LongitudeValid(INT8U *longitude);
BOOLEAN bal_LatitudeValid(INT8U *latitude);
/* // 老手柄程序用到的函数,暂屏蔽
BOOLEAN NeedRoundLine(INT8U *ptr, INT16U len);
INT16S  PlotLinePos(INT8U *textptr, INT16U textlen, INT16U *linepos, INT16U maxline, INT16U linesize);
INT16U  InsertEditBuf(INT8U *ptr, INT16U len, INT16U pos, INT16U ch);
INT8U   DelEditBuf(INT8U *ptr, INT16U len, INT16U pos);
void    WriteWordByPointer(INT8U *ptr, INT16U wdata);
INT16U  ReadWordByPointer(INT8U *ptr);
INT8U   FormatYear(INT8U *ptr, INT8U year, INT8U month, INT8U day);
INT8U   FormatTime(INT8U *ptr, INT8U hour, INT8U minute, INT8U second);
INT8U   CutRedundantLen(INT16U len,INT16U standardlen);
*/

/* add by qzc */
#define  MISS		   0
#define  OVERLAP       1
#define  SUBCLASS      2
#define  VACUUM        3

INT8U   bal_GetSmall(INT8U Data1,INT8U Data2);
INT8U   bal_CmpData(INT8U *Buf1,INT8U *Buf2,INT8U Len1,INT8U Len2);
BOOLEAN bal_IsInRange(INT8U Data,INT8U Low,INT8U High);
//(A In B=SUBCLASS)
INT8U   bal_RangeRelation(INT8U *A1,INT8U *A2,INT8U *B1,INT8U *B2,INT8U Len);
INT16U  bal_Code8_To_Code7(INT8U *Dptr,INT8U *Sptr,INT16U Len,INT16U LimitLen);
INT16U  bal_Code7_To_Code8(INT8U *Dptr,INT8U *Sptr,INT16U Len);

INT8U   bal_BitValue(INT8U Data,INT8U Index);
BOOLEAN bal_IsZero(INT8U *Ptr,INT8U Len);
//#if EN_OPEN_PROTOCOL > 0
void bal_ResumeHighBit(INT8U *data, INT8U mask);
void bal_MulGpsData(INT8U *result, INT8U *data, INT8U hdata, INT8U ldata);
//#endif

INT8U   bal_GetDigitalString(INT8U *dptr, INT8U *sptr, INT16U maxlen, INT8U flagchar, INT8U numflag);

//for me3080
BOOLEAN bal_SearchGBCode(INT8U *ptr, INT16U len);
INT16U  bal_TestUnicodeLen(INT8U *ptr, INT16U len);
INT16U  bal_TestGBLen(INT8U *ptr, INT16U len);

/*******************************************************************
** 函数名:     bal_ConvertIpStringToHex
** 函数描述:   解析IP地址,如ip_string为：11.12.13.14，则转换后的ip_long为0x0B0C0D0E
** 参数:       [out] ip_long:  ip地址，依次从从高位到低位
**             [out] sbits:    子网位数
**             [in]  ip_string:ip字符串，以'\0'为结束符，如"11.12.13.14"
** 返回:       转换成功返回0，失败返回失败描述符
********************************************************************/
char *bal_ConvertIpStringToHex(INT32U *ip_long, INT8U *sbits, char *ip_string);
//#define bal_ConvertIpStringToHex bal_ConvertIpStringToHex

/*******************************************************************
**  函数名  :  bal_ConvertIpHexToString
**  函数描述:  转换INT32U型ip为字符串,如0x11223344，则转换后为"17.34.51.68"
**  参数:      [in]  ip:     INT32U型ip
**             [out] dptr:   目的指针地址
**             [in]  maxlen: 目的缓存长度，不小于16
**  返回参数:  成功返回转换后的长度(包含结束符)，失败返回0
********************************************************************/
INT8U bal_ConvertIpHexToString(INT32U ip, char *dptr, INT8U maxlen);

INT16U 	bal_ByteToDecToAscii(INT8U *dptr,INT8U *sptr, INT16U len);
INT16U  bal_AsciiToDecToByte(INT8U *dptr, INT8U *sptr, INT16U len);
INT8U   bal_HexToSemiBCD(INT8U sdata);
INT8U bal_ChkSum_Xor(INT8U *Ptr,INT32U Len);

INT16U bal_HexToDec(INT8U *dptr, INT8U *sptr, INT16U len);
INT8U bal_AsciiToBcd(INT8U *dptr, INT8U* sptr, INT8U len);
INT8U bal_BcdToHex_Byte(INT8U bcd);
void bal_BcdToHex(INT8U *dptr, INT8U *sptr, INT16U len);
INT16U bal_HexToDec(INT8U *dptr, INT8U *sptr, INT16U len);
INT32U bal_GetTimeStamp(DATE_T *date, TIME_T *time);

/*******************************************************************
** 函数名:     bal_GetDateSubDays
** 函数描述:   根据某天日期后退day天后得到日期
** 参数:       [out] odate  :输出日期
**             [in]  idate  :输入日期
**             [in]  day    :往后退的天数
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN bal_GetDateSubDays(DATE_T *odate, DATE_T *idate,INT16U day);
//#define bal_GetDateSubDays    bal_GetDateSubDays

/*******************************************************************
** 函数名:      bal_SystimeToWord
** 函数描述:    系统时间转换为INT32U数据,位分布为：年(6)+月(4)+日(5)+时(5)+分(6)+秒(6)
** 参数:        [in]  systime:          系统时间                  
** 返回:        双字格式系统时间
********************************************************************/
INT32U bal_SystimeToWord(SYSTIME_T *systime);

/*******************************************************************
** 函数名:      bal_WordtimeToSystime
** 函数描述:    INT32U数据转换为系统时间,位分布为：年(6)+月(4)+日(5)+时(5)+分(6)+秒(6)
** 参数:        [in]   dwtime:           INT32U数据
** 参数:        [out]  systime:         系统时间                  
** 返回:        TRUE/FALSE
********************************************************************/
void bal_WordtimeToSystime(INT32U dwtime, SYSTIME_T *systime);
/*******************************************************************
**  函数名  :  bal_DlyNumMS
**  函数描述:  延时若干毫秒
**  参数:      [in]  num: 延时的毫秒数
**  返回参数:  无
********************************************************************/
void bal_DlyNumMS(INT32U num);

// 将长整型数按大端模式转换为4个字节，即高位字节在低地址
void bal_longtochar(INT8U *dest, INT32U src);
// 将4个字节按大端模式转换为长整型,即高位字节在低地址
INT32U bal_chartolong(INT8U *src);
// 将短整型数按大端模式转换为2个字节，即高位字节在低地址
void bal_shorttochar(INT8U *dest, INT16U src);
// 将2个字节按大端模式转换为短整型,即高位字节在低地址
INT16U bal_chartoshort(INT8U *src);
// 函数描述:    将大端存储的4字节数据转换小端模式存储
INT8U bal_DW_BigToLit(INT8U *sptr, INT8U *dptr);
/*******************************************************************
** 函数名:      bal_HW_BigToLit
** 函数描述:    将大端存储的2字节数据转换为小端模式存储
** 参数:        [in]  sptr:             源缓冲区
** 返回:        双字值
********************************************************************/
INT8U bal_HW_BigToLit(INT8U *sptr, INT8U *dptr);
INT8U bal_CutFromRight(INT8U* ptr, INT8U dat, INT8U len);
INT8U bal_CutFromLeft(INT8U* ptr, INT8U dat, INT8U datalen);
/*******************************************************************
** 函数名:      YX_BigEndModeToHWord
** 函数描述:    将大端存储的2字节数据转换为半字
** 参数:        [in]  sptr:             源缓冲区
** 返回:        双字值
********************************************************************/
INT16U YX_BigEndModeToHWord(INT8U *sptr);
/*******************************************************************
** 函数名:      YX_DWToLitEndMode
** 函数描述:    将双字转换为小端存储模式
** 参数:        [out]  dptr:            目的缓冲区
                [in]  dwvalue:          双字值
** 返回:        NULL
********************************************************************/
void YX_DWToLitEndMode(INT8U *dptr, INT32U dwvalue);
#endif
