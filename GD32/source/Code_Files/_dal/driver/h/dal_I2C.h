/**************************************************************************************************
**                                                                                               **
**  文件名称:  dal_i2c.h                                                                         **
**  版权所有:  CopyRight @ Xiamen Yaxon NetWork CO.LTD. 2012                                     **
**  创建信息:  2012-9-14 By lyj: 创建本文件                                                      **
**  文件描述:  模拟I2C接口                                                                       **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/
#ifndef __DAL_I2C_H
#define __DAL_I2C_H


void dal_iic_init(void);
BOOLEAN dal_iic_writedata(INT8U devaddr, INT8U mode, INT8U devaddrextend, INT8U writeaddr, INT8U *data, INT8U num);
INT16S dal_iic_readdata(INT8U devaddr, INT8U mode, INT8U devaddrextend, INT8U readaddr, INT8U *data, INT16U readnum);

#endif
/**************************** (C) COPYRIGHT 2012  XIAMEN YAXON.LTD **************END OF FILE******/

