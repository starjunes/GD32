/******************************************************************************
**
** 文件: dal_sd2058.c
** 版权: (c) 2005-2021 厦门雅迅网络股份有限公司
** 描述: 外部RTC sd2058驱动
** 创建: lbx 2021.03.18
**
*********************************************************************************/
#include "dal_include.h"
#include "hal_exrtc_sd2058_drv.h"
#include "debug_print.h"
#include "dal_gsen_drv.h"


/********************************************************************************
* 定义配置参数
*********************************************************************************/
#define SD2058_REG_SEC          0x00	//秒寄存器
#define SD2058_REG_MIN          0x01	//分钟寄存器
#define SD2058_REG_ALARMEN      0x0E	//报警允许寄存器
#define SD2058_REG_CTR1         0x0F	//控制寄存器1
#define SD2058_REG_CTR2         0x10	//控制寄存器2
#define SD2058_REG_CTR3         0x11	//控制寄存器3
#define SD2058_REG_ADJUST       0x12	//时间调整寄存器
#define SD2058_REG_COUNTDOWN    0x13	//倒计时寄存器
#define SD2058_REG_RAMBASE		0x14	//RAM寄存器基地址


/********************************************************************************
* 定义模块数据结构
*********************************************************************************/
static INT8U s_sd2058_rst = 0;  /* 1首次上电 */

/********************************************************************************
*                      本地接口实现
*********************************************************************************/

//hex转BCD
INT8U sd_Hex2BCD(INT8U hex)
{
    INT8U tmpl,tmph;
    tmpl = hex % 10;
    tmph = (hex / 10)<<4;
    return (tmph|tmpl);
}

//BCD转hex
static INT8U sd_BCD2Hex(INT8U bcd)
{
    INT8U tmp;
    tmp = (bcd>>4)*10 + (bcd & 0x0f);
    return tmp;
}

/*****************************************************************************
**  函数: HAL_sd2058_write
**  描述: 写数据
**  参数: [in]regaddr 寄存器地址
			  buf     写数据内容
			  buflen  数据长度
**  返回: TRUE 成功
          FALSE 失败
*****************************************************************************/
BOOLEAN HAL_sd2058_write(INT8U regaddr,INT8U* buf,INT8U buflen)
{
	BOOLEAN ret;
	/* 解锁 */
    sd_WriteByte(SD2058_REG_CTR2, 0x80);
    sd_WriteByte(SD2058_REG_CTR1, 0x84);
	
	ret = sd_WriteData(regaddr, buf, buflen);

	/* 上锁 */
    sd_WriteByte(SD2058_REG_CTR1, 0x00);
    sd_WriteByte(SD2058_REG_CTR2, 0x00);
	return ret;
}
/*****************************************************************************
**  函数: HAL_sd2058_writebyte
**  描述: 写数据
**  参数: [in]regaddr 寄存器地址
			  buf     写数据内容
**  返回: TRUE 成功
          FALSE 失败
*****************************************************************************/
BOOLEAN HAL_sd2058_writebyte(INT8U regaddr,INT8U rbuf)
{
	return HAL_sd2058_write(regaddr, &rbuf, 1);
}
/*****************************************************************************
**  函数: HAL_sd2058_read
**  描述: 读数据
**  参数: [in]regaddr 寄存器地址
			  buf     读数据内容缓存
			  buflen  数据长度
**  返回: TRUE 成功
          FALSE 失败
*****************************************************************************/
BOOLEAN HAL_sd2058_read(INT8U regaddr,INT8U* buf,INT8U buflen)
{
	return sd_ReadData(regaddr, buf, buflen);
}

/*****************************************************************************
**  函数: HAL_sd2058_readbyte
**  描述: 读数据
**  参数: [in]regaddr 寄存器地址
			  buf     读数据内容
**  返回: TRUE 成功
          FALSE 失败
*****************************************************************************/
BOOLEAN HAL_sd2058_readbyte(INT8U regaddr,INT8U *rbuf)
{
	return HAL_sd2058_read(regaddr, rbuf, 1);
}


/*****************************************************************************
**  函数: HAL_sd2058_SetCalendar
**  描述: 设置日期
**  参数: [in]data 设置日期数据缓存
**  返回: TRUE 成功
          FALSE 失败
*****************************************************************************/
BOOLEAN HAL_sd2058_SetCalendar(const INT8U* data)
{
	BOOLEAN ret;
    INT8U buf[7];


    /* 解锁 */
    sd_WriteByte(SD2058_REG_CTR2, 0x80);
   	sd_WriteByte(SD2058_REG_CTR1, 0x84);

    buf[0] = sd_Hex2BCD(data[0]);           /* 秒 */
    buf[1] = sd_Hex2BCD(data[1]);           /* 分 */
    buf[2] = sd_Hex2BCD(data[2]) | 0x80;    /* 时 24小时制 */
    buf[3] = sd_Hex2BCD(data[3]);              /* 星期 */
    buf[4] = sd_Hex2BCD(data[4]);           /* 日 */
    buf[5] = sd_Hex2BCD(data[5]);           /* 月 */
    buf[6] = sd_Hex2BCD(data[6]);           /* 年 */
    ret = sd_WriteData(SD2058_REG_SEC, buf, 7);
    if (ret == TRUE) {
        s_sd2058_rst = 0;
    } else {
        #if DEBUG_EX_RTC > 0
        Debug_SysPrint("rtc sd2058 设置时间失败\r\n");
        #endif
    }
    sd_WriteByte(SD2058_REG_ADJUST, 0x00);


    /* 上锁 */
    sd_WriteByte(SD2058_REG_CTR1, 0x00);
    sd_WriteByte(SD2058_REG_CTR2, 0x00);


    return ret;
}

/*****************************************************************************
**  函数: HAL_sd2058_ReadCalendar
**  描述: 读取日期
**  参数: [in]data 读取日期数据缓存
**  返回: TRUE 成功
          FALSE 失败
*****************************************************************************/
BOOLEAN HAL_sd2058_ReadCalendar(INT8U* data)
{
    BOOLEAN ret;
    INT8U reg_data;
    INT8U buf[7];


    if (sd_ReadByte(SD2058_REG_CTR1, &reg_data) && (reg_data & 0x01)) {
        s_sd2058_rst = 1;
    }
    if (s_sd2058_rst) {
        #if DEBUG_EX_RTC > 0
        Debug_SysPrint("rtc sd2058 发生断电, 需要重新设置时间\r\n");
        #endif

        return FALSE;
    }


    ret = sd_ReadData(SD2058_REG_SEC, buf, 7);
    if (ret == TRUE) {
        data[0] = sd_BCD2Hex(buf[0] & 0x7f);            /* 秒 */
        data[1] = sd_BCD2Hex(buf[1] & 0x7f);            /* 分 */
        data[2] = sd_BCD2Hex(buf[2] & 0x3f);            /* 时 */
        data[3] = sd_BCD2Hex(buf[3] & 0x07);            /* 星期 */
        if (data[3] == 0) {
            data[3] = 7;
        }
        data[4] = sd_BCD2Hex(buf[4] & 0x3f);            /* 日 */
        data[5] = sd_BCD2Hex(buf[5] & 0x1f);            /* 月 */
        data[6] = sd_BCD2Hex(buf[6] & 0xff);            /* 年 */
    } else {
        #if DEBUG_EX_RTC > 0
        Debug_SysPrint("rtc sd2058 读取时间失败\r\n");
        #endif
    }

    return ret;
}

/*****************************************************************************
**  函数: HAL_sd2058_Open
**  描述: 打开
**  参数: none
**  返回: TRUE 成功
          FALSE 失败
*****************************************************************************/
BOOLEAN HAL_sd2058_Open(void)
{
    INT8U reg_data;


    if (!sd_ReadByte(SD2058_REG_SEC, &reg_data)
        || !sd_ReadByte(SD2058_REG_MIN, &reg_data)) {
        #if DEBUG_EX_RTC > 0
        Debug_SysPrint("rtc sd2058 识别失败\r\n");
        #endif
        return FALSE;
    }


    if (sd_ReadByte(SD2058_REG_CTR1, &reg_data) && (reg_data & 0x01)) {
        s_sd2058_rst = 1;
    }
    if (s_sd2058_rst) {
        #if DEBUG_EX_RTC > 0
        Debug_SysPrint("rtc sd2058 发生断电, 需要重新设置时间\r\n");
        #endif
    }


    /* 解锁 */
    sd_WriteByte(SD2058_REG_CTR2, 0x80);
    sd_WriteByte(SD2058_REG_CTR1, 0x84);


    if (sd_ReadByte(SD2058_REG_ALARMEN, &reg_data) && reg_data != 0x00) {
        #if DEBUG_EX_RTC > 0
        Debug_SysPrint("rtc sd2058 0e[%x]\r\n", reg_data);
        #endif
        sd_WriteByte(SD2058_REG_ALARMEN, 0x00);
    }
    if (sd_ReadByte(SD2058_REG_CTR1, &reg_data) && reg_data != 0x84) {
        #if DEBUG_EX_RTC > 0
        Debug_SysPrint("rtc sd2058 0f[%x]\r\n", reg_data);
        #endif
        sd_WriteByte(SD2058_REG_CTR1, 0x84);
    }
    if (sd_ReadByte(SD2058_REG_CTR2, &reg_data) && reg_data != 0x80) {
        #if DEBUG_EX_RTC > 0
        Debug_SysPrint("rtc sd2058 10[%x]\r\n", reg_data);
        #endif
        sd_WriteByte(SD2058_REG_CTR2, 0x80);
    }
    if (sd_ReadByte(SD2058_REG_CTR3, &reg_data) && reg_data != 0x00) {    /* 打开时钟输出 */
        #if DEBUG_EX_RTC > 0
        Debug_SysPrint("rtc sd2058 11[%x]\r\n", reg_data);
        #endif
        sd_WriteByte(SD2058_REG_CTR3, 0x00);
    }
    if (sd_ReadByte(SD2058_REG_ADJUST, &reg_data) && reg_data != 0x00) {
        #if DEBUG_EX_RTC > 0
        Debug_SysPrint("rtc sd2058 12[%x]\r\n", reg_data);
        #endif
        sd_WriteByte(SD2058_REG_ADJUST, 0x00);
    }
    if (sd_ReadByte(SD2058_REG_COUNTDOWN, &reg_data) && reg_data != 0x00) {
        #if DEBUG_EX_RTC > 0
        Debug_SysPrint("rtc sd2058 13[%x]\r\n", reg_data);
        #endif
        sd_WriteByte(SD2058_REG_COUNTDOWN, 0x00);
    }


    /* 上锁 */
    sd_WriteByte(SD2058_REG_CTR1, 0x00);
    sd_WriteByte(SD2058_REG_CTR2, 0x00);


    #if DEBUG_EX_RTC > 0
    Debug_SysPrint("rtc sd2058 初始化成功\r\n");
    #endif


    return TRUE;
}


