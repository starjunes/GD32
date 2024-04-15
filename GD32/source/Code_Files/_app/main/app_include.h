/**************************************************************************************************
**                                                                                               **
**  文件名称:  app_include.h                                                                     **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2010                                    **
**  创建信息:  LEON -- 2010年12月1日                                                             **
**  文件描述:  应用层头包含文件                                                                  **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/
#ifndef __APP_INCLUDES_H
#define __APP_INCLUDES_H

#include  "dal_include.h"

/*************************************************************************************************/
/*                           定义app层参数极限值                                                 */
/*************************************************************************************************/
#define MAXPAGE              20                                       /* readpage最大分7页 */
#define MAX_SMS_LEN          160                                     /* 短信信息最大长度*/
#if SWITCH_FLASH == FLASH_512K
#define MAX_MSG_LEN          100                                     /* 系统信息的最大长度*/
#elif (SWITCH_FLASH == FLASH_2M)
#define MAX_MSG_LEN          140
#endif
#define MAX_TELLEN           20                                      /* 系统最大手机号码长度 */
#define DRIVERIDLEN          26//32                                      /* 司机代码长度 */


#if EN_LOCK > 0
#define SPEED_COEFFICIENT    103    /* 速度系数 */
#else
#define SPEED_COEFFICIENT    100    /* 速度系数 */
#endif

/*************************************************************************************************/
/*                           定义app层调试开关                                                   */
/*************************************************************************************************/
#if EN_DEBUG > 0
#define DEBUG_RAMDATA        0                                       /* RAM参数开关 */
#define DEBUG_PARA           0                                       /* 参数开关 */
#define DEBUG_PRINT          0                                       /* 打印机功能模块调试编译开关 */
#define DEBUG_EXPDATA        0                                       /* 数据导出模块调试编译开关 */
#define DEBUG_ICCARD         0                                       /* IC卡模块调试编译开关 */
#define DEBUG_CAN            0                                       /* CAN模块调试编译开关 */
#define DEBUG_BT             0                                       /* 蓝牙模块调试编译开关 */
#define DEBUG_PM             0                                       /* 协议机模块调试编译开关 */
#define DEBUG_RFID           0                                       /* RFID调试 */
#define DEBUG_FILESYS        0                                       /* 文件系统功能模块调试编译开关 */
#define DEBUG_DIR            0                                       /* 调试重复创建文件夹问题及文件个数受限 */
#define DEBUG_UPDATE         0                                       /* U盘升级功能模块调试编译开关 */
#define DEBUG_USB            0                                       /* USB功能模块调试编译开关，主要是U数据导出应用层调试 */
#define DEBUG_HCD            0                                       /* USB功能模块调试编译开关，主要是内核部分的调试 */
#define DEBUG_USBD           0                                       /*  */
#define DEBUG_UMASS          0                                       /*  */
#define DEBUG_USBIN          0                                       /*  */
#define DEBUG_USBCORE        0                                       /*  */
#define DEBUG_DATATEST       0                                       /*  */
#define DEBUG_DATATEST1      0                                       /*  */
#define DEBUG_LCD            0                                       /*  */
#define DEBUG_MEDIAEXP       0                                       /* 媒体数据导出模块调试编译开关 */
#define DEBUG_APP_UPDATE     1                                       /* 升级模块调试编译开关 */
#define DEBUG_TEST           0                                       /* 生产检测 */
#define DEBUG_PMM0_send      0                                       /* MOpmSEND */
#define DEBUG_EXIO           0
#define DEBUG_BUZZER         0
#define DEBUG_SMG            0
#define DEBUG_HIT            0                                       /* 碰撞模块 */
#define DEBUG_MSG            0                                       /* 系统信息调试 */
#define DEBUG_RESETBUG       0                                       /* 无理由复位查找*/
#define DEBUG_FILTER         0                                       /* 信号滤波*/
#define DEBUG_KLINE          0
#define DEBUG_COLER          0
#define DEBUG_PHONE          0
#define DEBUG_CONFIG         0
#define DEBUG_LOCK           0                                       /* 锁车 */
#define DEBUG_RESET          1                                       /* 复位记录 */
#define DEBUG_RTC            0                                       /* 实时时钟 */
#define DEBUG_CANERROR       0                                       /* CAN ERROR */
#define DEBUG_HEAPMEM        0
#define DEBUG_RESTEST        0
#define DEBUG_J1939          0
#define DEBUG_UDS            0
#define DEBUG_EX_RTC         0                                       /* 调试外部RTC */
#define DEBUG_TEMP			 0		/*临时调试用*/

#else
#define DEBUG_RAMDATA        0                                       /* RAM参数开关 */
#define DEBUG_PARA           0                                       /* 参数开关 */
#define DEBUG_PRINT          0                                       /* 打印机功能模块调试编译开关 */
#define DEBUG_EXPDATA        0                                       /* 数据导出模块调试编译开关 */
#define DEBUG_ICCARD         0                                       /* IC卡模块调试编译开关 */
#define DEBUG_CAN            0                                       /* CAN模块调试编译开关 */
#define DEBUG_BT             0                                       /* 蓝牙模块调试编译开关 */
#define DEBUG_PM             0                                       /* 协议机模块调试编译开关 */
#define DEBUG_RFID           0                                       /* RFID调试 */
#define DEBUG_FILESYS        0                                       /* 文件系统功能模块调试编译开关 */
#define DEBUG_DIR            0                                       /* 调试重复创建文件夹问题及文件个数受限 */
#define DEBUG_UPDATE         0                                       /* U盘升级功能模块调试编译开关 */
#define DEBUG_USB            0                                       /* USB功能模块调试编译开关，主要是U数据导出应用层调试 */
#define DEBUG_HCD            0                                       /* USB功能模块调试编译开关，主要是内核部分的调试 */
#define DEBUG_USBD           0                                       /*  */
#define DEBUG_UMASS          0                                       /*  */
#define DEBUG_USBIN          0                                       /*  */
#define DEBUG_USBCORE        0                                       /*  */
#define DEBUG_DATATEST       0                                       /*  */
#define DEBUG_DATATEST1      0                                       /*  */
#define DEBUG_LCD            0                                       /*  */
#define DEBUG_MEDIAEXP       0                                       /* 媒体数据导出模块调试编译开关 */
#define DEBUG_APP_UPDATE     0                                       /* 升级模块调试编译开关 */
#define DEBUG_TEST           0                                       /* 生产检测 */
#define DEBUG_PMM0_send      0                                       /* MOpmSEND */
#define DEBUG_EXIO           0
#define DEBUG_BUZZER         0
#define DEBUG_SMG            0
#define DEBUG_HIT            0                                       /* 碰撞模块 */
#define DEBUG_MSG            0                                       /* 系统信息调试 */
#define DEBUG_RESETBUG       0                                       /* 无理由复位查找*/
#define DEBUG_FILTER         0                                       /* 信号滤波*/
#define DEBUG_KLINE          0
#define DEBUG_COLER          0
#define DEBUG_PHONE          0
#define DEBUG_LOCK           0                                       /* 锁车 */
#define DEBUG_RESET          0
#define DEBUG_RTC            0
#define DEBUG_J1939          0
#define DEBUG_UDS            0
#define DEBUG_EX_RTC         0                                       /* 调试外部RTC */
#define DEBUG_TEMP			 0		/*临时调试用*/
#endif

/*************************************************************************************************/
/*                           APP层的断言函数定义                                                 */
/*************************************************************************************************/
#define APP_ASSERT(expr)     if (!(expr)) {mmi_resetinform(RESET_ABNORMAL, (INT8U *)__FILE__ + strlen(__FILE__) - FILENAMELEN, __LINE__, ERR_ID_ERR);\
    SysErrorExit( __FILE__, __LINE__);}

#endif

