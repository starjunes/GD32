/**************************************************************************************************
**                                                                                               **
**  文件名称:  dal_simi2c.h                                                                        **
**  版权所有:  CopyRight @ Xiamen Yaxon NetWork CO.LTD. 2012                                     **
**  创建信息:  2012-9-14 By lyj: 创建本文件                                                      **
**  文件描述:  模拟I2C接口                                                            **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/
#ifndef __DAL_SIMI2C_H
#define __DAL_SIMI2C_H
#include  "dal_include.h"

#define PER_READ_MAX       32

INT8U I2C_ReadByte(INT8U devaddr, INT8U readaddr, BOOLEAN addextend);
BOOLEAN I2C_WriteByte(INT8U devaddr, INT8U writeaddr, INT8U value, BOOLEAN addextend);
INT16U I2C_ReadNumByte(INT8U devaddr, INT8U readaddr, INT8U *data, INT16U readnum, BOOLEAN addextend);
BOOLEAN I2C_WriteNumByte(INT8U devaddr, INT8U writeaddr, INT8U *data, INT8U num, BOOLEAN addextend);
INT32U I2C_ReadWord(INT8U devaddr, INT8U readaddr, BOOLEAN addextend);
void I2C_delay(void);


#endif
/**************************** (C) COPYRIGHT 2012  XIAMEN YAXON.LTD **************END OF FILE******/
