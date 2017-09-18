/********************************************************************************
**
** 文件名:     at_pdu.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现短信编码转换
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2011/01/15 | 叶德焰 |  修改接口函数
*********************************************************************************/
#include "yx_include.h"
#include "yx_misc.h"
#include "yx_stream.h"
#include "at_pdu.h"
//#include "shell_reg_uni2gb.h"
//#include "shell_reg_gb2uni.h"


#if 0
static BOOLEAN ParseUserData(SM_T *desSM, INT8U *orgdata)
{
    INT16U temp;
    INT8U  len, udhl, udhl_7, fillbit, leftbit;

    if (desSM->PDUtype & 0x40) {
        udhl = orgdata[1] + 1;
    } else {
        udhl = 0;
    }
    len = orgdata[0];
    if (udhl > len) return FALSE;
    
    switch (desSM->DCS & 0x0c)
    {
        case SM_DCS_BIT7:
            if (len > 160) return FALSE;
            temp = len * 7;
            if (temp % 8) {
                temp /= 8;
                temp++;
            } else {
                temp /= 8;
            }
            
            fillbit = 7 - (udhl * 8) % 7;
            if (fillbit != 7) {
                leftbit = 8 - fillbit;
            } else {
                leftbit = 8;
            }
            udhl_7 = (udhl * 8) / 7;
            if (leftbit != 8) udhl_7++;

            desSM->UDL = len - udhl_7;
            YX_OctetToBit7(desSM->UD, &orgdata[udhl + 1], temp - udhl, leftbit);
            return TRUE;
        case SM_DCS_UCS2:
            if (len > 140) return FALSE;
            desSM->UDL = SHELL_UniToGB(desSM->UD, &orgdata[udhl + 1], len - udhl);
            return TRUE;
        default:
            if (len > 140) return FALSE;
            desSM->UDL = len - udhl;
            memcpy(desSM->UD, &orgdata[udhl + 1], len - udhl);
            return TRUE;
    }
}
#endif

BOOLEAN ParsePDUData(SM_T *desSM, INT8U *orgdata, INT16U orglen, INT16U pdulen)
{
#if 0
    INT8U templen;
    INT8U temp, curpos;
    
    orglen = YX_AsciiToHex(orgdata, orgdata, orglen - 2);          /* 2 = LF, CR */
    if (orglen == 0) return FALSE;
    if ((pdulen + orgdata[0] + 1) != orglen) return FALSE;
    
    if (orgdata[0] >= 11) return FALSE;
    if (orgdata[0] == 0) {
        desSM->SCAL = 0;
    } else {
        if (orgdata[1] == 0x91) {
            templen       = 1;
            desSM->SCA[0] = '+';
        } else {
            templen       = 0;
        }
        desSM->SCAL = templen + YX_SemiOctetToAscii(&desSM->SCA[templen], &orgdata[2], orgdata[0] - 1);
    }
    curpos = orgdata[0] + 1;
    desSM->PDUtype = orgdata[curpos++];
    
    temp = orgdata[curpos];
    if (temp >= 22) return FALSE;
    if (temp % 2) temp++;
    temp /= 2;
    if (temp == 0) {
        desSM->OAL = 0;
    } else {
        if (orgdata[curpos + 1] == 0x91) {
            templen      = 1;
            desSM->OA[0] = '+';
        } else {
            templen      = 0;
        }
        desSM->OAL = templen + YX_SemiOctetToAscii(&desSM->OA[templen], &orgdata[curpos + 2], temp);
    }
    curpos += (temp + 2);
    
    desSM->PID = orgdata[curpos++];
    desSM->DCS = orgdata[curpos++];
    
    YX_SemiOctetToAscii(desSM->SCTS, &orgdata[curpos], 6);
    YX_SemiOctetToHex(&desSM->timezone, &orgdata[curpos + 6], 1);
    curpos += 7;
    
    return ParseUserData(desSM, &orgdata[curpos]);
#endif

    return 0;
}


INT16U AssemblePDUData(INT8U DCS, INT8U *telptr, INT8U tellen, INT8U *dataptr, INT32U datalen, INT8U *dptr, INT32U dlen)
{
#if 0
    INT8U *tmpptr;
    INT32U len, tmplen;
    INT8U *tmp;
    STREAM_T wstrm;
    
    tmplen = datalen * 2 + 50;
    tmpptr = YX_DYM_Alloc(datalen * 2 + 50);
    if (tmpptr == 0) {
        return 0;
    }
    
    YX_InitStrm(&wstrm, tmpptr, tmplen);                 /* initialize stream */
    
    YX_WriteBYTE_Strm(&wstrm, 0x0);                      /* write SCA */
    YX_WriteBYTE_Strm(&wstrm, 0x11);                     /* write PDU-type */
    YX_WriteBYTE_Strm(&wstrm, 0x0);                      /* write MR */
    
    if (tellen > 22) {
        YX_DYM_Free(tmpptr);
        return 0;                                          /* write DA */
    }
    
    if (tellen == 0) {
        YX_WriteBYTE_Strm(&wstrm, 0x0);
        YX_WriteBYTE_Strm(&wstrm, 0x81);                 /* national number */
    } else {
        if (*telptr != '+') {
            YX_WriteBYTE_Strm(&wstrm, tellen);
            YX_WriteBYTE_Strm(&wstrm, 0x81);             /* national number */
            len = YX_AsciiToSemiOctet(YX_GetStrmPtr(&wstrm), telptr, tellen);
            YX_MovStrmPtr(&wstrm, len);
        } else {
            YX_WriteBYTE_Strm(&wstrm, tellen - 1);
            YX_WriteBYTE_Strm(&wstrm, 0x91);             /* international number */
            len = YX_AsciiToSemiOctet(YX_GetStrmPtr(&wstrm), telptr + 1, tellen - 1);
            YX_MovStrmPtr(&wstrm, len);
        }
    }
    
    YX_WriteBYTE_Strm(&wstrm, 0x0);                      /* write PID */
    YX_WriteBYTE_Strm(&wstrm, DCS);                      /* write DCS */
    YX_WriteBYTE_Strm(&wstrm, 143);                      /* write VP = 12 hours */
    tmp = YX_GetStrmPtr(&wstrm);                         /* record current position */
    YX_WriteBYTE_Strm(&wstrm, datalen);                  /* write UDL */
    
    switch (DCS)
    {
    /* default alphabet (7 bit data coding in the user data) */
    case SM_DCS_BIT7:
        if (datalen > 160) {
            YX_DYM_Free(tmpptr);
            return 0;
        }
            
        len = YX_Bit7ToOctet(YX_GetStrmPtr(&wstrm), dataptr, datalen);
        YX_MovStrmPtr(&wstrm, len);
        break;
    case SM_DCS_UCS2:
        if ((*tmp = len = SHELL_GBToUni(YX_GetStrmPtr(&wstrm), 140, dataptr, datalen)) == 0) {
            YX_DYM_Free(tmpptr);
            return 0;
        }
        YX_MovStrmPtr(&wstrm, len);
        break;
    default:
        if (datalen > 140) {
            YX_DYM_Free(tmpptr);
            return 0;
        }
            
        YX_WriteDATA_Strm(&wstrm, dataptr, datalen);
        break;
    }
    
    if (YX_GetStrmLen(&wstrm) * 2 >= dlen) {
        YX_DYM_Free(tmpptr);
        return 0;
    }
    
    len = YX_HexToAscii(dptr, tmpptr, YX_GetStrmLen(&wstrm));
    dptr[len++] = 0x1A;                         /* write CTRL+Z */
    
    YX_DYM_Free(tmpptr);
	return len;
#endif
    
    return 0;
}
#if 0
static char const GSMCODE[] = { '@', ' ', '$', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0x0A,' ', ' ', 0x0D,' ', ' ',
                               '_', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
                               ' ', '!', '"', '#', ' ', '%', '&', '\'','(', ')', '*', '+', ',', '-', '.', '/',
                               '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?',
                               ' ', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
                               'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', ' ', ' ', ' ', ' ', ' ',
                               '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
                               'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', ' ', ' ', ' ', ' ', ' '
                             };

INT8U GsmCodeToASCII(INT8U incode)
{
    return GSMCODE[incode & 0x7f];
}

INT8U ASCIIToGsmCode(INT8U incode)
{
    INT8U i;
    
    if (incode == ' ') return ' ';
    for (i = 0; i < sizeof(GSMCODE); i++) {
        if (incode == GSMCODE[i]) return i;
    }
    return ' ';
}

#endif
