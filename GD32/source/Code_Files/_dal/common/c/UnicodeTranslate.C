/**************************************************************************************************
**                                                                                               **
**  文件名称:  UnicodeTranslate.C                                                                **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2010                                    **
**  创建信息:  jump -- 2011年11月22日                                                            **
**  文件描述:  Unicode与GB2312码转换模块                                                         **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/
#include "app_include.h"
#include "unicodetranslate.h"
#include "debug_print.h"

/* 京津冀晋蒙辽吉黑沪苏浙皖闽赣鲁豫鄂湘粤桂琼渝川贵云藏陕甘青宁新港澳
甲乙丙丁己庚辛壬寅辰未申午戍领使警学挂试超知车牌 */
static const INT16U Unicode2GB2312[][2] = 
{
    0x4EAC,0xBEA9,
    0x6D25,0xBDF2,
    0x5180,0xBCBD,
    0x664B,0xBDFA,
    0x8499,0xC3C9,
    0x8FBD,0xC1C9,
    0x5409,0xBCAA,
    0x9ED1,0xBADA,
    0x6CAA,0xBBA6,
    0x82CF,0xCBD5,
    0x6D59,0xD5E3,
    0x7696,0xCDEE,
    0x95FD,0xC3F6,
    0x8D63,0xB8D3,
    0x9C81,0xC2B3,
    0x8C6B,0xD4A5,
    0x9102,0xB6F5,
    0x6E58,0xCFE6,
    0x7CA4,0xD4C1,
    0x6842,0xB9F0,
    0x743C,0xC7ED,
    0x6E1D,0xD3E5,
    0x5DDD,0xB4A8,
    0x8D35,0xB9F3,
    0x4E91,0xD4C6,
    0x85CF,0xB2D8,
    0x9655,0xC9C2,
    0x7518,0xB8CA,
    0x9752,0xC7E0,
    0x5B81,0xC4FE,
    0x65B0,0xD0C2,
    0x6E2F,0xB8DB,
    0x6FB3,0xB0C4,
    0x7532,0xBCD7,
    0x4E59,0xD2D2,
    0x4E19,0xB1FB,
    0x4E01,0xB6A1,
    0x5DF1,0xBCBA,
    0x5E9A,0xB8FD,
    0x8F9B,0xD0C1,
    0x58EC,0xC8C9,
    0x5BC5,0xD2FA,
    0x8FB0,0xB3BD,
    0x672A,0xCEB4,
    0x7533,0xC9EA,
    0x5348,0xCEE7,
    0x620D,0xCAF9,
    0x9886,0xC1EC,
    0x4F7F,0xCAB9,
    0x8B66,0xBEAF,
    0x5B66,0xD1A7,
    0x6302,0xB9D2,
    0x8BD5,0xCAD4,
    0x8D85,0xB3AC,
    0x77E5,0xD6AA,
    0x8F66,0xB3B5,
    0x724C,0xC5C6,
};

/**************************************************************************************************
**  函数名称:  Unicode2GBcode
**  功能描述:  单个unicode转为GB码
**  输入参数:  
**  输出参数:  
**************************************************************************************************/
INT16U Unicode2GBcode(INT16U iUnicode)
{
    INT16U i;

    switch (iUnicode) {
    case 0x0002:
        return 0x24;
    case 0x000a:
        return 0xa;
    case 0x000d:
        return 0xd;
    case 0x0040:
        return 0xA1;
    }

    if ((iUnicode >= 0x20 && iUnicode <= 0x7a)) {//&& iUnicode <= 0x5a) || (iUnicode >= 0x61 
        return iUnicode;
    }
    if (iUnicode == 0) {
        return iUnicode;
    }

    ClearWatchdog();
    for (i = 0; i < sizeof(Unicode2GB2312) / sizeof(Unicode2GB2312[0]); i++) {
        if (Unicode2GB2312[i][0] == iUnicode) {
            return Unicode2GB2312[i][1];
        }
    }
    return 0; //转换不成功
}

/**************************************************************************************************
**  函数名称:  Unicode2GB
**  功能描述:  将unicode码转为GB码
**  输入参数:  src unicode字符串地址指针，注意必须以0xffff结尾
**  返回参数:  总共转换成多少个GB码(不包括结束符)
**************************************************************************************************/
INT16U Unicode2GB(INT16U *src, INT8U *dest)
{
    INT16U tmphz;
    INT16U *pSrc;
    INT8U  *pDest;
    INT16U i;

    pSrc = src;
    pDest = dest;
    for (i = 0; *pSrc != 0xFFFF; pSrc++) {
        #if DEBUG_DIR > 0
        Debug_SysPrint(" %x>", *pSrc);
        #endif
        
        tmphz = Unicode2GBcode(*pSrc);
        #if DEBUG_DIR > 0
        Debug_SysPrint("%x", tmphz);
        #endif
        if (!tmphz || (tmphz > 0x7F && tmphz < 0xFF)) {
            *pDest = ' ';
            i++;
            continue;
        } else if (tmphz <= 0x7F) {
            *pDest++ = tmphz;
            i++;
        } else {
            *pDest++ = (tmphz >> 8);
            *pDest++ = tmphz;
            i += 2;
        }
    }

    *pDest = '\0';
    return i;
}

/**************************************************************************************************
**  函数名称:  GB2Unicode
**  功能描述:  将GB码转为unicode码
**  输入参数:  src GB字符串地址指针，注意必须以字符结束符结尾
**  返回参数:  
**************************************************************************************************/
void GB2Unicode(INT8U *src, INT16U *dest)
{
    INT8U i;
    INT16U gbcode;
    
    while (*src != '\0') {
        if (*src < 0x80) {
            *dest++ = (INT16U)(*src++);
        } else {
            ClearWatchdog();
            gbcode = (*src++ << 8);
            gbcode |= *src++;
            for (i = 0; i < sizeof(Unicode2GB2312) / sizeof(Unicode2GB2312[0]); i++) {
                if (Unicode2GB2312[i][1] == gbcode) {
                    *dest++ = Unicode2GB2312[i][0];
                }
            }
        }
    }
    *dest = 0;
}



/******************* (C) COPYRIGHT 2009 XIAMEN YAXON.LTD *********END OF FILE******/



