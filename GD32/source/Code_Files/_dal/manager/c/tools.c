/**************************************************************************************************
**                                                                                               **
**  文件名称:  Tools.C                                                                           **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2010                                    **
**  创建信息:  LEON -- 2010年12月1日                                                             **
**  文件描述:  工具函数集                                                                        **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/
#include "tools.h"

/*
*******************************************************************************
* Function Name  : u_chksum_1
* Description    : This function used to calculate chechsum with one byte.
* Arguments      : [in] pointer to data buffer
*                : [in] buffer length
* Return         : checksum
*******************************************************************************
*/
INT8U u_chksum_1(INT8U *ptr, INT16U len)
{
    INT16U i;
    HWORD_UNION chksum;
    
    chksum.hword = 0;
    for (i = 1; i <= len; i++) {
        chksum.hword += *ptr++;
        if (chksum.bytes.high > 0) {
            chksum.bytes.low += chksum.bytes.high;
            chksum.bytes.high = 0;
        }
    }
    chksum.bytes.low += 1;
    return chksum.bytes.low;
}

/*
*******************************************************************************
* Function Name  : u_chksum_2
* Description    : This function used to calculate chechsum with two byte.
* Arguments      : [in] pointer to data buffer
*                : [in] buffer length
* Return         : checksum
*******************************************************************************
*/
INT16U  u_chksum_2(INT8U *ptr, INT16U len)
{
    INT8U  temp;
    INT16U chksum;
    
    temp    = u_chksum_1(ptr, len);
    chksum  = (INT16U)temp << 8;
    temp    = 0xff - temp;
    chksum |= temp;
    return chksum;
}

/*
*******************************************************************************
* Function Name  : Getchksum
* Description    : This function used to calculate chechsum 
* Arguments      : [in] pointer to data buffer
*                : [in] buffer length
* Return         : checksum
*******************************************************************************
*/
INT8U Getchksum(INT8U *dptr, INT16U len)
{
    INT16U result;
    
    result = 0;
    for (; len > 0; len--) {
        result = (INT16U)(result + *dptr++);
    }
    return (INT8U)result;
}

/*
*******************************************************************************
* Function Name  : Getchksum_n
* Description    : This function used to calculate chechsum 
* Arguments      : [in] pointer to data buffer
*                : [in] buffer length
* Return         : checksum
*******************************************************************************
*/
INT8U Getchksum_n(INT8U *dptr, INT16U len)
{
     return (~Getchksum(dptr, len));
}

/********************************************************************************************
**  函数名称:  Get_N_Chksum
**  功能描述:  获取反码校验和
**  输入参数:  
**  返回参数:  
********************************************************************************************/
INT8U Get_N_Chksum(INT8U *dptr, INT16U len)
{
    INT16U result;
    
    result = 0;
    
    for (; len > 0; len--) {
        result = (INT16U)(result + ~(*dptr++));
    }
    
    return (INT8U)result;
}

/********************************************************************************************
**  函数名称:  Get_X_Chksum
**  功能描述:  获取校验异或结果
**  输入参数:  
**  返回参数:  
********************************************************************************************/
INT8U Get_X_Chksum(INT8U *dptr, INT16U len)
{
    INT8U result;
    
    result = 0;
    
    for (; len > 0; len--) {
        result ^= *dptr++;
    }
    
    return result;
}

/*
*******************************************************************************
* Function Name  : FindProcEntry
* Description    : This function used to find func entry and execute
* Arguments      : [in] entry index
*                : [in] entry table
*                : [in] entry table size
* Return         : TRUE or FALSE
*******************************************************************************
*/
BOOLEAN FindProcEntry(INT16U index,  const FUNCENTRY_T *funcentry, INT16U num)
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

/*
*******************************************************************************
* Function Name  : NeedRoundLine
* Description    : This function used to round line
* Arguments      : [in] pointer to char string
*                : [in] char string length
* Return         : TRUE or FALSE
*******************************************************************************
*/
BOOLEAN NeedRoundLine(INT8U *charptr, INT8U charlen)
{
    INT8U i;
    INT8U *ptr;
     
    ptr = charptr;
    ptr += charlen - 1;
     
    if ((*ptr & 0x80) == 0) {
        return FALSE;
    }
     
    for (i=charlen; i>0; i--) {  
        if ((*ptr-- & 0x80) == 0) { 
            break;
        }
    }
     
    if((charlen - i) % 2) {  
        return TRUE;                       
    } else if ((i == charlen) && (charlen % 2)) {
        return TRUE;
    } else {
        return FALSE;
    }      
} 


#define  BASE_YEAR         9
#define  LEAP_ADDS         (4 - (BASE_YEAR % 4))

static const INT8U month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

/*
*******************************************************************************
* Function Name  : Seconds2Systime
* Description    : seconds turn to year.month.day.hour.minute.second
* Arguments      : [in] seconds
*                : [out] systime
* Return         : None
*******************************************************************************
*/
void Seconds2Systime(INT32U seconds, SYSTIME_T *systime) 
{
    INT32U days;
    INT16U leap_y_count;
     
    systime->time.second  = seconds % 60;
    seconds              /= 60;
    systime->time.minute  = seconds % 60;
    seconds              /= 60;
    systime->time.hour    = seconds % 24;
     
    days                  = seconds / 24;
    leap_y_count          = (days + 365) / 1461;
     
    if (((days + 366) % 1461) == 0) {
         systime->date.year  = BASE_YEAR + (days / 366);
         systime->date.month = 12;
         systime->date.day   = 31;
         return;
    }
     
    days  -= leap_y_count;
    systime->date.year = BASE_YEAR + (days / 365);
    days  %= 365;
    days   = 1 + days;
     
    if ((systime->date.year % 4) == 0) {
        if (days > 60) {
            --days;
        } else {
            if (days == 60) {
                 systime->date.month = 2;
                 systime->date.day   = 29;
                 return;
            }
        }
    }
     
    for (systime->date.month = 0; month[systime->date.month] < days; systime->date.month++) {
        days -= month[systime->date.month];
    }
     
    ++systime->date.month;
    systime->date.day = days;
}

/*
*******************************************************************************
* Function Name  : Systime2Seconds
* Description    : year.month.day.hour.minute.second turn to seconds  
* Arguments      : [in] systime
* Return         : seconds
*******************************************************************************
*/
#define xMINUTE          (60         )
#define xHOUR            (60*xMINUTE )
#define xDAY             (24*xHOUR   )
#define xYEAR            (365*xDAY   )

INT32U Systime2Seconds(SYSTIME_T *systime)
{
    static const INT32U month[12]= {
    /*01*/xDAY * (0),
    /*02*/xDAY * (31),
    /*03*/xDAY * (31 + 28),
    /*04*/xDAY * (31 + 28 + 31),
    /*05*/xDAY * (31 + 28 + 31 + 30),
    /*06*/xDAY * (31 + 28 + 31 + 30 + 31),
    /*07*/xDAY * (31 + 28 + 31 + 30 + 31 + 30),
    /*08*/xDAY * (31 + 28 + 31 + 30 + 31 + 30 + 31),
    /*09*/xDAY * (31 + 28 + 31 + 30 + 31 + 30 + 31 + 31),
    /*10*/xDAY * (31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30),
    /*11*/xDAY * (31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31),
    /*12*/xDAY * (31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30)
    };
    INT32U seconds;
    INT8U  year;
     
    year     = systime->date.year - BASE_YEAR;
    seconds  = xYEAR * year + xDAY * ((year + 1) / 4);
    seconds += month[systime->date.month - 1];
     
    if ((systime->date.month > 2) && (((year + LEAP_ADDS) % 4) == 0)) {
        seconds += xDAY;
    }
     
    seconds += xDAY * (systime->date.day - 1);
    seconds += xHOUR * systime->time.hour;
    seconds += xMINUTE * systime->time.minute;
    seconds += systime->time.second;
     
    return seconds;     
}

/*
*******************************************************************************
* Function Name  : BubberSort
* Description    : bubber sort  
* Arguments      : [in] pointer to data buffer
*                : [in] buffer size
* Return         : None
*******************************************************************************
*/
void BubberSort(INT16U *a, INT8U n)
{
    INT8U   i, j;
    INT32U  temp;
     
    for (i = 0; i < n - 1; i++) {
        for (j = 0; j < n - 1 - i; j++) {
            if (a[j] > a[j + 1]) {
                temp = a[j];
                a[j] = a[j + 1];
                a[j + 1] = temp;
            }
        }
    }
}

/*
*******************************************************************************
* Function Name  : UpperChar
* Description    : change char case from  lowercase to uppercase 
* Arguments      : [in] char
* Return         : char in uppercase
*******************************************************************************
*/
INT8U UpperChar(INT8U ch)
{
    if (ch >= 'a' && ch <= 'z') {
        ch = 'A' + (ch - 'a');
    }
    return ch;
}

/*
*******************************************************************************
* Function Name  : LowerChar
* Description    : change char case from uppercase to lowercase 
* Arguments      : [in] char
* Return         : char in lowercase
*******************************************************************************
*/
INT8U LowerChar(INT8U ch)
{
    if (ch >= 'A' && ch <= 'Z'){
        ch = 'a' + (ch - 'A');
    }
    return ch;
}

/*
*******************************************************************************
* Function Name  : CmpChar
* Description    : compare two chars whether match case or not  
* Arguments      : [in] match case or not
*                : [in] char 1
*                : [in] char 2
* Return         : cmp result
*******************************************************************************
*/
static INT8U CmpChar(BOOLEAN matchcase, INT8U ch1, INT8U ch2)
{
    if (matchcase != CASESENSITIVE1) {
        ch1 = UpperChar(ch1);
        ch2 = UpperChar(ch2);
    }
    if (ch1 > ch2) {
       return STR_GREAT;
    } else if (ch1 < ch2) {
       return STR_LESS;
    } else {
       return STR_EQUAL;
    }
}

/*
*******************************************************************************
* Function Name  : CmpString
* Description    : compare two char string  
* Arguments      : [in] string 1
*                : [in] string 2
*                : [in] string length
* Return         : cmp result
*******************************************************************************
*/
INT8U CmpString(INT8U *ptr1, INT8U *ptr2, INT16U len)
{
    INT8U result;
    
    for (;;) {
        if (len == 0) {
            return STR_EQUAL;
        }
        result = CmpChar(CASESENSITIVE1, *ptr1++, *ptr2++);
        if (result != STR_EQUAL) {
            return result;
        } else {
            len--;
        }
    }
}


/*
*******************************************************************************
* Function Name  : AcmpString
* Description    : compare two char string  
* Arguments      : [in] matchcase: if case sensitive
*                : [in] ptr1:string 1
*                : [in] ptr2:string 2
*                : [in] len1:string1 length
*                : [in] len2:string2 length
* Return         : cmp result
*******************************************************************************
*/
INT8U AcmpString(BOOLEAN matchcase, INT8U *ptr1, INT8U *ptr2, INT16U len1, INT16U len2)
{
    INT8S result;
    
    for (;;) {
        if (len1 == 0 && len2 == 0) {
            return STR_EQUAL;
        }
        if (len1 == 0) {
            return STR_LESS;
        }
        if (len2 == 0) {
            return STR_GREAT;
        }
        
        result = CmpChar(matchcase, *ptr1++, *ptr2++);
        if (result != STR_EQUAL) {
            return result;
        } else {
            len1--;
            len2--;
        }
    }
}


/*
*******************************************************************************
* Function Name  : Assemblebyrule
* Description    : assemble data stream with rule  
* Arguments      : [in] dptr : buffer after assembling
*                : [in] sptr : buffer before assembing
*                : [in] len  : length of the buffer which needs assembling
*                : [in] maxlen : the max length of buffer which assemled
*                : [in] rule : assembling rule 
* Return         : the length of buffer after assembling
*******************************************************************************
*/
INT16U Assemblebyrule(INT8U *dptr, INT8U *sptr, INT16U len, INT16U maxlen, const ASMRULE_T *rule)
{
    INT8U  temp;
    INT16U rlen;
    
    if (rule == 0) {
        return 0;
    }
    
    *dptr++ = rule->c_flags;
    rlen    = 1;
    for (; len > 0; len--) {
        temp = *sptr++;
        if (temp == rule->c_flags) {            /* c_flags    = c_convert0 + c_convert1 */
            if ((rlen + 2) > maxlen) {
                return 0;
            }
            *dptr++ = rule->c_convert0;
            *dptr++ = rule->c_convert1;
            rlen += 2;
        } else if (temp == rule->c_convert0) {  /* c_convert0 = c_convert0 + c_convert2 */
            if ((rlen + 2) > maxlen) {
                return 0;
            }
            *dptr++ = rule->c_convert0;
            *dptr++ = rule->c_convert2;
            rlen += 2;
        } else {
            if (rlen >= maxlen) {
                return 0;
            }
            *dptr++ = temp;
            rlen++;
        }
    }
    if (rlen >= maxlen) {
        return 0;
    }
    *dptr = rule->c_flags;
    rlen++;
    return rlen;
}

/*
*******************************************************************************
* Function Name  : Deassemblebyrule
* Description    : deassemble data stream with rule  
* Arguments      : [in] dptr : buffer after deassembling
*                : [in] sptr : buffer before deassembing
*                : [in] len  : length of the buffer which needs deassembling
*                : [in] maxlen : the max length of buffer which deassemled
*                : [in] rule : deassembling rule 
* Return         : the length of buffer after deassembling
*******************************************************************************
*/
INT16U Deassemblebyrule(INT8U *dptr, INT8U *sptr, INT16U len, INT16U maxlen, const ASMRULE_T *rule)
{
    INT16U rlen;
    INT8U  prechar, curchar, c_convert0;

    if (rule == 0) {
        return 0;
    }
    c_convert0 = rule->c_convert0;
    rlen = 0;
    prechar = (INT8U)(~c_convert0);
    for (; len > 0; len--) {
        curchar = *sptr++;
        if (len >= maxlen) {
            return 0;
        }
        if (prechar == c_convert0) {
            if (curchar == rule->c_convert1) {            /* c_convert0 + c_convert1 = c_flags */
                prechar = (INT8U)(~c_convert0);
                *dptr++ = rule->c_flags;
                rlen++;
            } else if (curchar == rule->c_convert2) {     /* c_convert0 + c_convert2 = c_convert0 */
                prechar = (INT8U)(~c_convert0);
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


/*
*******************************************************************************
* 函数名:     gps_getassemblelenbyrule
* 函数描述:   获取按规则转义编码后的数据长度
* 参数:       [in]  sptr:       待编码缓冲区
*             [in]  len:        待编码数据长度
*             [in]  rule:       规则
* 返回:       编码后的数据长度
*******************************************************************************
*/
INT16U Getassemblelenbyrule(INT8U *sptr, INT16U len, const ASMRULE_T *rule)
{
    INT8U  temp;
    INT16U rlen;
    
    if (rule == 0) {
        return 0;
    }
    
    rlen = 1;
    for (; len > 0; len--) {
        temp = *sptr++;
        if (temp == rule->c_flags || temp == rule->c_convert0) {    /* c_flags    = c_convert0 + c_convert1 */
            rlen += 2;
        } else {
            rlen++;
        }
    }
    return rlen + 1;
}

/**************************************************************************************************
**  函数名称:  HexToChar
**  功能描述:  将16进制格式数据转换为字符输出
**  输入参数:  
**  输出参数:  
**************************************************************************************************/
INT8U HexToChar(INT8U ch)
{
    INT8U temp;
    
    temp = ch & 0x0f;
    
    if (temp <= 0x09) {
        temp = '0' + temp;
    } else {
        temp = 'a' + temp - 0x0a;
    }
    return temp;
}

/**************************************************************************************************
**  函数名称:  AlgToChar
**  功能描述:  将10进制格式数据转换为字符输出
**  输入参数:  
**  输出参数:  
**************************************************************************************************/
INT8U AlgToChar(INT8U ch)
{
    INT8U temp;
    
    temp = ch & 0x0f;
    
    if (temp <= 0x09) {
        temp = '0' + temp;
    } else {
        temp = '0' + temp;
    }
    return temp;         
}

/*
*******************************************************************************
* Function Name  : Chartohex
* Description    : convert format from char to hex 
* Arguments      : [in] schar
* Return         : convert hex value
*******************************************************************************
*/
INT8U Chartohex(INT8U schar)
{
    if (schar >= '0' && schar <= '9') {
        return (schar - '0');
    } else if (schar >= 'A' && schar <= 'F') {
        return (schar - 'A' + 0x0A);
    } else if (schar >= 'a' && schar <= 'f') {
        return (schar - 'a' + 0x0A);
    }
    else return 0;
}


/*
*******************************************************************************
* Function Name  : Longtochar
* Description    : convert format from long to char 
* Arguments      : [in] dest
*                : [in] src
* Return         : None
*******************************************************************************
*/
void Longtochar(INT8U *dest, INT32U src)
{
    dest[0] = (INT8U)(src >> 24);
    dest[1] = (INT8U)(src >> 16);
    dest[2] = (INT8U)(src >> 8);
    dest[3] = (INT8U)(src);
}

/*
*******************************************************************************
* Function Name  : Longtochar
* Description    : convert format from char to long 
* Arguments      : [in] dest
*                : [in] src
* Return         : None
*******************************************************************************
*/
void Chartolong(INT32U *dest, INT8U *src)
{
    *dest  = (INT32U)(src[0]) << 24;
    *dest |= (INT32U)(src[1]) << 16;
    *dest |= (INT32U)(src[2]) << 8;
    *dest |= (INT32U)(src[3]);
}

/*
*******************************************************************************
* Function Name  : Shorttochar
* Description    : convert format from short to char 
* Arguments      : [in] dest
*                : [in] src
* Return         : None
*******************************************************************************
*/
void Shorttochar(INT8U *dest, INT16U src)
{
    dest[0] = (INT8U)(src >> 8);
    dest[1] = (INT8U)(src);
}

/*
*******************************************************************************
* Function Name  : Chartoshort
* Description    : convert format from char to short
* Arguments      : [in] dest
*                : [in] src
* Return         : None
*******************************************************************************
*/
void Chartoshort(INT16U *dest, INT8U *src)
{
    *dest  = (INT16U)((INT16U)(src[0]) << 8);
    *dest |= (INT16U)(src[1]);
}

/*
*******************************************************************************
* Function Name  : SearchDigitalString
* 函数描述:   获取数据缓冲区中第numflag个flagchar之前的十进制数字
* 参数:       [in]  ptr:        数据缓冲区
*             [in]  len:        数据缓冲区长度
*             [in]  flagchar:   标识字符
*             [in]  numflag:    标识字符序号, 注意: 从0开始编码
* 返回:       获取到的十进制数据结果; 如获取失败, 则返回0xffff
* 注意:       十进制数据不得超过65535
*******************************************************************************
*/
#pragma O0
INT16U SearchDigitalString(INT8U *ptr, INT16U maxlen, INT8U flagchar, INT8U numflag)
{
    BOOLEAN find;
    INT16U  result;
    INT8U *bptr, *eptr;
    
    find = FALSE;
    bptr = 0;
    eptr = 0;
    for (;;) {
        if (maxlen == 0) {
            find = FALSE;
            break;
        }
        if (*ptr == flagchar) {
            if (numflag == 0) {
                eptr = ptr;
                break;
            } else {
                numflag--;
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
            result = result * 10 + Chartohex(*bptr++);
            if (bptr == eptr) {
                break;
            }
        }
    } else {
        result = 0xffff;
    }
    return result;
}


/*
*******************************************************************************
* Function Name  : SearchKeyWordFromHead
* Description    : check if key word exist from head in data buffer 
* Arguments      : [in] matchcase:true:case sensitive/false: not case sensitive
*                : [in] ptr:data buffer pointer
*                : [in] len:data length
*                : [in] sptr:key word buffer pointer
* Return         : true:search key word success
*                  false: can not find key word
*******************************************************************************
*/
BOOLEAN SearchKeyWordFromHead(BOOLEAN matchcase, INT8U *ptr, INT16U len, char *sptr)
{
    INT8U ch1, ch2;

    if (*sptr == 0) {
        return false;
    }
    while(len > 0) {
        ch1 = *ptr++;
        ch2 = *sptr++;
        if (!matchcase) {
            ch1 = LowerChar(ch1);
            ch2 = LowerChar(ch2);
        }
        if (ch1 == ch2) {
            if (*sptr == 0) {
                return true;
            }
        } else {
            return false;
        }
        len--;
    }
    return false;
}

/**************************************************************************************************
*   函数名称:  Hex8UtoAlg16U
*   功能描述:  单字节的16进制转换为双字节的10进制形式表示(例如输入0xff，转换后将输出0x255)
*   输入参数:  INT8U Data
*   输出参数:  INT16U
**************************************************************************************************/
INT16U Hex8UtoAlg16U(INT8U Data)
{
	return ((Data / 100) * 256 + (Data % 100) / 10 * 16 + (Data % 10));
}

/**************************************************************************************************
*   函数名称:  Hex16UtoAlg32U
*   功能描述:  双字节的16进制转换为四字节的10进制形式表示(例如输入0xffff，转换后将输出0x65535)
*   输入参数:  INT16U Data
*   输出参数:  INT32U
**************************************************************************************************/
INT32U Hex16UtoAlg32U(INT16U Data)
{
	return ((Data / 10000) * 65536 + (Data % 10000) / 1000 * 4096 + 
	        (Data % 1000) / 100 * 256 + (Data % 100) / 10 * 16 + (Data % 10));
}

/**************************************************************************************************
*   函数名称:  Alg16UtoHex8U
*   功能描述:  双字节的10进制转换为单字节的16进制形式表示(例如输入0x255，转换后将输出0xff)
*   输入参数:  INT16U Data
*   输出参数:  INT8U
**************************************************************************************************/
INT8U Alg16UtoHex8U(INT16U Data)
{
	INT16U Temp;
	INT8U Result;

	if (Data >= 0x256) {
        return 0;                                     /* 输入数据不能大于0x256 */
	}
	Temp = ((Data / 256) * 100 + (Data % 256) / 16 * 10 + (Data % 16));
	Result = (INT8U)Temp;
	return Result;
}

/**************************************************************************************************
*   函数名称:  Alg32UtoHex16U
*   功能描述:  四字节的10进制转换为双字节的16进制形式表示(例如输入0x65535，转换后将输出0xffff)
*   输入参数:  INT32U Data
*   输出参数:  INT16U
**************************************************************************************************/
INT16U Alg32UtoHex16U(INT32U Data)
{
	INT32U Temp;
	INT16U Result;

	if (Data >= 0x65536) {
        return 0;                                   /*  输入数据不能大于0x65536 */
	}
	Temp = ((Data / 65536) * 10000 + (Data % 65536) / 4096 * 1000 + 
	        (Data % 4096) / 256 * 100 + (Data % 256) / 16 * 10 + (Data % 16));
	Result = (INT16U)Temp;
	return Result;
}





/**************************************************************************************************
**  函数名称:  IsInRange
**  功能描述:  检查字符是否在上下限范围内。
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
BOOLEAN IsInRange(INT8U checkedchar,INT8U uplimit,INT8U downlimit)//检验checkedchar是否在上限和下限的范围内
{
    if ((checkedchar >= uplimit) && (checkedchar <= downlimit)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/**************************************************************************************************
**  函数名称:  CutRedundantLen
**  功能描述:  取较短的长度
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
INT8U  CutRedundantLen(INT8U len,INT8U standardlen)
{
      if (len < standardlen) {
         return len;
      } else {
         return standardlen;
      }
}


/**************************************************************************************************
**  函数名称:  HexToAscii
**  功能描述:  把100以内的16进制数转化为ASCII码。返回转化后长度。
**  输入参数:  
**  返回参数:  dptr不得大于100
**************************************************************************************************/
INT8U HexToAscii(INT8U *dptr, INT8U *sptr, INT8U len)
{
    INT8U i;
    INT8U stemp;
    
    for (i = 1; i <= len; i++) {
        stemp = *sptr++;
        *dptr++ = HexToChar(stemp / 10);
        *dptr++ = HexToChar(stemp % 10);
        // *dptr++ = HexToChar(stemp >> 4);
        // *dptr++ = HexToChar(stemp & 0x0F);
    }
    return (2 * len);
}


/**************************************************************************************************
**  函数名称:  HexToAscii1
**  功能描述:  将64以内的数转化为可见的ASCII码。
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
INT8U HexToAscii1(INT8U *dptr, INT8U sdata)
{
    
    INT8U j;
    if (sdata < 0x0a) {
        *dptr = 0x30;
        *(dptr+1) = sdata + 0x30; 
        return 2;
    } else if (sdata >= 0x0a && sdata < 0x64) {
        HexToAscii(dptr, &sdata, 1);
        return 2;
    } else if (sdata >= 0x64) {
        for (j = 2; j > 0; j--) {
            *(dptr+j) = HexToChar(sdata % 10);
            if (sdata < 10) sdata = 0;
            else sdata /= 10;
        }
        *dptr = HexToChar(sdata % 10);
        return 3;
    }
    return 0;
}

/**************************************************************************************************
**  函数名称:  AsciiToHex
**  功能描述:  将可见ASCII码转化成HEX。
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
INT8U AsciiToHex(INT8U *dptr, INT8U *sptr, INT8U len)
{
    INT8U i;
    INT8U  dtemp, stemp;
    
    if (len % 2) {
        return 0;
    }
    len /= 2;
    for (i = 1; i <= len; i++) {
        stemp   = Chartohex(*sptr++);
        dtemp   = stemp * 10;
        *dptr++ = Chartohex(*sptr++) + dtemp;
    }
    return len;
}

/**************************************************************************************************
**  函数名称:  CutFromRight
**  功能描述:  将指针指向的长度为len的字符串从后面裁掉符合dat的字符。
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
INT8U CutFromRight(INT8U* ptr, INT8U dat, INT8U len)
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
INT8U CutFromLeft(INT8U* ptr, INT8U dat, INT8U datalen)
{
    INT8U len;
    
    for (len = 0; len < datalen; len++) {
        if (ptr[len] == dat) {
            return len + 1;
        }
    }
    return 0xff;
}

/*
*******************************************************************************
* 函数名:     DecToAscii
* 函数描述:   将hex数据转换成十进制表示的字符串
* 参数:       [out] dptr:      转换后的字符串缓冲区
*             [in]  hex_data:  待转换的hex数据
*             [in]  reflen:    转换结果参考; 
*                              如reflen==0, 则转换结果不受reflen控制
*                              如reflen!=0, 则当转换结果的字符串长度小于reflen时, 
*                              在字符串前补(reflen-实际长度)个0
* 返回:       转换的字符串长度(包括前置0)
* 例子:       将0x1EDB(即7899), 转换成"7899"
*******************************************************************************
*/
INT8U DecToAscii(INT8U *dptr, INT32U data1, INT8U reflen)
{
    INT8U i, len, temp;
    INT8U *tempptr;
    
    len     = 0;
    tempptr = dptr;
    for (;;) {
        *dptr++ = HexToChar(data1 % 10);
        len++;
        if (data1 < 10) break;
        else data1 /= 10;
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
INT32U AsciiToDec(INT8U *sptr, INT8U len)
{
    INT32U result;
    
    result = 0;
    for (; len > 0; len--) {
        result = result * 10 + Chartohex(*sptr++);
    }
    return result;
}



/**************************************************************************************************
**  函数名称:  isdatavalid
**  功能描述:  检查长度为LEN的数据不全为0或FF。
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
BOOLEAN isdatavalid(INT8U *ptr,INT8U len)
{
    INT8U i, j;
    if (len == 0) {
        return false;
    }
    j = 0;
    for (i = 0; i < len; i++) {
        if (*(ptr+i) == 0x00 || *(ptr+i) == 0xff) {
            j++;
        }
    }
    if (j == len) {
        return false;
    }
    return true;    
}
/******************* (C) COPYRIGHT 2009 XIAMEN YAXON.LTD *********END OF FILE******/


