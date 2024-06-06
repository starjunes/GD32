/********************************************************************************
**
** 文件名:     bal_gpio_cfg.h
** 版权所有:   (c) 1998-2017 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现GPIO接口的定义和配置
**
*********************************************************************************
**                  修改历史记录
**===============================================================================
**| 日期              | 作者        |  修改记录
**===============================================================================
**| 2017/05/12 | 黄运峰    |  移植、修改、规范化
********************************************************************************/
#ifndef BAL_GPIO_CFG_H
#define BAL_GPIO_CFG_H      1

#include "port_gpio.h"
/*
*********************************************************************************
**              输出管脚操作接口定义
*********************************************************************************
*/
/********************************************************************************
*  看门狗
********************************************************************************/
void bal_ClearWatchdog(void);

/********************************************************************************
*  指示灯控制
********************************************************************************/
// 电源指示灯
void bal_Init_PIN_PWRLED(void);
void bal_Pullup_PWRLED(void);
void bal_Pulldown_PWRLED(void);

// 定位灯
void bal_Init_PIN_GPSLED(void);
void bal_Pullup_GPSLED(void);
void bal_Pulldown_GPSLED(void);
/* CAN灯 */
void bal_Init_PIN_CANLED(void);
void bal_Pullup_CANLED(void);
void bal_Pulldown_CANLED(void);
/* WIFI灯 */
void bal_Init_PIN_WIFILED(void);
void bal_Pullup_WIFILED(void);
void bal_Pulldown_WIFILED(void);


/********************************************************************************
*  板上电源使能控制
********************************************************************************/
// 板上外围3.3V电源开关
void bal_Pullup_EXT3V(void);
void bal_PullDown_EXT3V(void);
// 板上外围1.8V电源开关
void bal_Pullup_EXT1V8(void);
void bal_PullDown_EXT1V8(void);

// GSM4V电源DC芯片使能
void bal_Pullup_GSM4VIC(void);
void bal_Pulldown_GSM4VIC(void);

// GSM模块电源使能开关
void bal_Pullup_GSMPWR(void);
void bal_Pulldown_GSMPWR(void);

/* EC20串口通道功能切换 */
void bal_Pulldown_232CON(void);
void bal_Pullup_232CON(void);
/* USB供电控制 */
void bal_Pulldown_VBUSCTL(void);
void bal_Pullup_VBUSCTL(void);
/* 加密芯片唤醒 */
void bal_Pulldown_HSMWK(void);
void bal_Pullup_HSMWK(void);
/* 加密芯片电源控制 */
void bal_Pulldown_HSMPWR(void);
void bal_Pullup_HSMPWR(void);
/* 蓝牙/wifi电源控制 */
void bal_Pulldown_BTWLEN(void);
void bal_Pullup_BTWLEN(void);
/* GPS电源控制 */
void bal_Pulldown_GPSPWR(void);
void bal_Pullup_GPSPWR(void);
/* PHY电源控制 */
void bal_Pulldown_PHYPWR(void);
void bal_Pullup_PHYPWR(void);
/* 高精度定位模块热启动控制 */
void bal_Pulldown_GPSBAT(void);
void bal_Pullup_GPSBAT(void);
/* 音频，加密，串口转换电源控制 */
void bal_Pulldown_5VEXT(void);
void bal_Pullup_5VEXT(void);
/* 陀螺仪电源控制，拉高断电 */
void bal_Pulldown_GYRPWR(void);
void bal_Pullup_GYRPWR(void);
/* 备用电池充电使能控制 */
void bal_Pulldown_CHGEN(void);
void bal_Pullup_CHGEN(void);
/* CAN0 Standby模式使能控制 */
void bal_CAN0STB_Init(void);
void bal_Pulldown_CAN0STB(void);
void bal_Pullup_CAN0STB(void);
/* CAN1 Standby模式使能控制 */
void bal_Pulldown_CAN1STB(void);
void bal_Pullup_CAN1STB(void);
/* CAN2 Standby模式使能控制 */
void bal_Pulldown_CAN2STB(void);
void bal_Pullup_CAN2STB(void);

/* 通讯模块电源低功耗模式控制 */
void bal_Pulldown_PSCTRL(void);
void bal_Pullup_PSCTRL(void);

/* 备用电池总开关控制 */
void bal_Pulldown_BATSHUT(void);
void bal_Pullup_BATSHUT(void);



// ...

/*
*********************************************************************************
**              输入管脚操作接口定义
*********************************************************************************
*/
/********************************************************************************
*  ACC和IGN状态
********************************************************************************/
void bal_InitPort_ACC(void);
BOOLEAN bal_ReadPort_ACC(void);

void bal_InitPort_ECALL(void);
BOOLEAN bal_ReadPort_ECALL(void);

void bal_InitPort_BCALL(void);
BOOLEAN bal_ReadPort_BCALL(void);

void bal_InitPort_PWRDECT(void);
BOOLEAN bal_ReadPort_PWRDECT(void);

void bal_InitPort_GPSOPEN(void);
BOOLEAN bal_ReadPort_GPSOPEN(void);

void bal_InitPort_GPSSHORT(void);
BOOLEAN bal_ReadPort_GPSSHORT(void);

/*
********************************************************************************
*  主电源断电状态检测
********************************************************************************
*/
void bal_InitPort_PWRDECT(void);
BOOLEAN bal_ReadPort_PWRDECT(void);

/*
********************************************************************************
*  主电欠压状态检测
********************************************************************************
*/
void bal_InitPort_VINLOW(void);

/********************************************************************************
** 函数名:     ReadPort_VINLOW
** 函数描述:   读取欠压状态
** 参数:       无
** 返回:       欠压返回true，正常返回false
********************************************************************************/
BOOLEAN bal_ReadPort_VINLOW(void);

/********************************************************************************
** 函数名:     ReadValue_VINLOW
** 函数描述:   读取主电源ADC值
** 参数:       无
** 返回:       返回ADC值
********************************************************************************/
INT32U bal_ReadValue_VINLOW(void);

/********************************************************************************
** 函数名:     bal_gpio_InitCfg
** 函数描述:   初始化GPIO配置
** 参数:       无
** 返回:       无
** 备注：      未注册到input_reg和output_reg中的IO都要在此函数中初始化
********************************************************************************/
void bal_gpio_InitCfg(void);
//-------------------------------------------------------------------------------
#endif /* BAL_GPIO_CFG_H */

