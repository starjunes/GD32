/******************************************************************************
** 文件名:     yx_enc_fun.c
** 版权所有:   (c) 2005-2019 厦门雅迅网络股份有限公司
** 文件描述:   该模块实现加密芯片spi接口和其他接口函数(供加密芯片驱动库调用)
******************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者    |  修改记录
**===============================================================================
**| 2019 /4/30 | cym |  创建第一版本
*********************************************************************************/
#include "app_include.h"
#include "port_dm.h"
#include "port_gpio.h"
#include "port_spi.h"
#include "debug_print.h"
#include "yx_enc_fun.h"
/*
********************************************************************************
* 定义宏
********************************************************************************
*/

#define ENC_SPI              PORT_SPI1  
#define EN_RESET_SPI         1
#define ENC_INT_PORT         PORT_C
#define ENC_INT_PIN          GPIO_PIN4     /* HSM2_INT */
#define ENC_WK_PORT          PORT_C
#define ENC_WK_PIN           GPIO_PIN13    /* WAKE_HSM2 */    
#define ENC_PW_PORT          PORT_B
#define ENC_PW_PIN           GPIO_PIN10    /* 电源 */

#define ENC_SPI_PIN_PORT     PORT_A
#define ENC_SPI_PIN_CS       GPIO_PIN4
#define ENC_SPI_PIN_SCK      GPIO_PIN5
#define ENC_SPI_PIN_MISO     GPIO_PIN6
#define ENC_SPI_PIN_MOSI     GPIO_PIN7

/* 根据SystemCoreClock 修改 */
#define US_RATIO 120

/* (SystemCoreClock/1000)*10 - 1,因为设置systick 为10ms定时 */
#define SYSTICK_RELOAD_VAL 1199999    //系统频率:SYS_CLK_FREQ
         

/* spi传输等待 */
#define MAX_TRANS_DELAY_CNT 2000//2000 
#define MAX_SPI_ERR_OVER 3

#define ENC_SPI_MODE         1    /* 1:使用loop模式, 0:使用中断模式 */
/*
********************************************************************************
* 定义本地全局变量
********************************************************************************
*/
INT32U nts_se_debug_level = 0;

/* spi通讯错误统计 */
static INT8U err_cnt = 0;

/*
********************************************************************************
* 定义全局函数
********************************************************************************
*/

/*******************************************************************
** 函数名:     spi_malloc
** 函数描述:   申请分配内存接口
** 参数:       [in] datalen: 申请内存的长度
** 返回:       成功返回内存指针，失败返回0
********************************************************************/
void *spi_malloc(INT32U size)
{
    return PORT_Malloc(size);
}

/*******************************************************************
** 函数名:     spi_free
** 函数描述:   释放内存接口
** 参数:       [in] sptr: 释放内存的地址
** 返回:       无
********************************************************************/
void spi_free(void *pmem)
{
    PORT_Free(pmem);
}
    
/*******************************************************************
** 函数名:     hal_ClearWatchdogHS
** 函数描述:   清看门狗
** 参数:       无
** 返回:       无
********************************************************************/
void hal_ClearWatchdogHS(void)
{
    //ClearWatchdog();
}

/*****************************************************************************
**  函数名:  delay_init
**  函数描述: 初始化延迟函数
**  参数:        无
**  返 回 : 
*****************************************************************************/
void delay_init()	 
{ 
//	xx
}								    

/*****************************************************************************
**  函数名:  delay_us
**  函数描述: 延时nus微秒
**  参数:       [in] nus:延时us数
**  返 回 : 无
*****************************************************************************/
void delay_us(unsigned int nus)
{  
    INT32U delay_goal,delay_start,delay_time,delay_curr;
    INT32S x = 0;
    delay_time = nus;
    delay_time *= US_RATIO + 20;    //假设为8M，则1微秒为nus*8 个SysTick->VAL值
    delay_start = SysTick->VAL;//SysTick->VAL为递减
    x = delay_start - delay_time;

    if (x < 0) {
    	delay_goal = x + SYSTICK_RELOAD_VAL;
    	do {
    		delay_curr = SysTick->VAL;
    	} while((delay_curr > delay_goal) || (delay_curr < delay_start));
    } else {
    	delay_goal = x; 
    	do{
    		delay_curr = SysTick->VAL;
    	}while((delay_curr > delay_goal) && (delay_curr < delay_start));
    }
}

/*****************************************************************************
**  函数名:  delay_us
**  函数描述: 延时nus微秒
**  参数:       [in] nus:延时us数
**  返 回 : 无
*****************************************************************************/
void delay_us_for_ms(unsigned int nus)
{  
    INT32U delay_goal,delay_start,delay_time,delay_curr;
    INT32S x = 0;
    delay_time = nus + 10;
    delay_time *= US_RATIO;    //假设为8M，则1微秒为nus*8 个SysTick->VAL值
    delay_start = SysTick->VAL;//SysTick->VAL为递减
    x = delay_start - delay_time;

    if (x < 0) {
    	delay_goal = x + SYSTICK_RELOAD_VAL;
    	do {
    		delay_curr = SysTick->VAL;
    	} while((delay_curr > delay_goal) || (delay_curr < delay_start));
    } else {
    	delay_goal = x; 
    	do{
    		delay_curr = SysTick->VAL;
    	}while((delay_curr > delay_goal) && (delay_curr < delay_start));
    }
}

/*****************************************************************************
**  函数名:  delay_ms
**  函数描述: 延时mus毫秒
**  参数:       [in] nms:延时ms数
**  返 回 : 无
*****************************************************************************/
void delay_ms(unsigned int nms)
{
    INT16U i;
    for(i = 0; i < nms; i++){
        delay_us_for_ms(1000);
    }
} 

/*****************************************************************************
**  函数名:  gpio_cs_init
**  函数描述: 将一个GPIO初始化为SPI的CS引脚
**  参数:   :无
**  返 回 : 无
*****************************************************************************/
void gpio_cs_init(void)
{
    PORT_GpioOutLevel(ENC_SPI_PIN_PORT, ENC_SPI_PIN_CS, TRUE);
}

/*****************************************************************************
**  函数名:  gpio_cs_sel
**  函数描述: SPI协议中利用GPIO选中SPI设备
**  参数:   :无
**  返 回 : 无
*****************************************************************************/
void gpio_cs_sel(void)
{
    PORT_GpioOutLevel(ENC_SPI_PIN_PORT, ENC_SPI_PIN_CS, FALSE);
}

/*****************************************************************************
**  函数名:  gpio_cs_desel
**  函数描述: SPI协议中利用GPIO释放SPI设备
**  参数:   :无
**  返 回 : 无 
*****************************************************************************/
void gpio_cs_desel(void)
{
    PORT_GpioOutLevel(ENC_SPI_PIN_PORT, ENC_SPI_PIN_CS, TRUE);   
}

/*****************************************************************************
**  函数名:  gpio_reset
**  函数描述: 将一个GPIO初始化为复位引脚，并对SE设备硬件复位
**  参数:       [in]void  :
**  返 回 : 
*****************************************************************************/
void gpio_reset(void)
{
    #if 0
    PORT_GpioOutLevel(ENC_WK_PORT, ENC_WK_PIN, TRUE);
    delay_ms(10);
    PORT_GpioOutLevel(ENC_WK_PORT, ENC_WK_PIN, FALSE);
    delay_ms(10);
    #endif
}

/*****************************************************************************
**  函数名:  yx_spi_init
**  函数描述: MCU的spi接口初始化
**  参数:       [in]handle  :spi句柄
**              [in]para    :
**  返 回 : 是否成功，成功返回E_SUCCESS
*****************************************************************************/
unsigned long yx_spi_init(void **handle, void *para)
{
    PORT_SPI_INIT_T spi_init;
    spi_init.mode = PORT_SPI_MODE_MASTER;
    #if ENC_SPI_MODE > 0
    spi_init.hdl_mode = PORT_SPI_HDL_MODE_LOOP;
    #else
    spi_init.hdl_mode = PORT_SPI_HDL_MODE_INT;
    #endif
    spi_init.clock_mode = PORT_SPI_CLOCK_MODE_0;
    spi_init.baud = 1000;//1000K
    PORT_SpiOpen(ENC_SPI, spi_init);
    return E_SUCCESS;
}

/*****************************************************************************
**  函数名:  spi_deinit
**  函数描述: MCU的spi接口释放，句柄清空
**  参数:       [in]handle  :spi句柄
**  返 回 : 是否成功，成功返回E_SUCCESS
*****************************************************************************/
unsigned long spi_deinit(void *handle)
{
    PORT_SpiClose(ENC_SPI);
    return E_SUCCESS;
}

/*****************************************************************************
**  函数名:   spi_reset
**  函数描述: 复位spi
**  参数:     无
**  返 回 :   无
*****************************************************************************/
static void spi_reset(void)
{
    #if EN_RESET_SPI > 0
    if (++err_cnt >= MAX_SPI_ERR_OVER) {
        err_cnt = 0;
        /* 重启spi */
        spi_deinit(0);
        delay_ms(1);
        yx_spi_init(0, 0);
        delay_ms(1);  
    }		
    #endif
}

#if ENC_SPI_MODE == 0
/*****************************************************************************
**  函数名:  spi_ReadBytes
**  函数描述: 读取数据串
**  参数:   :无
**  返 回 : 无
*****************************************************************************/
static BOOLEAN spi_ReadBytes(INT8U idx, INT8U* pdata, INT32U len)
{
    INT16U i, j, k;
    INT32S read_byte;
    for (i = 0; i < len; ) {
        if (PORT_SpiRecvBufUsed(idx) >= len) {
            for (k = 0; k < len; k++) {
                read_byte = PORT_SpiReadByte(idx);
                if (read_byte != (-1)) {
                    pdata[k] = (INT8U)read_byte;
                }
            }
            break;
        }
        
        if (i == len) break;
        
        if (++j >= MAX_TRANS_DELAY_CNT) return FALSE;
        
        delay_us(10);
    }
    return TRUE;
}
#endif

/*****************************************************************************
**  函数名:  spi_transmittx
**  函数描述: spi数据发送
**  参数:       [in]handle:spi句柄
**              [in]pdata:发送的数据指针
**              [in]data_len:发送数据长度
**              [in]delay:片选延时微秒数
**  返 回 : 是否成功，成功返回E_SUCCESS
*****************************************************************************/
unsigned long spi_transmittx(void *handle,unsigned char* pdata,unsigned int data_len,unsigned int delay)
{
    INT8U ret;
    INT16U i;
    gpio_cs_sel();
    if (delay != 0) {
        delay_us(delay);
    } else {
        delay_us(20);
    }
    #if ENC_SPI_MODE > 0
    /* loop模式 */
    i = 0;
    ret = PORT_SpiSendData(ENC_SPI, pdata, data_len);
    #else
    /* 中断模式 */
    ret = PORT_SpiSendData(ENC_SPI, pdata, data_len);
    for (i = 0; i < MAX_TRANS_DELAY_CNT; i++) {
        delay_us(10);
        if (PORT_SpiIsSendIle(ENC_SPI)){
            break;
        }
    }
    #endif
    delay_us(10);
    gpio_cs_desel();
    delay_us(20);
    if (i == MAX_TRANS_DELAY_CNT) {
		spi_reset();
        return E_COMMUNICATE_FAILURE;
    }
    if (ret == FALSE) {
		spi_reset();
        return E_COMMUNICATE_FAILURE;
    }
    return E_SUCCESS;
}

/*****************************************************************************
**  函数名:  spi_transmitrx
**  函数描述: spi数据接收
**  参数:       [in]handle:spi句柄
**              [in]pdata:发送的数据指针
**              [in]data_len:发送数据长度
**              [in]delay:片选延时微秒数
**  返 回 : 是否成功，成功返回E_SUCCESS
*****************************************************************************/
unsigned long spi_transmitrx(void *handle, unsigned char * pdata,unsigned int data_len,unsigned int delay)
{
    INT8U ret;
    INT16U i;
    INT32S read_byte;

    #if ENC_SPI_MODE == 0
    INT8U rx_send_empty_buf[32];
    if (data_len > 16) {
        return E_COMMUNICATE_FAILURE;
    }
    memset(rx_send_empty_buf, 0x00, data_len);
    #endif
    
    gpio_cs_sel();
    if (delay != 0) {
        delay_us(delay);
    } else {
        delay_us(10);
    }
    #if ENC_SPI_MODE > 0
    ret = TRUE;
    /* loop模式 */
    for (i = 0; i < data_len; i++) {
        read_byte = PORT_SpiReadByte(ENC_SPI);
        if (read_byte == (-1)) {
            ret = FALSE;
            break;
        } else {
            pdata[i] = (INT8U)read_byte;
        }
    }
    #else
	/* 中断模式 */
    /* 发送空内容，MCU为主模式，要产生时钟，才能让从机传输数据 */
    PORT_SpiRecvBufReset(ENC_SPI);
    if (PORT_SpiSendData(ENC_SPI, rx_send_empty_buf, data_len)) {
        delay_us(20);
        ret = spi_ReadBytes(ENC_SPI, pdata, data_len);
    } else {
        ret = FALSE;
    }
    #endif
    delay_us(10);
    gpio_cs_desel();
    delay_us(20);
    if (ret == FALSE) {
        spi_reset();  
        return E_COMMUNICATE_FAILURE;
    }
    return E_SUCCESS;
}

/*****************************************************************************
**  函数名:  Enc_Wakeup
**  函数描述: 唤醒加密芯片(调试时用，因为用到延时函数占用系统时序)
**  参数:       [in]void  :
**  返 回 : 
*****************************************************************************/
void Enc_Wakeup(void)
{
    #if 1
    PORT_GpioOutLevel(ENC_WK_PORT, ENC_WK_PIN, TRUE);
    delay_ms(12);
    PORT_GpioOutLevel(ENC_WK_PORT, ENC_WK_PIN, FALSE);
    delay_ms(12);
    #endif
}

/*****************************************************************************
**  函数名:  Enc_WkPinCtl
**  函数描述: 控制唤醒脚
**  参数:    [in] ctl : TRUE:拉高，FALSE:拉低
**  返回:    无
*****************************************************************************/
void Enc_WkPinCtl(BOOLEAN ctl)
{
    if (ctl == TRUE) {
        PORT_GpioOutLevel(ENC_WK_PORT, ENC_WK_PIN, TRUE);
    } else {
        PORT_GpioOutLevel(ENC_WK_PORT, ENC_WK_PIN, FALSE);
    }
}

/*****************************************************************************
**  函数名:  Enc_PinEnOrDis
**  函数描述: 设置为开漏状态/正常状态
**  参数:    [in] ctl : TRUE:恢复，FALSE:关闭
**  返回:    无
*****************************************************************************/
void Enc_PinEnOrDis(BOOLEAN ctl)
{
    PORT_GPIO_CFG_T cfg;
    cfg.speed = GPIO_SPEED_FAST;
   
    if (ctl == FALSE) {
        cfg.port  = ENC_WK_PORT;
        cfg.pin   = ENC_WK_PIN;
        cfg.mode  = PORT_GPIO_Mode_AN_IN;
        PORT_GpioCfg(cfg);
        
        cfg.port  = ENC_SPI_PIN_PORT;
        cfg.pin   = ENC_SPI_PIN_CS;
        PORT_GpioCfg(cfg);
        cfg.port  = ENC_SPI_PIN_PORT;
        cfg.pin   = ENC_SPI_PIN_SCK;
        PORT_GpioCfg(cfg);
        cfg.port  = ENC_SPI_PIN_PORT;
        cfg.pin   = ENC_SPI_PIN_MISO;
        PORT_GpioCfg(cfg);
        cfg.port  = ENC_SPI_PIN_PORT;
        cfg.pin   = ENC_SPI_PIN_MOSI;
        PORT_GpioCfg(cfg);
        /* 关闭加密电源 */
        PORT_GpioOutLevel(ENC_PW_PORT, ENC_PW_PIN, FALSE);
	
    } else {
        cfg.port  = ENC_WK_PORT;
        cfg.pin   = ENC_WK_PIN;	
		cfg.speed = GPIO_SPEED_LOW;
        cfg.mode  = PORT_GPIO_Mode_Out_PP;
        PORT_GpioCfg(cfg);
		
        cfg.speed = GPIO_SPEED_FAST;
        cfg.port  = ENC_SPI_PIN_PORT;
        cfg.pin   = ENC_SPI_PIN_CS;
        PORT_GpioCfg(cfg);
        
        cfg.mode  = PORT_GPIO_Mode_AF_PP;
        cfg.port  = ENC_SPI_PIN_PORT;
        cfg.pin   = ENC_SPI_PIN_SCK;
        PORT_GpioCfg(cfg);
        
        cfg.port  = ENC_SPI_PIN_PORT;
        cfg.pin   = ENC_SPI_PIN_MOSI;
        PORT_GpioCfg(cfg);		

		cfg.mode  = PORT_GPIO_Mode_IN_FLOATING;
		cfg.port  = ENC_SPI_PIN_PORT;
        cfg.pin   = ENC_SPI_PIN_MISO;
        PORT_GpioCfg(cfg);
        
        /* 打开加密电源 */
        PORT_GpioOutLevel(ENC_PW_PORT, ENC_PW_PIN, TRUE);	
    }
}


/*****************************************************************************
**  函数名:  Enc_PinInit
**  函数描述: 加密芯片引脚初始化
**  参数:    无
**  返回:    无
*****************************************************************************/
void Enc_PinInit(void)
{
    PORT_GPIO_CFG_T cfg;
    cfg.speed = GPIO_SPEED_FAST;
    
    cfg.mode  = PORT_GPIO_Mode_Out_PP;
    cfg.port  = ENC_PW_PORT;
    cfg.pin   = ENC_PW_PIN;
    PORT_GpioCfg(cfg);
    
    cfg.mode  = PORT_GPIO_Mode_Out_PP;
	cfg.speed = GPIO_SPEED_LOW;
    cfg.port  = ENC_WK_PORT;
    cfg.pin   = ENC_WK_PIN;
    PORT_GpioCfg(cfg);
    
    /* 未使用，设置为模拟输入 */
    cfg.speed = GPIO_SPEED_LOW;
    cfg.mode  = PORT_GPIO_Mode_AN_IN;
    cfg.port  = ENC_INT_PORT;
    cfg.pin   = ENC_INT_PIN;
    PORT_GpioCfg(cfg);	

    #if 0
    /* 上电 */
    PORT_GpioOutLevel(ENC_PW_PORT, ENC_PW_PIN, FALSE);
    delay_ms(5);
    PORT_GpioOutLevel(ENC_PW_PORT, ENC_PW_PIN, TRUE);
    delay_ms(10);
    #endif
}

