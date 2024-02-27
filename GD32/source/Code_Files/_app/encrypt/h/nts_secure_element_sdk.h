/******************************************************************************
**
** 文件名:     nts_secure_element_sdk.h
** 版权所有:   (c) 2005-2019 厦门雅迅网络股份有限公司
** 文件描述   : 华大加密芯片接口
** 创建人:  cym  ,  2019年5月5日 
**
 ******************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者    |  修改记录
**===============================================================================
**| 2019 /5/5 | cym |  创建该文件
*********************************************************************************/


#ifndef _NTS_SECURE_ELEMENT_SDK_H_
#define _NTS_SECURE_ELEMENT_SDK_H_

extern int nts_se_debug_level;
#define SE_TYPE_HED

#define PUBLIC_ID    0x0F
#define PRIVATE_ID   0x10
#define ASE_KEY_ID   0x04

//算法类型
enum aes_algorithm_type {
	ECB_128 = 0x01,
	ECB_192 = 0x02,
	ECB_256 = 0x03
};

/*****************************************************************************
**  函数名:  nts_se_data_sm2_encrypt
**  函数描述: 加密函数，针对小数据块
**  参数:       [in]data                :待加密数据
**              [in]data_len            :待加密数据长度
**              [out]encrypted_data     :加密后数据
**              [out]encrypted_data_len :加密后数据长度
**              [in]key_id              :密钥ID
**  返 回 : 0:成功，非0:失败
*****************************************************************************/
int nts_se_data_sm2_encrypt(unsigned char *data, unsigned int data_len, unsigned char *encrypted_data, unsigned int *encrypted_data_len, unsigned int key_id);

/*****************************************************************************
**  函数名:  nts_se_data_sm2_decrypt
**  函数描述: 解密函数，针对小数据块
**  参数:       [in]data                :待解密数据
**              [in]data_len            :待解密数据长度
**              [out]decrypted_data     :解密后数据
**              [out]decrypted_data_len :解密后数据长度
**              [in]key_id              :密钥ID
**  返 回 : 0:成功，非0:失败
*****************************************************************************/
int nts_se_data_sm2_decrypt (unsigned char *data, unsigned int data_len, unsigned char *decrypted_data, unsigned int *decrypted_data_len, unsigned int key_id);

/*****************************************************************************
**  函数名:  nts_data_aes_encrypt
**  函数描述: 加密函数，针对大数据块
**  参数:       [in]data                :待加密数据
**              [in]data_len            :待加密数据长度
**              [out]encrypted_data     :加密后数据
**              [out]encrypted_data_len :加密后数据长度
**              [in]algorithm_type      :算法类型：aes_algorithm_type
**              [in]key_id              :密钥ID
**  返 回 : 0:成功，非0:失败
*****************************************************************************/
int nts_data_aes_encrypt(unsigned char *data, unsigned int data_len, unsigned char *encrypted_data, unsigned int *encrypted_data_len, enum aes_algorithm_type algorithm_type, int key_id);

/*****************************************************************************
**  函数名:  nts_data_aes_decrypt
**  函数描述: 解密函数，针对大数据块
**  参数:       [in]data                :待解密数据
**              [in]data_len            :待解密数据长度
**              [out]decrypted_data     :解密后数据
**              [out]decrypted_data_len :解密后数据长度
**              [in]algorithm_type      :算法类型：aes_algorithm_type
**              [in]key_id              :密钥ID
**  返 回 : 0:成功，非0:失败
*****************************************************************************/
int nts_data_aes_decrypt (unsigned char *data, unsigned int data_len, unsigned char *decrypted_data, unsigned int *decrypted_data_len, enum aes_algorithm_type algorithm_type, int key_id);

/*****************************************************************************
**  函数名:  nts_se_data_sm2_signature
**  函数描述: 数据签名函数
**  参数:       [in]data               :待签名数据
**              [in]data_len           :待签名数据长度
**              [out]sign_data         :返回的签名数据
**              [out]sign_data_len     :返回的签名数据长度
**              [in]key_id             :密钥ID
**  返 回 : 0:成功，非0:失败
*****************************************************************************/
int nts_se_data_sm2_signature (unsigned char *data, unsigned int data_len, unsigned char *sign_data, unsigned int * sign_data_len, unsigned int key_id);

/*****************************************************************************
**  函数名:  nts_se_data_sm2_verify
**  函数描述: 数据验签函数
**  参数:       [in]data               :待签名数据
**              [in]data_len           :待签名数据长度
**              [in]sign_data          :返回的签名数据
**              [in]sign_data_len      :返回的签名数据长度
**              [in]key_id             :密钥ID
**  返 回 : 0:成功，非0:失败
*****************************************************************************/
int nts_se_data_sm2_verify (unsigned char *data, unsigned int data_len, unsigned char *sign_data, unsigned int sign_data_len, unsigned int key_id);

/*****************************************************************************
**  函数名:  nts_se_generate_sm2_key
**  函数描述: 生成sm2公密钥对
**  参数:       [in]pub_keyid          :要生成的公钥ID,范围:0x01~0xFE,设置过不能再次设置
**              [out]pri_keyid         :返回私钥ID，设置成功私钥ID将相对公钥ID号加1，如:公钥0x0f,私钥0x10
**  返 回 : 0:成功，非0:失败
*****************************************************************************/
int nts_se_generate_sm2_key(unsigned char pub_keyid, unsigned char *pri_keyid);

/*****************************************************************************
**  函数名:  nts_se_export_sm2_pubkey
**  函数描述: 输出公钥
**  参数:       [in]pub_keyid          :公钥ID
**              [out]pub_key           :输出的公钥ID字符串
**              [out]pub_key_len       :输出的公钥长度
**  返 回 : 0:成功，非0:失败
*****************************************************************************/
int nts_se_export_sm2_pubkey(unsigned char pub_keyid, unsigned char *pub_key, unsigned int * pub_key_len);

/*****************************************************************************
**  函数名:  nts_se_write_se_id
**  函数描述: 写入芯片ID号，16字节
**  参数:       [in]se_id          :加密芯片ID内容
**              [in]se_id_len      :加密芯片ID长度
**  返 回 : 0:成功，非0:失败
**  注意:芯片ID只能写一次
*****************************************************************************/
int nts_se_write_se_id(unsigned char *se_id, unsigned int se_id_len);

/*****************************************************************************
**  函数名:  nts_se_get_se_id
**  函数描述: 读取加密芯片ID号
**  参数:       [out]se_id         :加密芯片ID内容
**              [in]se_id_len      :加密芯片ID长度
**  返 回 : 0:成功，非0:失败
*****************************************************************************/
int nts_se_get_se_id(unsigned char *se_id, unsigned int *se_id_len);

/*****************************************************************************
**  函数名:  nts_se_data_sm2_signature_with_za
**  函数描述: 签名
**  参数:       [in]data               :待签名数据(明文)
**              [in]data_len           :待签名数据长度
**              [out]sign_data         :输出签名内容
**              [out]sign_data_len     :签名长度(64bytes)
**              [in]pri_keyid          :私钥ID号
**              [in]pub_keyid          :公钥ID号
**              [in]userid             :用户ID，默认16字节{'1','2','3','4','5','6','7','8','1','2','3','4','5','6','7','8'}
**  返 回 : 0:成功，非0:失败
*****************************************************************************/
int nts_se_data_sm2_signature_with_za(unsigned char * data, unsigned int data_len, unsigned char * sign_data, unsigned int * sign_data_len, unsigned int pri_key_id, unsigned int pub_key_id, unsigned char userid[16]);

/*****************************************************************************
**  函数名:  nts_se_data_sm2_verify_with_za
**  函数描述: 验签
**  参数:       [in]data               :待签名数据(明文)
**              [in]data_len           :待签名数据长度
**              [in]sign_data          :签名内容
**              [in]sign_data_len      :签名长度(64bytes)
**              [in]pub_keyid          :公钥ID号(要与)
**              [in]userid             :用户ID，默认16字节{'1','2','3','4','5','6','7','8','1','2','3','4','5','6','7','8'}
**  返 回 : 0:成功，非0:失败
*****************************************************************************/
int nts_se_data_sm2_verify_with_za(unsigned char * data, unsigned int data_len, unsigned char * sign_data, unsigned int sign_data_len, unsigned int pub_key_id, unsigned char userid[16]);

#endif

