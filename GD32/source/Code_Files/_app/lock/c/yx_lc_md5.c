/********************************************************************************
**
** 文件名:     yx_lc_md5.c
** 版权所有:   (c) 2005-2015 厦门雅迅网络股份有限公司
** 文件描述:   实现获取MD5数字摘要
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       |     作者   |  修改记录
**===============================================================================
**| 2015/07/09 | 兰启明 |  创建第一版本
*********************************************************************************/
#include "yx_lc_md5.h"
#include "yx_includes.h"
#include "yx_lock.h"


#define SEEDLEN                     4
#define MASKLEN                     4
#define RANDLEN                     8
#define YUC_GPSID_LEN               4

#define HANDKEYLEN                  8
#define LOCKPSWLEN                  4
#define UNBINDPSWLEN                2


typedef struct {
    INT8U handkeylen;
    INT8U handkey[HANDKEYLEN];               /* 握手校验密码*/
    INT8U lockpswlen;
    INT8U lockpsw[LOCKPSWLEN];               /* 主动锁车密码*/
    INT8U unbindpswlen;
    INT8U unbindpsw[UNBINDPSWLEN];           /* 解绑密码*/
} LOCKKEY_T;


static LOCKKEY_T s_lockkey;
//static INT8U mask[4] = {0x40, 0x39, 0x00, 0x00};                                /* 32bit MASK码，由玉柴提供*/
static INT8U mask[4] = {0xEF, 0x13, 0x33, 0x32};                                /* 32bit MASK码，由玉柴提供*/


/*******************************************************************
** 函数名:      YX_Get_MD5
** 函数描述:    计算MD5值
** 参数:        [in]  str:    计算MD5的缓冲区入口指针
                         [in]  size:  计算MD5的字节长度
** 返回:        MD5计算结果
********************************************************************/
static MD5VAL_T YX_Get_MD5(INT8U *str, INT32U size)
{
    INT32U m, lm, ln, i, *x, *a, *b, *c, *d, aa, bb, cc, dd;
    INT8U *strw;
    MD5VAL_T val = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476};

    if (size == 0) {
        size = sizeof(str);
    }
    m = size % 64;
    lm = size - m;  //数据整块长度

    if (m < 56) {
        ln = lm + 64;
    } else {
        ln = lm + 128;
    }

    strw = (INT8U *)PORT_Malloc(ln);

    //复制原字串到缓冲区strw
    for (i = 0; i < size; i++) {
        strw[i] = str[i];
    }
    //补位
    strw[i++] = 0x80;
    for (; i < ln - 8; i++) {
        strw[i] = 0x00;
    }
    //补长度
    x = (INT32U *)(strw + i);
    *(x++) = size << 3;
    *(x++) = size >> 29;
    //初始化MD5参数

    a = &(val.a);
    b = &(val.b);
    c = &(val.c);
    d = &(val.d);

    for (i = 0; i < ln; i += 64) {
        x = (INT32U *)(strw + i);
        // Save the values
        aa=*a; bb=*b; cc=*c; dd=*d;
        // Round 1
        FF (*a, *b, *c, *d, x[ 0], S11, 0xd76aa478);  /* 1 */
        FF (*d, *a, *b, *c, x[ 1], S12, 0xe8c7b756);  /* 2 */
        FF (*c, *d, *a, *b, x[ 2], S13, 0x242070db);  /* 3 */
        FF (*b, *c, *d, *a, x[ 3], S14, 0xc1bdceee);  /* 4 */
        FF (*a, *b, *c, *d, x[ 4], S11, 0xf57c0faf);  /* 5 */
        FF (*d, *a, *b, *c, x[ 5], S12, 0x4787c62a);  /* 6 */
        FF (*c, *d, *a, *b, x[ 6], S13, 0xa8304613);  /* 7 */
        FF (*b, *c, *d, *a, x[ 7], S14, 0xfd469501);  /* 8 */
        FF (*a, *b, *c, *d, x[ 8], S11, 0x698098d8);  /* 9 */
        FF (*d, *a, *b, *c, x[ 9], S12, 0x8b44f7af);  /* 10 */
        FF (*c, *d, *a, *b, x[10], S13, 0xffff5bb1);  /* 11 */
        FF (*b, *c, *d, *a, x[11], S14, 0x895cd7be);  /* 12 */
        FF (*a, *b, *c, *d, x[12], S11, 0x6b901122);  /* 13 */
        FF (*d, *a, *b, *c, x[13], S12, 0xfd987193);  /* 14 */
        FF (*c, *d, *a, *b, x[14], S13, 0xa679438e);  /* 15 */
        FF (*b, *c, *d, *a, x[15], S14, 0x49b40821);  /* 16 */
        // Round 2
        GG (*a, *b, *c, *d, x[ 1], S21, 0xf61e2562);  /* 17 */
        GG (*d, *a, *b, *c, x[ 6], S22, 0xc040b340);  /* 18 */
        GG (*c, *d, *a, *b, x[11], S23, 0x265e5a51);  /* 19 */
        GG (*b, *c, *d, *a, x[ 0], S24, 0xe9b6c7aa);  /* 20 */
        GG (*a, *b, *c, *d, x[ 5], S21, 0xd62f105d);  /* 21 */
        GG (*d, *a, *b, *c, x[10], S22,  0x2441453);  /* 22 */
        GG (*c, *d, *a, *b, x[15], S23, 0xd8a1e681);  /* 23 */
        GG (*b, *c, *d, *a, x[ 4], S24, 0xe7d3fbc8);  /* 24 */
        GG (*a, *b, *c, *d, x[ 9], S21, 0x21e1cde6);  /* 25 */
        GG (*d, *a, *b, *c, x[14], S22, 0xc33707d6);  /* 26 */
        GG (*c, *d, *a, *b, x[ 3], S23, 0xf4d50d87);  /* 27 */
        GG (*b, *c, *d, *a, x[ 8], S24, 0x455a14ed);  /* 28 */
        GG (*a, *b, *c, *d, x[13], S21, 0xa9e3e905);  /* 29 */
        GG (*d, *a, *b, *c, x[ 2], S22, 0xfcefa3f8);  /* 30 */
        GG (*c, *d, *a, *b, x[ 7], S23, 0x676f02d9);  /* 31 */
        GG (*b, *c, *d, *a, x[12], S24, 0x8d2a4c8a);  /* 32 */
        // Round 3
        HH (*a, *b, *c, *d, x[ 5], S31, 0xfffa3942);  /* 33 */
        HH (*d, *a, *b, *c, x[ 8], S32, 0x8771f681);  /* 34 */
        HH (*c, *d, *a, *b, x[11], S33, 0x6d9d6122);  /* 35 */
        HH (*b, *c, *d, *a, x[14], S34, 0xfde5380c);  /* 36 */
        HH (*a, *b, *c, *d, x[ 1], S31, 0xa4beea44);  /* 37 */
        HH (*d, *a, *b, *c, x[ 4], S32, 0x4bdecfa9);  /* 38 */
        HH (*c, *d, *a, *b, x[ 7], S33, 0xf6bb4b60);  /* 39 */
        HH (*b, *c, *d, *a, x[10], S34, 0xbebfbc70);  /* 40 */
        HH (*a, *b, *c, *d, x[13], S31, 0x289b7ec6);  /* 41 */
        HH (*d, *a, *b, *c, x[ 0], S32, 0xeaa127fa);  /* 42 */
        HH (*c, *d, *a, *b, x[ 3], S33, 0xd4ef3085);  /* 43 */
        HH (*b, *c, *d, *a, x[ 6], S34,  0x4881d05);  /* 44 */
        HH (*a, *b, *c, *d, x[ 9], S31, 0xd9d4d039);  /* 45 */
        HH (*d, *a, *b, *c, x[12], S32, 0xe6db99e5);  /* 46 */
        HH (*c, *d, *a, *b, x[15], S33, 0x1fa27cf8);  /* 47 */
        HH (*b, *c, *d, *a, x[ 2], S34, 0xc4ac5665);  /* 48 */
        // Round 4 */
        II (*a, *b, *c, *d, x[ 0], S41, 0xf4292244);  /* 49 */
        II (*d, *a, *b, *c, x[ 7], S42, 0x432aff97);  /* 50 */
        II (*c, *d, *a, *b, x[14], S43, 0xab9423a7);  /* 51 */
        II (*b, *c, *d, *a, x[ 5], S44, 0xfc93a039);  /* 52 */
        II (*a, *b, *c, *d, x[12], S41, 0x655b59c3);  /* 53 */
        II (*d, *a, *b, *c, x[ 3], S42, 0x8f0ccc92);  /* 54 */
        II (*c, *d, *a, *b, x[10], S43, 0xffeff47d);  /* 55 */
        II (*b, *c, *d, *a, x[ 1], S44, 0x85845dd1);  /* 56 */
        II (*a, *b, *c, *d, x[ 8], S41, 0x6fa87e4f);  /* 57 */
        II (*d, *a, *b, *c, x[15], S42, 0xfe2ce6e0);  /* 58 */
        II (*c, *d, *a, *b, x[ 6], S43, 0xa3014314);  /* 59 */
        II (*b, *c, *d, *a, x[13], S44, 0x4e0811a1);  /* 60 */
        II (*a, *b, *c, *d, x[ 4], S41, 0xf7537e82);  /* 61 */
        II (*d, *a, *b, *c, x[11], S42, 0xbd3af235);  /* 62 */
        II (*c, *d, *a, *b, x[ 2], S43, 0x2ad7d2bb);  /* 63 */
        II (*b, *c, *d, *a, x[ 9], S44, 0xeb86d391);  /* 64 */
        // Add the original values
        (*a) += aa;
        (*b) += bb;
        (*c) += cc;
        (*d) += dd;
    }
    PORT_Free(strw);
    strw = NULL;

    return val;
}

/*******************************************************************
** 函数名:      get_rand_str
** 函数描述:    映射转换函数
** 参数:        [in]  rand:           需要转换的数据
                [out]  ptr:           映射转换后的数据
                [in]  size:           需要转换的数据长度
** 返回:        NULL
********************************************************************/
static void get_rand_str(INT8U *rand, INT8U *ptr, INT8U size)
{
    INT8U i;
    INT8U str[64] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    str[62] = 0;
    str[63] = 0;

    for (i = 0; i < size; i++) {
        while (rand[i] >=62) {
            rand[i] -= 62;
        }
        ptr[i] = str[rand[i]];
    }
}

#if 1
/*******************************************************************
** 函数名:      YX_LC_InputSeed
** 函数描述:    设置ECU握手校验时提供的算法的随机种子
** 参数:        [in]  seed:           算法的随机种子指针
                [in]  len:            种子长度
** 返回:        NULL
********************************************************************/
void YX_LC_InputSeed(INT8U algor, INT8U *seed, INT8U seedlen)
{
    INT8U gpsid[4];
    INT8U randin[12], randout[8], key[8];
    INT8U randlen = 0;
    MD5VAL_T md5val;

    memset((INT8U *)&s_lockkey, 0, sizeof(LOCKKEY_T));

    if (seed == NULL || seedlen != SEEDLEN) {
        return;
    }

    memcpy(randin, seed, SEEDLEN);
    randlen = SEEDLEN;
    #if DEBUG_LOCK > 0
    debug_printf("YX_LC_InputSeed seed: ");
    printf_hex(randin, 4);
    debug_printf("\r\n");
    #endif
    memcpy(randin + SEEDLEN, mask, MASKLEN);
    randlen += MASKLEN;
    #if DEBUG_LOCK > 0
    debug_printf("YX_LC_InputSeed mask: ");
    printf_hex(randin, 8);
    debug_printf("\r\n");
    #endif

    GetGpsID(gpsid);
    #if DEBUG_LOCK > 0
    debug_printf("YX_LC_InputSeed gpsid: ");
    printf_hex(gpsid, 4);
    debug_printf("\r\n");
    #endif

    if (algor == 0x01) {
        memcpy(randin + randlen, gpsid, YUC_GPSID_LEN); //新算法写入gpsid
        randlen += YUC_GPSID_LEN;
        #if DEBUG_LOCK > 0
        debug_printf("玉柴锁车新算法rand:");
        printf_hex(randin, randlen);
        debug_printf("\r\n");
        #endif
    }

    get_rand_str(randin, randout, RANDLEN);
    #if DEBUG_LOCK > 0
    debug_printf("玉柴锁车算法rand:");
    printf_hex(randin, randlen);
    debug_printf("\r\n");
    #endif
    
    md5val = YX_Get_MD5(randout, RANDLEN);
	
    if (algor == 0x00) {  //老算法
        key[0] = (md5val.b & 0xff);
        key[1] = (md5val.b & 0xff00) >> 8;
        key[2] = (md5val.b & 0xff0000) >> 16;
        key[3] = (md5val.b & 0xff000000) >> 24;
        key[4] = (md5val.c & 0xff);
        key[5] = (md5val.c & 0xff00) >> 8;
        key[6] = (md5val.c & 0xff0000) >> 16;
        key[7] = (md5val.c & 0xff000000) >> 24;

        s_lockkey.handkey[0] = key[2];
        s_lockkey.handkey[1] = gpsid[0];
        s_lockkey.handkey[2] = key[3];
        s_lockkey.handkey[3] = gpsid[1];
        s_lockkey.handkey[4] = key[4];
        s_lockkey.handkey[5] = gpsid[2];
        s_lockkey.handkey[6] = key[5];
        s_lockkey.handkey[7] = gpsid[3];
        s_lockkey.handkeylen = HANDKEYLEN;

        s_lockkey.lockpsw[0] = key[0];
        s_lockkey.lockpsw[1] = key[1];
        s_lockkey.lockpsw[2] = key[4];
        s_lockkey.lockpsw[3] = key[5];
        s_lockkey.lockpswlen = LOCKPSWLEN;

        s_lockkey.unbindpsw[0] = key[6];
        s_lockkey.unbindpsw[1] = key[7];
        s_lockkey.unbindpswlen = UNBINDPSWLEN;
    }else {     
        s_lockkey.handkey[0] = (md5val.b & 0xff);          //新算法
        s_lockkey.handkey[1] = (md5val.b & 0xff00) >> 8;
        s_lockkey.handkey[2] = (md5val.b & 0xff0000) >> 16;
        s_lockkey.handkey[3] = (md5val.b & 0xff000000) >> 24;
        s_lockkey.handkey[4] = (md5val.c & 0xff);
        s_lockkey.handkey[5] = (md5val.c & 0xff00) >> 8;
        s_lockkey.handkey[6] = (md5val.c & 0xff0000) >> 16; 
        s_lockkey.handkey[7] = (md5val.c & 0xff000000) >> 24;
        s_lockkey.handkeylen = HANDKEYLEN;
       
        s_lockkey.lockpsw[0] = (md5val.a & 0xff);
        s_lockkey.lockpsw[1] = (md5val.a & 0xff00) >> 8;
        s_lockkey.lockpsw[2] = (md5val.a & 0xff0000) >> 16;
        s_lockkey.lockpsw[3] = (md5val.a & 0xff000000) >> 24;
        s_lockkey.lockpswlen = LOCKPSWLEN;
        
        s_lockkey.unbindpsw[0] = (md5val.d & 0xff);
        s_lockkey.unbindpsw[1] = (md5val.d & 0xff00) >> 8;
        s_lockkey.unbindpswlen = UNBINDPSWLEN;
    }

    #if EN_DEBUG > 0
    debug_printf("randin1(seed+mask):");
    printf_hex((INT8U *)randin, sizeof(randin));
    debug_printf("randout:");
    printf_hex((INT8U *)randout, sizeof(randout));    
    debug_printf(",64bit psw:");
    printf_hex((INT8U *)key, sizeof(key));    
    debug_printf("hkey:");
    printf_hex((INT8U *)s_lockkey.handkey, sizeof(s_lockkey.handkey));   
    debug_printf("gpsid:");
    printf_hex((INT8U *)gpsid, sizeof(gpsid));  

    // debug_printf("lockpsw:");
    // printf_hex((INT8U *)s_lockkey.lockpsw, sizeof(s_lockkey.lockpsw)); 
    // debug_printf("unbindpsw:");
    // printf_hex((INT8U *)s_lockkey.unbindpsw, sizeof(s_lockkey.unbindpsw));
    // debug_printf("\r\n");
    #endif 
}
#endif
/*******************************************************************
** 函数名:      YX_LC_GetHKey
** 函数描述:    获取握手校验密码key
** 参数:        [out]  hkey:           握手校验密码key
** 返回:        TRUE/FALSE
********************************************************************/
BOOLEAN YX_LC_GetHKey(INT8U *hkey)
{
    if (s_lockkey.handkeylen != HANDKEYLEN) {
        return FALSE;
    }

    memcpy(hkey,s_lockkey.handkey, HANDKEYLEN);

    return TRUE;
}

/*******************************************************************
** 函数名:      YX_LC_GetLockPSW
** 函数描述:    获取主动锁车密码
** 参数:        [out]  psw:           主动锁车密码
** 返回:        TRUE/FALSE
********************************************************************/
BOOLEAN YX_LC_GetLockPSW(INT8U *psw)
{
    if (s_lockkey.lockpswlen != LOCKPSWLEN) {
        return FALSE;
    }

    memcpy(psw, s_lockkey.lockpsw, LOCKPSWLEN);

    return TRUE;
}

/*******************************************************************
** 函数名:      YX_LC_GetUnbindPSW
** 函数描述:    获取解绑密码
** 参数:        [out]  psw:           解绑密码
** 返回:        TRUE/FALSE
********************************************************************/
BOOLEAN YX_LC_GetUnbindPSW(INT8U *psw)
{
    if (s_lockkey.unbindpswlen != UNBINDPSWLEN) {
        return FALSE;
    }

    memcpy(psw, s_lockkey.unbindpsw, UNBINDPSWLEN);

    return TRUE;
}
#if 0
/*******************************************************************
** 函数名:      YX_LC_GpsID
** 函数描述:    由终端ID生成GPS ID
** 参数:        [out]  gpsid:           GPS ID
** 返回:        NULL
********************************************************************/
void YX_LC_GpsID(INT8U *gpsid)
{
    INT8U devid[8] = {0};
    DEVICEINFO_T deviceinfo;

    YX_MEMSET((INT8U *)&deviceinfo, 0, sizeof(DEVICEINFO_T));
    DAL_PP_ReadParaByID(DEVICEINFO_, (INT8U *)&deviceinfo, sizeof(DEVICEINFO_T));

    YX_MEMCPY(devid + 1, 7, deviceinfo.devid, 7);
    YX_AsciiToBcd(gpsid, devid, 8);
}
#endif

