/**************************************************************************************************
**                                                                                               **
**  文件名称:  dal_simi2_2.c                                                                       **
**  版权所有:  CopyRight @ Xiamen Yaxon NetWork CO.LTD. 2012                                     **
**  创建信息:  2012-9-14 By lyj: 创建本文件                                                      **
**  文件描述:  模拟I2C接口                                                            **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/
#include  "dal_simi2c_2.h"
#if EN_DEBUG > 0
#include  "debug_print.h"
#endif
#include  "dal_pinlist.h"
#include  "dal_gpio.h"
#define u8 int8_t
/*************************************************************************************************/
/*                           宏定义                                                              */
/*************************************************************************************************/
#define SCL_H         GPIO_BOP(GPIOA) = GPIO_PIN_8
#define SCL_L         GPIO_BC(GPIOA)  = GPIO_PIN_8

#define SDA_H         GPIO_BOP(GPIOC) = GPIO_PIN_9
#define SDA_L         GPIO_BC(GPIOC)  = GPIO_PIN_9

#define SCL_read      GPIO_ISTAT(GPIOA)   & GPIO_PIN_8
#define SDA_read      GPIO_ISTAT(GPIOC)   & GPIO_PIN_9

#define I2C_DELAY_NUM 80

// 定义I2C逻辑驱动状态机
#define I2C_SIM_STAT_INIT_      0x01
#define I2C_SIM_STAT_OPEN_      0x02
#define I2C_SIM_STAT_READY_     0x03
#define I2C_SIM_STAT_BUSY_      0x04

/*************************************************************************************************/
/*                           定义管理结构                                                        */
/*************************************************************************************************/
// 定义操作枚举类型
typedef enum {
    I2C_SIM_OP_NONE,
    I2C_SIM_OP_WRITE,
    I2C_SIM_OP_RDCMD,
    I2C_SIM_OP_READ,
    I2C_SIM_OP_MAX
} I2C_SIM_OP_E;

// 定义I2C管理结构体
typedef struct {
    INT8U           status;             // I2C逻辑驱动状态
    I2C_SIM_OP_E      operation;          // 操作类型
    INT8U           dev_addr;           // 从机地址
    INT8U           addr;                // 寄存器地址
    INT8U           *pbuf;              // 读写数据缓冲
    INT16U          dlen;               // 读写数据长度
} I2C_SIM_CB_T;

//管理结构体
static I2C_SIM_CB_T s_i2c_sim_cb[MAX_I2C_SIM_IDX];
/*
*********************************************************************************
*                      本地接口实现
*********************************************************************************
*/

// 重置icb管理结构体到初始化状态
static void ResetICBToInit(INT8U idx)
{
    if (idx >= MAX_I2C_SIM_IDX) return;

    s_i2c_sim_cb[idx].operation = I2C_SIM_OP_NONE;
    s_i2c_sim_cb[idx].pbuf   = NULL;
    s_i2c_sim_cb[idx].dlen   = 0;
    s_i2c_sim_cb[idx].status = I2C_SIM_STAT_INIT_;
}

// 重置icb管理结构体到就绪状态
static void ResetICBToReady(INT8U idx)
{
    if (idx >= MAX_I2C_SIM_IDX) return;

    s_i2c_sim_cb[idx].operation = I2C_SIM_OP_NONE;
    s_i2c_sim_cb[idx].pbuf   = NULL;
    s_i2c_sim_cb[idx].dlen   = 0;
    s_i2c_sim_cb[idx].status = I2C_SIM_STAT_READY_;
}

/**************************************************************************************************
**  函数名称:  Delay
**  功能描述:  延时函数
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
static void Delay(INT32U ticks)
{
    while(ticks--);
}
/**************************************************************************************************
**  函数名称:  I2C_delay
**  功能描述:  延时函数
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
static void I2C_delay(void)
{
   INT16U i = 40; //150 //这里可以优化速度 ，经测试最低到5还能写入
   while(i--);
}

/**************************************************************************************************
**  函数名称:  I2C_Start
**  功能描述:  发送启始位
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
static BOOLEAN I2C_Start(void)
{
    SDA_H;
    SCL_H;
    I2C_delay();
    if (!SDA_read) {
        return FALSE; //SDA线为低电平则总线忙,退出
    }
    SDA_L;
    I2C_delay();
    if (SDA_read) {
        return FALSE; //SDA线为高电平则总线出错,退出
    }
    SDA_L;
    I2C_delay();
    return TRUE;
}
/**************************************************************************************************
**  函数名称:  I2C_Stop
**  功能描述:  发送停止位
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
static void I2C_Stop(void)
{
    SCL_L;
    I2C_delay();
    SDA_L;
    I2C_delay();
    SCL_H;
    I2C_delay();
    SDA_H;
    I2C_delay();
}

/**************************************************************************************************
**  函数名称:  I2C_Ack
**  功能描述:  发送应答位
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
static void I2C_Ack(void)
{
    SCL_L;
    I2C_delay();
    SDA_L;
    I2C_delay();
    SCL_H;
    I2C_delay();
    SCL_L;
    I2C_delay();
}

/**************************************************************************************************
**  函数名称:  I2C_NoAck
**  功能描述:  不发送应答位
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
static void I2C_NoAck(void)
{
    SCL_L;
    I2C_delay();
    SDA_H;
    I2C_delay();
    SCL_H;
    I2C_delay();
    SCL_L;
    I2C_delay();
}

/**************************************************************************************************
**  函数名称:  I2C_WaitAck
**  功能描述:  等待应答
**  输入参数:  None
**  返回参数:  返回为:=1有ACK,=0无ACK
**************************************************************************************************/
static BOOLEAN I2C_WaitAck(void)
{
    SCL_L;
    I2C_delay();
    SDA_H;
    I2C_delay();
    SCL_H;
    I2C_delay();
    if (SDA_read) {
        SCL_L;
        return FALSE;
    }
    SCL_L;
    return TRUE;
}

/**************************************************************************************************
**  函数名称:  I2C_SendByte
**  功能描述:  发送一个byte ,数据从高位到低位
**  输入参数:  None
**  返回参数:  无
**************************************************************************************************/
static void I2C_SendByte(INT8U SendByte)
{
    u8 i = 8;

    while (i--) {
        SCL_L;
        I2C_delay();
        if (SendByte & 0x80) {
            SDA_H;
        } else {
            SDA_L;
        }
        SendByte<<=1;
        I2C_delay();
        SCL_H;
		I2C_delay();
		I2C_delay();
    }

    SCL_L;
}

/**************************************************************************************************
**  函数名称:  I2C_ReceiveByte
**  功能描述:  接收一个byte ,数据从高位到低位
**  输入参数:  None
**  返回参数:  无
**************************************************************************************************/
static INT8U I2C_ReceiveByte(void)
{
    u8 i = 8;
    u8 ReceiveByte = 0;

    SDA_H;
    while(i--) {
        ReceiveByte <<= 1;
        SCL_L;
        I2C_delay();
        SCL_H;
        I2C_delay();
        if (SDA_read) {
            ReceiveByte |= 0x01;
        }
    }

#if 0

    for (i = 0; i < 8; i ++) {
        SCL_H;
        I2C_delay();
        ReceiveByte<<=1;
        if(SDA_read)
        {
          ReceiveByte|=0x01;
        }
        SCL_L;
        I2C_delay();
    }
  #endif
    SCL_L;
    return ReceiveByte;
}

#if 0
/*****************************************************************************
**  函数名:  I2C_WriteByte
**  函数描述: 写一个字节到指定设备的指定地址中
**  参数:       [in] devaddr    :从机地址
**                     [in] writeaddr  :寄存器地址
**                     [in] value      :数据
**  返 回 :
*****************************************************************************/
static BOOLEAN I2C_WriteByte(INT8U devaddr, INT8U writeaddr, INT8U value)
{
    if (!I2C_Start()) {
        return FALSE;
    }

    I2C_SendByte(devaddr);  //硬件地址
    if (!I2C_WaitAck()) {
        I2C_Stop();
        return FALSE;
    }

    I2C_SendByte(writeaddr);    //寄存器地址
    if (!I2C_WaitAck()) {
        I2C_Stop();
        return FALSE;
    }

    I2C_SendByte(value);        //写入数值
    if (!I2C_WaitAck()) {
        I2C_Stop();
        return FALSE;
    }

    I2C_Stop();
    Delay(200);//Delay(2000);

    return TRUE;
}
#endif

/*****************************************************************************
**  函数名:  I2C_WriteBytes
**  函数描述: 写多字节数据到指定设备的指定地址中
**  参数:       [in]devaddr    :从机设备地址
**                     [in]writeaddr  :寄存器地址
**                     [in]data      :写入数据
**                     [in]dat_len    :写入数据长度
**  返 回 : TRUE:写入成功;FALSE:写入失败
*****************************************************************************/
static BOOLEAN I2C_WriteBytes(INT8U devaddr, INT8U writeaddr, INT8U *data,INT8U dat_len)
{
    INT8U i;
    if (!I2C_Start()) {
        return FALSE;
    }

    I2C_SendByte(devaddr);  //硬件地址
    if (!I2C_WaitAck()) {
        I2C_Stop();
        return FALSE;
    }

    I2C_SendByte(writeaddr);    //寄存器地址
    if (!I2C_WaitAck()) {
        I2C_Stop();
        return FALSE;
    }

    //写入数据
    for(i = 0; i < dat_len; i++){
        I2C_SendByte(data[i]);        //写入数值
        if (!I2C_WaitAck()) {
            I2C_Stop();
            return FALSE;
        }
    }
    I2C_Stop();

    Delay(200);//Delay(2000);

    return TRUE;
}

#if 0
/*****************************************************************************
**  函数名:  I2C_ReadByte
**  函数描述: 从从机读一个byte内容
**  参数:       [in]devaddr   :从机地址
**                     [in]readaddr  :寄存器地址
**                     [in]byte     :一个字节缓冲区
**  返 回 : TRUE:读取成功;FALSE:读取失败
*****************************************************************************/
static BOOLEAN I2C_ReadByte(INT8U devaddr, INT8U readaddr,INT8U byte[1])
{
    if (!I2C_Start()) {
        return FALSE;
    }

    I2C_SendByte(devaddr);
    if (!I2C_WaitAck()) {
        I2C_Stop();
        return FALSE;
    }

    I2C_SendByte(readaddr);
    if (!I2C_WaitAck()) {
        I2C_Stop();
        return FALSE;
    }

    if (!I2C_Start()) return FALSE;

    I2C_SendByte(devaddr|0x01); //读
    if (!I2C_WaitAck()) {
        I2C_Stop();
        return FALSE;
    }

    byte[0] = I2C_ReceiveByte();
    I2C_NoAck();

    I2C_Stop();
    Delay(200);

    return TRUE;
}
#endif

/*****************************************************************************
**  函数名:  I2C_ReadBytes
**  函数描述: 从从机读取多个字节数据
**  参数:       [in] devaddr   :从机地址
**                     [in] readaddr  :寄存器地址
**                     [in]data     :读取数据缓冲区
**                     [in] data_len  :读取数据长度
**  返 回 :
*****************************************************************************/
static BOOLEAN I2C_ReadBytes(INT8U devaddr, INT8U readaddr,INT8U* data,INT8U data_len)
{
    INT8U i;

    if (!I2C_Start()) {
        return FALSE;
    }

    I2C_SendByte(devaddr);                                           /* 设置器件地址+段地址  */
    if (!I2C_WaitAck()) {
        I2C_Stop();
        return FALSE;
    }

    I2C_SendByte(readaddr);                                          /* 设置低起始地址       */
    if (!I2C_WaitAck()) {
        I2C_Stop();
        return FALSE;
    }

    if (!I2C_Start()) return FALSE;

    I2C_SendByte(devaddr|0x01); //读
    if (!I2C_WaitAck()) {
        I2C_Stop();
        return FALSE;
    }

    for(i = 0; i < data_len; i++){
        data[i] = I2C_ReceiveByte();
        if(i == (data_len-1)){    //最后一个字节
            I2C_NoAck();          //最后一个字节无应答
        }
        else{
            I2C_Ack();            //非最后一个字节，应答
        }
    }

    I2C_Stop();

    Delay(200);
    return TRUE;
}

/*
*********************************************************************************
*                       对外接口实现
*********************************************************************************
*/

/********************************************************************************
**  函数名称:  dal_i2c_sim_ReadData
**  功能描述:  从I2C外设读数据
**  输入参数:  [in]  idx:     I2C通道编号,从0开始
**                  optpara:  I2C读写操作参数结构体指针
**  返回参数:  TRUE，读取成功； FALSE，读取失败
********************************************************************************/
BOOLEAN dal_i2c_sim_ReadData(INT8U idx, I2C_SIM_OPPARA_T *op_para)
{
    BOOLEAN ret;
    I2C_SIM_CB_T *pcb;

    if (idx >= MAX_I2C_SIM_IDX || op_para == NULL) return FALSE;
    pcb = &s_i2c_sim_cb[idx];
    if ((pcb->status & I2C_SIM_STAT_READY_) != I2C_SIM_STAT_READY_) return FALSE;
    if ((pcb->status & I2C_SIM_STAT_BUSY_) != 0) {
        return FALSE;                               // 通道忙
    }
    pcb->status |= I2C_SIM_STAT_BUSY_;

    pcb->operation = I2C_SIM_OP_READ;
    pcb->dev_addr = op_para->devaddr;
    pcb->addr = op_para->addr;
    pcb->pbuf = op_para->buf;
    pcb->dlen = op_para->dlen;

    ret = I2C_ReadBytes(pcb->dev_addr,pcb->addr,pcb->pbuf,pcb->dlen);

//retp:
    ResetICBToReady(idx);
    return ret;
}


/********************************************************************************
**  函数名称:  dal_i2c_sim_ReadByte
**  功能描述:  从I2C外设读一个字节
**  输入参数:  [in]  idx:     I2C通道编号,从0开始
**                  devaddr: 从机地址
**                  rdaddr:  读取的目标地址
**                  rd_buf: 读取缓冲,1字节
**  返回参数:  读取成功返回读到的字节； 读取失败返回 -1.
********************************************************************************/
BOOLEAN dal_i2c_sim_ReadByte(INT8U idx, INT8U devaddr, INT8U readaddr,INT8U *rd_buf)
{
    BOOLEAN ret;
    I2C_SIM_OPPARA_T op_para;
    //INT8U rch;

    if (idx >= MAX_I2C_SIM_IDX) return FALSE;
    op_para.devaddr = devaddr;
    op_para.addr = readaddr;
    op_para.buf  = rd_buf;
    op_para.dlen = 1;

    ret = dal_i2c_sim_ReadData(idx, &op_para);
    return ret;
}

/********************************************************************************
**  函数名称:  dal_i2c_sim_WriteData
**  功能描述:  向I2C外设写数据
**  输入参数:  [in]  idx:     I2C通道编号,从0开始
**                  optpara:  I2C读写操作参数结构体指针
**  返回参数:  TRUE，写入成功； FALSE，写入失败
********************************************************************************/
BOOLEAN dal_i2c_sim_WriteData(INT8U idx, I2C_SIM_OPPARA_T *op_para)
{
    BOOLEAN ret = FALSE;
    I2C_SIM_CB_T *pcb;
    //INT8U *pmem;

    if (idx >= MAX_I2C_SIM_IDX || op_para == NULL) return FALSE;
    pcb = &s_i2c_sim_cb[idx];
    if ((pcb->status & I2C_SIM_STAT_READY_) != I2C_SIM_STAT_READY_) return FALSE;
    if ((pcb->status & I2C_SIM_STAT_BUSY_) != 0) {
        return FALSE;                               // 通道忙
    }
    pcb->status |= I2C_SIM_STAT_BUSY_;

#if 0
    if ((pmem = hal_malloc(op_para->dlen)) == NULL) {
        ret = FALSE;
        goto retp;
    }
    //memcpy(pcb->pbuf, op_para->buf, op_para->dlen);
#endif
    pcb->pbuf = op_para->buf;//pcb->pbuf = pmem;
    pcb->dev_addr = op_para->devaddr;// 保存从机地址
    pcb->addr = op_para->addr;// 保存寄存器地址
    pcb->dlen = op_para->dlen;
    pcb->operation = I2C_SIM_OP_WRITE;

    ret = I2C_WriteBytes(pcb->dev_addr,pcb->addr,pcb->pbuf,pcb->dlen);

//retp:
    //if (pmem != NULL) hal_free(pmem);
    ResetICBToReady(idx);
    return ret;
}

/********************************************************************************
**  函数名称:  dal_i2c_sim_WriteByte
**  功能描述:  向I2C外设写一个字节
**  输入参数:  [in]  idx:     I2C通道编号,从0开始
**                  devaddr: 从机地址
**                  wraddr:  写入的目标地址
**                  wrbyte:  待写入的字节
**  返回参数:  TRUE，写入成功； FALSE，写入失败
********************************************************************************/
BOOLEAN dal_i2c_sim_WriteByte(INT8U idx, INT8U devaddr, INT8U wraddr, INT8U wrbyte)
{
    I2C_SIM_OPPARA_T op_para;

    if (idx >= MAX_I2C_SIM_IDX) return FALSE;
    op_para.devaddr = devaddr;
    op_para.addr = wraddr;
    op_para.buf  = &wrbyte;
    op_para.dlen = 1;

    return dal_i2c_sim_WriteData(idx, &op_para);
}

/********************************************************************************
**  函数名称:  dal_i2c_GetState
**  功能描述:  获取g-sensor设备工作状态
**  输入参数:  idx:传感器序号
**  返回参数:  TRUE:准备就绪    FALSE:未准备好
********************************************************************************/
BOOLEAN dal_i2c_GetState(INT8U idx)
{
	if(idx >= MAX_I2C_SIM_IDX)return FALSE;
	INT8U ret = s_i2c_sim_cb[idx].status & I2C_SIM_STAT_OPEN_;
	if(ret != 0)return TRUE;
	else return FALSE;
}

/********************************************************************************
**  函数名称:  dal_i2c_sim_Open
**  功能描述:  打开指定的I2C通道
**  输入参数:  [in] idx: I2C通道编号,从0开始
**  返回参数:  TRUE，打开成功； FALSE，打开失败
********************************************************************************/
BOOLEAN dal_i2c_sim_Open(INT8U idx)
{
    if (idx >= MAX_I2C_SIM_IDX) return FALSE;
    if ((s_i2c_sim_cb[idx].status & I2C_SIM_STAT_INIT_) == 0) return FALSE;    // 未初始化无法打开
    if ((s_i2c_sim_cb[idx].status & I2C_SIM_STAT_OPEN_) != 0) return FALSE;    // 无需重复打开

    s_i2c_sim_cb[idx].status |= I2C_SIM_STAT_OPEN_;
    return TRUE;
}

/********************************************************************************
**  函数名称:  dal_i2c_sim_Close
**  功能描述:  关闭指定的I2C通道
**  输入参数:  [in] idx: I2C通道编号,从0开始
**  返回参数:  无
********************************************************************************/
void dal_i2c_sim_Close(INT8U idx)
{

    if (idx >= MAX_I2C_SIM_IDX) return;
    if ((s_i2c_sim_cb[idx].status & I2C_SIM_STAT_OPEN_) == 0) return;    // 未打开，无需关闭

    ResetICBToInit(idx);
}

/********************************************************************************
**  函数名称:  dal_i2c_sim_Init
**  功能描述:  I2C通道初始化，底层系统级专用接口，非面向上层
**  输入参数:  无
**  返回参数:  无
********************************************************************************/
void dal_i2c_sim_Init(void)
{
    INT8U idx;
    CreateOutputPort(HIT_CL_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_2MHZ, FALSE);     /* 创建时钟输出管脚，推挽输出 */
    CreateOutputPort(HIT_DA_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_2MHZ, FALSE);    /* 创建数据IO管脚，开漏输出，可以读取管脚状态*/
    memset(s_i2c_sim_cb, 0, sizeof(s_i2c_sim_cb));
    for (idx = 0; idx < MAX_I2C_SIM_IDX; idx++) {
        s_i2c_sim_cb[idx].status = I2C_SIM_STAT_INIT_;
    }
}
/**************************** (C) COPYRIGHT 2012  XIAMEN YAXON.LTD **************END OF FILE******/

