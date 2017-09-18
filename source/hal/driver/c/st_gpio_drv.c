/******************************************************************************
**
** Filename:     st_gpio_drv.c
** Copyright:    
** Description:  该模块主要实现GPIO驱动管理
**
*******************************************************************************
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#include "hal_include.h"
#include "stm32f0xx.h"
#include "stm32f0xx_conf.h"
#include "st_gpio_reg.h"
#include "st_gpio_drv.h"


#if 0
/*******************************************************************
** 函数名称:  USERGPAB_IRQHandler
** 函数描述:  GPIOA\B中断入口函数
**  输入参数:  CStatus DStatus EStatus
**  输出参数:  无
**  返回参数:  无
********************************************************************/
void USERGPAB_IRQHandler(INT32U AStatus, INT32U BStatus)
{
    INT8U  pin;
    INT32U astatus, bstatus;

    astatus = AStatus;
    bstatus = BStatus;
    
    if (astatus) {
        for (pin = 0; pin < 16; pin++) {
            if (astatus & (0x00000001 << pin)) {
                if (gpioapinthand[pin]) {
                    gpioapinthand[pin]();
                }
            }
        }
    }

    if (bstatus) {
        for (pin = 0; pin < 16; pin++) {
            if (bstatus & (0x00000001 << pin)) {
                if (gpiobpinthand[pin]) {
                    gpiobpinthand[pin]();
                }
            }
        }
    }

}

/*******************************************************************
** 函数名称:  USERGPCDE_IRQHandler
** 函数描述:  GPIOC\D\E中断入口函数
**  输入参数:  CStatus DStatus EStatus
**  输出参数:  无
**  返回参数:  无
********************************************************************/
void USERGPCDE_IRQHandler(INT32U CStatus, INT32U DStatus, INT32U EStatus)
{
    INT8U  pin;
    INT32U cstatus, dstatus, estatus;

    cstatus = CStatus;
    dstatus = DStatus;
    estatus = EStatus;
    
    if (cstatus) {
        for (pin = 0; pin < 16; pin++) {
            if (cstatus & (0x00000001 << pin)) {
                if (gpiocpinthand[pin]) {
                    gpiocpinthand[pin]();
                }
            }
        }
    }

    if (dstatus) {
        for (pin = 0; pin < 16; pin++) {
            if (dstatus & (0x00000001 << pin)) {
                if (gpiodpinthand[pin]) {
                    gpiodpinthand[pin]();
                }
            }
        }
    }

    if (estatus) {
        for (pin = 0; pin < 16; pin++) {
            if (estatus & (0x00000001 << pin)) {
                if (gpioepinthand[pin]) {
                    gpioepinthand[pin]();
                }
            }
        }
    }

    
}

#endif


/*******************************************************************
** 函数名称: ST_GPIO_SetPin
** 函数描述: 初始化管脚状态
** 参数:     [in] id:        GPIO统一编号,GPIO_PIN_E
**           [in] direction: 方向,GPIO_DIR_E
**           [in] pull:      上下拉,GPIO_PULL_E
**           [in] level:     高低电平,TRUE-高电平，FALSE-低电平
** 返回:     成功返回TRUE，失败返回FALSE
********************************************************************/
void ST_GPIO_SetPin(INT8U id, INT8U direction, INT8U pull, INT8U level)
{
    const GPIO_REG_T *pinfo;
    const GPIO_CLASS_T *pclass;
    GPIO_InitTypeDef  GPIO_InitStructure;
    
    if (id >= ST_GPIO_GetCfgTblMax()) {
        return;
    }
    
    pinfo = ST_GPIO_GetCfgTblInfo(id);
    pclass = DAL_GPIO_GetRegClassInfo(pinfo->port);
    
    /* Configure the GPIO pin */
    GPIO_InitStructure.GPIO_Pin   = pinfo->pin;
    if (direction == GPIO_DIR_IN) {                                            /* 方向 */
        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
    } else {
        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
    }
    GPIO_InitStructure.GPIO_OType = (GPIOOType_TypeDef)pinfo->mode;
    
    if (pull == GPIO_PULL_UP) {                                                /* 上下拉 */
        GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    } else if (pull == GPIO_PULL_DOWN) {
        GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;
    } else {
        GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    }
    //GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    
    GPIO_Init((GPIO_TypeDef *)pclass->gpio_base, &GPIO_InitStructure);
    
    if (direction == GPIO_DIR_OUT) {                                           /* 输出电平 */
        ST_GPIO_WritePin(id, level);
    }
}

/*******************************************************************
** 函数名称: ST_GPIO_WritePin
** 函数描述: 读取GPIO管脚状态
** 参数:     [in] id:        GPIO统一编号,GPIO_PIN_E
**           [in] level:     初始化电平,TRUE-高电平，FALSE-低电平
** 返回:     成功返回TRUE，失败返回FALSE
********************************************************************/
BOOLEAN ST_GPIO_WritePin(INT8U id, INT8U level)
{
    const GPIO_REG_T *pinfo;
    const GPIO_CLASS_T *pclass;
    
    if (id >= ST_GPIO_GetCfgTblMax()) {
        return false;
    }
    
    pinfo = ST_GPIO_GetCfgTblInfo(id);
    pclass = DAL_GPIO_GetRegClassInfo(pinfo->port);
    if (level) {
        GPIO_SetBits((GPIO_TypeDef *)pclass->gpio_base, pinfo->pin);
    } else {
        GPIO_ResetBits((GPIO_TypeDef *)pclass->gpio_base, pinfo->pin);
    }
    return TRUE;
}

/*******************************************************************
** 函数名称: ST_GPIO_ReadPin
** 函数描述: 读取GPIO管脚状态
** 参数:     [in] id:  GPIO统一编号,GPIO_PIN_E
** 返回:     返回TRUE则表示是高电平，返回FALSE则表示是低电平
********************************************************************/
BOOLEAN ST_GPIO_ReadPin(INT8U id)
{
    const GPIO_REG_T *pinfo;
    const GPIO_CLASS_T *pclass;
    
    if (id >= ST_GPIO_GetCfgTblMax()) {
        return false;
    }
    
    pinfo = ST_GPIO_GetCfgTblInfo(id);
    pclass = DAL_GPIO_GetRegClassInfo(pinfo->port);
    if (GPIO_ReadInputDataBit((GPIO_TypeDef *)pclass->gpio_base, pinfo->pin) == Bit_SET) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/*******************************************************************
** 函数名称: ST_GPIO_ReadOutputDataBit
** 函数描述: 读取GPIO输出控制寄存器位值
** 参数:     [in] id:  GPIO统一编号,GPIO_PIN_E
** 返回:     返回TRUE则表示是高电平，返回FALSE则表示是低电平
********************************************************************/
BOOLEAN ST_GPIO_ReadOutputDataBit(INT8U id)
{
    const GPIO_REG_T *pinfo;
    const GPIO_CLASS_T *pclass;
    
    if (id >= ST_GPIO_GetCfgTblMax()) {
        return false;
    }
    
    pinfo = ST_GPIO_GetCfgTblInfo(id);
    pclass = DAL_GPIO_GetRegClassInfo(pinfo->port);
    if (GPIO_ReadOutputDataBit((GPIO_TypeDef *)pclass->gpio_base, pinfo->pin) == Bit_SET) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/*******************************************************************
** 函数名称: ST_GPIO_InitDrv
** 函数描述: 初始化GPIO驱动
** 参数:     无
** 返回:     无
********************************************************************/
void ST_GPIO_InitDrv(void)
{
    INT8U i, j, num;
    const GPIO_REG_T *pinfo;
    const GPIO_CLASS_T *pclass;
    GPIO_InitTypeDef  GPIO_InitStructure;
    
    num = DAL_GPIO_GetRegClassMax();
    for (i = 0; i < num; i++) {
        pclass = DAL_GPIO_GetRegClassInfo(i);
        pinfo = pclass->preg;
        RCC_AHBPeriphClockCmd(pclass->gpio_rcc, ENABLE);                       /* Enable the GPIO Clock */
        
        for (j = 0; j < pclass->nreg; j++) {
            if (pinfo->enable) {
                /* Configure the GPIO pin */
                GPIO_InitStructure.GPIO_Pin   = pinfo->pin;
                GPIO_InitStructure.GPIO_Mode  = (GPIOMode_TypeDef)pinfo->direction;
                GPIO_InitStructure.GPIO_OType = (GPIOOType_TypeDef)pinfo->mode;
                GPIO_InitStructure.GPIO_PuPd  = (GPIOPuPd_TypeDef)pinfo->pupd;
                GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
                GPIO_Init((GPIO_TypeDef *)pclass->gpio_base, &GPIO_InitStructure);
                
                if (pinfo->direction == GPIO_Mode_OUT) {
                    ST_GPIO_WritePin(pinfo->id, pinfo->level);
                }
            }
            pinfo++;
        }
    }
}





#if 0

/*******************************************************************
** 函数名称:  CreatInitPortAB
** 函数描述:  创建中断管脚,A B 口 除了GPB.14 GPB.15管脚
**  输入参数:  None
**  返回参数:  None
********************************************************************/
void CreateInitPortAB(GPIOX_BASE_E port, GPIO_PIN_E pin, GPIOINT_MODE mode, void (*callback)(void))
{
    if (GPIO_PORTA == port) {
        gpioapinthand[pin] = callback;
    } else {
        gpiobpinthand[pin] = callback;
    }
    
    DrvGPIO_Open((E_DRVGPIO_PORT)port, pin, E_IO_INPUT);
    DrvGPIO_SetIntCallback(USERGPAB_IRQHandler, 0);
    DrvGPIO_EnableInt((E_DRVGPIO_PORT)port, pin, (E_DRVGPIO_INT_TYPE)mode, E_MODE_EDGE); 
}

/*******************************************************************
** 函数名称:  CreatInitPortCDE
** 函数描述:  创建中断管脚,C D E口
**  输入参数:  None
**  返回参数:  None
********************************************************************/
void CreateInitPortCDE(GPIOX_BASE_E port, GPIO_PIN_E pin, GPIOINT_MODE mode, void (*callback)(void))
{
    if (GPIO_PORTC == port) {
        gpiocpinthand[pin] = callback;
    } else if (GPIO_PORTD == port) {
        gpiodpinthand[pin] = callback;
    } else {
        gpioepinthand[pin] = callback;
    }

    DrvGPIO_Open((E_DRVGPIO_PORT)port, pin, E_IO_INPUT);
    DrvGPIO_SetIntCallback(0, USERGPCDE_IRQHandler);
    DrvGPIO_EnableInt((E_DRVGPIO_PORT)port, pin, (E_DRVGPIO_INT_TYPE)mode, E_MODE_EDGE);
}



/*******************************************************************
** 函数名称:  CreatInitPortAB
** 函数描述:  创建INT0 INT1 中断管脚, B 口GPB.14 GPB.15管脚
**  输入参数:  None
**  返回参数:  None
********************************************************************/
void CreateINT01(GPIOX_BASE_E port, GPIO_PIN_E pin, GPIOINT_MODE mode,  void (*callback)(void))
{
    
    DrvGPIO_Open((E_DRVGPIO_PORT)port, pin, E_IO_INPUT);
    if (PIN14 == pin) {
        DrvGPIO_EnableEINT0((E_DRVGPIO_INT_TYPE)mode, E_MODE_EDGE, callback);
    } else if (PIN15 == pin) {
        DrvGPIO_EnableEINT1((E_DRVGPIO_INT_TYPE)mode, E_MODE_EDGE, callback);
    } else {
        DAL_ASSERT(0);
    }
}
#endif


/************************ (C) COPYRIGHT 2011 XIAMEN YAXON.LTD *******************END OF FILE******/


