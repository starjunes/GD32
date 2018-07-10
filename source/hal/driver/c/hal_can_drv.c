/******************************************************************************
**
** Filename:     hal_can_drv.c
** Copyright:    
** Description:  该模块主要实现CAN的驱动管理
**
*******************************************************************************
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#include "yx_include.h"
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "yx_dym_drv.h"
#include "yx_loopbuf.h"
#include "hal_can_reg.h"
#include "hal_can_drv.h"
#include "st_irq_drv.h"

#if EN_CAN > 0

/*
********************************************************************************
* define config parameters
********************************************************************************
*/
#define _SENDING             0x01
#define _OPEN                0x80

#define MAX_CAN_SEND         50
#define MAX_CAN_RECV         30

/*
********************************************************************************
* 数据结构定义
********************************************************************************
*/

typedef struct {
    INT8U        status;
    CAN_DATA_Q_T recvloop;
    CAN_DATA_Q_T sendloop;
} CAN_T;

/*
********************************************************************************
* define module variants
********************************************************************************
*/
static CAN_T s_can[CAN_COM_MAX];
static CAN_DATA_T s_can_recv[CAN_COM_MAX][MAX_CAN_RECV];
static CAN_DATA_T s_can_send[CAN_COM_MAX][MAX_CAN_SEND];


/*******************************************************************
** 函数名称:   LeftOfLoopBuffer
** 函数描述:   获取循环缓冲区中剩余的空间
** 参数:       [in]  loop:    循环缓冲区管理结构体
** 返回:       剩余空间字节数
********************************************************************/
INT32U LeftOfLoopBuffer(CAN_DATA_Q_T *loop)
{
    INT32U temp;
    volatile INT32U cpu_sr;
    
    OS_ENTER_CRITICAL();                            /* 关中断 */
    temp = loop->max - loop->used;
    OS_EXIT_CRITICAL();                             /* 开中断 */
    
    return temp;
}

/*******************************************************************
** 函数名称:   UsedOfLoopBuffer
** 函数描述:   获取循环缓冲区中已使用空间
** 参数:       [in]  loop:    循环缓冲区管理结构体
** 返回:       已使用空间字节数
********************************************************************/
static INT32U UsedOfLoopBuffer(CAN_DATA_Q_T *loop)
{
    INT32U temp;
    volatile INT32U cpu_sr;
    
    OS_ENTER_CRITICAL();                            /* 关中断 */
    temp = loop->used;
    OS_EXIT_CRITICAL();                             /* 开中断 */
    
    return temp;
}

/*******************************************************************
** 函数名称:   UsedOfLoopBuffer_INT
** 函数描述:   获取循环缓冲区中已使用空间
** 参数:       [in]  loop:    循环缓冲区管理结构体
** 返回:       已使用空间字节数
********************************************************************/
static INT32U UsedOfLoopBuffer_INT(CAN_DATA_Q_T *loop)
{
    return loop->used;
}

/*******************************************************************
** 函数名称:   ReadLoopData
** 函数描述:   从循环缓冲区中读取一帧数据
** 参数:       [in]  loop:    循环缓冲区管理结构体
**             [out] data:    读取数据
** 返回:       成功返回TRUE,失败返回FALSE
********************************************************************/
static BOOLEAN ReadLoopData(CAN_DATA_Q_T *loop, CAN_DATA_T *data)
{
    INT16U used, pos;
    volatile INT32U cpu_sr;
    
    OS_ENTER_CRITICAL();                            /* 关中断 */
    used = loop->used;
    pos  = loop->pos;
    OS_EXIT_CRITICAL();                             /* 开中断 */
    
    if (used == 0) {
        return false;
    }
    
    YX_MEMCPY((INT8U *)data, sizeof(CAN_DATA_T), (INT8U *)&loop->pmsg[pos], sizeof(CAN_DATA_T));
    
    OS_ENTER_CRITICAL();                            /* 关中断 */
    loop->used--;
    loop->pos++;
    if (loop->pos >= loop->max) {
        loop->pos -= loop->max;
    }
    OS_EXIT_CRITICAL();                             /* 开中断 */
    return true;
}

/*******************************************************************
** 函数名称:   ReadLoopData_INT
** 函数描述:   从循环缓冲区中读取一帧数据
** 参数:       [in]  loop:    循环缓冲区管理结构体
**             [out] data:    读取数据
** 返回:       成功返回TRUE,失败返回FALSE
********************************************************************/
static BOOLEAN ReadLoopData_INT(CAN_DATA_Q_T *loop, CAN_DATA_T *data)
{
    INT16U used, pos;
    
    used = loop->used;
    pos  = loop->pos;
    
    if (used == 0) {
        return false;
    }
    
    YX_MEMCPY((INT8U *)data, sizeof(CAN_DATA_T), (INT8U *)&loop->pmsg[pos], sizeof(CAN_DATA_T));
    
    loop->used--;
    loop->pos++;
    if (loop->pos >= loop->max) {
        loop->pos -= loop->max;
    }
    return true;
}

/*******************************************************************
** 函数名称:   WriteLoopData
** 函数描述:   往循环缓冲区中写入一帧数据
** 参数:       [in]  loop:   循环缓冲区管理结构体
**             [in]  data:   写入数据
** 返回:       成功返回true,失败返回false
********************************************************************/
static BOOLEAN WriteLoopData(CAN_DATA_Q_T *loop, CAN_DATA_T *data)
{
    INT16U used, pos;
    volatile INT32U cpu_sr;
    
    OS_ENTER_CRITICAL();                            /* 关中断 */
    used = loop->used;
    pos  = loop->pos;
    OS_EXIT_CRITICAL();                             /* 开中断 */
    
    if (used >= loop->max) {
        return FALSE;
    }
    
    pos += used;
    if (pos >= loop->max) {
        pos -= loop->max;
    }
        
    YX_MEMCPY((INT8U *)&loop->pmsg[pos], sizeof(CAN_DATA_T), (INT8U *)data, sizeof(CAN_DATA_T));
    
    OS_ENTER_CRITICAL();                            /* 关中断 */
    loop->used++;
    OS_EXIT_CRITICAL();                             /* 开中断 */
    return TRUE;
}

/*******************************************************************
** 函数名称:   WriteLoopData_INT
** 函数描述:   往循环缓冲区中写入一帧数据
** 参数:       [in]  loop:   循环缓冲区管理结构体
**             [in]  data:   写入数据
** 返回:       成功返回true,失败返回false
********************************************************************/
static BOOLEAN WriteLoopData_INT(CAN_DATA_Q_T *loop, CAN_DATA_T *data)
{
    INT16U used, pos;
    
    used = loop->used;
    pos  = loop->pos;
    
    if (used >= loop->max) {
        return FALSE;
    }
    
    pos += used;
    if (pos >= loop->max) {
        pos -= loop->max;
    }
        
    YX_MEMCPY((INT8U *)&loop->pmsg[pos], sizeof(CAN_DATA_T), (INT8U *)data, sizeof(CAN_DATA_T));
    
    loop->used++;
    return TRUE;
}

/*******************************************************************
** 函数名称: STM32_CAN_PinsConfig
** 函数描述: 管脚配置
** 参数:     [in] pinfo: 配置表
**           [in] onoff: 打开/关闭, TRUE-打开串口,FALSE-关闭串口
** 返回:     无
********************************************************************/
static void STM32_CAN_PinsConfig(const CAN_REG_T *pinfo, INT8U onoff)
{
    GPIO_InitTypeDef gpio_initstruct;
    
    if (pinfo == 0) {
        return;
    }
    
    RCC_AHBPeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOB, ENABLE);                        /* 开启GPIO系统时钟 */

    switch(pinfo->can_base) {
        case CAN1_BASE:
            GPIO_PinRemapConfig(GPIO_Remap1_CAN1 , ENABLE);
            break;
        case CAN2_BASE:
            GPIO_PinRemapConfig(GPIO_Remap_CAN2, ENABLE);
            break;
    }
    
    /* Configure USARTx_Tx as alternate function push-pull */
    gpio_initstruct.GPIO_Pin   = (INT16U)(1 << pinfo->pin_tx);
    gpio_initstruct.GPIO_Speed = GPIO_Speed_50MHz;
    //gpio_initstruct.GPIO_OType = GPIO_OType_PP;
    //gpio_initstruct.GPIO_PuPd  = GPIO_PuPd_UP;
    if (onoff) {
        gpio_initstruct.GPIO_Mode  = GPIO_Mode_AF_PP;
        GPIO_Init((GPIO_TypeDef *)pinfo->gpio_base, &gpio_initstruct);
        //GPIO_PinAFConfig((GPIO_TypeDef *)pinfo->gpio_base, pinfo->pin_tx, GPIO_AF_4);
    } else {
        gpio_initstruct.GPIO_Mode  = GPIO_Mode_AIN;
        GPIO_Init((GPIO_TypeDef *)pinfo->gpio_base, &gpio_initstruct);
        //GPIO_ResetBits((GPIO_TypeDef *)pinfo->gpio_base, (INT16U)(1 << pinfo->pin_tx));
    }
    
    /* Configure USARTx_Rx as input floating */
    gpio_initstruct.GPIO_Pin   = (INT16U)(1 << pinfo->pin_rx);
    gpio_initstruct.GPIO_Speed = GPIO_Speed_50MHz;
    //gpio_initstruct.GPIO_OType = GPIO_OType_PP;
    //gpio_initstruct.GPIO_PuPd  = GPIO_PuPd_UP;
    if (onoff) {
        gpio_initstruct.GPIO_Mode  = GPIO_Mode_IPU;
        GPIO_Init((GPIO_TypeDef *)pinfo->gpio_base, &gpio_initstruct);
        //GPIO_PinAFConfig((GPIO_TypeDef *)pinfo->gpio_base, pinfo->pin_rx, GPIO_AF_4);
    } else {
        gpio_initstruct.GPIO_Mode  = GPIO_Mode_AIN;
        GPIO_Init((GPIO_TypeDef *)pinfo->gpio_base, &gpio_initstruct);
        //GPIO_ResetBits((GPIO_TypeDef *)pinfo->gpio_base, (INT16U)(1 << pinfo->pin_rx));
    }

     

    //#if 0

    //#endif

}

/*******************************************************************
** 函数名称: STM32_CAN_IrqConfig
** 函数描述: 配置中断功能
** 参数:     [in] pinfo: 配置参数
**           [in] inter: 中断配置
** 返回:     无
********************************************************************/
static void STM32_CAN_IrqConfig(const CAN_REG_T *pinfo)
{
    INT32S irq_id;
    INT32U flag;
    IRQ_SERVICE_FUNC irqhandler;
    
    if (pinfo == 0) {
        return;
    }
    
    switch (pinfo->can_base)
    {
    case CAN1_BASE:
        irq_id = CAN1_RX0_IRQn;
        irqhandler = (IRQ_SERVICE_FUNC)CAN1_Rx_IrqHandle;
        ST_IRQ_ConfigIrqEnable(irq_id, false);                                      /* 关闭中断 */
        ST_IRQ_InstallIrqHandler(irq_id, irqhandler);                               /* install irq handle */
        ST_IRQ_ConfigIrqPriority(irq_id, IRQ_PRIOTITY_CAN);
        ST_IRQ_ConfigIrqEnable(irq_id, true);                                       /* 打开中断 */

        irq_id = CAN1_RX1_IRQn;
        irqhandler = (IRQ_SERVICE_FUNC)CAN1_Rx1_IrqHandle;
        ST_IRQ_ConfigIrqEnable(irq_id, false);                                      /* 关闭中断 */
        ST_IRQ_InstallIrqHandler(irq_id, irqhandler);                               /* install irq handle */
        ST_IRQ_ConfigIrqPriority(irq_id, IRQ_PRIOTITY_CAN);
        ST_IRQ_ConfigIrqEnable(irq_id, true); 

        irq_id = CAN1_TX_IRQn;
        irqhandler = (IRQ_SERVICE_FUNC)CAN1_Tx_IrqHandle;
        ST_IRQ_ConfigIrqEnable(irq_id, false);                                      /* 关闭中断 */
        ST_IRQ_InstallIrqHandler(irq_id, irqhandler);                               /* install irq handle */
        ST_IRQ_ConfigIrqPriority(irq_id, IRQ_PRIOTITY_CAN);
        ST_IRQ_ConfigIrqEnable(irq_id, true);  
    	break;

    case CAN2_BASE: 
        irq_id = CAN2_RX0_IRQn;
        irqhandler = (IRQ_SERVICE_FUNC)CAN2_Rx_IrqHandle;
        ST_IRQ_ConfigIrqEnable(irq_id, false);                                      /* 关闭中断 */
        ST_IRQ_InstallIrqHandler(irq_id, irqhandler);                               /* install irq handle */
        ST_IRQ_ConfigIrqPriority(irq_id, IRQ_PRIOTITY_CAN);
        ST_IRQ_ConfigIrqEnable(irq_id, true);                                       /* 打开中断 */

        irq_id = CAN2_RX1_IRQn;
        irqhandler = (IRQ_SERVICE_FUNC)CAN2_Rx1_IrqHandle;
        ST_IRQ_ConfigIrqEnable(irq_id, false);                                      /* 关闭中断 */
        ST_IRQ_InstallIrqHandler(irq_id, irqhandler);                               /* install irq handle */
        ST_IRQ_ConfigIrqPriority(irq_id, IRQ_PRIOTITY_CAN);
        ST_IRQ_ConfigIrqEnable(irq_id, true);                                       /* 打开中断 */

        irq_id = CAN2_TX_IRQn;
        irqhandler = (IRQ_SERVICE_FUNC)CAN2_Tx_IrqHandle;
        ST_IRQ_ConfigIrqEnable(irq_id, false);                                      /* 关闭中断 */
        ST_IRQ_InstallIrqHandler(irq_id, irqhandler);                               /* install irq handle */
        ST_IRQ_ConfigIrqPriority(irq_id, IRQ_PRIOTITY_CAN);
        ST_IRQ_ConfigIrqEnable(irq_id, true);  
    	break;
    default:
        return;
    }
    
   
    
    flag = CAN_IT_FF0 | CAN_IT_FF1 | CAN_IT_FMP0 | CAN_IT_FMP1;// | CAN_IT_BOF | CAN_IT_ERR;
    //flag = CAN_IT_FMP0;
    CAN_ITConfig((CAN_TypeDef *)pinfo->can_base, flag, ENABLE);                 /* Enable Interrupt */
}


/*******************************************************************
** 函数名称: STM32_CAN_Init
** 函数描述: 串口通信参数配置，并设置是否支持DMA
** 参数:     [in]  cfg:串口配置参数 
** 返回:     无
********************************************************************/
static BOOLEAN STM32_CAN_Init(CAN_CFG_T *cfg)
{
    INT8U com, result = false;
    INT32U baud;
    CAN_InitTypeDef can_initstruct;
    const CAN_REG_T *pinfo;
	  
    com = cfg->com;
    pinfo = HAL_CAN_GetRegTblInfo(com);

    /* can clock enable */
    RCC_APB1PeriphClockCmd(pinfo->can_rcc, ENABLE);
    
    /* deinit can */
	//CAN_DeInit((CAN_TypeDef *)pinfo->can_base);

	
	/* CAN cell init */
	CAN_StructInit(&can_initstruct);
    can_initstruct.CAN_TTCM = DISABLE;                                         /* time triggered communication mode */
    can_initstruct.CAN_ABOM = ENABLE;                                          /* automatic bus-off management */
    can_initstruct.CAN_AWUM = ENABLE;                                          /* automatic wake-up mode */
    can_initstruct.CAN_NART = ENABLE;                                          /* non-automatic retransmission mode */
    can_initstruct.CAN_RFLM = DISABLE;                                         /* Receive FIFO Locked mode */
    can_initstruct.CAN_TXFP = DISABLE;                                         /* transmit FIFO priority */
    switch (cfg->mode)                                                         /* the CAN operating mode */
    {
    case CAN_WORK_MODE_NORMAL:
        can_initstruct.CAN_Mode = CAN_Mode_Normal;
        break;
    case CAN_WORK_MODE_LOOPBACK:
        can_initstruct.CAN_Mode = CAN_Mode_LoopBack;
        break;
    case CAN_WORK_MODE_SILENT:
        can_initstruct.CAN_Mode = CAN_Mode_Silent;
        break;
    case CAN_WORK_MODE_SILENT_LOOPBACK:
        can_initstruct.CAN_Mode = CAN_Mode_Silent_LoopBack;
        break;
    default:
        can_initstruct.CAN_Mode = CAN_Mode_Normal;
        break;
    }
    
    /* CAN Baudrate = 1MBps (CAN clocked at 48 MHz) */
    baud = cfg->baud;
   // can_initstruct.CAN_SJW = CAN_SJW_1tq;                                      /* bit to perform resynchronization */
   // can_initstruct.CAN_BS1 = CAN_BS1_15tq;                                     /* Segment 1 */
   // can_initstruct.CAN_BS2 = CAN_BS2_8tq;                                      /* Segment 2 */
   // can_initstruct.CAN_Prescaler = 2 * (1000000 / baud);                       /* Specifies the length of a time quantum. It ranges from 1 to 1024. */

    #if 0
    can_initstruct.CAN_SJW = CAN_SJW_1tq;                                      /* bit to perform resynchronization */
    can_initstruct.CAN_BS1 = CAN_BS1_3tq;                                     /* Segment 1 */
    can_initstruct.CAN_BS2 = CAN_BS2_2tq;                                     /* Segment 2 */
    can_initstruct.CAN_Prescaler = (36000000 / (baud * (1 + 3 + 2)));       /* Specifies the length of a time quantum. It ranges from 1 to 1024. */
	#endif

    /* 采样点(1 + BS1)/(1 + BS1 + BS2) */
    /* 网上说法: <=500K 87.5% >500K  80% >800K  75% */
    /* 此处程序参照老m3 BS1 BS2*/
    
    if(baud <= 250000) {
        can_initstruct.CAN_SJW = CAN_SJW_2tq;                                       /* bit to perform resynchronization */
        can_initstruct.CAN_BS1 = CAN_BS1_13tq;                                      /* Segment 1 */
        can_initstruct.CAN_BS2 = CAN_BS2_2tq;                                       /* Segment 2 */
        can_initstruct.CAN_Prescaler = (36000000 / (baud * (1 + 13 + 2)));          /* Specifies the length of a time quantum. It ranges from 1 to 1024. */
    }else if(baud <= 800000) {
        can_initstruct.CAN_SJW = CAN_SJW_1tq;
        can_initstruct.CAN_BS1 = CAN_BS1_7tq;
        can_initstruct.CAN_BS2 = CAN_BS2_4tq;
        can_initstruct.CAN_Prescaler = (36000000 / (baud * (1 + 7 + 4)));
    }else if(baud <= 1000000) {
         can_initstruct.CAN_SJW = CAN_SJW_1tq;
        can_initstruct.CAN_BS1 = CAN_BS1_5tq; 
        can_initstruct.CAN_BS2 = CAN_BS2_3tq;
        can_initstruct.CAN_Prescaler = (36000000 / (baud * (1 + 5 + 3))); 
    }else {
        can_initstruct.CAN_SJW = CAN_SJW_1tq;
        can_initstruct.CAN_BS1 = CAN_BS1_13tq;
        can_initstruct.CAN_BS2 = CAN_BS2_4tq;
        can_initstruct.CAN_Prescaler = (36000000 / (baud * (1 + 13 + 4)));
    }
    
	/* configure can */
    result = CAN_Init((CAN_TypeDef *)pinfo->can_base, &can_initstruct);
    if (result == CAN_InitStatus_Success) {
        result = TRUE;
        STM32_CAN_IrqConfig(pinfo);                                            /* 配置中断使能 */
    } else {
        result = FALSE;
    }
    return result;
}

/*******************************************************************
** 函数名称: HAL_CAN_InitDrv
** 函数描述: 初始化驱动
** 参数:     无
** 返回:     无
********************************************************************/
void HAL_CAN_InitDrv(void)
{
    YX_MEMSET(&s_can, 0, sizeof(s_can));
}

/*******************************************************************
** 函数名:     HAL_CAN_GetStatus
** 函数描述:   获取CAN总线状态
** 参数:       [in] com:  通道编号,见 CAN_COM_E
**             [in] info: 状态信息,见 CAN_STATUS_T
** 返回:       成功返回true, 失败返回false
********************************************************************/
BOOLEAN HAL_CAN_GetStatus(INT8U com, CAN_STATUS_T *info)
{
    const CAN_REG_T *pinfo;
    
    if (com >= CAN_COM_MAX) {
        return false;
    }
    
    pinfo = HAL_CAN_GetRegTblInfo(com);
    
    info->status = HAL_CAN_IsOpened(com);
    info->lec    = CAN_GetLastErrorCode((CAN_TypeDef *)pinfo->can_base) >> 4;
    info->rec    = CAN_GetReceiveErrorCounter((CAN_TypeDef *)pinfo->can_base);
    info->tec    = CAN_GetLSBTransmitErrorCounter((CAN_TypeDef *)pinfo->can_base);
    
    if (CAN_GetFlagStatus((CAN_TypeDef *)pinfo->can_base, CAN_FLAG_BOF) == SET) {/* 总线关闭 */
        info->errorstep = CAN_ERROR_STEP_BUSOFF;
    } else if (CAN_GetFlagStatus((CAN_TypeDef *)pinfo->can_base, CAN_FLAG_EPV) == SET) {/* 被动错误 */
        info->errorstep = CAN_ERROR_STEP_PASSIVE;
    } else if (CAN_GetFlagStatus((CAN_TypeDef *)pinfo->can_base, CAN_FLAG_EWG) == SET) {/* 错误警告 */
        info->errorstep = CAN_ERROR_STEP_WARNING;
    } else if (info->rec > 0 && info->tec > 0) {                               /* 主动错误 */
        info->errorstep = CAN_ERROR_STEP_ACTIVE;
    } else {
        info->errorstep = CAN_ERROR_STEP_NOERR;
    }
    
    return true;
}

/*******************************************************************
** 函数名称:   HAL_CAN_IsOpened
** 函数描述:   通道是否已打开
** 参数:       [in] com: 通道编号,见CAN_COM_E
** 返回:       是返回true,否返回false
********************************************************************/
BOOLEAN HAL_CAN_IsOpened(INT8U com)
{
    if (com >= CAN_COM_MAX) {
        return false;
    }
    
    if ((s_can[com].status &_OPEN) != 0) {
        return true;
    } else {
        return false;
    }
}

/*******************************************************************
** 函数名称:   HAL_CAN_OpenCan
** 函数描述:   打开CAN通信总线
** 参数:       [in]  cfg: 配置参数 
** 返回:       成功返回true,失败返回false
********************************************************************/
BOOLEAN HAL_CAN_OpenCan(CAN_CFG_T *cfg)
{
    INT8U com;
    const CAN_REG_T *pinfo;
    
    OS_ASSERT((cfg != 0), RETURN_FALSE);
    //OS_ASSERT((cfg->com < CAN_COM_MAX), RETURN_FALSE);
    if (cfg->com >= CAN_COM_MAX) {
        return false;
    }
    
    com = cfg->com;
    pinfo = HAL_CAN_GetRegTblInfo(com);
    if (pinfo->enable == 0) {
        return false;
    }
    
    if ((s_can[com].status & _OPEN) != 0) {                                    /* 已打开 */
        s_can[com].status &= ~(_OPEN | _SENDING);
        
	    CAN_DeInit((CAN_TypeDef *)pinfo->can_base);                            /* deinit can */
    }
    
    s_can[com].recvloop.pmsg = &s_can_recv[com][0];                            /* 接收缓存 */
    s_can[com].recvloop.used = 0;
    s_can[com].recvloop.pos  = 0;
    s_can[com].recvloop.max  = MAX_CAN_RECV;
    
    s_can[com].sendloop.pmsg = &s_can_send[com][0];                            /* 发送缓存 */
    s_can[com].sendloop.used = 0;
    s_can[com].sendloop.pos  = 0;
    s_can[com].sendloop.max  = MAX_CAN_SEND;
    
    STM32_CAN_PinsConfig(pinfo, TRUE);                                         /* configure the rx & tx pins */
    if (STM32_CAN_Init(cfg)) {                                                 /* 通信参数配置 */
        s_can[com].status = _OPEN;
        return true;
    } else {
        return false;
    }
}

/*******************************************************************
** 函数名:     HAL_CAN_CloseCan
** 函数描述:   关闭CAN总线
** 参数:       [in] com: 通道编号,见CAN_COM_E
** 返回:       成功返回true, 失败返回false
********************************************************************/
BOOLEAN HAL_CAN_CloseCan(INT8U com)
{
    const CAN_REG_T *pinfo;
    
    if (com >= CAN_COM_MAX) {
        return false;
    }
    
    pinfo = HAL_CAN_GetRegTblInfo(com);
    if (pinfo->enable == 0) {
        return false;
    }
    STM32_CAN_PinsConfig(pinfo, FALSE);
    if ((s_can[com].status & _OPEN) != 0) {                                    /* 串口已打开 */
        CAN_DeInit((CAN_TypeDef *)pinfo->can_base);                            /* deinit can */
    }
    s_can[com].status &= ~(_OPEN | _SENDING);
    
    return true;
}

/*******************************************************************
** 函数名:     HAL_CAN_SetFilterParaByList
** 函数描述:   设置滤波参数,设置滤波ID列表过滤方式
** 参数:       [in] com:       通道编号,见 CAN_COM_E
**             [in] idtype:    帧ID类型,见 CAN_ID_TYPE_E
**             [in] idnum:     滤波ID个数,为0时表示接收所有ID
**             [in] pfilterid: 滤波ID数组
** 返回:       成功返回true, 失败返回false
********************************************************************/
BOOLEAN HAL_CAN_SetFilterParaByList(INT8U com, INT8U idtype, INT8U idnum, INT32U *pfilterid)
{
    INT8U i;
    INT8U ch_offset; 
    INT32U filterid;
    CAN_FilterInitTypeDef  can_filterinitstructure;
    const CAN_REG_T *pinfo;
    
    if (com >= CAN_COM_MAX) {
        return false;
    }
    
    if ((s_can[com].status & _OPEN) == 0) {
        return false;
    }

    pinfo = HAL_CAN_GetRegTblInfo(com);
    ch_offset = (com * 14);
    if (idtype == CAN_ID_TYPE_STD) {                                           /* 标准帧 */
        can_filterinitstructure.CAN_FilterMode           = CAN_FilterMode_IdList;
        can_filterinitstructure.CAN_FilterScale          = CAN_FilterScale_16bit;
        can_filterinitstructure.CAN_FilterFIFOAssignment = pinfo->fifo;
        can_filterinitstructure.CAN_FilterActivation     = ENABLE;
        
        for (i = 0; i < idnum && i < MAX_CAN_FILTER_ID_LIST_STD; i++) {
            filterid = pfilterid[i] << 5;
            
            if ((i % 4) == 0) {
                can_filterinitstructure.CAN_FilterIdHigh     = filterid & 0xFFFF;
                can_filterinitstructure.CAN_FilterIdLow      = filterid & 0xFFFF;
                can_filterinitstructure.CAN_FilterMaskIdHigh = filterid & 0xFFFF;
                can_filterinitstructure.CAN_FilterMaskIdLow  = filterid & 0xFFFF;
                can_filterinitstructure.CAN_FilterNumber     = i / 4 + ch_offset;
            } else if ((i % 4) == 1) {
                can_filterinitstructure.CAN_FilterIdLow      = filterid & 0xFFFF;
            } else if ((i % 4) == 2) {
                can_filterinitstructure.CAN_FilterMaskIdHigh = filterid & 0xFFFF;
            } else if ((i % 4) == 3) {
                can_filterinitstructure.CAN_FilterMaskIdLow  = filterid & 0xFFFF;
                can_filterinitstructure.CAN_FilterNumber     = i / 4 + ch_offset;
                CAN_FilterInit(&can_filterinitstructure);
            }
        }
        
        if ((i % 4) != 0) {
            CAN_FilterInit(&can_filterinitstructure);
        }
        
        for (i = (i + 3) / 4; i < MAX_CAN_FILTER_ID_BANK; i++) {               /* 关闭剩余滤波器组 */
            can_filterinitstructure.CAN_FilterActivation = DISABLE;
            can_filterinitstructure.CAN_FilterNumber     = i + ch_offset;
            CAN_FilterInit(&can_filterinitstructure);
        }
    } else {
        can_filterinitstructure.CAN_FilterMode           = CAN_FilterMode_IdList;
        can_filterinitstructure.CAN_FilterScale          = CAN_FilterScale_32bit;
        can_filterinitstructure.CAN_FilterFIFOAssignment = pinfo->fifo;
        can_filterinitstructure.CAN_FilterActivation     = ENABLE;
            
        for (i = 0; i < idnum && i < MAX_CAN_FILTER_ID_LIST_EXT; i++) {
            filterid = (pfilterid[i] << 3) | 0x04;                             /* 0x04表示扩展帧 */
            
            if ((i % 2) == 0) {
                can_filterinitstructure.CAN_FilterIdHigh     = (filterid >> 16) & 0xFFFF;
                can_filterinitstructure.CAN_FilterIdLow      = filterid & 0xFFFF;
                can_filterinitstructure.CAN_FilterMaskIdHigh = (filterid >> 16) & 0xFFFF;
                can_filterinitstructure.CAN_FilterMaskIdLow  = filterid & 0xFFFF;
                can_filterinitstructure.CAN_FilterNumber     = i / 2 + ch_offset;
            } else {
                can_filterinitstructure.CAN_FilterMaskIdHigh = (filterid >> 16) & 0xFFFF;
                can_filterinitstructure.CAN_FilterMaskIdLow  = filterid & 0xFFFF;
                can_filterinitstructure.CAN_FilterNumber     = i / 2 + ch_offset;
                CAN_FilterInit(&can_filterinitstructure);
            }
        }
        
        if ((i % 2) != 0) {
            CAN_FilterInit(&can_filterinitstructure);
        }
        
        for (i = (i + 1) / 2; i < MAX_CAN_FILTER_ID_BANK; i++) {               /* 关闭剩余滤波器组 */
            can_filterinitstructure.CAN_FilterActivation = DISABLE;
            can_filterinitstructure.CAN_FilterNumber     = i + ch_offset;
            CAN_FilterInit(&can_filterinitstructure);
        }
    }
    
    if (idnum == 0) {                                                          /* 滤波ID个数为0,表示不滤波,接收所有数据帧 */
        can_filterinitstructure.CAN_FilterMode       = CAN_FilterMode_IdMask;
        can_filterinitstructure.CAN_FilterActivation = ENABLE;
        can_filterinitstructure.CAN_FilterIdHigh     = 0;
        can_filterinitstructure.CAN_FilterIdLow      = 0;
        can_filterinitstructure.CAN_FilterMaskIdHigh = 0;
        can_filterinitstructure.CAN_FilterMaskIdLow  = 0;
        can_filterinitstructure.CAN_FilterNumber     = 0 + ch_offset;
        CAN_FilterInit(&can_filterinitstructure);
    }
    return true;
}

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
BOOLEAN HAL_CAN_SetFilterParaByMask(INT8U com, INT8U idtype, INT8U idnum, INT32U *pfilterid, INT32U *pmaskid)
{
    INT8U i;
    INT8U ch_offset;                                                           /* 通道偏移 */
    INT32U filterid, maskid;
    CAN_FilterInitTypeDef  can_filterinitstructure;
    const CAN_REG_T *pinfo;
    
    if (com >= CAN_COM_MAX) {
        return false;
    }
    
    if ((s_can[com].status & _OPEN) == 0) {
        return false;
    }

    pinfo = HAL_CAN_GetRegTblInfo(com);
    ch_offset = (com*14);
    if (idtype == CAN_ID_TYPE_STD) {
        can_filterinitstructure.CAN_FilterMode           = CAN_FilterMode_IdMask;
        can_filterinitstructure.CAN_FilterScale          = CAN_FilterScale_16bit;
        can_filterinitstructure.CAN_FilterFIFOAssignment = pinfo->fifo;
        can_filterinitstructure.CAN_FilterActivation     = ENABLE;
        
        for (i = 0; i < idnum && i < MAX_CAN_FILTER_ID_MASK_STD; i++) {
            filterid = pfilterid[i] << 5;
            maskid   = (pmaskid[i] << 5) | 0x08;                               /* 0x08需要掩码标准帧位 */
            
            if ((i % 2) == 0) {
                can_filterinitstructure.CAN_FilterIdHigh     = filterid & 0xFFFF;
                can_filterinitstructure.CAN_FilterMaskIdHigh = maskid & 0xFFFF;
                can_filterinitstructure.CAN_FilterIdLow      = filterid & 0xFFFF;
                can_filterinitstructure.CAN_FilterMaskIdLow  = maskid & 0xFFFF;
                can_filterinitstructure.CAN_FilterNumber     = i / 2 + ch_offset;
            } else {
                can_filterinitstructure.CAN_FilterIdLow      = filterid & 0xFFFF;
                can_filterinitstructure.CAN_FilterMaskIdLow  = maskid & 0xFFFF;
                can_filterinitstructure.CAN_FilterNumber     = i / 2 + ch_offset;
                CAN_FilterInit(&can_filterinitstructure);
            }
        }
        
        if ((i % 2) != 0) {
            CAN_FilterInit(&can_filterinitstructure);
        }
        
        for (i = (i + 1) / 2; i < MAX_CAN_FILTER_ID_BANK; i++) {               /* 关闭剩余滤波器组 */
            can_filterinitstructure.CAN_FilterActivation = DISABLE;
            can_filterinitstructure.CAN_FilterNumber     = i + ch_offset;
            CAN_FilterInit(&can_filterinitstructure);
        }
    } else {
        can_filterinitstructure.CAN_FilterMode           = CAN_FilterMode_IdMask;
        can_filterinitstructure.CAN_FilterScale          = CAN_FilterScale_32bit;
        can_filterinitstructure.CAN_FilterFIFOAssignment = pinfo->fifo;
        can_filterinitstructure.CAN_FilterActivation     = ENABLE;
            
        for (i = 0; i < idnum && i < MAX_CAN_FILTER_ID_MASK_EXT; i++) {
            filterid = (pfilterid[i] << 3) | 0x04;                             /* 0x04表示扩展帧 */
            maskid   = (pmaskid[i] << 3) | 0x04;
            
            can_filterinitstructure.CAN_FilterIdHigh     = (filterid >> 16) & 0xFFFF;
            can_filterinitstructure.CAN_FilterIdLow      = filterid & 0xFFFF;
            can_filterinitstructure.CAN_FilterMaskIdHigh = (maskid >> 16) & 0xFFFF;
            can_filterinitstructure.CAN_FilterMaskIdLow  = maskid & 0xFFFF;
            can_filterinitstructure.CAN_FilterNumber     = i + ch_offset;
            CAN_FilterInit(&can_filterinitstructure);
        }
        
        for (; i < MAX_CAN_FILTER_ID_BANK; i++) {                              /* 关闭剩余滤波器组 */
            can_filterinitstructure.CAN_FilterActivation = DISABLE;
            can_filterinitstructure.CAN_FilterNumber     = i + ch_offset;
            CAN_FilterInit(&can_filterinitstructure);
        }
    }
    
    if (idnum == 0) {                                                          /* 滤波ID个数为0,表示不滤波,接收所有数据帧 */
        can_filterinitstructure.CAN_FilterMode       = CAN_FilterMode_IdMask;
        can_filterinitstructure.CAN_FilterActivation = ENABLE;
        can_filterinitstructure.CAN_FilterIdHigh     = 0;
        can_filterinitstructure.CAN_FilterIdLow      = 0;
        can_filterinitstructure.CAN_FilterMaskIdHigh = 0;
        can_filterinitstructure.CAN_FilterMaskIdLow  = 0;
        can_filterinitstructure.CAN_FilterNumber     = 0 + ch_offset;
        CAN_FilterInit(&can_filterinitstructure);
    }
    return true;
}

/*******************************************************************
** 函数名称: HAL_CAN_ReadData
** 函数描述: 读取一帧数据
** 参数:     [in]  com:  通道编号,见CAN_COM_E
**           [out] data: 数据帧
** 返回:     成功返回数据，失败返回-1
********************************************************************/
BOOLEAN HAL_CAN_ReadData(INT8U com, CAN_DATA_T *data)
{
    if (com >= CAN_COM_MAX) {
        return false;
    }
    
    if ((s_can[com].status & _OPEN) == 0) {
        return false;
    }
    
    return ReadLoopData(&s_can[com].recvloop, data);
}

/*******************************************************************
** 函数名称: HAL_CAN_SendData
** 函数描述: 发送一帧数据
** 参数:     [in] com:  通道编号,见CAN_COM_E
**           [in] data: 数据帧
** 返回:     成功返回true,失败返回false
********************************************************************/
static BOOLEAN StartCanSend(const CAN_REG_T *pinfo, CAN_DATA_T *data);
BOOLEAN HAL_CAN_SendData(INT8U com, CAN_DATA_T *data)
{
    INT8U result;
    volatile INT32U cpu_sr;
    const CAN_REG_T *pinfo;
    CAN_DATA_T candata;

    if (com >= CAN_COM_MAX) {
        return FALSE;
    }
    
    if ((s_can[com].status & _OPEN) == 0) {
        return FALSE;
    }
    
    result = WriteLoopData(&s_can[com].sendloop, data);
    pinfo = HAL_CAN_GetRegTblInfo(com);
    
    OS_ENTER_CRITICAL();
    if ((s_can[com].status & _SENDING) == 0) {
        OS_EXIT_CRITICAL();
        
        CAN_ITConfig((CAN_TypeDef *)pinfo->can_base, CAN_IT_TME, DISABLE);     /* 关闭发送中断 */
        
        CAN_CancelTransmit((CAN_TypeDef *)pinfo->can_base, 0);
        CAN_CancelTransmit((CAN_TypeDef *)pinfo->can_base, 1);
        CAN_CancelTransmit((CAN_TypeDef *)pinfo->can_base, 2);
        
        ReadLoopData_INT(&s_can[com].sendloop, &candata);
        if (StartCanSend(pinfo, &candata)) {
            s_can[com].status |= _SENDING;
            CAN_ITConfig((CAN_TypeDef *)pinfo->can_base, CAN_IT_TME, ENABLE);  /* 打开发送中断 */
        }
    } else {
        OS_EXIT_CRITICAL();
    }
    
    return result;
}

/*******************************************************************
** 函数名称: HAL_CAN_UsedOfRecvbuf
** 函数描述: 获取已接收帧数
** 参数:     [in] com: 通道编号,见CAN_COM_E
** 返回:     已接收数据帧数
********************************************************************/
INT32U HAL_CAN_UsedOfRecvbuf(INT8U com)
{
    if (com >= CAN_COM_MAX) {
        return 0;
    }
    
    if ((s_can[com].status & _OPEN) == 0) {
        return 0;
    }
    
    return UsedOfLoopBuffer(&s_can[com].recvloop);
}

/*******************************************************************
** 函数名称: HAL_CAN_LeftOfSendbuf
** 函数描述: 获取发送缓存剩余空间
** 参数:     [in] com: 通道编号,见CAN_COM_E
** 返回:     剩余数据帧数
********************************************************************/
INT32U HAL_CAN_LeftOfSendbuf(INT8U com)
{
    if (com >= CAN_COM_MAX) {
        return 0;
    }
    
    if ((s_can[com].status & _OPEN) == 0) {
        return 0;
    }
    
    return LeftOfLoopBuffer(&s_can[com].sendloop);
}

/*******************************************************************
** 函数名称:   HAL_CAN_WriteLoopData
** 函数描述:   往循环缓冲区中写入一帧数据
** 参数:       [in]  loop:   循环缓冲区管理结构体
**             [in]  data:   写入数据
** 返回:       成功返回true,失败返回false
********************************************************************/
BOOLEAN HAL_CAN_WriteLoopData(CAN_DATA_Q_T *loop, CAN_DATA_T *data)
{
   return WriteLoopData_INT(loop, data);
}

/*******************************************************************
** 函数名称:   ReadLoopData_INT
** 函数描述:   从循环缓冲区中读取一帧数据
** 参数:       [in]  loop:    循环缓冲区管理结构体
**             [out] data:    读取数据
** 返回:       成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN HAL_CAN_ReadLoopData(CAN_DATA_Q_T *loop, CAN_DATA_T *data)
{
   return ReadLoopData_INT(loop, data);
}

/*******************************************************************
** 函数名称:   HAL_CAN_UsedOfLoopBuffer_INT
** 函数描述:   获取循环缓冲区中已使用空间
** 参数:       [in]  loop:    循环缓冲区管理结构体
** 返回:       已使用空间字节数
********************************************************************/
INT32U HAL_CAN_UsedOfLoopBuffer(CAN_DATA_Q_T *loop)
{
    return UsedOfLoopBuffer_INT(loop);
}

/*******************************************************************
** 函数名称:   ReadLoopData_INT
** 函数描述:   从循环缓冲区中读取一帧数据
** 参数:       [in]  loop:    循环缓冲区管理结构体
**             [out] data:    读取数据
** 返回:       成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN HAL_CAN_InitLoopData(CAN_DATA_Q_T *loop, CAN_DATA_T * initloop, INT8U cnt)
{
    loop->pmsg = initloop;                         
    loop->used = 0;
    loop->pos  = 0;
    loop->max  = cnt;

    return TRUE;
}


/*******************************************************************
** 函数名称: CAN1_IrqHandle
** 函数描述: CAN中断处理
** 参数:     无
** 返回:     无
********************************************************************/
static BOOLEAN StartCanSend(const CAN_REG_T *pinfo, CAN_DATA_T *data)
{
    INT8U mailbox;
    CanTxMsg txmessage;
    
    if (data->idtype == CAN_ID_TYPE_STD) {                                 /* 标准ID */
        txmessage.IDE   = CAN_ID_STD;
        txmessage.StdId = data->id;
        txmessage.ExtId = 0x00;
    } else {
        txmessage.IDE   = CAN_ID_EXT;
        txmessage.StdId = 0x00;
        txmessage.ExtId = data->id;
    }
    
    if (data->datatype == CAN_RTR_TYPE_DATA) {                             /* 数据帧 */
        txmessage.RTR = CAN_RTR_DATA;
    } else {
        txmessage.RTR = CAN_RTR_REMOTE;
    }
    
    txmessage.DLC = data->dlc;
    YX_MEMCPY(txmessage.Data, 8, data->data, 8);
        
    mailbox = CAN_Transmit((CAN_TypeDef *)pinfo->can_base, &txmessage);
    if (mailbox == CAN_TxStatus_NoMailBox) {
        CAN_CancelTransmit((CAN_TypeDef *)pinfo->can_base, 0);
        CAN_CancelTransmit((CAN_TypeDef *)pinfo->can_base, 1);
        CAN_CancelTransmit((CAN_TypeDef *)pinfo->can_base, 2);
        return false;
    } else {
        return true;
    }
}


__attribute__ ((section ("IRQ_HANDLE"))) void CAN_Recive_IRQ(CAN_TypeDef* CANx, INT8U FIFONumber, INT32U flag)
{
    INT8U i, j, com;
    CanRxMsg rxmessage;
    CAN_DATA_T recvdata;
    
    com = CAN_COM_0;
    switch((INT32U)CANx) {
        case CAN1_BASE:
            com = CAN_COM_0;
            break;
        case CAN2_BASE:
            com = CAN_COM_1;
            break;

            default:
                return;
    }
    if (CAN_GetFlagStatus((CAN_TypeDef *)CANx, flag) != RESET) {                       /* FIFO0数据接收中断 */
        CAN_ClearITPendingBit(CANx, flag);                               /* Clears the CAN1 interrupt pending bit */
       
        for (i = 0; i < 1; i++) {
            CAN_Receive((CAN_TypeDef *)CANx, FIFONumber, &rxmessage);
            
            if (rxmessage.IDE == CAN_ID_STD) {
                recvdata.idtype = CAN_ID_TYPE_STD;
                recvdata.id     = rxmessage.StdId;
            } else {
                recvdata.idtype = CAN_ID_TYPE_EXT;
                recvdata.id     = rxmessage.ExtId;
            }
            
            if (rxmessage.RTR == CAN_RTR_DATA) {
                recvdata.datatype = CAN_RTR_TYPE_DATA;
            } else {
                recvdata.datatype = CAN_RTR_TYPE_REMOTE;
            }
            
            recvdata.index = rxmessage.FMI;
            recvdata.dlc   = rxmessage.DLC;
            
            for (j = 0; j < rxmessage.DLC; j++) {
                recvdata.data[j] = rxmessage.Data[j];
            }
            
            WriteLoopData_INT(&s_can[com].recvloop, &recvdata);
            
            #if DEBUG_CAN > 0
            printf_com("CAN%d_IrqHandle(ID:%08x)(idtype:%d)(datatype:%d)(FIFONumber:%d)\r\n",com, recvdata.id
                                                                                            ,recvdata.idtype
                                                                                            , recvdata.datatype
                                                                                            , FIFONumber);
            printf_hex(recvdata.data, recvdata.dlc);
            printf_com("\r\n");
            #endif
        }
    }
}



__attribute__ ((section ("IRQ_HANDLE"))) void CAN1_Rx_IrqHandle(void)
{

   CAN_Recive_IRQ(CAN1, CAN_FIFO0, CAN_FLAG_FMP0);
}

__attribute__ ((section ("IRQ_HANDLE"))) void CAN1_Rx1_IrqHandle(void)
{

    CAN_Recive_IRQ(CAN1, CAN_FIFO1, CAN_FLAG_FMP1);
    
}

__attribute__ ((section ("IRQ_HANDLE"))) void CAN1_Tx_IrqHandle(void)
{
    INT8U  used, com;
    CAN_DATA_T recvdata;
    const CAN_REG_T *pinfo;

    com = CAN_COM_0;
    if (CAN_GetITStatus(CAN1, CAN_IT_TME) != RESET) {                           /* 数据发送中断 */
        CAN_ClearITPendingBit(CAN1, CAN_IT_TME);                                /* Clears the CAN1 interrupt pending bit */
        
        pinfo = HAL_CAN_GetRegTblInfo(com);
        if (((s_can[com].status & _OPEN) == 0) || ((s_can[com].status & _SENDING) == 0)) {
            CAN_ITConfig((CAN_TypeDef *)pinfo->can_base, CAN_IT_TME, DISABLE); /* 关闭发送中断 */
        } else {
            used = UsedOfLoopBuffer_INT(&s_can[com].sendloop);
            if (used == 0) {                                                   /* 无数据要发送 */
                s_can[com].status &= (~_SENDING);
                CAN_ITConfig((CAN_TypeDef *)pinfo->can_base, CAN_IT_TME, DISABLE); /* 关闭发送中断 */
            } else {
                ReadLoopData_INT(&s_can[com].sendloop, &recvdata);
                StartCanSend(pinfo, &recvdata);
            }
        }
    }
}

__attribute__ ((section ("IRQ_HANDLE"))) void CAN2_Rx_IrqHandle(void)
{

    CAN_Recive_IRQ(CAN2, CAN_FIFO0, CAN_FLAG_FMP0);

}

__attribute__ ((section ("IRQ_HANDLE"))) void CAN2_Rx1_IrqHandle(void)
{

    CAN_Recive_IRQ(CAN2, CAN_FIFO1, CAN_FLAG_FMP1);

}


__attribute__ ((section ("IRQ_HANDLE"))) void CAN2_Tx_IrqHandle(void)
{
    INT8U  used, com;
    CAN_DATA_T recvdata;
    const CAN_REG_T *pinfo;

    com = CAN_COM_1;
    if (CAN_GetITStatus(CAN2, CAN_IT_TME) != RESET) {                           /* 数据发送中断 */
        CAN_ClearITPendingBit(CAN2, CAN_IT_TME);                                /* Clears the CAN1 interrupt pending bit */
        
        pinfo = HAL_CAN_GetRegTblInfo(com);
        if (((s_can[com].status & _OPEN) == 0) || ((s_can[com].status & _SENDING) == 0)) {
            CAN_ITConfig((CAN_TypeDef *)pinfo->can_base, CAN_IT_TME, DISABLE); /* 关闭发送中断 */
        } else {
            used = UsedOfLoopBuffer_INT(&s_can[com].sendloop);
            if (used == 0) {                                                   /* 无数据要发送 */
                s_can[com].status &= (~_SENDING);
                CAN_ITConfig((CAN_TypeDef *)pinfo->can_base, CAN_IT_TME, DISABLE); /* 关闭发送中断 */
            } else {
                ReadLoopData_INT(&s_can[com].sendloop, &recvdata);
                StartCanSend(pinfo, &recvdata);
            }
        }
    }
}


#endif


