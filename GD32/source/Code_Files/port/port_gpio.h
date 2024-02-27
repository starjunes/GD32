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
#ifndef PORT_GPIO_H
#define PORT_GPIO_H          1

#include  "man_irq.h"

typedef void (*PORT_ISR_PTR)(void) __irq;
/*
*********************************************************************************
*                   模块数据类型及宏定义
*********************************************************************************
*/
// 端口输入输出方向
typedef enum {    
    DIR_INPUT = 0,   // 输入模式
    DIR_OUTPUT       // 输出模式
} PIN_DIR_E;

// 端口工作模式定义
typedef enum {
    MODE_ANALOG = 0, // 模拟模式
    MODE_DIGITAL     // 数字模式
} PIN_MODE_E;

// 端口通道号定义
typedef enum {
    PORT_A = 0,
    PORT_B,
    PORT_C,
    PORT_D,
    PORT_E,
    MAX_PORT_N
} PORT_ID_E;

// 定义端口中断模式
typedef enum {
    INT_RISING_EDGE,    // 上升沿
    INT_FALLING_EDGE,   // 下降沿
    INT_EITHER_EDGE,    // 边沿触发(即上升沿和下降沿)
    INT_MODE_MAX
} PORT_INTMODE_E;

typedef enum {
	//----------------------------------------------------------
	//                  输入管脚定义(17)
	//----------------------------------------------------------
	PIN_CHGSTAT,
    PIN_RTCWK,
    PIN_GYRINT,
    PIN_MCUWK,
	PIN_ACCIN,
    PIN_ECALL,
	PIN_BCALL,
    PIN_PWRDECT,
    PIN_CAN0RXWK,
    PIN_CAN1RXWK,
    PIN_CAN2RXWK,
    PIN_DOWNLDIN,
    PIN_GPSSHORT,
	PIN_GPSOPEN,
	PIN_CHGDECT,
    PIN_AIRBAG, // EN_GZTEST
    PIN_ENCINT,
	PIN_BAT,	//备用电池电压检测
//----------------------------------------------------------
//                  输出管脚定义(31)
//----------------------------------------------------------
	PIN_VBUSCTL,
	PIN_CAN0STB,
    PIN_CAN1STB,
    PIN_CAN2STB,
    PIN_GPSPWR,
    PIN_FDWDG,
    //PIN_232CON,
    PIN_GSMPWR,
	PIN_HSMWK,
    PIN_CHGEN,
    PIN_GSM4V,
    PIN_5VEXT,
    PIN_DCEN,
    PIN_BATCTL,
    PIN_CCVEN,
    PIN_EC20WK,
	PIN_GPSBAT,
    PIN_GYRPWR,
    PIN_EXT1V8,
    PIN_EXT3V,
    PIN_GPSLED,
    PIN_GSMPU,
	PIN_HSMPWR,
	PIN_MUTECON,
    PIN_CANLED,
    PIN_ENCCS,
    PIN_ENCWAKEUP,
    PIN_PSCTRL,
    PIN_BATSHUT,
    PIN_215_RST,
    PIN_WIFILED,
    PIN_ECALL_IL_CON,

    MAX_GPIO_ID
} GPIO_ID_E;

// 端口输入输出方向
typedef enum {
    GPIO_DIR_IN = 0,   // 输入模式
    GPIO_DIR_OUT       // 输出模式
} GPIO_DIR_E;

typedef enum {
    GPIO_PIN0,
    GPIO_PIN1,
    GPIO_PIN2,
    GPIO_PIN3,
    GPIO_PIN4,   
    GPIO_PIN5,
    GPIO_PIN6,
    GPIO_PIN7,
    GPIO_PIN8,
    GPIO_PIN9, 
    GPIO_PIN10,
    GPIO_PIN11,
    GPIO_PIN12,
    GPIO_PIN13,
    GPIO_PIN14,   
    GPIO_PIN15,  
    MAX_GPIO_PIN
} GPIO_PIN_NUM_E;

typedef enum {
    PORT_GPIO_Mode_AN_IN 		= GPIO_MODE_AIN,
    PORT_GPIO_Mode_IN_FLOATING 	= GPIO_MODE_IN_FLOATING,
    PORT_GPIO_Mode_IPD 			= GPIO_MODE_IPD,
    PORT_GPIO_Mode_IPU 			= GPIO_MODE_IPU,
    PORT_GPIO_Mode_Out_OD 		= GPIO_MODE_OUT_OD,
    PORT_GPIO_Mode_Out_PP 		= GPIO_MODE_OUT_PP,
    PORT_GPIO_Mode_AF_OD 		= GPIO_MODE_AF_OD,
    PORT_GPIO_Mode_AF_PP 		= GPIO_MODE_AF_PP,
} PORT_GPIO_MODE_E;
typedef enum {
    GPIO_SPEED_LOW 	= GPIO_OSPEED_2MHZ,
    GPIO_SPEED_MID 	= GPIO_OSPEED_10MHZ,
    GPIO_SPEED_FAST = GPIO_OSPEED_50MHZ,
    GPIO_SPEED_HIGH = GPIO_OSPEED_MAX,    
} GPIO_SPEED_E;


/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    PORT_GPIO_MODE_E mode;
    PORT_ID_E        port;
    GPIO_PIN_NUM_E   pin;
    GPIO_SPEED_E     speed;
} PORT_GPIO_CFG_T;

/*
*********************************************************************************
*                   对外接口声明
*********************************************************************************
*/
void  hal_sys_WakeupByPio(void) __irq;
/********************************************************************************
** 函数名:     PORT_GpioConfigure
** 函数描述:   GPIO配置接口
** 参数:       [in] pin:    管脚编号，见GPIO_ID_E
**             [in] dir:    GPIO方向，见GPIO_DIR_E
**             [in] level:  管脚初始化电平，1-高电平，0-低电平
** 返回:       无
********************************************************************************/
void PORT_GpioConfigure(INT32U pin, INT32U dir, INT8U level);

/********************************************************************************
** 函数名:     PORT_GetGpioPinState
** 函数描述:   读取GPIO引脚状态，输入输出皆可读取
** 参数:       [in] pin: 管脚编号，见GPIO_ID_E
** 返回:       1:高电平; 0:低电平
********************************************************************************/
BOOLEAN PORT_GetGpioPinState(INT32U pin);

/********************************************************************************
** 函数名:     PORT_SetGpioPin
** 函数描述:   置位GPIO引脚，仅用于置位输出引脚且GPIO已使能
** 参数:       [in] pin: 管脚编号，见GPIO_ID_E
** 返回:       无
********************************************************************************/
void PORT_SetGpioPin(INT32U pin);

/********************************************************************************
** 函数名:     PORT_ClearGpioPin
** 函数描述:   清零GPIO引脚，仅用于输出引脚且GPIO已使能
** 参数:       [in] pin: 管脚编号，见GPIO_ID_E
** 返回:       无
********************************************************************************/
void PORT_ClearGpioPin(INT32U pin);

/********************************************************************************
** 函数名:     PORT_PinToggle
** 函数描述:   切换指定IO管脚电平
** 参数:       [in] pin: 管脚编号，见GPIO_ID_E
** 返回:       无
********************************************************************************/
void PORT_PinToggle(INT32U pin);

/********************************************************************************
** 函数名:   PORT_SetPinIntMode
** 函数描述: 设置端口中断使能
** 参数:     [in] pid： 端口编号
**           [in] enable：使能选项
** 返回:     设置成功返回TRUE，设置失败返回FALSE.
********************************************************************************/
BOOLEAN PORT_SetPinIntMode(INT32U pin, BOOLEAN enable);

/********************************************************************************
**  函数名称:  PORT_PinGetAirbagCount
**  功能描述:  获取脉冲计数个数
**  输入参数:  无
**  返回参数:  返回计数值（当达到65535后翻转）
********************************************************************************/
INT16U PORT_PinGetAirbagCount(void);

/********************************************************************************
**  函数名称:  PORT_PinGetAirbagFreq
**  功能描述:  获取安全气囊脉冲频率
**  输入参数:  无
**  返回参数:  返回VSS频率
********************************************************************************/
INT32U PORT_PinGetAirbagFreq(void);

void PORT_GpioInit(void);
	
/*****************************************************************************
**  函数名:  PORT_GpioOutLevel
**  函数描述: gpio输出设置
**  参数:    [in] port  : 端口号, ref: PORT_ID_E
             [in] pin   : 引脚号, ref: GPIO_PIN_NUM_E
             [in] level : 电平, TRUE:高, FALSE: 低
**  返回:    无
*****************************************************************************/
void PORT_GpioOutLevel(PORT_ID_E port, GPIO_PIN_NUM_E pin, BOOLEAN level);

/*****************************************************************************
**  函数名:  PORT_GpioCfg
**  函数描述: gpio设置
**  参数:    [in] cfg : gpio配置参数
**  返回:    无
*****************************************************************************/
void PORT_GpioCfg(PORT_GPIO_CFG_T cfg);

/*****************************************************************************
**  函数名:  PORT_GetIoInitSta
**  函数描述: io口中断状态
**  参数:    无
**  返回:    无
*****************************************************************************/
INT16U PORT_GetIoInitSta(void);

/*****************************************************************************
**  函数名:  PORT_ClearGpioIntSta
**  函数描述: 清除中断标志
**  参数:    无
**  返回:    无
*****************************************************************************/
void PORT_ClearGpioIntSta(void);

/*****************************************************************************
**  函数名:  PORT_GpioLowPower
**  函数描述: gpio低功耗(包括复用io)
**  参数:    [in] gpio_lp : TRUE : 低功耗模式, FALSE : 正常模式
**  返回:    无
*****************************************************************************/
void PORT_GpioLowPower(BOOLEAN gpio_lp);

//-------------------------------------------------------------------------------
#endif /* PORT_GPIO_H */

