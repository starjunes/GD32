/********************************************************************************
**
** 文件名:     at_urc_comon.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现手机模块普通消息接收解析处理
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
#include "yx_dym_drv.h"
#include "at_com.h"
#include "at_recv.h"
#include "at_send.h"
#include "at_urc_common.h"

#if EN_AT > 0

/*
********************************************************************************
* 定义模块配置参数
********************************************************************************
*/

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/


/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static BOOLEAN s_6320C_less_B05_ver = FALSE;



/*
********************************************************************************
* HANDLER: +CPBR:
********************************************************************************
*/
static INT8U Handler_CPBR(INT8U ct_recv, INT8U event, INT8U *sptr, INT16U slen)
{
#if 0
    INT8U index, tellen, textlen;
    INT8U  *tel,  *text;

    tel     = &s_recvbuf[0];
    tellen  = YX_SearchString(tel, sizeof(s_recvbuf), sptr, slen, '"', 1);
    text    = &s_recvbuf[tellen];
    textlen = YX_SearchString(text, sizeof(s_recvbuf) - tellen, sptr, slen, '"', 2);
    index   = YX_SearchDigitalString(sptr, slen, ',', 1);
    HdlPhoneBookRec(index, tel, tellen, text, textlen);
    return AT_SUCCESS;
#endif
    return AT_SUCCESS;
}

#if 0
/*
********************************************************************************
* HANDLER: +CTTS
********************************************************************************
*/
void ADP_TTS_InformPlayOver(void);
static INT8U Handler_TTSStatus(INT8U ct_recv, INT8U event, INT8U *sptr, INT16U slen)
{
    if (YX_SearchDigitalString(sptr, slen, '\r', 1) == 0) {
        ADP_TTS_InformPlayOver();
    }
    return AT_SUCCESS;
}

/*
********************************************************************************
* HANDLER: +QADC
********************************************************************************
*/
static INT8U Handler_ADC(INT8U ct_recv, INT8U event, INT8U *sptr, INT16U slen)
{
    if (s_rcb.n_recv <= 1) {
        return RET_CONTINUE;
    } else {
        return AT_SUCCESS;
    }
}

/*
********************************************************************************
* HANDLER: +QENG
********************************************************************************
*/

static RF_CELL_INFO_T s_cell;
void GetCellInfo(RF_CELL_INFO_T *cell)
{
    YX_MEMCPY(cell, &s_cell, sizeof(s_cell));
}

static INT16U ReadValue(INT8U **ptr, INT16U *len)
{
    INT8U *sptr;
    INT16U value, slen, pos;
    
    sptr = *ptr;
    slen = *len;
    value = YX_SearchDigitalString(sptr, *len, ',', 1);
    pos = YX_FindCharPos(sptr, ',', 0, *len) + 1;
    *ptr = sptr + pos;
    *len = slen - pos;
    
    if (*sptr == 'x') {
        return 0;
    } else if (*sptr == '-') {
        return (-value);
    } else {
        return value;
    }
}

static INT16U ReadHexValue(INT8U **ptr, INT16U *len)
{
    INT8U *sptr;
    INT16U value, slen, pos;
    
    sptr = *ptr;
    slen = *len;
    value = YX_SearchHexString(sptr, *len, ',', 1);
    pos = YX_FindCharPos(sptr, ',', 0, *len) + 1;
    *ptr = sptr + pos;
    *len = slen - pos;
    
    if (*sptr == 'x') {
        return 0;
    } else if (*sptr == '-') {
        return (-value);
    } else {
        return value;
    }
}

static INT8U Handler_QENG(INT8U event, INT8U  *sptr, INT16U slen)
{
    INT8U *ptr;
    INT8U i, index, mode, dump;
    INT16U len;

    /*  实例解析
    [15:24:26] AT_RECV(0):AT+QENG?
    [15:24:26] AT_RECV(0):+QENG: 1,0
    [15:24:27] AT_RECV(0):+QENG: 0,460,00,592f,2976,520,44,-78,45,69,0,20,x,x,x,x,x,x,x
    [15:24:27] AT_RECV(0):OK
    
    AT_RECV(0):+QENG: 0,460,00,592f,66fa,65,25,-76,85,85,5,12,x,x,x,x,x,x,x
<Handler_QENG,mcc:(460) mnc:(0) lac:(0x592f) cellid:(65535) bcch:(65) bsic:(25) dbm:(76) c1:(85) c2:(85) 
txp:(5) rla:(12) tch:(-1) ts:(65535) maio:(-1) hsn:(-1) ta:(65535) rxq_sub:(65535) rxq_full:(65535)> 
    */
    
    ptr = sptr;
    len = slen;
    if (len < 20) {
        mode = YX_SearchDigitalString(sptr, len, ',', 1);
        dump = YX_SearchDigitalString(sptr, len, '\r', 1);
        if (mode != 1 || dump != 1) {
            SetQeng(true, 0);
        }
        #if DEBUG_AT > 0
        printf_com("<Handler_QENG(%d)(%d)>\r\n", mode, dump);
        #endif
    } else {
        index = ReadValue(&ptr, &len);
        if (index == 0) {
            YX_MEMSET(&s_cell, 0, sizeof(s_cell));
            s_cell.num = 1;
            s_cell.cell[index].cmcc_cell.mcc      = ReadValue(&ptr, &len);
            s_cell.cell[index].cmcc_cell.mnc      = ReadValue(&ptr, &len);
            s_cell.cell[index].cmcc_cell.lac      = ReadHexValue(&ptr, &len);
            s_cell.cell[index].cmcc_cell.cellid   = ReadHexValue(&ptr, &len);
            s_cell.cell[index].cmcc_cell.bcch     = ReadValue(&ptr, &len);
            s_cell.cell[index].cmcc_cell.bsic     = ReadValue(&ptr, &len);
            s_cell.cell[index].cmcc_cell.dbm      = ReadValue(&ptr, &len);
            s_cell.cell[index].cmcc_cell.c1       = ReadValue(&ptr, &len);
            s_cell.cell[index].cmcc_cell.c2       = ReadValue(&ptr, &len);
            s_cell.cell[index].cmcc_cell.txp      = ReadValue(&ptr, &len);
            s_cell.cell[index].cmcc_cell.rla      = ReadValue(&ptr, &len);
            s_cell.cell[index].cmcc_cell.tch      = ReadValue(&ptr, &len);//X
            s_cell.cell[index].cmcc_cell.ts       = ReadValue(&ptr, &len);
            s_cell.cell[index].cmcc_cell.maio     = ReadValue(&ptr, &len);
            s_cell.cell[index].cmcc_cell.hsn      = ReadValue(&ptr, &len);
            s_cell.cell[index].cmcc_cell.ta       = ReadValue(&ptr, &len);
            s_cell.cell[index].cmcc_cell.rxq_sub  = ReadValue(&ptr, &len);
            s_cell.cell[index].cmcc_cell.rxq_full = YX_SearchDigitalString(ptr, len, '\r', 1);
            #if DEBUG_AT > 0
            printf_com("<Handler_QENG mcc(%d),mnc(%d),lac(0x%x),cellid(0x%x),bcch(%d),bsic(%d),dbm(%d),c1(%d),c2(%d),txp(%d),rla(%d),tch(%d),ts(%d),maio(%d),hsn(%d),ta(%d),rxq_sub(%d),rxq_full(%d)>\r\n", 
                                        s_cell.cell[index].cmcc_cell.mcc,
                                        s_cell.cell[index].cmcc_cell.mnc,
                                        s_cell.cell[index].cmcc_cell.lac,
                                        s_cell.cell[index].cmcc_cell.cellid,
                                        s_cell.cell[index].cmcc_cell.bcch,
                                        s_cell.cell[index].cmcc_cell.bsic,
                                        s_cell.cell[index].cmcc_cell.dbm,
                                        s_cell.cell[index].cmcc_cell.c1,
                                        s_cell.cell[index].cmcc_cell.c2,
                                        s_cell.cell[index].cmcc_cell.txp,
                                        s_cell.cell[index].cmcc_cell.rla,
                                        s_cell.cell[index].cmcc_cell.tch,
                                        s_cell.cell[index].cmcc_cell.ts,
                                        s_cell.cell[index].cmcc_cell.maio,
                                        s_cell.cell[index].cmcc_cell.hsn,
                                        s_cell.cell[index].cmcc_cell.ta,
                                        s_cell.cell[index].cmcc_cell.rxq_sub,
                                        s_cell.cell[index].cmcc_cell.rxq_full);
            #endif
        } else if (index == 1) {
            s_cell.num = 7;
            for (i = 0; i < 6; i++) {
                index = ReadValue(&ptr, &len);
                s_cell.cell[index].cmcc_cell.bcch       = ReadValue(&ptr, &len);
                s_cell.cell[index].cmcc_cell.dbm        = ReadValue(&ptr, &len);
                s_cell.cell[index].cmcc_cell.bsic       = ReadValue(&ptr, &len);
                s_cell.cell[index].cmcc_cell.c1         = ReadValue(&ptr, &len);
                s_cell.cell[index].cmcc_cell.c2         = ReadValue(&ptr, &len);
                s_cell.cell[index].cmcc_cell.mcc        = ReadValue(&ptr, &len);
                s_cell.cell[index].cmcc_cell.mnc        = ReadValue(&ptr, &len);
                s_cell.cell[index].cmcc_cell.lac        = ReadHexValue(&ptr, &len);
                if (i != 5) {
                    s_cell.cell[index].cmcc_cell.cellid = ReadHexValue(&ptr, &len);
                } else {
                    s_cell.cell[index].cmcc_cell.cellid = YX_SearchHexString(ptr, len, '\r', 1);
                }
            
                #if DEBUG_AT > 0
                printf_com("<Handler_QENG mcc(%d),mnc(%d),lac(0x%x),cellid(0x%x),bcch(%d),bsic(%d),dbm(%d),c1(%d),c2(%d),txp(%d),rla(%d),tch(%d),ts(%d),maio(%d),hsn(%d),ta(%d),rxq_sub(%d),rxq_full(%d)>\r\n", 
                                        s_cell.cell[index].cmcc_cell.mcc,
                                        s_cell.cell[index].cmcc_cell.mnc,
                                        s_cell.cell[index].cmcc_cell.lac,
                                        s_cell.cell[index].cmcc_cell.cellid,
                                        s_cell.cell[index].cmcc_cell.bcch,
                                        s_cell.cell[index].cmcc_cell.bsic,
                                        s_cell.cell[index].cmcc_cell.dbm,
                                        s_cell.cell[index].cmcc_cell.c1,
                                        s_cell.cell[index].cmcc_cell.c2,
                                        s_cell.cell[index].cmcc_cell.txp,
                                        s_cell.cell[index].cmcc_cell.rla,
                                        s_cell.cell[index].cmcc_cell.tch,
                                        s_cell.cell[index].cmcc_cell.ts,
                                        s_cell.cell[index].cmcc_cell.maio,
                                        s_cell.cell[index].cmcc_cell.hsn,
                                        s_cell.cell[index].cmcc_cell.ta,
                                        s_cell.cell[index].cmcc_cell.rxq_sub,
                                        s_cell.cell[index].cmcc_cell.rxq_full);
                #endif
            }
        }
    }
    return AT_SUCCESS;
}

/*
********************************************************************************
* HANDLER: +CPIN: SIM PIN
********************************************************************************
*/
static INT8U Handler_SIMPIN(INT8U ct_recv, INT8U event, INT8U *sptr, INT16U slen)
{
    ADP_NET_InformSimPinCodeStatus(true);
    return AT_SUCCESS;
}
#endif
/*
********************************************************************************
* HANDLER: +CPIN: READY
********************************************************************************
*/
static INT8U Handler_SIMREADY(INT8U ct_recv, INT8U event, INT8U *sptr, INT16U slen)
{
    //ADP_NET_InformSimPinCodeStatus(false);
    return AT_SUCCESS;
}

/*
********************************************************************************
* Handler: CGMR
********************************************************************************
*/

static INT8U Handler_CGMR(INT8U ct_recv, INT8U event, INT8U *sptr, INT16U slen)
{
    INT8U pos;

    s_6320C_less_B05_ver = FALSE;
    
    if (YX_SearchKeyWord(sptr, slen, "ERROR")) {
        return AT_FAILURE;
    }

    pos = YX_FindCharPos(sptr, 'B', 0, slen);
    #if EN_DEBUG > 0
    printf_com("Handler_CGMR1 (slen= %d)(%s)\r\n", slen, sptr);
    printf_com("Handler_CGMR (slen= %d)(pos=%d)(%c)\r\n", slen, pos, sptr[pos+2]);
    #endif
    if ((pos != 0) && (sptr[pos+1] == '0') && (sptr[pos+2] < '5')) {
        s_6320C_less_B05_ver = TRUE;
    }

    return AT_SUCCESS;
}

/*
********************************************************************************
* define receive control block
********************************************************************************
*/
static URC_HDL_TBL_T const s_hdl_tbl[] = {
     {"START",                2,   true,   0}
    ,{"+STIN:",               2,   true,   0}
    ,{"PB DONE",              2,   true,   0}
    ,{"SMS DONE",             2,   true,   0}
    
    //{"+QADC",                2,  true,    Handler_ADC}
    //{"AT+QADC?",             2,   true,   Handler_ADC}
    //{"+QENG:",               2,   true,   Handler_QENG}
                  
    //{"+CPIN: SIM PIN",       2,   true,   Handler_SIMPIN}
    ,{"+CPIN: READY",         2,   true,   Handler_SIMREADY}
    //{"+CTTS:",               2,   true,   Handler_TTSStatus}
    ,{"+CPBR:",               2,   true,   Handler_CPBR}
    ,{"+CGMR:",               2,   true,   Handler_CGMR}
};


/*******************************************************************
** 函数名:     AT_URC_InitCommon
** 函数描述:   初始化函数
** 参数:       无
** 返回:       无
********************************************************************/
void AT_URC_InitCommon(void)
{
    INT8U i;
    
    for (i = 0; i < sizeof(s_hdl_tbl) / sizeof(s_hdl_tbl[0]); i++) {
        AT_RECV_RegistUrcHandler((URC_HDL_TBL_T *)&s_hdl_tbl[i]);
    }
}

/*******************************************************************
** 函数名:     ADP_URC_Is6320CLessB05Ver
** 函数描述:   是否是6320C模块且版本是小于B05的版本
** 参数:       无
** 返回:       小于B05的版本返回TRUE， 否则返回FALSE
********************************************************************/
BOOLEAN ADP_URC_Is6320CLessB05Ver(void)
{
    return s_6320C_less_B05_ver;
}

#endif
