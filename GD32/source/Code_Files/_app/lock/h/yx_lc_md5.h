/********************************************************************************
**
** 文件名:     yx_lc_md5.h
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

#ifndef H_YX_LC_MD5
#define H_YX_LC_MD5                 1
#include "yx_includes.h"


//DEFINES for MD5

/* F, G, H and I are basic MD5 functions. */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

/* ROTATE_LEFT rotates x left n bits. */
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
Rotation is separate from addition to prevent recomputation. */
#define FF(a, b, c, d, x, s, ac) { \
 (a) += F ((b), (c), (d)) + (x) + (INT32U)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define GG(a, b, c, d, x, s, ac) { \
 (a) += G ((b), (c), (d)) + (x) + (INT32U)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define HH(a, b, c, d, x, s, ac) { \
 (a) += H ((b), (c), (d)) + (x) + (INT32U)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define II(a, b, c, d, x, s, ac) { \
 (a) += I ((b), (c), (d)) + (x) + (INT32U)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }

// Constants for MD5 Transform routine.
#define S11     7
#define S12     12
#define S13     17
#define S14     22
#define S21     5
#define S22     9
#define S23     14
#define S24     20
#define S31     4
#define S32     11
#define S33     16
#define S34     23
#define S41     6
#define S42     10
#define S43     15
#define S44     21


typedef struct {
    INT32U a;
    INT32U b;
    INT32U c;
    INT32U d;
} MD5VAL_T;



/*******************************************************************
** 函数名:      YX_LC_InputSeed
** 函数描述:    设置ECU握手校验时提供的算法的随机种子
** 参数:        [in]  seed:           算法的随机种子指针
                         [in]  len:            种子长度
** 返回:        NULL
********************************************************************/
void YX_LC_InputSeed(INT8U *seed, INT8U len);

/*******************************************************************
** 函数名:      YX_LC_GetHKey
** 函数描述:    获取握手校验密码key
** 参数:        [out]  hkey:           握手校验密码key
** 返回:        TRUE/FALSE
********************************************************************/
BOOLEAN YX_LC_GetHKey(INT8U *hkey);

/*******************************************************************
** 函数名:      YX_LC_GetLockPSW
** 函数描述:    获取主动锁车密码
** 参数:        [out]  psw:           主动锁车密码
** 返回:        TRUE/FALSE
********************************************************************/
BOOLEAN YX_LC_GetLockPSW(INT8U *psw);

/*******************************************************************
** 函数名:      YX_LC_GetUnbindPSW
** 函数描述:    获取解绑密码
** 参数:        [out]  psw:           解绑密码
** 返回:        TRUE/FALSE
********************************************************************/
BOOLEAN YX_LC_GetUnbindPSW(INT8U *psw);

/*******************************************************************
** 函数名:      YX_LC_GpsID
** 函数描述:    由终端ID生成GPS ID
** 参数:        [out]  gpsid:           GPS ID
** 返回:        NULL
********************************************************************/
void YX_LC_GpsID(INT8U *gpsid);


#endif

