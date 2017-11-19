/******************************************************************************
**
** Filename:     st_uart_drv.c
** Copyright:    
** Description:  该模块主要实现串口的驱动管理
**
*******************************************************************************
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#include "yx_include.h"
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "yx_dym_drv.h"
#include "yx_loopbuf.h"
#include "st_uart_reg.h"
#include "st_uart_irq.h"
#include "st_uart_drv.h"
#include "st_irq_drv.h"

/*
********************************************************************************
* define config parameters
********************************************************************************
*/
#define _SENDING             0x01
#define _OPEN                0x80

#define DMA_TX_LEN           32
#define DMA_RX_LEN           32

/*
********************************************************************************
* define struct
********************************************************************************
*/

typedef struct {
    INT8U       status;
    INT8U       sel;
    INT8U       p_dma_tx[DMA_TX_LEN];
    INT8U       p_dma_rx0[DMA_RX_LEN];
    INT8U       p_dma_rx1[DMA_RX_LEN];
    INT8U      *p_rx;
    INT8U      *p_tx;

	INT32U      tx_len;
	INT32U      rx_len;
    
    LOOP_BUF_T  r_round;
    LOOP_BUF_T  s_round;
} UART_T;

/*
********************************************************************************
* define module variants
********************************************************************************
*/
static INT8U s_uart1map;
static UART_T s_uart[UART_COM_MAX];





/*******************************************************************
** 函数名称: STM32_UART_PinsConfig
** 函数描述: 管脚配置
** 参数:     [in] pinfo: 配置表
**           [in] onoff: 打开/关闭串口, TRUE-打开串口,FALSE-关闭串口
** 返回:     无
********************************************************************/
static void STM32_UART_PinsConfig(const UART_TBL_T *pinfo, INT8U onoff)
{
    GPIO_InitTypeDef gpio_initstruct;
    
    if (pinfo == 0) {
        return;
    }
    
    /* remap uart pins if need */
    //if (pinfo->uart_remap != 0) {
    //    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
        //GPIO_PinRemapConfig(pinfo->uart_remap, ENABLE);
   // }

    if(pinfo->is_remap != 0xff) {
        if(pinfo->is_remap) {
            GPIO_PinRemapConfig(pinfo->uart_remap, ENABLE);
        }else {
            GPIO_PinRemapConfig(pinfo->uart_remap, DISABLE);
        }
    }
    
    
    //RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);                        /* 开启GPIO系统时钟 */
    
    /* Configure USARTx_Tx as alternate function push-pull */
    gpio_initstruct.GPIO_Pin   = (INT16U)(1 << pinfo->uart_pin_tx);
    gpio_initstruct.GPIO_Speed = GPIO_Speed_50MHz;

    if (onoff) {
        gpio_initstruct.GPIO_Mode  = GPIO_Mode_AF_PP;
        GPIO_Init((GPIO_TypeDef *)pinfo->gpio_base, &gpio_initstruct);
        
        #if 0
        if (pinfo->gpio_base == GPIOB_BASE) {
            GPIO_PinAFConfig((GPIO_TypeDef *)pinfo->gpio_base, pinfo->uart_pin_tx, GPIO_AF_0);
        } else {
            GPIO_PinAFConfig((GPIO_TypeDef *)pinfo->gpio_base, pinfo->uart_pin_tx, GPIO_AF_1);
        }
        #endif
    } else {
        gpio_initstruct.GPIO_Mode  = GPIO_Mode_Out_PP;
        GPIO_Init((GPIO_TypeDef *)pinfo->gpio_base, &gpio_initstruct);
        GPIO_ResetBits((GPIO_TypeDef *)pinfo->gpio_base, (INT16U)(1 << pinfo->uart_pin_tx));
    }
    
    /* Configure USARTx_Rx as input floating */
    gpio_initstruct.GPIO_Pin   = (INT16U)(1 << pinfo->uart_pin_rx);
    gpio_initstruct.GPIO_Speed = GPIO_Speed_50MHz;
    //gpio_initstruct.GPIO_OType = GPIO_OType_PP;
    //gpio_initstruct.GPIO_PuPd  = GPIO_PuPd_UP;
    if (onoff) {
        gpio_initstruct.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
        GPIO_Init((GPIO_TypeDef *)pinfo->gpio_base, &gpio_initstruct);
        #if 0
        if (pinfo->gpio_base == GPIOB_BASE) {
            GPIO_PinAFConfig((GPIO_TypeDef *)pinfo->gpio_base, pinfo->uart_pin_rx, GPIO_AF_0);
        } else {
            GPIO_PinAFConfig((GPIO_TypeDef *)pinfo->gpio_base, pinfo->uart_pin_rx, GPIO_AF_1);
        }
        #endif
    } else {
        gpio_initstruct.GPIO_Mode  = GPIO_Mode_Out_PP;
        GPIO_Init((GPIO_TypeDef *)pinfo->gpio_base, &gpio_initstruct);
        GPIO_ResetBits((GPIO_TypeDef *)pinfo->gpio_base, (INT16U)(1 << pinfo->uart_pin_rx));
    }
}

/*******************************************************************
** 函数名称: STM32_UART_DMAConfig
** 函数描述: 配置UART发送为DMA方式
** 参数:     [in] pinfo  : 配置参数
** 返回:     无
********************************************************************/
static void STM32_UART_DMAConfig(const UART_TBL_T *pinfo)
{
    INT32S irq_id;
    BOOLEAN is_dma1 = FALSE;
    IRQ_SERVICE_FUNC irqhandler;
    DMA_InitTypeDef dma_initstruct;
    
    if (pinfo == 0) {
        return;
    }
    
    /* 配置发送通道 */
	if (pinfo->dma_base_tx != 0) {
        switch (pinfo->dma_base_tx)
	    {
        case DMA1_Channel2_BASE:                          /* for uart1 tx */
            irq_id   = DMA1_Channel2_IRQn;
            irqhandler = (IRQ_SERVICE_FUNC)DMA1_CHANNEL2_IrqHandle;
            is_dma1  = TRUE;
            break;
        case DMA1_Channel4_BASE:                          /* for uart2 tx */
            irq_id = DMA1_Channel4_IRQn;
            irqhandler = (IRQ_SERVICE_FUNC)DMA1_CHANNEL4_IrqHandle;
            break;
        case DMA1_Channel7_BASE:                          /* for uart3 tx */
            irq_id   = DMA1_Channel7_IRQn;
            irqhandler = (IRQ_SERVICE_FUNC)DMA1_CHANNEL7_IrqHandle;
            is_dma1  = TRUE;
	  	    break;
        default:
            return;
        }
    
        /* dma clock enable */
        if (is_dma1) {
            RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
        } else {
            RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, ENABLE);
        }
    
        DMA_DeInit((DMA_Channel_TypeDef *)pinfo->dma_base_tx);        /* 恢复DMA通道 */
    
        DMA_StructInit(&dma_initstruct);
        dma_initstruct.DMA_PeripheralBaseAddr  = (INT32U)&(((USART_TypeDef *)pinfo->uart_base)->DR);/* 外设基准地址 */
        dma_initstruct.DMA_MemoryBaseAddr      = 0;                            /* 内存基准地址 */
        dma_initstruct.DMA_DIR                 = DMA_DIR_PeripheralDST;        /* 通道方向 */
        dma_initstruct.DMA_BufferSize          = 0;                            /* 内存缓存大小 */
        dma_initstruct.DMA_PeripheralInc       = DMA_PeripheralInc_Disable;    /* 外设地址不自增 */
        dma_initstruct.DMA_MemoryInc           = DMA_MemoryInc_Enable;         /* 内存地址自增 */
        dma_initstruct.DMA_PeripheralDataSize  = DMA_PeripheralDataSize_Byte;  /* 字节模式 */
        dma_initstruct.DMA_MemoryDataSize      = DMA_MemoryDataSize_Byte;      /* 字节模式 */
        dma_initstruct.DMA_Mode                = DMA_Mode_Normal;              /* 正常模式 */
        dma_initstruct.DMA_Priority            = DMA_Priority_Low;                 
        dma_initstruct.DMA_M2M                 = DMA_M2M_Disable;
    
        DMA_Init((DMA_Channel_TypeDef *)pinfo->dma_base_tx, &dma_initstruct);
        ST_IRQ_ConfigIrqEnable(irq_id, false);                                  /* 关闭中断 */
        ST_IRQ_InstallIrqHandler(irq_id, irqhandler);                           /* install irq handle */
        ST_IRQ_ConfigIrqPriority(irq_id, IRQ_PRIOTITY_TXDMA);
        ST_IRQ_ConfigIrqEnable(irq_id, true);                                   /* 打开中断 */
        
    }
    
    /* 配置接收通道 */
    if (pinfo->dma_base_rx != 0) {
        switch (pinfo->dma_base_rx)
	    {
        case DMA1_Channel3_BASE:                          /* for uart1 rx */
            irq_id   = DMA1_Channel3_IRQn;
            irqhandler = (IRQ_SERVICE_FUNC)DMA1_CHANNEL3_IrqHandle;
            is_dma1  = TRUE;
            break;
        case DMA1_Channel5_BASE:                          /* for uart2 rx */
            irq_id   = DMA1_Channel5_IRQn;
            irqhandler = (IRQ_SERVICE_FUNC)DMA1_CHANNEL5_IrqHandle;
            is_dma1  = TRUE;
	  	    break;
        case DMA1_Channel6_BASE:                          /* for uart3 rx */
            irq_id   = DMA1_Channel6_IRQn;
            irqhandler = (IRQ_SERVICE_FUNC)DMA1_CHANNEL6_IrqHandle;
            is_dma1  = TRUE;
	  	    break;
        default:
            return;
        }
    
        /* dma clock enable */
        if (is_dma1) {
            RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
        } else {
            RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, ENABLE);
        }
    
        DMA_DeInit((DMA_Channel_TypeDef *)pinfo->dma_base_rx);             /* 恢复DMA通道 */
    
        DMA_StructInit(&dma_initstruct);
        dma_initstruct.DMA_PeripheralBaseAddr  = (INT32U)&(((USART_TypeDef *)pinfo->uart_base)->DR);/* 外设基准地址 */
        dma_initstruct.DMA_MemoryBaseAddr      = (INT32U)s_uart[pinfo->com].p_dma_rx0;     /* 内存基准地址 */
        dma_initstruct.DMA_DIR                 = DMA_DIR_PeripheralSRC;        /* 通道方向 */
        dma_initstruct.DMA_BufferSize          = DMA_RX_LEN;                   /* 内存缓存大小 */
        dma_initstruct.DMA_PeripheralInc       = DMA_PeripheralInc_Disable;    /* 外设地址不自增 */
        dma_initstruct.DMA_MemoryInc           = DMA_MemoryInc_Enable;         /* 内存地址自增 */
        dma_initstruct.DMA_PeripheralDataSize  = DMA_PeripheralDataSize_Byte;  /* 字节模式 */
        dma_initstruct.DMA_MemoryDataSize      = DMA_MemoryDataSize_Byte;      /* 字节模式 */
        dma_initstruct.DMA_Mode                = DMA_Mode_Normal;              /* 正常模式 */
        dma_initstruct.DMA_Priority            = DMA_Priority_Low;                 
        dma_initstruct.DMA_M2M                 = DMA_M2M_Disable;
        s_uart[pinfo->com].sel = 0;
    
        DMA_Init((DMA_Channel_TypeDef *)pinfo->dma_base_rx, &dma_initstruct);
    
        ST_IRQ_ConfigIrqEnable(irq_id, false);                                  /* 关闭中断 */
        ST_IRQ_InstallIrqHandler(irq_id, irqhandler);                           /* install irq handle */
        ST_IRQ_ConfigIrqPriority(irq_id, IRQ_PRIOTITY_RXDMA);
        ST_IRQ_ConfigIrqEnable(irq_id, true);                                   /* 打开中断 */
    }
}

/*******************************************************************
** 函数名称: STM32_UART_TxDMAStart
** 函数描述: 启动串口DMA发送功能
** 参数:     [in] uart_id: 串口通道编号
**           [in] memaddr: DMA内存地址
**           [in] memlen:  发送数据长度
** 返回:     无
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) static void STM32_UART_TxDMAStart(INT8U com, INT8U *memaddr, INT16U memlen)
{
    const UART_TBL_T *pinfo;
    DMA_Channel_TypeDef  *dma_channel;
    
    pinfo = ST_UART_GetRegTblInfo(com);
    dma_channel = (DMA_Channel_TypeDef *)pinfo->dma_base_tx;
    
    DMA_Cmd(dma_channel, DISABLE);                                             /* disable DMA channelx transfer */
    DMA_ITConfig(dma_channel, DMA_IT_TC, DISABLE);                             /* disable DMA channelx transfer complete interrupt */
    
    dma_channel->CMAR  = (INT32U)memaddr;
    dma_channel->CNDTR = memlen;
    
    DMA_ITConfig(dma_channel, DMA_IT_TC, ENABLE);                              /* enable DMA channelx transfer complete interrupt */
    DMA_Cmd(dma_channel, ENABLE);                                              /* enable DMA channelx transfer */
}

/*******************************************************************
** 函数名称: STM32_UART_IrqConfig
** 函数描述: 配置串口中断功能
** 参数:     [in] pinfo  : 配置参数
**           [in] inter  : 中断配置
** 返回:     无
********************************************************************/
static void STM32_UART_IrqConfig(const UART_TBL_T *pinfo)
{
    INT32S irq_id;
    IRQ_SERVICE_FUNC irqhandler;
    
    if (pinfo == 0) {
        return;
    }
    
    switch (pinfo->uart_base)
    {
    case USART1_BASE:
        irq_id = USART1_IRQn;
        irqhandler = (IRQ_SERVICE_FUNC)UART1_IrqHandle;
	  	break;
    case USART2_BASE:
        irq_id = USART2_IRQn;
        irqhandler = (IRQ_SERVICE_FUNC)UART2_IrqHandle;
        break;
    case USART3_BASE:
        irq_id = USART3_IRQn;
        irqhandler = (IRQ_SERVICE_FUNC)UART3_IrqHandle;
        break;
    default:
        return;
    }
    
    ST_IRQ_ConfigIrqEnable(irq_id, false);                                      /* 关闭中断 */
    ST_IRQ_InstallIrqHandler(irq_id, irqhandler);                               /* install irq handle */
    ST_IRQ_ConfigIrqPriority(irq_id, IRQ_PRIOTITY_UART);
    ST_IRQ_ConfigIrqEnable(irq_id, true);                                       /* 打开中断 */
    //USART_ITConfig((USART_TypeDef *)pinfo->uart_base, inter, ENABLE); /* enable the usart rx or tx interrupt */
}

/*******************************************************************
** 函数名称: STM32_UART_Init
** 函数描述: 串口通信参数配置，并设置是否支持DMA
** 参数:     [in]  cfg:串口配置参数 
** 返回:     无
********************************************************************/
BOOLEAN printf_com(const char *fmt, ...);
static void STM32_UART_Init(UART_CFG_T *cfg)
{
    INT8U com;
    INT16U stopbit, databit, parity, mode;
    INT32U baud;
    USART_InitTypeDef usart_initstruct;
    USART_ClockInitTypeDef usart_clock;
    const UART_TBL_T *pinfo;
	  
    com = cfg->com;
    pinfo = ST_UART_GetRegTblInfo(com);

    
	usart_clock.USART_Clock               = USART_Clock_Disable;
    usart_clock.USART_CPOL                = USART_CPOL_Low;
    usart_clock.USART_CPHA                = USART_CPHA_2Edge;
    usart_clock.USART_LastBit             = USART_LastBit_Disable;    

    
    baud = cfg->baud;
    switch (cfg->databit)
    {
	case UART_DATABIT_8:
	    databit = USART_WordLength_8b;
	    break;
	case UART_DATABIT_9:
	    databit = USART_WordLength_9b;
	    break;
	default:
	    databit = USART_WordLength_8b;
	    break;
	}
	     
    switch (cfg->stopbit)
    {
    case UART_STOPBIT_1:
        stopbit = USART_StopBits_1;
	    break;
	case UART_STOPBIT_1_5:
	    stopbit = USART_StopBits_1_5;
	    break;
	case UART_STOPBIT_2:
	    stopbit = USART_StopBits_2;
	    break;
	case UART_STOPBIT_0_5:
	    //stopbit = USART_StopBits_0_5;
        stopbit = USART_StopBits_1;
	    break;
	default:
	    stopbit = USART_StopBits_1;
	    break;
	}
	
	switch (cfg->parity)
	{
	case UART_PARITY_NONE:
	    parity = USART_Parity_No;
	    break;
	case UART_PARITY_EVEN:
	    parity = USART_Parity_Even;
	    break;
	case UART_PARITY_ODD:
	    parity = USART_Parity_Odd;
	    break;
	default:
	    parity = USART_Parity_No;
	    break;
	}
	mode = USART_Mode_Rx | USART_Mode_Tx;
	
	/* deinit usart */
	//USART_Cmd((USART_TypeDef *)pinfo->uart_base, DISABLE);
	USART_DeInit((USART_TypeDef *)pinfo->uart_base);
	
	usart_initstruct.USART_BaudRate            = baud;
    //usart_initstruct.USART_BaudRate            = 9600L;
	usart_initstruct.USART_WordLength          = databit;
	usart_initstruct.USART_StopBits            = stopbit;
	usart_initstruct.USART_Parity              = parity;
	usart_initstruct.USART_Mode                = mode;
	usart_initstruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
     /* usart clock enable */
     RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE); /* 开启gpio复用时钟 */
    if (pinfo->uart_base == USART1_BASE) {
        RCC_APB2PeriphClockCmd(pinfo->uart_rcc, ENABLE);
    } else {
        RCC_APB1PeriphClockCmd(pinfo->uart_rcc, ENABLE);
    }

	/* configure usart */
	USART_Init((USART_TypeDef *)pinfo->uart_base, &usart_initstruct);
    USART_ClockInit((USART_TypeDef *)pinfo->uart_base, &usart_clock);

	if (pinfo->dma_base_tx != 0 && pinfo->dma_base_rx != 0) {
	    /* enable usart dma tx and rx request */
	    USART_DMACmd((USART_TypeDef *)pinfo->uart_base, USART_DMAReq_Tx | USART_DMAReq_Rx, ENABLE);
	    STM32_UART_IrqConfig(pinfo);
	    USART_ITConfig((USART_TypeDef *)pinfo->uart_base, USART_IT_IDLE, ENABLE);
    } else if (pinfo->dma_base_tx != 0) {
        /* enable usart dma tx request */
        USART_DMACmd((USART_TypeDef *)pinfo->uart_base, USART_DMAReq_Tx, ENABLE);
        STM32_UART_IrqConfig(pinfo);
        USART_ITConfig((USART_TypeDef *)pinfo->uart_base, USART_IT_RXNE, ENABLE);
        //USART_ITConfig((USART_TypeDef *)pinfo->uart_base, USART_IT_ERR, ENABLE);
    } else if (pinfo->dma_base_rx != 0) {
        /* enable usart dma rx request */
        USART_DMACmd((USART_TypeDef *)pinfo->uart_base, USART_DMAReq_Rx, ENABLE);
        STM32_UART_IrqConfig(pinfo);
        USART_ITConfig((USART_TypeDef *)pinfo->uart_base, USART_IT_TXE, ENABLE);
        USART_ITConfig((USART_TypeDef *)pinfo->uart_base, USART_IT_IDLE, ENABLE);
    } else {
	    /* config usart rx or tx interrupt and enable it*/
        STM32_UART_IrqConfig(pinfo);
        USART_ITConfig((USART_TypeDef *)pinfo->uart_base, USART_IT_RXNE, ENABLE);
        USART_ITConfig((USART_TypeDef *)pinfo->uart_base, USART_IT_TXE, ENABLE);
	}

    /* enable the usart */
    USART_Cmd((USART_TypeDef *)pinfo->uart_base, ENABLE);
}

/*******************************************************************
** 函数名称: ST_UART_InitDrv
** 函数描述: 初始化串口驱动
** 参数:     无
** 返回:     无
********************************************************************/
void ST_UART_InitDrv(void)
{
    UART_CFG_T cfg;
    
    YX_MEMSET(&s_uart, 0, sizeof(s_uart));
    
    cfg.com     = UART_COM_0;
    cfg.baud    = 115200;
    cfg.parity  = UART_PARITY_NONE;
    cfg.databit = UART_DATABIT_8;
    cfg.stopbit = UART_STOPBIT_1;
    
    cfg.rx_fcm  = UART_FCM_NULL;     /* 接收流控模式,见UART_FCM_E */
    cfg.tx_fcm  = UART_FCM_NULL;     /* 发送流控模式,见UART_FCM_E */
    
    cfg.rx_len  = 256;               /* 配置平台接收缓存长度 */
    cfg.tx_len  = 512;               /* 配置平台发送缓存长度 */
    
    //ST_UART_OpenUart(&cfg);
    
    cfg.com     = UART_COM_1;
    cfg.baud    = 9600;
    cfg.rx_len  = 128;               /* 配置平台接收缓存长度 */
    cfg.tx_len  = 512;               /* 配置平台发送缓存长度 */
    ST_UART_OpenUart(&cfg);
}

/*******************************************************************
** 函数名称:   ST_UART_OpenUart
** 函数描述:   打开串口并初始化
** 参数:       [in]  cfg:串口配置参数 
** 返回:       成功返回true,失败返回false
********************************************************************/
BOOLEAN ST_UART_OpenUart(UART_CFG_T *cfg)
{
    INT8U com;
    const UART_TBL_T *pinfo;
    
    OS_ASSERT((cfg != 0), RETURN_FALSE);
    OS_ASSERT((cfg->com < UART_COM_MAX), RETURN_FALSE);
    OS_ASSERT((cfg->tx_len != 0), RETURN_FALSE);
    OS_ASSERT((cfg->rx_len != 0), RETURN_FALSE);

    
    com = cfg->com;
    pinfo = ST_UART_GetRegTblInfo(com);
    if (pinfo->enable == 0) {
        return false;
    }
    
    if (com == UART_COM_2) {
        s_uart1map = UART_COM_2 - UART_COM_0;
    }
    
    if ((s_uart[com].status & _OPEN) != 0) {                                   /* 串口已打开 */
        s_uart[com].status &= ~(_OPEN | _SENDING);
        USART_Cmd((USART_TypeDef *)pinfo->uart_base, DISABLE);
        USART_DeInit((USART_TypeDef *)pinfo->uart_base);
        
        if (pinfo->dma_base_tx != 0) {
            DMA_DeInit((DMA_Channel_TypeDef *)pinfo->dma_base_tx);
        }
        
        if (pinfo->dma_base_rx != 0) {
            DMA_DeInit((DMA_Channel_TypeDef *)pinfo->dma_base_rx);
        }
    }
    
    if (s_uart[com].p_rx == 0 || s_uart[com].rx_len != cfg->rx_len) {
        if (s_uart[com].p_rx != 0) {
            YX_DYM_Free(s_uart[com].p_rx);
        }
        s_uart[com].p_rx = YX_DYM_Alloc(cfg->rx_len);
        if (s_uart[com].p_rx == 0) {
            #if DEBUG_ERR > 0
            printf_com("<p_rx malloc memory fail>\r\n");
            #endif
            return false;
        }
        
        s_uart[com].rx_len = cfg->rx_len;
        YX_InitLoopBuffer(&s_uart[com].r_round, s_uart[com].p_rx, s_uart[com].rx_len);
    }
    
    if (s_uart[com].p_tx == 0 || s_uart[com].tx_len != cfg->tx_len) {
        if (s_uart[com].p_tx != 0) {
            YX_DYM_Free(s_uart[com].p_tx);
        }
        s_uart[com].p_tx = YX_DYM_Alloc(cfg->tx_len);
        if (s_uart[com].p_tx == 0) {
            #if DEBUG_ERR > 0
            printf_com("<p_tx malloc memory fail>\r\n");
            #endif
            return false;
        }
        
        s_uart[com].tx_len = cfg->tx_len;
        YX_InitLoopBuffer(&s_uart[com].s_round, s_uart[com].p_tx, s_uart[com].tx_len);
    }
    
    STM32_UART_PinsConfig(pinfo, TRUE);                                        /* configure the rx & tx pins */
    STM32_UART_DMAConfig(pinfo);                                               /* config uart1-4 tx DMA mode */
    STM32_UART_Init(cfg);                                                      /* 通信参数配置 */
    
    s_uart[com].status = _OPEN;
    return true;
}

/*******************************************************************
** 函数名:     ST_UART_CloseUart
** 函数描述:   关闭串口
** 参数:       [in] com: 通道编号,见UART_COM_E
** 返回:       成功返回true, 失败返回false
********************************************************************/
BOOLEAN ST_UART_CloseUart(INT8U com)
{
    const UART_TBL_T *pinfo;
    
    if (com >= UART_COM_MAX) {
        return false;
    }
    
    pinfo = ST_UART_GetRegTblInfo(com);
    if (pinfo->enable == 0) {
        return false;
    }
    
    if (com == UART_COM_2) {
        s_uart1map = 0;
    }
    
    if ((s_uart[com].status & _OPEN) != 0) {                                   /* 串口已打开 */
        s_uart[com].status &= ~(_OPEN | _SENDING);
        STM32_UART_PinsConfig(pinfo, FALSE);
        USART_Cmd((USART_TypeDef *)pinfo->uart_base, DISABLE);
        USART_DeInit((USART_TypeDef *)pinfo->uart_base);
        
        if (pinfo->dma_base_tx != 0) {
            DMA_DeInit((DMA_Channel_TypeDef *)pinfo->dma_base_tx);
        }
        
        if (pinfo->dma_base_rx != 0) {
            DMA_DeInit((DMA_Channel_TypeDef *)pinfo->dma_base_rx);
        }
    }
    s_uart[com].status &= ~(_OPEN | _SENDING);
    
    if (s_uart[com].p_rx != 0) {
        YX_DYM_Free(s_uart[com].p_rx);
        s_uart[com].p_rx = 0;
        s_uart[com].rx_len = 0;
    }
    
    if (s_uart[com].p_tx != 0) {
        YX_DYM_Free(s_uart[com].p_tx);
        s_uart[com].p_tx = 0;
        s_uart[com].tx_len = 0;
    }
    
    return true;
}

/*******************************************************************
** 函数名称: ST_UART_ReadChar
** 函数描述: 获取一个字节数据
** 参数:     [in] com: 通道编号,见UART_COM_E
** 返回:     成功返回串口数据，失败返回-1
********************************************************************/
INT32S ST_UART_ReadChar(INT8U com)
{
    if (com >= UART_COM_MAX) {
        return -1;
    }
    
    if ((s_uart[com].status & _OPEN) == 0) {
        return -1;
    }
    
    return YX_ReadLoopBuffer(&s_uart[com].r_round);
}

/*******************************************************************
** 函数名称: ST_UART_WriteChar
** 函数描述: 发送一个字节
** 参数:     [in] com: 通道编号,见UART_COM_E
**           [in] data:字节数据
** 返回:     成功返回true,失败返回false
********************************************************************/
BOOLEAN ST_UART_WriteChar(INT8U com, INT8U data)
{
    INT8U result;
    INT16U i, bufsize;
    INT32U cpu_sr;

    if (com >= UART_COM_MAX) {
        return FALSE;
    }
    
    if ((s_uart[com].status & _OPEN) == 0) {
        return FALSE;
    }
    
    result = YX_WriteLoopBuffer(&s_uart[com].s_round, data);
    
    OS_ENTER_CRITICAL();
    if ((s_uart[com].status & _SENDING) == 0) {
        OS_EXIT_CRITICAL();
        
        bufsize = YX_UsedOfLoopBuffer(&s_uart[com].s_round);
        if (bufsize > 0) {
            if (bufsize > DMA_TX_LEN) {
                bufsize = DMA_TX_LEN;
            }
    
            for (i = 0; i < bufsize; i++) {
                s_uart[com].p_dma_tx[i] = YX_ReadLoopBuffer(&s_uart[com].s_round);
            }
            
            s_uart[com].status |= _SENDING;
            STM32_UART_TxDMAStart(com, s_uart[com].p_dma_tx, bufsize);
        }
    } else {
        OS_EXIT_CRITICAL();
    }
    
    return result;
}

/*******************************************************************
** 函数名称: ST_UART_WriteCharWait
** 函数描述: 发送一个字节,等待发送完毕
** 参数:     [in] com: 通道编号,见UART_COM_E
**           [in] data:字节数据
** 返回:     成功返回true,失败返回false
********************************************************************/
BOOLEAN ST_UART_WriteCharWait(INT8U com, INT8U data)
{
    const UART_TBL_T *pinfo;
    
    if (com >= UART_COM_MAX) {
        return false;
    }
    
    pinfo = ST_UART_GetRegTblInfo(com);
    if (pinfo->enable == 0) {
        return false;
    }

    USART_SendData((USART_TypeDef *)pinfo->uart_base, data);
    while(USART_GetFlagStatus((USART_TypeDef *)pinfo->uart_base, USART_FLAG_TXE) == RESET)
    {
    }
    #if 0
    //while ((((USART_TypeDef *)pinfo->uart_base)->ISR & 0x80) == 0) {};
    ((USART_TypeDef *)pinfo->uart_base)->DR = (data & 0x01FF);
    while ((((USART_TypeDef *)pinfo->uart_base)->SR & 0x40) == 0) {};
    #endif
    return true;
}

/*******************************************************************
** 函数名称: ST_UART_WriteBlock
** 函数描述: 发送一串数据
** 参数:     [in] com: 通道编号,见UART_COM_E
**           [in] sptr:数据指针
**           [in] slen:数据长度
** 返回:     成功返回true,失败返回false
********************************************************************/
BOOLEAN ST_UART_WriteBlock(INT8U com, INT8U *sptr, INT16U slen)
{
    INT8U result;
    INT16U i, bufsize;
    INT32U cpu_sr;
	  
    if ((sptr == 0) || (slen == 0)) {
        return FALSE;
    }
    
    if (com >= UART_COM_MAX) {
        return FALSE;
    }
    
    if ((s_uart[com].status & _OPEN) == 0) {
        return FALSE;
    }
    
    result = YX_WriteBlockLoopBuffer(&s_uart[com].s_round, sptr, slen);
        
    OS_ENTER_CRITICAL();
    if ((s_uart[com].status & _SENDING) == 0) {
        OS_EXIT_CRITICAL();
        
        bufsize = YX_UsedOfLoopBuffer(&s_uart[com].s_round);
        if (bufsize > 0) {
            if (bufsize > DMA_TX_LEN) {
                bufsize = DMA_TX_LEN;
            }
    
            for (i = 0; i < bufsize; i++) {
                s_uart[com].p_dma_tx[i] = YX_ReadLoopBuffer(&s_uart[com].s_round);
            }

    		s_uart[com].status |= _SENDING;
            STM32_UART_TxDMAStart(com, s_uart[com].p_dma_tx, bufsize);
        }
    } else {
        OS_EXIT_CRITICAL();
    }
    
    return result;
}

/*******************************************************************
** 函数名称: ST_UART_GetRecvBytes
** 函数描述: 获取已接收字节数
** 参数:     [in] com: 通道编号,见UART_COM_E
** 返回:     已接收字节数
********************************************************************/
INT32U ST_UART_GetRecvBytes(INT8U com)
{
    if (com >= UART_COM_MAX) {
        return 0;
    }
    
    if ((s_uart[com].status & _OPEN) == 0) {
        return 0;
    }
    
    return YX_UsedOfLoopBuffer(&s_uart[com].r_round);
}

/*******************************************************************
** 函数名称: ST_UART_LeftOfSendbuf
** 函数描述: 获取发送缓存剩余空间
** 参数:     [in] com: 通道编号,见UART_COM_E
** 返回:     剩余空间字节数
********************************************************************/
INT32U ST_UART_LeftOfSendbuf(INT8U com)
{
    if (com >= UART_COM_MAX) {
        return 0;
    }
    
    if ((s_uart[com].status & _OPEN) == 0) {
        return 0;
    }
    
    return YX_LeftOfLoopBuffer(&s_uart[com].s_round);
}

/*******************************************************************
** 函数名称: UARTx_TxDMAIrqService
** 函数描述: DMA发送中断处理
** 参数:     [in] com: 通道编号,见UART_COM_E
** 返回:     无
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void UARTx_TxDMAIrqService(INT8U com)
{
    INT16U i, bufsize;
	const UART_TBL_T *pinfo;
	
	if (com >= UART_COM_MAX) {
	    return;
	}
	
	com = (com == 0) ? (com + s_uart1map) : com;
	    
    pinfo = ST_UART_GetRegTblInfo(com);
    if (((s_uart[com].status & _OPEN) == 0) || ((s_uart[com].status & _SENDING) == 0)) {
        DMA_Cmd((DMA_Channel_TypeDef *)pinfo->dma_base_tx, DISABLE);
        DMA_ITConfig((DMA_Channel_TypeDef *)pinfo->dma_base_tx, DMA_IT_TC, DISABLE);
        return;
    }
    
    bufsize = YX_UsedOfLoopBuffer(&s_uart[com].s_round);
    if (bufsize == 0) {
        s_uart[com].status &= (~_SENDING);
        DMA_ITConfig((DMA_Channel_TypeDef *)pinfo->dma_base_tx, DMA_IT_TC, DISABLE);
        DMA_Cmd((DMA_Channel_TypeDef *)pinfo->dma_base_tx, DISABLE);
        return;
    }
    
    if (bufsize > DMA_TX_LEN) {
        bufsize = DMA_TX_LEN;
    }
    
    for (i = 0; i < bufsize; i++) {
        s_uart[com].p_dma_tx[i] = YX_ReadLoopBuffer_INT(&s_uart[com].s_round);
    }
    
    STM32_UART_TxDMAStart(com, s_uart[com].p_dma_tx, bufsize);
}

/*******************************************************************
** 函数名称: UARTx_RxDMAIrqService
** 函数描述: DMA接收中断处理
** 参数:     [in] com: 通道编号,见UART_COM_E
** 返回:     无
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void UARTx_RxDMAIrqService(INT8U com)
{
    INT16U bufsize;
	const UART_TBL_T *pinfo;
	DMA_Channel_TypeDef *dma_register_base;
	
	if (com >= UART_COM_MAX) {
	    return;
	}
    
    com = (com == 0) ? (com + s_uart1map) : com;
	    
    pinfo = ST_UART_GetRegTblInfo(com);
	dma_register_base = (DMA_Channel_TypeDef *)pinfo->dma_base_rx;
    if ((s_uart[com].status & _OPEN) == 0) {
        DMA_Cmd(dma_register_base, DISABLE);
        return;
    }
    
    bufsize = DMA_RX_LEN - DMA_GetCurrDataCounter(dma_register_base);
    
    DMA_Cmd(dma_register_base, DISABLE);  //停止使能 才能修改计数器
    if (s_uart[com].sel == 0) {
        dma_register_base->CMAR  = (INT32U)s_uart[pinfo->com].p_dma_rx1;
        dma_register_base->CNDTR = DMA_RX_LEN;
    } else {
        dma_register_base->CMAR  = (INT32U)s_uart[pinfo->com].p_dma_rx0;
        dma_register_base->CNDTR = DMA_RX_LEN;
    }
    DMA_Cmd(dma_register_base, ENABLE);
    
    if (s_uart[com].sel == 0) {
        s_uart[com].sel = 1;
        YX_WriteBlockLoopBuffer_INT(&s_uart[com].r_round, s_uart[pinfo->com].p_dma_rx0, bufsize);
    } else {
        s_uart[com].sel = 0;
        YX_WriteBlockLoopBuffer_INT(&s_uart[com].r_round, s_uart[pinfo->com].p_dma_rx1, bufsize);
    }
}

/*******************************************************************
** 函数名称: Uartx_TxIrqService
** 函数描述: UART发送中断处理
** 参数:     [in] com:                 通道编号,见UART_COM_E
**           [in] uart_base:  uart寄存器基址
** 返回:     无
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void Uartx_TxIrqService(INT8U com, INT32U uart_base)
{
    INT32S ch;
    
    if (com >= UART_COM_MAX) {
	    return;
	}
    
    com = (com == 0) ? (com + s_uart1map) : com;
	
	if ((s_uart[com].status & _OPEN) != 0) {
	    ch = YX_ReadLoopBuffer_INT(&s_uart[com].s_round);
	    if (ch != -1) {
	        ((USART_TypeDef *)uart_base)->DR = (ch & 0x01FF);
	    }
	} else {
	    USART_ITConfig((USART_TypeDef *)uart_base, USART_IT_TXE, DISABLE);
	}
}

/*******************************************************************
** 函数名称: Uartx_RxIrqService
** 函数描述: UART接收中断处理
** 参数:     [in] com:                 通道编号,见UART_COM_E
**           [in] uart_base:  uart寄存器基址
**           [in] data:                字节数据
** 返回:     无
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void Uartx_RxIrqService(INT8U com, INT32U uart_base, INT16U data)
{
    if (com >= UART_COM_MAX) {
	    return;
	}
	
	com = (com == 0) ? (com + s_uart1map) : com;
	
	if ((s_uart[com].status & _OPEN) != 0) {
	    YX_WriteLoopBuffer_INT(&s_uart[com].r_round, data);
	} else {
	    USART_ITConfig((USART_TypeDef *)uart_base, USART_IT_RXNE, DISABLE);
	}
}




