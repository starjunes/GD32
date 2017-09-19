/********************************************************************************
**
** 文件名:     at_cmd_phone.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现手机模块AT指令组帧和解析
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2014/06/15 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "yx_include.h"
#include "yx_misc.h"
#include "yx_stream.h"
#include "at_cmd_common.h"
#include "at_q_phone.h"
#include "at_socket_drv.h"

#if EN_AT > 0
#if EN_AT_PHONE > 0

/*
********************************************************************************
* Handler:  Common AT Commands
********************************************************************************
*/
static INT8U Handler_Common(INT8U *recvbuf, INT16U len)
{
    if (YX_SearchKeyWord(recvbuf, len, "OK")) {
        return AT_SUCCESS;
    } else {
        return AT_FAILURE;
    }
}

/*
********************************************************************************
* Handler:  ATD
********************************************************************************
*/
static INT8U Handler_AT_ATD(INT8U *recvbuf, INT16U len)
{
    if (YX_SearchKeyWord(recvbuf, len, "OK")                           /* SIM300C模块上电后如SIM卡未准备就绪，*/
    ||  YX_SearchKeyWord(recvbuf, len, "+CME ERROR: 769")) {           /* 则电话接通后返回+CME ERROR: 769 */
        return AT_SUCCESS;
    } else {
        return AT_FAILURE;
    }
}

/*
********************************************************************************
* Handler:  AT+CLCC ----- List current calls of ME
********************************************************************************
*/
static INT8U Handler_AT_CLCC(INT8U *recvbuf, INT16U len)
{
    INT8U temp1, temp2;
    
    if (ATCmdAck.numEC == 1) ATCmdAck.ackbuf[0] = FALSE;
    if (YX_SearchKeyWord(recvbuf, len, "OK")) {
        return AT_SUCCESS;
    } else {
        if (YX_SearchKeyWord(recvbuf, len, "+CLCC:")) {
            temp1 = YX_SearchDigitalString(recvbuf, len, ',', 2);
            temp2 = YX_SearchDigitalString(recvbuf, len, ',', 3);
            if (temp1 == 1 && temp2 == 4) ATCmdAck.ackbuf[0] = TRUE;
        }
        return AT_CONTINUE;
    }
}


/*********************************************************************************
**                                                                               *
**                                                                               *
**                 AT Commands parameters                                        *
**                                                                               *
**                                                                               *
*********************************************************************************/


AT_CMD_PARA_T const g_phone_hangup_para       = { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_phone_pickup_para       = { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_phone_ringup_para       = { 0,              120,  1,  1,  Handler_AT_ATD      };
AT_CMD_PARA_T const g_phone_ringup_data_para  = { 0,              120,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_phone_dtmf_para         = { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_phone_in_para           = { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_phone_out_para          = { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_phone_query_para        = { ATCMD_INSANT,     4,  1,  0,  Handler_AT_CLCC     };
AT_CMD_PARA_T const g_phone_setchannel_para   = { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_phone_setspeaker_para   = { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_phone_setmic_para       = { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_phone_setecho_para      = { 0,                4,  1,  1,  Handler_Common      };
AT_CMD_PARA_T const g_phone_setsidetone_para  = { 0,                4,  1,  1,  Handler_Common      };



/*
********************************************************************************
* ATH    Disconnect existing connection
********************************************************************************
*/
INT8U AT_CMD_Hangup(INT8U *dptr, INT32U maxlen)
{
    //char const str_ATH[] = {"ATH\r"};
    char const str_CHUP[] = {"AT+CHUP\r"};
    
    YX_MEMCPY(dptr, maxlen, str_CHUP, sizeof(str_CHUP) - 1);
    return sizeof(str_CHUP) - 1;
}

/*
********************************************************************************
* ATA    Answer a call
********************************************************************************
*/
INT8U AT_CMD_Pickup(INT8U *dptr, INT32U maxlen)
{
    char const str_ATA[] = {"ATA\r"};
    
    YX_MEMCPY(dptr, maxlen, str_ATA, sizeof(str_ATA) - 1);
    return sizeof(str_ATA) - 1;
}

/*
********************************************************************************
* ATD    Mobile originated call to dial a number
********************************************************************************
*/
INT8U AT_CMD_Ringup(INT8U *dptr, INT32U maxlen, BOOLEAN DataMode, INT8U  *tel, INT8U tellen)
{
    char const str_ATD[] = {"ATD"};
    
    YX_MEMCPY(dptr, maxlen, str_ATD, sizeof(str_ATD) - 1);
    dptr += sizeof(str_ATD) - 1;
    YX_MEMCPY(dptr, maxlen, tel, tellen);
    dptr += tellen;
    if (!DataMode) {
        *dptr++ = ';';
    }
    *dptr = '\r';
    return ((sizeof(str_ATD) - 1) + tellen + 2);
}

/*
********************************************************************************
* AT+VTS    DTMF and tone generation (<Tone> in {0-9, *, #, A, B, C, D})
********************************************************************************
*/
INT8U AT_CMD_SendDtmf(INT8U *dptr, INT32U maxlen, char dtmf)
{
    char const str_VTS[] = {"AT+VTS=\""}; 
    
    YX_MEMCPY(dptr, maxlen, str_VTS, sizeof(str_VTS) - 1);
    dptr   += sizeof(str_VTS) - 1;
    *dptr++ = dtmf;
    *dptr++ = '"';
    *dptr   = '\r';
    return ((sizeof(str_VTS) - 1) + 3);
}

/*
********************************************************************************
* AT+CLIP    Calling line identification presentation
********************************************************************************
*/
INT8U AT_CMD_SetIncomingDisplay(INT8U *dptr, INT32U maxlen)
{
    char const str_CLIP[] = {"AT+CLIP=1\r"};
    
    YX_MEMCPY(dptr, maxlen, str_CLIP, sizeof(str_CLIP) - 1);
    return sizeof(str_CLIP) - 1;
}

/*
********************************************************************************
* AT+CNMP    设置电信网络强制社会自成3G网络，因为4G网络短信和电话存在问题
********************************************************************************
*/
INT8U AT_CMD_Set_CTCC_Net3G(INT8U *dptr, INT32U maxlen)
{
    char const str_CNMP[] = {"AT+CNMP=22\r"};
    
    YX_MEMCPY(dptr, maxlen, str_CNMP, sizeof(str_CNMP) - 1);
    return sizeof(str_CNMP) - 1;
}

/*
********************************************************************************
* AT+CNMP    全网通如果在电信网络，需强制设置成3G网络，否则打电话和短信会有问题
********************************************************************************
*/
INT8U AT_CMD_Set_CTCC_3G_Net(INT8U *dptr, INT32U maxlen)
{
    char const str_CNMP[] = {"AT+CNMP=22\r"};
    
    YX_MEMCPY(dptr, maxlen, str_CNMP, sizeof(str_CNMP) - 1);
    return sizeof(str_CNMP) - 1;
}

/*
********************************************************************************
* AT+CLCC    List current calls of ME
********************************************************************************
*/
INT8U AT_CMD_QueryPhoneStatus(INT8U *dptr, INT32U maxlen)
{
    char const str_CLCC[] = {"AT+CLCC=1\r"};
    
    YX_MEMCPY(dptr, maxlen, str_CLCC, sizeof(str_CLCC) - 1);
    return sizeof(str_CLCC) - 1;
}

/*
********************************************************************************
* AT+COLP
********************************************************************************
*/
INT8U AT_CMD_SetOutgoingDisplay(INT8U *dptr, INT32U maxlen)
{
    char const str_COLP[] = {"AT+COLP=0\r"};
    
    YX_MEMCPY(dptr, maxlen, str_COLP, sizeof(str_COLP) - 1);
    return sizeof(str_COLP) - 1;
}

/*
********************************************************************************
* AT+CLVL
********************************************************************************
*/

INT8U AT_CMD_SetSpkLevel(INT8U *dptr, INT32U maxlen, INT8U ch, INT8U gain)
{
    STREAM_T wstrm;
    char const str_CLVL[] = {"AT+CLVL="};
    char const str_listen[] = {"AT+CLVL=0\r"};
    
    char const gain_headset[11][4] = {
                                        {"1\r"},  {"10\r"}, {"40\r"}, {"55\r"}, {"57\r"}
                                       ,{"60\r"}, {"65\r"}, {"75\r"}, {"80\r"}, {"90\r"}
                                       ,{"95\r"}};
    char const gain_free[11][4] = {
                                        {"1\r"},  {"10\r"}, {"15\r"}, {"20\r"}, {"22\r"}
                                       ,{"25\r"}, {"30\r"}, {"35\r"}, {"50\r"}, {"70\r"}
                                       ,{"95\r"}};
    char const gain_tts[11][4] = {
                                        {"1\r"},  {"10\r"}, {"15\r"}, {"20\r"}, {"22\r"}
                                       ,{"25\r"}, {"30\r"}, {"35\r"}, {"50\r"}, {"90\r"}
                                       ,{"95\r"}};
    char const gain_audio[11][4]   =  {
                                        {"1\r"},  {"10\r"}, {"15\r"}, {"20\r"}, {"22\r"}
                                       ,{"25\r"}, {"30\r"}, {"35\r"}, {"50\r"}, {"90\r"}
                                       ,{"95\r"}};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    switch (ch)
    {
    case AUDIO_CHANNEL_HEADSET:
        YX_WriteSTR_Strm(&wstrm, (char *)str_CLVL);
        YX_WriteSTR_Strm(&wstrm, (char *)(gain_headset[gain]));
        break;
    case AUDIO_CHANNEL_FREE:
        YX_WriteSTR_Strm(&wstrm, (char *)str_CLVL);
        YX_WriteSTR_Strm(&wstrm, (char *)(gain_free[gain]));
        break;
    case AUDIO_CHANNEL_LISTEN:
        YX_WriteSTR_Strm(&wstrm, (char *)str_listen);
        break;
    case AUDIO_CHANNEL_TTS:
        YX_WriteSTR_Strm(&wstrm, (char *)str_CLVL);
        YX_WriteSTR_Strm(&wstrm, (char *)(gain_tts[gain]));
        break;
    case AUDIO_CHANNEL_AUDIO:
        YX_WriteSTR_Strm(&wstrm, (char *)str_CLVL);
        YX_WriteSTR_Strm(&wstrm, (char *)(gain_audio[gain]));
        break;
    default:
        return 0;
    }
    return YX_GetStrmLen(&wstrm);
}

/*
********************************************************************************
* AT+CHFA, 
********************************************************************************
*/
INT8U AT_CMD_SelectChannel(INT8U *dptr, INT32U maxlen, INT8U ch)
{
    STREAM_T wstrm;
    //char const str_headset[] = {"AT+QAUDCH=0\r"};
    //char const str_free[]    = {"AT+QAUDCH=2\r"};
    
    char const str_headset[] = {"AT+CHFA=0\r"};
    char const str_free[]    = {"AT+CHFA=1\r"};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    switch (ch)
    {
    case AUDIO_CHANNEL_HEADSET:
        YX_WriteSTR_Strm(&wstrm, (char *)str_headset);
        break;
    case AUDIO_CHANNEL_FREE:
    case AUDIO_CHANNEL_LISTEN:
    case AUDIO_CHANNEL_TTS:
    case AUDIO_CHANNEL_AUDIO:
        YX_WriteSTR_Strm(&wstrm, (char *)str_free);
        break;
    default:
        break;
    }
    return YX_GetStrmLen(&wstrm);
}

/*
********************************************************************************
* AT_CMD_SetMicLevel
********************************************************************************
*/
INT8U AT_CMD_SetMicLevel(INT8U *dptr, INT32U maxlen, INT8U ch, INT8U gain)
{
    INT8U len;
    STREAM_T wstrm;
    
    //char const str_headset[] = {"AT+QMIC=0,"};
    //char const str_free[]    = {"AT+QMIC=2,"};
    
    char const str_headset[] = {"AT+CMIC=0,"};
    char const str_free[]    = {"AT+CMIC=1,"};
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    switch (ch)
    {
    case AUDIO_CHANNEL_HEADSET:
        gain += 2;
        YX_WriteSTR_Strm(&wstrm, (char *)str_headset);
        break;
    case AUDIO_CHANNEL_FREE:
        gain += 2;
        YX_WriteSTR_Strm(&wstrm, (char *)str_free);
        break;
    case AUDIO_CHANNEL_LISTEN:
        gain += 3;
        YX_WriteSTR_Strm(&wstrm, (char *)str_free);
        break;
    case AUDIO_CHANNEL_TTS:
        gain = 0;
        YX_WriteSTR_Strm(&wstrm, (char *)str_free);
        break;
    case AUDIO_CHANNEL_AUDIO:
        gain += 2;
        YX_WriteSTR_Strm(&wstrm, (char *)str_free);
        break;
    default:
        return 0;
    }
    len = YX_DecToAscii(YX_GetStrmPtr(&wstrm), gain, 0);
    YX_MovStrmPtr(&wstrm, len);
    YX_WriteSTR_Strm(&wstrm, "\r");
    
    return YX_GetStrmLen(&wstrm);
}

/*
********************************************************************************
* AT_CMD_SetEchoCancel
********************************************************************************
*/
INT8U AT_CMD_SetEchoCancel(INT8U *dptr, INT32U maxlen, INT8U ch, INT8U gain)
{
    STREAM_T wstrm;
    /* ch,nlp,aec,nr,ns,enable*/
    char const str_handset[] = {"AT+ECHO=0,0,0,0,0,0\r"};                 /* 关闭回波抵消 */
    char const str_free[]  = {"AT+ECHO=1,224,96,57351,5256,1\r"};         /* 开启回波抵消 */
    /*char const str_echo[] = {"AT+QECHO="};
    char const str_headset[11][22] = {
                                      {"221,1024,16388,849,0\r"}
                                     ,{"221,1024,16388,849,0\r"}
                                     ,{"221,1024,16388,849,0\r"}
                                     ,{"221,1024,16388,849,0\r"}
                                     ,{"221,1024,16388,849,0\r"}
                                     ,{"221,1024,16388,849,0\r"}
                                     ,{"221,1024,16388,849,0\r"}
                                     ,{"221,1024,16388,849,0\r"}
                                     ,{"221,1024,16388,849,0\r"}
                                     ,{"221,1024,16388,849,0\r"}
                                     ,{"221,1024,16388,849,0\r"}};
    char const str_free[11][22]    = {
                                      {"224,1024,5218,374,2\r"}
                                     ,{"224,1024,5218,374,2\r"}
                                     ,{"224,1024,5218,374,2\r"}
                                     ,{"224,1024,5218,374,2\r"}
                                     ,{"224,1024,5218,374,2\r"}
                                     ,{"224,1024,5218,374,2\r"}
                                     ,{"224,1024,5218,374,2\r"}
                                     ,{"224,1024,5218,374,2\r"}
                                     ,{"224,1024,5218,374,2\r"}
                                     ,{"224,1024,5218,374,2\r"}
                                     ,{"224,1024,5218,374,2\r"}};*/
    
    YX_InitStrm(&wstrm, dptr, maxlen);
    switch (ch)
    {
    case AUDIO_CHANNEL_HEADSET:
        YX_WriteSTR_Strm(&wstrm, (char *)str_handset);
        break;
    case AUDIO_CHANNEL_FREE:
    case AUDIO_CHANNEL_LISTEN:
    case AUDIO_CHANNEL_TTS:
    case AUDIO_CHANNEL_AUDIO:
        YX_WriteSTR_Strm(&wstrm, (char *)str_free);
        break;
    default:
        return 0;
    }
    return YX_GetStrmLen(&wstrm);
}

/*
********************************************************************************
* AT_CMD_SetSideTone
********************************************************************************
*/
INT8U AT_CMD_SetSideTone(INT8U *dptr, INT32U maxlen, INT8U ch, INT8U gain)
{
    STREAM_T wstrm;
    
    char const str_handset[]  = {"AT+SIDET=0,"};                           /* 开启侧音 */
    char const str_free[]     = {"AT+SIDET=1,"};                           /* 关闭侧音 */
    //char const str_SIDET[]  = {"AT+SIDET="};
    
    /*char const str_SIDET[]  = {"AT+QSIDET="};
    char const gain_headset[11][5] = {
                                        {"0\r"},  {"20\r"}, {"40\r"}, {"60\r"}, {"70\r"}
                                       ,{"80\r"}, {"90\r"}, {"100\r"}, {"120\r"}, {"150\r"}
                                       ,{"200\r"}};
    char const gain_free[11][5] = {
                                        {"0\r"}, {"20\r"}, {"40\r"}, {"60\r"}, {"70\r"}
                                       ,{"0\r"}, {"90\r"}, {"100\r"}, {"120\r"}, {"150\r"}
                                       ,{"200\r"}};*/
    char const gain_headset[11][5] = {
                                        {"0\r"},  {"2\r"}, {"4\r"}, {"6\r"}, {"7\r"}
                                       ,{"8\r"}, {"9\r"}, {"10\r"}, {"12\r"}, {"14\r"}
                                       ,{"15\r"}};
    char const gain_free[11][5] = {
                                        {"0\r"},  {"2\r"}, {"4\r"}, {"6\r"}, {"7\r"}
                                       ,{"0\r"}, {"9\r"}, {"10\r"}, {"12\r"}, {"14\r"}
                                       ,{"15\r"}};
                                       
    YX_InitStrm(&wstrm, dptr, maxlen);
    switch (ch)
    {
    case AUDIO_CHANNEL_HEADSET:
        YX_WriteSTR_Strm(&wstrm, (char *)str_handset);
        YX_WriteSTR_Strm(&wstrm, (char *)gain_headset[gain]);
        break;
    case AUDIO_CHANNEL_FREE:
    case AUDIO_CHANNEL_LISTEN:
        YX_WriteSTR_Strm(&wstrm, (char *)str_free);
        YX_WriteSTR_Strm(&wstrm, (char *)gain_free[gain]);
        break;
    default:
        break;
    }
    return YX_GetStrmLen(&wstrm);
}

#endif
#endif
