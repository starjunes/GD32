/******************************************************************************
**
** Filename:     yx_misc.h
** Copyright:    
** Description:  该模块主要实现通用杂项工具类函数
**
-------------------------------------------------------------------------------
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#ifndef YX_MISC_H
#define YX_MISC_H



/*
********************************************************************************
* include lib header file
********************************************************************************
*/
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
/*
********************************************************************************
* define config parameters
********************************************************************************
*/
#define YX_MEMSET            memset
//#define YX_MEMCPY            memcpy
#define YX_VSPRINTF          vsprintf
#define YX_STRLEN            strlen
#define YX_STRCPY            strcpy
#define YX_STRCAT            strcat
#define YX_MEMCMP            memcmp

#define YX_VA_START          va_start
#define YX_VA_END            va_end

#define YX_MEMCPY(DET_PTR, DET_LEN, SRC_PTR, SRC_LEN)   \
do {                                                    \
    if ((DET_LEN) < (SRC_LEN)) {                        \
        OS_RESET(RESET_EVENT_ERR);                      \
    } else {                                            \
        memcpy(DET_PTR, SRC_PTR, SRC_LEN);              \
    }                                                   \
} while(0)

/* 结构体成员在结构体中的偏移量 */
#define T_OFFSET(T, MEM)	 ((INT32U)&(((T *)0)->MEM))
/*
********************************************************************************
* define struct
********************************************************************************
*/


/*******************************************************************
** 函数名称:   YX_HexToChar
** 函数描述:   将输入16进制字节数据的低4位值转换成字符
** 参数:       [in]  inbyte: 输入字节
** 返回:       返回字符
********************************************************************/
INT8U YX_HexToChar(INT8U inbyte);

/*******************************************************************
** 函数名称:   YX_CharToHex
** 函数描述:   将输入字符转化成16进制字节数据
** 参数:       [in]  inchar: 输入字符
** 返回:       返回16进制值
********************************************************************/
INT8U YX_CharToHex(INT8U inchar);

/*******************************************************************
** 函数名:     YX_GetChkSum
** 函数描述:   计算逐个字节累加校验码,取最低字节
** 参数:       [in] dptr:      数据区地址
**             [in] len:       数据区长度
** 返回:       单字节累加校验码
********************************************************************/
INT8U YX_GetChkSum(INT8U *dptr, INT32U len);

/*******************************************************************
** 函数名:     YX_GetChkSum_N
** 函数描述:   计算逐个字节累加校验码,取最低字节取反
** 参数:       [in] dptr:      数据区地址
**             [in] len:       数据区长度
** 返回:       单字节累加校验码反码
********************************************************************/
INT8U YX_GetChkSum_N(INT8U *dptr, INT32U len);

/*******************************************************************
** 函数名:     YX_GetNChkSum
** 函数描述:   计算逐个字节取反后累加校验码,取最低字节
** 参数:       [in] dptr:      数据区地址
**             [in] len:       数据区长度
** 返回:       单字节累加校验码
********************************************************************/
INT8U YX_GetNChkSum(INT8U *dptr, INT32U len);

/*******************************************************************
** 函数名:     YX_ChkSum_XOR
** 函数描述:   计算逐个字节异或校验码
** 参数:       [in] dptr:      数据区地址
**             [in] len:       数据区长度
** 返回:       校验码
********************************************************************/
INT8U YX_ChkSum_XOR(INT8U *dptr, INT32U len);

/*******************************************************************
** 函数名:     YX_CheckSum_1U
** 函数描述:   单字节校验码算法
** 参数:       [in] ptr:      目标数据地址
**             [in] len:      目标数据长度
** 返回:       单字节校验码
********************************************************************/
INT8U YX_CheckSum_1U(INT8U *ptr, INT32U len);

/*******************************************************************
** 函数名:     YX_CheckSum_1UB
** 函数描述:   单字节校验码算法,数据取反码后再计算校验和
** 参数:       [in] ptr:      目标数据地址
**             [in] len:      目标数据长度
** 返回:       单字节校验码
********************************************************************/
INT8U YX_CheckSum_1UB(INT8U *ptr, INT32U len);

/*******************************************************************
** 函数名:     YX_CheckSum_2U
** 函数描述:   双字节校验码算法
** 参数:       [in] ptr:      目标数据地址
**             [in] len:      目标数据长度
** 返回:       双字节校验码
********************************************************************/
INT16U YX_CheckSum_2U(INT8U *ptr, INT32U len);

/*******************************************************************
** 函数名:     YX_AsciiToBcd
** 函数描述:   ASCII码转换为BCD码
** 参数:       [out] dptr: 输出转换数据
** 参数:       [out] dptr: 输入转换数据
**             [in]  len:  转换数据长度
** 返回:       校验码
********************************************************************/
INT32U YX_AsciiToBcd(INT8U *dptr, INT8U *sptr, INT32U slen);

/*******************************************************************
** 函数名:     YX_EncodeStuffData
** 函数描述:   编码填充指定填充字
** 参数:       [out] dptr:   目的数据指针
**             [in]  maxlen: 目的数据最大长度
**             [in]  sptr:   源数据指针
**             [in]  slen:   源数据长度
**             [in]  flag:   填充字
** 返回:       无
********************************************************************/
void YX_EncodeStuffData(INT8U *dptr, INT16U maxlen, INT8U *sptr, INT16U slen, INT8U flag);

/*******************************************************************
** 函数名:     YX_DecodeStuffData
** 函数描述:   解码以填充字为结束符数据
** 参数:       [out] dptr:   目的数据指针
**             [in]  maxlen: 目的数据最大长度
**             [in]  sptr:   源数据指针
**             [in]  slen:   源数据长度
**             [in]  flag:   填充字
** 返回:       返回解码后长度
********************************************************************/
INT16U YX_DecodeStuffData(INT8U *dptr, INT16U maxlen, INT8U *sptr, INT16U slen, INT8U flag);

/*******************************************************************
** 函数名:     YX_DecodeStuffDataLen
** 函数描述:   解码以填充字为结束符数据
** 参数:       [in]  sptr:   源数据指针
**             [in]  slen:   源数据长度
**             [in]  flag:   填充字
** 返回:       返回解码后长度
********************************************************************/
INT16U YX_DecodeStuffDataLen(INT8U *sptr, INT16U slen, INT8U flag);

/*******************************************************************
** 函数名:     YX_DecToAscii
** 函数描述:   10进制转化为ASCII码字符串,如0x11->"17"
** 参数:       [out] dptr:   目的数据指针
**             [in]  data:   待转换数据
**             [in]  reflen: 转换后长度,如果填0，则以实际转换位数为准,不在前面填充0
** 返回:       返回转换后长度
********************************************************************/
INT8U YX_DecToAscii(INT8U *dptr, INT32U data, INT8U reflen);

/*******************************************************************
** 函数名:     YX_HexToBcd
** 函数描述:   HEX转换为BCD
** 参数:       [out] sptr:   待转换数据指针
**             [in]  slen:   待转换数据最大长度
**             [in]  dptr:   转换后数据
**             [in]  maxlen: 数据缓存最大长度
** 返回:       无
********************************************************************/
void YX_HexToBcd(INT8U *sptr, INT32U slen, INT8U *dptr, INT32U maxlen);

/*******************************************************************
** 函数名:     YX_AsciiToHex
** 函数描述:   ASCII转换为HEX,如"1122"->0x11 0x22
** 参数:       [out] dptr:   转换后数据
**             [in]  maxlen: 转换后数据缓存最大长度
**             [in]  sptr:   待转换数据指针
**             [in]  slen:   待转换数据长度,必须为偶数
** 返回:       返回转后长度,失败返回0
********************************************************************/
INT32U YX_AsciiToHex(INT8U *dptr, INT32U maxlen, INT8U *sptr, INT32U slen);

/*******************************************************************
** 函数名:     YX_DeassembleByRules
** 函数描述:   标准外设数据解码
** 参数:       [out] dptr:   目的数据指针
**             [in]  dlen:   目的缓存最大长度
**             [in]  sptr:   待转换数据
**             [in]  slen:   转转换数据长度
**             [in]  rule:   转转规则
** 返回:       返回转换后长度
********************************************************************/
INT16U YX_DeassembleByRules(INT8U *dptr, INT16U dlen, INT8U *sptr, INT16U slen, ASMRULE_T const *rule);

/*******************************************************************
** 函数名:     YX_AssembleByRules
** 函数描述:   标准外设数据编码
** 参数:       [out] dptr:   目的数据指针
**             [in]  dlen:   目的缓存最大长度
**             [in]  sptr:   待转换数据
**             [in]  slen:   转转换数据长度
**             [in]  rule:   转转规则
** 返回:       返回转换后长度
********************************************************************/
INT16U YX_AssembleByRules(INT8U *dptr, INT16U dlen, INT8U *sptr, INT16U slen, ASMRULE_T const *rule);

//#if EN_AT > 0

#define STR_EQUAL                0
#define STR_GREAT                1
#define STR_LESS                 2

#define CASESENSITIVE            1


INT16U YX_FindCharPos(INT8U *sptr, char findchar, INT8U numchar, INT16U maxlen);
INT16U YX_SearchDigitalString(INT8U *ptr, INT16U maxlen, INT8U flagchar, INT8U numflag);
INT16U YX_SearchString(INT8U *dptr, INT16U limitlen, INT8U *sptr, INT16U maxlen, INT8U flagchar, INT8U numflag);
INT8U YX_ACmpString(BOOLEAN matchcase, INT8U *ptr1, INT8U *ptr2, INT16U len1, INT16U len2);
BOOLEAN YX_SearchKeyWordFromHead(INT8U *ptr, INT16U maxlen, char *sptr);
/*******************************************************************
** 函数名:     YX_SearchKeyWord
** 函数描述:   搜索字符串
** 参数:       [in] ptr:    数据指针
**              [in] maxlen: 数据长度
**              [in] sptr:   需要在ptr中查找的字符串，以‘\0’为结束符
** 返回:       成功返回TRUE,失败返回false
********************************************************************/
BOOLEAN YX_SearchKeyWord(INT8U *ptr, INT16U maxlen, char *sptr);

/*******************************************************************
** 函数名:     YX_ConvertIpStringToHex
** 函数描述:   解析IP地址,如ip_string为：11.12.13.14，则转换后的ip_long为0x0B0C0D0E
** 参数:       [out] ip_long:  ip地址，依次从从高位到低位
**             [out] sbits:    子网位数
**             [in]  ip_string:ip字符串，以'\0'为结束符，如“11.12.13.14”
** 返回:       转换成功返回0，失败返回失败描述符
********************************************************************/
char *YX_ConvertIpStringToHex(INT32U *ip_long, INT8U *sbits, char *ip_string);

/*******************************************************************
** 函数名:     YX_ConvertIpHexToString
** 函数描述:   装欢IP地址,将hex格式转换为string格式,,如ip_string为0x0B0C0D0E，则转换后的ip_long为"11.12.13.14"
** 参数:       [in] ip:  hex格式ip地址
** 返回:       成功返回string格式IP地址,以"\0"为结束符
********************************************************************/
char *YX_ConvertIpHexToString(INT32U ip);

/***************************************************************
** 函数名:    YX_ConvertLatitudeOrLongitude
** 功能描述:  转换度+分+小数分格式为度数据格式(度×1000000格式)
** 参数:      [in] sptr: 度+分+小数分
** 返回值:    返回转换后值
***************************************************************/
INT32U YX_ConvertLatitudeOrLongitude(INT8U *sptr);


INT16U YX_Bit7ToOctet(INT8U *dptr, INT8U *sptr, INT16U len);
INT16U YX_OctetToBit7(INT8U *dptr, INT8U *sptr, INT16U len, INT8U leftbit);
INT16U YX_SemiOctetToAscii(INT8U *dptr, INT8U *sptr, INT16U len);
void YX_SemiOctetToHex(INT8U *dptr, INT8U *sptr, INT16U len);
INT16U YX_AsciiToSemiOctet(INT8U *dptr, INT8U *sptr, INT16U len);

//#endif

#endif

