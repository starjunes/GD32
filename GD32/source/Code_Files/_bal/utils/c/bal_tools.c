/****************************************************************
**                                                              *
**  FILE         :  YX_Tools.C                                  *
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
#include "yx_includes.h"
#include "YX_Structs.h"
//#include "YX_System.h"
#include "stdlib.h"
#include "bal_tools.h"

/*
********************************************************************************
*                  DEFINE MASK
********************************************************************************
*/
static INT8U s_mask[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
static const INT8U s_month_day[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};


/*******************************************************************
** 函数名:     bal_u_chksum_1
** 函数描述:   单字节校验码算法
** 参数:       [in] ptr:      目标数据地址
**             [in] len:      目标数据长度
** 返回:       单字节校验码
********************************************************************/
 
//INT8U bal_u_chksum_1(INT8U *ptr, INT16U len)
INT8U bal_u_chksum_1(INT8U *ptr, INT32U len)
{
    INT32U i;
    HWORD_UNION chksum;
    
    chksum.hword = 0;
    for (i = 1; i <= len; i++) {
        chksum.hword += *ptr++;
        if (chksum.bytes.high > 0) {
            chksum.bytes.low += chksum.bytes.high;
            chksum.bytes.high = 0;
        }
        if ((i % 4096) == 0) {
            //  若文件较大，此处需
        	ClearWatchdog();
        }
    }
    chksum.bytes.low += 1;
    return chksum.bytes.low;
}

/*******************************************************************
** 函数名:     bal_u_chksum_1b
** 函数描述:   单字节校验码算法,数据取反码后再计算校验和
** 参数:       [in] ptr:      目标数据地址
**             [in] len:      目标数据长度
** 返回:       单字节校验码
********************************************************************/

//INT8U bal_u_chksum_1b(INT8U *ptr, INT16U len)
INT8U bal_u_chksum_1b(INT8U *ptr, INT32U len)
{
    INT32U i;
    HWORD_UNION chksum;
    
    chksum.hword = 0;
    for (i = 1; i <= len; i++) {
        chksum.hword += (INT8U)(~(*ptr++));
        if (chksum.bytes.high > 0) {
            chksum.bytes.low += chksum.bytes.high;
            chksum.bytes.high = 0;
        }
        if ((i % 4096) == 0) {
            //  若文件较大，此处需
        	ClearWatchdog();
        }
    }
    chksum.bytes.low += 1;
    return chksum.bytes.low;
}

/*******************************************************************
** 函数名:     bal_u_chksum_2
** 函数描述:   双字节校验码算法
** 参数:       [in] ptr:      目标数据地址
**             [in] len:      目标数据长度
** 返回:       双字节校验码
********************************************************************/

//INT16U bal_u_chksum_2(INT8U *ptr, INT16U len)
INT16U bal_u_chksum_2(INT8U *ptr, INT32U len)
{
    INT8U  temp;
    INT16U chksum;
    
    temp    = bal_u_chksum_1(ptr, len);
    chksum  = (INT16U)temp << 8;
    temp    = bal_u_chksum_1b(ptr, len);
    chksum |= temp;
    return chksum;
}


/*******************************************************************
** 函数名:     bal_GetChkSum_IP
** 函数描述:   
** 参数:       [in] dptr:      源数据地址
**             [in] len:       源数据长度
** 返回:       
********************************************************************/
INT8U bal_GetChkSum_IP(INT8U *dptr, INT16U len)   //yyy
{
    INT8U result;
	
    result = 0;
    while ( len > 0 ) {
    	if (*dptr == 0x19) {
    	 result += *dptr++;
    	 dptr++;      //跳过0X32
    	 len -= 2;
    	} else {
         result += *dptr++;
         len--;
        }
    }
    return result;
}

/*******************************************************************
** 函数名:     bal_MovXX
** 函数描述:   HEX数据拷贝函数
** 参数:       [in] dptr:      目标存储区地址
**             [in] sptr:      源数据地址
**             [in] len:       源数据长度
** 返回:       无
********************************************************************/
void bal_MovXX(INT8U *dptr, INT8U *sptr, INT16U len)
{
    for (; len > 0; len--) {
        *dptr++ = *sptr++;
    }
}

/*******************************************************************
** 函数名:     bal_MovStr
** 函数描述:   字符数据拷贝到无符号存储区
** 参数:       [in] dptr:      目标存储区地址
**             [in] sptr:      字符数据源地址
**             [in] len:       字符数据长度
** 返回:       无
********************************************************************/
void bal_MovStr(INT8U *dptr, char *sptr)
{
    while(*sptr) {
        *dptr++ = *sptr++;
    }
}

/*******************************************************************
** 函数名:     bal_InitBuf
** 函数描述:   存储区单一数据初始化函数
** 参数:       [in] dptr:      目标存储区地址
**             [in] initdata:  初始化数据
**             [in] len:       存储区需要初始化的长度
** 返回:       无
********************************************************************/
void bal_InitBuf(INT8U *dptr, INT8U initdata, INT16U len)
{
    for (; len > 0; len--) {
        *dptr++ = initdata;
    }
}

/*******************************************************************
** 函数名:     bal_GetChkSum
** 函数描述:   获取数据区的累加校验码
** 参数:       [in] dptr:      数据区地址
**             [in] len:       数据区长度
** 返回:       单字节累加校验码
********************************************************************/
INT8U bal_GetChkSum(INT8U *dptr, INT16U len)
{
    INT8U result;
	
    result = 0;
    for (; len > 0; len--) {
        result += *dptr++;
    }
    return result;
}

/*******************************************************************
** 函数名:     bal_GetChkSum_N
** 函数描述:   获取数据区的累加校验码的反码
** 参数:       [in] dptr:      数据区地址
**             [in] len:       数据区长度
** 返回:       单字节累加校验码反码
********************************************************************/
INT8U bal_GetChkSum_N(INT8U *dptr, INT16U len)
{
    return (~bal_GetChkSum(dptr, len));
}

/*******************************************************************
** 函数名:     bal_GetChkSum_C
** 函数描述:   获取数据区带进位累加校验码
** 参数:       [in] dptr:      数据区地址
**             [in] len:       数据区长度
** 返回:       单字节带进位累加校验码
********************************************************************/
INT8U bal_GetChkSum_C(INT8U *dptr, INT16U len)
{
    HWORD_UNION result;
	
    result.hword = 0;
    for (; len > 0; len--) {
        result.hword += *dptr++;
        if (result.bytes.high > 0) {
            result.bytes.low += result.bytes.high;
            result.bytes.high = 0;
        }
    }
    return (result.bytes.low);
}

/*******************************************************************
*   名称: bal_GetChkSum_CRC16
*   作者: 唐海勇  2009年9月3日
*
*   描述: 获取CRC16校验和
*   输入: [in] dptr:      数据区地址
*         [in] len:       数据区长度
*   返回: CRC16校验和
*******************************************************************/
INT16U bal_GetChkSum_CRC16(INT8U *dptr, INT16U len)
{
	INT16U  crc = 0xFFFF; 
	INT16U  i, j;
	BOOLEAN c15, bit;
	INT8U   tempdata;

	for (i = 0; i < len; i++) {
	    tempdata = *dptr++;
		for (j = 0; j < 8; j++) {
			c15 = ((crc >> 15 & 1) == 1);
			bit = ((tempdata >> (7 - j) & 1) == 1);
			crc <<= 1;
			if (c15 ^ bit) crc ^= 0x1021; 
		}
	}
	return crc;
}

/*******************************************************************
** 函数名:     bal_ChkSum_Xor
** 函数描述:   获取数据区异或校验码
** 参数:       [in] Ptr:       数据区地址
**             [in] Len:       数据区长度
** 返回:       单字节带异或校验码
********************************************************************/
INT8U bal_ChkSum_Xor(INT8U *Ptr,INT32U Len)
{
    INT8U Sum;
   
    Sum = 0;
    while (Len--) {
        Sum ^= *Ptr++;
    }
   
    return Sum;
}

INT8U bal_GetChkSum_7C(INT8U *dptr, INT16U len)
{
    INT8U result;
    
    result  = bal_GetChkSum_C(dptr, len);
    result &= 0x7f;
    if (result == 0) result = 0x7f;
    return result;
}

void bal_ConvertData(INT8U *dptr, INT16U len, INT8U ddata, INT8U sdata)
{
    for (; len > 0; len--) {
        if (*dptr == sdata) *dptr = ddata;
        dptr++;
    }
}

static INT8U bal_UpperChar(INT8U ch)
{
    if (ch >= 'a' && ch <= 'z') 
        ch = 'A' + (ch - 'a');
    return ch;
}
/*
static INT8U LowerChar(INT8U ch)
{
    if (ch >= 'A' && ch <= 'Z')
        ch = 'a' + (ch - 'A');
    return ch;
}
*/
static INT8U bal_CmpChar(BOOLEAN matchcase, INT8U ch1, INT8U ch2)
{
    if (matchcase != CASESENSITIVE) {
        ch1 = bal_UpperChar(ch1);
        ch2 = bal_UpperChar(ch2);
    }
    if (ch1 > ch2) return STR_GREAT;
    else if (ch1 < ch2) return STR_LESS;
    else return STR_EQUAL;
}

BOOLEAN bal_CmpString_Byte(INT8U *ptr, INT16U len, INT8U cb)
{
    if (len == 0) return FALSE;
    for (; len > 0; len--) {
        if (*ptr++ != cb) return FALSE;
    }
    return TRUE;
}

INT8U bal_CmpString(INT8U *ptr1, INT8U *ptr2, INT16U len)
{
    INT8U result;
    
    for (;;) {
        if (len == 0) return STR_EQUAL;
        result = bal_CmpChar(CASESENSITIVE, *ptr1++, *ptr2++);
        if (result != STR_EQUAL) return result;
        else len--;
    }
    //return STR_EQUAL;
}

INT8U bal_ACmpString(BOOLEAN matchcase, INT8U *ptr1, INT8U *ptr2, INT16U len1, INT16U len2)
{
    INT8U result;
    
    for (;;) {
        if (len1 == 0 && len2 == 0) return STR_EQUAL;
        if (len1 == 0) return STR_LESS;
        if (len2 == 0) return STR_GREAT;
        
        result = bal_CmpChar(matchcase, *ptr1++, *ptr2++);
        if (result != STR_EQUAL) return result;
        else {
            len1--;
            len2--;
        }
    }
    //return STR_EQUAL;
}

INT16U bal_Bit7ToOctet(INT8U *dptr, INT8U *sptr, INT16U len)
{
    INT8U  j, pos, stemp, dtemp;
    INT16U rlen;
    
    pos   = 0;
    rlen  = 0;
    dtemp = 0;
    for (; len > 0; len--) {
        stemp = *sptr++;
        for (j = 0; j <= 6; j++) {
            if (stemp & s_mask[j]) dtemp |= s_mask[pos];
            pos++;
            if (pos == 8) {
               *dptr++ = dtemp;
               pos   = 0;
               dtemp = 0;
               rlen++;
            }
        }
    }
    if (pos != 0) {
        *dptr = dtemp;
        rlen++;
    }
    return rlen;
}

INT16U bal_OctetToBit7(INT8U *dptr, INT8U *sptr, INT16U len, INT8U leftbit)
{
    INT8U  j, pos, stemp, dtemp;
    INT16U rlen;
    
    pos   = 0;
    rlen  = 0;
    dtemp = 0;
    for (; len > 0; len--) {
        stemp = *sptr++;
        for (j = (8 - leftbit); j <= 7; j++) {
            if (stemp & s_mask[j]) dtemp |= s_mask[pos];
            pos++;
            if (pos == 7) {
                *dptr++ = dtemp;
                pos   = 0;
                dtemp = 0;
                rlen++;
                if (len == 1 && j != 0) break;
            }
        }
        leftbit = 8;
    }
    return rlen;
}

INT8U bal_HexToChar(INT8U sbyte)
{
    sbyte &= 0x0F;
    if (sbyte < 0x0A) return (sbyte + '0');
    else return (sbyte - 0x0A + 'A');
}

INT8U bal_CharToHex(INT8U schar)
{
    if (schar >= '0' && schar <= '9') return (schar - '0');
    else if (schar >= 'A' && schar <= 'F') return (schar - 'A' + 0x0A);
    else if (schar >= 'a' && schar <= 'f') return (schar - 'a' + 0x0A);
    else return 0;
}

INT8U bal_SemiOctetToChar(INT8U sbyte)
{
    sbyte &= 0x0F;
    switch(sbyte)
    {
        case 0x0A:
            return '*';
        case 0x0B:
            return '#';
        case 0x0C:
            return 'a';
        case 0x0D:
            return 'b';
        case 0x0E:
            return 'c';
        case 0x0F:
            return 0;
        default:
            return (sbyte + '0');
    }
}

INT8U bal_CharToSemiOctet(INT8U schar)
{
    if (schar >= '0' && schar <= '9') return (schar - '0');
    switch(schar)
    {
        case '*':
            return 0x0A;
        case '#':
            return 0x0B;
        case 'a':
            return 0x0C;
        case 'b':
            return 0x0D;
        case 'c':
            return 0x0E;
        default:
            return 0x0F;
    }
}

INT16U bal_HexToAscii(INT8U *dptr, INT8U *sptr, INT16U len)
{
    INT16U i;
    INT8U  stemp;
    
    for (i = 1; i <= len; i++) {
        stemp = *sptr++;
        *dptr++ = bal_HexToChar(stemp >> 4);
        *dptr++ = bal_HexToChar(stemp & 0x0F);
    }
    return (2 * len);
}

INT16U bal_AsciiToHex(INT8U *dptr, INT8U *sptr, INT16U len)
{
    INT16U i;
    INT8U  dtemp, stemp;
    
    if (len % 2) return 0;
    len /= 2;
    for (i = 1; i <= len; i++) {
        stemp   = bal_CharToHex(*sptr++);
        dtemp   = stemp << 4;
        *dptr++ = bal_CharToHex(*sptr++) | dtemp;
    }
    return len;
}

/* SAMPLE: 7899 -----> '7','8','9','9' */
INT8U bal_DecToAscii(INT8U *dptr, INT32U data, INT8U reflen)
{
    INT8U i, len, temp;
    INT8U *tempptr;
    
    len     = 0;
    tempptr = dptr;
    for (;;) {
        *dptr++ = bal_HexToChar(data % 10);
        len++;
        if (data < 10) break;
        else data /= 10;
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
                if (i <= len) *(dptr + (reflen - len)) = *dptr;
                else *(dptr + (reflen - len)) = '0';
                dptr--;
            }
            len = reflen;
        }
    }
    return len;
}

/* SAMPLE: '7','8','9','9' -----> 7899 */
INT32U bal_AsciiToDec(INT8U *sptr, INT8U len)
{
    INT32U result;
    
    result = 0;
    for (; len > 0; len--) {
        result = result * 10 + bal_CharToHex(*sptr++);
    }
    return result;
}

INT16U bal_SemiOctetToAscii(INT8U *dptr, INT8U *sptr, INT16U len)
{
    INT16U rlen;
    INT8U  stemp, temp;
    
    rlen = 0;
    for (; len > 0; len--) {
        stemp = *sptr++;
        if ((temp = bal_SemiOctetToChar(stemp)) == 0) break;
        else {
            *dptr++ = temp;
            rlen++;
            stemp >>= 4;
            if ((temp = bal_SemiOctetToChar(stemp)) == 0) break;
            else {
                *dptr++ = temp;
                rlen++;
            }
        }
    }
    return rlen;
}

void bal_SemiOctetToHex(INT8U *dptr, INT8U *sptr, INT16U len)
{
    INT8U stemp, temp;
    
    for (; len > 0; len--) {
        stemp   = *sptr++;
        temp    = stemp >> 4;
        stemp  &= 0x0F;
        *dptr++ = stemp*10 + temp;
    }
}

INT16U bal_AsciiToSemiOctet(INT8U *dptr, INT8U *sptr, INT16U len)
{
    INT16U  rlen;
    INT8U   stemp;
    INT8U 	dtemp = 0;
    BOOLEAN isodd;
    
    rlen   = 0;
    isodd  = TRUE;
    for (; len > 0; len--) {
        stemp = *sptr++;
        if ((stemp = bal_CharToSemiOctet(stemp)) == 0x0F) break;
        if (isodd) {
            isodd = FALSE;
            dtemp = stemp;
        } else {
            isodd   = TRUE;
            stemp <<= 4;
            *dptr++ = dtemp | stemp;
            rlen++;
        }
    }
    if (!isodd) {
        *dptr = dtemp | 0xF0;
        rlen++;
    }
    return rlen;
}

/*********************************************************************************
** 函数名:     bal_ConvertGmtToLocaltime
** 函数描述:   将格林尼治时间转换成本地时间
** 参数:       [in/out]  date:     日期
**             [in/out]  time:     时间
**             [in]      diftime:  时差
** 返回:       无
*********************************************************************************/
void bal_ConvertGmtToLocaltime(DATE_T *date, TIME_T *time, INT8U diftime)
{
    INT8U curday;
    
    if (date->month == 2 && (date->year % 4) == 0) {
        curday = 29;
    } else {
        curday = s_month_day[date->month - 1];
    }

    if (time->second >= 60) {
        time->second -= 60;
        time->minute++;
    }
    if (time->minute >= 60) {
        time->minute -= 60;
        time->hour++;
    }
    time->hour += diftime;
    if (time->hour >= 24) {
        time->hour -= 24;
        date->day++;
    }
    
    if (date->day > curday) {
        date->day = 1;
        date->month++;
    }
    if (date->month > 12) {
        date->month = 1;
        date->year++;
    }
}

/*********************************************************************************
** 函数名:     bal_ConvertLocaltimeToGmt
** 函数描述:   将本地时间转换成格林尼治时间
** 参数:       [in/out]  date:     日期
**             [in/out]  time:     时间
**             [in]      diftime:  时差
** 返回:       无
*********************************************************************************/
void bal_ConvertLocaltimeToGmt(DATE_T *date, TIME_T *time, INT8U diftime)
{
    INT8U curday, dec;
    
    if (date->month > 1) {                                                     /* 大于2月 */
        if ((date->month - 1) == 2 && (date->year % 4) == 0) {
            curday = 29;
        } else {
            curday = s_month_day[date->month - 1 - 1];
        }
    } else {
        curday = s_month_day[11];
    }

    if (time->second >= 60) {
        time->second -= 60;
        time->minute++;
    }
    if (time->minute >= 60) {
        time->minute -= 60;
        time->hour++;
    }
    
    dec = 0;
    if (time->hour < diftime) {
        time->hour += 24;
        dec = 1;
    }
    time->hour -= diftime;
    
    if (dec) {                                                                 /* 凌晨0-diftime */
        if (date->day > 1) {
            dec = 0;
            date->day -= 1;
        } else {
            dec = 1;
            date->day = curday;
        }
    }
        
    if (dec) {                                                                 /* 每月1号 */
        if (date->month > 1) {
            dec = 0;
            date->month -= 1;
        } else {
            dec = 1;
            date->month = 12;
        }
    }
    
    if (dec) {
        date->year--;
    }
}

BOOLEAN bal_SearchDataInMem(INT8U *bptr, INT16U blen, INT8U *sptr, INT16U slen)
{
    INT8U *tempptr;
    INT16U templen;
    
    if ((slen == 0) ||(slen > blen)) return FALSE;
    tempptr = sptr;
    templen = slen;
    while(blen > 0) {
        if (*bptr == *tempptr) {
			bptr++;
			tempptr++;
 	        blen--;
            if (--templen == 0) return TRUE;
        } else {
			if (tempptr == sptr) {
				bptr++;
				blen--;
			}
            tempptr = sptr;
            templen = slen;
			continue;
        }
    }
    return FALSE;
}

/*******************************************************************
**  函数名:     bal_SearchKeyWord
**  函数描述:   搜索字符串
**  参数:       [in] ptr:    数据指针
**              [in] maxlen: 数据长度
**              [in] sptr:   需要在ptr中查找的字符串，以‘\0’为结束符
**  返回:       成功返回TRUE,失败返回false
********************************************************************/
BOOLEAN bal_SearchKeyWord(INT8U *ptr, INT16U maxlen, char *sptr)
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
** 函数名:      bal_ASearchKeyWord
** 函数描述:    搜索字符串，可以区分大小写，在s1中搜索是否包含有s2
** 参数:        [in] matchcase:true-区分大小写，false-不区分大小写
**              [in] s1：      原字符串
**              [in] len1：    s1长度
**              [in] s2：      要查找的字符串
**              [in] len2：    s2长度
** 返回:        成功返回true，失败返回false
********************************************************************/
BOOLEAN bal_ASearchKeyWord(BOOLEAN matchcase, INT8U *s1, INT16U len1, INT8U *s2, INT16U len2)
{
    INT16U cmplen1, cmplen2;
    INT8U ch1, ch2;
    INT8U *xptr, *cptr;

    if (len2 == 0 || len1 == 0 || len2 > len1) {
        return false;
    }
        
    xptr = s1;
    cptr = s2;
    cmplen1 = len1;
    cmplen2 = len2;
    while (len1 > 0) {
        ch1 = *xptr++;
        ch2 = *cptr++;
        if (matchcase == false) {                                              /* 不区分大小写 */
            ch1 = bal_UpperChar(ch1);
            ch2 = bal_UpperChar(ch2);
        }
        if (ch1 == ch2) {
            if (--cmplen2 == 0) {
                return true;
            }
            if (--cmplen1 == 0) {
                return false;
            }
        } else {
            cptr = s2;
            cmplen2 = len2;
            
            s1++;
            len1--;
            xptr = s1;
            cmplen1 = len1;
        }
    }
    return false;
}

BOOLEAN bal_SearchKeyWordFromHead(INT8U *ptr, INT16U maxlen, char *sptr)
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

//#pragma O0
INT16U bal_SearchDigitalString(INT8U *ptr, INT16U maxlen, INT8U flagchar, INT8U numflag)
{
    BOOLEAN find;
    INT16U  result;
    INT8U *bptr = NULL;
    INT8U *eptr = NULL;
    
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
            result = result * 10 + bal_CharToHex(*bptr++);
            if (bptr == eptr) break;
        }
    } else {
        result = 0xffff;
    }
    return result;
}

INT16U bal_SearchHexString(INT8U *ptr, INT16U maxlen, INT8U flagchar, INT8U numflag)
{
    BOOLEAN find;
    INT16U  result;
    INT8U *bptr = NULL;
    INT8U *eptr = NULL;
    
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
        if ((*ptr >= '0' && *ptr <= '9') || (*ptr >= 'a' && *ptr <= 'f') || (*ptr >= 'A' && *ptr <= 'F')) {
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
            result = result * 16 + bal_CharToHex(*bptr++);
            if (bptr == eptr) break;
        }
    } else {
        result = 0xffff;
    }
    return result;
}

INT16U bal_SearchString(INT8U *dptr, INT16U limitlen, INT8U *sptr, INT16U maxlen, INT8U flagchar, INT8U numflag)
{
    BOOLEAN find;
    INT16U  len;
    INT8U *bptr = NULL;
    INT8U *eptr = NULL;
    
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


//#pragma O1
INT16U bal_FindCharPos(INT8U *sptr, char findchar, INT8U numchar, INT16U maxlen)
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

BOOLEAN bal_DecGpsData(INT8U *result, INT8U *data1, INT8U *data2)
{
    INT8U   cmpresult;
    BOOLEAN carried;
    INT8U *temp;
    
    cmpresult = bal_CmpString(data1, data2, 4);
    if (cmpresult == STR_LESS) {
        temp  = data1;
        data1 = data2;
        data2 = temp;
    }
    
    if (data1[3] < data2[3]) {
        result[3] = (data1[3] + 100) - data2[3];
        carried   = TRUE;
    } else {
        result[3] = data1[3] - data2[3];
        carried   = FALSE;
    }
    
    if (data1[2] < (data2[2] + (INT8U)carried)) {
        result[2] = (data1[2] + 100) - (data2[2] + (INT8U)carried);
        carried   = TRUE;
    } else {
        result[2] = data1[2] - (data2[2] + (INT8U)carried);
        carried   = FALSE;
    }
    
    if (data1[1] < (data2[1] + (INT8U)carried)) {
        result[1] = (data1[1] + 60) - (data2[1] + (INT8U)carried);
        carried   = TRUE;
    } else {
        result[1] = data1[1] - (data2[1] + (INT8U)carried);
        carried   = FALSE;
    }
    
    if (data1[0] < (data2[0] + (INT8U)carried)) {
        result[0] = (data1[0] + 100) - (data2[0] + (INT8U)carried);
        carried   = TRUE;
    } else {
        result[0] = data1[0] - (data2[0] + (INT8U)carried);
        carried   = FALSE;
    }
    
    if (cmpresult == STR_LESS) return TRUE;
    else return FALSE;
}

void bal_AddGpsData(INT8U *result, INT8U *data1, INT8U *data2)
{
    BOOLEAN carried;
    
    result[3] = data1[3] + data2[3];
    if (result[3] >= 100) {
        carried    = TRUE;
        result[3] -= 100;
    } else {
        carried    = FALSE;
    }
    
    result[2] = data1[2] + data2[2] + (INT8U)(carried);
    if (result[2] >= 100) {
        carried    = TRUE;
        result[2] -= 100;
    } else {
        carried    = FALSE;
    }
    
    result[1] = data1[1] + data2[1] + (INT8U)carried;
    if (result[1] >= 60) {
        carried    = TRUE;
        result[1] -= 60;
    } else {
        carried    = FALSE;
    }
    
    result[0] = data1[0] + data2[0] + (INT8U)carried;
}

BOOLEAN bal_ShieldHighBit(INT8U *data)
{
    INT8U temp;
    
    temp = *data;
    if (temp == 0x00) temp = 0xff;
    else if (temp == 0x80) temp = 0xfe;
    
    *data = temp & 0x7f;
    if (temp & 0x80) return TRUE;
    else return FALSE;
}

//#if EN_OPEN_PROTOCOL > 0
static INT8U CodeTbl7to6[] = { 0x30, 0x38, 0x47, 0x4F, 0x57, 0x65, 0x6D, 0x75
                              ,0x31, 0x39, 0x48, 0x50, 0x58, 0x66, 0x6E, 0x76
                              ,0x32, 0x41, 0x49, 0x51, 0x59, 0x67, 0x6F, 0x77  
                              ,0x33, 0x42, 0x4A, 0x52, 0x5A, 0x68, 0x70, 0x78
                              ,0x34, 0x43, 0x4B, 0x53, 0x61, 0x69, 0x71, 0x79
                              ,0x35, 0x44, 0x4C, 0x54, 0x62, 0x6A, 0x72, 0x7A
                              ,0x36, 0x45, 0x4D, 0x55, 0x63, 0x6B, 0x73, 0x2C
                              ,0x37, 0x46, 0x4E, 0x56, 0x64, 0x6C, 0x74, 0x2E
                            };
//#endif
static INT8U CodeTbl[] = { 0x2e, 0x2c, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,          /* .,012345 */
                           0x36, 0x37, 0x38, 0x39, 0x41, 0x42, 0x43, 0x44,          /* 6789ABCD */
                           0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c,          /* EFGHIJKL */
                           0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54,          /* MNOPQRST */
                           0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x61, 0x62,          /* UVWXYZab */
                           0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a,          /* cdefghij */
                           0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72,          /* klmnopqr */
                           0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a           /* stuvwxyz */
                         };

static INT8U Encode(INT8U data, INT8U type, BOOLEAN openpro)
{
    if (type == MODE_CODE8 || !openpro) {
        return CodeTbl[data & 0x3f];
    } else {
        return CodeTbl7to6[data & 0x3f];
    }
}

static INT8U Decode(INT8U data,INT8U type, BOOLEAN openpro)   
{
    INT8U i;
    INT8U *ptr;

    if (type == MODE_CODE8 || !openpro) {
        ptr = CodeTbl;
    } else {
        ptr = CodeTbl7to6;
    }
    for (i = 0; i < sizeof(CodeTbl); i++) {
        if (data == ptr[i]) {
            return i;
        }
    }
    
    return 0xff;
}

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
//#pragma O0
INT16U bal_CodeX_To_Code6(INT8U mode, INT8U *dptr, INT16U limitlen, INT8U *sptr, INT16U len, BOOLEAN openpro)
{
    INT8U  i, j;
    INT8U stemp = 0;
    INT8U dtemp = 0;
    INT16U rlen;
    
    if (len == 0) return 0;

    rlen  = 0;
    i     = 0;
    j     = 6;
    dtemp = 0;
    for (;;) {
        if (i == 0) {
            if (len == 0) {
                if (rlen >= limitlen) {
                    rlen = 0;
                } else {
                    *dptr = Encode(dtemp, mode, openpro);
                    rlen++;
                }
                break;
            }
            if (mode == MODE_CODE8) i = 8;
            else i = 7;
            stemp = *sptr++;
            len--;
        }
        if (j == 0) {
            if (rlen >= limitlen) {
                rlen = 0;
                break;
            }
            j = 6;
            *dptr++ = Encode(dtemp, mode, openpro);
            dtemp   = 0;
            rlen++;
        }
        if (stemp & s_mask[i - 1]) dtemp |= s_mask[j - 1];
        i--;
        j--;
    }
    return rlen;
}

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
INT16U bal_Code6_To_CodeX(INT8U mode, INT8U *dptr, INT16U limitlen, INT8U *sptr, INT16U len, BOOLEAN openpro)
{
    INT8U  i, j;
    INT8U stemp = 0;
    INT8U dtemp = 0;
    INT16U rlen;
    
    rlen  = 0;
    i     = 0;
    
    if (mode == MODE_CODE8) j = 8;
    else j = 7;
    for (;;) {
        if (j == 0) {
            if (mode == MODE_CODE8) j = 8;
            else j = 7;
            if (rlen > limitlen) {
                rlen = 0;
                break;
            }
                
            *dptr++ = dtemp;
            rlen++;
            dtemp = 0;
            if (len == 0) break;
        }
        if (i == 0) {
            if (len == 0) {
                rlen = 0;
                break;
            }
            if ((stemp = Decode(*sptr++, mode, openpro)) == 0xff) {
                rlen = 0;
                break;
            } else {
                i = 6;
                len--;
            }
        }
        if (stemp & s_mask[i - 1]) dtemp |= s_mask[j - 1];
        i--;
        j--;
    }
    return rlen;
}

//#pragma O1
void bal_EncodeTel(INT8U *dptr, INT8U *sptr, INT16U len, INT16U maxlen)
{
    if (len > maxlen) len = maxlen;
    for (; len > 0; len--, maxlen--) {
        *dptr++ = *sptr++;
    }
    for (; maxlen > 0; maxlen--) {
        *dptr++ = ' ';
    }
}

INT16U bal_DecodeTel(INT8U *dptr, INT8U *sptr, INT16U len)
{
    INT16U rlen;
    
    rlen = 0;
    for (; len > 0; len--) {
        if (*sptr == ' ') break;
        *dptr++ = *sptr++;
        rlen++;
    }
    return rlen;
}

INT16U bal_DecodeTellen(INT8U *sptr, INT16U len)
{
    INT16U rlen;
    
    rlen = 0;
    for (; len > 0; len--) {
        if (*sptr++ == ' ') break;
        rlen++;
    }
    return rlen;
}

BOOLEAN bal_FindProcEntry(INT16U index, FUNCENTRY_T *funcentry, INT16U num)
{
    for (; num > 0; num--, funcentry++) {
        if (index == funcentry->index) {
            if (funcentry->entryproc != 0) {
                funcentry->entryproc();
                return TRUE;
            }
        }
    }
    return FALSE;
}


//#pragma O0
static BOOLEAN YX_Adjust_Thousand(INT16U *ptr, INT8U len)
{
    INT16U carried;
    
    carried = 0;
    for (; len > 0; len--) {
        *ptr += carried;
        if (*ptr > 999) {
            carried = *ptr / 1000;
            *ptr   %= 1000;
        } else {
            carried = 0;
        }
        ptr++;
    }

    if (carried) 
        return TRUE;
    else 
        return FALSE;
}

BOOLEAN bal_Add_Thousand(INT16U *add1, INT16U *add2, INT8U len1, INT8U len2)
{
    INT8U tlen;
    INT16U *tadd;
    
    if (YX_Adjust_Thousand(add1, len1)) return TRUE;
    if (YX_Adjust_Thousand(add2, len2)) return TRUE;

    tadd = add1;
    tlen = len1;
    for (;; len1--, len2--) {
        if (len1 == 0 || len2 == 0) {
            if (len2 > 0) {
                for (; len2 > 0; len2--) {
                    if (*add2++ > 0) return TRUE;
                }
            }
            break;
        }
        *add1++ += *add2++;
    }
    
    return YX_Adjust_Thousand(tadd, tlen);
}

BOOLEAN bal_Dec_Thousand(INT16U *dec1, INT16U *dec2, INT8U len1, INT8U len2)
{
    INT16U carried, temp;
    
    if (YX_Adjust_Thousand(dec1, len1)) return TRUE;
    if (YX_Adjust_Thousand(dec2, len2)) return TRUE;
    
    carried = 0;
    for (;;) {
        if (len1 == 0) {
            if (len2 > 0) {
                for (; len2 > 0; len2--) {
                    if (*dec2++ > 0) return TRUE;
                }
            }
            if (carried)
                return TRUE;
            else
                return FALSE;
        }
        
        if (len2 > 0) {
            temp = *dec2 + carried;
        } else {
            temp = carried;
        }

        if (*dec1 < temp) {
            *dec1  += (1000 - temp);
            carried = 1;
        } else {
            *dec1  -= temp;
            carried = 0;
        }
        
        dec1++;
        len1--;
        if (len2 > 0) {
            dec2++;
            len2--;
        }
    }
    //return TRUE;
}

void bal_Mul_Thousand(INT16U *result, INT16U mul1, INT16U mul2)
{
    INT32U temp;
    
    temp = mul1 * mul2;
    result[0] = temp % 1000;
    result[1] = temp / 1000;
    if (result[1] > 999) result[1] = 999;
}

INT8U bal_ThousandToAscii(INT8U *ptr, INT16U opdata)
{
    *ptr++  = bal_HexToChar(opdata / 100);
    opdata %= 100;
    *ptr++  = bal_HexToChar(opdata / 10);
    opdata %= 10;
    *ptr    = bal_HexToChar(opdata);
    return 3;
}

//#pragma O1
INT16U bal_DeassembleByRules(INT8U *dptr, INT8U *sptr, INT16U len, ASMRULE_T *rule)
{
    INT16U rlen;
    INT8U  prechar, curchar, c_convert0;
    
    if (rule == NULL) return 0;
    c_convert0 = rule->c_convert0;
    rlen = 0;
    prechar = (~c_convert0);
    for (; len > 0; len--) {
        curchar = *sptr++;
        if (prechar == c_convert0) {
            if (curchar == rule->c_convert1) {            /* c_convert0 + c_convert1 = c_flags */
                prechar = (~c_convert0);
                *dptr++ = rule->c_flags;
                rlen++;
            } else if (curchar == rule->c_convert2) {     /* c_convert0 + c_convert2 = c_convert0 */
                prechar = (~c_convert0);
                *dptr++ = c_convert0;
                rlen++;
            } else {
                return 0;
            }
        } else {
            if ((prechar = curchar) != c_convert0) {
                *dptr++ = curchar;
                rlen++;
            }
        }
    }
    return rlen;
}

INT16U bal_AssembleByRules(INT8U *dptr, INT8U *sptr, INT16U len, ASMRULE_T *rule)
{
    INT8U  temp;
    INT16U rlen;
    
    if (rule == NULL) return 0;
    
    *dptr++ = rule->c_flags;
    rlen    = 1;
    for (; len > 0; len--) {
        temp = *sptr++;
        if (temp == rule->c_flags) {            /* c_flags    = c_convert0 + c_convert1 */
            *dptr++ = rule->c_convert0;
            *dptr++ = rule->c_convert1;
            rlen += 2;
        } else if (temp == rule->c_convert0) {  /* c_convert0 = c_convert0 + c_convert2 */
            *dptr++ = rule->c_convert0;
            *dptr++ = rule->c_convert2;
            rlen += 2;
        } else {
            *dptr++ = temp;
            rlen++;
        }
    }
    *dptr = rule->c_flags;
    rlen++;
    return rlen;
}

BOOLEAN bal_DateValid(DATE_T *date)
{
    if (date->year >= 29) return FALSE;
    if ((date->month > 12) || (date->month == 0)) return FALSE;
    if ((date->day > 31) || (date->day == 0)) return FALSE;
    else return TRUE;
}

BOOLEAN bal_TimeValid(TIME_T *time)
{
    if (time->hour > 24)   return FALSE;
    if (time->minute > 60) return FALSE;
    if (time->second > 60) return FALSE;
    else return TRUE;
}

BOOLEAN bal_LongitudeValid(INT8U *longitude)
{
    if (longitude[0] > 180) return FALSE;
    if (longitude[1] > 60)  return FALSE;
    if (longitude[2] > 99)  return FALSE;
    if (longitude[3] > 99)  return FALSE;
    else return TRUE;
}

BOOLEAN bal_LatitudeValid(INT8U *latitude)
{
    if (latitude[0] > 90) return FALSE;
    if (latitude[1] > 60) return FALSE;
    if (latitude[2] > 99) return FALSE;
    if (latitude[3] > 99) return FALSE;
    else return TRUE;
}
/* // 老手柄程序用到的函数,暂屏蔽
BOOLEAN NeedRoundLine(INT8U *ptr, INT16U len)
{
    BOOLEAN isgbcode;
    
    for (isgbcode = FALSE; len > 0; ptr++, len--) {
        if (!isgbcode) {
            if (*ptr >= 0xa1) isgbcode = TRUE;
        } else {
            if (*ptr >= 0xa1) isgbcode = FALSE;
        }
    }
    return isgbcode;
}

INT16S PlotLinePos(INT8U *textptr, INT16U textlen, INT16U *linepos, INT16U maxline, INT16U linesize)
{
    INT16U pos, len, line;
    
    pos = line = len = linepos[0] = 0;
    if (textlen == 0) return 0;
    
    for (;;) {
        if (*textptr++ == '\n') {
            if (++line > maxline) return -1;
            len = 0;
            linepos[line] = ++pos;
        } else {
            pos++;
            if (++len == linesize) {
                if (NeedRoundLine(textptr - linesize, linesize)) {
                    pos--;
                    textptr--;
                }
                if (++line > maxline) return -1;
                len = 0;
                linepos[line] = pos;
            }
        }
        if (pos > textlen - 1) {
            if (len == 0) line--;
            break;
        }
    }
    return (line + 1);
}

INT16U InsertEditBuf(INT8U *ptr, INT16U len, INT16U pos, INT16U ch)
{
    INT16U i;
    INT8U  temp, *tempptr, ret;
    
    if (pos > len) return len;
    if (ch >= 0xA100) 
        ret = 2;
    else 
        ret = 1;
    for (;;) {
        tempptr = ptr + len;
        for (i = 1; i <= len - pos; i++) {
            temp       = *(tempptr - 1);
            *tempptr-- = temp;
        }
        if (ret == 2) 
            *tempptr = ch >> 8;
        else 
            *tempptr = ch;
        len++;
        pos++;
        if (--ret == 0) return len;
    }
}

INT8U DelEditBuf(INT8U *ptr, INT16U len, INT16U pos)
{
    INT16U j;
    INT8U  i, temp, *tempptr, ret;
    
    ret = 0;
    if (pos > len || pos == 0) return 0;
    if (ptr[pos - 1] >= 0xA1) 
        ret = 2;
    else 
        ret = 1;
    if (pos < ret) return 0;
    for (i = 1; i <= ret; i++) {
        tempptr = ptr + pos;
        for (j = 1; j <= (len - pos); j++) {
            temp           = *tempptr;
            *(tempptr - 1) = temp;
            tempptr++;
        }
        len--;
        pos--;
    }
    return ret;
}

void WriteWordByPointer(INT8U *ptr, INT16U wdata)
{
    *ptr++ = wdata >> 8;
    *ptr   = wdata;
}

INT16U ReadWordByPointer(INT8U *ptr)
{
    INT16U temp;
    
    temp  = *ptr++;
    temp  <<= 8;
    temp |= *ptr;
    return temp;
}

INT8U FormatYear(INT8U *ptr, INT8U year, INT8U month, INT8U day)
{
    if (year > 99) year = 99;
    if (month > 12 || month == 0) month = 12;
    if (day > 31 || day == 0) day = 1;
    *ptr++ = '2';
    *ptr++ = '0';
    DecToAscii(ptr, year, 2);
    ptr   += 2;
    *ptr++ = '.';
    DecToAscii(ptr, month, 2);
    ptr   += 2;
    *ptr++ = '.';
    DecToAscii(ptr, day, 2);
    return (2 + 2 + 1 + 2 + 1 + 2);
}

INT8U FormatTime(INT8U *ptr, INT8U hour, INT8U minute, INT8U second)
{
    minute += second / 60;
    second %= 60;
    hour   += minute / 60;
    minute %= 60;
    hour   %= 24;

    DecToAscii(ptr, hour, 2);
    ptr   += 2;
    *ptr++ = ':';
    DecToAscii(ptr, minute, 2);
    ptr   += 2;
    *ptr++ = ':';
    DecToAscii(ptr, second, 2);
    return (2 + 1 + 2 + 1 + 2);
}

INT8U  CutRedundantLen(INT16U len,INT16U standardlen)
{
      if (len < standardlen) {
         return len;
      } else {
         return standardlen;
      }      
}
*/
//add by qzc
INT8U bal_GetSmall(INT8U Data1,INT8U Data2)
{
  if(Data1<Data2) return Data1;
  else return Data2;
}

INT8U bal_CmpData(INT8U *Buf1,INT8U *Buf2,INT8U Len1,INT8U Len2)
{
   INT8U Result;

   Result = bal_CmpString(Buf1,Buf2,bal_GetSmall(Len1,Len2));
   if(Result==STR_EQUAL)
     {
      if(Len1>Len2) return STR_GREAT;
      else if(Len1<Len2) return STR_LESS;
     }
   return Result;
}

BOOLEAN bal_IsInRange(INT8U Data,INT8U Low,INT8U High)
{
   if((Data>=Low)&&(Data<=High)) return TRUE;
   return FALSE;
}

//(A In B=SUBCLASS)
INT8U bal_RangeRelation(INT8U *A1,INT8U *A2,INT8U *B1,INT8U *B2,INT8U Len)
{
   if(bal_CmpString(A1,A2,Len)==STR_GREAT) return VACUUM;
   if(bal_CmpString(B1,B2,Len)==STR_GREAT) return VACUUM;

   switch(bal_CmpString(A1,B1,Len))
     {
      case STR_LESS: if(bal_CmpString(A2,B1,Len)==STR_LESS) return VACUUM;
                     return OVERLAP;
      case STR_GREAT:if(bal_CmpString(A2,B2,Len)!=STR_GREAT) return SUBCLASS;
                     if(bal_CmpString(A1,B2,Len)==STR_GREAT) return VACUUM;
                     return OVERLAP;
      case STR_EQUAL:if(bal_CmpString(A2,B2,Len)==STR_GREAT) return OVERLAP;
                     return SUBCLASS;
      default:return MISS;
     }
}

//#pragma O0
INT16U bal_Code8_To_Code7(INT8U *dptr, INT8U *sptr, INT16U len, INT16U limitlen)
{
    INT8U  i, j; 
    INT8U stemp = 0;
    INT8U dtemp = 0;
    INT16U rlen;
    
    if (len == 0) return 0;
    rlen  = 0;
    i     = 0;
    j     = 7;
    dtemp = 0;
    for (;;) {
        if (i == 0) {
            if (len == 0) {
                if (rlen >= limitlen) {
                   rlen = 0;
                } else {
                   *dptr = dtemp;
                   rlen++;
                }
                break;
            }
            i = 8;
            stemp = *sptr++;
            len--;
        }
        if (j == 0) {
            if (rlen >= limitlen) {
                rlen = 0;
                break;
            }
            j = 7;
            *dptr++ = dtemp;
            dtemp   = 0;
            rlen++;
        }
        if (stemp & s_mask[i - 1]) dtemp |= s_mask[j - 1];
        i--;
        j--;
    }
    return rlen;
}

INT16U bal_Code7_To_Code8(INT8U *dptr, INT8U *sptr, INT16U len)
{
    INT8U  i, j;
    INT8U stemp = 0;
    INT8U dtemp = 0;
    INT16U rlen;
    
    rlen  = 0;
    i     = 0;
    j     = 8;
    
    for (;;) {
        if (j == 0) {
            j = 8;
            *dptr++ = dtemp;
            rlen++;
            dtemp = 0;
            if (len == 0) break;
        }
        if (i == 0) {
            if (len == 0) {
                rlen = 0;
                break;
            }
            stemp = *sptr++;
            i = 7;
            len--;
        }
        if (stemp & s_mask[i - 1]) dtemp |= s_mask[j - 1];
        i--;
        j--;
    }
    return rlen;    
}
//#pragma O1

INT8U bal_BitValue(INT8U Data,INT8U Index)
{
   if((Data & s_mask[Index])==0) return 0;
   return 1;
}

BOOLEAN bal_IsZero(INT8U *Ptr,INT8U Len)
{
   while(Len--)
     {
      if(*Ptr==0) Ptr++;
      else return FALSE;
     }
   return TRUE;
}

//#if EN_OPEN_PROTOCOL > 0
void bal_MulGpsData(INT8U *result, INT8U *data, INT8U hdata, INT8U ldata)
{
    INT16U temp;
    
    temp = data[3] * ldata;
    result[3]  = temp % 100;
    result[2]  = temp / 100;
    
    temp = data[2] * ldata;
    result[2] += temp % 100;
    result[1]  = temp / 100;
    
    temp = data[1] * ldata;
    result[1] += temp % 100;
    result[0]  = temp / 100;
    
    temp = result[2];
    result[2]  = temp % 100;
    result[1] += temp / 100;
    
    temp = result[1];
    result[1]  = temp % 100;
    result[0] += temp / 100;
    
    temp = data[3] * hdata;
    result[2] += temp % 100;
    result[1] += temp / 100;
    
    temp = data[2] * hdata;
    result[1] += temp % 100;
    result[0] += temp / 100;
    
    temp = data[1] * hdata;
    result[0] += temp % 100;
    
    temp = result[3];
    result[3]  = temp % 100;
    result[2] += temp / 100;
    
    temp = result[2];
    result[2]  = temp % 100;
    result[1] += temp / 100;
    
    temp = result[1] + result[0] * 100;
    result[1] = temp % 60;
    result[0] = temp / 60;
    result[3] = result[2];
    result[2] = result[1];
    result[1] = result[0];
     
}


void bal_ResumeHighBit(INT8U *data, INT8U mask)
{
    INT8U temp;
    
    temp = *data;
    if (mask) {
        temp |= 0x80;
    }
    if (temp == 0xff) temp = 0x00;
    if (temp == 0xfe) temp = 0x80;
    *data = temp;    
}
//#endif

//#pragma O0
INT8U bal_GetDigitalString(INT8U *dptr, INT8U *sptr, INT16U maxlen, INT8U flagchar, INT8U numflag)//061116
{
    BOOLEAN find = FALSE;
    INT8U *bptr = NULL;
    INT8U *eptr = NULL;
    INT8U rlen;
        
    eptr = 0;
    rlen = 0;
    
    for(;;) {
        if (maxlen == 0) {
            find = FALSE;
            break;
        }
        if (*sptr == flagchar) {
            numflag--;
            if (numflag == 0) {
                eptr = sptr;
                break;
            }
        }
        if (*sptr >= '0' && *sptr <= '9') {
            if (!find) {
                find = TRUE;
                bptr = sptr;
            }
        } else {
            find = FALSE;
        }
        sptr++;
        maxlen--;
    }
    if (eptr == 0 || eptr < bptr) return 0;
    if (find) {
        for(;;) {
            *dptr++ = *bptr++;
            rlen++;
            if (bptr == eptr) break;
        }
        return rlen;
    } else {
        return 0;
    }
}

BOOLEAN bal_SearchGBCode(INT8U *ptr, INT16U len)
{
    for (; len > 0; len--) {
        if (*ptr++ >= 0x80) return true;
    }
    return false;
}

INT16U bal_TestUnicodeLen(INT8U *ptr, INT16U len)
{
    INT16U rlen;
    
    if (!bal_SearchGBCode(ptr, len)) {                                  /* 缓冲区中不存在gb码 */
        return len;
    }
    
    rlen = 0;
    for (;;) {
        if (len == 0) break;
        rlen += 2;
        len--;
        if (len == 0) break;
        if (*ptr++ >= 0xa0) {
            ptr++;
            len--;
        }
    }
    return rlen;
}

/*******************************************************************
** 函数名:     bal_ConvertIpStringToHex
** 函数描述:   解析IP地址,如ip_string为：11.12.13.14，则转换后的ip_long为0x0B0C0D0E
** 参数:       [out] ip_long:  ip地址，依次从从高位到低位
**             [out] sbits:    子网位数
**             [in]  ip_string:ip字符串，以'\0'为结束符，如“11.12.13.14”
** 返回:       转换成功返回0，失败返回失败描述符
********************************************************************/
char *bal_ConvertIpStringToHex(INT32U *ip_long, INT8U *sbits, char *ip_string)
{
    char *cp;
    INT8U dots = 0;
    INT32U number;
    LONG_UNION retval;
    char *toobig = "each number must be less than 255";

    cp = ip_string;
    while (*cp) {
        if (*cp > '9' || *cp < '.' || *cp == '/') {
            return("all chars must be digits (0-9) or dots (.)");
        }
        if (*cp == '.') {
            dots++;
        }
        cp++;
    }

    if (dots != 3 ) {
        return("string must contain 3 dots (.)");
    }

    cp = ip_string;                                                             /* 第一字节 */
    number = atoi(cp);
    if (number > 255) {
        return(toobig);
    }
    retval.bytes.byte1 = (INT8U)number;

    while (*cp != '.') {                                                       /* 第二字节 */
        cp++;
    }
    cp++;
    number = atoi(cp);
    if (number > 255) {
        return(toobig);
    }
    retval.bytes.byte2 = (INT8U)number;
    
    while (*cp != '.') {                                                       /* 第三字节 */
        cp++;
    }
    cp++;
    number = atoi(cp);
    if (number > 255) {
        return(toobig);
    }
    retval.bytes.byte3 = (INT8U)number;
    
    while (*cp != '.') {                                                       /* 第四字节 */
        cp++;
    }
    cp++;
    number = atoi(cp);
    if (number > 255) {
        return(toobig);
    }
    retval.bytes.byte4 = (INT8U)number;

    if (retval.bytes.byte1 < 128) *sbits = 8;
    else if(retval.bytes.byte1 < 192) *sbits = 16;
    else *sbits = 24;

    *ip_long = retval.ulong;
    return (NULL);
}

// 唐海勇添加. 函数作用说明待补充
INT16U 	bal_ByteToDecToAscii(INT8U *dptr,INT8U *sptr, INT16U len)
{
    INT16U i;
    INT16U asciilen;

    asciilen = 0;

    for(i=0;i<len;i++) {
      *dptr++ = bal_HexToChar(*sptr / 100);
      *dptr++ = bal_HexToChar((*sptr % 100 ) / 10);
      *dptr++ = bal_HexToChar(*sptr++ % 10 );
      asciilen += 3;
    }

    return asciilen;

}

// 唐海勇添加. 函数作用说明待补充
INT16U bal_AsciiToDecToByte(INT8U *dptr, INT8U *sptr, INT16U len)
{
   INT8U  byte[3];
   INT16U i;
   INT16U datalen;

   datalen = 0;

   if ( len % 3) return 0;

    len /= 3;

   for( i = 0; i< len ; i++) {
      byte[0] = bal_CharToHex(*sptr++);
      byte[1] = bal_CharToHex(*sptr++);
      byte[2] = bal_CharToHex(*sptr++);

      *dptr++ = byte[0] * 100 + byte[1] * 10 + byte[2];
      datalen++;
   }
    
   return datalen;    
}

INT8U bal_HexToSemiBCD(INT8U sdata)
{
	INT8U stemp,dtemp,bcddata;

	stemp   = sdata%100;
	dtemp   = stemp/10;
	bcddata = dtemp<<4;
	dtemp   = stemp%10;
	bcddata += dtemp;
	return bcddata;
}

INT8U bal_AsciiToBcd(INT8U *dptr, INT8U* sptr, INT8U len)
{
   INT8U retlen, i;

   if (0 == len) return 0;   
    
    retlen = 0;
    
    if (len % 2) {
        *dptr++ = bal_CharToHex(*sptr++);
        len--;
        retlen++;
    }
    
    for (i = 1; i <= len; i++) {
       if (0 != (i % 2)) {
           *dptr = (bal_CharToHex(*sptr++) << 4) & 0xf0;
       } else {
           *dptr += bal_CharToHex(*sptr++);
 	       dptr++;
 	       retlen++;
       }
   }
   return retlen;
}

INT8U bal_BcdToHex_Byte(INT8U bcd)
{
    INT8U temp;
    
    temp = (bcd >> 4) * 10 + (bcd & 0x0f);
    
    return temp;
}

void bal_BcdToHex(INT8U *dptr, INT8U *sptr, INT16U len)
{
    INT16U i;
    
    for (i = 0; i < len; i++) {
        *dptr++ = bal_BcdToHex_Byte(*sptr++);
    }
}

INT16U bal_HexToDec(INT8U *dptr, INT8U *sptr, INT16U len)
{
    INT16U i;
    INT8U  stemp;
    
    for (i = 1; i <= len; i++) {
        stemp = *sptr++;
        *dptr++ = bal_HexToChar(stemp / 10);
        *dptr++ = bal_HexToChar(stemp % 10);
    }
    return (2 * len);
}

/*******************************************************************
** 函数名:     bal_GetTimeStamp
** 函数描述:   根据输入的格林尼志日期和时间, 计算对应的秒
** 参数:       [in]  date:     输入格林尼志日期
**             [in]  time:     输入格林尼志时间      
** 返回:       GPS秒数, 自2000年1月1日零时起算
********************************************************************/
INT32U bal_GetTimeStamp(DATE_T *date, TIME_T *time)
{
    INT8U  i;
    INT16U days, year;

    year = date->year;
    if (year == 0) days = 0; //2000年
    else days = year*365 + ((year - 1)/4 + 1);

    for (i = 1; i < date->month; i++) {
        days += s_month_day[i - 1];
    }

    if ((date->month > 2) && (date->year % 4) == 0) {
        days++;
    }
    days += date->day - 1;
    return (days * (24*3600) + time->hour * 3600 + time->minute * 60 + time->second);
}

/*******************************************************************
** 函数名:     bal_GetDateSubDays
** 函数描述:   根据某天日期后退day天后得到日期
** 参数:       [out] odate  :输出日期
**             [in]  idate  :输入日期
**             [in]  day    :往后退的天数
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN bal_GetDateSubDays(DATE_T *date2, DATE_T *date1,INT16U day)
{
    INT32U day1;
    INT8U year,month,i;
    INT16U mode;
    INT8U  cday;

    day1 = 0;
    if (date1->year >= 1) {
        year = date1->year;
        day1 = year * 365 + 1 + (year-1)/4 ;
    }
    day1 += date1->day;
    for (i = 1; i < date1->month; i++) {
        cday = s_month_day[i-1];
        if ((i==2) && (0 == (date1->year%4))) {
            cday = 29;
        }
        day1 += cday;
    }
    if (day1 < day) {
        return FALSE;
    }
    day1 -= day;
    year = day1 / 366;
    day1 = day1 % 366;
    if(0 != year) { 
        day1 = day1 + year - 1 - ((year-1) / 4);
    }
    if(0 == (year%4)) {
        mode = 366;
    } else {
        mode = 365;
    }
    if (day1 > mode){
        year++;
        day1 -= mode;
    }
    month = 1;
    for (i = 1; i <= 12; i++) {
        cday = s_month_day[i-1];
        if (i == 2) {
            if(0 == (year % 4)) {
                cday = 29;
            } else {
                cday = 28;
            }
        }
        if (day1 <= cday) break;  
        day1 -= cday;
        month++;
    }
    if ((0 == day1) && (year >= 1)) {
        year--;
        month = 12;
        day1 = 31;
    }
    date2->year = year;
    date2->month = month;
    date2->day = day1; 
    return TRUE;
}


/*******************************************************************
**  函数名  :  bal_ConvertIpHexToString
**  函数描述:  转换INT32U型ip为字符串,如0x11223344，则转换后为"17.34.51.68"
**  参数:      [in]  ip:     INT32U型ip
**             [out] dptr:   目的指针地址
**             [in]  maxlen: 目的缓存长度，不小于16
**  返回参数:  成功返回转换后的长度(包含结束符)，失败返回0
********************************************************************/
INT8U bal_ConvertIpHexToString(INT32U ip, char *dptr, INT8U maxlen)
{
    INT8U i, ch, temp;
    
    if (maxlen < 16) {
        return 0;
    }
    
    YX_MEMCPY(dptr, maxlen, "000.000.000.000", 16);
    
    for (i = 0; i < 4; i++) {
        ch = ip >> ((4 - i - 1) * 8);
        temp = ch / 100;
        dptr[i * 4] = temp + 0x30;
        temp = (ch / 10) % 10;
        dptr[i * 4 + 1] = temp + 0x30;
        temp = ch % 10;
        dptr[i * 4 + 2] = temp + 0x30;
    }
    return 16;
}

/*******************************************************************
**  函数名  :   bal_DlyNumMS
**  函数描述:  延时若干毫秒
**  参数:      [in]  num: 延时的毫秒数
**  返回参数:  无
********************************************************************/
void bal_DlyNumMS(INT32U num)
{
    PORT_DlyNumMS(num);
}

// 将长整型数按大端模式转换为4个字节，即高位字节在低地址
void bal_longtochar(INT8U *dest, INT32U src)
{
    dest[0] = (INT8U)(src >> 24);
    dest[1] = (INT8U)(src >> 16);
    dest[2] = (INT8U)(src >> 8);
    dest[3] = (INT8U)(src);
}
// 将4个字节按大端模式转换为长整型,即高位字节在低地址
INT32U bal_chartolong(INT8U *src)
{
    INT32U ret = 0;

    ret = (INT32U)(src[0]) << 24;
    ret |= (INT32U)(src[1]) << 16;
    ret |= (INT32U)(src[2]) << 8;
    ret |= (INT32U)(src[3]);

    return ret;
}

// 将短整型数按大端模式转换为2个字节，即高位字节在低地址
void bal_shorttochar(INT8U *dest, INT16U src)
{
	dest[0] = (INT8U) (src >> 8);
	dest[1] = (INT8U) (src);
}

// 将2个字节按大端模式转换为短整型,即高位字节在低地址
INT16U bal_chartoshort(INT8U *src)
{
    INT16U ret = 0;
    ret = (INT16U) ((INT16U) (src[0]) << 8);
    ret |= (INT16U) (src[1]);
    return ret;
}

/*******************************************************************
** 函数名:      bal_DW_BigToLit
** 函数描述:    将大端存储的4字节数据转换小端模式存储
** 参数:        [in]  sptr:             源缓冲区
** 返回:        双字值
********************************************************************/
INT8U bal_DW_BigToLit(INT8U *sptr, INT8U *dptr)
{
    dptr[0] = sptr[3];
    dptr[1] = sptr[2];
    dptr[2] = sptr[1];
    dptr[3] = sptr[0];
    return 4;
}

/*******************************************************************
** 函数名:      bal_HW_BigToLit
** 函数描述:    将大端存储的2字节数据转换为小端模式存储
** 参数:        [in]  sptr:             源缓冲区
** 返回:        双字值
********************************************************************/
INT8U bal_HW_BigToLit(INT8U *sptr, INT8U *dptr)
{
    dptr[0] = sptr[1];
    dptr[1] = sptr[0];
    return 2;

}
/**************************************************************************************************
**  函数名称:  CutFromRight
**  功能描述:  将指针指向的长度为len的字符串从后面裁掉符合dat的字符。
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
INT8U bal_CutFromRight(INT8U* ptr, INT8U dat, INT8U len)
{
    while (len > 0) {
        len--;
        if (ptr[len] != dat) {
            return len + 1;
        }
    }
    return 0xff;
}

/**************************************************************************************************
**  函数名称:  CutFromLeft
**  功能描述:  返回指向的长度为len的字符串符合dat的字符首次出现的位置偏移量。
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
INT8U bal_CutFromLeft(INT8U* ptr, INT8U dat, INT8U datalen)
{
    INT8U len;
    
    for (len = 0; len < datalen; len++) {
        if (ptr[len] == dat) {
            return len + 1;
        }
    }
    return 0xff;
}




