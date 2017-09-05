/********************************************************************************
**
** 文件名:     st_exti_drv.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现外部中断配置管理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2014/04/09 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "hal_include.h"
#include "stm32f10x.h"
#include "st_gpio_drv.h"
#include "st_irq_drv.h"
#include "st_exti_drv.h"
#include "yx_debug.h"



/*
********************************************************************************
* 定义配置参数
********************************************************************************
*/
typedef enum{
    EXTI_SOURCE_LINE0,
    EXTI_SOURCE_LINE1,
    EXTI_SOURCE_LINE2,
    EXTI_SOURCE_LINE3,
    EXTI_SOURCE_LINE4,
    EXTI_SOURCE_LINE5,
    EXTI_SOURCE_LINE6,
    EXTI_SOURCE_LINE7,
    EXTI_SOURCE_LINE8,
    EXTI_SOURCE_LINE9,
    EXTI_SOURCE_LINE10,
    EXTI_SOURCE_LINE11,
    EXTI_SOURCE_LINE12,
    EXTI_SOURCE_LINE13,
    EXTI_SOURCE_LINE14,
    EXTI_SOURCE_LINE15,
    MAX_EXTI_LINE
} EXTI_SOURCE_LINE_E;

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    void (*callback)(void);
} DCB_T;

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static DCB_T s_exti[MAX_EXTI_LINE];

/*******************************************************************
** 函数名称: EXTI_GetPinNumByBit
** 函数描述: 根据位位置获取编号
** 参数:     [in] pinfo:  配置表
** 返回:     返回管脚编号,从0开始
********************************************************************/
static INT8U EXTI_GetPinNumByBit(const GPIO_REG_T *pinfo)
{
    INT8U i;
    
    for (i = 0; i < 32; i++) {
        if ((pinfo->pin & (1 << i)) != 0) {
            break;
        }
    }
    return i;
}

/*******************************************************************
** 函数名称: EXTI_PinsConfig
** 函数描述: 管脚配置
** 参数:     [in] pinfo:  配置表
**           [in] pclass: 配置表
** 返回:     无
********************************************************************/
static void EXTI_PinsConfig(const GPIO_REG_T *pinfo, const GPIO_CLASS_T *pclass)
{
    GPIO_InitTypeDef gpio_initstruct;
    
    if (pinfo == 0) {
        return;
    }
    
    //RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);                             /* 开启系统时钟 */
    
    /* Configure gpio SCL */
    gpio_initstruct.GPIO_Pin   = pinfo->pin;
    gpio_initstruct.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_initstruct.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    //gpio_initstruct.GPIO_OType = GPIO_OType_OD;
    //gpio_initstruct.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    
    GPIO_Init((GPIO_TypeDef *)pclass->gpio_base, &gpio_initstruct);
    //GPIO_PinAFConfig((GPIO_TypeDef *)pinfo->gpio_base, pinfo->pin, GPIO_AF_1);
}

/*******************************************************************
** 函数名称: EXTI_ExtiLineConfig
** 函数描述: 中断线配置
** 参数:     [in] pinfo:  配置表
**           [in] pclass: 配置表
** 返回:     无
********************************************************************/
static void EXTI_ExtiLineConfig(const GPIO_REG_T *pinfo, const GPIO_CLASS_T *pclass)
{
    INT8U port;
    
    switch(pclass->gpio_base)
    {
    case GPIOA_BASE:
        port = GPIO_PortSourceGPIOA;
        break;
    case GPIOB_BASE:
        port = GPIO_PortSourceGPIOB;
        break;
    case GPIOC_BASE:
        port = GPIO_PortSourceGPIOC;
        break;
    case GPIOD_BASE:
        port = GPIO_PortSourceGPIOD;
        break;
    case GPIOF_BASE:
        port = GPIO_PortSourceGPIOF;
        break;
    default:
        //OS_ASSERT(0, RETURN_VOID);
        //break;
        return;
    }
    
    GPIO_EXTILineConfig(port, EXTI_GetPinNumByBit(pinfo));
}

/*******************************************************************
** 函数名称: EXTI_ExtiModeConfig
** 函数描述: 中断模式配置
** 参数:     [in] pinfo:  配置表
**           [in] pclass: 配置表
**           [in] trig:   边沿触发模式EXTI_TRIG_E
** 返回:     无
********************************************************************/
static void EXTI_ExtiModeConfig(const GPIO_REG_T *pinfo, const GPIO_CLASS_T *pclass, INT8U trig)
{
    EXTI_InitTypeDef     exti_initstructure;
    
    exti_initstructure.EXTI_Line = pinfo->pin;                                 /*!< Specifies the EXTI lines to be enabled or disabled.This parameter can be any combination of @ref EXTI_Lines */
    exti_initstructure.EXTI_Mode = EXTI_Mode_Interrupt;                        /*!< Specifies the mode for the EXTI lines.This parameter can be a value of @ref EXTIMode_TypeDef */
    if (trig == EXTI_TRIG_RAIS) {                                              /*!< Specifies the trigger signal active edge for the EXTI lines. */
        exti_initstructure.EXTI_Trigger = EXTI_Trigger_Rising;
    } else if (trig == EXTI_TRIG_FALL) {
        exti_initstructure.EXTI_Trigger = EXTI_Trigger_Falling;
    } else {
        exti_initstructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    }
    exti_initstructure.EXTI_LineCmd = ENABLE;                                  /*!< Specifies the new state of the selected EXTI lines.This parameter can be set either to ENABLE or DISABLE */
    
    EXTI_Init(&exti_initstructure);
}

/*******************************************************************
** 函数名称: EXTI_IrqConfig
** 函数描述: 配置中断功能
** 参数:     [in] pinfo:  配置表
**           [in] pclass: 配置表
**           [in] enable: 中断使能
** 返回:     无
********************************************************************/
static void EXTI_IrqConfig(const GPIO_REG_T *pinfo, const GPIO_CLASS_T *pclass, INT8U enable)
{
    INT32S irq_id;
    IRQ_SERVICE_FUNC irqhandler;
    
    if (pinfo->pin == GPIO_Pin_0) {
        irq_id = EXTI0_IRQn;
        irqhandler = (IRQ_SERVICE_FUNC)EXTI0_IrqHandle;
    } else if (pinfo->pin == GPIO_Pin_1) {
        irq_id = EXTI1_IRQn;
        irqhandler = (IRQ_SERVICE_FUNC)EXTI1_IrqHandle;
    }else if (pinfo->pin == GPIO_Pin_2) {
        irq_id = EXTI2_IRQn;
        irqhandler = (IRQ_SERVICE_FUNC)EXTI2_IrqHandle;
    } else if (pinfo->pin == GPIO_Pin_3) {
        irq_id = EXTI3_IRQn;
        irqhandler = (IRQ_SERVICE_FUNC)EXTI3_IrqHandle;
    } else if (pinfo->pin == GPIO_Pin_4) {
        irq_id = EXTI4_IRQn;
        irqhandler = (IRQ_SERVICE_FUNC)EXTI4_IrqHandle;
    } else if (pinfo->pin >= GPIO_Pin_10) {
        irq_id = EXTI15_10_IRQn;
        irqhandler = (IRQ_SERVICE_FUNC)EXTI15_10_IrqHandle;
    } else if (pinfo->pin >= GPIO_Pin_5) {
        irq_id = EXTI9_5_IRQn;
        irqhandler = (IRQ_SERVICE_FUNC)EXTI9_5_IrqHandle;
    } else {
        return;
    }
    
    ST_IRQ_ConfigIrqEnable(irq_id, false);                                      /* 关闭中断 */
    ST_IRQ_InstallIrqHandler(irq_id, irqhandler);                               /* install irq handle */
    ST_IRQ_ConfigIrqPriority(irq_id, IRQ_PRIOTITY_EXTI);
    ST_IRQ_ConfigIrqEnable(irq_id, enable);                                     /* 打开中断 */
}

/*******************************************************************
** 函数名称: ST_EXTI_InitDrv
** 函数描述: 初始化模块
** 参数:     无
** 返回:     无
********************************************************************/
void ST_EXTI_InitDrv(void)
{
    YX_MEMSET(&s_exti, 0, sizeof(s_exti));
    
    //SYSCFG_DeInit();
    EXTI_DeInit();
}

/*******************************************************************
** 函数名称: ST_EXTI_OpenExtiFunction
** 函数描述: 打开外部中断功能
** 参数:     [in] pinid:    GPIO统一编号,GPIO_PIN_E
**           [in] trig:     中断边沿触发模式,EXTI_TRIG_E
**           [in] callback: 中断处理函数
** 返回:     成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN ST_EXTI_OpenExtiFunction(INT8U pinid, INT8U trig, void (*callback)(void))
{
    INT8U pin_source;
    const GPIO_REG_T *pinfo;
    const GPIO_CLASS_T *pclass;
    
    if (pinid >= ST_GPIO_GetCfgTblMax()) {
        return false;
    }
    OS_ASSERT(trig < EXTI_TRIG_MAX, RETURN_FALSE);
    
    pinfo = ST_GPIO_GetCfgTblInfo(pinid);
    pclass = DAL_GPIO_GetRegClassInfo(pinfo->port);
    pin_source = EXTI_GetPinNumByBit(pinfo);
    
    if(pin_source >= MAX_EXTI_LINE) {
        return FALSE;
    }
    
    s_exti[pin_source].callback = callback;
    
    /* Enable AFIO clock */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE); 
     
    /* Configure the GPIO ports */
    EXTI_PinsConfig(pinfo, pclass);
    
    /* Connect EXTI Line to  GPIO Pin */
    EXTI_ExtiLineConfig(pinfo, pclass);
    
    /* Configure EXTI Line to generate an interrupt on trig_mode */
    EXTI_ExtiModeConfig(pinfo, pclass, trig);
    
    /* Install irq handle and set irq priotity,and enable the irq channel */
    EXTI_IrqConfig(pinfo, pclass, true);
    return TRUE;
}

/*******************************************************************
** 函数名称: ST_EXTI_CloseExtiFunction
** 函数描述: 关闭外部中断功能
** 参数:     [in] pinid:    GPIO统一编号,GPIO_PIN_E
** 返回:     成功返回TRUE,失败返回FALSE
********************************************************************/
BOOLEAN ST_EXTI_CloseExtiFunction(INT8U pinid)
{
    INT8U pin_source;
    EXTI_InitTypeDef     exti_initstructure;
    const GPIO_REG_T *pinfo;
    const GPIO_CLASS_T *pclass;
    
    if (pinid >= ST_GPIO_GetCfgTblMax()) {
        return false;
    }
    
    pinfo = ST_GPIO_GetCfgTblInfo(pinid);
    pclass = DAL_GPIO_GetRegClassInfo(pinfo->port);
    pin_source = EXTI_GetPinNumByBit(pinfo);
    
    if(pin_source >= MAX_EXTI_LINE) {
        return FALSE;
    }

    exti_initstructure.EXTI_Line = pinfo->pin;
    exti_initstructure.EXTI_LineCmd = DISABLE;
    EXTI_Init(&exti_initstructure);
    //EXTI_DeInit();
    EXTI_IrqConfig(pinfo, pclass, false);
    s_exti[pin_source].callback = 0;
    return true;
}


/*******************************************************************
** 函数名称: EXTI0_1_IrqHandle
** 函数描述: 中断服务程序
** 参数:     无
** 返回:     无
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void EXTI0_IrqHandle(void)
{
    if (EXTI_GetITStatus(EXTI_Line0) != RESET) {
        EXTI_ClearITPendingBit(EXTI_Line0);

        if (s_exti[EXTI_SOURCE_LINE0].callback != 0) {
            s_exti[EXTI_SOURCE_LINE0].callback();
        }
    }
}

/*******************************************************************
** 函数名称: EXTI1_IrqHandle
** 函数描述: 中断服务程序
** 参数:     无
** 返回:     无
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void EXTI1_IrqHandle(void)
{
    if (EXTI_GetITStatus(EXTI_Line1) != RESET) {
        EXTI_ClearITPendingBit(EXTI_Line1);

        if (s_exti[EXTI_SOURCE_LINE1].callback != 0) {
            s_exti[EXTI_SOURCE_LINE1].callback();
        }
    }
}


/*******************************************************************
** 函数名称: EXTI2_IrqHandle
** 函数描述: 中断服务程序
** 参数:     无
** 返回:     无
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void EXTI2_IrqHandle(void)
{
    if (EXTI_GetITStatus(EXTI_Line2) != RESET) {
        EXTI_ClearITPendingBit(EXTI_Line2);

        if (s_exti[EXTI_SOURCE_LINE2].callback != 0) {
            s_exti[EXTI_SOURCE_LINE2].callback();
        }
    }
    
}

/*******************************************************************
** 函数名称: EXTI3_IrqHandle
** 函数描述: 中断服务程序
** 参数:     无
** 返回:     无
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void EXTI3_IrqHandle(void)
{

    
    if (EXTI_GetITStatus(EXTI_Line3) != RESET) {
        EXTI_ClearITPendingBit(EXTI_Line3);
        
        if (s_exti[EXTI_SOURCE_LINE3].callback != 0) {
            s_exti[EXTI_SOURCE_LINE3].callback();
        }
    }
}

/*******************************************************************
** 函数名称: EXTI4_IrqHandle
** 函数描述: 中断服务程序
** 参数:     无
** 返回:     无
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void EXTI4_IrqHandle(void)
{

    
    if (EXTI_GetITStatus(EXTI_Line4) != RESET) {
        EXTI_ClearITPendingBit(EXTI_Line4);
        
        if (s_exti[EXTI_SOURCE_LINE4].callback != 0) {
            s_exti[EXTI_SOURCE_LINE4].callback();
        }
    }
}


/*******************************************************************
** 函数名称: EXTI9_5_IrqHandle
** 函数描述: 中断服务程序
** 参数:     无
** 返回:     无
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void EXTI9_5_IrqHandle(void)
{
    INT32U i;

    for(i = EXTI_SOURCE_LINE5; i <= EXTI_SOURCE_LINE9; i++) {
        if (EXTI_GetITStatus((1 << i)) != RESET) {
            EXTI_ClearITPendingBit((1 << i));
            
            if (s_exti[i].callback != 0) {
                s_exti[i].callback();
            }
        }
    }
}


/*******************************************************************
** 函数名称: EXTI15_10_IrqHandle
** 函数描述: 中断服务程序
** 参数:     无
** 返回:     无
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void EXTI15_10_IrqHandle(void)
{
    INT32U i;

    for(i = EXTI_SOURCE_LINE10; i <= EXTI_SOURCE_LINE15; i++) {
        if (EXTI_GetITStatus((1 << i)) != RESET) {
            EXTI_ClearITPendingBit((1 << i));
            
            if (s_exti[i].callback != 0) {
                s_exti[i].callback();
            }
        }
    }
}



#if 0

/*******************************************************************
** 函数名称: EXTI4_15_IrqHandle
** 函数描述: 中断服务程序
** 参数:     无
** 返回:     无
********************************************************************/
__attribute__ ((section ("IRQ_HANDLE"))) void EXTI4_15_IrqHandle(void)
{
    //if (EXTI_GetITStatus(EXTI_Line2) != RESET) {
        EXTI_ClearITPendingBit((EXTI_Line4 | EXTI_Line5 | EXTI_Line6 | EXTI_Line7 | \
                               EXTI_Line8 | EXTI_Line9 | EXTI_Line10 | EXTI_Line11 | \
                               EXTI_Line12 | EXTI_Line13 | EXTI_Line14 | EXTI_Line15));
    //}
}

#endif




/**************************** (C) COPYRIGHT 2012  XIAMEN YAXON.LTD **************END OF FILE******/

