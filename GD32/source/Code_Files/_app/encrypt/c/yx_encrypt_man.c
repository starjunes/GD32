/*
********************************************************************************
** 文件名:     yx_encrypt_man.c
** 版权所有:   (c) 2018 厦门雅迅网络股份有限公司
** 文件描述:   加密算法接口
** 创建人：    阙存先，2019.05.17
********************************************************************************
*/
#include "yx_includes.h"
#include "appmain.h"
#include "port_dm.h"
#include "port_timer.h"

#include "debug_print.h"
#include "bal_stream.h"
#include "bal_tools.h"

#include "yx_enc_fun.h"
#include "yx_com_send.h"
#include "nts_secure_element_sdk.h"

#if EN_DEBUG > 1
#undef DEBUG_ENC
#define DEBUG_ENC               0
#endif

#define CMD_ENC                 0x26

#ifdef YX_MEMCPY
#undef YX_MEMCPY
#endif

#define YX_MEMCPY(a, b , c, d)    memcpy(a, c, d)               

typedef enum {
    ENC_SUCCESS                 = 0x00,     /* 成功 */
    ENC_FAILED                  = 0x01,     /* 失败 */
} ENC_ACK_E;

typedef enum {
    ALG_SM2_1                   = 0x00,     /* 算法类型 0x01 - SM2_1 */
    ALG_SM2_2                   = 0x01,     /* 算法类型 0x02 - SM2_2 */
    ALG_SM2_3                   = 0x02,     /* 算法类型 0x03 - SM2_3 */

    ALG_SM3_1                   = 0x10,     /* 算法类型 SM3_1 */
} ENC_ALG_E;

typedef enum {
    ENC_TYPE_NONE               = 0x00,     /* 空 */
    ENC_TYPE_READ_CHIPID        = 0x01,     /* 获取加密芯片ID */
    ENC_TYPE_READ_PUBKEY        = 0x02,     /* 读取公钥 */
    ENC_TYPE_HASH               = 0x03,     /* 生成摘要 */
    ENC_TYPE_SIGN               = 0x04,     /* 生成签名 */
    ENC_TYPE_ENCRYPT            = 0x05,     /* 加密明文 */
    ENC_TYPE_DECRYPT            = 0x06,     /* 解密密文 */
    ENC_TYPE_GEN_KEYPAIR        = 0x07,     /* 生成公私钥对 */
    ENC_TYPE_SET_CHIPID         = 0x08,     /* 设置加密芯片ID */
    RC_TYPE_MAX,
} ENC_TYPE_E;

#define STEP_PULLUP_WK_PIN      0x01
#define STEP_PULLDOWN_WK_PIN    0x02
#define STEP_HDL_ENC_DATA       0x03
#define STEP_ENC_HDL_END        0x04

typedef struct {
    BOOLEAN is_busy;
    INT8U   step;
    INT8U   data_buf[1024];
} ENC_HDL_T;

static INT8U s_reset_enc = 3;
static INT32U  s_encrypt_tmr;
static INT32U  s_encrypt_tmr2;
static ENC_HDL_T s_enc_hdl;

#if DEBUG_ENC >= 0
static INT32U signal_err_times = 0;
#endif

#if 0
static INT8U Pub_PrikeyID[6] = {0x0f, 0x10, 0x1f, 0x20, 0x2f, 0x30};
#else
static INT8U Pub_PrikeyID[8] = {
  /*公钥，私钥 */ 
    0x0f, 0x10,    /* 第1组公私钥对,为默认公私钥对，实际使用中不使用，华大加密芯片默认公私要对，所有芯片都一样 */
    0x1f, 0x20,    /* 第2组公私钥对 */   
    0x2f, 0x30,    /* 第3组公私钥对 */  
    0x13, 0x14     
};
#endif

/*******************************************************************************
 **  函数名称:  YX_Encrypt_Read_ChipID
 **  功能描述:  获取加密芯片ID
 **  输入参数:  [IN]	sn:	     流水号
                [IN]	*buffer: 数据指针
 **  		    [IN]	datelen: 数据长度
 **  返回参数:  无
 ******************************************************************************/
static void YX_Encrypt_Read_ChipID(INT16U sn, INT8U *buf, INT16U len)
{
    int ret = 0;
    INT32U outlen = 0;
    INT8U  *outbuf;
	//INT8U outbuf[256];
    STREAM_T *wstrm;

    wstrm = bal_STREAM_GetBufferStream();

    #if DEBUG_ENC > 0
	debug_printf_dir("\r\n获取加密芯片ID  len:%d  buffer:", len);
    //printf_hex(buf, len);
    debug_printf_dir("\r\n");
    #endif

    outbuf = PORT_Malloc(16);
    if (outbuf == NULL) {
        ret = 0xFFFF;
        goto END;
    }
	//YX_MEMSET(outbuf, 0, sizeof(outbuf));
    /* ************************** START 不同的加密芯片替换此部分代码 START ******************** */
    ret = nts_se_get_se_id(outbuf, &outlen);
	if (ret != 0) {
		s_reset_enc++;
        #if DEBUG_ENC > 0
		debug_printf_dir("Dmt_Get_SN is Failed!\r\n");
        debug_printf_dir("The return code is: 0x%08X\r\n", ret);
        #endif
	} else {
        s_reset_enc = 0;
        #if DEBUG_ENC > 0
        debug_printf_dir("Dmt_Get_SN is OK!\r\n");
        debug_printf_dir("The return SN is(outlen:%d):\r\n", outlen);
        printf_hex_dir(outbuf, outlen);
        debug_printf_dir("\r\n");
        #endif
	}
    /* **************************** END  不同的加密芯片替换此部分代码 END ********************** */

END:
    /* 应答 */
    bal_WriteHWORD_Strm(wstrm, CLIENT_CODE);
    bal_WriteBYTE_Strm(wstrm, CMD_ENC);
    bal_WriteHWORD_Strm(wstrm, sn);
    bal_WriteBYTE_Strm(wstrm, ENC_TYPE_READ_CHIPID);
    if (ret != 0) {
        bal_WriteHWORD_Strm(wstrm, 1);
        bal_WriteBYTE_Strm(wstrm, ENC_FAILED);
    } else {
        bal_WriteHWORD_Strm(wstrm, outlen + 3);
        bal_WriteBYTE_Strm(wstrm, ENC_SUCCESS);
        bal_WriteHWORD_Strm(wstrm, outlen);
        bal_WriteDATA_Strm(wstrm, outbuf, outlen);
    }
    YX_COM_DirSend( CLIENT_FUNCTION_DOWN_REQ_ACK, bal_GetStrmStartPtr(wstrm), bal_GetStrmLen(wstrm));

    if (outbuf != NULL) {
		PORT_Free(outbuf);
	}
}

/*******************************************************************************
 **  函数名称:  YX_Encrypt_Read_Pubkey
 **  功能描述:  读取公钥
 **  输入参数:  [IN]	sn:	     流水号
                [IN]	*buffer: 数据指针
 **  		    [IN]	datelen: 数据长度
 **  返回参数:  无
 ******************************************************************************/
static void YX_Encrypt_Read_Pubkey(INT16U sn, INT8U *buf, INT16U len)
{
    int ret = 0;
	INT8U index = 0; /* SM2密钥对索引号 */
	INT32U outlen = 0;
    INT8U  *outbuf;
	//INT8U outbuf[64] = {0};
    STREAM_T *wstrm;
    wstrm = bal_STREAM_GetBufferStream();

    #if DEBUG_ENC > 0
	debug_printf_dir("\r\n读取公钥  len:%d  buffer:", len);
    //printf_hex(buf, len);
    debug_printf_dir("\r\n");
    #endif

    index = buf[0];
    if (index > ALG_SM2_3) {
        #if DEBUG_ENC > 0
        debug_printf_dir("算法类型错误 %d!\r\n", index);
        #endif

        ret = 0xFFFF;
        goto END;
    }

    outbuf = PORT_Malloc(64);
    if (outbuf == NULL) {
        ret = 0xFFFF;
        goto END;
    }
	//YX_MEMSET(outbuf, 0, sizeof(outbuf));
    /* ************************** START 不同的加密芯片替换此部分代码 START ******************** */
    ret = nts_se_export_sm2_pubkey(Pub_PrikeyID[index * 2], outbuf, &outlen);
    if (ret != 0) {
		s_reset_enc++;
        #if DEBUG_ENC >= 0
        debug_printf_dir("The SM2 pubkey %d Export is Failed!\r\n", index);
        debug_printf_dir("The return code is: 0x%08X\r\n", ret);
        #endif
    } else {
        s_reset_enc = 0;
        #if DEBUG_ENC > 0
        debug_printf_dir("The Pubkey %d is(outlen:%d):\r\n", index, outlen);
        printf_hex_dir(outbuf, outlen);
        debug_printf_dir("\r\n");
        debug_printf_dir("The Pubkey %d Export is OK!\r\n", index);
        #endif
    }
    /* **************************** END  不同的加密芯片替换此部分代码 END ********************** */


END:
    /* 应答 */
    bal_WriteHWORD_Strm(wstrm, CLIENT_CODE);
    bal_WriteBYTE_Strm(wstrm, CMD_ENC);
    bal_WriteHWORD_Strm(wstrm, sn);
    bal_WriteBYTE_Strm(wstrm, ENC_TYPE_READ_PUBKEY);
    if (ret != 0) {
        bal_WriteHWORD_Strm(wstrm, 1);
        bal_WriteBYTE_Strm(wstrm, ENC_FAILED);
        #if DEBUG_ENC > 0
        debug_printf_dir("err\r\n");
        #endif
    } else {
        bal_WriteHWORD_Strm(wstrm, outlen + 3);
        bal_WriteBYTE_Strm(wstrm, ENC_SUCCESS);
        bal_WriteHWORD_Strm(wstrm, outlen);
        bal_WriteDATA_Strm(wstrm, outbuf, outlen);
        #if DEBUG_ENC > 0
        debug_printf_dir("out\r\n");
        #endif
    }
	#if DEBUG_ENC > 0
	debug_printf("YX_Encrypt_Read_Pubkey str:");
	Debug_PrintHex(TRUE,bal_GetStrmStartPtr(wstrm),bal_GetStrmLen(wstrm));
	#endif
	YX_COM_DirSend( CLIENT_FUNCTION_DOWN_REQ_ACK, bal_GetStrmStartPtr(wstrm), bal_GetStrmLen(wstrm));

    if (outbuf != NULL) {
		PORT_Free(outbuf);
	}
}

/*******************************************************************************
 **  函数名称:  YX_Encrypt_Hash
 **  功能描述:  生成摘要
 **  输入参数:  [IN]	sn:	     流水号
                [IN]	*buffer: 数据指针
 **  		    [IN]	datelen: 数据长度
 **  返回参数:  无
 ******************************************************************************/
static void YX_Encrypt_Hash(INT16U sn, INT8U *buf, INT16U len)
{
    int  ret = 0;
	INT8U  index = 0; /* SM2密钥对索引号 */
	INT16U inlen = 0;
    INT32U outlen = 0;
    INT8U  *outbuf;
	//INT8U  outbuf[256];
    STREAM_T *wstrm;
    wstrm = bal_STREAM_GetBufferStream();

    #if DEBUG_ENC > 0
	debug_printf_dir("\r\n生成摘要  len:%d  buffer:", len);
    //printf_hex(buf, len);
    debug_printf_dir("\r\n");
    #endif

    index = buf[0];
    if (index != ALG_SM3_1) {
        #if DEBUG_ENC > 0
        debug_printf_dir("算法类型错误 %d!\r\n", index);
        #endif

        ret = 0xFFFF;
        goto END;
    }

    inlen = bal_chartoshort(&buf[1]);     /* 明文长度 */
    outbuf = PORT_Malloc(256);
    if (outbuf == NULL) {
        ret = 0xFFFF;
        goto END;
    }
	//YX_MEMSET(outbuf, 0, sizeof(outbuf));
    /* ************************** START 不同的加密芯片替换此部分代码 START ******************** */
    ret = 0xFFFF;
    goto END;

    inlen = inlen;
	//ret = Dmt_SM3_Hash(&buf[3], inlen, outbuf);
	if (ret != 0) {
		s_reset_enc++;
        #if DEBUG_ENC > 0
		debug_printf_dir("Dmt_SM3_Hash is Failed!\r\n");
		debug_printf_dir("The return code is: 0x%08X\r\n", ret);
        #endif
	} else {
	    s_reset_enc = 0;
        outlen = 32;
        #if DEBUG_ENC > 0
		debug_printf_dir("Dmt_SM3_Hash is OK!\r\n");
		debug_printf_dir("The return hash is: ");
		printf_hex_dir(outbuf, outlen);
		debug_printf_dir("\r\n");
        #endif

	}
    /* **************************** END  不同的加密芯片替换此部分代码 END ********************** */


END:
    /* 应答 */
    bal_WriteHWORD_Strm(wstrm, CLIENT_CODE);
    bal_WriteBYTE_Strm(wstrm, CMD_ENC);
    bal_WriteHWORD_Strm(wstrm, sn);
    bal_WriteBYTE_Strm(wstrm, ENC_TYPE_HASH);
    if (ret != 0) {
        bal_WriteHWORD_Strm(wstrm, 1);
        bal_WriteBYTE_Strm(wstrm, ENC_FAILED);
    } else {
        bal_WriteHWORD_Strm(wstrm, outlen + 3);
        bal_WriteBYTE_Strm(wstrm, ENC_SUCCESS);
        bal_WriteHWORD_Strm(wstrm, outlen);
        bal_WriteDATA_Strm(wstrm, outbuf, outlen);
    }
    YX_COM_DirSend( CLIENT_FUNCTION_DOWN_REQ_ACK, bal_GetStrmStartPtr(wstrm), bal_GetStrmLen(wstrm));

    if (outbuf != NULL) {
		PORT_Free(outbuf);
	}
}

/*******************************************************************************
 **  函数名称:  YX_Encrypt_Sign
 **  功能描述:  生成签名
 **  输入参数:  [IN]	sn:	     流水号
                [IN]	*buffer: 数据指针
 **  		    [IN]	datelen: 数据长度
 **  返回参数:  无
 ******************************************************************************/
static void YX_Encrypt_Sign(INT16U sn, INT8U *buf, INT16U len)
{
    int  ret = 0;
	INT8U  index = 0; /* SM2密钥对索引号 */
	INT8U  datatype; /* 数据类型：0x00 - 摘要  0x01 - 明文 */
	INT16U idlen = 0, inlen = 0;
	INT32U outlen = 0;
    INT8U  *idptr, *dataptr;
    INT8U  *outbuf;
	
    STREAM_T rstrm;
    STREAM_T *wstrm;
    wstrm = bal_STREAM_GetBufferStream();

    #if DEBUG_ENC > 0
	debug_printf_dir("\r\n生成签名  len:%d  buffer:", len);
    //printf_hex(buf, len);
    debug_printf_dir("\r\n");
    #endif


    bal_InitStrm(&rstrm, (INT8U *)buf, len);
    index = bal_ReadBYTE_Strm(&rstrm);
    #if DEBUG_ENC > 0
    debug_printf_dir("Sign算法类型: %d\r\n", index);
    #endif
    if (index > ALG_SM2_3) {
        #if DEBUG_ENC > 0
        debug_printf_dir("算法类型错误 %d!\r\n", index);
        #endif

        ret = 0xFFFF;
        goto END;
    }

    datatype = bal_ReadBYTE_Strm(&rstrm);
    #if DEBUG_ENC > 0
    debug_printf_dir("Sign数据类型: %d\r\n", datatype);
    #endif
    if (datatype == 0x00) { /* 摘要 */
        idlen = bal_ReadHWORD_Strm(&rstrm);    /* ID长度 */
        if (idlen != 0x00) {
            #if DEBUG_ENC > 0
            debug_printf_dir("对摘要签名ID长度不为0 %d!\r\n", idlen);
            #endif

            ret = 0xFFFF;
            goto END;
        }
        inlen = bal_ReadHWORD_Strm(&rstrm);    /* 摘要长度 */
        dataptr = bal_GetStrmPtr(&rstrm);
        bal_MovStrmPtr(&rstrm, inlen);

        /* 目前不支持对摘要签名 */
        #if DEBUG_ENC > 0
        debug_printf_dir("目前不支持对摘要签名\r\n");
        #endif
        ret = 0xFFFF;
        goto END;
    } else if (datatype == 0x01) { /* 明文 */
        idlen = bal_ReadHWORD_Strm(&rstrm);    /* ID长度 */
        idptr = bal_GetStrmPtr(&rstrm);
        bal_MovStrmPtr(&rstrm, idlen);
        #if DEBUG_ENC > 0
        debug_printf_dir("idlen:%d  id:",idlen);
		    printf_hex_dir(idptr, idlen);
        debug_printf_dir("\r\n");
        #endif

        inlen = bal_ReadHWORD_Strm(&rstrm);    /* 明文长度 */
        dataptr = bal_GetStrmPtr(&rstrm);
        bal_MovStrmPtr(&rstrm, inlen);
        #if DEBUG_ENC > 0
        debug_printf_dir("inlen:%d  data:",inlen);
		    printf_hex_dir(dataptr, 30);
        debug_printf_dir("\r\n");
        #endif
    } else {
        #if DEBUG_ENC > 0
        debug_printf_dir("数据类型错误 %d!\r\n", datatype);
        #endif

        ret = 0xFFFF;
        goto END;
    }

    outbuf = PORT_Malloc(64);
    if (outbuf == NULL) {
        ret = 0xFFFF;
        goto END;
    }
	
	//YX_MEMSET(outbuf, 0, sizeof(outbuf));
    /* ************************** START 不同的加密芯片替换此部分代码 START ******************** */
    ret = nts_se_data_sm2_signature_with_za(dataptr, inlen, outbuf, &outlen, Pub_PrikeyID[index * 2 + 1], Pub_PrikeyID[index * 2], idptr);

	if (ret != 0) {
		s_reset_enc++;
        #if DEBUG_ENC > 0
		debug_printf_dir("The return code is: 0x%08X\r\n", ret);
		debug_printf_dir("Dmt_SM2_Sign Test is Failed!\r\n");
        signal_err_times++;	
        #endif
	} else {
        s_reset_enc = 0;
	    #if DEBUG_ENC > 0
	    
		#if 1
		debug_printf_dir("The SM2 Sign result is(outlen:%d):\r\n", outlen);
		printf_hex_dir(outbuf, outlen);
        debug_printf_dir("\r\n");	

		#else
        int rt = 0;
		debug_printf_dir("The SM2 Sign result is(outlen:%d):\r\n", outlen);
		printf_hex_dir(outbuf, outlen); 
        debug_printf_dir("\r\n");
		//Enc_Wakeup();
        rt = nts_se_data_sm2_verify_with_za(dataptr, inlen, outbuf, outlen, Pub_PrikeyID[index * 2], idptr);
		if (rt != 0) {
            debug_printf_dir("The return code is: 0x%08X\r\n", rt);
			debug_printf_dir("Dmt_SM2_Verify is Failed!\r\n");
			debug_printf_dir("So Dmt_SM2_Sign Test is Failed!\r\n");
		} else {
			debug_printf_dir("Dmt_SM2_Verify is OK!\r\n");
			debug_printf_dir("Dmt_SM2_Sign is OK!\r\n");
		}
		#endif
		
        #endif

	}
    /* **************************** END  不同的加密芯片替换此部分代码 END ********************** */


END:
    /* 应答 */
    bal_WriteHWORD_Strm(wstrm, CLIENT_CODE);
    bal_WriteBYTE_Strm(wstrm, CMD_ENC);
    bal_WriteHWORD_Strm(wstrm, sn);
    bal_WriteBYTE_Strm(wstrm, ENC_TYPE_SIGN);

    if (ret != 0) {
        bal_WriteHWORD_Strm(wstrm, 1);
        bal_WriteBYTE_Strm(wstrm, ENC_FAILED);
    } else {
        bal_WriteHWORD_Strm(wstrm, outlen + 3);
        bal_WriteBYTE_Strm(wstrm, ENC_SUCCESS);
        bal_WriteHWORD_Strm(wstrm, outlen);
        bal_WriteDATA_Strm(wstrm, outbuf, outlen);
    }
    YX_COM_DirSend( CLIENT_FUNCTION_DOWN_REQ_ACK, bal_GetStrmStartPtr(wstrm), bal_GetStrmLen(wstrm));

    if (outbuf != NULL) {
		PORT_Free(outbuf);
	}
}

/*******************************************************************************
 **  函数名称:  YX_Encrypt_Encrypt
 **  功能描述:  加密明文
 **  输入参数:  [IN]	sn:	     流水号
                [IN]	*buffer: 数据指针
 **  		    [IN]	datelen: 数据长度
 **  返回参数:  无
 ******************************************************************************/
static void YX_Encrypt_Encrypt(INT16U sn, INT8U *buf, INT16U len)
{
    int ret = 0;
	INT8U  index = 0;
	INT16U inlen = 0;
	INT32U outlen = 0;
	//INT8U  outbuf[512];
    INT8U  *outbuf;
    STREAM_T *wstrm;
    wstrm = bal_STREAM_GetBufferStream();

    #if DEBUG_ENC > 0
	debug_printf_dir("\r\n加密明文  len:%d  buffer:", len);
    //printf_hex(buf, len);
    debug_printf_dir("\r\n");
    #endif

    index = buf[0];
    if (index > ALG_SM2_3) {
        #if DEBUG_ENC > 0
        debug_printf_dir("算法类型错误 %d!\r\n", index);
        #endif

        ret = 0xFFFF;
        goto END;
    }

    inlen = bal_chartoshort(&buf[1]);     /* 明文长度 */
    outbuf = PORT_Malloc(inlen + 96);
    if (outbuf == NULL) {
        ret = 0xFFFF;
        goto END;
    }
    //YX_MEMSET(outbuf, 0, sizeof(outbuf));
    #if DEBUG_ENC > 0
	debug_printf_dir("inlen: %d\r\n", inlen + 96);
    #endif

    /* ************************** START 不同的加密芯片替换此部分代码 START ******************** */
    ret = nts_se_data_sm2_encrypt(&buf[3], inlen, outbuf, &outlen, Pub_PrikeyID[index * 2]);
	if (ret != 0) {
		s_reset_enc++;
        #if DEBUG_ENC > 0
		debug_printf_dir("The return code is: 0x%08X\r\n", ret);
		debug_printf_dir("Dmt_SM2_Encrypt is Failed!\r\n");
        #endif
	} else {
	    s_reset_enc = 0;
        #if DEBUG_ENC > 0
		debug_printf_dir("The SM2 Encrypted cipher is(outlen:%d):\r\n", outlen);
		printf_hex_dir(outbuf, outlen);
        debug_printf_dir("\r\n");
		debug_printf_dir("Dmt_SM2_Encrypt is OK!\r\n");
        #endif
	}
    /* **************************** END  不同的加密芯片替换此部分代码 END ********************** */


END:
    /* 应答 */
    bal_WriteHWORD_Strm(wstrm, CLIENT_CODE);
    bal_WriteBYTE_Strm(wstrm, CMD_ENC);
    bal_WriteHWORD_Strm(wstrm, sn);
    bal_WriteBYTE_Strm(wstrm, ENC_TYPE_ENCRYPT);
    if (ret != 0) {
        bal_WriteHWORD_Strm(wstrm, 1);
        bal_WriteBYTE_Strm(wstrm, ENC_FAILED);
    } else {
        bal_WriteHWORD_Strm(wstrm, outlen + 3);
        bal_WriteBYTE_Strm(wstrm, ENC_SUCCESS);
        bal_WriteHWORD_Strm(wstrm, outlen);
        bal_WriteDATA_Strm(wstrm, outbuf, outlen);
    }
    YX_COM_DirSend( CLIENT_FUNCTION_DOWN_REQ_ACK, bal_GetStrmStartPtr(wstrm), bal_GetStrmLen(wstrm));

    if (outbuf != NULL) {
		PORT_Free(outbuf);
	}
}

/*******************************************************************************
 **  函数名称:  YX_Encrypt_Decrypt
 **  功能描述:  解密密文
 **  输入参数:  [IN]	sn:	     流水号
                [IN]	*buffer: 数据指针
 **  		    [IN]	datelen: 数据长度
 **  返回参数:  无
 ******************************************************************************/
static void YX_Encrypt_Decrypt(INT16U sn, INT8U *buf, INT16U len)
{
    int ret = 0;
	INT8U  index = 0;
	INT16U inlen = 0;
	INT32U outlen = 0;
    INT8U  *outbuf;
	//INT8U  outbuf[256];
    STREAM_T *wstrm;
    wstrm = bal_STREAM_GetBufferStream();

    #if DEBUG_ENC > 0
	debug_printf_dir("\r\n解密密文  len:%d  buffer:", len);
    //printf_hex(buf, len);
    debug_printf_dir("\r\n");
    #endif

    index = buf[0];
    if (index > ALG_SM2_3) {
        #if DEBUG_ENC > 0
        debug_printf_dir("算法类型错误 %d!\r\n", index);
        #endif

        ret = 0xFFFF;
        goto END;
    }


    inlen = bal_chartoshort(&buf[1]);     /* 密文长度 */
    if (inlen <= 96) {
        ret = 0xFFFF;
        goto END;
    }
    outbuf = PORT_Malloc(inlen - 96);
    if (outbuf == NULL) {
        ret = 0xFFFF;
        goto END;
    }
    //YX_MEMSET(outbuf, 0, sizeof(outbuf));
    #if DEBUG_ENC > 0
	debug_printf_dir("outlen: %d\r\n", inlen - 96);
    #endif

    /* ************************** START 不同的加密芯片替换此部分代码 START ******************** */
	ret = nts_se_data_sm2_decrypt(&buf[3], inlen, outbuf, &outlen, Pub_PrikeyID[index * 2 + 1]);
	if (ret != 0) {
		s_reset_enc++;
        #if DEBUG_ENC > 0
		debug_printf_dir("The return code is: 0x%08X\r\n", ret);
		debug_printf_dir("The SM2 Decrypt is Failed!\r\n");
        #endif
	} else {
	    s_reset_enc = 0;
        #if DEBUG_ENC > 0
		debug_printf_dir("The SM2 Decrypted msg is(outlen:%d):\r\n", outlen);
		printf_hex_dir(outbuf, outlen);
        debug_printf_dir("\r\n");
		debug_printf_dir("The SM2 Decrypt is OK!\r\n");
        #endif
	}
    /* **************************** END  不同的加密芯片替换此部分代码 END ********************** */


END:
    /* 应答 */
    bal_WriteHWORD_Strm(wstrm, CLIENT_CODE);
    bal_WriteBYTE_Strm(wstrm, CMD_ENC);
    bal_WriteHWORD_Strm(wstrm, sn);
    bal_WriteBYTE_Strm(wstrm, ENC_TYPE_DECRYPT);
    if (ret != 0) {
        bal_WriteHWORD_Strm(wstrm, 1);
        bal_WriteBYTE_Strm(wstrm, ENC_FAILED);
    } else {
        bal_WriteHWORD_Strm(wstrm, outlen + 3);
        bal_WriteBYTE_Strm(wstrm, ENC_SUCCESS);
        bal_WriteHWORD_Strm(wstrm, outlen);
        bal_WriteDATA_Strm(wstrm, outbuf, outlen);
    }
    YX_COM_DirSend( CLIENT_FUNCTION_DOWN_REQ_ACK, bal_GetStrmStartPtr(wstrm), bal_GetStrmLen(wstrm));

    if (outbuf != NULL) {
		PORT_Free(outbuf);
	}
}

/*******************************************************************************
 **  函数名称:  YX_Encrypt_Gen_Keypair
 **  功能描述:  生成公私钥对
 **  输入参数:  [IN]	sn:	     流水号
                [IN]	*buffer: 数据指针
 **  		    [IN]	datelen: 数据长度
 **  返回参数:  无
 ******************************************************************************/
static void YX_Encrypt_Gen_Keypair(INT16U sn, INT8U *buf, INT16U len)
{
    int ret = 0;
	INT8U  index = 0;
    STREAM_T *wstrm;
    INT8U pri_key_gen = 0;

    wstrm = bal_STREAM_GetBufferStream();

    #if DEBUG_ENC > 0
	debug_printf_dir("\r\n生成公私钥对  len:%d  buffer:", len);
    //printf_hex(buf, len);
    debug_printf_dir("\r\n");
    #endif

    index = buf[0];
    if (index > ALG_SM2_3) {
        #if DEBUG_ENC > 0
        debug_printf_dir("算法类型错误 %d!\r\n", index);
        #endif

        ret = 0xFFFF;
        goto END;
    }


    /* ************************** START 不同的加密芯片替换此部分代码 START ******************** */
    ret = nts_se_generate_sm2_key(Pub_PrikeyID[index * 2], &pri_key_gen); // 同一个ID只能生成一次
	if (ret != 0) {
		s_reset_enc++;
        #if DEBUG_ENC > 0
		debug_printf_dir("The SM2 Generate Keypair %d is Failed!\r\n", index);
		debug_printf_dir("The return code is: 0x%08X\r\n", ret);
        #endif
	} else {
	    s_reset_enc = 0;
	    #if DEBUG_ENC > 0
	    debug_printf_dir("The SM2 Generate Keypair %d is OK!\r\n", index);
        #endif
	}
    /* **************************** END  不同的加密芯片替换此部分代码 END ********************** */


END:
    /* 应答 */
    bal_WriteHWORD_Strm(wstrm, CLIENT_CODE);
    bal_WriteBYTE_Strm(wstrm, CMD_ENC);
    bal_WriteHWORD_Strm(wstrm, sn);
    bal_WriteBYTE_Strm(wstrm, ENC_TYPE_GEN_KEYPAIR);
    if (ret != 0) {
        bal_WriteHWORD_Strm(wstrm, 1);
        bal_WriteBYTE_Strm(wstrm, ENC_FAILED);
    } else {
        bal_WriteHWORD_Strm(wstrm, 1);
        bal_WriteBYTE_Strm(wstrm, ENC_SUCCESS);
    }
    YX_COM_DirSend( CLIENT_FUNCTION_DOWN_REQ_ACK, bal_GetStrmStartPtr(wstrm), bal_GetStrmLen(wstrm));
}

/*******************************************************************************
 **  函数名称:  YX_Encrypt_SET_CHIPID
 **  功能描述:  设置加密芯片ID
 **  输入参数:  [IN]	sn:	     流水号
                [IN]	*buffer: 数据指针
 **  		    [IN]	datelen: 数据长度
 **  返回参数:  无
 ******************************************************************************/
static void YX_Encrypt_SET_CHIPID(INT16U sn, INT8U *buf, INT16U len)
{
    int ret = 0;
    STREAM_T *wstrm;
	INT32U inlen = 0;

    wstrm = bal_STREAM_GetBufferStream();

    #if DEBUG_ENC > 0
	debug_printf_dir("\r\n设置加密芯片ID  len:%d  buffer:", len);
    //printf_hex(buf, len);
    debug_printf_dir("\r\n");
    #endif
    inlen = bal_chartoshort(&buf[0]);     /* 密文长度 */
    #if DEBUG_ENC > 0
    debug_printf_dir("inlen:%d\r\n", inlen);
    #endif

    
    /* ************************** START 不同的加密芯片替换此部分代码 START ******************** */
    ret = nts_se_write_se_id(&buf[2], inlen);
	if (ret != 0) {
		s_reset_enc++;
        #if DEBUG_ENC > 0
		debug_printf_dir("The SM2 Set ChipID is Failed!\r\n");
		debug_printf_dir("The return code is: 0x%08X\r\n", ret);
        #endif
	} else {
	    s_reset_enc = 0;
	    #if DEBUG_ENC > 0
	    debug_printf_dir("The SM2 Set ChipID is OK!\r\n");
        #endif
	}
    /* **************************** END  不同的加密芯片替换此部分代码 END ********************** */


    /* 应答 */
    bal_WriteHWORD_Strm(wstrm, CLIENT_CODE);
    bal_WriteBYTE_Strm(wstrm, CMD_ENC);
    bal_WriteHWORD_Strm(wstrm, sn);
    bal_WriteBYTE_Strm(wstrm, ENC_TYPE_SET_CHIPID);
    if (ret != 0) {
        bal_WriteHWORD_Strm(wstrm, 1);
        bal_WriteBYTE_Strm(wstrm, ENC_FAILED);
    } else {
        bal_WriteHWORD_Strm(wstrm, 1);
        bal_WriteBYTE_Strm(wstrm, ENC_SUCCESS);
    }
    YX_COM_DirSend( CLIENT_FUNCTION_DOWN_REQ_ACK, bal_GetStrmStartPtr(wstrm), bal_GetStrmLen(wstrm));
}

/*****************************************************************************
**  函数名:  YX_EncDataHdl
**  函数描述: 处理加密数据
**  参数:    [in] buffer : 数据内容
**  返回:    无
*****************************************************************************/
static void YX_EncDataHdl(INT8U* buffer)
{
    INT8U  type;
    INT16U sn, len;
    
    #if DEBUG_ENC > 1
    debug_printf_dir("\r\n<**** 处理函数 ****>\r\n");
    #endif

	if (nts_se_debug_level == 0) {
        Debug_Print_Sw(FALSE);
	}
	
    sn = bal_chartoshort(buffer);           /* 流水号 */
    type = buffer[2];                       /* 控制类型 */
    len  = bal_chartoshort(&buffer[3]);     /* 信号长度 */
    switch (type) {
        case ENC_TYPE_READ_CHIPID:          /* 获取加密芯片ID */
            YX_Encrypt_Read_ChipID(sn, &buffer[5], len);
            break;
        case ENC_TYPE_READ_PUBKEY:          /* 读取公钥 */
            YX_Encrypt_Read_Pubkey(sn, &buffer[5], len);
            break;
        case ENC_TYPE_HASH:                 /* 生成摘要 */
            YX_Encrypt_Hash(sn, &buffer[5], len);
            break;
        case ENC_TYPE_SIGN:                 /* 生成签名 */
            YX_Encrypt_Sign(sn, &buffer[5], len);
            break;
        case ENC_TYPE_ENCRYPT:              /* 加密明文 */
            YX_Encrypt_Encrypt(sn, &buffer[5], len);
            break;
        case ENC_TYPE_DECRYPT:              /* 解密密文 */
            YX_Encrypt_Decrypt(sn, &buffer[5], len);
            break;
        case ENC_TYPE_GEN_KEYPAIR:        /* 生成公私钥对 */
            YX_Encrypt_Gen_Keypair(sn, &buffer[5], len);
            break;
        case ENC_TYPE_SET_CHIPID:        /* 设置芯片ID */
            YX_Encrypt_SET_CHIPID(sn, &buffer[5], len);
            break;
        default:
            break;
    }
    Debug_Print_Sw(TRUE);
}

/*******************************************************************************
 **  函数名称:  YX_Encrypt_Func
 **  功能描述:  加密算法请求
 **  输入参数:  [in] buffer: 数据指针
                [in] len   : 长度
 **  返回参数:  无
 ******************************************************************************/
void YX_Encrypt_Func(INT8U *buffer, INT16U len)
{
    #if DEBUG_ENC > 1
    debug_printf_dir("\r\n<**** 主机下发数据,len:%d,sta:%d ****>\r\n",len, s_enc_hdl.is_busy);
	Debug_PrintHex(true,buffer,len);
	#endif
    if (s_enc_hdl.is_busy == FALSE) {
        if (len <= 1024) {
            s_enc_hdl.is_busy = TRUE;
            s_enc_hdl.step = STEP_PULLUP_WK_PIN;
            /* 先备份数据 */
            YX_MEMCPY(s_enc_hdl.data_buf, len, buffer, len);
            OS_StartTmr(s_encrypt_tmr2, MILTICK, 2);
        }
    } else {
        return;
    }
}

#if DEBUG_ENC >= 0
static void Test_Read_ChipID(void)
{
    INT8U data_buf[5];
    /* 2bytes 流水号 */
    data_buf[0] = 0x00;
    data_buf[1] = 0x00;
    /* 1byte 操作类型 */
    data_buf[2] = ENC_TYPE_READ_CHIPID;
    /* 2bytes 数据长度 */
    bal_shorttochar(&data_buf[3], 0);
    YX_Encrypt_Func(data_buf, 5);
    //YX_Encrypt_Read_ChipID(0, NULL, 0);
}

static void Test_Read_Pubkey(void)
{
    INT8U data_buf[6];
    /* 2bytes 流水号 */
    data_buf[0] = 0x00;
    data_buf[1] = 0x00;
    /* 1byte 操作类型 */
    data_buf[2] = ENC_TYPE_READ_PUBKEY;
    /* 2bytes 数据长度 */
    bal_shorttochar(&data_buf[3], 1);

    /* 数据内容 */
    data_buf[5] = ALG_SM2_2;
	
    YX_Encrypt_Func(data_buf, sizeof(data_buf));
}

static void Test_SM3_Hash(void)
{
    #if 0
	static const INT8U PDATA1_1[32] = {
		0xBF,0x47,0xD6,0x24,0x8F,0x03,0xF5,0xFB,0x5D,0x09,0x48,0x60,0x16,0xDC,0xE5,0x09,
        0xED,0x88,0x1C,0x37,0x16,0xE0,0x0F,0x0B,0x6A,0x4F,0xEE,0x4E,0x59,0x9A,0x94,0xAA
	};
	static const INT8U PDATA1_2[240] = {
		0xC6,0x3C,0x00,0xB7,0x3D,0xB8,0x47,0xD7,0x6D,0x42,0xD2,0xA2,0xCB,0x9B,0x65,0x47,
        0x65,0x96,0x9D,0xB6,0x5D,0x98,0x9D,0xA7,0x53,0x37,0xDC,0x29,0xFD,0x3A,0xFB,0xCF,
		0xD6,0x7C,0x8F,0xA4,0x7C,0xB0,0x9A,0xA5,0xF3,0x78,0x09,0x62,0xC4,0xD0,0xC1,0x3D,
	    0x86,0x91,0x43,0xBF,0xBE,0x81,0x14,0x17,0x15,0x66,0xD2,0x05,0x29,0xCB,0x95,0x27,
		0x26,0x96,0x53,0x0B,0xD4,0x1F,0x2B,0xD3,0x9C,0xC1,0x82,0x7C,0xC0,0x4F,0x98,0x07,
		0xF2,0x94,0xFB,0xE0,0xB9,0x38,0xDD,0x0B,0x61,0x1D,0xEA,0x89,0xE5,0x39,0x12,0xB1,
		0x1B,0xFB,0x74,0x15,0x50,0x24,0xD3,0xF0,0x92,0xD1,0xF3,0xD0,0xEB,0xB9,0x2E,0x67,
		0x60,0x18,0xA0,0x15,0xA4,0xB2,0xCD,0x41,0xFC,0x86,0x9D,0xD5,0x07,0x7C,0x06,0x5F,
		0xFE,0xF7,0x51,0xE1,0xB8,0x3B,0xDA,0x70,0xF7,0x80,0xC7,0x57,0x60,0x5B,0xA8,0x6A,
		0x02,0x51,0xFE,0x82,0xDB,0xAE,0x0E,0xE2,0xA9,0xF7,0x15,0x4C,0x6D,0xDD,0xBE,0x26,
		0x76,0xF3,0x87,0xB9,0xD0,0x71,0xB0,0x85,0x50,0x52,0xDE,0x04,0x61,0xF5,0x0F,0xAD,
		0xEE,0x3F,0xA5,0x9E,0xD9,0xB7,0xA6,0xEA,0x25,0xD2,0x53,0xFF,0xE5,0x3B,0x08,0x14,
		0xC8,0x62,0xB2,0xC2,0x42,0x37,0x7D,0xE4,0x5C,0x30,0x5A,0x61,0x26,0x63,0x47,0x5F,
		0xC3,0xDA,0x8D,0x6E,0x41,0xDB,0x10,0xAC,0xC7,0x9D,0x36,0x14,0x0A,0xF5,0xE4,0x5E,
		0x69,0x20,0xC8,0xDC,0xC1,0x08,0xEC,0x51,0xAC,0x00,0x3E,0xA6,0x0A,0xDF,0x31,0x6B
	};
	static const INT8U PDATA1_3[96] = {
		0x65,0xB0,0xCD,0x87,0x9A,0xD3,0x6E,0xE1,0x67,0x20,0xAB,0xA9,0xF6,0x12,0x8B,0xF7,
        0x37,0x95,0x6A,0xDF,0x7C,0xE2,0xBD,0xEE,0xF8,0x40,0xF8,0x7E,0xC8,0x57,0x83,0x1D,
		0x90,0xB7,0xEF,0xE0,0x23,0x1F,0xFE,0x42,0x96,0x12,0x07,0x9F,0xC3,0x8E,0x47,0xBC,
		0xC3,0x78,0x37,0xFF,0x58,0xAF,0x31,0xA7,0x1E,0xEE,0x65,0xCF,0x1C,0x12,0x2B,0x88,
		0x53,0x4F,0x0E,0xF5,0x69,0x08,0x12,0xCD,0x7E,0xB0,0xFE,0x5C,0xD4,0x4A,0xD9,0x50,
		0x1B,0x17,0x05,0xB8,0x24,0xC7,0x4E,0x60,0x5D,0x54,0x95,0x4D,0x3A,0x0D,0x69,0x0A
	};
	static const INT8U hash[32] =
	{
		0xDC,0x35,0xB0,0xD4,0x3E,0xFB,0x98,0x5B,0xAD,0x77,0x20,0x8B,0x7B,0xA2,0xED,0x5E,
        0x4E,0xD6,0xA4,0x23,0x03,0xAB,0x27,0x9B,0xEF,0x1E,0x83,0xFF,0xB2,0x71,0x38,0xF9
	};
	INT8U inputbuf[512];
    INT16U len = 0;

	YX_MEMSET(inputbuf, 0, sizeof(inputbuf));
    len = sizeof(PDATA1_1) + sizeof(PDATA1_2) + sizeof(PDATA1_3);
    inputbuf[0] = ALG_SM3_1;
    bal_shorttochar(&inputbuf[1], len);
	YX_MEMCPY(inputbuf + 3,                                         sizeof(PDATA1_1), (INT8U *)PDATA1_1, sizeof(PDATA1_1));
	YX_MEMCPY(inputbuf + 3 + sizeof(PDATA1_1),                      sizeof(PDATA1_2), (INT8U *)PDATA1_2, sizeof(PDATA1_2));
	YX_MEMCPY(inputbuf + 3 + sizeof(PDATA1_1) + sizeof(PDATA1_2),   sizeof(PDATA1_3), (INT8U *)PDATA1_3, sizeof(PDATA1_3));

    YX_Encrypt_Hash(0, inputbuf, 3 + len);
	#endif
}

static INT8U s_signbuf[600];
static void Test_SM2_Sign(void)
{
    INT16U idlen = 0, datalen = 0;
    INT8U id[16]  = {0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38};
    
    idlen = sizeof(id);
    datalen = 560;
    YX_MEMSET(s_signbuf, 0xA3, sizeof(s_signbuf));
    /* 2bytes 流水号 */
    s_signbuf[0] = 0x00;
    s_signbuf[1] = 0x00;
    /* 1byte 操作类型 */
    s_signbuf[2] = ENC_TYPE_SIGN;    /* 签名 */
    /* 2bytes 数据长度 */
    bal_shorttochar(&s_signbuf[3], 6 + idlen + datalen);
    
    /* 数据内容 */
    s_signbuf[5] = ALG_SM2_1;
    s_signbuf[6] = 0x01; /* 明文 */
	/* id长度 */
    bal_shorttochar(s_signbuf + 7, idlen);
	/* id内容 */
	YX_MEMCPY(s_signbuf + 9, idlen, (INT8U *)id, idlen);
	/* 明文长度 */
    bal_shorttochar(s_signbuf + 9 + idlen, datalen);
    /* 明文内容(已经填充0xA3) */
    /* 调用签名 */
    YX_Encrypt_Func(s_signbuf, 11 + idlen + datalen);
}

static void Test_SM2_Encrypt(void)
{
    INT8U data_buf[250];
	INT16U len = 0;

	INT8U msg[200] = {
		0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF,0x00,
        0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF,0x00
	};
	
	len = sizeof(msg);
    /* 2bytes 流水号 */
    data_buf[0] = 0x00;
    data_buf[1] = 0x00;
    /* 1byte 操作类型 */
    data_buf[2] = ENC_TYPE_ENCRYPT;
    /* 2bytes 数据长度 */
    bal_shorttochar(&data_buf[3], len + 3);

    /* 数据内容 */
    data_buf[5] = ALG_SM2_1;
    bal_shorttochar(&data_buf[6], len);
	YX_MEMCPY(&data_buf[8], len, (INT8U *)msg, len);	
	/* 加密 */
    YX_Encrypt_Func(data_buf, len + 8);	
}

static void Test_SM2_Decrypt(void)
{
    INT16U len = 0;
    /*
        注:由于每个测试流程都会重新生成秘钥，用固定的密文进行解密在秘钥改变后会解密失败，导致复位。所以测试解密时先执行加密，
        用加密后得到的密文再进行解密，保证这两个过程用的是同一组秘钥。
    */
    /* cipher是执行Test_SM2_Encrypt对msg进行加密生成的密文，对其解密明文若是msg，则解密正确 */
    const INT8U cipher[296] = {
        0x67,0x7F,0xBA,0x36,0x81,0xF3,0x99,0x6B,0x48,0x27,0x39,0x99,0x3A,0xF4,0x97,0xE3,
        0xF0,0x2A,0x0E,0xBC,0x03,0xC8,0xE2,0x5B,0x0C,0x72,0xFC,0xDB,0x4F,0xD9,0x38,0x8B,
        0x2E,0x1F,0x74,0x05,0xB8,0x25,0x70,0x01,0xFA,0xDF,0x3E,0x56,0xD6,0xAA,0x5E,0x30,
        0xE1,0x93,0x73,0x4C,0x75,0xBD,0x10,0xC3,0xAF,0x4A,0x3B,0x39,0xE6,0xD8,0x2E,0xF5,
        0xCB,0x4D,0xBD,0x79,0xFD,0x84,0x50,0x4C,0x41,0x27,0x33,0xD7,0x63,0x1E,0xA6,0x44,
        0x01,0xC0,0xFD,0x04,0x07,0x4D,0x14,0xCF,0xB7,0xB8,0x98,0x65,0x0B,0xA7,0xBA,0x0B,
        0x82,0x82,0x84,0x90,0x99,0x36,0x71,0xC1,0xA1,0x9F,0xF6,0x0D,0x5F,0x64,0xEF,0xAE,
        0x5D,0x71,0x43,0x52,0xC7,0x57,0xB1,0x70,0x4E,0xA3,0x5B,0x34,0x8C,0xBA,0x2A,0xC6,
        0xFC,0xBA,0x23,0x02,0x7F,0xCE,0x88,0x29,0xDA,0x09,0x37,0x63,0x08,0x63,0x9F,0xD2,
        0xDF,0xB5,0xCC,0x8A,0xD5,0x56,0x25,0xD4,0xF8,0xF7,0xB3,0x7F,0x0C,0x0B,0xD0,0x0F,
        0x21,0x82,0xA2,0x5E,0x7E,0x7E,0x11,0x28,0x4B,0x52,0xBD,0x9A,0xC7,0xC5,0xCC,0x7C,
        0xF5,0x1D,0xB0,0x39,0xEE,0x31,0xA0,0xA0,0x0D,0xF1,0x8D,0x29,0x43,0x3C,0xF2,0xA6,
        0x66,0x2A,0xEF,0x02,0x8F,0xFF,0x71,0xD8,0x49,0xF8,0xE2,0x9C,0x6B,0xFB,0x1B,0x12,
        0x40,0xBC,0x90,0x51,0x36,0x18,0xFC,0xAF,0x7E,0xCA,0xF6,0xA7,0x48,0xC7,0x57,0x71,
        0x25,0x0E,0x49,0xF5,0xDA,0xC0,0xC5,0x65,0xFF,0xDF,0x26,0x66,0x1E,0x1E,0x9D,0x0C,
        0x79,0xB3,0xA6,0x1A,0x0E,0x98,0xCC,0xD8,0x66,0x73,0x52,0x38,0x33,0x8D,0x8C,0x6E,
        0x38,0xA2,0x1E,0x34,0xF5,0x2F,0x90,0x3F,0x24,0x1A,0xDF,0xF8,0x02,0x64,0x22,0x60,
        0xCB,0x9F,0x4F,0xFB,0x67,0xBC,0x48,0x6F,0xC8,0x8E,0x36,0x5D,0x53,0x61,0x7F,0x42,
        0x1B,0x99,0x25,0xC5,0xA5,0xF4,0x6E,0x2E
	};
    
    INT8U data_buf[350];
	len = sizeof(cipher);
    /* 2bytes 流水号 */
    data_buf[0] = 0x00;
    data_buf[1] = 0x00;
    /* 1byte 操作类型 */
    data_buf[2] = ENC_TYPE_READ_PUBKEY;
    /* 2bytes 数据长度 */
    bal_shorttochar(&data_buf[3], len + 3);
    
    /* 数据内容 */
    data_buf[5] = ALG_SM2_1;
    bal_shorttochar(&data_buf[6], len);
	YX_MEMCPY(&data_buf[8], len, (INT8U *)cipher, len);	
    
    YX_Encrypt_Func(data_buf, len + 8);
}

static void Test_Gen_Keypair(void)
{
    INT8U data_buf[6];
    /* 2bytes 流水号 */
    data_buf[0] = 0x00;
    data_buf[1] = 0x00;
    /* 1byte 操作类型 */
    data_buf[2] = ENC_TYPE_GEN_KEYPAIR;
    /* 2bytes 数据长度 */
    bal_shorttochar(&data_buf[3], 1);
    
    /* 数据内容 */
    data_buf[5] = ALG_SM2_2;
    
    YX_Encrypt_Func(data_buf, sizeof(data_buf));
}

static void Test_Set_ChipID(void)
{
    INT8U data_buf[32];
	INT8U id[16]  = {'y','a','x','o','n','_','t','e','s','t','1','2','3','4','5','6'};

    /* 2bytes 流水号 */
    data_buf[0] = 0x00;
    data_buf[1] = 0x00;
    /* 1byte 操作类型 */
    data_buf[2] = ENC_TYPE_SET_CHIPID;
    /* 2bytes 数据长度 */
    bal_shorttochar(&data_buf[3], sizeof(id) + 2);
    
    /* 数据内容 */
    bal_shorttochar(&data_buf[5], sizeof(id));    /* id长度 */
    YX_MEMCPY(&data_buf[7], sizeof(id), (INT8U *)id, sizeof(id));    /* id内容 */
    
    YX_Encrypt_Func(data_buf,  sizeof(id) + 7);	
}

static void Test(INT8U Index)
{
    
    switch(Index) {
        case ENC_TYPE_READ_CHIPID:          /* 获取加密芯片ID */
            Test_Read_ChipID();
            break;
        case ENC_TYPE_READ_PUBKEY:          /* 读取公钥 */
            Test_Read_Pubkey();
            break;
        case ENC_TYPE_HASH:                 /* 生成摘要 */
            //Test_SM3_Hash();
            break;
        case ENC_TYPE_SIGN:                 /* 生成签名 */
            Test_SM2_Sign();
            break;  
        case ENC_TYPE_ENCRYPT:              /* 加密明文 */
            //Test_SM2_Encrypt();
            break;
        case ENC_TYPE_DECRYPT:              /* 解密密文 */
            //Test_SM2_Decrypt();
            break;
        case ENC_TYPE_GEN_KEYPAIR:          /* 生成公私钥对 */
            Test_Gen_Keypair();
            break;
        case ENC_TYPE_SET_CHIPID:           /* 设置芯片ID */
            //Test_Set_ChipID();
            break;
        default:
            break;
    }
}
#endif

static void YX_Encrypt_Tmr(void* para)
{
    static INT8U reset_delay = 0;
    static BOOLEAN flag_reset = FALSE;
	static INT32U err_cnt = 0;

	#if DEBUG_ENC > 0
    INT8U i;
    static INT8U  test_time = 0;
    static INT32U s_cnt = 0;
	static INT8U  test_time_start = 30;
    #endif

    if (s_reset_enc >= 3) {
        s_reset_enc = 0;
        if (flag_reset == FALSE) {
            flag_reset = TRUE;
            reset_delay = 5;          // 延时500ms
            Enc_PinEnOrDis(FALSE);
			err_cnt++;
		    #if DEBUG_ENC > 0
            debug_printf_dir("<关闭加密芯片通信脚，关闭电源>\r\n");
		    #endif			
        }
    }

    if (flag_reset == TRUE) {
        if (reset_delay) {
            --reset_delay;
            if (reset_delay == 0) {
                flag_reset = FALSE;
                Enc_PinEnOrDis(TRUE);
			    #if DEBUG_ENC > 0
                debug_printf_dir("<恢复加密芯片通信脚，打开电源>\r\n");
			    #endif			
            }
        }
    } 
	
    #if DEBUG_ENC > 0
    if (++test_time >= test_time_start) { // 3s
        test_time = 0;
        test_time_start = 10;  // 上电为3秒，上电之后会1秒
        debug_printf_dir("\r\n*************************** cnt:%d, err_cnt:%d ,signal err:%d *******************************\r\n", s_cnt++,err_cnt , signal_err_times);
        debug_printf_dir("(1)已使用动态内存大小:%d, 内存块数量:%d\r\n",PORT_GetDmSize(), PORT_GetDmBlocks());
        for (i = ENC_TYPE_READ_CHIPID; i < RC_TYPE_MAX; i++) {
		    #if DEBUG_ENC > 1
            Test(i);
		    #endif
        }
        debug_printf_dir("(2)已使用动态内存大小:%d, 内存块数量:%d\r\n",PORT_GetDmSize(), PORT_GetDmBlocks());	
    }
    #endif

}

static void YX_Encrypt_Tmr2(void* para)
{
    if (s_enc_hdl.step == STEP_PULLUP_WK_PIN) {
        Enc_WkPinCtl(TRUE);
        s_enc_hdl.step++;
	    #if DEBUG_ENC > 1
        debug_printf_dir("\r\n<**** pull up ****>\r\n");
	    #endif		
    } else if (s_enc_hdl.step == STEP_PULLDOWN_WK_PIN) {
        Enc_WkPinCtl(FALSE);
        s_enc_hdl.step++;
	    #if DEBUG_ENC > 1
        debug_printf_dir("\r\n<**** pull down ****>\r\n");
	    #endif			
    } else if (s_enc_hdl.step == STEP_HDL_ENC_DATA) {
	    #if DEBUG_ENC > 1
        debug_printf_dir("\r\n<**** hdl data ****>\r\n");
	    #endif			
        YX_EncDataHdl(s_enc_hdl.data_buf);
        s_enc_hdl.step++;
    } else {
	    #if DEBUG_ENC > 1
        debug_printf_dir("\r\n<**** stop ****>\r\n");
	    #endif		
        s_enc_hdl.is_busy = FALSE;
        OS_StopTmr(s_encrypt_tmr2);
    }

}

/*******************************************************************************
 **  函数名称:  YX_Encrypt_Init
 **  功能描述:  加密算法模块初始化
 **  输入参数:  [IN]	*buffer: 数据指针
 **  返回参数:  无
 ******************************************************************************/
void YX_Encrypt_Init(void)
{    
	YX_MEMSET((INT8U*)&s_enc_hdl, 0x00, sizeof(ENC_HDL_T));

    Enc_PinInit();
    yx_spi_init(0, 0);
    s_encrypt_tmr = OS_InstallTmr(TSK_ID_OPT, 0, YX_Encrypt_Tmr);
	OS_StartTmr(s_encrypt_tmr, MILTICK, 10);    /* 100ms */
	
    s_encrypt_tmr2 = OS_InstallTmr(TSK_ID_OPT, 0, YX_Encrypt_Tmr2);
	OS_StartTmr(s_encrypt_tmr2, MILTICK, 2);    /* 20ms */
}


