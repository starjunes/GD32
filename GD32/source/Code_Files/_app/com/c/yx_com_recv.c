/*
********************************************************************************
** 文件名:     yx_com_recv.c
** 版权所有:   (c) 2017 厦门雅迅网络股份有限公司
** 文件描述:    协议数据处理接口(接收)
** 创建人：        谢金成，2017.5.18
********************************************************************************
*/
#include  "yx_includes.h"
#include  "yx_protocal_type.h"
#include  "yx_protocal_hdl.h"
#include  "yx_com_recv.h"
#include  "yx_com_send.h"
#include  "yx_com_man.h"
/******************************************************************************/
/*                           协议命令与处理函数关联结构体                                                                                                             */
/*********************************************************** ******************/
#define  PERIOD_SCAN               MILTICK,1              /* 10ms */
#define  SIZE_RECVBUF              1536



typedef struct {
    INT8U  command;                                       /* 命令编码    */
    void   (*hdl)(INT8U,INT8U,INT8U *, INT16U);           /* 指向接口函数指针     */
}PROTOHDL_REG_S;


typedef struct {
    INT8U  chksum;                               /* 校验码                        */
    INT8U  version;                              /* 协议版本                  */
    INT8U  devcode;                              /* 外设编码                  */
   // INT8U  devid;                                /* 外设序号                  */
    INT8U  command;                              /* 命令                              */
} RECV_FRAME_HEAD_S;


/******************************************************************************/
/*                           注册接口函数                                                                                                                                                */
/******************************************************************************/

#ifdef   PROTOHDL_REG_DEF
#undef   PROTOHDL_REG_DEF
#endif

#define  PROTOHDL_REG_DEF(_COMMAND_,_HDL)  {_COMMAND_,_HDL},

 static  const PROTOHDL_REG_S  s_protohdl_reg_tbl[] = {
        #include "yx_protohdl_reg.def"
        {0,0}
 };

/******************************************************************************/
/*                           定义相关变量                                                                                                                                                */
/******************************************************************************/
 static INT8U   s_recvbuf[SIZE_RECVBUF];
 static INT16S  s_recvlen = -1;
 static INT8U   s_scantmrid;                              /* 扫描定时器 */
 static ASMRULE_T  s_rules = {0x7e, 0x7d, 0x02, 0x01};    /* 转义规则   */

/*******************************************************************************
**  函数名称:  ParseRecvProtocolData
**  功能描述:  分析接收自主串口的协议数据，通过处理函数注册表跳到相对应的处理函数
**  输入参数:  data     : 指向接收到的协议数据
**          :  datalen  : 协议数据长度
**  返回参数:  None
*******************************************************************************/
static void ParseRecvProtocolData(INT8U *data, INT16U datalen)
{
    INT8U  i;
    INT8U  nums;
    INT8U *userdata;
    INT16U userdatalen;

    RECV_FRAME_HEAD_S *recvhead = (RECV_FRAME_HEAD_S *)data;

    #if DEBUG_COM_REC > 0
    debug_printf("ParseRecvProtocolData data:");              /* 打印信息 */
    printf_hex(data,datalen > 30 ? 30 : datalen);
    debug_printf("\r\n");              /* 打印信息 */
    #endif
		if (recvhead->devcode != DEV_CODE) return;

    nums        = sizeof(s_protohdl_reg_tbl) / sizeof(s_protohdl_reg_tbl[0]) -1;
    userdata    = data + sizeof(RECV_FRAME_HEAD_S);
    userdatalen = datalen - sizeof(RECV_FRAME_HEAD_S);
    for (i = 0; i < nums; i++) {
        if (s_protohdl_reg_tbl[i].command == recvhead->command) {
            if (s_protohdl_reg_tbl[i].hdl != 0) {
                s_protohdl_reg_tbl[i].hdl(recvhead->version, recvhead->command, userdata, userdatalen);
            }
            return;
        }
    }

}

#if 0
/*******************************************************************************
**  函数名称:  ScanTmrProc
**  功能描述:  定时扫描入口函数
**  输入参数:  None
**  返回参数:  None
*******************************************************************************/
static INT16S FindFrameHead(INT8U *pdata, INT16U datalen)
{
    INT16U findpos, dlen;
    INT8U *dataptr;

    dataptr = pdata;
    dlen = datalen;
    while (dlen > 0) {
        findpos = bal_FindCharPos(dataptr, 0xaa, 0, dlen);
        if (findpos == dlen - 1) return datalen - 1;
        if (findpos == dlen) return -1;
        if (*(dataptr + findpos + 1) == 0x75) return findpos + (dataptr - pdata);
        dataptr += findpos + 1;
        dlen -= findpos + 1;
    }
    return -1;
}

// 返回FALSE表示需要退出循环解析
static BOOLEAN HandleFrame(void)
{
    INT16U  datalen;
    INT16S findpos;

    if (s_recvlen >= 7) {
    	datalen = ((s_recvbuf[5] << 8) + s_recvbuf[6]);
        if ((datalen + 7) > SIZE_RECVBUF) {                   // 总长超出缓存
            s_recvlen -= 2;
            findpos = FindFrameHead(s_recvbuf + 2, s_recvlen);
            if (findpos != -1) {
                s_recvlen -= findpos;
                memmove(s_recvbuf, s_recvbuf + 2 + findpos, s_recvlen);
                return true;
            } else {
                s_recvlen = 0;
                return FALSE;
            }
        }
        if (s_recvlen >= (datalen + 7)) {                     // 接收完成
            if ((s_recvbuf[2] ==  bal_ChkSum_Xor(s_recvbuf + 4, datalen + 3)) && (s_recvbuf[3] == DEVICETYPE)) {
                ParseRecvProtocolData(s_recvbuf + 4, datalen + 3);
                s_recvlen -= datalen + 7;
                if(s_recvlen != 0) {
                	memmove(s_recvbuf, s_recvbuf + datalen + 7, s_recvlen);
                }
            } else {
                s_recvlen -= 2;
                findpos = FindFrameHead(s_recvbuf + 2, s_recvlen);
                if (findpos != -1) {
                    s_recvlen -= findpos;
                    memmove(s_recvbuf, s_recvbuf + 2 + findpos, s_recvlen);
                }
            }
        } else {
            return FALSE;
        }
    }
    return TRUE;
}
#endif


static void ScanTmrProc(void* pdata)
{
    INT16S  curchar;
    //BOOLEAN ret;
    while ((curchar = PORT_UartRead(MAIN_COM)) != -1) {
        if (s_recvlen == -1) {
            if (curchar == 0x7e) {
                s_recvlen = 0;
            }
            #if DEBUG_COM_REC > 1
            debug_printf("receive: %02x ", curchar);              /* 打印信息 */
            #endif
        } else {
             #if DEBUG_COM_REC > 1
             debug_printf("%02x ", curchar);                     /* 打印信息 */
             #endif
             if (curchar == 0x7e) {
                #if DEBUG_COM_REC > 1
                debug_printf("\r\n", curchar);                     /* 打印信息 */
                #endif
                if (s_recvlen > 0) {
                    #if DEBUG_CAN_OTA > 1
                    if (DATA_SEQ_TRANSF_CAN == s_recvbuf[3]) {
                        //debug_printf("s_recvbuf %d:", s_recvlen);
                        //printf_hex_dir(s_recvbuf, s_recvlen);
                        //debug_printf("\r\n");
                        //printf_hex(&s_recvbuf[256], 30);
                        //debug_printf("\r\n");
                    }
                    #endif

                    s_recvlen = bal_DeassembleByRules(s_recvbuf,s_recvbuf, (INT16U)s_recvlen, &s_rules);
                    if (s_recvbuf[0] == bal_GetChkSum(&s_recvbuf[1], s_recvlen - 1)) {
                        if (s_recvlen >= 2) {
                            ParseRecvProtocolData(s_recvbuf, s_recvlen);
                        }
                    } else {
                        #if DEBUG_CAN_OTA > 1
                        debug_printf("ChkSumErr\r\n", curchar);                     /* 打印信息 */
                        #endif
                    }
                    s_recvlen = -1;
                }
            } else {
                if (s_recvlen >= SIZE_RECVBUF) {
                    s_recvlen = -1;
                    #if DEBUG_CAN_OTA > 0
                    debug_printf("s_recvlen >= SIZE_RECVBUF\r\n");                     /* 打印信息 */
                    #endif
                } else {
                    s_recvbuf[s_recvlen++] = (INT8U)curchar;
                }
            }
        }
	}
}

/*******************************************************************************
**  函数名称:  YX_ComRecv_Init
**  功能描述:  初始化接收协议数据模块
**  输入参数:  None
**  返回参数:  None
*******************************************************************************/
void YX_ComRecv_Init(void)
{
	s_scantmrid = OS_InstallTmr(TSK_ID_OPT, 0, ScanTmrProc);
	OS_StartTmr(s_scantmrid, PERIOD_SCAN);
}

/************************ (C) COPYRIGHT 2010 XIAMEN YAXON.LTD *****************/
 /*****END OF FILE******/
