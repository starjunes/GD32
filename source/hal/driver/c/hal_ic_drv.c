/********************************************************************************
**
** 文件名:     hal_ic_drv.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现IC卡读写卡功能（4442和24CXX系列的IC卡）
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2014/04/09 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "yx_include.h"
#include "st_gpio_drv.h"
#include "st_i2c_simu.h"
#include "hal_ic_drv.h"
#include "yx_debug.h"


//#if EN_ICCARD > 0

/*
********************************************************************************
* 定义配置参数
********************************************************************************
*/
#define ATR_HEAD_DATA        0xA2131091     /* H1=0xA2;H2=0x13;H3=0x10;H4=0x91. 按从低到高位接收 */
#define MAX_MEMORY           256
#define MAX_ADDR             256

#define I2C_COM_IC           I2C_COM_1

//public static  byte[] KeyA1 = { 0xFF, 0xFF, 0xFF };
//public static byte[] KeyA2 = { 0x98, 0x26, 0x94 };//“YXRFID”

//static INT8U   RefData[3]={0x88, 0x88, 0x88};
/* 读写卡命令 */
#define CMD_RD_MAIN          0x30      /* 读主存储区 */
#define CMD_WR_MAIN          0x38      /* 写主存储区 */
#define CMD_RD_PROTECT       0x34      /* 读保护存储区 */
#define CMD_WR_PROTECT       0x3C      /* 写保护存储区 */
#define CMD_RD_SECURITY      0x31      /* 读加密存储区 */
#define CMD_WR_SECURITY      0x39      /* 写加密存储区(4个字节，1字节计数和3字节密码) */
#define CMD_VERIFY_PSC       0x33      /* 校验一个PSC，用于验证密码 */

/* 定义管脚 */
#define PIN_IC_IO            GPIO_PIN_C6
#define PIN_IC_RST           GPIO_PIN_C7
#define PIN_IC_CLK           GPIO_PIN_C8
#define PIN_IC_DECT          GPIO_PIN_C9

/* 定义管脚控制 */
#define Pullup_IO()          ST_GPIO_WritePin(PIN_IC_IO, true)
#define Pulldown_IO()        ST_GPIO_WritePin(PIN_IC_IO, false)
#define ReadPin_IO()         ST_GPIO_ReadPin(PIN_IC_IO)

#define Pullup_RST()         ST_GPIO_WritePin(PIN_IC_RST, true)
#define Pulldown_RST()       ST_GPIO_WritePin(PIN_IC_RST, false)

#define Pullup_CLK()         ST_GPIO_WritePin(PIN_IC_CLK, true)
#define Pulldown_CLK()       ST_GPIO_WritePin(PIN_IC_CLK, false)


/*******************************************************************
** 函数名:     PinsConfig
** 函数描述:   IC卡管脚配置
** 参数:       无
** 返回:       无
********************************************************************/
static void PinsConfig(void)
{
    ST_GPIO_SetPin(PIN_IC_CLK,  GPIO_DIR_OUT, GPIO_MODE_PP, 0);
    ST_GPIO_SetPin(PIN_IC_RST,  GPIO_DIR_OUT, GPIO_MODE_PP, 0);
    ST_GPIO_SetPin(PIN_IC_IO,   GPIO_DIR_OUT, GPIO_MODE_PP, 1);
    //ST_GPIO_SetPin(PIN_IC_DECT, GPIO_DIR_IN,  GPIO_MODE_PULL, 0);
}

/*******************************************************************
** 函数名:     Delay
** 函数描述:   延时一段时间
** 参数:       [in] period: 延时时间,单位：us
** 返回:       无
********************************************************************/
static void Delay(INT32U period)
{
    /*INT32U count;
    
    count = period * 24;
    while (count-- > 0) {
        ;
    }*/
}

/*******************************************************************
** 函数名:     OutputClk
** 函数描述:   执行n个CLK脉冲控制，每个CLK脉冲约20us
** 参数:       [in] nclk:  时脉冲数
** 返回:       无
********************************************************************/
static void OutputClk(INT16U nclk)
{
    while (nclk-- > 0) {
       Delay(5);
	   Pullup_CLK();
	   Delay(10);
	   Pulldown_CLK();
	   Delay(5);
    }
}

/*******************************************************************
** 函数名:     SendOneByte
** 函数描述:   发送一个字节
** 参数:       [in] data:  字节数据
** 返回:       无
********************************************************************/
static void SendOneByte(INT8U data)
{
	INT8U i;

    for (i = 0; i < 8; i++) {
        if ((data & (1 << i)) != 0) {                                          /* 从最低位开始 */
            Pullup_IO();
        } else {
            Pulldown_IO();
        }
        
        OutputClk(1);                                                          /* 输出一个脉冲 */
	}
}

/*******************************************************************
** 函数名:     ReadOneByte
** 函数描述:   读取一个字节
** 参数:       [in] data:  字节数据
** 返回:       无
********************************************************************/
static INT8U ReadOneByte(void)
{
    INT8U i;
    INT8U data = 0;
    
    for (i = 0; i < 8; i++) {
        OutputClk(1);
	    if (ReadPin_IO()) {
            data |= (1 << i);   
	    }
    }
    
    return data;
}

/*******************************************************************
** 函数名:     AnswerToReset
** 函数描述:   复位操作，并读取IC卡中开头4个字节的数据（一般为厂商固化的信息，包含卡的数据传输协议，存储容量信息等）
** 参数:       无
** 返回:       返回头4个字节数据
********************************************************************/
static INT32U AnswerToReset(void)
{
	INT8U i, readbyte;
	INT32U temp;
	
	Pullup_IO();
	Pulldown_RST();          //RST_L;
    Pulldown_CLK();          //CLK_L;
	Pullup_RST();            //RST_H;
	Delay(5);
	Pullup_CLK();            //CLK_H;
	Delay(20);
	Pulldown_CLK();          //CLK_L;
	Delay(5);
	Pulldown_RST();          //RST_L;
	Delay(5);
	
	readbyte = 0;
	if (ReadPin_IO()) {                                                        /* 第一字节 */
	    readbyte |= 1;   
	}
	for (i = 1; i < 8; i++) {
        OutputClk(1);
	    if (ReadPin_IO()) {
            readbyte |= (1 << i);
	    }
    }
	
	temp = readbyte;
    for (i = 0; i < 3; i++) {                                                  /* 后3个字节 */
        temp <<= 8;
        temp |=  ReadOneByte();
	}
	OutputClk(1);                                                              /* 这个CLK脉冲将使I/O置为high impedance z */
	return temp;
}

/*******************************************************************
** 函数名:     SendCommand
** 函数描述:   发送命令到IC卡
** 参数:       [in] cmd： 控制命令
**             [in] addr: 地址
**             [in] data：数据
** 返回:       无
** 备注：     SLE4442的控制命令有：
**            0x30 -> 读主存储区;    0x38 -> 写主存储区;
**            0x34 -> 读保护存储区;  0x3c -> 写保护存储区; 
**            0x31 -> 读加密存储区;  0x39 -> 写加密存储区(4个字节，1字节技术和3字节密码);
**            0x33 -> 校验一个PSC，用于验证密码
********************************************************************/
static void SendCommand(INT8U cmd, INT16U addr, INT8U data)
{
    /*---start condition--------------*/
    Pullup_IO();    //IC_O_H;
	Pullup_CLK();   //CLK_H;
	Delay(5);
	Pulldown_IO();   //IC_O_L;
	Delay(5);
	Pulldown_CLK();  //CLK_L;
	
    /*---send command,addr,data-------*/
	SendOneByte(cmd);                                                          /* 输出控制命令 */
    SendOneByte(addr);                                                         /* 输出地址 */
    SendOneByte(data);                                                         /* 输出数据 */
    
    /*---stop condition----------------*/
    Pulldown_IO();   //IC_O_L;
    Delay(5); 
    Pullup_CLK();   //CLK_H;
    Delay(5);
    Pullup_IO();    //IC_O_H;
    Delay(5);
    /* 此时CLK和I/O均处于高电平，并且已经持续4us以上 */
}

/*******************************************************************
** 函数名:     ReadMainMemoryData
** 函数描述:   读取主存储器数据
** 参数:       [in]  addr:   地址
**             [out] dptr：  数据指针
**             [in]  maxlen：数据长度
** 返回:       读取到数据长度
********************************************************************/
static INT16U ReadMainMemoryData(INT16U addr, INT8U *dptr, INT16U maxlen)
{
    INT16U i;
    
    if (addr >= MAX_ADDR || dptr == 0) {
        return 0;
    }
    
    SendCommand(CMD_RD_MAIN, addr, 0x00);                                      /* 发送读取主存储器命令 */
    for (i = 0; i < maxlen; i++) {
        dptr[i] = ReadOneByte();
    }
    
    OutputClk((256 - addr - i) * 8);                                           /* 输出剩余的必要CLK脉冲 */
    OutputClk(1);                                                              /* 这个CLK脉冲将使I/O置为high impedance z*/
    return i;
}

#if 0
/*******************************************************************
** 函数名:     WriteMainMemoryData
** 函数描述:   写主存储器数据
** 参数:       [in]  addr: 地址
**             [in]  data：数据
** 返回:       读取到数据长度
********************************************************************/
static BOOLEAN WriteMainMemoryData(INT16U addr, INT8U data)
{
    INT16U count;
    
    if (addr >= MAX_ADDR) {
        return false;
    }
    
    count = 256;
    SendCommand(CMD_WR_MAIN, addr, data);
    OutputClk(1);
    while (!ReadPin_IO() && --count > 0) {                                     /* 大约需要2.5ms或5ms,取决于原数据和新数据 */
        OutputClk(1);
    }
    
    if (count > 0) {
        return true;
    } else {
        return false;
    }
}

/*******************************************************************
** 函数名:     ReadProtectMemoryData
** 函数描述:   读取保护存储器数据
** 参数:       无
** 返回:       返回4个字节数据
********************************************************************/
static INT32U ReadProtectMemoryData(void)
{
    INT8U i;
	INT32U temp;
    
    SendCommand(CMD_RD_PROTECT, 0, 0);
    temp = 0;
    for (i = 0; i < 4; i++) {      
        temp |= (i * 8 ) << ReadOneByte();
	}
	OutputClk(1);                                                           /* 这个CLK脉冲将使I/O置为high impedance z */
	return temp;
}

/*******************************************************************
** 函数名:     WriteProtectMemoryData
** 函数描述:   写保护存储器数据
** 参数:       [in]  addr: 地址
**             [in]  data：数据
** 返回:       读取到数据长度
********************************************************************/
static BOOLEAN WriteProtectMemoryData(INT16U addr, INT8U data)
{
    INT16U count;
    
    if (addr >= MAX_ADDR) {
        return false;
    }
    
    count = 256;
    SendCommand(CMD_WR_PROTECT, addr, data);
    OutputClk(1);
    while (!ReadPin_IO() && count-- > 0) {                                     /* 大约需要2.5ms或5ms,取决于原数据和新数据 */
        OutputClk(1);
    }
    
    if (count > 0) {
        return true;
    } else {
        return false;
    }
}

/*******************************************************************
** 函数名:     ReadSecurityMemoryData
** 函数描述:   读取加密存储器数据
** 参数:       无
** 返回:       返回4个字节数据
********************************************************************/
static INT32U ReadSecurityMemoryData(void)
{
    INT8U i;
	INT32U temp;
    
    SendCommand(CMD_RD_SECURITY, 0, 0);
    temp = 0;
    for (i = 0; i < 4; i++) {      
        temp |= (i * 8 ) << ReadOneByte();
	}
	OutputClk(1);                                                           /* 这个CLK脉冲将使I/O置为high impedance z */
	return temp;
}

/*******************************************************************
** 函数名:     WriteSecurityMemoryData
** 函数描述:   写加密存储器数据
** 参数:       [in]  addr: 地址
**             [in]  data：数据
** 返回:       读取到数据长度
********************************************************************/
static BOOLEAN WriteSecurityMemoryData(INT16U addr, INT8U data)
{
    INT16U count;
    
    if (addr >= MAX_ADDR) {
        return false;
    }
    
    count = 256;
    SendCommand(CMD_WR_SECURITY, addr, data);
    OutputClk(1);
    while (!ReadPin_IO() && count-- > 0) {                                     /* 大约需要2.5ms或5ms,取决于原数据和新数据 */
        OutputClk(1);
    }
    
    if (count > 0) {
        return true;
    } else {
        return false;
    }
}

/*******************************************************************
** 函数名:     ComparePscData
** 函数描述:   校验加密存储器密码（3字节）
** 参数:       [in]  addr: 地址
**             [in]  data：数据
** 返回:       读取到数据长度
********************************************************************/
static void ComparePscData(INT8U *psc)
{
    INT8U i;
    INT16U count;

    for(i = 0; i < 3; i++){
        count = 2;
        SendCommand(CMD_VERIFY_PSC, i + 1, psc[i]);
        OutputClk(1);
        while (!ReadPin_IO() && count-- > 0) {                                     /* 大约需要2个脉冲 */
            OutputClk(1);
        }
    }
}

/**************************************************************************************************
**  函数名称:  VerifyPSC
**  功能描述:  PSC可编程保密代码验证(密码验证)
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
static BOOLEAN VerifyPSC(void)
{
    #if 0
    INT8U temp_ec;

    //----read EC----------
    ReadSecurityData();
    temp_ec = ReadDataBuf[0];

    if(temp_ec <= 1 ){//若只剩一次机会就不继续验证，否则会损坏IC卡
       return FALSE;
    }
        //PrintStr_Uart0("temp_ec is!!\n",0xff); 
        //PutHexByUart0(temp_ec);
        //PrintStr_Uart0("\n",0xff); 


    //----Update EC---------
    temp_ec >>= 1;//右移1位，将左边第一个不是0的位置为0,即0000$0111 -> 0000$0011
    WriteSecurityMem(0x0, temp_ec);
    //----compare refdata---
    CompareVerificationData();
    //----erase EC----------
    WriteSecurityMem(0x0, 0xff);
    //----read SM-----------
    ReadSecurityData();
        //PrintStr_Uart0("PSC are!!\n",0xff); 
        //PrintfHexByUart0(ReadDataBuf,4);
        //PrintStr_Uart0("\n",0xff); 
    if (ReadDataBuf[0] == 0x07 && ReadDataBuf[1] == RefData[0] &&
        ReadDataBuf[2] == RefData[1] && ReadDataBuf[3]==RefData[2]) {
       
        return TRUE;   
    } else {
        return FALSE;
    }
    #endif
    return false;
}
#endif

/*******************************************************************
** 函数名:     HAL_IC_InitDrv
** 函数描述:   IC卡模块初始化
** 参数:       无
** 返回:       无
********************************************************************/
void HAL_IC_InitDrv(void)
{
    PinsConfig();
}

/*******************************************************************
** 函数名:     HAL_IC_ReadSLE4442
** 函数描述:   读取SLE4442卡数据
** 参数:       [in]  addr:   地址
**             [out] dptr：  数据指针
**             [in]  maxlen：数据长度
** 返回:       读取到数据长度
********************************************************************/
INT16U HAL_IC_ReadSLE4442(INT16U addr, INT8U *dptr, INT16U maxlen)
{
    INT16U i, len;
    INT32U temp;
    
    PinsConfig();
    temp = AnswerToReset();                                                    /* 复位IC卡，并读取头4个字节数据 */
    for (i = 0; i < 4; i++) {
        if ((temp & (0xFF << (i * 8))) == (ATR_HEAD_DATA & (0xFF << (i * 8)))) {
            break;
        }
    }
    
    if (i < 4) {                                                               /* 验证IC卡的卡型 */
        len = ReadMainMemoryData(0x00, dptr, maxlen);
        return len;
    } else {
        return 0;
    }
}

/*******************************************************************
** 函数名:     HAL_IC_WriteSLE4442
** 函数描述:   写SLE4442卡数据
** 参数:       [in] addr: 地址
**             [in] sptr：数据指针
**             [in] slen：数据长度
** 返回:       写入数据长度
********************************************************************/
INT16U HAL_IC_WriteSLE4442(INT16U addr, INT8U *sptr, INT16U slen)
{
    return 0;
}

/*******************************************************************
** 函数名:     HAL_IC_Read24CXX
** 函数描述:   读取24CXX卡数据
** 参数:       [in]  addr:   地址
**             [out] dptr：  数据指针
**             [in]  maxlen：数据长度
** 返回:       读取到数据长度
********************************************************************/
INT16U HAL_IC_Read24CXX(INT16U addr, INT8U *dptr, INT16U maxlen)
{
    INT16U readlen = 0;
    
    ST_I2C_OpenCom(I2C_COM_IC);
    if (ST_I2C_SetReadAddr(I2C_COM_IC, 0xA0, 0)) {
        readlen = ST_I2C_ReadData(I2C_COM_IC, 0xA1, dptr, maxlen);
    }
    ST_I2C_CloseCom(I2C_COM_IC);
    return readlen;
}

/*******************************************************************
** 函数名:     HAL_IC_WriteData
** 函数描述:   写卡数据
** 参数:       [in] offset: 偏移地址
**             [in] sptr：  数据指针
**             [in] slen:   数据长度
** 返回:       写入数据长度
********************************************************************/
INT32U HAL_IC_WriteData(INT32U offset, INT8U *sptr, INT16U slen)
{
    return 0;
}

//#endif

/************************** (C) COPYRIGHT 2010 XIAMEN YAXON.LTD *******************END OF FILE******/


