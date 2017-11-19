/******************************************************************************
**
** Filename:     hal_can_drv.h
** Copyright:    
** Description:  该模块主要实现串口的驱动管理
**
*******************************************************************************
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#ifndef HAL_CAN_DRV_H
#define HAL_CAN_DRV_H

#if EN_CAN > 0

#include "hal_can_reg.h"
/*
********************************************************************************
* define config parameters
********************************************************************************
*/

#define MAX_CAN_FILTER_ID_BANK         14        /* 滤波器组个数 */

#define MAX_CAN_FILTER_ID_LIST_STD     (MAX_CAN_FILTER_ID_BANK * 4)            /* 标准帧滤波ID的列表类型个数 */
#define MAX_CAN_FILTER_ID_MASK_STD     (MAX_CAN_FILTER_ID_BANK * 2)            /* 标准帧滤波ID的掩码类型个数 */

#define MAX_CAN_FILTER_ID_LIST_EXT     (MAX_CAN_FILTER_ID_BANK * 2)            /* 扩展帧滤波ID的列表类型个数 */
#define MAX_CAN_FILTER_ID_MASK_EXT     (MAX_CAN_FILTER_ID_BANK * 1)            /* 扩展帧滤波ID的掩码类型个数 */

/* 工作模式 */
typedef enum {
    CAN_WORK_MODE_NORMAL = 0,                    /* normal mode */
    CAN_WORK_MODE_LOOPBACK,                      /* loopback mode */
    CAN_WORK_MODE_SILENT,                        /* silent mode */
    CAN_WORK_MODE_SILENT_LOOPBACK,               /* loopback combined with silent mode */
    CAN_WORK_MODE_MAX
} CAN_WORK_MODE_E;

/* 帧ID类型 */
typedef enum {
    CAN_ID_TYPE_STD = 0,                         /* 标准帧 */
    CAN_ID_TYPE_EXT,                             /* 扩展帧 */
    CAN_ID_TYPE_MAX
} CAN_ID_TYPE_E;

/* 帧数据类型 */
typedef enum {
    CAN_RTR_TYPE_DATA = 0,                       /* 数据帧 */
    CAN_RTR_TYPE_REMOTE,                         /* 远程帧 */
    CAN_RTR_TYPE_MAX
} CAN_RTR_TYPE_E;

/* 错误类型 */
typedef enum {
    CAN_ERROR_NOERR = 0,                         /* 正常 */ 
    CAN_ERROR_STUFF,                             /* 位填充错误 */ 
    CAN_ERROR_FORM,                              /* 格式错误 */ 
    CAN_ERROR_ACK,                               /* 确认错误 */ 
    CAN_ERROR_BITRECESSIVE,                      /* 隐性位错误 */ 
    CAN_ERROR_BITDOMINANT,                       /* 显性位错误 */ 
    CAN_ERROR_CRC,                               /* CRC错误 */ 
    CAN_ERROR_SOFTWARESET,                       /* 软件设置错误 */ 
    CAN_ERROR_MAX
} CAN_ERROR_E;

/* 错误状态 */
typedef enum {
    CAN_ERROR_STEP_NOERR = 0,                    /* 正常 */ 
    CAN_ERROR_STEP_WARNING,                      /* 警告错误 */ 
    CAN_ERROR_STEP_ACTIVE,                       /* 主动错误 */ 
    CAN_ERROR_STEP_PASSIVE,                      /* 被动错误 */ 
    CAN_ERROR_STEP_BUSOFF,                       /* 总线关闭 */ 
    CAN_ERROR_STEP_MAX
} CAN_ERROR_STEP_E;


/*
********************************************************************************
* define struct
********************************************************************************
*/
/* 接收结构体 */
typedef struct {
    INT32U id;               /* 帧ID,标准帧则取值0~0x7FF,扩展帧则取值0~0x1FFFFFFF. */
    INT8U  idtype;           /* 帧ID类型,见 CAN_ID_TYPE_E */
    INT8U  datatype;         /* 帧数据类型,见 CAN_RTR_TYPE_E */
    INT8U  dlc;              /* 数据长度,取值0~8 */
    INT8U  data[8];          /* 帧数据 */
    INT8U  index;            /* 滤波器索引号 */
} CAN_DATA_T;

/* 配置参数 */
typedef struct {
    INT8U  com;        /* 串口通道编号,见CAN_COM_E */
    INT8U  mode;       /* 工作模式,见 CAN_WORK_MODE_E */
    INT8U  idtype;     /* 帧ID类型,见 CAN_ID_TYPE_E */
    INT32U baud;       /* 波特率 */
} CAN_CFG_T;

/* 总线状态 */
typedef struct {
    INT8U  status;           /* 总线状态,TRUE-开启,FALSE-关闭 */
    INT8U  lec;              /* 最后(当前)错误码,即当前错误码,见 CAN_ERROR_E */
    INT8U  errorstep;        /* 错误等级，见 CAN_ERROR_STEP_E */
    INT8U  tec;              /* 发送错误计数器 */
    INT8U  rec;              /* 接收错误计数器 */
} CAN_STATUS_T;


/*******************************************************************
** 函数名称: HAL_CAN_InitDrv
** 函数描述: 初始化驱动
** 参数:     无
** 返回:     无
********************************************************************/
void HAL_CAN_InitDrv(void);

/*******************************************************************
** 函数名:     HAL_CAN_GetStatus
** 函数描述:   获取CAN总线状态
** 参数:       [in] com:  通道编号,见 CAN_COM_E
**             [in] info: 状态信息,见 CAN_STATUS_T
** 返回:       成功返回true, 失败返回false
********************************************************************/
BOOLEAN HAL_CAN_GetStatus(INT8U com, CAN_STATUS_T *info);

/*******************************************************************
** 函数名称:   HAL_CAN_IsOpened
** 函数描述:   通道是否已打开
** 参数:       [in] com: 通道编号,见CAN_COM_E
** 返回:       是返回true,否返回false
********************************************************************/
BOOLEAN HAL_CAN_IsOpened(INT8U com);

/*******************************************************************
** 函数名称:   HAL_CAN_OpenCan
** 函数描述:   打开CAN通信总线
** 参数:       [in]  cfg: 配置参数 
** 返回:       成功返回true,失败返回false
********************************************************************/
BOOLEAN HAL_CAN_OpenCan(CAN_CFG_T *cfg);

/*******************************************************************
** 函数名:     HAL_CAN_CloseCan
** 函数描述:   关闭CAN总线
** 参数:       [in] com: 通道编号,见CAN_COM_E
** 返回:       成功返回true, 失败返回false
********************************************************************/
BOOLEAN HAL_CAN_CloseCan(INT8U com);

/*******************************************************************
** 函数名:     HAL_CAN_SetFilterParaByList
** 函数描述:   设置滤波参数,设置滤波ID列表过滤方式
** 参数:       [in] com:       通道编号,见 CAN_COM_E
**             [in] idtype:    帧ID类型,见 CAN_ID_TYPE_E
**             [in] idnum:     滤波ID个数,为0时表示接收所有ID
**             [in] pfilterid: 滤波ID数组
** 返回:       成功返回true, 失败返回false
********************************************************************/
BOOLEAN HAL_CAN_SetFilterParaByList(INT8U com, INT8U idtype, INT8U idnum, INT32U *pfilterid);

/*******************************************************************
** 函数名:     HAL_CAN_SetFilterParaByMask
** 函数描述:   设置滤波参数,设置滤波ID掩码过滤方式
** 参数:       [in] com:      通道编号,见 CAN_COM_E
**             [in] idtype:   帧ID类型,见 CAN_ID_TYPE_E
**             [in] idnum:    滤波ID个数,为0时表示接收所有ID
**             [in] pfilterid: 滤波ID数组
**             [in] pmaskid:   掩码ID数组
** 返回:       成功返回true, 失败返回false
********************************************************************/
BOOLEAN HAL_CAN_SetFilterParaByMask(INT8U com, INT8U idtype, INT8U idnum, INT32U *pfilterid, INT32U *pmaskid);

/*******************************************************************
** 函数名称: HAL_CAN_ReadData
** 函数描述: 读取一帧数据
** 参数:     [in]  com:  通道编号,见CAN_COM_E
**           [out] data: 数据帧
** 返回:     成功返回数据，失败返回-1
********************************************************************/
BOOLEAN HAL_CAN_ReadData(INT8U com, CAN_DATA_T *data);

/*******************************************************************
** 函数名称: HAL_CAN_SendData
** 函数描述: 发送一帧数据
** 参数:     [in] com:  通道编号,见CAN_COM_E
**           [in] data: 数据帧
** 返回:     成功返回true,失败返回false
********************************************************************/
BOOLEAN HAL_CAN_SendData(INT8U com, CAN_DATA_T *data);

/*******************************************************************
** 函数名称: HAL_CAN_UsedOfRecvbuf
** 函数描述: 获取已接收帧数
** 参数:     [in] com: 通道编号,见CAN_COM_E
** 返回:     已接收数据帧数
********************************************************************/
INT32U HAL_CAN_UsedOfRecvbuf(INT8U com);

/*******************************************************************
** 函数名称: HAL_CAN_LeftOfSendbuf
** 函数描述: 获取发送缓存剩余空间
** 参数:     [in] com: 通道编号,见CAN_COM_E
** 返回:     剩余数据帧数
********************************************************************/
INT32U HAL_CAN_LeftOfSendbuf(INT8U com);


__attribute__ ((section ("IRQ_HANDLE"))) void CAN1_Rx_IrqHandle(void);
__attribute__ ((section ("IRQ_HANDLE"))) void CAN1_Rx1_IrqHandle(void);
__attribute__ ((section ("IRQ_HANDLE"))) void CAN1_Tx_IrqHandle(void);

__attribute__ ((section ("IRQ_HANDLE"))) void CAN2_Rx_IrqHandle(void);
__attribute__ ((section ("IRQ_HANDLE"))) void CAN2_Rx1_IrqHandle(void);
__attribute__ ((section ("IRQ_HANDLE"))) void CAN2_Tx_IrqHandle(void);



/*******************************************************************
** 函数名称: CAN1_IrqHandle
** 函数描述: CAN中断处理
** 参数:     无
** 返回:     无
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void CAN1_IrqHandle(void);

#endif
#endif

