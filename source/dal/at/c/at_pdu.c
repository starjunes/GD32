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

/*
********************************************************************************
* 定义模块配置参数
********************************************************************************
*/
#define EN_CHINESE           0


/*******************************************************************
** 函数名:     ParseUserData
** 函数描述:   解析PDU短信内容数据
** 参数:       [out] sm:      解析后的短信内容
**             [in]  orgdata: 待解析PDU短信内容数据
** 返回:       成功返回TRUE,失败返回FALSE
********************************************************************/
static BOOLEAN ParseUserData(SM_T *sm, INT8U *orgdata)
{
    INT16U temp;
    INT8U  len, udhl, udhl_7, fillbit, leftbit;

    if ((sm->pdutype & 0x40) != 0) {
        udhl = orgdata[1] + 1;
    } else {
        udhl = 0;
    }
    len = orgdata[0];
    if (udhl > len) {
        return FALSE;
    }
    
    switch (sm->dcs & 0x0c)
    {
    case SM_DCS_BIT7:
        if (len > 160) {
            return FALSE;
        }
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

        sm->udlen = len - udhl_7;
        YX_OctetToBit7(sm->ud, &orgdata[udhl + 1], temp - udhl, leftbit);
        return TRUE;
    case SM_DCS_UCS2:
#if EN_CHINESE > 0
        if (len > 140) {
            return FALSE;
        }
        sm->udlen = SHELL_UniToGB(sm->ud, &orgdata[udhl + 1], len - udhl);
        return TRUE;
#else
        return FALSE;
#endif
    default:
        if (len > 140) {
            return FALSE;
        }
        sm->udlen = len - udhl;
        memcpy(sm->ud, &orgdata[udhl + 1], len - udhl);
        return TRUE;
    }
}

/*******************************************************************
** 函数名:     AT_SMS_ParsePduData
** 函数描述:   解析PDU数据格式短信
** 参数:       [out] sm:      解析后的短信内容
**             [in]  orgdata: 待解析PDU数据
**             [in]  orgdata: 待解析PDU数据长度
**             [in]  pdulen:  实际PDU数据长度
** 返回:       成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN AT_SMS_ParsePduData(SM_T *sm, INT8U *orgdata, INT16U orglen, INT16U pdulen)
{
    INT8U templen;
    INT8U temp, curpos;
    
    orglen = YX_AsciiToHex(orgdata, orglen, orgdata, orglen - 2);              /* 2 = LF, CR */
    if (orglen == 0) {
        return FALSE;
    }
    
    if ((pdulen + orgdata[0] + 1) != orglen) {
        return FALSE;
    }
    
    if (orgdata[0] >= 11) {
        return FALSE;
    }
    
    if (orgdata[0] == 0) {
        sm->scalen = 0;
    } else {
        if (orgdata[1] == 0x91) {
            templen       = 1;
            sm->sca[0] = '+';
        } else {
            templen       = 0;
        }
        sm->scalen = templen + YX_SemiOctetToAscii(&sm->sca[templen], &orgdata[2], orgdata[0] - 1);
    }
    curpos = orgdata[0] + 1;
    sm->pdutype = orgdata[curpos++];
    
    temp = orgdata[curpos];
    if (temp >= 22) {
        return FALSE;
    }
    if (temp % 2) {
        temp++;
    }
    temp /= 2;
    if (temp == 0) {
        sm->oalen = 0;
    } else {
        if (orgdata[curpos + 1] == 0x91) {
            templen      = 1;
            sm->oa[0] = '+';
        } else {
            templen      = 0;
        }
        sm->oalen = templen + YX_SemiOctetToAscii(&sm->oa[templen], &orgdata[curpos + 2], temp);
    }
    curpos += (temp + 2);
    
    sm->pid = orgdata[curpos++];
    sm->dcs = orgdata[curpos++];
    
    YX_SemiOctetToHex(&sm->date.year, &orgdata[curpos], 6);                    /* 解析短信时间 */
    YX_SemiOctetToHex(&sm->timezone, &orgdata[curpos + 6], 1);                 /* 解析短信时间戳 */
    curpos += 7;
    
    return ParseUserData(sm, &orgdata[curpos]);
}

/*******************************************************************
** 函数名:     AT_SMS_AssemblePduData
** 函数描述:   按照pdu格式要求对待发送的短消息进行组帧
** 参数:       [in]  dcs:     短消息编码格式
**             [in]  telptr:  接收方手机号码
**             [in]  tellen:  接收方手机号码长度
**             [in]  dataptr: 短消息内容缓冲区
**             [in]  datalen: 短消息内容长度
**             [out] dptr:    返回存放组帧后的pdu格式数据的缓冲区
**             [in]  dlen:    缓冲区最大长度
** 返回:       成功返回组帧后长度,失败返回0
********************************************************************/
INT16U AT_SMS_AssemblePduData(INT8U dcs, INT8U *telptr, INT8U tellen, INT8U *dataptr, INT32U datalen, INT8U *dptr, INT32U dlen)
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
    
    YX_WriteBYTE_Strm(&wstrm, 0x0);                      /* write sca */
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
    
    YX_WriteBYTE_Strm(&wstrm, 0x0);                      /* write pid */
    YX_WriteBYTE_Strm(&wstrm, dcs);                      /* write dcs */
    YX_WriteBYTE_Strm(&wstrm, 143);                      /* write VP = 12 hours */
    tmp = YX_GetStrmPtr(&wstrm);                         /* record current position */
    YX_WriteBYTE_Strm(&wstrm, datalen);                  /* write udlen */
    
    switch (dcs)
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
    dptr[len++] = 0x1A;                                                        /* write CTRL+Z */
    
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
