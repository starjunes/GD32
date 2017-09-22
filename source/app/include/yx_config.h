/******************************************************************************
**
** filename:     yx_config.h
** copyright:    
** description:  该模块主要实现功能配置
**
-------------------------------------------------------------------------------
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#ifndef YX_CONFIG_H
#define YX_CONFIG_H


/*
********************************************************************************
* define config parameters
********************************************************************************
*/

#define VERSION_HW_OLD       0                   /* 旧硬件版本 */
#define VERSION_HW_NEW       1                   /* 新硬件版本 */
#define VERSION_HW           VERSION_HW_NEW

#define EN_APP               1                   /* 0表示BOOT程序,1表示应用程序 */
#define EN_FREECONST         1
#define EN_ICCARD            1                   /* IC卡刷卡 */
#define EN_ICCARD_PASER      0                   /* IC卡刷卡,解析数据 */
#define EN_MMI               1                   /* 与主机通信驱动 */
#define EN_GSENSOR           1                   /* 重力传感器 */
#define EN_CAN               1                   /* CAN总线通信 */
#define EN_UARTEXT           1                   /* UART3串口扩展与AD1/AD2共用 */

#define EN_AT                1                   /* AT指令驱动 */
#define EN_AT_PHONE          1                   /* 电话 */
#define EN_AT_SMS            1                   /* 短消息 */
#define EN_AT_GPRS           0                   /* GPRS */


#define GSM_SIM800           0
#define GSM_SIMCOM           1
#define GSM_BENQ             2
#define GSM_TYPE             GSM_SIMCOM


#if EN_DEBUG > 0
#define DEBUG_MMI            1                   /* 调试外设通信 */
#define DEBUG_ICCARD         1                   /* 调试IC卡刷卡功能 */
#define DEBUG_PP             1                   /* 调试PP 存储驱动 */
#define DEBUG_KEYBOARD       1                   /* 调试按键 */
#define DEBUG_ADC            1                   /* ADC采样 */
#define DEBUG_GSENSOR        1                   /* 重力传感器 */
#define DEBUG_AT             1                   /* AT指令驱动 */
#define DEBUG_TLINK          1                   /* 调试与服务器通信链路 */
#else

#define DEBUG_MMI            1
#define DEBUG_ICCARD         1
#define DEBUG_PP             1
#define DEBUG_KEYBOARD       1
#define DEBUG_ADC            1
#define DEBUG_GSENSOR        1
#define DEBUG_AT             1
#define DEBUG_TLINK          1
#endif


#endif

/*****END OF FILE****/
