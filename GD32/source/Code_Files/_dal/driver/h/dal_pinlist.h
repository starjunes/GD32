/**************************************************************************************************
**                                                                                               **
**  文件名称:  Dal_PinList.h                                                                     **
**  版权所有:  CopyRight @ Xiamen Yaxon NetWork CO.LTD. 2012                                     **
**  创建信息:  2012-1-12 By clt: 创建本文件                                                      **
**  文件描述:  所用管脚列表文件                                                                  **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/
#ifndef __DAL_PINLIST_H
#define __DAL_PINLIST_H

#include "dal_include.h"

#define  GPIO_Pin_0		GPIO_PIN_0
#define  GPIO_Pin_1		GPIO_PIN_1
#define  GPIO_Pin_2		GPIO_PIN_2
#define  GPIO_Pin_3		GPIO_PIN_3
#define  GPIO_Pin_4		GPIO_PIN_4
#define  GPIO_Pin_5		GPIO_PIN_5
#define  GPIO_Pin_6		GPIO_PIN_6
#define  GPIO_Pin_7		GPIO_PIN_7
#define  GPIO_Pin_8		GPIO_PIN_8
#define  GPIO_Pin_9		GPIO_PIN_9
#define  GPIO_Pin_10	GPIO_PIN_10
#define  GPIO_Pin_11	GPIO_PIN_11
#define  GPIO_Pin_12	GPIO_PIN_12
#define  GPIO_Pin_13	GPIO_PIN_13
#define  GPIO_Pin_14	GPIO_PIN_14
#define  GPIO_Pin_15	GPIO_PIN_15

/*                      －　－　－　－　－　－　－　－　－　－　－　－　－                       */
/*                      |                               　　　　　　　　　|                      */
/*                      |                               　　　　　　　　　|                      */
/*                      |                               　　　　　　　　　|                      */
/*                      |                               　　　　　　　　　|                      */
/*                      |                               　　　　　　　　　|                      */
/*                      |                               　　　　　　　　　|                      */
/*                      |                               　　　　　　　　　|                      */
/*                      |              　     GD32F305                   |                      */
/*                      |                               　　　　　　　　　|                      */
/*                      |                               　　　　　　　　　|                      */
/*                      |                               　　　　　　　　　|                      */
/*                      |                               　　　　　　　　　|                      */
/*                      |                               　　　　　　　　　|                      */
/*                      |                               　　　　　　　　　|                      */
/*                      |                               　　　　　　　　　|                      */
/*                      |                               　　　　　　　　　|                      */
/*                      －　－　－　－　－　－　－　－　－　－　－　－　－                       */

/*************************************************************************************************/
/*                           ADC功能相关管脚定义                                                 */
/*************************************************************************************************/
#define  USER0_ADC_CH        ADC_CHANNEL_11
#define  USER0_ADC_GPIOx     GPIOC
#define  USER0_ADC_Pin       GPIO_Pin_1

#define  USER1_ADC_CH        ADC_CHANNEL_12
#define  USER1_ADC_GPIOx     GPIOC
#define  USER1_ADC_Pin       GPIO_Pin_2

#define  VIN_ADC_CH          ADC_CHANNEL_8
#define  VIN_ADC_GPIOx       GPIOB
#define  VIN_ADC_Pin         GPIO_Pin_0
/*************************************************************************************************/
/*                           CAN功能相关管脚定义                                                 */
/*************************************************************************************************/
//#define  CAN_PIN_REMAP        1                                              /* 是否有重映射管脚 */
#define  CAN1_PIN_IO         GPIOD
#define  CAN1_PIN_TX         GPIO_Pin_1
#define  CAN1_PIN_RX         GPIO_Pin_0

#define  CAN2_PIN_IO         GPIOB
#define  CAN2_PIN_TX         GPIO_Pin_6
#define  CAN2_PIN_RX         GPIO_Pin_5

/*************************************************************************************************/
/*                           I2C功能相关管脚定义                                                 */
/*************************************************************************************************/
//#define  I2C_PIN_IO          GPIOB
//#define  I2C_PIN_CLK         GPIO_Pin_6
//#define  I2C_PIN_DATA        GPIO_Pin_7
/*************************************************************************************************/
/*                           物理串口功能相关管脚定义                                            */
/*************************************************************************************************/
#define  USART1_PIN_IO       GPIOA                                             /* 主串口 */
#define  USART1_PIN_TX       GPIO_Pin_9
#define  USART1_PIN_RX       GPIO_Pin_10

#define  USART2_PIN_IO       GPIOA                                             /* 备用串口 */
#define  USART2_PIN_CTX      GPIO_Pin_0
#define  USART2_PIN_RTX      GPIO_Pin_1
#define  USART2_PIN_TX       GPIO_Pin_2
#define  USART2_PIN_RX       GPIO_Pin_3
#define  EXT_PHY2_POWCTRL    GPIOD, GPIO_Pin_7                                 /* 备用串口电源控制管脚 */

#define  USART3_PIN_IO       GPIOD                                             /* K线 */
#define  USART3_PIN_TX       GPIO_Pin_8
#define  USART3_PIN_RX       GPIO_Pin_9

#define  USART4_PIN_IO       GPIOC                                             /* DB9行驶记录导出 */
#define  USART4_PIN_TX       GPIO_Pin_10
#define  USART4_PIN_RX       GPIO_Pin_11

#define  USART5_PIN_TXIO     GPIOC                                             /* 备用串口(485) */
#define  USART5_PIN_TX       GPIO_Pin_12
#define  USART5_PIN_RXIO     GPIOD
#define  USART5_PIN_RX       GPIO_Pin_2

/*************************************************************************************************/
/*                           蜂鸣器功能管脚定义                                                  */
/*************************************************************************************************/
#define  BUZZER0_PORT        GPIOB
#define  BUZZER0_PIN         GPIO_Pin_1

/*************************************************************************************************/
/*                           flash管脚定义                                                       */
/*************************************************************************************************/
#define  PORT_SFLASH_CS      GPIOA, GPIO_Pin_15
#define  PORT_SFLASH_SCK     GPIOC, GPIO_Pin_10
#define  PORT_SFLASH_SO      GPIOC, GPIO_Pin_11
#define  PORT_SFLASH_SI      GPIOC, GPIO_Pin_12

/*************************************************************************************************/
/*                           碰撞功能相关管脚定义                                                */
/*************************************************************************************************/
#define HIT_DA_PORT          GPIOC, GPIO_Pin_9
#define HIT_CL_PORT          GPIOA, GPIO_Pin_8

/*************************************************************************************************/
/*                           SPI功能相关管脚定义                                                 */
/*************************************************************************************************/
/* SPI1用于加密芯片 */
#define SPI1_PIN_IO          GPIOA
#define SPI1_PIN_CS          GPIO_Pin_4
#define SPI1_PIN_SCK         GPIO_Pin_5
#define SPI1_PIN_MISO        GPIO_Pin_6
#define SPI1_PIN_MOSI        GPIO_Pin_7

/* SPI4用于can芯片 */
#define SPI4_PIN_IO          GPIOE
#define SPI4_PIN_CS          GPIO_Pin_4
#define SPI4_PIN_SCK         GPIO_Pin_2
#define SPI4_PIN_MISO        GPIO_Pin_5
#define SPI4_PIN_MOSI        GPIO_Pin_6
#endif
/**************************** (C) COPYRIGHT 2012  XIAMEN YAXON.LTD **************END OF FILE******/

