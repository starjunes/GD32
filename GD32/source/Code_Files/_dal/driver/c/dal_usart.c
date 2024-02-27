/**************************************************************************************************
**                                                                                               **
**  文件名称:  dal_usart.C                                                                       **
**  版权所有:  CopyRight @ Xiamen Yaxon NetWork CO.LTD. 2012                                     **
**  创建信息:  2012-1-12 By clt: 创建本文件                                                      **
**  文件描述:  串口驱动接口函数                                                                  **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**  暂存问题:  DMA本身机制所限，连续发送数据频率过快时(如反复连续调用发送函数)，可能导致发送出错 **
**************************************************************************************************/
#include "dal_include.h"
#include "tools.h"
#include "man_irq.h"
#include "man_error.h"
#include "dal_usart.h"
#include "dal_pinlist.h"
#include "dal_hard.h"
#include "man_timer.h"
#include "dal_gpio.h"
#include "debug_print.h"


#define  DMA_RCVBUF           100                                               /* DMA接收缓存 */
#define  DMA_SENDBUF          100                                               /* DMA发送缓存 */
/*************************************************************************************************/
/*                           模块结构体定义                                                      */
/*************************************************************************************************/
typedef struct {
    USART_SS_E volatile status;                                                 /* 当前工作状态 */
    ROUNDBUF_T          recvrb;                                                 /* 接收环形缓冲区 */
    ROUNDBUF_T          sendrb;                                                 /* 发送环形缓冲区 */
    USART_PARITY_E      parity;                                                 /* 校验位，判断是否是SPACE与MARK校验 */
    BOOLEAN             dmairq;                                                 /* 是否进入过DMA中断 */
    BOOLEAN             timerread;                                              /* 定时器正在考备数据，若此时进DMA中断则直接退出 */
    INT16U              oldlocat;                                               /* 上回DMA写位置 */
    INT8U               readlocat;                                              /* 读位置 */
    INT8U               *rcvdmabuf;                                             /* DMA接收缓存 */
    INT16U              *senddmabuf;                                            /* DMA发送缓存 */
} USART_CTRL_T;

/*************************************************************************************************/
/*                           USART回调APP层结构体                                                  */
/*************************************************************************************************/
typedef struct {
    USART_LBINDEX_E index;                                                      /* 回调索引号 */
    void  (*lbhandle) (void);                                                   /* 回调函数指针 */
} USART_LBAPP_E;

/*************************************************************************************************/
/*                           模块静态变量定义                                                    */
/*************************************************************************************************/

static INT8U       s_phy1_dmarecvbuf[DMA_RCVBUF];                               /* 接收缓冲区 */
static INT8U       s_phy2_dmarecvbuf[DMA_RCVBUF];                               /* 接收缓冲区 */
static INT8U       s_phy3_dmarecvbuf[DMA_RCVBUF];                               /* 接收缓冲区 */
static INT8U       s_phy4_dmarecvbuf[DMA_RCVBUF];                               /* 接收缓冲区 */
static INT8U       s_phy5_dmarecvbuf[DMA_RCVBUF];                               /* 接收缓冲区 */

static INT16U      s_phy1_dmasendbuf[DMA_SENDBUF];                              /* 发送缓冲区 */
static INT16U      s_phy2_dmasendbuf[DMA_SENDBUF];                              /* 发送缓冲区 */
static INT16U      s_phy3_dmasendbuf[DMA_SENDBUF];                              /* 发送缓冲区 */
static INT16U      s_phy4_dmasendbuf[DMA_SENDBUF];                              /* 发送缓冲区 */
static INT16U      s_phy5_dmasendbuf[DMA_SENDBUF];                              /* 发送缓冲区 */

static USART_CTRL_T  s_ucbt[USART_MAX];                                         /* USART控制块 */
static USART_LBAPP_E s_lbapp[LB_USART_MAX];
/*第一次发送总线数据，防止串口刚初始化后检测不到总线为空状态*/
static BOOLEAN     s_send485statu = false;
static INT32U      s_485_receive_cnt; 

static INT32U      s_Delaytmrid;                                               /* 延时判断定时计数 */
static BOOLEAN     s_tmrcreatsatu = false;                                     /* 定时器是否已经创建 */
static BOOLEAN     s_idledetectable;                                           /* 判断是否可以检测总线空闲 */

/**************************************************************************************************
**  函数名称:  USART_Rx_Contrl
**  功能描述:  串口发送功能控制
**  输入参数:  index: 串口号
**  输出参数:  
**************************************************************************************************/
static void USART_Rx_Contrl(INT32U USARTx,ControlStatus NewState)
{
    if (NewState != DISABLE) {
		usart_transmit_config(USARTx, USART_TRANSMIT_ENABLE);
    } else {
        usart_transmit_config(USARTx, USART_TRANSMIT_DISABLE);
    }
}

/*******************************************************************
** 函数名:     usartx_startsend
** 函数描述:   启动USARTX发送数据
** 参数:       [in] index: 串口号
** 返回:       无
********************************************************************/
static BOOLEAN usartx_startsend(USART_INDEX_E index)
{
    INT32S  temp = 0, check = 0;
    INT32U  tlen, i, blen; 
    INT16U  *tptr;

    tlen = UsedOfRoundBuf(&s_ucbt[index].sendrb);
    if (tlen <= 0)  return false;
    s_ucbt[index].status = STATUS_SENDING;                                      /* 置于发送状态 */

    if (s_ucbt[index].parity == PARITY_MARK) {                                  /* 校验位设置  */
        check = 1 << 8;
    }
    tptr = s_ucbt[index].senddmabuf; 
    blen = DMA_SENDBUF < (tlen) ? DMA_SENDBUF : (tlen);

 //   Debug_SysPrint("usart %d blen = %d,tlen = %d,dlen = %d\r\n",index,blen,tlen,s_ucbt[index].dlen);
    switch (index)                                                              /* 启动DMA发送 */
    {
        case USART_NO1:
            dma_flag_clear(DMA0,DMA_CH3,DMA_FLAG_FTF);
			dma_channel_disable(DMA0,DMA_CH3);
            for (i = 0; i < blen; i++) {                                        /* 依次复制每一个数据 */
                temp = ReadRoundBuf(&s_ucbt[index].sendrb);
                *tptr++ = check | temp;
            }
            dma_transfer_number_config(DMA0,DMA_CH3,blen);
			dma_interrupt_enable(DMA0,DMA_CH3,DMA_INT_FTF);
            dma_channel_enable(DMA0,DMA_CH3);
            break;
            
        case USART_NO2:
            dma_flag_clear(DMA0,DMA_CH6,DMA_FLAG_FTF);
			//dma_interrupt_disable(DMA0,DMA_CH6,DMA_INT_FTF);
			dma_channel_disable(DMA0,DMA_CH6);
            for (i = 0; i < blen; i++) {                                        /* 依次复制每一个数据 */
                temp = ReadRoundBuf(&s_ucbt[index].sendrb);
                *tptr++ = check | temp;
            }  
            dma_transfer_number_config(DMA0,DMA_CH6,blen);
			dma_interrupt_enable(DMA0,DMA_CH6,DMA_INT_FTF);
            dma_channel_enable(DMA0,DMA_CH6);
            break;
            
        case USART_NO3:
            dma_flag_clear(DMA0,DMA_CH1,DMA_FLAG_FTF);
			dma_channel_disable(DMA0,DMA_CH1);
			dma_interrupt_disable(DMA0,DMA_CH1,DMA_INT_FTF);
            for (i = 0; i < blen; i++) {                                         /* 依次复制每一个数据 */
                temp = ReadRoundBuf(&s_ucbt[index].sendrb);
                *tptr++ = check | temp;  
            }
            dma_transfer_number_config(DMA0,DMA_CH1,blen);
			dma_interrupt_enable(DMA0,DMA_CH1,DMA_INT_FTF);
            dma_channel_enable(DMA0,DMA_CH1);
            break;

        case USART_NO4:
            temp = check | ReadRoundBuf(&s_ucbt[index].sendrb);
			usart_data_transmit(UART3,(INT16U)temp);
            usart_interrupt_enable(UART3,USART_INT_TBE);
            break;

        case USART_NO5:
            temp = check | ReadRoundBuf(&s_ucbt[index].sendrb);
			usart_data_transmit(UART4,(INT16U)temp);
            usart_interrupt_enable(UART4,USART_INT_TBE);
            break;             

        default:
            DAL_ASSERT(0);                                                      /* 此处加入出错指示 */
            break;
    }
    return true;
}

/*******************************************************************
** 函数名:     USER_DMA0_Stream4_IRQHandler
** 函数描述:   USART0的DMA中断处理 - 仅处理接收中断
** 参数:       无
** 返回:       无
********************************************************************/
static __attribute__ ((section ("IRQ_HANDLE"))) void USER_DMA0_Stream4_IRQHandler(void) __irq
{
    
    INT16S readlen = 0;
   
	if (dma_interrupt_flag_get(DMA0, DMA_CH4, DMA_INT_FLAG_HTF)) {							/* 半发送中断 */
		dma_interrupt_flag_clear(DMA0, DMA_CH4, DMA_INT_FLAG_HTF);		 
		if (s_ucbt[USART_NO1].timerread == true) return;						/* 定时器正在考备数据 */

		readlen = (DMA_RCVBUF - s_ucbt[USART_NO1].readlocat - dma_transfer_number_get(DMA0,DMA_CH4));
		if (readlen < 0) {														/* 缓存尾部有数据未读走 */
			readlen = DMA_RCVBUF - s_ucbt[USART_NO1].readlocat;
			WriteBlockRoundBuf(&s_ucbt[USART_NO1].recvrb, &(s_ucbt[USART_NO1].rcvdmabuf[s_ucbt[USART_NO1].readlocat]), readlen);
			s_ucbt[USART_NO1].readlocat = 0;
			readlen = (DMA_RCVBUF - dma_transfer_number_get(DMA0,DMA_CH4) - s_ucbt[USART_NO1].readlocat);
		}
		WriteBlockRoundBuf(&s_ucbt[USART_NO1].recvrb, &(s_ucbt[USART_NO1].rcvdmabuf[s_ucbt[USART_NO1].readlocat]), readlen);
		s_ucbt[USART_NO1].readlocat += readlen;
		s_ucbt[USART_NO1].dmairq = true;
	}  
	 
	if (dma_interrupt_flag_get(DMA0, DMA_CH4, DMA_INT_FLAG_FTF)) {							/* 半发送中断 */
		dma_interrupt_flag_clear(DMA0, DMA_CH4, DMA_INT_FLAG_FTF);		  
		if (s_ucbt[USART_NO1].timerread == true) return;						/* 定时器正在考备数据 */

		readlen = (DMA_RCVBUF - s_ucbt[USART_NO1].readlocat);
		WriteBlockRoundBuf(&s_ucbt[USART_NO1].recvrb, &(s_ucbt[USART_NO1].rcvdmabuf[s_ucbt[USART_NO1].readlocat]), readlen);
		s_ucbt[USART_NO1].readlocat = 0;
		s_ucbt[USART_NO1].dmairq = true;
	}

}

/*******************************************************************
** 函数名:     USER_DMA0_Stream3_IRQHandler
** 函数描述:   USART0的DMA中断处理 - 仅处理发送中断
** 参数:       无
** 返回:       无
********************************************************************/
static __attribute__ ((section ("IRQ_HANDLE"))) void USER_DMA0_Stream3_IRQHandler(void) __irq
{
    INT32U tlen;
    
    if (dma_interrupt_flag_get(DMA0, DMA_CH3, DMA_INT_FLAG_FTF)) {                          /* 半发送中断 */
        dma_interrupt_flag_clear(DMA0, DMA_CH3, DMA_INT_FLAG_FTF);  
		dma_channel_disable(DMA0, DMA_CH3);
		dma_interrupt_disable(DMA0, DMA_CH3,DMA_INT_FTF);
        tlen = UsedOfRoundBuf(&s_ucbt[USART_NO1].sendrb);                       /* 判断roundbuf中是否有数据要发送 */
        if (tlen <= 0) {
            s_ucbt[USART_NO1].status = STATUS_IDLENOW;
        } else {                                                                /* 还有数据需要发送 */
            usartx_startsend(USART_NO1);
        }
    }
}

/*******************************************************************
** 函数名:     USER_DMA0_Stream5_IRQHandler
** 函数描述:   USART1的DMA中断处理 - 仅处理接收中断
** 参数:       无
** 返回:       无
********************************************************************/
static __attribute__ ((section ("IRQ_HANDLE"))) void USER_DMA0_Stream5_IRQHandler(void) __irq
{

    
	INT16S readlen = 0;
	
	if (dma_interrupt_flag_get(DMA0, DMA_CH5, DMA_INT_FLAG_HTF)) {							/* 半发送中断 */
		dma_interrupt_flag_clear(DMA0, DMA_CH5, DMA_INT_FLAG_HTF); 

		if (s_ucbt[USART_NO2].timerread == true) return;						/* 定时器正在考备数据 */
		readlen = (DMA_RCVBUF - s_ucbt[USART_NO2].readlocat - dma_transfer_number_get(DMA0,DMA_CH5));
		if (readlen < 0) {														/* 缓存尾部有数据未读走 */
			readlen = DMA_RCVBUF - s_ucbt[USART_NO2].readlocat;
			WriteBlockRoundBuf(&s_ucbt[USART_NO2].recvrb, &(s_ucbt[USART_NO2].rcvdmabuf[s_ucbt[USART_NO2].readlocat]), readlen);
			s_ucbt[USART_NO2].readlocat = 0;
			readlen = (DMA_RCVBUF - dma_transfer_number_get(DMA0,DMA_CH5) - s_ucbt[USART_NO2].readlocat);
		}

		WriteBlockRoundBuf(&s_ucbt[USART_NO2].recvrb, &(s_ucbt[USART_NO2].rcvdmabuf[s_ucbt[USART_NO2].readlocat]), readlen);
		s_ucbt[USART_NO2].readlocat += readlen;
		s_ucbt[USART_NO2].dmairq = true;
	}
	   
	if (dma_interrupt_flag_get(DMA0, DMA_CH5, DMA_INT_FLAG_FTF)) {							/* 发送结束 */
		dma_interrupt_flag_clear(DMA0, DMA_CH5, DMA_INT_FLAG_FTF); 

		if (s_ucbt[USART_NO2].timerread == true) return;						/* 定时器正在考备数据 */

		readlen = (DMA_RCVBUF - s_ucbt[USART_NO2].readlocat);
		WriteBlockRoundBuf(&s_ucbt[USART_NO2].recvrb, &(s_ucbt[USART_NO2].rcvdmabuf[s_ucbt[USART_NO2].readlocat]), readlen);
		s_ucbt[USART_NO2].readlocat = 0;
		s_ucbt[USART_NO2].dmairq = true;
	}

}

/*******************************************************************
** 函数名:     USER_DMA0_Stream6_IRQHandler
** 函数描述:   USART1的DMA中断处理 - 仅处理发送中断
** 参数:       无
** 返回:       无
********************************************************************/
static __attribute__ ((section ("IRQ_HANDLE"))) void USER_DMA0_Stream6_IRQHandler(void) __irq
{
    INT32U tlen;
    
    if (dma_interrupt_flag_get(DMA0, DMA_CH6, DMA_INT_FLAG_FTF)) {                          /* 半发送中断 */
        dma_interrupt_flag_clear(DMA0, DMA_CH6, DMA_INT_FLAG_FTF);  
		dma_channel_disable(DMA0, DMA_CH6);
		dma_interrupt_disable(DMA0, DMA_CH6,DMA_INT_FTF);
        tlen = UsedOfRoundBuf(&s_ucbt[USART_NO2].sendrb);                       /* 判断roundbuf中是否有数据要发送 */
        if (tlen <= 0) {
            s_ucbt[USART_NO2].status = STATUS_IDLENOW;
        } else {                                                                /* 还有数据需要发送 */
            usartx_startsend(USART_NO2);
        }
    }
}

/*******************************************************************
** 函数名:     USER_DMA0_Stream2_IRQHandler
** 函数描述:   USART2的DMA中断处理 - 仅处理接收中断
** 参数:       无
** 返回:       无
********************************************************************/
static __attribute__ ((section ("IRQ_HANDLE"))) void USER_DMA0_Stream2_IRQHandler(void) __irq
{
    INT16S  readlen = 0;
   
    if (dma_interrupt_flag_get(DMA0, DMA_CH2, DMA_INT_FLAG_HTF)) {                          /* 半发送中断 */
        dma_interrupt_flag_clear(DMA0, DMA_CH2, DMA_INT_FLAG_HTF); 
        if (s_ucbt[USART_NO3].timerread == true) return;                        /* 定时器正在考备数据 */
		//debug_printf("USART2 DMA_INT_HTF\r\n");
        readlen = (DMA_RCVBUF - s_ucbt[USART_NO3].readlocat - dma_transfer_number_get(DMA0,DMA_CH2));
        if (readlen < 0) {                                                      /* 缓存尾部有数据未读走 */
            readlen = DMA_RCVBUF - s_ucbt[USART_NO3].readlocat;
            WriteBlockRoundBuf(&s_ucbt[USART_NO3].recvrb, &(s_ucbt[USART_NO3].rcvdmabuf[s_ucbt[USART_NO3].readlocat]), readlen);
            s_ucbt[USART_NO3].readlocat = 0;
            readlen = (DMA_RCVBUF - dma_transfer_number_get(DMA0,DMA_CH2) - s_ucbt[USART_NO3].readlocat);
        }

        WriteBlockRoundBuf(&s_ucbt[USART_NO3].recvrb, &(s_ucbt[USART_NO3].rcvdmabuf[s_ucbt[USART_NO3].readlocat]), readlen);
        s_ucbt[USART_NO3].readlocat += readlen;
        s_ucbt[USART_NO3].dmairq  = true;
    }
       
    if (dma_interrupt_flag_get(DMA0, DMA_CH2, DMA_INT_FLAG_FTF)) {                          /* 半发送中断 */
        dma_interrupt_flag_clear(DMA0, DMA_CH2, DMA_INT_FLAG_FTF);
        if (s_ucbt[USART_NO3].timerread == true) return;                        /* 定时器正在考备数据 */

		//debug_printf("USART2 DMA_INT_FLAG_FTF\r\n");
        readlen = (DMA_RCVBUF - s_ucbt[USART_NO3].readlocat);
        WriteBlockRoundBuf(&s_ucbt[USART_NO3].recvrb, &(s_ucbt[USART_NO3].rcvdmabuf[s_ucbt[USART_NO3].readlocat]), readlen);
        s_ucbt[USART_NO3].readlocat = 0;
        s_ucbt[USART_NO3].dmairq = true;
    } 
}

/*******************************************************************
** 函数名:     USER_DMA0_Stream1_IRQHandler
** 函数描述:   USART2的DMA中断处理 - 仅处理发送中断
** 参数:       无
** 返回:       无
********************************************************************/
static __attribute__ ((section ("IRQ_HANDLE"))) void USER_DMA0_Stream1_IRQHandler(void) __irq
{
    INT32U tlen;
    
    if (dma_interrupt_flag_get(DMA0, DMA_CH1, DMA_INT_FLAG_FTF)) {                          /* 半发送中断 */
        dma_interrupt_flag_clear(DMA0, DMA_CH1, DMA_INT_FLAG_FTF);  
		dma_channel_disable(DMA0, DMA_CH1);
		dma_interrupt_disable(DMA0, DMA_CH1,DMA_INT_FTF);
        tlen = UsedOfRoundBuf(&s_ucbt[USART_NO3].sendrb);                       /* 判断roundbuf中是否有数据要发送 */
        if (tlen <= 0) {
            //USART_ITConfig(USART3, USART_IT_TC, ENABLE);
            s_ucbt[USART_NO3].status = STATUS_IDLENOW;
        } else {                                                                /* 还有数据需要发送 */
            usartx_startsend(USART_NO3);
        }
    }
}

/**************************************************************************************************
**  函数名称:  USER_USART2_IRQHandler
**  功能描述:  USART2的中断处理函数 - 处理发送移位寄存器
**  输入参数:  
**  输出参数:  
**************************************************************************************************/
static __attribute__ ((section ("IRQ_HANDLE"))) void USER_USART2_IRQHandler(void) __irq
{
    INT32U tlen;

    ENTER_CRITICAL();

    if (usart_interrupt_flag_get(USART2, USART_INT_FLAG_IDLE) != RESET) {
		usart_interrupt_disable(USART2,USART_INT_IDLE);
		usart_interrupt_flag_clear(USART2, USART_INT_FLAG_IDLE);
		usart_data_receive(USART2);
		tlen = UsedOfRoundBuf(&s_ucbt[USART_NO3].sendrb);
		if (tlen > 0) {
			USART_Rx_Contrl(USART2,USART_INT_FLAG_IDLE);
			usartx_startsend(USART_NO3);
			//WriteBlockRoundBuf(&s_ucbt[USART_NO3].recvrb,s_ucbt[USART_NO3].rcvdmabuf,tlen);
        }
	}

    if (usart_interrupt_flag_get(USART2, USART_INT_FLAG_TC) != RESET) {                     /* 发送移位寄存器为空*/
        if (usart_flag_get(USART2, USART_FLAG_TC) == 1) {                /* 这里判断，防止在移位发送最后一个字节时APP层往缓冲中写数据 */
            usart_interrupt_disable(USART2, USART_INT_FLAG_TC);
            USART_Rx_Contrl(USART2, ENABLE);
        }
        usart_interrupt_flag_clear(USART2, USART_INT_FLAG_TC);                          /* 清除标志位 */
		//usart_interrupt_enable(USART2,USART_INT_FLAG_TC);
	}
    EXIT_CRITICAL();
}
#if 0
/*******************************************************************
** 函数名:     USER_DMA1_Stream2_IRQHandler
** 函数描述:   USART4的DMA中断处理 - 仅处理接收中断
** 参数:       无
** 返回:       无
********************************************************************/
static __attribute__ ((section ("IRQ_HANDLE"))) void USER_DMA1_Stream2_IRQHandler(void) __irq
{
    INT16S  readlen = 0;
   
    if (DMA_GetITStatus(DMA1_Stream2, DMA_IT_HTIF2)) {                          /* 半接收中断 */
        DMA_ClearITPendingBit(DMA1_Stream2, DMA_IT_HTIF2);       
        if (s_ucbt[USART_NO4].timerread == true) return;                        /* 定时器正在考备数据 */

        readlen = (DMA_RCVBUF - s_ucbt[USART_NO4].readlocat - DMA1_Stream2->NDTR);
        if (readlen < 0) {                                                      /* 缓存尾部有数据未读走 */
            readlen = DMA_RCVBUF - s_ucbt[USART_NO4].readlocat;
            WriteBlockRoundBuf(&s_ucbt[USART_NO4].recvrb, &(s_ucbt[USART_NO4].rcvdmabuf[s_ucbt[USART_NO4].readlocat]), readlen);
            s_ucbt[USART_NO4].readlocat = 0;
            readlen = (DMA_RCVBUF - DMA1_Stream2->NDTR - s_ucbt[USART_NO4].readlocat);
        }

        WriteBlockRoundBuf(&s_ucbt[USART_NO4].recvrb, &(s_ucbt[USART_NO4].rcvdmabuf[s_ucbt[USART_NO4].readlocat]), readlen);
        s_ucbt[USART_NO4].readlocat += readlen;
        s_ucbt[USART_NO4].dmairq = true;
    }
       
    if (DMA_GetITStatus(DMA1_Stream2, DMA_IT_TCIF2)) {                          /* 全接收中断 */
        DMA_ClearITPendingBit(DMA1_Stream2, DMA_IT_TCIF2);
        if (s_ucbt[USART_NO4].timerread == true) return;                        /* 定时器正在考备数据 */

        readlen = (DMA_RCVBUF  - s_ucbt[USART_NO4].readlocat);
        WriteBlockRoundBuf(&s_ucbt[USART_NO4].recvrb, &(s_ucbt[USART_NO4].rcvdmabuf[s_ucbt[USART_NO4].readlocat]), readlen);
        s_ucbt[USART_NO4].readlocat = 0;
        s_ucbt[USART_NO4].dmairq = true;
    }
}

/*******************************************************************
** 函数名:     USER_DMA1_Stream4_IRQHandler
** 函数描述:   USART4的DMA中断处理 - 仅处理发送中断
** 参数:       无
** 返回:       无
********************************************************************/
static __attribute__ ((section ("IRQ_HANDLE"))) void USER_DMA1_Stream4_IRQHandler(void) __irq
{
    INT32U tlen;
    
    if (DMA_GetITStatus(DMA1_Stream4, DMA_IT_TCIF4)) {                          /* 发送结束 */
        DMA_ClearITPendingBit(DMA1_Stream4, DMA_IT_TCIF4);
        DMA_Cmd(DMA1_Stream4, DISABLE);                                         /* 禁用DMA发送 */
        DMA_ITConfig(DMA1_Stream4, DMA_IT_TC, DISABLE);
        tlen = UsedOfRoundBuf(&s_ucbt[USART_NO4].sendrb);                       /* 判断roundbuf中是否有数据要发送 */
        if (tlen <= 0) {
            s_ucbt[USART_NO4].status = STATUS_IDLENOW;
        } else {                                                                /* 还有数据需要发送 */
            usartx_startsend(USART_NO4);
        }
    }
}

/*******************************************************************
** 函数名:     USER_DMA1_Stream0_IRQHandler
** 函数描述:   USART5的DMA中断处理 - 仅处理接收中断
** 参数:       无
** 返回:       无
********************************************************************/
static __attribute__ ((section ("IRQ_HANDLE"))) void USER_DMA1_Stream0_IRQHandler(void) __irq
{
    INT16S  readlen = 0;
   
    if (DMA_GetITStatus(DMA1_Stream0, DMA_IT_HTIF0)) {                          /* 发送结束 */
        DMA_ClearITPendingBit(DMA1_Stream0, DMA_IT_HTIF0);       
        if (s_ucbt[USART_NO5].timerread == true) return;                        /* 定时器正在考备数据 */

        readlen = (DMA_RCVBUF - s_ucbt[USART_NO5].readlocat - DMA1_Stream0->NDTR);
        if (readlen < 0) {                                                      /* 缓存尾部有数据未读走 */
            readlen = DMA_RCVBUF - s_ucbt[USART_NO5].readlocat;
            WriteBlockRoundBuf(&s_ucbt[USART_NO5].recvrb, &(s_ucbt[USART_NO5].rcvdmabuf[s_ucbt[USART_NO5].readlocat]), readlen);
            s_ucbt[USART_NO5].readlocat = 0;
            readlen = (DMA_RCVBUF - DMA1_Stream0->NDTR - s_ucbt[USART_NO5].readlocat);
        }

        WriteBlockRoundBuf(&s_ucbt[USART_NO5].recvrb, &(s_ucbt[USART_NO5].rcvdmabuf[s_ucbt[USART_NO5].readlocat]), readlen);
        s_ucbt[USART_NO5].readlocat += readlen;
        s_ucbt[USART_NO5].dmairq = true;
    }
       
    if (DMA_GetITStatus(DMA1_Stream0, DMA_IT_TCIF0)) {                          /* 发送结束 */
        DMA_ClearITPendingBit(DMA1_Stream0, DMA_IT_TCIF0);
        if (s_ucbt[USART_NO5].timerread == true) return;                        /* 定时器正在考备数据 */

        readlen = (DMA_RCVBUF  - s_ucbt[USART_NO5].readlocat);
        WriteBlockRoundBuf(&s_ucbt[USART_NO5].recvrb, &(s_ucbt[USART_NO5].rcvdmabuf[s_ucbt[USART_NO5].readlocat]), readlen);
        s_ucbt[USART_NO5].readlocat = 0;
        s_ucbt[USART_NO5].dmairq = true;
    }
}

/*******************************************************************
** 函数名:     USER_DMA1_Stream7_IRQHandler
** 函数描述:   USART5的DMA中断处理 - 仅处理发送中断
** 参数:       无
** 返回:       无
********************************************************************/
static __attribute__ ((section ("IRQ_HANDLE"))) void USER_DMA1_Stream7_IRQHandler(void) __irq
{
    INT32U tlen;
    
    if (DMA_GetITStatus(DMA1_Stream7, DMA_IT_TCIF7)) {                          /* 发送结束 */
        DMA_ClearITPendingBit(DMA1_Stream7, DMA_IT_TCIF7);
        DMA_Cmd(DMA1_Stream7, DISABLE);                                         /* 禁用DMA发送 */
        DMA_ITConfig(DMA1_Stream7, DMA_IT_TC, DISABLE);
        tlen = UsedOfRoundBuf(&s_ucbt[USART_NO5].sendrb);                       /* 判断roundbuf中是否有数据要发送 */
        if (tlen <= 0) {
            USART_ITConfig(UART5, USART_IT_TC, ENABLE);                         /* 485总线改动，与其他串口不同需注意 */
            s_ucbt[USART_NO5].status = STATUS_IDLENOW;
        } else {                                                                /* 还有数据需要发送 */
            usartx_startsend(USART_NO5);
        }
    }
}

/**************************************************************************************************
**  函数名称:  UART5_IRQHandler
**  功能描述:  UART5的中断处理函数 - 处理发送移位寄存器
**  输入参数:  
**  输出参数:  
**************************************************************************************************/
static __attribute__ ((section ("IRQ_HANDLE"))) void USER_UART5_IRQHandler(void) __irq
{
    INT32U tlen;

    ENTER_CRITICAL();

    if (USART_GetITStatus(UART5, USART_IT_IDLE) != RESET) {                    /* 发送完成中断 */
        USART_ITConfig(UART5, USART_IT_IDLE, DISABLE);
        tlen = UsedOfRoundBuf(&s_ucbt[USART_NO5].sendrb);                      /* 判断roundbuf中是否有数据要发送 */
        if (tlen > 0) {
            s_lbapp[LB_485_TOSEND].lbhandle();
            usartx_startsend(USART_NO5);
        }
        USART_ClearITPendingBit(UART5, USART_IT_IDLE);                         /* 清除标志位 */
    }

    if (USART_GetITStatus(UART5, USART_IT_TC) != RESET) {                      /* 发送移位寄存器为空*/
        if (USART_GetFlagStatus(UART5,USART_FLAG_TXE) == 1) {                  /* 这里判断，防止在移位发送最后一个字节时APP层往缓冲中写数据 */
            USART_ITConfig(UART5, USART_IT_TC, DISABLE);
            s_lbapp[LB_485_TORCV].lbhandle();
        }
        USART_ClearITPendingBit(UART5, USART_IT_TC);                           /* 清除标志位 */
    }
    EXIT_CRITICAL();
}
#endif
/*******************************************************************
** 函数名:     usartx_send_dma_config
** 函数描述:   USARTX_DMA发送参数配置 - 配置完默认是禁用的
** 参数:       [in] index: 串口号
** 返回:       无
********************************************************************/
static void usartx_send_dma_config(USART_INDEX_E index)
{
    dma_parameter_struct ICB;
        
	ICB.direction	= DMA_MEMORY_TO_PERIPHERAL;
	ICB.memory_addr = (INT32U)s_ucbt[index].senddmabuf;
	ICB.memory_inc 	= DMA_MEMORY_INCREASE_ENABLE;
	ICB.memory_width= DMA_MEMORY_WIDTH_16BIT;
	ICB.number		= DMA_SENDBUF;
	ICB.periph_inc	= DMA_PERIPH_INCREASE_DISABLE;
	ICB.periph_width= DMA_PERIPHERAL_WIDTH_16BIT;
	ICB.priority	= DMA_PRIORITY_ULTRA_HIGH;
  
    switch (index)                                                              /* 允许产生DMA传输完成中断 */
    {
        case USART_NO1:
			dma_deinit(DMA0, DMA_CH3);
            ICB.periph_addr = (INT32U)&USART_DATA(USART0);
			dma_init(DMA0, DMA_CH3, &ICB);
			dma_circulation_disable(DMA0, DMA_CH3);
			dma_memory_to_memory_disable(DMA0, DMA_CH3);
			dma_interrupt_disable(DMA0, DMA_CH3,DMA_INT_FTF);
			dma_channel_disable(DMA0, DMA_CH3);                                   /* 首先禁用，要发送数据时再启用 */
            break;
            
        case USART_NO2:
			dma_deinit(DMA0, DMA_CH6);
            ICB.periph_addr = (INT32U)&USART_DATA(USART1);
			
			dma_circulation_disable(DMA0, DMA_CH6);
			dma_memory_to_memory_disable(DMA0, DMA_CH6);
			dma_init(DMA0, DMA_CH6, &ICB);
			dma_interrupt_disable(DMA0, DMA_CH6,DMA_INT_FTF);
			dma_channel_disable(DMA0, DMA_CH6);                                       /* 首先禁用，要发送数据时再启用 */
            break;
            
        case USART_NO3:
			dma_deinit(DMA0, DMA_CH1);
            ICB.periph_addr = (INT32U)&USART_DATA(USART2);
			dma_init(DMA0, DMA_CH1, &ICB);
			dma_circulation_disable(DMA0, DMA_CH1);
			dma_memory_to_memory_disable(DMA0, DMA_CH1);
			dma_interrupt_disable(DMA0, DMA_CH1,DMA_INT_FTF);
			dma_channel_disable(DMA0, DMA_CH1);                                       /* 首先禁用，要发送数据时再启用 */
            break;

        case USART_NO4:                                    /* 首先禁用，要发送数据时再启用 */
            break;

        case USART_NO5:                                   /* 首先禁用，要发送数据时再启用 */
            break;
            
        default:
            DAL_ASSERT(0);                                                      /* 此处加入出错指示 */
            break;
    }
}

/*******************************************************************
** 函数名:     usartx_rcv_dma_config
** 函数描述:   USARTX_DMA接收参数配置 - 配置完默认是禁用的
** 参数:       [in] index: 串口号
** 返回:       无
********************************************************************/
static void usartx_rcv_dma_config(USART_INDEX_E index)
{
    dma_parameter_struct ICB;
        
	ICB.direction	= DMA_PERIPHERAL_TO_MEMORY;
	ICB.memory_addr = (INT32U)s_ucbt[index].rcvdmabuf;
	ICB.memory_inc 	= DMA_MEMORY_INCREASE_ENABLE;
	ICB.memory_width= DMA_MEMORY_WIDTH_8BIT;
	ICB.number		= DMA_RCVBUF;
	ICB.periph_inc	= DMA_PERIPH_INCREASE_DISABLE;
	ICB.periph_width= DMA_PERIPHERAL_WIDTH_8BIT;
	ICB.priority	= DMA_PRIORITY_ULTRA_HIGH;
  
    switch (index)                                                              /* 允许产生DMA传输完成中断 */
    {
        case USART_NO1:
			ICB.periph_addr = (INT32U)&USART_DATA(USART0);
			
			dma_circulation_enable(DMA0, DMA_CH4);
			dma_memory_to_memory_disable(DMA0, DMA_CH4);
			dma_init(DMA0, DMA_CH4, &ICB);
			usart_dma_receive_config(USART0, USART_DENR_ENABLE);
			dma_transfer_number_config(DMA0, DMA_CH4,DMA_RCVBUF);
			dma_interrupt_enable(DMA0, DMA_CH4,DMA_INT_FTF);
			dma_interrupt_enable(DMA0, DMA_CH4,DMA_INT_HTF);
			dma_channel_enable(DMA0, DMA_CH4);    
            break;
            
        case USART_NO2:
           	ICB.periph_addr = (INT32U)&USART_DATA(USART1);
			dma_circulation_enable(DMA0, DMA_CH5);
			dma_memory_to_memory_disable(DMA0, DMA_CH5);
			dma_init(DMA0, DMA_CH5, &ICB);
			usart_dma_receive_config(USART1, USART_DENR_ENABLE);
			dma_transfer_number_config(DMA0, DMA_CH5,DMA_RCVBUF);
			dma_interrupt_enable(DMA0, DMA_CH5,DMA_INT_FTF);
			dma_interrupt_enable(DMA0, DMA_CH5,DMA_INT_HTF);
			dma_channel_enable(DMA0, DMA_CH5); 
            break;
            
        case USART_NO3:
            ICB.periph_addr = (INT32U)&USART_DATA(USART2);
			dma_circulation_enable(DMA0, DMA_CH2);
			dma_memory_to_memory_disable(DMA0, DMA_CH2);
			dma_init(DMA0, DMA_CH2, &ICB);
			usart_dma_receive_config(USART2, USART_DENR_ENABLE);
			dma_transfer_number_config(DMA0, DMA_CH2,DMA_RCVBUF);
			dma_interrupt_enable(DMA0, DMA_CH2,DMA_INT_FTF);
			dma_interrupt_enable(DMA0, DMA_CH2,DMA_INT_HTF);
			dma_channel_enable(DMA0, DMA_CH2); 
            break;

        case USART_NO4:
            break;

        case USART_NO5:
            break;
            
        default:
            DAL_ASSERT(0);                                                      /* 此处加入出错指示 */
            break;
    }
}

/*******************************************************************
** 函数名:     usartx_wmconfig
** 函数描述:   USARTX的工作模式配置 - 配置完默认是禁用的
** 参数:       [in] index: 串口号
**             [in] usart: 串口参数
** 返回:       无
**  注意事项:  RCC_APB2PeriphClockCmd语句一定要置于其他初始化指令之前
********************************************************************/
static void usartx_wmconfig(USART_INDEX_E index, USART_PARA_T *usart)
{
    switch (index)                                                              /* 允许接收中断，允许发送DMA，禁止串口功能 */
    {
        case USART_NO1:
			usart_baudrate_set(USART0, usart->baud);
			usart_receive_config(USART0,USART_RECEIVE_ENABLE);
			usart_transmit_config(USART0,USART_TRANSMIT_ENABLE);
			usart_dma_receive_config(USART0,USART_DENR_ENABLE);
			usart_dma_transmit_config(USART0,USART_DENT_ENABLE);
			usart_interrupt_disable(USART0,USART_INT_RBNE);
			usart_disable(USART0);
            break;
            
        case USART_NO2:
            usart_baudrate_set(USART1, usart->baud);
			usart_receive_config(USART1,USART_RECEIVE_ENABLE);
			usart_transmit_config(USART1,USART_TRANSMIT_ENABLE);
			usart_dma_receive_config(USART1,USART_DENR_ENABLE);
			usart_dma_transmit_config(USART1,USART_DENT_ENABLE);
			usart_interrupt_disable(USART1,USART_INT_RBNE);
			//usart_interrupt_enable(USART1,USART_INT_IDLE);
			usart_disable(USART1);
            break;
            
        case USART_NO3:
            usart_baudrate_set(USART2, usart->baud);
			usart_receive_config(USART2,USART_RECEIVE_ENABLE);
			usart_transmit_config(USART2,USART_TRANSMIT_ENABLE);
			usart_dma_receive_config(USART2,USART_DENR_ENABLE);
			usart_dma_transmit_config(USART2,USART_DENT_ENABLE);
			usart_interrupt_disable(USART2,USART_INT_RBNE);
			//usart_interrupt_enable(USART2,USART_INT_IDLE);
			usart_disable(USART2);
            break;

        case USART_NO4:
            break;

        case USART_NO5:             
            break;
            
        default:
            DAL_ASSERT(0);                                                      /* 此处加入出错指示 */
            break;
    }
    
    s_ucbt[index].status = STATUS_DISABLE;                                      /* 初始化完成后默认置于禁用状态 */
}

/*******************************************************************
** 函数名:     usartx_delaytmrproc
** 函数描述:   定时扫描DMA接收缓存残留数据
** 参数:       无
** 返回:       无
********************************************************************/
static void usartx_delaytmrproc(void)
{
    INT16S readlen = 0;
    INT8U NDTR  = 0;
    USART_INDEX_E i = USART_NO1;
   
    for (i = USART_NO1; i <= USART_NO5; i++) {
        if (s_ucbt[i].status == STATUS_DISABLE || s_ucbt[i].status == STATUS_INITING) {
            continue;
        }
        switch(i) {
            case USART_NO1:
				NDTR = dma_transfer_number_get(DMA0, DMA_CH4);
                break;
            case USART_NO2:
                NDTR = dma_transfer_number_get(DMA0, DMA_CH5);
                break;
            case USART_NO3:
                NDTR = dma_transfer_number_get(DMA0, DMA_CH2);
                break;
            case USART_NO4:
                break;
            case USART_NO5:
                break;
            default:
                continue;
        }
        ENTER_CRITICAL();
        if (s_ucbt[i].dmairq == true) {
            s_ucbt[i].dmairq   = false;
            s_ucbt[i].oldlocat = NDTR;
            EXIT_CRITICAL() ;
            continue;                                                           /* 在与前次扫描间有进过中断，则退出 */
        }
        if (s_ucbt[i].oldlocat == NDTR) {                                       /* 一个定时时间内没收到新数据 */
            s_ucbt[i].timerread = true;
        } else {
            s_ucbt[i].timerread = false;
        }
        EXIT_CRITICAL();

        if (s_ucbt[i].timerread == true) {                                      /* 一个定时时间内没收到新数据 */
            readlen = (DMA_RCVBUF - NDTR - s_ucbt[i].readlocat);
            if (readlen == 0) {                                                 /* 没收到新数据，数据已读过 */
                s_ucbt[i].timerread = false;
                s_ucbt[i].oldlocat = NDTR;
                continue;
            } else if (readlen < 0) {                                           /* 缓存尾部有数据未读走 */
                readlen = DMA_RCVBUF - s_ucbt[i].readlocat;
                WriteBlockRoundBuf(&s_ucbt[i].recvrb, &(s_ucbt[i].rcvdmabuf[s_ucbt[i].readlocat]), readlen);
                s_ucbt[i].readlocat = 0;
                readlen = (DMA_RCVBUF - NDTR - s_ucbt[i].readlocat);
            }
            WriteBlockRoundBuf(&s_ucbt[i].recvrb, &(s_ucbt[i].rcvdmabuf[s_ucbt[i].readlocat]), readlen);
            s_ucbt[i].readlocat +=  (INT8U )readlen;
        }
        s_ucbt[i].timerread = false;
        s_ucbt[i].oldlocat  = NDTR;
    }
    if (s_send485statu) {
        if (++s_485_receive_cnt > 300) {
            s_send485statu = false;
        }
    }
}

/*******************************************************************
** 函数名:     USARTX_Initiate
** 函数描述:   初始化USARTX(此处也配置了DMA) - 初始化完后默认是禁用的
** 参数:       [in] index: 串口号
**             [in] usart: 串口参数
**             [in] buft:  串口缓冲区
** 返回:       无
**  备注    :  UART1~UART4有带DMA，UART5不带DMA，配置上有所差别(如果使用DMA发送情况下要注意)
********************************************************************/
void USARTX_Initiate(USART_INDEX_E index, USART_PARA_T *usart, USART_BUF_T *buft)
{
    DAL_ASSERT(index < USART_MAX);
    DAL_ASSERT(buft != PNULL);

    s_ucbt[index].status = STATUS_INITING; 
    s_ucbt[index].parity = usart->parity;
       
	InitRoundBuf(&s_ucbt[index].recvrb, buft->rbuf, buft->rlen, 0);
    InitRoundBuf(&s_ucbt[index].sendrb, buft->sbuf, buft->slen, 0);

    switch (index)                                                              /* 指定串口发送和接收中断的自定义处理函数 */
    {
        case USART_NO1:
            s_ucbt[index].timerread = false;
            s_ucbt[index].dmairq    = true;
            s_ucbt[index].oldlocat  = DMA_RCVBUF;
            s_ucbt[index].readlocat = 0; 
            s_ucbt[index].rcvdmabuf = s_phy1_dmarecvbuf;
            s_ucbt[index].senddmabuf= s_phy1_dmasendbuf;

            usartx_wmconfig(index, usart);                                      /* 配置USART工作模式 */
            usartx_rcv_dma_config(index);
            usartx_send_dma_config(index);

            NVIC_IrqHandleInstall(DMA0_C4_IRQ, USER_DMA0_Stream4_IRQHandler, UART_TXDMA_PRIOTITY, true);
            NVIC_IrqHandleInstall(DMA0_C3_IRQ, USER_DMA0_Stream3_IRQHandler, UART_TXDMA_PRIOTITY, true); 
            break;
            
        case USART_NO2:
            s_ucbt[index].timerread = false;
            s_ucbt[index].dmairq    = true;
            s_ucbt[index].oldlocat  = DMA_RCVBUF;
            s_ucbt[index].readlocat = 0;
            s_ucbt[index].rcvdmabuf = s_phy2_dmarecvbuf;
            s_ucbt[index].senddmabuf= s_phy2_dmasendbuf;
            
            usartx_wmconfig(index, usart);                                      /* 配置USART工作模式 */
            usartx_rcv_dma_config(index);
            usartx_send_dma_config(index);

            NVIC_IrqHandleInstall(DMA0_C5_IRQ, USER_DMA0_Stream5_IRQHandler, UART_TXDMA_PRIOTITY, true);
            NVIC_IrqHandleInstall(DMA0_C6_IRQ, USER_DMA0_Stream6_IRQHandler, UART_TXDMA_PRIOTITY, true);
            
            break;
            
        case USART_NO3:
            s_ucbt[index].timerread = false;
            s_ucbt[index].dmairq    = true;
            s_ucbt[index].oldlocat  = DMA_RCVBUF;
            s_ucbt[index].readlocat = 0; 
            s_ucbt[index].rcvdmabuf = s_phy3_dmarecvbuf;
            s_ucbt[index].senddmabuf= s_phy3_dmasendbuf;
            s_idledetectable        = FALSE;
            
            usartx_wmconfig(index, usart);                                      /* 配置USART工作模式 */
            usartx_rcv_dma_config(index);
            usartx_send_dma_config(index);
            
            NVIC_IrqHandleInstall(DMA0_C1_IRQ, USER_DMA0_Stream1_IRQHandler, UART_TXDMA_PRIOTITY, true);
            NVIC_IrqHandleInstall(DMA0_C2_IRQ, USER_DMA0_Stream2_IRQHandler, UART_TXDMA_PRIOTITY, true);
            //NVIC_IrqHandleInstall(USART3_IRQ, USER_USART3_IRQHandler, UART_RX_PRIOTITY, true);
            break;
            
        
        default:
            DAL_ASSERT(0);                                                      /* 此处加入出错指示 */
            break;
    }

    if (s_tmrcreatsatu == false) {
        s_Delaytmrid = CreateTimer(usartx_delaytmrproc);
        s_tmrcreatsatu = true;
    }
    StartTimer(s_Delaytmrid, _TICK, 1, TRUE);
}

/*******************************************************************
** 函数名:     USARTX_IOConfig
** 函数描述:   USARTX的IO口配置
** 参数:       [in] index: 串口号
** 返回:       无
********************************************************************/
void USARTX_IOConfig(USART_INDEX_E index)
{
    DAL_ASSERT(index < USART_MAX);

    switch (index)                                                              /* 配置串口管脚并锁定配置 */
    {
        case USART_NO1:
			rcu_periph_clock_enable(RCU_USART0);
			//TX
			gpio_init(USART1_PIN_IO,GPIO_MODE_AF_PP,GPIO_OSPEED_50MHZ,USART1_PIN_TX);
			//RX
			gpio_init(USART1_PIN_IO,GPIO_MODE_IN_FLOATING,GPIO_OSPEED_50MHZ,USART1_PIN_RX);
            break;
            
        case USART_NO2:
            rcu_periph_clock_enable(RCU_USART1);
			usart_hardware_flow_cts_config(USART1,USART_CTS_ENABLE);
			usart_hardware_flow_rts_config(USART1,USART_RTS_ENABLE);
			//TX
			gpio_init(USART2_PIN_IO,GPIO_MODE_AF_PP,GPIO_OSPEED_50MHZ,USART2_PIN_TX);
			gpio_init(USART2_PIN_IO,GPIO_MODE_AF_PP,GPIO_OSPEED_50MHZ,USART2_PIN_RTX);
			//RX
			gpio_init(USART2_PIN_IO,GPIO_MODE_IN_FLOATING,GPIO_OSPEED_50MHZ,USART2_PIN_RX);
			gpio_init(USART2_PIN_IO,GPIO_MODE_IN_FLOATING,GPIO_OSPEED_50MHZ,USART2_PIN_CTX);
			break;
            
        case USART_NO3:
			rcu_periph_clock_enable(RCU_GPIOD);
            rcu_periph_clock_enable(RCU_USART2);
			rcu_periph_clock_enable(RCU_AF);
			gpio_pin_remap_config(GPIO_USART2_FULL_REMAP,ENABLE);
			//TX
			gpio_init(USART3_PIN_IO,GPIO_MODE_AF_PP,GPIO_OSPEED_50MHZ,USART3_PIN_TX);
			//RX
			gpio_init(USART3_PIN_IO,GPIO_MODE_IN_FLOATING,GPIO_OSPEED_50MHZ,USART3_PIN_RX);
            break;

        case USART_NO4:
            rcu_periph_clock_enable(RCU_UART3);
			//TX
			gpio_init(USART3_PIN_IO,GPIO_MODE_AF_PP,GPIO_OSPEED_50MHZ,USART3_PIN_TX);
			//RX
			gpio_init(USART3_PIN_IO,GPIO_MODE_IN_FLOATING,GPIO_OSPEED_50MHZ,USART3_PIN_RX);
            break;
            
        case USART_NO5:                                                         /* UART5比较特殊，收与发不属于同一个GPIO口 */
            rcu_periph_clock_enable(RCU_UART4);
			//TX
			gpio_init(USART4_PIN_IO,GPIO_MODE_AF_PP,GPIO_OSPEED_50MHZ,USART4_PIN_TX);
			//RX
			gpio_init(USART4_PIN_IO,GPIO_MODE_IN_FLOATING,GPIO_OSPEED_50MHZ,USART4_PIN_RX);
            break;
            
        default:
            DAL_ASSERT(0);                                                      /* 此处加入出错指示 */
            break;
    }
}


/*******************************************************************
** 函数名:     USARTX_Enable
** 函数描述:   启用USARTX
** 参数:       [in] index: 串口号
** 返回:       无
********************************************************************/
void USARTX_Enable(USART_INDEX_E index)
{
    DAL_ASSERT(index < USART_MAX);

    switch (index)                                                              /* 启动相应串口功能 */
    {
        case USART_NO1:
            usart_enable(USART0);
            break;
            
        case USART_NO2:
            usart_enable(USART1);
            break;
            
        case USART_NO3:
			usart_enable(USART2);
            break;

        case USART_NO4:
			usart_enable(UART3);
            break;
            
        case USART_NO5:
			usart_enable(UART4);
            break;
            
        default:
            DAL_ASSERT(0);
            break;
    }
    
    s_ucbt[index].status = STATUS_IDLENOW;
}

/*******************************************************************
** 函数名:     USARTX_Disable
** 函数描述:   停用USARTX
** 参数:       [in] index: 串口号
** 返回:       无
********************************************************************/
void USARTX_Disable(USART_INDEX_E index)
{
    DAL_ASSERT(index < USART_MAX);

    s_ucbt[index].status = STATUS_DISABLE;

    switch (index)                                                              /* 停止相应串口功能 */
    {
        case USART_NO1:
            usart_disable(USART0);
            break;
            
        case USART_NO2:
            usart_disable(USART1);
            break;
            
        case USART_NO3:
            usart_disable(USART2);
            break;

        case USART_NO4:
			usart_disable(UART3);
            break;
            
        case USART_NO5:
			usart_disable(UART4);
            break;
            
        default:
            DAL_ASSERT(0);
            break;
    }
}

/*******************************************************************
** 函数名:     USARTX_LeftofBuf
** 函数描述:   获取指定串口的发送缓冲区剩余空间
** 参数:       [in] index: 串口号
** 返回:       剩余空间长度
********************************************************************/
INT32S USARTX_LeftofBuf(USART_INDEX_E index)
{
    DAL_ASSERT(index < USART_MAX);

    if ((s_ucbt[index].status != STATUS_IDLENOW) && (s_ucbt[index].status != STATUS_SENDING)) {
        return -1;
    } else {
        return LeftOfRoundBuf(&s_ucbt[index].sendrb);
    }
}

/*******************************************************************
** 函数名:     USARTX_ReadByte
** 函数描述:   USARTX读取单个字节
** 参数:       [in] index: 串口号
** 返回:       剩余空间长度
********************************************************************/
INT32S USARTX_ReadByte(USART_INDEX_E index)
{
    DAL_ASSERT(index < USART_MAX);

    return  ReadRoundBuf(&s_ucbt[index].recvrb);
}

/*******************************************************************
** 函数名:     USARTX_SendByte
** 函数描述:   USARTX发送单个字节
** 参数:       [in] index: 串口号
**             [in] byte: 字节数据
** 返回:       无
********************************************************************/
BOOLEAN USARTX_SendByte(USART_INDEX_E index, INT8U byte)
{
    DAL_ASSERT(index < USART_MAX);

    if (WriteRoundBuf(&s_ucbt[index].sendrb, byte)) {
        if (s_ucbt[index].status == STATUS_IDLENOW) {                           /* 处于空闲状态，调用开始发送 */
            return usartx_startsend(index);
        } else if (s_ucbt[index].status == STATUS_SENDING) {                    /* 处于发送状态 */
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

/*******************************************************************
** 函数名:     USARTX_SendData
** 函数描述:   USARTX发送连续数据
** 参数:       [in] index: 串口号
**             [in] ptr:   数据指针
**             [in] tlen:  数据长度
** 返回:       发送结果
********************************************************************/
BOOLEAN USARTX_SendData(USART_INDEX_E index, INT8U *ptr, INT32U tlen)
{
    DAL_ASSERT(index < USART_MAX);
    
    if ((ptr == 0) || (tlen == 0)) {
        return false;
    }

    if (WriteBlockRoundBuf(&s_ucbt[index].sendrb, ptr, tlen)) {
        if (s_ucbt[index].status == STATUS_IDLENOW) {                           /* 处于空闲状态，调用开始发送 */
            if (index == USART_NO5) {
                if (usart_flag_get(UART4, USART_FLAG_IDLE) || s_send485statu == false) {
                    s_lbapp[LB_485_TOSEND].lbhandle();
                    s_send485statu = true;
                    s_485_receive_cnt = 0;
                } else {   
                    usart_interrupt_enable(UART4, USART_INT_IDLE); 
                    return true;
                }
            }
            #if 0
            if (index == USART_NO3) {
                if (USART_GetFlagStatus(USART3, USART_FLAG_IDLE) || (!s_idledetectable)) {
                    USART_Rx_Contrl(USART3, DISABLE);
                } else {   
                    USART_ITConfig(USART3, USART_IT_IDLE, ENABLE); 
                    return true;
                }
            }
            #endif
            return usartx_startsend(index);
        } else if (s_ucbt[index].status == STATUS_SENDING) {                    /* 处于发送状态 */
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

/*******************************************************************
** 函数名:     SetMarkOrSpace
** 函数描述:   设置为MARK或SPACE校验
** 参数:       [in] index:  串口号
**             [in] parity: 校验类型
** 返回:       无
********************************************************************/
void SetMarkOrSpace(USART_INDEX_E index,USART_PARITY_E parity)
{
    DAL_ASSERT(parity == PARITY_MARK || parity == PARITY_SPACE);
    if ( parity == PARITY_MARK ) {
        s_ucbt[index].parity = PARITY_MARK;
    } else if ( parity == PARITY_SPACE ) {
        s_ucbt[index].parity = PARITY_SPACE;
    }
}

/*******************************************************************
** 函数名:     Dal_UsartLBRepReg
** 函数描述:   Usart回调上报函数
** 参数:       [in] index:  串口号
**             [in] handle: 指向APP层的函数指针
** 返回:       无
********************************************************************/
void Dal_UsartLBRepReg(USART_LBINDEX_E index, void (* handle) (void))
{
    s_lbapp[index].lbhandle = handle;
}

/**************************************************************************************************
**  函数名称:  Setidledetectable
**  功能描述:   
**  输入参数:   
**  返回参数:  None
**************************************************************************************************/
void Setidledetectable(BOOLEAN statu)
{
    s_idledetectable = statu;
}

/**************************** (C) COPYRIGHT 2012  XIAMEN YAXON.LTD **************END OF FILE******/

