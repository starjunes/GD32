/*
********************************************************************************
** 文件名:     yx_exuart_man.c
** 版权所有:   (c) 2017 厦门雅迅网络股份有限公司
** 文件描述:    物理扩展串口应用层程序接口
** 创建人：        谢金成，2017.7.13
********************************************************************************
*/
#include "yx_includes.h"
#include "yx_exuart_man.h"
#include "yx_com_man.h"
#include "yx_com_send.h"
#include "port_uart.h"
#include "port_gpio.h"

/******************************************************************************/
/*                           定时器周期                                       */
/******************************************************************************/
#define  DELAY_TICKS          1
#define  PERIOD_SCAN          MILTICK, 1
#define  PERIOD_DELAY         MILTICK, DELAY_TICKS

/******************************************************************************/
/*                           定义数据缓冲区宏                                 */
/******************************************************************************/
#define  SIZE_RECVBUF_PHY1    (1200 * 1)                       /* 接收缓冲大小*/
#if EN_DEBUG > 0
#define  SIZE_SENDBUF_PHY1    (1000 * 2)                       /* 发送缓冲大小(主机配置扩展串口时调试串口会被重新配置。开启调试时设置缓冲区为2400，解决打印不全问题) */
#else
#define  SIZE_SENDBUF_PHY1    (1200 * 1)                       /* 发送缓冲大小*/
#endif
#define  SIZE_TRANSBUF_PHY1    256                            /* 透传缓存区大小*/

#define  SIZE_RECVBUF_PHY2    (1200 * 1)                       /* 接收缓冲大小*/
#define  SIZE_SENDBUF_PHY2    (1200 * 1)                       /* 发送缓冲大小*/
#define  SIZE_TRANSBUF_PHY2    256                            /* 透传缓存区大小*/


static INT8U s_phy1_transbuf[SIZE_TRANSBUF_PHY1 + 5];           /* 透传缓冲区(+5是为了防止存储时数组越界) */
#if 0
static INT8U s_phy2_transbuf[SIZE_TRANSBUF_PHY2 + 5];           /* 透传缓冲区(+5是为了防止存储时数组越界) */
#endif

/******************************************************************************/
/*                           定义扩展串口接收结构体                                                         */
/******************************************************************************/
typedef struct {
   INT16U   rlen_limit;                           /* 接收达到的最大长度后就上传 */
   INT16U   otime_limit;                          /* 超时上传 */
   INT8U*   transbuf;                             /* 指向存储上传数据的存储空间 */
   INT16U   transbufsize;                         /* 上传缓冲区大小 */
   INT16U   recvlen;                              /* 接收到的长度 */
   INT16U   delaytime;                            /* 延时的时间 */
} EXUSART_RECV_T;

/******************************************************************************/
/*                      定义主串口向扩展串口发送数据缓存结构体                */
/******************************************************************************/
static EXUSART_RECV_T   s_phy_recv[MAX_EXUART_IDX];

static INT8U      s_scantmrid;
static INT8U      s_delaytmrid;

static INT16U     s_send_index;                  /* 发送流水号 */
static INT16U     s_rec_index[MAX_EXUART_IDX];                   /* 接收流水号 */
static BOOLEAN    s_rec_flag[MAX_EXUART_IDX];                    /* 第一包流水号一样需要特殊处理 */

static BOOLEAN    s_exuartstatus[MAX_EXUART_IDX] = {false, false};
static INT32U     s_com_baudvalue[MAX_EXUART_IDX] = {BAUD_115200, BAUD_9600};


static INT16U const s_sendbuf_size[MAX_EXUART_IDX] = {SIZE_SENDBUF_PHY1, SIZE_SENDBUF_PHY2};
static INT16U const s_recvbuf_size[MAX_EXUART_IDX] = {SIZE_RECVBUF_PHY1, SIZE_RECVBUF_PHY2};

/*******************************************************************************
**  函数名称:  ExUartData_Down_Hdl
**  功能描述:  数据透传下行协议处理
**  输入参数:
**  返回参数:
*******************************************************************************/
void ExUartData_Down_Hdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen)
{
	INT8U uartno;
    INT16U len;
    INT16U index;
    INT8U  temp[3];


    #if DEBUG_EXUART > 0
    debug_printf("扩展串口下行输出\r\n");
    printf_hex(data, datalen);
    debug_printf("\r\n");
    #endif

    uartno = data[0] - 1;
    if (uartno >= MAX_EXUART_IDX) {
        #if DEBUG_EXUART > 0
        debug_printf("扩展串口号错误 %d\r\n", uartno);
        #endif
        return;
    }
	if (s_exuartstatus[uartno] == false) {
        #if DEBUG_EXUART > 0
        debug_printf("扩展串口未初始化\r\n");
        #endif
        return;
	}
    len = data[3] << 8 | data[4];
    index = data[1] << 8 | data[2];

    #if DEBUG_EXUART > 0
    debug_printf("uartno:%d len:%d index:%d  s_rec_index:%d  s_rec_flag:%d\r\n", uartno, len, index, s_rec_index[uartno], s_rec_flag[uartno]);
    #endif
    if (index != s_rec_index[uartno] || (s_rec_flag[uartno] == FALSE)) {
        if (len > s_sendbuf_size[uartno]) {
            len = s_sendbuf_size[uartno];
        }
        PORT_UartWriteBlock(s_exuart_idx[uartno], data + 5, len);
        #if DEBUG_EXUART > 0
        debug_printf("PORT_UartWriteBlock  len:%d  data: ", len);
        printf_hex(data + 5, len);
        debug_printf("\r\n");
        #endif

        s_rec_flag[uartno] = TRUE;
        s_rec_index[uartno] = index;
    }

    memcpy(temp, data, 3);
    YX_COM_DirSend( DATA_DELIVER_DOWN_ACK, temp, 3);
}

/*******************************************************************************
**  函数名称:  DataDeliver_ReqAck_Hdl
**  功能描述:  数据透传上行应答处理
**  输入参数:
**  返回参数:
*******************************************************************************/
void ExUartData_ReqAck_Hdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen)
{
    mancode = mancode;
    command = command;
    datalen = datalen;
}
/*******************************************************************************
**  函数名称:  EXTDelayTmrProc
**  功能描述:  延时定时器入口函数，一旦超时，就上报
**  输入参数:  无
**  输出参数:  无
**  返回参数:  无
*******************************************************************************/
static void ExtDelayTmrProc(void *pdata)
{
    INT8U i;

    for (i = EXUART_IDX1; i < MAX_EXUART_IDX; i++) {
    	if (s_exuartstatus[i] == false) {
            return;
    	}

        if (s_phy_recv[i].recvlen > 0) {
            s_phy_recv[i].delaytime += (DELAY_TICKS * 10);
            if (s_phy_recv[i].delaytime >= s_phy_recv[i].otime_limit) {
                s_phy_recv[i].transbuf[0] = i + 1;   /* 串口号 */
			    s_phy_recv[i].transbuf[1] = s_send_index >> 8;
			    s_phy_recv[i].transbuf[2] = s_send_index;
			    s_phy_recv[i].transbuf[3] = s_phy_recv[i].recvlen >> 8;
			    s_phy_recv[i].transbuf[4] = s_phy_recv[i].recvlen;
				if (YX_COM_Islink()) {
					YX_COM_DirSend( DATA_DELIVER_REQ, s_phy_recv[i].transbuf, s_phy_recv[i].recvlen + 5);
				}
				s_send_index++;
                s_phy_recv[i].recvlen = 0;
                s_phy_recv[i].delaytime = 0;
            }
        }
    }
}
/*******************************************************************************
**  函数名称:  EXTScanTmrProc
**  功能描述:  定时扫描扩展串口缓冲区
**  输入参数:  无
**  输出参数:  无
**  返回参数:  无
*******************************************************************************/
static void ExtScanTmrProc(void *pdata)
{
	INT8U i;
    INT32S  curchar;

    for (i = EXUART_IDX1; i < MAX_EXUART_IDX; i++) {
    	if (s_exuartstatus[i] == false) {
            return;
    	}

        while ((curchar = PORT_UartRead(s_exuart_idx[i])) != -1) {
            #if DEBUG_EXUART > 1
            PORT_UartWriteByte(DEBUG_UART_NO, curchar);
            #endif
        	s_phy_recv[i].transbuf[s_phy_recv[i].recvlen + 5] = (INT8U)curchar;
            if (++s_phy_recv[i].recvlen >= s_phy_recv[i].transbufsize) {
                s_phy_recv[i].transbuf[0] = i + 1;   /* 串口号 */
			    s_phy_recv[i].transbuf[1] = s_send_index >> 8;
			    s_phy_recv[i].transbuf[2] = s_send_index;
			    s_phy_recv[i].transbuf[3] = s_phy_recv[i].recvlen >> 8;
			    s_phy_recv[i].transbuf[4] = s_phy_recv[i].recvlen;
				if (YX_COM_Islink()) {
					YX_COM_DirSend( DATA_DELIVER_REQ, s_phy_recv[i].transbuf, s_phy_recv[i].recvlen + 5);
				}
				s_send_index++;
                s_phy_recv[i].recvlen = 0;
                s_phy_recv[i].delaytime = 0;
            }
        }

        if (s_phy_recv[i].recvlen >= s_phy_recv[i].rlen_limit + 3) {
            s_phy_recv[i].transbuf[0] = i + 1;   /* 串口号 */
		    s_phy_recv[i].transbuf[1] = s_send_index >> 8;
		    s_phy_recv[i].transbuf[2] = s_send_index;
		    s_phy_recv[i].transbuf[3] = s_phy_recv[i].recvlen >> 8;
		    s_phy_recv[i].transbuf[4] = s_phy_recv[i].recvlen;
			if (YX_COM_Islink()) {
				YX_COM_DirSend( DATA_DELIVER_REQ, s_phy_recv[i].transbuf, s_phy_recv[i].recvlen + 5);
			}
			s_send_index++;
            s_phy_recv[i].recvlen = 0;
            s_phy_recv[i].delaytime = 0;
        }
    }
}

/*******************************************************************************
 ** 函数名:    YX_ExUart_Init
 ** 函数描述:  扩展串口初始化
 ** 参数:       无
 ** 返回:       无
 ******************************************************************************/
void YX_ExUart_Init(void)
{
    /* 扩展串口1 */
    s_phy_recv[EXUART_IDX1].rlen_limit  = 128;                  /* 超过64字节就上传 */
    s_phy_recv[EXUART_IDX1].otime_limit = 100;                 /* 超过100ms缓冲区里有数据也上传 */
    s_phy_recv[EXUART_IDX1].transbuf = s_phy1_transbuf;
    s_phy_recv[EXUART_IDX1].transbufsize = SIZE_TRANSBUF_PHY1;
    s_phy_recv[EXUART_IDX1].recvlen = 0;
    s_phy_recv[EXUART_IDX1].delaytime = 0;
    PORT_CloseUart(s_exuart_idx[EXUART_IDX1]);
    PORT_InitUart(s_exuart_idx[EXUART_IDX1], s_com_baudvalue[EXUART_IDX1], s_recvbuf_size[EXUART_IDX1], s_sendbuf_size[EXUART_IDX1]);
    s_exuartstatus[EXUART_IDX1] = true;
    s_rec_flag[EXUART_IDX1] = false;

    #if 0
    /* 扩展串口2 */
    s_phy_recv[EXUART_IDX2].rlen_limit  = 128;                  /* 超过64字节就上传 */
    s_phy_recv[EXUART_IDX2].otime_limit = 100;                 /* 超过100ms缓冲区里有数据也上传 */
    s_phy_recv[EXUART_IDX2].transbuf = s_phy2_transbuf;
    s_phy_recv[EXUART_IDX2].transbufsize = SIZE_TRANSBUF_PHY2;
    s_phy_recv[EXUART_IDX2].recvlen = 0;
    s_phy_recv[EXUART_IDX2].delaytime = 0;
    PORT_CloseUart(s_exuart_idx[EXUART_IDX2]);
    PORT_InitUart(s_exuart_idx[EXUART_IDX2], s_com_baudvalue[EXUART_IDX2], s_recvbuf_size[EXUART_IDX2], s_sendbuf_size[EXUART_IDX2]);
    s_exuartstatus[EXUART_IDX2] = true;
    s_rec_flag[EXUART_IDX2] = false;
    #endif
	
	s_send_index = 0;
    s_scantmrid  = OS_InstallTmr(TSK_ID_OPT, 0, ExtScanTmrProc);
    s_delaytmrid = OS_InstallTmr(TSK_ID_OPT, 0, ExtDelayTmrProc);

    OS_StartTmr(s_scantmrid, PERIOD_SCAN);
    OS_StartTmr(s_delaytmrid, PERIOD_DELAY);
}


/**************************************************************************************************
**  函数名称:  EXUartParaSetReqHdl
**  功能描述:  物理串口参数设置请求处理函数
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
void EXUsartParaSetReqHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen)
{
    INT8U ack[2];
    INT8U uartno;

    ack[0] = data[0];

    uartno = data[0] - 1;
    if (uartno >= MAX_EXUART_IDX) {
        ack[1] = 0x02;
        YX_COM_DirSend( PHYCOM_PARA_CONFIG_REQ_ACK, ack, 2);
        return;
    }

    /* 波特率 */
    switch (data[1]) {
        case 0x01:
            s_com_baudvalue[uartno] = BAUD_1200;
            break;
        case 0x02:
            s_com_baudvalue[uartno] = BAUD_2400;
            break;
        case 0x03:
            s_com_baudvalue[uartno] = BAUD_4800;
            break;
        case 0x04:
            s_com_baudvalue[uartno] = BAUD_9600;
            break;
        case 0x05:
            s_com_baudvalue[uartno] = BAUD_19200;
            break;
        case 0x06:
            s_com_baudvalue[uartno] = BAUD_38400;
            break;
        case 0x07:
            s_com_baudvalue[uartno] = BAUD_57600;
            break;
        case 0x08:
           s_com_baudvalue[uartno] = BAUD_115200;
            break;
        default:
            ack[1] = 0x02;
            YX_COM_DirSend( PHYCOM_PARA_CONFIG_REQ_ACK, ack, 2);
            return;
    }
    /* 数据位 */
    switch (data[2]) {
        case 0x04:
           // s_phy_para[channel].databits = DATABITS_8;
            break;
        //case 0x05:
            //s_phy_para[channel].databits = DATABITS_9;
          //  break;
        default:
            ack[1] = 0x02;
            YX_COM_DirSend( PHYCOM_PARA_CONFIG_REQ_ACK, ack, 2);
            return;
    }
    /* 停止位 */
    switch (data[3]) {
        case 0x01:
            //s_phy_para[channel].stopbits = STOPBITS_1;
            break;
      //  case 0x03:
            //s_phy_para[channel].stopbits = STOPBITS_2;
           // break;
        default:
            ack[1] = 0x02;
            YX_COM_DirSend( PHYCOM_PARA_CONFIG_REQ_ACK, ack, 2);
            return;
    }
    /* 校验方式 */
    switch (data[4]) {
        case 0x01:
            break;
        default:
            ack[1] = 0x02;
            YX_COM_DirSend( PHYCOM_PARA_CONFIG_REQ_ACK, ack, 2);
            return;
    }

    PORT_CloseUart(s_exuart_idx[uartno]);
    PORT_InitUart(s_exuart_idx[uartno], s_com_baudvalue[uartno], s_recvbuf_size[uartno], s_sendbuf_size[uartno]);

    ack[1] = 0x01;
    YX_COM_DirSend( PHYCOM_PARA_CONFIG_REQ_ACK, ack, 2);
}

/**************************************************************************************************
**  函数名称:  EXUartParaQueryHdl
**  功能描述:  物理串口参数查询处理函数
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
void EXUsartParaQueryHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen)
{
    INT8U  ack[6];
    INT8U  uartno;

    uartno = data[0] - 1;
    ack[0] = data[0];

    if (uartno >= MAX_EXUART_IDX) {
        ack[1] = 0x02;
        YX_COM_DirSend( PHYCOM_PARA_QUERY_ACK, ack, 2);
        return;
    }

    ack[1] = 0x01;
    switch (s_com_baudvalue[uartno]) {
        case BAUD_1200:
            ack[2] = 0x01;
            break;
        case BAUD_2400:
            ack[2] = 0x02;
            break;
        case BAUD_4800:
            ack[2] = 0x03;
            break;
        case BAUD_9600:
            ack[2] = 0x04;
            break;
        case BAUD_19200:
            ack[2] = 0x05;
            break;
        case BAUD_38400:
            ack[2] = 0x06;
            break;
        case BAUD_57600:
            ack[2] = 0x07;
            break;
        case BAUD_115200:
            ack[2] = 0x08;
            break;
        default:
            break;
    }
    /* 数据位 */
    ack[3] = 0x04;
    ack[4] = 0x01;
    ack[5] = 0x01;
    #if 0
    switch (s_phy_para[channel].databits) {
        case DATABITS_8:
            ack[3] = 0x04;
            break;
        case DATABITS_9:
            ack[3] = 0x05;
            break;
        default:
            break;
    }

    /* 停止位 */
    switch (s_phy_para[channel].stopbits) {
        case STOPBITS_1:
            ack[4] = 0x01;
            break;
        case STOPBITS_2:
            ack[4] = 0x03;
            break;
        default:
            break;
    }
    /* 校验方式 */
    switch (s_phy_para[channel].parity) {
        case PARITY_NONE:
            ack[5] = 0x01;
            break;
        case PARITY_EVEN:
            ack[5] = 0x02;
            if (s_phy_para[channel].databits == DATABITS_9) {
                ack[3] = 0x04;
            }
            break;
        case PARITY_ODD:
            ack[5] = 0x03;
            if (s_phy_para[channel].databits == DATABITS_9) {
                ack[3] = 0x04;
            }
            break;
        default:
            break;
    }
    #endif
    YX_COM_DirSend( PHYCOM_PARA_QUERY_ACK, ack, 6);
}

/**************************************************************************************************
**  函数名称:  EXUartPowerCtrlHdl
**  功能描述:  物理串口电源控制处理函数
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
void EXUsartPowerCtrlHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen)
{
    INT8U  ack[3];
    INT8U  uartno;

    ack[0] = data[0];
    ack[1] = data[1];
    uartno = data[0] - 1;

    if (uartno >= MAX_EXUART_IDX){
        ack[2] = 0x02;
        YX_COM_DirSend( PHYCOM_POWER_CONTROL_REQ_ACK, ack, 2);
        return;
    }

    switch (uartno) {
        case EXUART_IDX1:
            if (0x01 == data[1]) {
            } else if (0x02 == data[1]) {
            } else {
                ack[2] = 0x02;
                YX_COM_DirSend( PHYCOM_POWER_CONTROL_REQ_ACK, ack, 3);
                return;
            }
            break;
        case EXUART_IDX2:
            if (0x01 == data[1]) {
                PORT_ClearGpioPin(PIN_GPSPWR);
            } else if (0x02 == data[1]) {
                PORT_SetGpioPin(PIN_GPSPWR);
            } else {
                ack[2] = 0x02;
                YX_COM_DirSend( PHYCOM_POWER_CONTROL_REQ_ACK, ack, 3);
                return;
            }
            break;
        default:
            break;
    }

    #if 0
    if (0x01 == data[1]) {
        PORT_CloseUart(s_exuart_idx[uartno]);
        PORT_InitUart(s_exuart_idx[uartno], s_com_baudvalue[uartno], s_recvbuf_size[uartno], s_sendbuf_size[uartno]);
    } else if (0x02 == data[1]) {
        PORT_CloseUart(s_exuart_idx[uartno]);
    } else {
        ack[2] = 0x02;
        YX_COM_DirSend( PHYCOM_POWER_CONTROL_REQ_ACK, ack, 3);
        return;
    }
    #endif

    ack[2] = 0x01;
    YX_COM_DirSend( PHYCOM_POWER_CONTROL_REQ_ACK, ack, 3);
}



