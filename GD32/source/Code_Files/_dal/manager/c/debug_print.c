/********************************************************************************
**
** 文件名:     debug_print.c
** 版权所有:   (c) 2014-2020 厦门雅迅网络股份有限公司
** 文件描述:   系统调试打印函数接口
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2010/12/30 | LEON   | 创建本模块
**
*********************************************************************************/
#include "dal_include.h"
#include "tools.h"
#include "dal_usart.h"
#include "debug_print.h"
#include "gd32f30x_usart.h"

#define DBG_BUF_SIZE    1024
#define STAT_INIT_      0x01

typedef struct {
    INT8U stat;
    INT8U *dbg_buf;
} DBG_CB_T;

static INT8U s_printf_buf[DBG_BUF_SIZE];
static BOOLEAN debug_sw = TRUE;

static DBG_CB_T s_dbg_cb = {STAT_INIT_, s_printf_buf};

/*************************************************************************************************/
/*                                     重定义系统打印函数                                        */
/*************************************************************************************************/
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE  int  __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE  int  fputc(int ch, FILE *f)
#endif

PUTCHAR_PROTOTYPE
{
    //Debug_PrintByte((INT8U)ch);                                      /* Write a character to USART */
	usart_data_transmit(USART2, (INT8U)ch);
	while (RESET == usart_flag_get(USART2,USART_FLAG_TC));
	
    return ch;
}

/*******************************************************************
** 函数名:     Debug_PrintByte
** 函数描述:   打印输出单个字节
** 参数:       [in] byte   数据
** 返回:       无
********************************************************************/
void Debug_PrintByte(INT8U byte)
{
    #if 1
    USARTX_SendData(USART_DEBUG, &byte, 1);
    #if 0
    switch (USART_DEBUG)
    {
        case USART_NO1:
            //USART_SendData(USART1, (INT16U)byte);
            //while ((USART1->SR & USART_SR_TXE) == 0) {};
            USARTX_SendData(USART_DEBUG, &byte, 1);
            break;

        case USART_NO2:
            USART_SendData(USART2, (INT16U)byte);
            while ((USART2->SR & USART_SR_TXE) == 0) {};
            break;

        case USART_NO3:
            USART_SendData(USART3, (INT16U)byte);
            while ((USART3->SR & USART_SR_TXE) == 0) {};
            break;

        case USART_NO4:
            USART_SendData(UART4, (INT16U)byte);
            while ((UART4->SR & USART_SR_TXE) == 0) {};
            break;

        case USART_NO5:
            USART_SendData(UART5, (INT16U)byte);
            while ((UART5->SR & USART_SR_TXE) == 0) {};
            break;

        default:
            DAL_ASSERT(0);                                           /* 此处加入出错指示 */
            break;
    }
    #endif
  #endif
}

/*******************************************************************
** 函数名:     Debug_PrintCRLF
** 函数描述:   在打印信息后补加回车换行符
** 参数:       无
** 返回:       无
********************************************************************/
void Debug_PrintCRLF(void)
{
    Debug_PrintByte('\r');
    Debug_PrintByte('\n');
}

/**************************************************************************************************
**  函数名称:  Debug_PrintHex
**  功能描述:  以16进制格式打印连续数据
**  输入参数:
**  输出参数:
**************************************************************************************************/
void Debug_PrintHex(BOOLEAN end, INT8U *ptr, INT16U size)
{
    INT8U  ch[3];
    INT16U i;

    for (i = 0; i < size; i++) {
        //ch = ;
		sprintf(ch, "%02x ", *ptr++);
        Debug_PrintByte(ch[0]);
        Debug_PrintByte(ch[1]);
        Debug_PrintByte(ch[2]);
    }

    if (end == TRUE) Debug_PrintCRLF();                              /* 在结尾处补加一个回车换行符 */
}

/*******************************************************************
** 函数名:     Debug_PrintStr
** 函数描述:   以字符串形式打印连续内容
** 参数:       [in] ptr   数据
** 返回:       无
********************************************************************/
void Debug_PrintStr(char *ptr)
{
    INT32U i, tlen;

    tlen = strlen(ptr);

    for (i = 0; i < tlen; i++) {
        Debug_PrintByte(*ptr++);
    }
}

/*******************************************************************
** 函数名:     Debug_Initiate
** 函数描述:   初始化打印串口
** 参数:       无
** 返回:       无
********************************************************************/
void Debug_Initiate(void)
{
#if EN_DEBUG > 0
		USART_PARA_T usart;
	
		usart.baud		= BAUD_DEBUG;
		usart.databits	= DATABITS_8;
		usart.parity	= PARITY_NONE;
		usart.stopbits	= STOPBITS_1;
	
		s_dbg_cb.dbg_buf = s_printf_buf;
		s_dbg_cb.stat |= STAT_INIT_;
	
		USARTX_IOConfig(USART_DEBUG); 
	
		switch (USART_DEBUG)											 /* 不开启中断，仅使能串口功能 */
		{
			case USART_NO1:
				//RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
				//USART_Init(USART0, &ICB);
				//USART_ClockInit(USART0, &ICL);
				//USART_Cmd(USART0, ENABLE);
				usart_deinit(USART0);
				rcu_periph_clock_enable(RCU_USART0);
				usart_baudrate_set(USART0, BAUD_DEBUG);
				usart_receive_config(USART0,USART_RECEIVE_ENABLE);
				usart_transmit_config(USART0,USART_TRANSMIT_ENABLE);
				usart_dma_receive_config(USART0,USART_DENR_ENABLE);
				usart_dma_transmit_config(USART0,USART_DENT_ENABLE);
				usart_interrupt_disable(USART0,USART_INT_RBNE);
				usart_enable(USART0);
				break;
	
			case USART_NO2:
				//RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
				usart_deinit(USART1);
				rcu_periph_clock_enable(RCU_USART1);
				usart_baudrate_set(USART1, BAUD_DEBUG);
				usart_receive_config(USART1,USART_RECEIVE_ENABLE);
				usart_transmit_config(USART1,USART_TRANSMIT_ENABLE);
				usart_dma_receive_config(USART1,USART_DENR_ENABLE);
				usart_dma_transmit_config(USART1,USART_DENT_ENABLE);
				usart_interrupt_disable(USART1,USART_INT_RBNE);
				usart_enable(USART1);
				break;
	
			case USART_NO3:
				usart_deinit(USART2);
				rcu_periph_clock_enable(RCU_USART2);
				usart_baudrate_set(USART2, BAUD_DEBUG);
				usart_receive_config(USART2,USART_RECEIVE_ENABLE);
				usart_transmit_config(USART2,USART_TRANSMIT_ENABLE);
				//usart_dma_receive_config(USART2,USART_DENR_ENABLE);
				//usart_dma_transmit_config(USART2,USART_DENT_ENABLE);
				usart_interrupt_disable(USART2,USART_INT_RBNE);
				usart_enable(USART2);
				break;


        case USART_NO4:
            usart_deinit(UART3);
				rcu_periph_clock_enable(UART3);
				usart_baudrate_set(UART3, BAUD_DEBUG);
				usart_receive_config(UART3,USART_RECEIVE_ENABLE);
				usart_transmit_config(UART3,USART_TRANSMIT_ENABLE);
				//usart_dma_receive_config(USART2,USART_DENR_ENABLE);
				//usart_dma_transmit_config(USART2,USART_DENT_ENABLE);
				usart_interrupt_disable(UART3,USART_INT_RBNE);
				usart_enable(UART3);
            break;

        case USART_NO5:
            rcu_periph_clock_enable(UART4);
				usart_baudrate_set(UART4, BAUD_DEBUG);
				usart_receive_config(UART4,USART_RECEIVE_ENABLE);
				usart_transmit_config(UART4,USART_TRANSMIT_ENABLE);
				//usart_dma_receive_config(USART2,USART_DENR_ENABLE);
				//usart_dma_transmit_config(USART2,USART_DENT_ENABLE);
				usart_interrupt_disable(UART4,USART_INT_RBNE);
				usart_enable(UART4);
				break;

        default:
            DAL_ASSERT(0);                                           /* 此处加入出错指示 */
            break;
    }
#endif
}

/*******************************************************************
** 函数名:     Debug_Print_Sw
** 函数描述:   打印开关(主要用于加密芯片调用debug_printf打印接口输出控制)
** 参数:       [in] db_sw : TRUE:打开，FALSE:关闭
** 返回:       无
********************************************************************/
void Debug_Print_Sw(BOOLEAN db_sw) 
{
    if (db_sw == TRUE) {
        debug_sw = TRUE;
    } else {
        debug_sw = FALSE;
    }
}

/*******************************************************************
** 函数名:     debug_printf
** 函数描述:   打印接口
** 参数:       无
** 返回:       无
********************************************************************/
BOOLEAN debug_printf(const char * fmt, ...)
{
    #if 1
    if (debug_sw == FALSE) {
        return TRUE;
    } else {
        va_list ap;
        INT16U len;
        
        if ((s_dbg_cb.stat & STAT_INIT_) == 0) return FALSE;
        va_start(ap, fmt);
        len = vsnprintf((char *)s_dbg_cb.dbg_buf, DBG_BUF_SIZE, fmt, ap);
        va_end(ap);
        
        return USARTX_SendData(USART_DEBUG, s_dbg_cb.dbg_buf, len);
    }
    #else 
    return TRUE;
    #endif
} 


