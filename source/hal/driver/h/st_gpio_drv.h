/******************************************************************************
**
** Filename:     st_gpio_drv.h
** Copyright:    
** Description:  该模块主要实现GPIO驱动管理
**
*******************************************************************************
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#ifndef ST_GPIO_DRV_H
#define ST_GPIO_DRV_H        1

#include "st_gpio_reg.h"


/* gpio方向 */
typedef enum {
    GPIO_DIR_IN = 0,
    GPIO_DIR_OUT,
    GPIO_DIR_MAX
} GPIO_DIR_E;

/* IO模式设置 */
typedef enum {
  GPIO_MODE_NULL = 0x00,     /* 无上下拉 */
  GPIO_MODE_UP,              /* 上拉 */
  GPIO_MODE_DOWN,            /* 下拉 */
  GPIO_MODE_ANALOG,          /* 模拟输入 */
  GPIO_MODE_OD,              /* 开漏输出 */
  GPIO_MODE_PP,              /* 推挽输出 */
  GPIO_PULL_MAX
} GPIO_MODE_E;

/* gpio速度 */
typedef enum {
    GPIO_SPEED_LOW = 0X00,
    GPIO_SPEED_MEDI,
    GPIO_SPEED_FAST,
    GPIO_SPEED_HIGH
} GPIO_SPEED_E;




/*******************************************************************
** 函数名称: ST_GPIO_SetPinEx
** 函数描述: 初始化管脚状态
** 参数:     [in] id:        GPIO统一编号,GPIO_PIN_E
**           [in] direction: 方向,GPIO_DIR_E
**           [in] mode:      上下拉,GPIO_MODE_E
             [in] speed       io口速度, GPIO_SPEED_E
**           [in] level:     高低电平,TRUE-高电平，FALSE-低电平
** 返回:     成功返回TRUE，失败返回FALSE
********************************************************************/
void ST_GPIO_SetPinEx(INT8U id, INT8U direction, INT8U mode,INT8U speed, INT8U level);


/*******************************************************************
** 函数名称: ST_GPIO_SetPin
** 函数描述: 初始化管脚状态
** 参数:     [in] id:        GPIO统一编号,GPIO_PIN_E
**           [in] direction: 方向,GPIO_DIR_E
**           [in] mode:      上下拉,GPIO_MODE_E
**           [in] level:     高低电平,TRUE-高电平，FALSE-低电平
** 返回:     成功返回TRUE，失败返回FALSE
********************************************************************/
void ST_GPIO_SetPin(INT8U id, INT8U direction, INT8U mode, INT8U level);

/*******************************************************************
** 函数名称: ST_GPIO_WritePin
** 函数描述: 读取GPIO管脚状态
** 参数:     [in] id:   GPIO统一编号,GPIO_PIN_E
**           [in] level: 高低电平,TRUE-高电平，FALSE-低电平
** 返回:     成功返回TRUE，失败返回FALSE
********************************************************************/
BOOLEAN ST_GPIO_WritePin(INT8U id, INT8U level);

/*******************************************************************
** 函数名称: ST_GPIO_ReadPin
** 函数描述: 读取GPIO管脚状态
** 参数:     [in] id:  GPIO统一编号,GPIO_PIN_E
** 返回:     返回TRUE则表示是高电平，返回FALSE则表示是低电平
********************************************************************/
BOOLEAN ST_GPIO_ReadPin(INT8U id);

/*******************************************************************
** 函数名称: ST_GPIO_ReadOutputDataBit
** 函数描述: 读取GPIO输出控制寄存器位值
** 参数:     [in] id:  GPIO统一编号,GPIO_PIN_E
** 返回:     返回TRUE则表示是高电平，返回FALSE则表示是低电平
********************************************************************/
BOOLEAN ST_GPIO_ReadOutputDataBit(INT8U id);

/*******************************************************************
** 函数名称: ST_GPIO_InitDrv
** 函数描述: 初始化GPIO驱动
** 参数:     无
** 返回:     无
********************************************************************/
void ST_GPIO_InitDrv(void);


#endif

/************************ (C) COPYRIGHT 2011 XIAMEN YAXON.LTD *******************END OF FILE******/

