/**************************************************************************************************
**                                                                                               **
**  文件名称:  Dal_Include.H                                                                     **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2010                                    **
**  创建信息:  LEON -- 2010年12月1日                                                             **
**  文件描述:  设备抽象层头包含文件                                                              **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/
#ifndef __DAL_INCLUDES_H
#define __DAL_INCLUDES_H

#include  "gd32f30x.h"
#include  "dal_structs.h"
#include  "man_error.h"
#include  "dal_hard.h"
#include  "mmi_resetcnt.h"
#include  "debug_print.h"

#define PNULL NULL
/*************************************************************************************************/
/*                           定义设备类型及一些硬件配置                                         */
/*************************************************************************************************/
#define GK_120R              0x01
#define GK_110R              0x02
#define GK_120D2             0x03
#define SWITCH_DEVICE        GK_110R                                 /* 在这里选择设备类型 */

#define DOT19264             0x01                                    /* 点阵192 X 64 并行 */
#define DOT12864             0x02                                    /* 点阵128 X 64 串行 */
#define DOT12832             0x03                                    /* 点阵128*32串行 */
#define SWITCH_DOT           DOT12832                                /* 在这里选择液晶屏类型 */

#define LCD_PATALLEL         0x01                                    /* 并口液晶屏 */
#define LCD_SERIAL           0x02                                    /* 串口液晶屏 */
#define SWITCH_LCD           LCD_SERIAL                              /* 在这里选择液晶屏类型 */

#define SLCD_NORMAL          0x01                                    /* 常温液晶屏 */
#define SLCD_WIDE            0x02                                    /* 宽温液晶屏 */
#define SWITCH_LCD_STATU     SLCD_NORMAL                             /* */


#define FLASH_512K           0x01                                    /* 512K */
#define FLASH_2M             0x02                                    /* 2M */
#define SWITCH_FLASH         FLASH_2M                                /* 在这里选择串行FLASH类型 */

#define GPRS                 0x01
#define CDMA                 0x02
#define SWITCH_NET           GPRS                                    /* 在这里选择网络类型 */

#define PRINTER_NONE         0x00                                    /* 不带打印机驱动(但可能外接串口打印机，按通用串口数据由车台处理) */
#define PRINTER_EXTERNAL     0x01                                    /* 用于120R旧版本设备，外接打印机驱动芯片，按特殊串口数据由M3处理 */
#define PRINTER_BUILT        0x02                                    /* 用内置打印机驱动 */
#define SWITCH_PRINTER       PRINTER_BUILT                           /* 在这里选择打印机驱动类型 */


#define MAX_CANCHAN          2                                       /* 总共两路CAN通道 */

/*************************************************************************************************/
/*                           定义系统时钟频率(这个与drv_hard.c中的配置对应)                      */
/*************************************************************************************************/
#define SYS_CLK_FREQ         120000000
#define PCLK2_FREQ           120000000
#define PCLK1_FREQ           60000000

/*************************************************************************************************/
/*                           定义dal层参数极限值                                                 */
/*************************************************************************************************/
#define Q_MAX_NUMS           6                                       /* 消息队列最大个数 */
#define MAX_SCANENTRY        7                                       /* 扫描器最大个数 */
#define MEM_MAX_PART         6                                       /* 动态内存最大个数 */

/*************************************************************************************************/
/*                           定义dal层功能开关                                                   */
/*************************************************************************************************/
#define EN_ADC               1                                       /* AD采集 */
#define EN_CAN               1                                       /* CAN数据通信 */
#define EN_I2C               1                                       /* I2C数据通信 */
#define EN_EXPDATA           1                                       /* 数据导出 */
#define EN_PRINTER           1                                       /* 打印机 */
#define EN_USB               0                                       /* U盘功能 */
#define EN_USBUPDATE         0                                       /* U盘升级 */
#define EN_PWM_CLK           1                                       /* 实时秒脉冲输出 */
#define EN_GSENSOR           1                                       /* 六轴加速度传感器 */
#define EN_RTC               0                                       /* 实时时钟 */

#define EN_DELETE            1                                       /* 允许菜单上有删除功能 */
#define EN_CONNECT_TEST      0                                       /* 定义用来开机检测连接 */
#define EN_CHECKFLASH        0                                       /* 写数据库时必须有正确的校验码 */
#define EN_POPMSG            0                                       /* 禁止系统消息自动弹出,此处不要打开 */
#define EN_CHK_SMSNAME       0                                       /* 按照号码所对应名字显示短信, 打开会很慢 */
#define EN_HELP              0                                       /* 求助功能 */
#define EN_DRIVE_RECORDE     1                                       /* 行驶记录功能 */
#define EN_PASSWORD_NET      0                                       /* 网络参数增加一层密码 */
#define EN_IO_EXTEND         0                                       /* SN7325(120R) IO扩展功能 */
#define EN_PRINTDATATOUSB    1                                       /* 打印数据同时写到U盘中 */
#define EN_LOCKSCREEN        0                                       /* 是否锁屏 */

#define EN_IO_EXTEND_R6      0                                       /* SN7325(110R6) IO扩展功能 */
#define EN_IO_CHK            0                                       /* IO扩展检测功能(110R) */

#define EN_ICCANPARA_RD      0                                       /* 查询IC卡信息是否读取配置卡信息 */
#define EN_ICCAN_PARA        1                                       /* 配置卡使能。为0时默认发送车速、时间报文，无需配置卡配置 */

#define EN_LOCK              1                                       /* 是否包含锁车功能 */
#define EN_CAN_EXCHG         0                                       /* 是否CAN通道交换 */

#define EN_ECUDATA_REPORT    0                                       /* 是否上报ECU通信数据 */

#define SHD_OLD              0x01                                    /* 硬发时出现的中间版本，有10台样机，未生产，后期也不会用*/
#define SHD_NEW              0x02
#define SWITCH_SHD_VER       SHD_NEW                                 /* 在这里选择硬件版本，新版本在硬件版本上对IO口做了调整 */

#define SDB_DEBUG            0x01
#define SDB_BLUTH            0x02
#define SDB_UNDEF            0x03
#define SWITCH_DEBUG_BT      SDB_DEBUG//SDB_UNDEF                               /* 在这里选择调试，或是蓝牙，或是留空 */

#define SIR_ICCD             0x01
#define SIR_RFID             0x02                                    /* 屏有解稀的处理 */
#define SWITCH_IC_RFID       SIR_ICCD                                /* 在这里选择IC卡还是RFID */
#define SATELLITE_CHOOSE     1                                       /* 增加卫星定位模式选择 */

/*************************************************************************************************/
/*                           以下语句不用改动，自适应                                            */
/*************************************************************************************************/
#if SWITCH_DEBUG_BT == SDB_UNDEF
#define EN_DEBUG             0                                       /* 禁用调试功能 */
#define EN_BT                0                                       /* 禁用蓝牙功能 */
#define USART_DEBUG          USART_NO3
#define BAUD_DEBUG           BAUD_115200
#elif SWITCH_DEBUG_BT == SDB_DEBUG
#define EN_DEBUG             1                                       /* 启用调试功能 */
#define EN_BT                0                                       /* 禁用蓝牙功能 */
#define USART_DEBUG          USART_NO3
#define BAUD_DEBUG           BAUD_115200
#else
#define EN_DEBUG             0                                       /* 禁用调试功能 */
#define EN_BT                1                                       /* 启用蓝牙功能 */
#define USART_BT             USART_NO3
#define BAUD_BT              BAUD_57600
#endif

/*************************************************************************************************/
/*                           以下语句不用改动，自适应                                            */
/*************************************************************************************************/
#if SWITCH_IC_RFID == SIR_ICCD
#define EN_ICCARD            1                                       /* 启用IC卡功能 */
#define EN_RFID              0                                       /* 禁用RFID功能 */
#else
#define EN_ICCARD            0                                       /* 禁用IC卡功能 */
#define EN_RFID              1                                       /* 启用RFID功能 */
#endif

/*************************************************************************************************/
/*                           定义FLASH的扇区大小和扇区数目                                       */
/*************************************************************************************************/

#define FLASH_MAX_SECTOR     8
#define FLASH_MAX_PAGES      256
#define FLASH_PAGE_SIZE      (2 * 1024)


/*************************************************************************************************/
/*                           定义主FLASH的扇区基址和最大容量                                       */
/*************************************************************************************************/
#define FLASH_BASE_ADDR      (0x08000000L)                           /* base address of flash */
#define FLASH_MAX_SIZE       (FLASH_PAGE_SIZE * FLASH_MAX_PAGES)     /* flash max size */

/*************************************************************************************************/
/*                           定义主FLASH的扇区分配                                               */
/*************************************************************************************************/
#define B_BOOT_SECTOR         (0)                                    /* boot程序起始块 */
#define E_BOOT_SECTOR         (15)                                    /* 占据32K  */

#define B_MAIN_SECTOR         (32)                                    /* 主程序起始块 */
#define E_MAIN_SECTOR         (255)                                    /* 占据448K  */

#define B_BMAIN_SECTOR        (128)                                    /* 主程序备份起始页 */
#define E_BMAIN_SECTOR        (255)                                    /* 占据256K  */

#define B_PARA1_SECTOR        (16)                                    /* 参数区1起始块 */
#define E_PARA1_SECTOR        (23)                                    /* 占据16K  */

#define B_PARA2_SECTOR        (24)                                    /* 参数区2起始块 */
#define E_PARA2_SECTOR        (31)                                    /* 占据16K  */

#define B_BOOT_PAGE           (0)                                    /* boot程序起始页 */
#define E_BOOT_PAGE           (15)                                    /* 占据32K  */

#define B_MAIN_PAGE           (32)                                    /* 主程序起始页 */
#define E_MAIN_PAGE           (255)                                   /* 占据448K  */

#define B_BMAIN_PAGE          (128)                                   /* 主程序备份起始页 */
#define E_BMAIN_PAGE          (255)                                   /* 占据256K  */

#define B_PARA1_PAGE          (16)                                    /* 参数区1起始页 */
#define E_PARA1_PAGE          (16)                                    /* 占据16K  */

#define B_PARA2_PAGE          (24)                                    /* 参数区2起始页 */
#define E_PARA2_PAGE          (24)                                    /* 占据16K  */

/*************************************************************************************************/
/*                           定义串行FLASH的扇区分配(仅用于110R2-10)                             */
/*************************************************************************************************/
#define B_FONTS_PAGE          (0)                                    /* 字库起始扇区 */
#define E_FONTS_PAGE          (255)                                  /* 占据扇区256扇区 1M  */

#define B_USRPARA_PAGE        (256)                                  /* 用户参数区1起始扇区 */
#define E_USRPARA_PAGE        (399)                                  /* 占据扇区144扇区 576K  */

#define B_BACK_PAGE           (400)                                  /* 备份区1起始扇区 */
#define E_BACK_PAGE           (511)                                  /* 占据页数112扇区 448K  */

#if E_MAIN_PAGE >= FLASH_MAX_PAGES
#error  "Error: Set To Much Sectors...!"
#endif

/*************************************************************************************************/
/*                           DAL层的断言函数定义                                                 */
/*************************************************************************************************/
#define DAL_ASSERT(expr)     if (!(expr)) {mmi_resetinform(RESET_ABNORMAL, (INT8U *)__FILE__ + strlen(__FILE__) - FILENAMELEN, __LINE__, ERR_ID_ERR);\
    SysErrorExit( __FILE__, __LINE__);}

/*************************************************************************************************/
/*                           系统进入中断保护现场                                                */
/*************************************************************************************************/
#define ENTER_CRITICAL()     do {                                    \
                                   if (!(__get_IPSR() & 0x1ff)) {    \
                                      __disable_fault_irq();         \
                                   }                                 \
                                } while(0)

/*************************************************************************************************/
/*                           系统退出中断恢复现场                                                */
/*************************************************************************************************/
#define EXIT_CRITICAL()      do {                                    \
                                   if (!(__get_IPSR() & 0x1ff)) {    \
                                      __enable_fault_irq();          \
                                   }                                 \
                                } while(0)

#define DM_OT_NULL                      0
#define MemAlloc(SIZE)                mmi_memalloc(SIZE, DM_OT_NULL, __FILE__, __LINE__)
#define MemFree(PTR)                  mmi_memfree(PTR, __FILE__, __LINE__)

#define MMI_MEMCPY(DET_PTR, DET_LEN, SRC_PTR, SRC_LEN)  \
do {                                                    \
    DAL_ASSERT((DET_LEN) >= (SRC_LEN));                 \
    memcpy(DET_PTR, SRC_PTR, SRC_LEN);                  \
} while(0)

#endif

