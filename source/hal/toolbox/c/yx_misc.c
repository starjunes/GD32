/******************************************************************************
**
** Filename:     yx_misc.c
** Copyright:    
** Description:  该模块主要实现通用杂项工具类函数
**
-------------------------------------------------------------------------------
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#include "hal_include.h"
#include "yx_misc.h"


/*
********************************************************************************
* define config parameters
********************************************************************************
*/

/*
********************************************************************************
* define struct
********************************************************************************
*/

/*
********************************************************************************
* define module variants
********************************************************************************
*/



/*******************************************************************
** 函数名称:   YX_HexToChar
** 函数描述:   将输入16进制字节数据的低4位值转换成字符
** 参数:       [in]  inbyte: 输入字节
** 返回:       返回字符
********************************************************************/
INT8U YX_HexToChar(INT8U inbyte)
{
    INT8U temp;
    
    temp = inbyte & 0x0F;
    if (temp < 0x0A) {
        return (temp + '0');
    } else {
        return (temp - 0x0A + 'A');
    }
}

/*******************************************************************
** 函数名称:   YX_CharToHex
** 函数描述:   将输入字符转化成16进制字节数据
** 参数:       [in]  inchar: 输入字符
** 返回:       返回16进制值
********************************************************************/
INT8U YX_CharToHex(INT8U inchar)
{
    if (inchar >= '0' && inchar <= '9') {
        return (inchar - '0');
    } else if (inchar >= 'A' && inchar <= 'F') {
        return (inchar - 'A' + 0x0A);
    } else if (inchar >= 'a' && inchar <= 'f') {
        return (inchar - 'a' + 0x0A);
    } else {
        return 0;
    }
}

/*******************************************************************
** 函数名:     YX_GetChkSum
** 函数描述:   计算逐个字节累加校验码,取最低字节
** 参数:       [in] dptr:      数据区地址
**             [in] len:       数据区长度
** 返回:       单字节累加校验码
********************************************************************/
INT8U YX_GetChkSum(INT8U *dptr, INT32U len)
{
    INT8U result;
	
    result = 0;
    for (; len > 0; len--) {
        result += *dptr++;
    }
    return result;
}

/*******************************************************************
** 函数名:     YX_GetChkSum_N
** 函数描述:   计算逐个字节累加校验码,取最低字节取反
** 参数:       [in] dptr:      数据区地址
**             [in] len:       数据区长度
** 返回:       单字节累加校验码反码
********************************************************************/
INT8U YX_GetChkSum_N(INT8U *dptr, INT32U len)
{
    return (~YX_GetChkSum(dptr, len));
}

/*******************************************************************
** 函数名:     YX_GetNChkSum
** 函数描述:   计算逐个字节取反后累加校验码,取最低字节
** 参数:       [in] dptr:      数据区地址
**             [in] len:       数据区长度
** 返回:       单字节累加校验码
********************************************************************/
INT8U YX_GetNChkSum(INT8U *dptr, INT32U len)
{
    INT8U result;
	
    result = 0;
    for (; len > 0; len--) {
        result += (INT8U)(~(*dptr++));
    }
    return result;
}

/*******************************************************************
** 函数名:     YX_ChkSum_XOR
** 函数描述:   计算逐个字节异或校验码
** 参数:       [in] dptr:      数据区地址
**             [in] len:       数据区长度
** 返回:       校验码
********************************************************************/
INT8U YX_ChkSum_XOR(INT8U *dptr, INT32U len)
{
    INT8U result;
	
    result = 0;
    for (; len > 0; len--) {
        result ^= *dptr++;
    }
    return result;
}

/*******************************************************************
** 函数名:     YX_CheckSum_1U
** 函数描述:   单字节校验码算法
** 参数:       [in] ptr:      目标数据地址
**             [in] len:      目标数据长度
** 返回:       单字节校验码
********************************************************************/
INT8U YX_CheckSum_1U(INT8U *ptr, INT32U len)
{
    INT32U i;
    INT16U chksum;
    
    chksum = 0;
    for (i = 0; i < len; i++) {
        chksum += *ptr++;
        chksum = (chksum & 0xFF) + (chksum >> 8);
    }
    chksum += 1;
    return chksum;
}

/*******************************************************************
** 函数名:     YX_CheckSum_1UB
** 函数描述:   单字节校验码算法,数据取反码后再计算校验和
** 参数:       [in] ptr:      目标数据地址
**             [in] len:      目标数据长度
** 返回:       单字节校验码
********************************************************************/
INT8U YX_CheckSum_1UB(INT8U *ptr, INT32U len)
{
    INT32U i;
    INT16U chksum;
    
    chksum = 0;
    for (i = 0; i < len; i++) {
        chksum += (INT8U)(~(*ptr++));
        chksum = (chksum & 0xFF) + (chksum >> 8);
    }
    chksum += 1;
    return chksum;
}

/*******************************************************************
** 函数名:     YX_CheckSum_2U
** 函数描述:   双字节校验码算法
** 参数:       [in] ptr:      目标数据地址
**             [in] len:      目标数据长度
** 返回:       双字节校验码
********************************************************************/
INT16U YX_CheckSum_2U(INT8U *ptr, INT32U len)
{
    INT8U  temp;
    INT16U chksum;
    
    temp    = YX_CheckSum_1U(ptr, len);
    chksum  = (INT16U)temp << 8;
    temp    = YX_CheckSum_1UB(ptr, len);
    chksum |= temp;
    return chksum;
}

/*******************************************************************
** 函数名:     YX_AsciiToBcd
** 函数描述:   ASCII码转换为BCD码
** 参数:       [out] dptr: 输出转换数据
** 参数:       [out] dptr: 输入转换数据
**             [in]  len:  转换数据长度
** 返回:       校验码
********************************************************************/
INT32U YX_AsciiToBcd(INT8U *dptr, INT8U *sptr, INT32U slen)
{
    INT32U retlen, i;

    if (0 == slen) {
        return 0;
    }
    
    retlen = 0;
    if ((slen % 2) != 0) {
        *dptr++ = YX_CharToHex(*sptr++);
        slen--;
        retlen++;
    }
    
    for (i = 0; i < slen; i++) {
       if ((i % 2) == 0) {
           *dptr = (YX_CharToHex(*sptr++) << 4) & 0xF0;
       } else {
           *dptr += YX_CharToHex(*sptr++);
 	       dptr++;
 	       retlen++;
       }
   }
   
   return retlen;
}

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
void YX_EncodeStuffData(INT8U *dptr, INT16U maxlen, INT8U *sptr, INT16U slen, INT8U flag)
{
    if (slen > maxlen) {
        slen = maxlen;
    }
    
    for (; slen > 0; slen--, maxlen--) {
        *dptr++ = *sptr++;
    }
    
    for (; maxlen > 0; maxlen--) {
        *dptr++ = flag;
    }
}

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
INT16U YX_DecodeStuffData(INT8U *dptr, INT16U maxlen, INT8U *sptr, INT16U slen, INT8U flag)
{
    INT16U rlen;
    
    rlen = 0;
    for (; slen > 0 && maxlen > 0; slen--, maxlen--) {
        if (*sptr == flag) {
            break;
        }
        *dptr++ = *sptr++;
        rlen++;
    }
    return rlen;
}

/*******************************************************************
** 函数名:     YX_DecodeStuffDataLen
** 函数描述:   解码以填充字为结束符数据
** 参数:       [in]  sptr:   源数据指针
**             [in]  slen:   源数据长度
**             [in]  flag:   填充字
** 返回:       返回解码后长度
********************************************************************/
INT16U YX_DecodeStuffDataLen(INT8U *sptr, INT16U slen, INT8U flag)
{
    INT16U rlen;
    
    rlen = 0;
    for (; slen > 0; slen--) {
        if (*sptr++ == flag) {
            break;
        }
        rlen++;
    }
    return rlen;
}

/*******************************************************************
** 函数名:     YX_DecToAscii
** 函数描述:   10进制转化为ASCII码字符串,如0x11->"17"
** 参数:       [out] dptr:   目的数据指针
**             [in]  data:   待转换数据
**             [in]  reflen: 转换后长度,如果填0，则以实际转换位数为准,不在前面填充0
** 返回:       返回转换后长度
********************************************************************/
INT8U YX_DecToAscii(INT8U *dptr, INT32U data, INT8U reflen)
{
    INT8U i, len, temp;
    INT8U *tempptr;
    
    len     = 0;
    tempptr = dptr;
    for (;;) {
        *dptr++ = YX_HexToChar(data % 10);
        len++;
        if (data < 10) {
            break;
        } else {
            data /= 10;
        }
    }
    
    if (len != 0) {
        dptr = tempptr;
        for (i = 0; i < (len/2); i++) {
            temp = *(dptr + i);
            *(dptr + i) = *(dptr + (len - 1 - i));
            *(dptr + (len - 1 - i)) = temp;
        }
        
        if (reflen > len) {
            dptr = tempptr + (len - 1);
            for (i = 1; i <= reflen; i++) {
                if (i <= len) {
                    *(dptr + (reflen - len)) = *dptr;
                } else {
                    *(dptr + (reflen - len)) = '0';
                }
                dptr--;
            }
            len = reflen;
        }
    }
    return len;
}

/*******************************************************************
** 函数名:     YX_HexToBcd
** 函数描述:   HEX转换为BCD
** 参数:       [out] sptr:   待转换数据指针
**             [in]  slen:   待转换数据最大长度
**             [in]  dptr:   转换后数据
**             [in]  maxlen: 数据缓存最大长度
** 返回:       无
********************************************************************/
void YX_HexToBcd(INT8U *sptr, INT32U slen, INT8U *dptr, INT32U maxlen)
{
    INT32U i;
    
    if (maxlen < slen) {
        return;
    }
    
    for (i = 0; i < slen; i++) {
        dptr[i] = (sptr[i] / 10) * 16 + (sptr[i] % 10);
    }
}

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
INT16U YX_DeassembleByRules(INT8U *dptr, INT16U dlen, INT8U *sptr, INT16U slen, ASMRULE_T const *rule)
{
    INT16U rlen;
    INT8U  prechar, curchar, c_convert0;
    
    if (rule == 0) {
        return 0;
    }
    
    c_convert0 = rule->c_convert0;
    rlen = 0;
    prechar = (~c_convert0);
    for (; slen > 0; slen--) {
        curchar = *sptr++;
        if (prechar == c_convert0) {
            if (curchar == rule->c_convert1) {            /* c_convert0 + c_convert1 = c_flags */
                prechar = (~c_convert0);
                if (rlen < dlen) {
                    *dptr++ = rule->c_flags;
                    rlen++;
                } else {
                    return 0;
                }
            } else if (curchar == rule->c_convert2) {     /* c_convert0 + c_convert2 = c_convert0 */
                prechar = (~c_convert0);
                if (rlen < dlen) {
                    *dptr++ = c_convert0;
                    rlen++;
                } else {
                    return 0;
                }
            } else {
                return 0;
            }
        } else {
            if ((prechar = curchar) != c_convert0) {
                if (rlen < dlen) {
                    *dptr++ = curchar;
                    rlen++;
                } else {
                    return 0;
                }
            }
        }
    }
    return rlen;
}

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
INT16U YX_AssembleByRules(INT8U *dptr, INT16U dlen, INT8U *sptr, INT16U slen, ASMRULE_T const *rule)
{
    INT8U  temp;
    INT16U rlen;
    
    if (rule == 0) {
        return 0;
    }
    
    *dptr++ = rule->c_flags;
    rlen    = 1;
    for (; slen > 0; slen--) {
        temp = *sptr++;
        if (temp == rule->c_flags) {            /* c_flags    = c_convert0 + c_convert1 */
            if (rlen < dlen - 2) {
                *dptr++ = rule->c_convert0;
                *dptr++ = rule->c_convert1;
                 rlen += 2;
             } else {
                 return 0;
             }
        } else if (temp == rule->c_convert0) {  /* c_convert0 = c_convert0 + c_convert2 */
            if (rlen < dlen - 2) {
                *dptr++ = rule->c_convert0;
                *dptr++ = rule->c_convert2;
                rlen += 2;
            } else {
                return 0;
            }
        } else {
            if (rlen < dlen - 1) {
                *dptr++ = temp;
                rlen++;
            } else {
                 return 0;
             }
        }
    }
    
    *dptr = rule->c_flags;
    rlen++;
    return rlen;
}

//#if EN_AT > 0
static INT8U YX_UpperChar(INT8U ch)
{
    if (ch >= 'a' && ch <= 'z') 
        ch = 'A' + (ch - 'a');
    return ch;
}

static INT8U YX_CmpChar(BOOLEAN matchcase, INT8U ch1, INT8U ch2)
{
    if (matchcase != CASESENSITIVE) {
        ch1 = YX_UpperChar(ch1);
        ch2 = YX_UpperChar(ch2);
    }
    if (ch1 > ch2) return STR_GREAT;
    else if (ch1 < ch2) return STR_LESS;
    else return STR_EQUAL;
}

INT16U YX_FindCharPos(INT8U *sptr, char findchar, INT8U numchar, INT16U maxlen)
{
    INT16U pos;
    
    if (maxlen == 0) return 0;
    pos = 0;
    for (;;) {
        if (*sptr++ == findchar) {
            if (numchar == 0) break;
            else numchar--;
        }
        maxlen--;
        pos++;
        if (maxlen == 0) break;
    }
    return pos;
}

INT16U YX_SearchDigitalString(INT8U *ptr, INT16U maxlen, INT8U flagchar, INT8U numflag)
{
    BOOLEAN find;
    INT16U  result;
    volatile INT8U *bptr = 0, *eptr = 0;
    
    find = FALSE;
    for(;;) {
        if (maxlen == 0) {
            find = FALSE;
            break;
        }
        if (*ptr == flagchar) {
            numflag--;
            if (numflag == 0) {
                eptr = ptr;
                break;
            }
        }
        if (*ptr >= '0' && *ptr <= '9') {
            if (!find) {
                find = TRUE;
                bptr = ptr;
            }
        } else {
            find = FALSE;
        }
        ptr++;
        maxlen--;
    }
    
    if (find) {
        result = 0;
        for(;;) {
            result = result * 10 + YX_CharToHex(*bptr++);
            if (bptr == eptr) break;
        }
    } else {
        result = 0xffff;
    }
    return result;
}

INT16U YX_SearchString(INT8U *dptr, INT16U limitlen, INT8U *sptr, INT16U maxlen, INT8U flagchar, INT8U numflag)
{
    BOOLEAN find;
    INT16U  len;
    volatile INT8U *bptr, *eptr;
    
    find = FALSE;
    for(;;) {
        if (maxlen == 0) {
            find = FALSE;
            break;
        }
        if (*sptr == flagchar) {
            if (!find) {
                find = TRUE;
                bptr = sptr;
            } else {
                numflag--;
                if (numflag == 0) {
                    eptr = sptr;
                    break;
                } else {
                    find = FALSE;
                }
            }
        }
        sptr++;
        maxlen--;
    }
    
    if (find) {
        len = 0;
        bptr++;
        while(bptr != eptr) {
            if (len >= limitlen) {
                len = 0;
                break;
            }
            *dptr++ = *bptr++;
            len++;
        }
        return len;
    } else {
        return 0;
    }
}

INT8U YX_ACmpString(BOOLEAN matchcase, INT8U *ptr1, INT8U *ptr2, INT16U len1, INT16U len2)
{
    INT8U result;
    
    for (;;) {
        if (len1 == 0 && len2 == 0) return STR_EQUAL;
        if (len1 == 0) return STR_LESS;
        if (len2 == 0) return STR_GREAT;
        
        result = YX_CmpChar(matchcase, *ptr1++, *ptr2++);
        if (result != STR_EQUAL) {
            return result;
        } else {
            len1--;
            len2--;
        }
    }
}

BOOLEAN YX_SearchKeyWordFromHead(INT8U *ptr, INT16U maxlen, char *sptr)
{
    if (*sptr == 0) return FALSE;
    while(maxlen > 0) {
        if (*ptr++ == *sptr++) {
            if (*sptr == 0) return TRUE;
        } else {
            return FALSE;
        }
        maxlen--;
    }
    return FALSE;
}

/*******************************************************************
** 函数名:     YX_SearchKeyWord
** 函数描述:   搜索字符串
** 参数:       [in] ptr:    数据指针
**              [in] maxlen: 数据长度
**              [in] sptr:   需要在ptr中查找的字符串，以‘\0’为结束符
** 返回:       成功返回TRUE,失败返回false
********************************************************************/
BOOLEAN YX_SearchKeyWord(INT8U *ptr, INT16U maxlen, char *sptr)
{
    INT16U len;
    INT8U *xptr;
    char *cptr;

    if (*sptr == 0) return false;
    cptr = sptr;
    xptr = ptr;
    len  = maxlen;
    while(maxlen > 0) {
        if (*xptr++ == *cptr++) {
            if (*cptr == 0) return true;
            if (--len == 0) return false;
        } else {
            cptr = sptr;
            ptr++;
            maxlen--;
            xptr = ptr;
            len  = maxlen;
        }
    }
    return false;
}

/*******************************************************************
** 函数名:     YX_ConvertIpStringToHex
** 函数描述:   解析IP地址,如ip_string为：11.12.13.14，则转换后的ip_long为0x0B0C0D0E
** 参数:       [out] ip_long:  ip地址，依次从从高位到低位
**             [out] sbits:    子网位数
**             [in]  ip_string:ip字符串，以'\0'为结束符，如“11.12.13.14”
** 返回:       转换成功返回0，失败返回失败描述符
********************************************************************/
char *YX_ConvertIpStringToHex(INT32U *ip_long, INT8U *sbits, char *ip_string)
{
    char *cp;
    INT8U dots = 0;
    INT32U number, tempbyte;
    char *toobig = "over 255";
    
    if (YX_STRLEN(ip_string) > 15) {
        return ("too long");
    }
        
    cp = ip_string;
    while (*cp) {
        if ((*cp >= '0' && *cp <= '9') || *cp == '.') {
            if (*cp == '.') {
                dots++;
            }
        } else {
            return ("error chars");
        }
        cp++;
    }
    
    if (dots != 3 ) {
        return ("error dots");
    }
    
    number = 0;
    tempbyte = 0;
    cp = ip_string;
    while (*cp) {
        if (*cp == '.') {
            if (tempbyte > 255) {
                return toobig;
            }
                
            number <<= 8;
            number += tempbyte;
            tempbyte = 0;
        } else {
            tempbyte *= 10;
            tempbyte += (*cp - 0x30);
        }
        cp++;
    }
    number <<= 8;
    number += tempbyte;
    
    *ip_long = number;
    return 0; 
}

/*******************************************************************
** 函数名:     YX_ConvertIpHexToString
** 函数描述:   装欢IP地址,将hex格式转换为string格式,,如ip_string为0x0B0C0D0E，则转换后的ip_long为"11.12.13.14"
** 参数:       [in] ip:  hex格式ip地址
** 返回:       成功返回string格式IP地址,以"\0"为结束符
********************************************************************/
static char s_ipstring[20];
char *YX_ConvertIpHexToString(INT32U ip)
{
    INT8U i, ch, temp;
    
    YX_STRCPY(s_ipstring, "000.000.000.000");
    
    for (i = 0; i < 4; i++) {
        ch = ip >> ((4 - i - 1) * 8);
        temp = ch / 100;
        s_ipstring[i * 4] = temp + 0x30;
        temp = (ch / 10) % 10;
        s_ipstring[i * 4 + 1] = temp + 0x30;
        temp = ch % 10;
        s_ipstring[i * 4 + 2] = temp + 0x30;
    }
    return s_ipstring;
}

/***************************************************************
** 函数名:    YX_ConvertLatitudeOrLongitude
** 功能描述:  转换度+分+小数分格式为度数据格式(度×1000000格式)
** 参数:      [in] sptr: 度+分+小数分
** 返回值:    返回转换后值
***************************************************************/
INT32U YX_ConvertLatitudeOrLongitude(INT8U *sptr)
{
    if (sptr[1] >= 60) {
       sptr[0] += sptr[1]/60;
       sptr[1] %= 60;
    }
    return (sptr[0]*1000000L + (sptr[1]*1000000L + sptr[2]*10000L + sptr[3]*100L) / 60);
}

//#endif





