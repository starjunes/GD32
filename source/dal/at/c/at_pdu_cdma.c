/********************************************************************************
**
** 文件名:     at_pdu_cdma.c
** 版权所有:   (c) 2014-2020 厦门雅迅网络股份有限公司
** 文件描述:   实现CDMA模块短消息在pdu模式下的解析和封装
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2015/04/03 | 叶德焰 |  创建该文件
**
*********************************************************************************/
#include "yx_include.h"
#include "yx_misc.h"
#include "yx_stream.h"
#include "at_pdu.h"
#include "at_pdu_cdma.h"



/*
********************************************************************************
* 定义模块局部变量
********************************************************************************
*/





/*******************************************************************
** 函数名:     PaserTimeStamp
** 函数描述:   解析短消息发送时间戳
** 参数:       [out] sm:                存放解析后的短消息
**             [in]  pmem:              待解析的数据缓冲区
**             [in]  lmem:              待解析的数据缓冲区大小
** 返回:       true:  解析成功
**             false: 解析失败
********************************************************************/
static BOOLEAN PaserTimeStamp(SM_T *sm, INT8U *pmem, INT8U lmem)
{
    if (lmem < 6) {
        return false;
    }
    sm->date.year   = (pmem[0] & 0x0f) + (pmem[0] >> 4) * 10;
    sm->date.month  = (pmem[1] & 0x0f) + (pmem[1] >> 4) * 10;
    sm->date.day    = (pmem[2] & 0x0f) + (pmem[2] >> 4) * 10;
    
    sm->time.hour   = (pmem[3] & 0x0f) + (pmem[3] >> 4) * 10;
    sm->time.minute = (pmem[4] & 0x0f) + (pmem[4] >> 4) * 10;
    sm->time.second = (pmem[5] & 0x0f) + (pmem[5] >> 4) * 10;
    return true;
}

/*******************************************************************
** 函数名:     ParseUserData
** 函数描述:   解析短消息内容
** 参数:       [out] sm:                存放解析后的短消息
**             [in]  pmem:              待解析的数据缓冲区
**             [in]  lmem:              待解析的数据缓冲区大小
** 返回:       true:  解析成功
**             false: 解析失败
********************************************************************/
static BOOLEAN ParseUserData(SM_T *sm, INT8U *pmem, INT8U lmem)
{
    INT8U dcs, smlen, left;
    INT16U i, pos;

    if (lmem < 2) {                                                             /* 解析短消息内容 */
        return false;
    }
    
    dcs   = pmem[0] >> 3;                                                       /* 解析编码格式 */
    smlen = (pmem[0] << 5) + (pmem[1] >> 3);                                    /* 解析短消息内容长度 */
    if (dcs == 0x02) {                                                          /* bit7编码 */
        BOOLEAN     long_sm = false;
    
        if (smlen > 160) {
            return false;
        }
        if (smlen >= 7) {                                                       /* 可能存在长短消息 */
            INT8U   buf[3];
            
            pos  = 1;
            left = 3;
            for (i = 0; i < sizeof(buf); i++) {
                if (pos >= lmem) {
                    return false;
                }
                buf[i]  = pmem[pos++] << 5;
                if (pos >= lmem) {
                    return false;
                }
                buf[i] |= pmem[pos] >> 3;
            }
            if (buf[0] == 0x05 && buf[1] == 0x00 && buf[2] == 0x03) {           /* 检测到长短消息标志 */
                long_sm = true;
            }
        }
        if (long_sm) {                                                          /* 如果为长短消息, 则将占用49bit的用户数据段 */
            smlen = smlen - 7;
            pos   = 7;
            left  = 2;
        } else {
            pos   = 1;
            left  = 3;
        }
        for (i = 0; i < smlen; i++) {
            if (pos >= lmem) {
                return false;
            }
            if (left == 8) {
                sm->ud[i] = pmem[pos] >> 1; 
                left = 1;
            } else if (left == 7) {
                sm->ud[i] = pmem[pos++] & 0x7f;
                left = 8;
            } else {
                sm->ud[i] = (pmem[pos++] << (7 - left)) & 0x7f;
                if (pos >= lmem) {
                    return false;
                }
                sm->ud[i] |= pmem[pos] >> (1 + left);
                left = 1 + left;
            }
        }
        sm->udlen = smlen;
        sm->dcs = SM_DCS_BIT7;
    } else if (dcs == 0x04) {                                                   /* ucs2编码 */
        smlen *= 2;
        if (smlen > 140) {
            return false;
        }
        pos = 1;
        for (i = 0; i < smlen; i++) {
            if (pos >= lmem) {
                return false;
            }
            sm->ud[i]  = pmem[pos++] << 5;
            if (pos >= lmem) {
                return false;
            }
            sm->ud[i] |= pmem[pos] >> 3;
        }
        if (smlen >= 6) {
            if (sm->ud[0] == 0x05 && sm->ud[1] == 0x00 && sm->ud[2] == 0x03) {  /* 检测到长短消息标志 */
                memcpy(&sm->ud[0], &sm->ud[6], smlen - 6);                      /* 去掉长短消息头部信息 */
                smlen -= 6;
                sm->ud[smlen] = '\0';
            }
        }
        sm->udlen = smlen;
        sm->dcs = SM_DCS_UCS2;
    } else {                                                                    /* 其它编码格式 */
        return false;
    }
    return true;
}

/*******************************************************************
** 函数名:     AT_SMS_ParseCdmaPduData
** 函数描述:   按照pdu格式解析接收到的短消息
** 参数:       [out] sm:                存放解析后的短消息
**             [in]  pdubuf:            存放接收到pdu格式内容数据
**             [in]  pdusize:           pdu格式内容数据长度
** 返回:       true:  解析成功
**             false: 解析失败
********************************************************************/
BOOLEAN AT_SMS_ParseCdmaPduData(SM_T *sm, INT8U *pdubuf, INT16U pdusize)
{
    INT8U tellen, oalen, left, type, length;
    INT16U lmem, i, pos;
    BOOLEAN timestamp_valid, userdata_valid;
    INT8U *pmem;

    OS_ASSERT((sm != 0), RETURN_FALSE);
    YX_MEMSET((INT8U *)sm, 0, sizeof(SM_T));

    if ((pdusize % 2)) {
        return false;
    }
    
    pmem = pdubuf;
    lmem = YX_AsciiToHex(pdubuf, pdusize, pdubuf, pdusize);                    /* 将ascii数据转换成hex格式 */

    if (lmem < 5) {
        return false;
    }
    if (*pmem++ != 0x00) {
        return false;
    }
    if (*pmem++ != 0x00) {
        return false;
    }
    if (*pmem++ != 0x02) {
        return false;
    }
    if (*pmem++ != 0x10) {
        return false;
    }
    if (*pmem++ != 0x02) {
        return false;
    }
    lmem -= 5;

    if (lmem < 1) {
        return false;
    }
    if (*pmem++ != 0x02) {                                                      /* 0x02表示为发起方号码 */
        return false;
    }
    lmem--;

    if (lmem == 0) {
        return false;
    }
    oalen = *pmem++;                                                              /* 取发起方号码数据段长度(注意: 不是号码长度) */
    lmem--;

    if (oalen < 2 || lmem < oalen) {
        return false;
    }
    if ((pmem[0] & 0xc0) != 0) {                                                /* 说明电话号码的编码格式不支持 */
        return false;
    }
    tellen  = (pmem[0] << 2);
    tellen |= pmem[1] >> 6;
    if (tellen >= sizeof(sm->oa)) {
        return false;
    }
    left = 6;
    pos  = 1;
    for (i = 0; i < tellen; i++) {
        INT8U   ch;

        if (pos >= oalen) {
            return false;
        }
        if (left < 4) {
            ch  = pmem[pos++] << (4 - left);
            if (pos >= oalen) {
                return false;
            }
            
            ch |= pmem[pos] >> (4 + left);
            
            left = 4 + left;
        } else if (left == 4) {
            ch = pmem[pos++];
            left = 8;
        } else {
            ch = pmem[pos] >> (left - 4);
            left -= 4;
        }
        ch &= 0x0f;
        if (ch > 9) {
            ch -= 10;
        }
        sm->oa[i] = YX_HexToChar(ch);
    }
    sm->oalen = i;
    pmem += oalen;
    lmem -= oalen;

    if (lmem < 3) {                                                             /* 跳过: 0x06 0x01 0xfc */
        return false;
    }
    pmem += 3;
    lmem -= 3;

    if (lmem < 2) {
        return false;
    }
    if (pmem[0] != 0x08) {
        return false;
    }
    pmem += 2;
    lmem -= 2;

    timestamp_valid = false;                                                    /* 解析发送时间戳和短消息内容 */
    userdata_valid  = false;
    for (;;) {
        if (timestamp_valid && userdata_valid) {
            break;
        }
    
        if (lmem < 2) {
            return false;
        }
        type   = pmem[0];                                                       /* 获取字段类型 */
        length = pmem[1];                                                       /* 获取字段长度 */
        pmem  += 2;
        lmem  -= 2;
        if (lmem < length) {
            return false;
        }

        if (type == 0x01) {                                                     /* 解析短消息内容 */
            if (ParseUserData(sm, pmem, length)) {
                userdata_valid = true;
            } else {
                return false;
            }
        } else if (type == 0x03) {                                              /* 解析时间戳 */
            if (PaserTimeStamp(sm, pmem, length)) {
                timestamp_valid = true;
            } else {
                return false;
            }
        } else if (type == 0x00) {                                              /* 解析消息ID */
            ;
        }
        pmem += length;
        lmem -= length;
    }
    return true;
}

/*******************************************************************
** 函数名:     AT_SMS_AssembleCdmaPduData
** 函数描述:   按照pdu格式要求对待发送的短消息进行组帧
** 参数:       [out] dptr:              返回存放组帧后的pdu格式数据的缓冲区
**             [in]  dcs:               短消息编码格式
**             [in]  telptr:            接收方手机号码
**             [in]  tellen:            接收方手机号码长度
**             [in]  dataptr:           短消息内容缓冲区
**             [in]  datalen:           短消息内容长度
** 返回:       组帧后的数据长度; 如组帧失败, 则返回0
********************************************************************/
INT16U AT_SMS_AssembleCdmaPduData(INT8U **dptr, SM_DCS_E dcs, INT8U *telptr, INT8U tellen, const INT8U *dataptr, INT8U datalen)
{
#if 0
    INT16U  len, pos;
    INT8U  *p_save1, *p_save2, *p_tmp;

    OS_ASSERT(dptr != PNULL);
    if (telptr == PNULL || tellen == 0) {
        return 0;
    }
    if (dataptr == PNULL || datalen == 0) {
        return 0;
    }
    if (tellen > 22) {                                                          /* 目标手机号码的长度不得超过22位 */
        return 0;
    }

    p_tmp = s_tmpbuf;

    *p_tmp++ = 0x00;                                                            /* 0x00: 点对点, 0x01: 小区广播, 0x02: 短信确认 */
    *p_tmp++ = 0x00;                                                            /* 电信业务ID: 0x00021002 */
    *p_tmp++ = 0x02;
    *p_tmp++ = 0x10;
    *p_tmp++ = 0x02;

    *p_tmp++ = 0x04;                                                            /* 表示后面跟的手机号码为接收方号码 */
    if (telptr[0] == '+') {                                                     /* 去掉手机号码前的国家编码 */
        if (tellen > 3) {
            telptr += 3;
            tellen -= 3;
        } else {
            return 0;
        }
    }

    p_tmp[0]  = 0;                                                              /* 预留用于保存手机字段长度 */
    p_tmp[1]  = 0;
    p_tmp[1] |= tellen >> 2;                                                    /* 填入手机号码长度 */
    p_tmp[2]  = tellen << 6;
    pos = 2;
    for (;;) {
        INT8U   ch;
    
        ch = ril_char_to_hex(*telptr++);
        p_tmp[pos] |= ch << 2;
        if (--tellen == 0) {
            break;
        }
        ch = ril_char_to_hex(*telptr++);
        p_tmp[pos] |= ch >> 2;
        pos++;
        p_tmp[pos]  = ch << 6;
        if (--tellen == 0) {
            break;
        }
    }
    p_tmp[0] = pos;
    p_tmp += pos + 1;

    *p_tmp++ = 0x08;                                                            /* 参数ID, 固定为0x08 */
    p_save1 = p_tmp;                                                            /* 用于写入数据长度 */
    p_tmp++;

    *p_tmp++ = 0x00;                                                            /* 写入MSI, 即消息id */
    *p_tmp++ = 0x03;
    *p_tmp++ = 0x20;
    *p_tmp++ = 0x00;
    *p_tmp++ = 0x00;

    *p_tmp++ = 0x01;
    p_save2 = p_tmp;                                                            /* 用于写入用户数据长度 */
    p_tmp++;

    if (dcs == SM_DCS_UCS2) {                                               /* 处理ucs2编码 */
        if (datalen > 140 || (datalen % 2) != 0) {
            return 0;
        }
        
        *p_tmp = 0x20;                                                          /* 写入编码格式 */
        *p_tmp++ |= (datalen / 2) >> 5;
        *p_tmp    = (datalen / 2 ) << 3;
        for (;;) {
            if (datalen == 0) {
                break;
            }
            *p_tmp++ |= *dataptr >> 5;
            *p_tmp    = *dataptr << 3;
            dataptr++;
            datalen--;
        }
        p_tmp++;
    } else if (dcs == SM_DCS_BIT7) {                                       /* 处理ascii编码 */
        INT8U   left;
    
        if (datalen > 160) {
            return 0;
        }

        *p_tmp    = 0x10;                                                       /* 写入编码格式 */
        *p_tmp++ |= datalen >> 5;
        *p_tmp    = datalen << 3;

        left = 3;
        for (;;) {
            if (datalen == 0) {
                break;
            }
            if (left == 8) {
                *p_tmp = *dataptr << 1;
                dataptr++;
                datalen--;
                left = 1;
            } else if (left == 7) {
                *p_tmp++ |= *dataptr;
                dataptr++;
                datalen--;

                left   = 8;
                *p_tmp = 0;
            } else {
                *p_tmp++ |= *dataptr >> (7 - left);
                *p_tmp    = *dataptr << (left + 1);
                left = 1 + left;                                                /* 此处可保证只要left初始值不为0, 则在循环当中不可能出现left为0的情况 */
                dataptr++;
                datalen--;
            }
        }
        if (left != 8) {
            p_tmp++;
        }
    } else {                                                                    /* 处理其它编码类型 */
        return 0;
    }

    *p_save2 = (INT8U)(p_tmp - p_save2 - 1);                                    /* 填充用户数据长度 */
    *p_tmp++ = 0x08;
    *p_tmp++ = 0x01;
    *p_tmp++ = 0x00;
    
    *p_tmp++ = 0x09;
    *p_tmp++ = 0x01;
    *p_tmp++ = 0x00;
    
    *p_tmp++ = 0x0a;
    *p_tmp++ = 0x01;
    *p_tmp++ = 0x00;
    
    *p_tmp++ = 0x0d;
    *p_tmp++ = 0x01;
    *p_tmp++ = 0x06;
    *p_save1 = (INT8U)(p_tmp - p_save1 - 1);                                    /* 填充承载数据长度 */
    OS_ASSERT(p_tmp <= (s_tmpbuf + sizeof(s_tmpbuf)));

    len = ril_hex_to_ascii(s_pdumem, s_tmpbuf, (INT32U)(p_tmp - s_tmpbuf));
    s_pdumem[len++] = 0x1A;                                                     /* 写入"CTRL+Z" */
    OS_ASSERT(len <= sizeof(s_pdumem));
    
    *dptr = s_pdumem;
	return len;
#endif
    return 0;
}


