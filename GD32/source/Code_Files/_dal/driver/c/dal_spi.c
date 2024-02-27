/**************************************************************************************************
**                                                                                               **
**  文件名称:  dal_spi.c                                                                         **
**  版权所有:  CopyRight @ Xiamen Yaxon NetWork CO.LTD. 2012                                     **
**  创建信息:                                                                                    **
**  文件描述:  SPI驱动接口函数                                                                   **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/
#include "dal_include.h"
#include "tools.h"
#include "roundbuf.h"
#include "dmemory.h"
#include "man_timer.h"
#include "man_irq.h"
#include "man_error.h"
#include "dal_spi.h"
#include "dal_pinlist.h"
#include "dal_hard.h"
#include "dal_gpio.h"
#include "debug_print.h"

#if EN_DEBUG > 1
#define DEBGU_SPI                   1
#else
#define DEBGU_SPI                   0
#endif

#define DATASIZE_8b                 0x01                                       /* 数据宽度8位 */
#define DATASIZE_16b                0x02                                       /* 数据宽度16位 */
#define SPI_DATASIZE                DATASIZE_8b                                /* 在这里选择SPI数据宽度 */

#define DMA_RCVBUF                  255                                        /* DMA接收缓存 */
#define DMA_SENDBUF                 255                                        /* DMA发送缓存 */

#define SPI_ROUNDBUF_TX_LEN         512                                       /*  发送缓冲  */
#define SPI_ROUNDBUF_RX_LEN         512                                       /*  接收缓冲  */

/* SPI_IND_1 */
#define SPI_IDX1_PORT               SPI0
#define SPI_IDX1_DMAx				DMA0
#define SPI_IDX1_RX_DMA_STREAM      DMA_CH1
#define SPI_IDX1_TX_DMA_STREAM      DMA_CH2

/* SPI_IND_2 */
#define SPI_IDX2_PORT               SPI2
#define SPI_IDX2_DMAx				DMA1
#define SPI_IDX2_RX_DMA_STREAM      DMA_CH0
#define SPI_IDX2_TX_DMA_STREAM      DMA_CH1
//#define SPI_IDX2_TX_CH            DMA_Channel_4
//#define SPI_IDX2_RX_CH            DMA_Channel_4

/*************************************************************************************************/
/*                           SPI发送状态枚举                                                    */
/*************************************************************************************************/
typedef enum {
    SPI_STATUS_CLOSED = 0x00,                                                  /* 串口被关闭 */
    SPI_STATUS_DISABLE,                                                        /* 串口被禁用 */
    SPI_STATUS_INITING,                                                        /* 串口初始化 */
    SPI_STATUS_SENDING,                                                        /* 串口正忙 */
    SPI_STATUS_IDLENOW                                                         /* 串口空闲 */
} SPI_SS_E;

/*************************************************************************************************/
/*                           模块结构体定义                                                      */
/*************************************************************************************************/
typedef struct {
    volatile SPI_SS_E   status;                                                 /* 当前工作状态 */
    
    INT8U               *p_rx;                                                  /* 接收环形缓冲区内存指针 */
    INT8U               *p_tx;                                                  /* 发送环形缓冲区内存指针 */
    INT32U              tx_len;                                                 /* 接收环形缓冲区大小 */
    INT32U              rx_len;                                                 /* 发送环形缓冲区大小 */
    ROUNDBUF_T          recvrb;                                                 /* 接收环形缓冲区 */
    ROUNDBUF_T          sendrb;                                                 /* 发送环形缓冲区 */
    
    SPI_DMA_MODE_E      dma_en;                                                 /* 当前工作状态 */
    
    BOOLEAN             dmairq;                                                 /* 是否进入过DMA中断 */
    BOOLEAN             timerread;                                              /* 定时器正在考备数据，若此时进DMA中断则直接退出 */
    INT16U              oldlocat;                                               /* 上回DMA写位置 */
    INT8U               readlocat;                                              /* 读位置 */
    INT8U               *rcvdmabuf;                                             /* DMA接收缓存 */   //ry
    INT16U              *senddmabuf;                                            /* DMA发送缓存 */
    INT16U              spifirstbit;                                            /* MSB or LSB bit */
} SPI_CTRL_T;

/*************************************************************************************************/
/*                           模块静态变量定义                                                    */
/*************************************************************************************************/
static INT8U       s_spi_dmarecvbuf[MAX_SPI_IND][DMA_RCVBUF];                               /* 接收缓冲区 */  // ry
static INT16U      s_spi_dmasendbuf[MAX_SPI_IND][DMA_SENDBUF];                              /* 发送缓冲区 */
static SPI_CTRL_T  s_scbt[MAX_SPI_IND];                                                     /* SPI控制块 */
static SPI_CALLBACK_T  s_spi_callback[MAX_SPI_IND];
static INT32U      s_spi_tmr_id;          /* 延时判断定时计数 */
static BOOLEAN     s_is_creat = false;    /* 定时器是否已经创建 */
//static BOOLEAN     s_spi_idle;            /* 判断是否可以检测总线空闲 */

static INT32U s_spi_port[MAX_SPI_IND] 			= {SPI_IDX1_PORT, SPI_IDX2_PORT};
static INT32U s_spi_dma_periph[MAX_SPI_IND]		= {SPI_IDX1_DMAx,SPI_IDX2_DMAx};
static INT8U  s_spi_dma_rx_stream[MAX_SPI_IND] 	= {SPI_IDX1_RX_DMA_STREAM, SPI_IDX2_RX_DMA_STREAM};
static INT8U  s_spi_dma_tx_stream[MAX_SPI_IND]  = {SPI_IDX1_TX_DMA_STREAM, SPI_IDX2_TX_DMA_STREAM};

/**************************************************************************************************
**  函数名称:  WriteSPI
**  功能描述:  向SPI2写一个字节数据
**  输入参数:  输出数据
**  返回参数:  无
**************************************************************************************************/
static INT32S WriteSPI(SPI_IND_E idx, INT8U DataOut)
{
    
	INT8U retry = 0;
	
	while (spi_i2s_flag_get(s_spi_port[idx], SPI_FLAG_TBE) == RESET) { //检查指定的SPI标志位设置与否:发送缓存空标志位
		retry++;
		if (retry > 200) {
			return -1;
		}
	}
	spi_i2s_data_transmit(s_spi_port[idx], (INT8U)DataOut); //通过外设SPIx发送一个数据

	retry = 0;
	while (spi_i2s_flag_get(s_spi_port[idx], SPI_FLAG_RBNE) == RESET) { //检查指定的SPI标志位设置与否:接受缓存非空标志位
		retry++;
		if (retry > 200) {
			return -1;
		}
	}
	return spi_i2s_data_receive(s_spi_port[idx]); //返回通过SPIx最近接收的数据
}

/**************************************************************************************************
**  函数名称:  ReadSPI
**  功能描述:  从SPI读取一个字节数据
**  输入参数:  无
**  返回参数:  输入数据
**************************************************************************************************/
static INT32S ReadSPI(SPI_IND_E idx)
{
    return WriteSPI(idx, 0x00); //返回通过SPIx最近接收的数据
}

INT32U read_roundbuf(SPI_IND_E idx)
{
	return UsedOfRoundBuf(&s_scbt[idx].sendrb);
}

/*******************************************************************
** 函数名:     start_send
** 函数描述:   启动SPIX发送数据
** 参数:       无
** 返回:       无
********************************************************************/
static BOOLEAN start_send(SPI_IND_E idx)
{
    INT32S  temp = 0;
    INT32U  tlen, i, blen, k;
    INT16U  *tptr;
#if SPI_DATASIZE == DATASIZE_16b
    INT8U   j = 0;
    INT8U   wrbuff[2] = {0xFF, 0xFF};
    INT16U  sword = 0;
#endif

    tlen = UsedOfRoundBuf(&s_scbt[idx].sendrb);
    if (tlen <= 0)  {
        return FALSE;
    }

    s_scbt[idx].status = SPI_STATUS_SENDING;                                      /* 置于发送状态 */

    if (s_scbt[idx].dma_en == SPI_DMA_MODE_DISABLE) {
        #if EN_DEBUG > 1
        Debug_SysPrint("start_send 中断发送\r\n");
        #endif

		#if SPI_DATASIZE == DATASIZE_16b
        sword = 0;
		if (s_scbt[idx].spifirstbit == SPI_ENDIAN_MSB) {
			for (j = 0; j < 2; j++) {
				wrbuff[j] = ReadRoundBuf(&s_scbt[idx].sendrb);
				sword = (sword << 8) + wrbuff[j];
			}
		} else {
			for (j = 0; j < 2; j++) {
				wrbuff[j] = ReadRoundBuf(&s_scbt[idx].sendrb);
				sword = sword + ((INT16U)wrbuff[j] << ((INT16U)8U*j));
			}
		}
        spi_i2s_data_transmit(s_spi_port[idx], sword);
        spi_i2s_interrupt_enable(s_spi_port[idx], SPI_I2S_INT_TBE);
        #else
		#if 0
        for (i = 0; i <  tlen; i++) {
			temp = ReadRoundBuf(&s_scbt[idx].sendrb);
			SPI_SendData(s_spi_port[idx], (INT16U)temp);
			k = 0;
			while (!SPI_I2S_GetFlagStatus(s_spi_port[idx], SPI_I2S_FLAG_TXE)) {
                if (k++ >= 4000) {
                    break;
				}
			}
		}
		
		s_scbt[idx].status = SPI_STATUS_IDLENOW;  
		#else
        temp = ReadRoundBuf(&s_scbt[idx].sendrb);
        spi_i2s_data_transmit(s_spi_port[idx], (INT16U)temp);
        spi_i2s_interrupt_enable(s_spi_port[idx],SPI_I2S_INT_TBE);
		#endif
        #endif
    } else {
        #if EN_DEBUG > 1
        Debug_SysPrint("start_send DMA发送\r\n");
        //Debug_PrintHex(TRUE, (INT8U *)s_scbt[idx].senddmabuf, tlen);
        #endif
        tptr = s_scbt[idx].senddmabuf;
		#if 0
        blen = DMA_SENDBUF < (tlen + 4) ? DMA_SENDBUF : (tlen + 4);
        *tptr++ = 0xFF;
        *tptr++ = 0x5A;
        *tptr++ = 0xA5;
        *tptr++ = blen-4;
        DMA_ClearFlag(s_spi_dma_tx_stream[idx], DMA_FLAG_TCIF4);
        DMA_Cmd(s_spi_dma_tx_stream[idx], DISABLE);
		
        for (i = 0; i < blen - 4; i++) {                                        /* 依次复制每一个数据 */
            temp = ReadRoundBuf(&s_scbt[idx].sendrb);
            *tptr++ = temp;
        }		
	    #else
        blen = DMA_SENDBUF < (tlen) ? DMA_SENDBUF : (tlen);
        dma_flag_clear(s_spi_dma_periph[idx],s_spi_dma_tx_stream[idx], DMA_INT_FTF);
        dma_channel_disable(s_spi_dma_periph[idx],s_spi_dma_tx_stream[idx]);
		
        for (i = 0; i < blen; i++) {                                        /* 依次复制每一个数据 */
            temp = ReadRoundBuf(&s_scbt[idx].sendrb);
            *tptr++ = temp;
        }		
		#endif
        dma_transfer_number_config(s_spi_dma_periph[idx],s_spi_dma_tx_stream[idx],blen);
		dma_interrupt_enable(s_spi_dma_periph[idx],s_spi_dma_tx_stream[idx],DMA_INT_FTF);
		dma_channel_enable(s_spi_dma_periph[idx],s_spi_dma_tx_stream[idx]);
    }

    return TRUE;
}

/**************************************************************************************************
**  函数名称:  USER_SPI0_IRQHandler
**  功能描述:  SPI0的中断处理函数 - 处理接和发送中断
**  输入参数:
**  输出参数:
**************************************************************************************************/
static __attribute__ ((section ("IRQ_HANDLE"))) void USER_SPI0_IRQHandler(void) __irq
{
    INT32U tlen;
    INT16U rword;
	
    #if SPI_DATASIZE == DATASIZE_16b
    INT16S j;
    INT8U  rcbuff[2] = {0xFF, 0xFF};
    #endif

    //ENTER_CRITICAL();

    /* 处理接收中断 */
    if (spi_i2s_interrupt_flag_get(SPI_IDX1_PORT, SPI_I2S_INT_FLAG_RBNE) != RESET) {
		
	    #if SPI_DATASIZE == DATASIZE_16b	/* 16位数据宽度 */
        rword = spi_i2s_data_receive(SPI_IDX1_PORT);
        if (s_scbt[SPI_IND_1].spifirstbit == SPI_ENDIAN_MSB) { // Msb first, rxBuff始终应为大端模式，即高位字节在前地位字节在后
            for (j= 1; j >= 0; j--) {
            	rcbuff[j] = (INT8U)(rword >> ((INT8U)j*(INT8U)8));
                WriteRoundBuf(&s_scbt[SPI_IND_1].recvrb, rcbuff[j]);
                if (s_spi_callback[SPI_IND_1].callback_spiintrxmsg != NULL) {
                    s_spi_callback[SPI_IND_1].callback_spiintrxmsg(rcbuff[j]);
                }
            }
        } else {
            for (j = 0; j < 2; j++) {
            	rcbuff[j] = (INT8U)(rword >> ((INT8U)j*(INT8U)8));
                WriteRoundBuf(&s_scbt[SPI_IND_1].recvrb, rcbuff[j]);
                if (s_spi_callback[SPI_IND_1].callback_spiintrxmsg != NULL) {
                    s_spi_callback[SPI_IND_1].callback_spiintrxmsg(rcbuff[j]);
                }
            }
        }
	    #else	 /* 8位数据宽度 */
        rword = spi_i2s_data_receive(SPI_IDX1_PORT);
        WriteRoundBuf(&s_scbt[SPI_IND_1].recvrb, (INT8U)rword);
        if (s_spi_callback[SPI_IND_1].callback_spiintrxmsg != NULL) {
            s_spi_callback[SPI_IND_1].callback_spiintrxmsg((INT8U)rword);
        }
	    #endif
        //SPI_ClearITPendingBit(SPI_IDX1_PORT, SPI_IT_RXNE);                         /* 清除标志位 */
        
    }

	/* 处理发送中断 */
    if (spi_i2s_interrupt_flag_get(SPI_IDX1_PORT, SPI_I2S_INT_FLAG_TBE) != RESET) {                     /* 发送完成中断 */
        //SPI_ITConfig(SPI_IDX2_PORT, SPI_IT_TXE, DISABLE);
        tlen = UsedOfRoundBuf(&s_scbt[SPI_IND_1].sendrb);                      /* 判断roundbuf中是否有数据要发送 */
        if (tlen <= 0) {
            spi_i2s_data_transmit(SPI_IDX1_PORT, 0xFFFF); /* 无效数据设置为0xFFFF。不设置的情况下MISO 数据线的数据为最后发送的2字节数据，带有7E，对主机数据解析会有影响 */
            s_scbt[SPI_IND_1].status = SPI_STATUS_IDLENOW;                         /* 置于发送结束状态 */
        } else {
            start_send(SPI_IND_1);
        }

        //SPI_ClearITPendingBit(SPI_IDX1_PORT, SPI_IT_TXE);                         /* 清除标志位 */
    }

    if (spi_i2s_interrupt_flag_get(s_spi_port[SPI_IND_1], SPI_I2S_INT_FLAG_FERR) != RESET) {                     /* */
        //SPI_ClearITPendingBit(s_spi_port[SPI_IND_1], SPI_IT_ERR);                         /* 清除标志位 */
    }

    if (spi_i2s_interrupt_flag_get(s_spi_port[SPI_IND_1], SPI_I2S_INT_FLAG_RXORERR) != RESET) {                     /* */
        //SPI_ClearITPendingBit(s_spi_port[SPI_IND_1], SPI_IT_OVR);                         /* 清除标志位 */
    }
    

    //EXIT_CRITICAL();
}

/*******************************************************************
** 函数名:     USER_DMA1_Stream0_IRQHandler
** 函数描述:   SPI0的DMA中断处理 - 仅处理接收中断
** 参数:       无
** 返回:       无
********************************************************************/
static __attribute__ ((section ("IRQ_HANDLE"))) void USER_DMA1_Stream0_IRQHandler(void) __irq
{
    INT16S readlen = 0;
    INT16S j;
    INT32U ndtr;


    if (dma_interrupt_flag_get(s_spi_dma_periph[SPI_IND_1],s_spi_dma_rx_stream[SPI_IND_1],DMA_INT_FLAG_HTF)) {                          /* 半发送中断 */
        #if DEBGU_SPI > 0
        //Debug_SysPrint("HNDTR:0x%x\r\n", SPI_IDX2_RX_DMA_STREAM->NDTR);
        #endif

        dma_interrupt_flag_clear(s_spi_dma_periph[SPI_IND_1],s_spi_dma_rx_stream[SPI_IND_1],DMA_INT_FLAG_HTF);

        //while(SPI_I2S_GetFlagStatus(SPI_IDX2_PORT,SPI_I2S_FLAG_BSY) != RESET){}
        if (s_scbt[SPI_IND_1].timerread == TRUE) {
            //DMA_ClearITPendingBit(SPI_IDX2_RX_DMA_STREAM, DMA_IT_HTIF3);
            return;                        /* 定时器正在考备数据 */
        }

        readlen = (DMA_RCVBUF - s_scbt[SPI_IND_1].readlocat - dma_transfer_number_get(s_spi_dma_periph[SPI_IND_1],s_spi_dma_rx_stream[SPI_IND_1]));
        if (readlen < 0) {                                                      /* 缓存尾部有数据未读走 */
            readlen = DMA_RCVBUF - s_scbt[SPI_IND_1].readlocat;
			if(!WriteBlockRoundBuf(&s_scbt[SPI_IND_1].recvrb, (INT8U *)&(s_scbt[SPI_IND_1].rcvdmabuf[s_scbt[SPI_IND_1].readlocat]), readlen)){
			}
            for (j = 0; j < readlen; j++) {
                if (s_spi_callback[SPI_IND_1].callback_spiintrxmsg != NULL) {
                    s_spi_callback[SPI_IND_1].callback_spiintrxmsg(s_scbt[SPI_IND_1].rcvdmabuf[s_scbt[SPI_IND_1].readlocat + j]);
                }
            }
            s_scbt[SPI_IND_1].readlocat = 0;
            readlen = (DMA_RCVBUF - dma_transfer_number_get(s_spi_dma_periph[SPI_IND_1],s_spi_dma_rx_stream[SPI_IND_1]) - s_scbt[SPI_IND_1].readlocat);
        }
		if(!WriteBlockRoundBuf(&s_scbt[SPI_IND_1].recvrb, (INT8U *)&(s_scbt[SPI_IND_1].rcvdmabuf[s_scbt[SPI_IND_1].readlocat]), readlen)){

		}
        for (j = 0; j < readlen; j++) {
            if (s_spi_callback[SPI_IND_1].callback_spiintrxmsg != NULL) {
                s_spi_callback[SPI_IND_1].callback_spiintrxmsg(s_scbt[SPI_IND_1].rcvdmabuf[s_scbt[SPI_IND_1].readlocat + j]);
            }
        }
        s_scbt[SPI_IND_1].readlocat += readlen;
        s_scbt[SPI_IND_1].dmairq = TRUE;
        //DMA_ClearITPendingBit(s_spi_dma_rx_stream[SPI_IND_2], DMA_IT_HTIF3);
    }
	
    if (dma_interrupt_flag_get(s_spi_dma_periph[SPI_IND_1],s_spi_dma_rx_stream[SPI_IND_1],DMA_INT_FLAG_FTF)) {                          /* 完全发送中断*/
        /*
        DMA_ClearITPendingBit(SPI_IDX2_RX_DMA_STREAM, DMA_IT_TCIF3);
        DMA_ITConfig(SPI_IDX2_RX_DMA_STREAM, DMA_IT_TC, DISABLE);
        WriteBlockRoundBuf(&s_scbt[idx].recvrb, s_scbt[idx].rcvdmabuf, DMA_RCVBUF);
        DMA_ITConfig(SPI_IDX2_RX_DMA_STREAM, DMA_IT_TC, ENABLE);
        DMA_Cmd(SPI_IDX2_RX_DMA_STREAM, DISABLE);
        DMA_Cmd(SPI_IDX2_RX_DMA_STREAM, ENABLE);
        return;*/

        
        dma_interrupt_flag_clear(s_spi_dma_periph[SPI_IND_1],s_spi_dma_rx_stream[SPI_IND_1], DMA_INT_FLAG_FTF);
        //while(SPI_I2S_GetFlagStatus(SPI_IDX2_PORT,SPI_I2S_FLAG_BSY) != RESET){}
        //alt_int_count++;
        if (s_scbt[SPI_IND_1].timerread == TRUE) {
            //DMA_ClearITPendingBit(SPI_IDX2_RX_DMA_STREAM, DMA_IT_TCIF3);
            return;                        /* 定时器正在考备数据 */
        }

        readlen = (DMA_RCVBUF - s_scbt[SPI_IND_1].readlocat);
		if(!WriteBlockRoundBuf(&s_scbt[SPI_IND_1].recvrb, (INT8U *)&(s_scbt[SPI_IND_1].rcvdmabuf[s_scbt[SPI_IND_1].readlocat]), readlen)){

		}
        for (j = 0; j < readlen; j++) {
            if (s_spi_callback[SPI_IND_1].callback_spiintrxmsg != NULL) {
                s_spi_callback[SPI_IND_1].callback_spiintrxmsg(s_scbt[SPI_IND_1].rcvdmabuf[s_scbt[SPI_IND_1].readlocat + j]);
            }
        }
        s_scbt[SPI_IND_1].readlocat = 0;
        s_scbt[SPI_IND_1].dmairq = TRUE;
        //DMA_ClearITPendingBit(SPI_IDX2_RX_DMA_STREAM, DMA_IT_TCIF3);
    }
}


/*******************************************************************
** 函数名:     USER_DMA1_Stream1_IRQHandler
** 函数描述:   SPI0的DMA中断处理 - 仅处理发送中断
** 参数:       无
** 返回:       无
********************************************************************/
static __attribute__ ((section ("IRQ_HANDLE"))) void USER_DMA1_Stream1_IRQHandler(void) __irq
{
    INT32U tlen;

    if (dma_interrupt_flag_get(s_spi_dma_periph[SPI_IND_1],s_spi_dma_tx_stream[SPI_IND_1], DMA_INT_FLAG_FTF)) {                          /* 发送结束 */
        dma_interrupt_flag_clear(s_spi_dma_periph[SPI_IND_1],s_spi_dma_tx_stream[SPI_IND_1], DMA_INT_FLAG_FTF);
        //DMA_Cmd(s_spi_dma_tx_stream[SPI_IND_1], DISABLE);                                         /* 禁用DMA发送 */
        //DMA_ITConfig(s_spi_dma_tx_stream[SPI_IND_1], DMA_IT_TC, DISABLE);
		dma_interrupt_disable(s_spi_dma_periph[SPI_IND_1],s_spi_dma_rx_stream[SPI_IND_1],DMA_INT_FTF);
		dma_channel_disable(s_spi_dma_periph[SPI_IND_1],s_spi_dma_rx_stream[SPI_IND_1]);
        tlen = UsedOfRoundBuf(&s_scbt[SPI_IND_1].sendrb);                       /* 判断roundbuf中是否有数据要发送 */
        if (tlen <= 0) {
            s_scbt[SPI_IND_1].status = SPI_STATUS_IDLENOW;
        } else {                                                                /* 还有数据需要发送 */
            start_send(SPI_IND_1);
        }
    }
}

/**************************************************************************************************
**  函数名称:  USER_SPI2_IRQHandler
**  功能描述:  SPI2的中断处理函数 - 处理接和发送中断
**  输入参数:
**  输出参数:
**************************************************************************************************/
static __attribute__ ((section ("IRQ_HANDLE"))) void USER_SPI2_IRQHandler(void) __irq
{
    INT32U tlen;
    INT16U rword;
	
    #if SPI_DATASIZE == DATASIZE_16b
    INT16S j;
    INT8U  rcbuff[2] = {0xFF, 0xFF};
    #endif


    //ENTER_CRITICAL();
    
    /* 处理接收中断 */
    if (spi_i2s_interrupt_flag_get(SPI_IDX2_PORT, SPI_I2S_INT_FLAG_RBNE) != RESET) {
		
        #if SPI_DATASIZE == DATASIZE_16b    /* 16位数据宽度 */
        rword = spi_i2s_data_receive(SPI_IDX2_PORT);
        if (s_scbt[SPI_IND_2].spifirstbit == SPI_ENDIAN_MSB) { // Msb first, rxBuff始终应为大端模式，即高位字节在前地位字节在后
            for (j= 1; j >= 0; j--) {
            	rcbuff[j] = (INT8U)(rword >> ((INT8U)j*(INT8U)8));
                WriteRoundBuf(&s_scbt[SPI_IND_2].recvrb, rcbuff[j]);
                if (s_spi_callback[SPI_IND_2].callback_spiintrxmsg != NULL) {
                    s_spi_callback[SPI_IND_2].callback_spiintrxmsg(rcbuff[j]);
                }
            }
        } else {
            for (j = 0; j < 2; j++) {
            	rcbuff[j] = (INT8U)(rword >> ((INT8U)j*(INT8U)8));
                WriteRoundBuf(&s_scbt[SPI_IND_2].recvrb, rcbuff[j]);
                if (s_spi_callback[SPI_IND_2].callback_spiintrxmsg != NULL) {
                    s_spi_callback[SPI_IND_2].callback_spiintrxmsg(rcbuff[j]);
                }
            }
        }
        #else    /* 8位数据宽度 */
        rword = spi_i2s_data_receive(SPI_IDX2_PORT);
        WriteRoundBuf(&s_scbt[SPI_IND_2].recvrb, (INT8U)rword);
        if (s_spi_callback[SPI_IND_2].callback_spiintrxmsg != NULL) {
            s_spi_callback[SPI_IND_2].callback_spiintrxmsg((INT8U)rword);
        }
        #endif
        //SPI_ClearITPendingBit(SPI_IDX2_PORT, SPI_IT_RXNE);                         /* 清除标志位 */ 
    }
	
	/* 处理发送中断 */
    if (spi_i2s_interrupt_flag_get(SPI_IDX2_PORT, SPI_I2S_INT_FLAG_TBE) != RESET) {                     /* 发送完成中断 */
        spi_i2s_interrupt_disable(SPI_IDX2_PORT, SPI_I2S_INT_TBE);
		#if 1
        tlen = UsedOfRoundBuf(&s_scbt[SPI_IND_2].sendrb);                      /* 判断roundbuf中是否有数据要发送 */
        if (tlen <= 0) {
            //SPI_SendData(SPI_IDX2_PORT, 0xFFFF); /* 无效数据设置为0xFFFF。不设置的情况下MISO 数据线的数据为最后发送的2字节数据，带有7E，对主机数据解析会有影响 */
            s_scbt[SPI_IND_2].status = SPI_STATUS_IDLENOW;                         /* 置于发送结束状态 */
        } else {
            start_send(SPI_IND_2);
        }
        #endif
        //SPI_ClearITPendingBit(SPI_IDX2_PORT, SPI_IT_TXE);                         /* 清除标志位 */
    }

    if (spi_i2s_interrupt_flag_get(s_spi_port[SPI_IND_2], SPI_I2S_INT_FLAG_FERR) != RESET) {                     /* */
       // SPI_ClearITPendingBit(s_spi_port[SPI_IND_2], SPI_IT_ERR);                         /* 清除标志位 */
    }

    if (spi_i2s_interrupt_flag_get(s_spi_port[SPI_IND_2], SPI_I2S_INT_FLAG_RXORERR) != RESET) {                     /* */
       // SPI_ClearITPendingBit(s_spi_port[SPI_IND_2], SPI_IT_OVR);                         /* 清除标志位 */
    }
    
    //EXIT_CRITICAL();
}

/*******************************************************************
** 函数名:     USER_DMA0_Stream1_IRQHandler
** 函数描述:   SPI2的DMA中断处理 - 仅处理接收中断
** 参数:       无
** 返回:       无
********************************************************************/
static __attribute__ ((section ("IRQ_HANDLE"))) void USER_DMA0_Stream1_IRQHandler(void) __irq
{
    INT16S readlen = 0;
    INT16S j;
    INT32U ndtr;

	#if  1

    if (dma_interrupt_flag_get(s_spi_dma_periph[SPI_IND_2],s_spi_dma_rx_stream[SPI_IND_2], DMA_INT_FLAG_HTF)) {                          /* 半发送中断 */
        #if DEBGU_SPI > 0
        //Debug_SysPrint("HNDTR:0x%x\r\n", SPI_IDX2_RX_DMA_STREAM->NDTR);
        #endif
        #if 1
        dma_interrupt_flag_clear(s_spi_dma_periph[SPI_IND_2],s_spi_dma_rx_stream[SPI_IND_2], DMA_INT_FLAG_HTF);

        //while(SPI_I2S_GetFlagStatus(SPI_IDX2_PORT,SPI_I2S_FLAG_BSY) != RESET){}
        if (s_scbt[SPI_IND_2].timerread == TRUE) {
            //DMA_ClearITPendingBit(SPI_IDX2_RX_DMA_STREAM, DMA_IT_HTIF3);
            return;                        /* 定时器正在考备数据 */
        }

        readlen = (DMA_RCVBUF - s_scbt[SPI_IND_2].readlocat - dma_transfer_number_get(s_spi_dma_periph[SPI_IND_2],s_spi_dma_rx_stream[SPI_IND_2]));
        if (readlen < 0) {                                                      /* 缓存尾部有数据未读走 */
            readlen = DMA_RCVBUF - s_scbt[SPI_IND_2].readlocat;
			if(!WriteBlockRoundBuf(&s_scbt[SPI_IND_2].recvrb, (INT8U *)&(s_scbt[SPI_IND_2].rcvdmabuf[s_scbt[SPI_IND_2].readlocat]), readlen)){
			}
            for (j = 0; j < readlen; j++) {
                if (s_spi_callback[SPI_IND_2].callback_spiintrxmsg != NULL) {
                    s_spi_callback[SPI_IND_2].callback_spiintrxmsg(s_scbt[SPI_IND_2].rcvdmabuf[s_scbt[SPI_IND_2].readlocat + j]);
                }
            }
            s_scbt[SPI_IND_2].readlocat = 0;
            readlen = (DMA_RCVBUF - dma_transfer_number_get(s_spi_dma_periph[SPI_IND_2],s_spi_dma_rx_stream[SPI_IND_2]) - s_scbt[SPI_IND_2].readlocat);
        }
		if(!WriteBlockRoundBuf(&s_scbt[SPI_IND_2].recvrb, (INT8U *)&(s_scbt[SPI_IND_2].rcvdmabuf[s_scbt[SPI_IND_2].readlocat]), readlen)){

		}
        for (j = 0; j < readlen; j++) {
            if (s_spi_callback[SPI_IND_2].callback_spiintrxmsg != NULL) {
                s_spi_callback[SPI_IND_2].callback_spiintrxmsg(s_scbt[SPI_IND_2].rcvdmabuf[s_scbt[SPI_IND_2].readlocat + j]);
            }
        }
        s_scbt[SPI_IND_2].readlocat += readlen;
        s_scbt[SPI_IND_2].dmairq = TRUE;
        //DMA_ClearITPendingBit(s_spi_dma_rx_stream[SPI_IND_2], DMA_IT_HTIF3);
        #else
        ndtr = SPI_IDX2_RX_DMA_STREAM->NDTR;
        DMA_ClearITPendingBit(SPI_IDX2_RX_DMA_STREAM, DMA_IT_HTIF3);
        if (ndtr == 0x40) {
            s_ndtr0x40++;
        } else if (ndtr < 0x40){
            s_ndtr0x41++;
        }
        //while(SPI_I2S_GetFlagStatus(SPI_IDX2_PORT,SPI_I2S_FLAG_BSY) != RESET){}
        half_int_count++;
        if (s_scbt[idx].timerread == TRUE) {
            //DMA_ClearITPendingBit(SPI_IDX2_RX_DMA_STREAM, DMA_IT_HTIF3);
            return;                        /* 定时器正在考备数据 */
        }

        readlen = DMA_RCVBUF/2;//(DMA_RCVBUF - s_scbt[idx].readlocat - ndtr);
        if (readlen < 0) {                                                      /* 缓存尾部有数据未读走 */
            readlen = DMA_RCVBUF - s_scbt[idx].readlocat;
			if(WriteBlockRoundBuf(&s_scbt[idx].recvrb, (INT8U *)&(s_scbt[idx].rcvdmabuf[s_scbt[idx].readlocat]), readlen)){

			}
            for (j = 0; j < readlen; j++) {
                if (s_spi_callback.callback_spiintrxmsg != NULL) {
                    s_spi_callback.callback_spiintrxmsg(s_scbt[idx].rcvdmabuf[s_scbt[idx].readlocat + j]);
                }
            }
            s_scbt[idx].readlocat = 0;
            readlen = (DMA_RCVBUF - ndtr - s_scbt[idx].readlocat);
        }
		if(WriteBlockRoundBuf(&s_scbt[idx].recvrb, (INT8U *)&(s_scbt[idx].rcvdmabuf[s_scbt[idx].readlocat]), readlen)){
			
		}
        for (j = 0; j < readlen; j++) {
            if (s_spi_callback.callback_spiintrxmsg != NULL) {
                s_spi_callback.callback_spiintrxmsg(s_scbt[idx].rcvdmabuf[s_scbt[idx].readlocat + j]);
            }
        }
        s_scbt[idx].readlocat += readlen;
        s_scbt[idx].dmairq = TRUE;
        //DMA_ClearITPendingBit(SPI_IDX2_RX_DMA_STREAM, DMA_IT_HTIF3);
        #endif
    }
	#endif
	
    if (dma_interrupt_flag_get(s_spi_dma_periph[SPI_IND_2],s_spi_dma_rx_stream[SPI_IND_2], DMA_INT_FLAG_FTF)) {                          /* 完全发送中断*/
        #if 1
        dma_interrupt_flag_clear(s_spi_dma_periph[SPI_IND_2],s_spi_dma_rx_stream[SPI_IND_2], DMA_INT_FLAG_FTF);
        //while(SPI_I2S_GetFlagStatus(SPI_IDX2_PORT,SPI_I2S_FLAG_BSY) != RESET){}
        //alt_int_count++;
        if (s_scbt[SPI_IND_2].timerread == TRUE) {
            //DMA_ClearITPendingBit(SPI_IDX2_RX_DMA_STREAM, DMA_IT_TCIF3);
            return;                        /* 定时器正在考备数据 */
        }

        readlen = (DMA_RCVBUF - s_scbt[SPI_IND_2].readlocat);
		if(!WriteBlockRoundBuf(&s_scbt[SPI_IND_2].recvrb, (INT8U *)&(s_scbt[SPI_IND_2].rcvdmabuf[s_scbt[SPI_IND_2].readlocat]), readlen)){

		}
        for (j = 0; j < readlen; j++) {
            if (s_spi_callback[SPI_IND_2].callback_spiintrxmsg != NULL) {
                s_spi_callback[SPI_IND_2].callback_spiintrxmsg(s_scbt[SPI_IND_2].rcvdmabuf[s_scbt[SPI_IND_2].readlocat + j]);
            }
        }
        s_scbt[SPI_IND_2].readlocat = 0;
        s_scbt[SPI_IND_2].dmairq = TRUE;
        //DMA_ClearITPendingBit(SPI_IDX2_RX_DMA_STREAM, DMA_IT_TCIF3);
        #else
        ndtr = SPI_IDX2_RX_DMA_STREAM->NDTR;
        DMA_ClearITPendingBit(SPI_IDX2_RX_DMA_STREAM, DMA_IT_TCIF3);
        if (ndtr == 0x00) {
            s_ndtr0x80++;
        } else {
            s_ndtr0xet++;
        }
        alt_int_count++;
        /*
        DMA_ClearITPendingBit(SPI_IDX2_RX_DMA_STREAM, DMA_IT_TCIF3);
        DMA_ITConfig(SPI_IDX2_RX_DMA_STREAM, DMA_IT_TC, DISABLE);
        WriteBlockRoundBuf(&s_scbt[idx].recvrb, s_scbt[idx].rcvdmabuf, DMA_RCVBUF);
        DMA_ITConfig(SPI_IDX2_RX_DMA_STREAM, DMA_IT_TC, ENABLE);
        DMA_Cmd(SPI_IDX2_RX_DMA_STREAM, DISABLE);
        DMA_Cmd(SPI_IDX2_RX_DMA_STREAM, ENABLE);
        return;*/
        
        //while(SPI_I2S_GetFlagStatus(SPI_IDX2_PORT,SPI_I2S_FLAG_BSY) != RESET){}
        if (s_scbt[idx].timerread == TRUE) {
            //DMA_ClearITPendingBit(SPI_IDX2_RX_DMA_STREAM, DMA_IT_TCIF3);
            return;                        /* 定时器正在考备数据 */
        }

        readlen = DMA_RCVBUF/2;//(DMA_RCVBUF - s_scbt[idx].readlocat - ndtr);
        if (readlen < 0) {                                                      /* 缓存尾部有数据未读走 */
            readlen = DMA_RCVBUF - s_scbt[idx].readlocat;
			if(WriteBlockRoundBuf(&s_scbt[idx].recvrb, (INT8U *)&(s_scbt[idx].rcvdmabuf[s_scbt[idx].readlocat]), readlen)){

			}
            for (j = 0; j < readlen; j++) {
                if (s_spi_callback.callback_spiintrxmsg != NULL) {
                    s_spi_callback.callback_spiintrxmsg(s_scbt[idx].rcvdmabuf[s_scbt[idx].readlocat + j]);
                }
            }
            s_scbt[idx].readlocat = 0;
            readlen = (DMA_RCVBUF - ndtr - s_scbt[idx].readlocat);
        }
		if(WriteBlockRoundBuf(&s_scbt[idx].recvrb, (INT8U *)&(s_scbt[idx].rcvdmabuf[s_scbt[idx].readlocat]), readlen)){
			
		}
		
        for (j = 0; j < readlen; j++) {
            if (s_spi_callback.callback_spiintrxmsg != NULL) {
                s_spi_callback.callback_spiintrxmsg(s_scbt[idx].rcvdmabuf[s_scbt[idx].readlocat + j]);
            }
        }
        s_scbt[idx].readlocat =0;//+= readlen;
        s_scbt[idx].dmairq = TRUE;
        #endif
    }
}


/*******************************************************************
** 函数名:     USER_DMA0_Stream2_IRQHandler
** 函数描述:   SPI2的DMA中断处理 - 仅处理发送中断
** 参数:       无
** 返回:       无
********************************************************************/
static __attribute__ ((section ("IRQ_HANDLE"))) void USER_DMA0_Stream2_IRQHandler(void) __irq
{
    INT32U tlen;

    if (dma_interrupt_flag_get(s_spi_dma_periph[SPI_IND_2],s_spi_dma_tx_stream[SPI_IND_2], DMA_INT_FLAG_FTF)) {                          /* 发送结束 */
        dma_interrupt_flag_clear(s_spi_dma_periph[SPI_IND_2],s_spi_dma_tx_stream[SPI_IND_2], DMA_INT_FLAG_FTF);
		dma_interrupt_disable(s_spi_dma_periph[SPI_IND_2],s_spi_dma_rx_stream[SPI_IND_2],DMA_INT_FTF);
		dma_channel_disable(s_spi_dma_periph[SPI_IND_2],s_spi_dma_rx_stream[SPI_IND_2]);
        tlen = UsedOfRoundBuf(&s_scbt[SPI_IND_2].sendrb);                       /* 判断roundbuf中是否有数据要发送 */
        if (tlen <= 0) {
            s_scbt[SPI_IND_2].status = SPI_STATUS_IDLENOW;
        } else {                                                                /* 还有数据需要发送 */
            start_send(SPI_IND_2);
        }
    }
}


/*******************************************************************
** 函数名:     send_dma_config
** 函数描述:   SPIX_DMA发送参数配置 - 配置完默认是禁用的
** 参数:       [in] index: SPI号
** 返回:       无
********************************************************************/
static void send_dma_config(SPI_IND_E idx)
{
   dma_parameter_struct ICB;

	ICB.memory_addr			= (INT32U)s_scbt[idx].rcvdmabuf;
	ICB.memory_inc			= DMA_MEMORY_INCREASE_ENABLE;
	ICB.memory_width		= DMA_MEMORY_WIDTH_8BIT;
	ICB.number				= sizeof(s_scbt);
	ICB.periph_addr			= SPI_DATA(s_spi_port[idx]);
	ICB.periph_inc			= DMA_PERIPH_INCREASE_DISABLE;
	ICB.periph_width		= DMA_PERIPHERAL_WIDTH_8BIT;
	ICB.priority			= DMA_PRIORITY_ULTRA_HIGH;
	ICB.direction			= DMA_PERIPHERAL_TO_MEMORY;
	
    dma_deinit(s_spi_dma_periph[idx],s_spi_dma_rx_stream[idx]);
    dma_init(s_spi_dma_periph[idx],s_spi_dma_rx_stream[idx], &ICB);
    //s_spi_dma_rx_stream[idx]->NDTR = (INT32U)DMA_RCVBUF;
    dma_circulation_disable(s_spi_dma_periph[idx],s_spi_dma_rx_stream[idx]);
	dma_memory_to_memory_disable(s_spi_dma_periph[idx],s_spi_dma_rx_stream[idx]);
	
	dma_interrupt_enable(s_spi_dma_periph[idx],s_spi_dma_rx_stream[idx],DMA_INT_HTF);
	dma_interrupt_enable(s_spi_dma_periph[idx],s_spi_dma_rx_stream[idx],DMA_INT_FTF);
	dma_channel_enable(s_spi_dma_periph[idx],s_spi_dma_rx_stream[idx]);                                     /* 首先禁用，要发送数据时再启用 */
}

/*******************************************************************
** 函数名:     rcv_dma_config
** 函数描述:   SPIX_DMA接收参数配置 - 配置完默认是禁用的
** 参数:       [in] index: SPI号
** 返回:       无
********************************************************************/
static void rcv_dma_config(SPI_IND_E idx)
{
     dma_parameter_struct ICB;

	ICB.periph_addr			   = (INT32U)&SPI_DATA(s_spi_port[idx]);
    ICB.memory_addr			   = (INT32U)s_scbt[idx].senddmabuf;              /* 指向DMA缓冲区地址 */
    ICB.direction			   = DMA_MEMORY_TO_PERIPHERAL;
    ICB.number				   = sizeof(s_scbt);
    ICB.periph_inc			   = DMA_PERIPH_INCREASE_DISABLE;
    ICB.memory_inc			   = DMA_MEMORY_INCREASE_ENABLE;
    ICB.periph_width		   = DMA_PERIPHERAL_WIDTH_16BIT;
    ICB.memory_width		   = DMA_MEMORY_WIDTH_16BIT;
	ICB.priority			   = DMA_PRIORITY_ULTRA_HIGH;

    dma_deinit(s_spi_dma_periph[idx],s_spi_dma_tx_stream[idx]);
    dma_init(s_spi_dma_periph[idx],s_spi_dma_tx_stream[idx], &ICB);
	dma_circulation_disable(s_spi_dma_periph[idx],s_spi_dma_tx_stream[idx]);
	dma_memory_to_memory_disable(s_spi_dma_periph[idx],s_spi_dma_tx_stream[idx]);
	//DMA_ITConfig(s_spi_dma_tx_stream[idx], DMA_IT_TC, DISABLE);
    //DMA_Cmd(s_spi_dma_tx_stream[idx], DISABLE);                                     /* 首先禁用，要发送数据时再启用 */
	dma_interrupt_disable(s_spi_dma_periph[idx],s_spi_dma_rx_stream[idx],DMA_INT_FTF);
	dma_channel_disable(s_spi_dma_periph[idx],s_spi_dma_rx_stream[idx]); 
}

/*******************************************************************
** 函数名:     work_mode_config
** 函数描述:   SPIX的工作模式配置 - 配置完默认是禁用的
** 参数:       [in] idx        : spi编号
               [in] mode       : SPI_Mode_Master or SPI_Mode_Slave
               [in] baud_pres  : SCK时钟的波特率预分频器值
               [in] clock_mode : 时钟模式(0~3:mode0 ~ mode3)
** 返回:       无
**  注意事项:  RCC_APB2PeriphClockCmd语句一定要置于其他初始化指令之前
********************************************************************/
static void work_mode_config(SPI_IND_E idx, INT16U mode, INT16U baud_pres , INT8U clock_mode)
{
    spi_parameter_struct ICB;

    #if DEBGU_SPI > 0
    Debug_SysPrint("spi work_mode_config 主从模式:0x%x 时钟选择:%d\r\n", mode, baud_pres);
    #endif

    memset(&ICB, 0, sizeof(ICB));

    //ICB.= SPI_Direction_2Lines_FullDuplex;
#if SPI_DATASIZE == DATASIZE_16b
    ICB.frame_size   = SPI_FRAMESIZE_16BIT;
#else
    ICB.frame_size	 = SPI_FRAMESIZE_8BIT;
#endif
    if (clock_mode == 0) {
        ICB.clock_polarity_phase 	= SPI_CK_PL_LOW_PH_1EDGE;
    } else if (clock_mode == 1) {
        ICB.clock_polarity_phase 	= SPI_CK_PL_LOW_PH_2EDGE;
    } else if (clock_mode == 2) {
        ICB.clock_polarity_phase 	= SPI_CK_PL_HIGH_PH_1EDGE;
    } else {
        ICB.clock_polarity_phase 	= SPI_CK_PL_HIGH_PH_2EDGE;
    }
	ICB.trans_mode           	= SPI_TRANSMODE_FULLDUPLEX;
    ICB.device_mode          	= mode;
    ICB.frame_size           	= SPI_FRAMESIZE_8BIT;
    ICB.nss                  	= SPI_NSS_SOFT;
    ICB.prescale             	= baud_pres;
    ICB.endian              	= SPI_ENDIAN_MSB;
    s_scbt[idx].spifirstbit = ICB.endian;                              /* 保存MSB/LSB状态 */

    #if DEBGU_SPI > 0
    Debug_SysPrint("spi work_mode_config 数据宽度:0x%x\r\n", ICB.SPI_DataSize);
    #endif

    if (s_scbt[idx].dma_en == SPI_LOOP_MODE_ENABLE) {
        #if DEBGU_SPI > 0
        Debug_SysPrint("spi work_mode_config 配置为LOOP模式\r\n");
        #endif
        spi_i2s_deinit(s_spi_port[idx]);
        spi_init(s_spi_port[idx], &ICB);
		spi_i2s_interrupt_disable(s_spi_port[idx],SPI_I2S_INT_RBNE);
        spi_i2s_interrupt_disable(s_spi_port[idx],SPI_I2S_INT_TBE);

		spi_i2s_interrupt_enable(s_spi_port[idx],SPI_I2S_INT_ERR);
        
        spi_enable(s_spi_port[idx]);

	} else if (s_scbt[idx].dma_en == SPI_DMA_MODE_DISABLE) {
        #if DEBGU_SPI > 0
        Debug_SysPrint("spi work_mode_config 配置为中断模式\r\n");
        #endif
        spi_i2s_deinit(s_spi_port[idx]);
        spi_init(s_spi_port[idx], &ICB);
		spi_i2s_interrupt_enable(s_spi_port[idx],SPI_I2S_INT_RBNE);
        spi_i2s_interrupt_disable(s_spi_port[idx],SPI_I2S_INT_TBE);

        spi_i2s_interrupt_enable(s_spi_port[idx],SPI_I2S_INT_ERR);
        
        spi_disable(s_spi_port[idx]);
    } else {
        spi_i2s_deinit(s_spi_port[idx]);
        spi_init(s_spi_port[idx], &ICB);
        //SPI_NSSInternalSoftwareConfig(SPI_IDX2_PORT, SPI_NSSInternalSoft_Reset);
        spi_i2s_interrupt_disable(s_spi_port[idx],SPI_I2S_INT_RBNE);
        spi_i2s_interrupt_disable(s_spi_port[idx],SPI_I2S_INT_TBE);

        spi_i2s_interrupt_enable(s_spi_port[idx],SPI_I2S_INT_ERR);
        spi_enable(s_spi_port[idx]);
    }

    s_scbt[idx].status = SPI_STATUS_DISABLE;                                     /* 初始化完成后默认置于禁用状态 */
}

/*******************************************************************
** 函数名:     spix_delaytmrproc
** 函数描述:   定时扫描DMA接收缓存残留数据
** 参数:       无
** 返回:       无
********************************************************************/
static void spix_delaytmrproc(void)
{
    INT16S readlen = 0;
    INT8U NDTR  = 0;
    INT8U idx;
	
	for (idx = 0; idx < MAX_SPI_IND; idx++) {
		if (s_scbt[idx].dma_en == SPI_DMA_MODE_DISABLE) {
			return;
		}
		
		if ((s_scbt[idx].status == SPI_STATUS_DISABLE) || (s_scbt[idx].status == SPI_STATUS_INITING)) {
			return;
		}
		
		NDTR = dma_transfer_number_get(s_spi_dma_periph[idx],s_spi_dma_rx_stream[idx]);
		
		ENTER_CRITICAL();
		if (s_scbt[idx].dmairq == TRUE) {
			s_scbt[idx].dmairq	 = FALSE;
			s_scbt[idx].oldlocat = NDTR;
			EXIT_CRITICAL() ;
			return; 														  /* 在与前次扫描间有进过中断，则退出 */
		}
		if (s_scbt[idx].oldlocat == NDTR) { 									  /* 一个定时时间内没收到新数据 */
			s_scbt[idx].timerread = TRUE;
		} else {
			s_scbt[idx].timerread = FALSE;
		}
		EXIT_CRITICAL();
		
		if (s_scbt[idx].timerread == TRUE) {									  /* 一个定时时间内没收到新数据 */
			readlen = (DMA_RCVBUF - NDTR - s_scbt[idx].readlocat);
			if (readlen == 0) { 												/* 没收到新数据，数据已读过 */
				s_scbt[idx].timerread = FALSE;
				s_scbt[idx].oldlocat = NDTR;
				return;
			} else if (readlen < 0) {											/* 缓存尾部有数据未读走 */
				readlen = DMA_RCVBUF - s_scbt[idx].readlocat;
				if(!WriteBlockRoundBuf(&s_scbt[idx].recvrb, (INT8U *)&(s_scbt[idx].rcvdmabuf[s_scbt[idx].readlocat]), readlen)){
		
				}
				s_scbt[idx].readlocat = 0;
				readlen = (DMA_RCVBUF - NDTR - s_scbt[idx].readlocat);
			}
			if(!WriteBlockRoundBuf(&s_scbt[idx].recvrb, (INT8U *)&(s_scbt[idx].rcvdmabuf[s_scbt[idx].readlocat]), readlen)){
			}
			s_scbt[idx].readlocat +=  (INT8U )readlen;
		}
		s_scbt[idx].timerread = FALSE;
		s_scbt[idx].oldlocat  = NDTR;

	}


}

/*******************************************************************
** 函数名:     Spix_Initiate
** 函数描述:   初始化SPIX(此处也配置了DMA) - 初始化完后默认是禁用的
** 参数:       [in] spi_mode :主机/从机模式
               [in] dma_mode : dma/中断模式
               [in] baud_pres : 预分频
               [in] clock_mode : 时钟模式(0~3:mode0 ~ mode3)
** 返回:       无
********************************************************************/
static BOOLEAN Spix_Initiate(SPI_IND_E idx, INT16U spi_mode, INT8U dma_mode, INT16U baud_pres, INT8U clock_mode)
{
    s_scbt[idx].status = SPI_STATUS_INITING;

    /* 接收环形缓冲区 */
    if (s_scbt[idx].p_rx != NULL) {
        MemFree(s_scbt[idx].p_rx);
    }
    s_scbt[idx].p_rx = MemAlloc(SPI_ROUNDBUF_RX_LEN);
    if (s_scbt[idx].p_rx == NULL) {
        #if DEBGU_SPI > 0
        Debug_SysPrint("<**** Spix_Initiate(),申请RX动态内存失败****>\r\n");
        #endif		
        return FALSE;
    }
    s_scbt[idx].rx_len = SPI_ROUNDBUF_RX_LEN;
	/* 初始化接收缓存 */
	InitRoundBuf(&s_scbt[idx].recvrb, s_scbt[idx].p_rx, s_scbt[idx].rx_len, 0);

    /* 发送环形缓冲区 */
    if (s_scbt[idx].p_tx != NULL) {
        MemFree(s_scbt[idx].p_tx);
    }
    s_scbt[idx].p_tx = MemAlloc(SPI_ROUNDBUF_TX_LEN);
    if (s_scbt[idx].p_tx == NULL) {
        #if DEBGU_SPI > 0
        Debug_SysPrint("<**** Spix_Initiate(),申请TX动态内存失败****>\r\n");
        #endif			
        return FALSE;
    }
    s_scbt[idx].tx_len = SPI_ROUNDBUF_TX_LEN;
	/* 初始化发送缓存 */
    InitRoundBuf(&s_scbt[idx].sendrb, s_scbt[idx].p_tx, s_scbt[idx].tx_len, 0);

    if (dma_mode == SPI_LOOP_MODE_ENABLE) {
        #if DEBGU_SPI > 0
        Debug_SysPrint("spi Spix_Initiate LOOP模式\r\n");
         #endif
		
        s_scbt[idx].dma_en = SPI_LOOP_MODE_ENABLE;
        work_mode_config(idx, spi_mode, baud_pres, clock_mode); /* 配置SPI工作模式 */
		if (idx == SPI_IND_1) {
            NVIC_IrqHandleInstall(SPI0_IRQ, USER_SPI0_IRQHandler, UART_RX_PRIOTITY, TRUE);
		} else if (idx == SPI_IND_2) {
            NVIC_IrqHandleInstall(SPI2_IRQ, USER_SPI2_IRQHandler, UART_RX_PRIOTITY, TRUE);
		} 

	} else if (SPI_DMA_MODE_DISABLE == dma_mode) {
        #if DEBGU_SPI > 0
        Debug_SysPrint("spi Spix_Initiate 中断模式\r\n");
        #endif
		
        s_scbt[idx].dma_en = SPI_DMA_MODE_DISABLE;
        work_mode_config(idx, spi_mode, baud_pres, clock_mode); /* 配置SPI工作模式 */
		if (idx == SPI_IND_1) {
            NVIC_IrqHandleInstall(SPI0_IRQ, USER_SPI0_IRQHandler, UART_RX_PRIOTITY, TRUE);
		} else if (idx == SPI_IND_2) {
            NVIC_IrqHandleInstall(SPI2_IRQ, USER_SPI2_IRQHandler, UART_RX_PRIOTITY, TRUE);
		} 
        
    } else {
        rcu_periph_clock_enable(RCU_DMA1);
		rcu_periph_clock_enable(RCU_DMA0);
        #if DEBGU_SPI > 1
        Debug_SysPrint("spi Spix_Initiate DMA模式\r\n");
        #endif
        s_scbt[idx].dma_en    = SPI_DMA_MODE_ENABLE;
        s_scbt[idx].timerread = FALSE;
        s_scbt[idx].dmairq    = TRUE;
        s_scbt[idx].oldlocat  = DMA_RCVBUF;
        s_scbt[idx].readlocat = 0;
        s_scbt[idx].rcvdmabuf = s_spi_dmarecvbuf[idx];
        s_scbt[idx].senddmabuf= s_spi_dmasendbuf[idx];

        work_mode_config(idx, spi_mode, baud_pres, clock_mode); /* 配置SPI工作模式 */
        rcv_dma_config(idx);
        send_dma_config(idx);
		if (idx == SPI_IND_1) {
			NVIC_IrqHandleInstall(SPI0_IRQ, USER_SPI0_IRQHandler, UART_RX_PRIOTITY, TRUE);
			NVIC_IrqHandleInstall(DMA0_C1_IRQ, USER_DMA0_Stream1_IRQHandler, UART_RX_PRIOTITY, TRUE);	//		UART_TXDMA_PRIOTITY
			NVIC_IrqHandleInstall(DMA0_C2_IRQ, USER_DMA0_Stream2_IRQHandler, UART_TXDMA_PRIOTITY, TRUE);
		} else if (idx == SPI_IND_2) {
			NVIC_IrqHandleInstall(SPI2_IRQ, USER_SPI2_IRQHandler, UART_RX_PRIOTITY, TRUE);
			NVIC_IrqHandleInstall(DMA1_C0_IRQ, USER_DMA1_Stream0_IRQHandler, UART_RX_PRIOTITY, TRUE);	//		UART_TXDMA_PRIOTITY
			NVIC_IrqHandleInstall(DMA1_C1_IRQ, USER_DMA1_Stream1_IRQHandler, UART_TXDMA_PRIOTITY, TRUE);
		} 
		if (s_is_creat == FALSE) {
			s_spi_tmr_id = CreateTimer(spix_delaytmrproc);
			s_is_creat = TRUE;
		}
		StartTimer(s_spi_tmr_id, _TICK, 1, TRUE);

    }
    return TRUE;
}
 
/*******************************************************************
** 函数名:     Spix_IOConfig
** 函数描述:   SPIX的IO口配置
** 参数:       无
** 返回:       无
********************************************************************/
static void Spix_IOConfig(SPI_IND_E idx)
{
    #if DEBGU_SPI > 0
    Debug_SysPrint("<**** 初始化spi_idx %d的spi通讯脚 ****>\r\n",idx);
    #endif

    if (idx == SPI_IND_1) {
        rcu_periph_clock_enable(RCU_AF);
		rcu_periph_clock_enable(RCU_SPI0);
		rcu_periph_clock_enable(SPI1_PIN_IO);
		/* 配置串口管脚并锁定配置 */
		gpio_init(SPI1_PIN_IO,GPIO_MODE_AF_PP,GPIO_OSPEED_50MHZ,SPI1_PIN_SCK);
		gpio_init(SPI1_PIN_IO,GPIO_MODE_AF_PP,GPIO_OSPEED_50MHZ,SPI1_PIN_MOSI);
		gpio_init(SPI1_PIN_IO,GPIO_MODE_IN_FLOATING,GPIO_OSPEED_50MHZ,SPI1_PIN_MISO);
		gpio_init(SPI1_PIN_IO,GPIO_MODE_OUT_PP,GPIO_OSPEED_50MHZ,SPI1_PIN_CS);
    } else if (idx == SPI_IND_2) {
        rcu_periph_clock_enable(RCU_AF);
        rcu_periph_clock_enable(RCU_SPI2);
		rcu_periph_clock_enable(SPI4_PIN_IO);
		
        /* 配置串口管脚并锁定配置 */
		gpio_init(SPI4_PIN_IO,GPIO_MODE_AF_PP,GPIO_OSPEED_50MHZ,SPI4_PIN_SCK);
		gpio_init(SPI4_PIN_IO,GPIO_MODE_AF_PP,GPIO_OSPEED_50MHZ,SPI4_PIN_MOSI);
		gpio_init(SPI4_PIN_IO,GPIO_MODE_IN_FLOATING,GPIO_OSPEED_50MHZ,SPI4_PIN_MISO);
		gpio_init(SPI4_PIN_IO,GPIO_MODE_OUT_PP,GPIO_OSPEED_50MHZ,SPI4_PIN_CS);
    } 
}

/*******************************************************************
** 函数名:     Spix_Enable
** 函数描述:   启用SPIX
** 参数:       无
** 返回:       无
********************************************************************/
static void Spix_Enable(SPI_IND_E idx)
{
	spi_enable(s_spi_port[idx]);
    s_scbt[idx].status = SPI_STATUS_IDLENOW;
}

/*******************************************************************
** 函数名:     Spix_Disable
** 函数描述:   停用SPIX
** 参数:       无
** 返回:       无
********************************************************************/
static void Spix_Disable(SPI_IND_E idx)
{
    s_scbt[idx].status = SPI_STATUS_DISABLE;

    spi_disable(s_spi_port[idx]);
}

/*******************************************************************
** 函数名:     Spix_GetDmaMode
** 函数描述:   获取收据收发模式
** 参数:       无
** 返回:       0x00: 中断模式
               0x01: DMA模式
********************************************************************/
INT8U Spix_GetDmaMode(SPI_IND_E idx)
{
    return s_scbt[idx].dma_en;
}

/*******************************************************************
** 函数名:     Spix_LeftofBuf
** 函数描述:   获取指定串口的发送缓冲区剩余空间
** 参数:       无
** 返回:       剩余空间长度
********************************************************************/
INT32S Spix_LeftofBuf(SPI_IND_E idx)
{
    if ((s_scbt[idx].status != SPI_STATUS_IDLENOW) && (s_scbt[idx].status != SPI_STATUS_SENDING)) {
        return -1;
    } else {
        return LeftOfRoundBuf(&s_scbt[idx].sendrb);
    }
}

/*******************************************************************
** 函数名:     Spix_ReadByte
** 函数描述:   SPIX读取单个字节
** 参数:       [in] idx : spi编号
** 返回:       剩余空间长度
********************************************************************/
INT32S Spix_ReadByte(SPI_IND_E idx)
{
    if (s_scbt[idx].dma_en == SPI_LOOP_MODE_ENABLE) {
        return ReadSPI(idx);
    } else {
        return ReadRoundBuf(&s_scbt[idx].recvrb);
    }
}

/*******************************************************************
** 函数名:     Spix_RecvBufUsed
** 函数描述:   SPIX接收的大小
** 参数:       [in] idx : spi编号
** 返回:       接收大小
********************************************************************/
INT32U Spix_RecvBufUsed(SPI_IND_E idx)
{
    return  UsedOfRoundBuf(&s_scbt[idx].recvrb);
}

/*******************************************************************
** 函数名:     Spix_RecvBufReset
** 函数描述:   复位接收缓存
** 参数:       [in] idx : spi编号
** 返回:       无
********************************************************************/
void Spix_RecvBufReset(SPI_IND_E idx)
{
    ResetRoundBuf(&s_scbt[idx].recvrb);
}

/*******************************************************************
** 函数名:     Spix_SendByte
** 函数描述:   SPIX发送单个字节
** 参数:       [in] byte: 字节数据
** 返回:       无
********************************************************************/
BOOLEAN Spix_SendByte(SPI_IND_E idx, INT8U byte)
{
    if (s_scbt[idx].dma_en == SPI_LOOP_MODE_ENABLE) {
        if (WriteSPI(idx, byte) == (-1)) {
            return FALSE;
		}
	} else {
		if (WriteRoundBuf(&s_scbt[idx].sendrb, byte)) {
			if (s_scbt[idx].status == SPI_STATUS_IDLENOW) { 						  /* 处于空闲状态，调用开始发送 */
				return start_send(idx);
			} else if (s_scbt[idx].status == SPI_STATUS_SENDING) {					  /* 处于发送状态 */
				return TRUE;
			} else {
				return FALSE;
			}
		} else {
			return FALSE;
		}
	}
	return TRUE;
}

/*******************************************************************
** 函数名:     Spix_SendData
** 函数描述:   SPIX发送连续数据
** 参数:       [in] ptr:   数据指针
**             [in] tlen:  数据长度
** 返回:       发送结果
********************************************************************/
BOOLEAN Spix_SendData(SPI_IND_E idx, INT8U *ptr, INT32U tlen)
{
    INT32U i;
    if ((ptr == 0) || (tlen == 0)) {
        return FALSE;
    }
    if (s_scbt[idx].dma_en == SPI_LOOP_MODE_ENABLE) {
		for (i = 0; i < tlen; i++) {
			if (WriteSPI(idx, ptr[i]) == (-1)) {
				return FALSE;
			}
		}
	} else {
        #if EN_DEBUG > 1
    		Debug_SysPrint("< Spix_SendData:%d wait to send:%d>\r\n",s_scbt[idx].status,UsedOfRoundBuf(&s_scbt[idx].sendrb));
        #endif
        if (WriteBlockRoundBuf(&s_scbt[idx].sendrb, ptr, tlen)) {
            if (s_scbt[idx].status == SPI_STATUS_IDLENOW) {                           /* 处于空闲状态，调用开始发送 */
                return start_send(idx);
            } else if (s_scbt[idx].status == SPI_STATUS_SENDING) {                    /* 处于发送状态 */
                return TRUE;
            } else {
                return FALSE;
            }
        } else {
            return FALSE;
        }
  	}
	return TRUE;
}

/*******************************************************************
** 函数名:     Spix_Open_Port
** 函数描述:   打开SPI端口
** 参数:       [in] spi_mode : 0为主机模式; 1为从机模式
               [in] dma_mode : dma/中断模式/loop
               [in] bps      : 波特率
               [in] clock_mode : 时钟模式(0~3:mode0 ~ mode3)
** 返回:       TRUE ：成功   FALSE ：失败
********************************************************************/
BOOLEAN Spix_Open_Port(SPI_IND_E idx, INT8U spi_mode, INT8U dma_mode, INT16U bps, INT8U clock_mode)
{
    INT16U SpiMode, SpiPrescal;

    #if DEBGU_SPI > 0
	   Debug_SysPrint("<**** Spix_Open_Port(),idx:%d,mode:%d, dma_mode:%d,bps:%d,clk_mode:%d ****>\r\n",idx, spi_mode, dma_mode, bps, clock_mode);
    #endif
    
    #if 1
    if (spi_mode == 0x00) {                             /* 主机模式 */
        SpiMode = SPI_MASTER;
    } else if (spi_mode == 0x01) {                      /* 从机模式 */
        SpiMode = SPI_SLAVE;
    } else {
        return FALSE;
    }

    if (bps > 4000) { /* 4M */
        SpiPrescal = SPI_PSC_8;
    } else if (bps > 2000) { /* 2M */
        SpiPrescal = SPI_PSC_16;
    } else if (bps > 1000) { /* 1M */
        SpiPrescal = SPI_PSC_32;
    } else if (bps > 500) { /* 500K */
        SpiPrescal = SPI_PSC_64;
    } else if (bps > 250) { /* 250K */
        SpiPrescal = SPI_PSC_128;
    } else if (bps > 125) { /* 125K */
        SpiPrescal = SPI_PSC_256;
    } else {
        SpiPrescal = SPI_PSC_32;
    }

    #else
    SpiMode = SPI_Mode_Slave;
    SpiPrescal = SPI_BaudRatePrescaler_32;
    #endif
	
    #if DEBGU_SPI > 0
    Debug_SysPrint("<**** Spix_Open_Port(),prescale:0x%x ****>\r\n",SpiPrescal);
    #endif

    if (s_scbt[idx].status == SPI_STATUS_CLOSED) {
        Spix_IOConfig(idx);
        if (Spix_Initiate(idx, SpiMode, dma_mode, SpiPrescal, clock_mode) == FALSE) {
            #if DEBGU_SPI > 0
            Debug_SysPrint("<**** Spix_Open_Port(),初始化失败 ****>\r\n");
            #endif
            return FALSE;
        }
        Spix_Enable(idx);
    }
    #if DEBGU_SPI > 0
    Debug_SysPrint("<**** Spix_Open_Port(),初始化成功 ****>\r\n");
    #endif
    return TRUE;
}

/*******************************************************************
** 函数名:     Spix_Close_Port
** 函数描述:   关闭SPI端口
** 参数:       无
** 返回:       TRUE ：成功   FALSE ：失败
********************************************************************/
BOOLEAN Spix_Close_Port(SPI_IND_E idx)
{
    if (s_scbt[idx].status != SPI_STATUS_CLOSED) {
		spi_i2s_interrupt_disable(s_spi_port[idx],SPI_I2S_INT_RBNE);
		spi_i2s_interrupt_disable(s_spi_port[idx],SPI_I2S_INT_TBE);
		spi_i2s_interrupt_disable(s_spi_port[idx],SPI_I2S_INT_ERR);
		Spix_Disable(idx);
		spi_i2s_deinit(s_spi_port[idx]);
        /* 释放内存 */
        if (s_scbt[idx].p_rx != NULL) {
            MemFree(s_scbt[idx].p_rx);
            s_scbt[idx].p_rx = NULL;
            s_scbt[idx].rx_len = 0;
        }

        if (s_scbt[idx].p_tx != NULL) {
            MemFree(s_scbt[idx].p_tx);
            s_scbt[idx].p_tx = NULL;
            s_scbt[idx].tx_len = 0;
        }

        /* 设置状态 */
        s_scbt[idx].status = SPI_STATUS_CLOSED;
    }

    return TRUE;
}

/*******************************************************************
** 函数名:     Spix_RegistHandler  上下层
** 函数描述:   注册SPI模块中断接收回调函数
** 参数:       [in] ReceivedDat：要接收到的数据
** 返回:       无
********************************************************************/
void Spix_RegistHandler(SPI_IND_E idx, INT8U (* handle) (INT8U ReceivedDat))
{
    s_spi_callback[idx].callback_spiintrxmsg = handle;
}

/*******************************************************************
** 函数名:     Spix_IsSendIle
** 函数描述:   发送是否在空闲状态
** 参数:       [in] idx : spi编号
** 返回:       TRUE : 空闲 FALSE: 非空闲状态
********************************************************************/
BOOLEAN Spix_IsSendIle(SPI_IND_E idx)
{
    if (s_scbt[idx].status == SPI_STATUS_IDLENOW) {
        return TRUE;
	}
    return FALSE;
}

