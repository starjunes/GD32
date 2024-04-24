/********************************************************************************
**
** 文件名:     port_gpio.c
** 版权所有:   (c) 1998-2017 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现底层与gpio操作相关接口的封装
**
*********************************************************************************
**                  修改历史记录
**===============================================================================
**| 日期              | 作者        |  修改记录
**===============================================================================
**| 2017/05/12 | 黄运峰    |  创建该文件
********************************************************************************/
#include  "dal_include.h"
#include  "dal_gpio.h"
#include  "dal_exti.h"
#include  "port_gpio.h"
#include  "dal_gsen_drv.h"
#include  "os_reg.h"


typedef struct {
    PORT_ID_E  port_id;         // 端口通道ID
    INT16U     pin_pos;         // 在某个通道中的管脚序号
    PIN_DIR_E  pin_dir;         // 管脚输入输出方向
    INT8U      def_value;       // 管脚默认输出电平,对于输入管脚无作用
    BOOLEAN    int_en;          // 对于输入管脚是否允许中断
    PORT_INTMODE_E int_mode;    // 管脚中断模式，若未使能中断，则该字段填0
    PORT_ISR_PTR handler;       // 端口输入中断处理函数指针
} PIOCFG_T;

#ifdef HAL_PIO_DEF
#undef HAL_PIO_DEF
#endif
#define HAL_PIO_DEF(_PIO_ID_, _PORT_ID_, _PIN_POS_, _PIO_DIR_, _DEF_VALUE_, _INT_EN_, _INT_MODE_, _HANDLER_)  \
                             {_PORT_ID_, _PIN_POS_, _PIO_DIR_, _DEF_VALUE_, _INT_EN_, _INT_MODE_, _HANDLER_},
static const PIOCFG_T s_piocfg[] = {
    #include "hal_pio_drv.def"
};

/*
********************************************************************************
* 定义配置参数
********************************************************************************
*/
static INT32U gpio_port_tbl[MAX_PORT_N] = {GPIOA, GPIOB, GPIOC, GPIOD, GPIOE};
static const INT16U  gpio_pin_tbl[MAX_GPIO_PIN] = {
    GPIO_PIN_0,
    GPIO_PIN_1,
    GPIO_PIN_2,
    GPIO_PIN_3,
    GPIO_PIN_4,
    GPIO_PIN_5,
    GPIO_PIN_6,
    GPIO_PIN_7,
    GPIO_PIN_8,
    GPIO_PIN_9,
    GPIO_PIN_10,
    GPIO_PIN_11,
    GPIO_PIN_12,
    GPIO_PIN_13,
    GPIO_PIN_14,
    GPIO_PIN_15,	
};

static INT16U gpio_line_stat = 0;

typedef struct {
    PORT_GPIO_CFG_T sleep_cfg;
    PORT_GPIO_CFG_T wk_cfg;
} GPIO_LP_T;

static const GPIO_LP_T s_gpio_lp_tb[] = {
    /* 输入 */
    {{PORT_GPIO_Mode_AN_IN, PORT_D, GPIO_PIN12, GPIO_SPEED_LOW}, {PORT_GPIO_Mode_IN_FLOATING, PORT_D, GPIO_PIN12, GPIO_SPEED_FAST}},/* [IN] PIO_CHGSTAT */ 
    //{{PORT_GPIO_Mode_AN_IN, PORT_D, GPIO_PIN10, GPIO_SPEED_LOW}, {PORT_GPIO_Mode_IN_FLOATING, PORT_D, GPIO_PIN10, GPIO_SPEED_FAST}},/* [IN] PIO_RTCWK */ 

    #if 1
    {{PORT_GPIO_Mode_AN_IN, PORT_E, GPIO_PIN8 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_IN_FLOATING, PORT_E, GPIO_PIN8 , GPIO_SPEED_FAST}},/* [IN] PIO_ECALL */ 
    {{PORT_GPIO_Mode_AN_IN, PORT_E, GPIO_PIN9 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_IN_FLOATING, PORT_E, GPIO_PIN9 , GPIO_SPEED_FAST}},/* [IN] PIO_BCALL */ 
    {{PORT_GPIO_Mode_AN_IN, PORT_D, GPIO_PIN3 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_IN_FLOATING, PORT_D, GPIO_PIN3 , GPIO_SPEED_FAST}},/* [IN] PIO_CAN0RXWK */ 
    {{PORT_GPIO_Mode_AN_IN, PORT_B, GPIO_PIN4 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_IN_FLOATING, PORT_B, GPIO_PIN4 , GPIO_SPEED_FAST}},/* [IN] PIO_CAN1RXWK */ 
    {{PORT_GPIO_Mode_AN_IN, PORT_B, GPIO_PIN9 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_IN_FLOATING, PORT_B, GPIO_PIN9 , GPIO_SPEED_FAST}},/* [IN] PIO_CAN2RXWK */ 
    {{PORT_GPIO_Mode_AN_IN, PORT_B, GPIO_PIN12, GPIO_SPEED_LOW}, {PORT_GPIO_Mode_IN_FLOATING, PORT_B, GPIO_PIN12, GPIO_SPEED_FAST}},/* [IN] PIO_DOWNLOAD */ 
    {{PORT_GPIO_Mode_AN_IN, PORT_E, GPIO_PIN14, GPIO_SPEED_LOW}, {PORT_GPIO_Mode_IN_FLOATING, PORT_E, GPIO_PIN14, GPIO_SPEED_FAST}},/* [IN] PIO_GPSSHORT */ 
    {{PORT_GPIO_Mode_AN_IN, PORT_E, GPIO_PIN13, GPIO_SPEED_LOW}, {PORT_GPIO_Mode_IN_FLOATING, PORT_E, GPIO_PIN13, GPIO_SPEED_FAST}},/* [IN] PIO_GPSOPEN */ 
    //{{PORT_GPIO_Mode_AN_IN, PORT_E, GPIO_PIN7 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_IN_FLOATING, PORT_E, GPIO_PIN7 , GPIO_SPEED_FAST}},/* [IN] PIO_CHGDET */ 
    {{PORT_GPIO_Mode_AN_IN, PORT_B, GPIO_PIN1 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_IN_FLOATING, PORT_B, GPIO_PIN1 , GPIO_SPEED_FAST}},/* [IN] PIO_AIRBAG */ 
    #endif
    
    /* 输出 */
    {{PORT_GPIO_Mode_AN_IN, PORT_B, GPIO_PIN13, GPIO_SPEED_LOW}, {PORT_GPIO_Mode_Out_PP, PORT_B, GPIO_PIN13, GPIO_SPEED_FAST}},/* [OUT] PIO_VBUSCTL */  
    {{PORT_GPIO_Mode_AN_IN, PORT_C, GPIO_PIN3 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_Out_PP, PORT_C, GPIO_PIN3 , GPIO_SPEED_FAST}},/* [OUT] PIO_CAN0STB */
    {{PORT_GPIO_Mode_AN_IN, PORT_C, GPIO_PIN3 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_Out_PP, PORT_C, GPIO_PIN3 , GPIO_SPEED_FAST}},/* [OUT] PIO_CAN1STB */
    {{PORT_GPIO_Mode_AN_IN, PORT_C, GPIO_PIN3 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_Out_PP, PORT_C, GPIO_PIN3 , GPIO_SPEED_FAST}},/* [OUT] PIO_CAN2STB */ 
    //{{PORT_GPIO_Mode_AN_IN, PORT_E, GPIO_PIN12, GPIO_SPEED_LOW}, {PORT_GPIO_Mode_Out_PP, PORT_E, GPIO_PIN12, GPIO_SPEED_FAST}},/* [OUT] PIO_GPSPWR */
    {{PORT_GPIO_Mode_AN_IN, PORT_C, GPIO_PIN0 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_Out_PP, PORT_C, GPIO_PIN0 , GPIO_SPEED_FAST}},/* [OUT] PIO_FDWDG */
    {{PORT_GPIO_Mode_AN_IN, PORT_E, GPIO_PIN11, GPIO_SPEED_LOW}, {PORT_GPIO_Mode_Out_PP, PORT_E, GPIO_PIN11, GPIO_SPEED_FAST}},/* [OUT] PIO_HSMWK */
    //{{PORT_GPIO_Mode_AN_IN, PORT_D, GPIO_PIN13, GPIO_SPEED_LOW}, {PORT_GPIO_Mode_Out_PP, PORT_D, GPIO_PIN13, GPIO_SPEED_FAST}},/* [OUT] PIO_CHGEN */
    {{PORT_GPIO_Mode_AN_IN, PORT_B, GPIO_PIN2 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_Out_PP, PORT_B, GPIO_PIN2 , GPIO_SPEED_FAST}},/* [OUT] PIO_5VEXT */
    {{PORT_GPIO_Mode_AN_IN, PORT_B, GPIO_PIN14, GPIO_SPEED_LOW}, {PORT_GPIO_Mode_Out_PP, PORT_B, GPIO_PIN14, GPIO_SPEED_FAST}},/* [OUT] PIO_CCVEN */
    //{{PORT_GPIO_Mode_AN_IN, PORT_E, GPIO_PIN15, GPIO_SPEED_LOW}, {PORT_GPIO_Mode_Out_PP, PORT_E, GPIO_PIN15, GPIO_SPEED_FAST}},/* [OUT] PIO_GPSBAT */
    {{PORT_GPIO_Mode_AN_IN, PORT_C, GPIO_PIN6 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_Out_PP, PORT_C, GPIO_PIN6 , GPIO_SPEED_FAST}},/* [OUT] PIO_EXT1V8 */
    {{PORT_GPIO_Mode_AN_IN, PORT_C, GPIO_PIN15, GPIO_SPEED_LOW}, {PORT_GPIO_Mode_Out_PP, PORT_C, GPIO_PIN15, GPIO_SPEED_FAST}},/* [OUT] PIO_EXT3V */
    {{PORT_GPIO_Mode_AN_IN, PORT_D, GPIO_PIN4 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_Out_PP, PORT_D, GPIO_PIN4 , GPIO_SPEED_FAST}},/* [OUT] PIO_GPSLED */
    {{PORT_GPIO_Mode_AN_IN, PORT_D, GPIO_PIN5 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_Out_PP, PORT_D, GPIO_PIN5 , GPIO_SPEED_FAST}},/* [OUT] PIO_CANLED */
    {{PORT_GPIO_Mode_AN_IN, PORT_B, GPIO_PIN15, GPIO_SPEED_LOW}, {PORT_GPIO_Mode_Out_PP, PORT_B, GPIO_PIN15, GPIO_SPEED_FAST}},/* [OUT] PIO_GSMPU */
    {{PORT_GPIO_Mode_AN_IN, PORT_B, GPIO_PIN10, GPIO_SPEED_LOW}, {PORT_GPIO_Mode_Out_PP, PORT_B, GPIO_PIN10, GPIO_SPEED_FAST}},/* [OUT] PIO_HSMPWR */
    #if 0
    {{PORT_GPIO_Mode_AN_IN, PORT_C, GPIO_PIN5 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_Out_PP, PORT_C, GPIO_PIN5 , GPIO_SPEED_FAST}},/* [OUT] PIO_232_CON */
    {{PORT_GPIO_Mode_AN_IN, PORT_A, GPIO_PIN12, GPIO_SPEED_LOW}, {PORT_GPIO_Mode_Out_PP, PORT_A, GPIO_PIN12, GPIO_SPEED_FAST}},/* [OUT] PIO_WAKEUP_EC20 */
    {{PORT_GPIO_Mode_AN_IN, PORT_B, GPIO_PIN0 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_Out_PP, PORT_B, GPIO_PIN0 , GPIO_SPEED_FAST}},/* [OUT] PIO_MUTE_CON */
    {{PORT_GPIO_Mode_IPD, PORT_E, GPIO_PIN10, GPIO_SPEED_LOW}, {PORT_GPIO_Mode_IPD, PORT_E, GPIO_PIN10, GPIO_SPEED_FAST}},/* [OUT] PIO_ECALL_IL_CON */
    #endif
    /* 串口 */
    {{PORT_GPIO_Mode_AN_IN, PORT_A, GPIO_PIN0 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_IN_FLOATING , 	PORT_A, GPIO_PIN0 , GPIO_SPEED_FAST}},/* [NA] U2_CTS_MCU(未使用) */ 
    {{PORT_GPIO_Mode_AN_IN, PORT_A, GPIO_PIN1 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_AF_PP , 		PORT_A, GPIO_PIN1 , GPIO_SPEED_FAST}},/* [NA] U2_RTS_MCU(未使用) */ 
    {{PORT_GPIO_Mode_AN_IN, PORT_A, GPIO_PIN2 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_AF_PP ,		PORT_A, GPIO_PIN2 , GPIO_SPEED_FAST}},/* [FA] USART2_TX */
    {{PORT_GPIO_Mode_AN_IN, PORT_A, GPIO_PIN3 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_IN_FLOATING , 	PORT_A, GPIO_PIN3 , GPIO_SPEED_FAST}},/* [FA] USART2_RX */
    {{PORT_GPIO_Mode_AN_IN, PORT_D, GPIO_PIN8 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_AF_PP , 		PORT_D, GPIO_PIN8 , GPIO_SPEED_FAST}},/* [FA] USART3_TX */
    {{PORT_GPIO_Mode_AN_IN, PORT_D, GPIO_PIN9 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_IN_FLOATING , 	PORT_D, GPIO_PIN9 , GPIO_SPEED_FAST}},/* [FA] USART3_RX */
    
    /* 加密芯片 */
    {{PORT_GPIO_Mode_AN_IN, PORT_C, GPIO_PIN4 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_AN_IN , PORT_C, GPIO_PIN4 , GPIO_SPEED_FAST}},/* [NA]  HSM2_INT(未使用) */ 
    {{PORT_GPIO_Mode_AN_IN, PORT_C, GPIO_PIN13, GPIO_SPEED_LOW}, {PORT_GPIO_Mode_Out_PP, PORT_C, GPIO_PIN13, GPIO_SPEED_FAST}},/* [OUT] WAKE_HSM2 */
    {{PORT_GPIO_Mode_AN_IN, PORT_A, GPIO_PIN4 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_Out_PP, PORT_A, GPIO_PIN4 , GPIO_SPEED_FAST}},/* [OUT] SPI1_CS_HSM2 */ 
    {{PORT_GPIO_Mode_AN_IN, PORT_A, GPIO_PIN5 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_AF_PP , PORT_A, GPIO_PIN5 , GPIO_SPEED_FAST}},/* [FA]  SPI1_CLK_HSM2 */
    {{PORT_GPIO_Mode_AN_IN, PORT_A, GPIO_PIN6 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_IN_FLOATING , PORT_A, GPIO_PIN6 , GPIO_SPEED_FAST}},/* [FA]  SPI1_MISO_HSM2 */
    {{PORT_GPIO_Mode_AN_IN, PORT_A, GPIO_PIN7 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_AF_PP , PORT_A, GPIO_PIN7 , GPIO_SPEED_FAST}},/* [FA]  SPI1_MOSI_HSM2 */
    
    /* CAN芯片 */
    #if 1
    {{PORT_GPIO_Mode_AN_IN, PORT_B, GPIO_PIN7 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_AN_IN , PORT_B, GPIO_PIN7 , GPIO_SPEED_FAST}},/* [OUT] 215_RST(未使用) */ 
    {{PORT_GPIO_Mode_IPD  , PORT_E, GPIO_PIN3 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_IPD   , PORT_E, GPIO_PIN3 , GPIO_SPEED_FAST}},/* [IN]  215_INT(未使用) */
    {{PORT_GPIO_Mode_IPD  , PORT_E, GPIO_PIN4 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_IPD   , PORT_E, GPIO_PIN4 , GPIO_SPEED_FAST}},/* [NA]  SPI4_CS_215(未使用) */ 
    {{PORT_GPIO_Mode_IPD  , PORT_E, GPIO_PIN2 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_IPD   , PORT_E, GPIO_PIN2 , GPIO_SPEED_FAST}},/* [NA]  SPI4_SLK_215(未使用) */
    {{PORT_GPIO_Mode_IPD  , PORT_E, GPIO_PIN5 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_IN_FLOATING   , PORT_E, GPIO_PIN6 , GPIO_SPEED_FAST}},/* [NA]  SPI4_MISO_215(未使用) */
    {{PORT_GPIO_Mode_IPD  , PORT_E, GPIO_PIN6 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_IPD   , PORT_E, GPIO_PIN6 , GPIO_SPEED_FAST}},/* [NA]  SPI4_MOSI_215(未使用) */   
    #endif
    
    /* 六轴 */
    {{PORT_GPIO_Mode_AN_IN, PORT_A, GPIO_PIN9 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_IN_FLOATING, PORT_A, GPIO_PIN9 , GPIO_SPEED_FAST}},/* [IN]  PIO_GYRINT */
    {{PORT_GPIO_Mode_AN_IN, PORT_A, GPIO_PIN8 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_Out_PP     , PORT_A, GPIO_PIN8 , GPIO_SPEED_FAST}},/* [OUT] I2C3_SCL_6X */
    {{PORT_GPIO_Mode_AN_IN, PORT_C, GPIO_PIN9 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_Out_OD     , PORT_C, GPIO_PIN9 , GPIO_SPEED_FAST}},/* [OD]  I2C3_SDA_6X */

    /* mcu内部can通道 */
    {{PORT_GPIO_Mode_AN_IN, PORT_D, GPIO_PIN1 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_AF_PP , PORT_D, GPIO_PIN1 , GPIO_SPEED_FAST}},/* [FA] CAN1_TX */
    {{PORT_GPIO_Mode_AN_IN, PORT_D, GPIO_PIN0 , GPIO_SPEED_LOW}, {GPIO_MODE_IPU , PORT_D, GPIO_PIN0 , GPIO_SPEED_FAST}},/* [FA] CAN1_RX */
    {{PORT_GPIO_Mode_AN_IN, PORT_B, GPIO_PIN6 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_AF_PP , PORT_B, GPIO_PIN6 , GPIO_SPEED_FAST}},/* [FA] CAN2_TX */
    {{PORT_GPIO_Mode_AN_IN, PORT_B, GPIO_PIN5 , GPIO_SPEED_LOW}, {GPIO_MODE_IPU , PORT_B, GPIO_PIN5 , GPIO_SPEED_FAST}},/* [FA] CAN2_RX */
    {{PORT_GPIO_Mode_IPD  , PORT_E, GPIO_PIN1 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_IPD   , PORT_E, GPIO_PIN1 , GPIO_SPEED_FAST}},/* [NA] CAN3_TX(未使用) */
    {{PORT_GPIO_Mode_IPD  , PORT_E, GPIO_PIN0 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_IPD   , PORT_E, GPIO_PIN0 , GPIO_SPEED_FAST}},/* [NA] CAN3_RX(未使用) */    
    
    /* flash芯片 */
    //{{PORT_GPIO_Mode_AN_IN, PORT_A, GPIO_PIN15, GPIO_SPEED_LOW}, {PORT_GPIO_Mode_Out_PP      , PORT_A, GPIO_PIN15, GPIO_SPEED_FAST}},/* [OUT] SPI3_CS_NOR */ 
    {{PORT_GPIO_Mode_AN_IN, PORT_C, GPIO_PIN10, GPIO_SPEED_LOW}, {PORT_GPIO_Mode_Out_PP      , PORT_C, GPIO_PIN10, GPIO_SPEED_FAST}},/* [FA]  SPI3_CLK_NOR */
    {{PORT_GPIO_Mode_AN_IN, PORT_C, GPIO_PIN11, GPIO_SPEED_LOW}, {PORT_GPIO_Mode_IN_FLOATING , PORT_C, GPIO_PIN11, GPIO_SPEED_FAST}},/* [FA]  SPI3_MISO_NOR */
    {{PORT_GPIO_Mode_AN_IN, PORT_C, GPIO_PIN12, GPIO_SPEED_LOW}, {PORT_GPIO_Mode_Out_PP      , PORT_C, GPIO_PIN12, GPIO_SPEED_FAST}},/* [FA]  SPI3_MOSI_NOR */ 
    
    #if 1
    /* 悬空引脚 */
    {{PORT_GPIO_Mode_IPD, PORT_B, GPIO_PIN3 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_IPD , PORT_B, GPIO_PIN3 , GPIO_SPEED_FAST}},/* [NA] 悬空 */
    {{PORT_GPIO_Mode_IPD, PORT_D, GPIO_PIN7 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_IPD , PORT_D, GPIO_PIN7 , GPIO_SPEED_FAST}},/* [NA] 悬空 */
    {{PORT_GPIO_Mode_IPD, PORT_D, GPIO_PIN6 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_IPD , PORT_D, GPIO_PIN6 , GPIO_SPEED_FAST}},/* [NA] 悬空 */
    {{PORT_GPIO_Mode_IPD, PORT_C, GPIO_PIN8 , GPIO_SPEED_LOW}, {PORT_GPIO_Mode_IPD , PORT_C, GPIO_PIN8 , GPIO_SPEED_FAST}},/* [NA] 悬空 */
    {{PORT_GPIO_Mode_IPD, PORT_D, GPIO_PIN11, GPIO_SPEED_LOW}, {PORT_GPIO_Mode_IPD , PORT_D, GPIO_PIN11, GPIO_SPEED_FAST}},/* [NA] 悬空 */
    #endif
};

/**************************************************************************************************
**  函数名称:  hal_sys_WakeupByPio
**  功能描述:  外部中断处理函数
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void  hal_sys_WakeupByPio(void) __irq
{
	INT32U cnt = 0;
    if (exti_interrupt_flag_get(EXTI_2) == SET) {
        exti_interrupt_flag_clear(EXTI_2);
        OS_PostMsg(0, MSG_OPT_OTWAKE_ENENT, 0, 0);
        #if 0//EN_DEBUG > 1
        Debug_SysPrint("主电源中断:line2\r\n");
        #endif
		cnt = 0xFFF;
		while(cnt--);
		if(PORT_GetGpioPinState(PIN_PWRDECT) == FALSE){
	        gpio_line_stat |= EXTI_2;
	        PORT_SetGpioPin(PIN_BATCTL);
		}
    }
	if (exti_interrupt_flag_get(EXTI_5) == SET) {
        exti_interrupt_flag_clear(EXTI_5);
        #if 0//EN_DEBUG > 1
        Debug_SysPrint("备用电池电量低中断:line5\r\n");
        #endif
    }
    if (exti_interrupt_flag_get(EXTI_8) == SET) {
        exti_interrupt_flag_clear(EXTI_8);
        OS_PostMsg(0, MSG_OPT_OTWAKE_ENENT, 0, 0);
        #if 0//EN_DEBUG > 1
        Debug_SysPrint("ACC中断:line8\r\n");
        #endif
        gpio_line_stat |= EXTI_8;
    }
	if (exti_interrupt_flag_get(EXTI_9) == SET) {
		exti_interrupt_flag_clear(EXTI_9);
		
		GsenIntCalbakFunc(); //PRIVATE_FUNC
        OS_PostMsg(0, MSG_OPT_OTWAKE_ENENT, 0, 0);
        #if 0//EN_DEBUG > 1
        Debug_SysPrint("六轴中断(PA9)\r\n");
        #endif
        gpio_line_stat |= EXTI_9;
	}

	if (exti_interrupt_flag_get(EXTI_10) == SET) {
		exti_interrupt_flag_clear(EXTI_10);
		
		HAL_sd2058_ClearAlarm();
        OS_PostMsg(0, MSG_OPT_OTWAKE_ENENT, 0, 0);
        #if 0//EN_DEBUG > 1
        debug_printf("RTC中断(P10)\r\n");
        #endif
        gpio_line_stat |= EXTI_10;
	}

	if (exti_interrupt_flag_get(EXTI_11) == SET) {
		exti_interrupt_flag_clear(EXTI_11);
        OS_PostMsg(0, MSG_OPT_OTWAKE_ENENT, 0, 0);
        #if 0//EN_DEBUG > 1
        Debug_SysPrint("<**** Mcu wakeup 中断 ****>\r\n");
        #endif
        gpio_line_stat |= EXTI_11;
	}    
}

/********************************************************************************
** 函数名:     PORT_GpioConfigure
** 函数描述:   GPIO配置接口
** 参数:       [in] pin:    管脚编号，见GPIO_ID_E
**			   [in]	dir:	GPIO方向，见GPIO_DIR_E
**			   [in]	level:	管脚初始化电平，1-高电平，0-低电平
** 返回:       无
********************************************************************************/
void PORT_GpioConfigure(INT32U pin, INT32U dir, INT8U level)
{
    INT32U gpioPort,pnum;
    
    switch(s_piocfg[pin].port_id) {
        case PORT_A:
            gpioPort = GPIOA;
            break;
        case PORT_B:
            gpioPort = GPIOB;
            break;
        case PORT_C:
            gpioPort = GPIOC;
            break;
        case PORT_D:
            gpioPort = GPIOD;
            break;
        case PORT_E:
            gpioPort = GPIOE;
            break;
        default:
            return;
    }
	pnum = (1 << s_piocfg[pin].pin_pos);
    if (dir == GPIO_DIR_IN) {
        CreateInputPort(gpioPort, pnum, GPIO_MODE_IN_FLOATING, false);
    } else {
    	if(GPIOC == gpioPort){
			if((GPIO_PIN_14 == pnum)||(GPIO_PIN_15 == pnum)||(GPIO_PIN_13 == pnum)){
				CreateOutputPort(gpioPort, pnum, GPIO_MODE_OUT_PP, GPIO_OSPEED_2MHZ, false);
				WriteOutputPort(gpioPort, pnum, level);
				return;
			}
    	}
    	CreateOutputPort(gpioPort, pnum, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, false);
        WriteOutputPort(gpioPort, pnum, level);
		return;
    }
}

/********************************************************************************
** 函数名:     PORT_GetGpioPinState
** 函数描述:   读取GPIO引脚状态，输入输出皆可读取
** 参数:       [in] pin: 管脚编号，见GPIO_ID_E
** 返回:       1:高电平; 0:低电平
********************************************************************************/
BOOLEAN PORT_GetGpioPinState(INT32U pin)
{
    INT32U gpioPort;
    
    switch(s_piocfg[pin].port_id) {
        case PORT_A:
            gpioPort = GPIOA;
            break;
        case PORT_B:
            gpioPort = GPIOB;
            break;
        case PORT_C:
            gpioPort = GPIOC;
            break;
        case PORT_D:
            gpioPort = GPIOD;
            break;
        case PORT_E:
            gpioPort = GPIOE;
            break;
        default:
            return 0;
    }
    if (s_piocfg[pin].pin_dir == DIR_INPUT) {            //输入
        return ReadInputPort(gpioPort, 1 << s_piocfg[pin].pin_pos);
    } else {
        return ReadOutputPort(gpioPort, 1 << s_piocfg[pin].pin_pos);
    }
}

/********************************************************************************
** 函数名:     PORT_SetGpioPin
** 函数描述:   置位GPIO引脚，仅用于置位输出引脚且GPIO已使能
** 参数:       [in] pin: 管脚编号，见GPIO_ID_E
** 返回:       无
********************************************************************************/
void PORT_SetGpioPin(INT32U pin)
{
    INT32U gpioPort;
    
    switch(s_piocfg[pin].port_id) {
        case PORT_A:
            gpioPort = GPIOA;
            break;
        case PORT_B:
            gpioPort = GPIOB;
            break;
        case PORT_C:
            gpioPort = GPIOC;
            break;
        case PORT_D:
            gpioPort = GPIOD;
            break;
        case PORT_E:
            gpioPort = GPIOE;
            break;
        default:
            return;
    }
    WriteOutputPort(gpioPort, 1 << s_piocfg[pin].pin_pos, 1);
}
/********************************************************************************
** 函数名:     PORT_ClearGpioPin
** 函数描述:   清零GPIO引脚，仅用于输出引脚且GPIO已使能
** 参数:       [in] pin: 管脚编号，见GPIO_ID_E
** 返回:       无
********************************************************************************/
void PORT_ClearGpioPin(INT32U pin)
{
    INT32U gpioPort;
    
    switch(s_piocfg[pin].port_id) {
        case PORT_A:
            gpioPort = GPIOA;
            break;
        case PORT_B:
            gpioPort = GPIOB;
            break;
        case PORT_C:
            gpioPort = GPIOC;
            break;
        case PORT_D:
            gpioPort = GPIOD;
            break;
        case PORT_E:
            gpioPort = GPIOE;
            break;
        default:
            return;
    }
    WriteOutputPort(gpioPort, 1 << s_piocfg[pin].pin_pos, 0);
}

/********************************************************************************
** 函数名:     PORT_PinToggle
** 函数描述:   切换指定IO管脚电平
** 参数:       [in] pin: 管脚编号，见GPIO_ID_E
** 返回:       无
********************************************************************************/
void PORT_PinToggle(INT32U pin)
{
    //暂未用到
	//hal_pio_PinToggle(pin);
}

/********************************************************************************
** 函数名:   PORT_SetPinIntMode
** 函数描述: 设置端口中断使能
** 参数:     [in] pid： 端口编号
**           [in] enable：使能选项
** 返回:     设置成功返回TRUE，设置失败返回FALSE.
********************************************************************************/
BOOLEAN PORT_SetPinIntMode(INT32U pin, BOOLEAN enable)
{
	if ((s_piocfg[pin].pin_dir == DIR_INPUT) && (s_piocfg[pin].int_en)) {
	    if (enable) {
            SetExtIntLine((PORT_GRP_E)s_piocfg[pin].port_id, (PORT_IDX_E)s_piocfg[pin].pin_pos, (TRIG_MODE_E)s_piocfg[pin].int_mode);
        } else {
            ClearExtIntLine((PORT_GRP_E)s_piocfg[pin].port_id, (PORT_IDX_E)s_piocfg[pin].pin_pos, (TRIG_MODE_E)s_piocfg[pin].int_mode);
        }
        return true;
    } else {
        return false;
    }
}

/********************************************************************************
**  函数名称:  PORT_PinGetAirbagCount
**  功能描述:  获取脉冲计数个数
**  输入参数:  无
**  返回参数:  返回计数值（当达到65535后翻转）
********************************************************************************/
INT16U PORT_PinGetAirbagCount(void)
{
	//return hal_pulse_GetCount();
	return 0;
}

/********************************************************************************
**  函数名称:  PORT_PinGetAirbagFreq
**  功能描述:  获取安全气囊脉冲频率
**  输入参数:  无
**  返回参数:  返回VSS频率
********************************************************************************/
INT32U PORT_PinGetAirbagFreq(void)
{
	//return hal_pulse_GetFreq();
	return 0;
}

/********************************************************************************
** 函数名:   PORT_GpioInit
** 函数描述: 管脚初始化
** 参数:     [in] pid： 端口编号
**           [in] enable：使能选项
** 返回:     设置成功返回TRUE，设置失败返回FALSE.
********************************************************************************/
void PORT_GpioInit(void)
{
    INT8U pin;
	rcu_periph_clock_enable(RCU_PMU);
	rcu_periph_clock_enable(RCU_AF);
    pmu_backup_write_enable();
	rcu_osci_off(RCU_LXTAL);
	pmu_backup_write_disable();
    for (pin = 0; pin < MAX_GPIO_ID; pin++) {
        if ((s_piocfg[pin].pin_dir == DIR_INPUT) && (s_piocfg[pin].int_en)) {
            CreateExtIntLine(s_piocfg[pin].port_id, s_piocfg[pin].pin_pos,s_piocfg[pin].int_mode, s_piocfg[pin].handler);
        } else {
            PORT_GpioConfigure(pin, s_piocfg[pin].pin_dir, s_piocfg[pin].def_value);
        }
    }
}

/*****************************************************************************
**  函数名:  PORT_GpioOutLevel
**  函数描述: gpio输出设置
**  参数:    [in] port  : 端口号, ref: GPIO_PORT_E
             [in] pin   : 引脚号, ref: GPIO_PIN_NUM_E
             [in] level : 电平, TRUE:高, FALSE: 低
**  返回:    无
*****************************************************************************/
void PORT_GpioOutLevel(PORT_ID_E port, GPIO_PIN_NUM_E pin, BOOLEAN level)
{
    WriteOutputPort(gpio_port_tbl[port], gpio_pin_tbl[pin], level);
}

/*****************************************************************************
**  函数名:  PORT_GpioCfg
**  函数描述: gpio设置
**  参数:    [in] cfg : gpio配置参数
**  返回:    无
*****************************************************************************/
void PORT_GpioCfg(PORT_GPIO_CFG_T cfg)
{
    DAL_GpioPortCfg(gpio_port_tbl[cfg.port], gpio_pin_tbl[cfg.pin], (INT32U)cfg.mode, (INT32U)cfg.speed, FALSE);
}

/*****************************************************************************
**  函数名:  PORT_GetIoInitSta
**  函数描述: io口中断状态
**  参数:    无
**  返回:    无
*****************************************************************************/
INT16U PORT_GetIoInitSta(void)
{
    INT16U temp;
    temp = gpio_line_stat;
    /* 读取完状态后清除中断标志 */
    gpio_line_stat = 0;
    return temp;
}

/*****************************************************************************
**  函数名:  PORT_ClearGpioIntSta
**  函数描述: 清除中断标志
**  参数:    无
**  返回:    无
*****************************************************************************/
void PORT_ClearGpioIntSta(void)
{
    exti_interrupt_flag_clear(EXTI_2);
    exti_interrupt_flag_clear(EXTI_8);
    exti_interrupt_flag_clear(EXTI_9);
    exti_interrupt_flag_clear(EXTI_11);
    gpio_line_stat = 0;
}

/*****************************************************************************
**  函数名:  PORT_GpioLowPower
**  函数描述: gpio低功耗(包括复用io)
**  参数:    [in] gpio_lp : TRUE : 低功耗模式, FALSE : 正常模式
**  返回:    无
*****************************************************************************/
void PORT_GpioLowPower(BOOLEAN gpio_lp)
{
    INT8U gpio_idx;
    INT8U gpio_num;
    
    gpio_num = sizeof(s_gpio_lp_tb)/sizeof(GPIO_LP_T);
    #if EN_DEBUG > 1
    debug_printf_dir("<**** lp gpio num:%d ****>\r\n",gpio_num);
    #endif
    if (gpio_lp) {       
        for (gpio_idx = 0; gpio_idx < gpio_num; gpio_idx++) {
            PORT_GpioCfg(s_gpio_lp_tb[gpio_idx].sleep_cfg);
        }
    } else {
        for (gpio_idx = 0; gpio_idx < gpio_num; gpio_idx++) {
            PORT_GpioCfg(s_gpio_lp_tb[gpio_idx].wk_cfg);
        }
    }
}

//------------------------------------------------------------------------------
/* End of File */

