/********************************************************************************
**
** 文件名:     yx_mmi_com.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现dvr外设协议匹配，数据缓存
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2011/01/21 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "yx_include.h"
#include "yx_dym_drv.h"
#include "yx_loopbuf.h"
#include "st_uart_drv.h"
#include "dal_gpio_cfg.h"
#include "yx_debug.h"

#if EN_MMI > 0
#include "yx_mmi_drv.h"


/*
********************************************************************************
* 宏定义
********************************************************************************
*/
#define MAX_COM              1          /* 用于匹配的通道数 */
#define PE_TYPE_DEF          PE_TYPE_YXMMI
#define SIZE_RECV            512
#define SIZE_HDL             30
#define UART_COM_MMI         UART_COM_0


/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    INT8U step;
    //INT8U curtype;
    //INT8U curcom;                      /* 已匹配的统一通道编号 */
    //INT8U bakcom;                      /* 备份已匹配的统一通道编号，为curcom的反码 */
    //INT8U rlen[MAX_COM];
    //INT8U rbuf[MAX_COM][SIZE_HDL];
    //LOOP_BUF_T recvround;
   // INT8U recvbuf[SIZE_RECV];
} RCB_T;


/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
//static RCB_T s_rcb;


/*******************************************************************
** 函数名:     YX_MMI_PullUp
** 函数描述:   打开电源
** 参数:       无
** 返回:       无
********************************************************************/
void YX_MMI_PullUp(void)
{
    /*OS_ASSERT((s_rcb.curcom == (INT8U)(~s_rcb.bakcom)), RETURN_VOID);
    
    if (s_rcb.curcom != 0xff) {
        YX_PE_CtlCom(s_rcb.curcom, true);
    }*/
    
    YX_MMI_PowerOn();
}

/*******************************************************************
** 函数名:     YX_MMI_PullDown
** 函数描述:   关掉电源
** 参数:       无
** 返回:       无
********************************************************************/
void YX_MMI_PullDown(void)
{
    /*OS_ASSERT((s_rcb.curcom == (INT8U)(~s_rcb.bakcom)), RETURN_VOID);
    
    if (s_rcb.curcom != 0xff) {
        YX_PE_CtlCom(s_rcb.curcom, false);
    }*/
    
    YX_MMI_PowerOff();
}

/*******************************************************************
** 函数名:     YX_MMI_CfgBaud
** 函数描述:   MMI通道的波特率设置
** 参数:       [in] baud:   波特率
**             [in] databit:数据位,见PE_DATABIT_E
**             [in] stopbit:停止位,见PE_STOPBIT_E
**             [in] chkbit: 校验位,见PE_CHKBIT_E
** 返回:       无
********************************************************************/
void YX_MMI_CfgBaud(INT32U baud, PE_DATABIT_E databit, PE_STOPBIT_E stopbit, PE_CHKBIT_E chkbit)
{
    UART_CFG_T cfg;
    
    /*OS_ASSERT((s_rcb.curcom == (INT8U)(~s_rcb.bakcom)), RETURN_VOID);
    
    #if DEBUG_MMI > 0
    printf_com("<配置MMI占用通道的波特率>\r\n");
    #endif
    
    if (s_rcb.curcom != 0xff) {
        YX_PE_CfgCom(s_rcb.curcom, baud, databit, stopbit, chkbit);
    }*/
    
    cfg.com     = UART_COM_MMI;
    cfg.baud    = baud;
    cfg.parity  = chkbit;
    cfg.databit = databit;
    cfg.stopbit = stopbit;
    
    cfg.rx_fcm = UART_FCM_NULL;
    cfg.tx_fcm = UART_FCM_NULL;
    
    cfg.rx_len = 512;
    cfg.tx_len = 768;
    
    ST_UART_OpenUart(&cfg);
    
    #if DEBUG_MMI > 0
    printf_com("<配置MMI占用通道的波特率>\r\n");
    #endif
}

/*******************************************************************
** 函数名:     YX_MMI_CloseCom
** 函数描述:   关闭MMI通道串口
** 参数:       无
** 返回:       无
********************************************************************/
void YX_MMI_CloseCom(void)
{
    ST_UART_CloseUart(UART_COM_MMI);
    //ST_UART_CloseUart(UART_COM_1);
}

/*******************************************************************
** 函数名:     YX_MMI_Reset
** 函数描述:   复位通道为共享状态
** 参数:       无
** 返回:       无
********************************************************************/
void YX_MMI_Reset(void)
{
#if 0
    INT8U i, com;
    
    #if DEBUG_MMI > 0
    printf_com("<复位MMI占用的通道(%x)为共享>\r\n", s_rcb.curcom);
    #endif

    OS_ASSERT((s_rcb.curcom == (INT8U)(~s_rcb.bakcom)), RETURN_VOID);
    
    com = s_rcb.curcom;
    
    if (s_rcb.curcom != 0xff) {
        s_rcb.curcom = 0xff;
        s_rcb.bakcom = (INT8U)(~s_rcb.curcom);
        
        YX_PE_Reset(com, PE_TYPE_DEF);                                      /* 复位底层通道为共享 */
    }
    s_rcb.curtype = 0;
    YX_ResetRoundBuf(&s_rcb.recvround);
    
    for (i = 0; i < MAX_COM; i++) {                                            /* 清除通道，以便下次匹配 */
        s_rcb.rlen[i] = 0;
    }
#endif
}

/*******************************************************************
** 函数名:     YX_MMI_ResetBuf
** 函数描述:   复位通道接收缓存
** 参数:       无
** 返回:       无
********************************************************************/
void YX_MMI_ResetBuf(void)
{
#if 0
    #if DEBUG_MMI > 0
    printf_com("<复位MMI接收缓存,当前通道(%x)>\r\n", s_rcb.curcom);
    #endif
    
    OS_ASSERT((s_rcb.curcom == (INT8U)(~s_rcb.bakcom)), RETURN_VOID);
    
    if (s_rcb.curcom != 0xff) {
        YX_PE_ResetBuf(s_rcb.curcom, PE_TYPE_DEF);                             /* 复位底层通道接收缓存 */
    }
    YX_ResetRoundBuf(&s_rcb.recvround);
#endif
}

/*******************************************************************
** 函数名:     YX_MMI_UartLeftOfSendbuf
** 函数描述:   获取发送缓存剩余长度
** 参数:       无
** 返回:       剩余字节数
********************************************************************/
INT32U YX_MMI_UartLeftOfSendbuf(void)
{
    return ST_UART_LeftOfSendbuf(UART_COM_MMI);
}

/*******************************************************************
** 函数名:     YX_MMI_ReadByte
** 函数描述:   通道数据接收接口
** 参数:       无
** 返回:       数据，无数据返回－1
********************************************************************/
INT16S YX_MMI_ReadByte(void)
{
    //return YX_ReadRoundBuf(&s_rcb.recvround);
    return ST_UART_ReadChar(UART_COM_MMI);
}

/*******************************************************************
** 函数名:     YX_MMI_SendData
** 函数描述:   通道数据发送接口
** 参数:       [in] ptr: 数据指针
**             [in] len: 数据长度
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN YX_MMI_SendData(INT8U *ptr, INT16U len)
{
#if 0
    BOOLEAN result;
    
    #if DEBUG_MMI > 0 
    if ((ptr[4] != 0x40) && (ptr[4] != 0x41) && (ptr[4] != 0xda) && (ptr[4] != 0xE5) && 
        (ptr[4] != 0x96) && (ptr[4] != 0xa0) && (ptr[4] != 0xa1)) {        //屏蔽调度屏实时状态应答
            
       printf_com("<MMI发送,通道(%x), 长度(%d)：", s_rcb.curcom, len);
       printf_hex(ptr, len > 300 ? 300 : len);
       printf_com(">\r\n");
    }
    #endif
    
    result = false;
    
    if (s_rcb.curcom != 0xff) {
       result = YX_PE_SendData(s_rcb.curcom, PE_TYPE_DEF, ptr, len);
    }
    return result;
#endif
    
    #if DEBUG_MMI > 0 
    if (ptr[4] != UP_PE_CMD_REPORT_ODOPULSE 
       && ptr[4] != UP_PE_CMD_LINK_REQ 
       && ptr[4] != UP_PE_CMD_REPORT_SENSOR_STATUS
       && ptr[4] != UP_PE_CMD_REPORT_AD
       && ptr[4] != UP_PE_CMD_GPS_DATA_SEND) {
        printf_com("<MMI发送, 长度(%d):", len);
        printf_hex(ptr, len > 64 ? 64 : len);
        //printf_hex(ptr, len);
        printf_com(">\r\n");
    }
    #endif
    
    return ST_UART_WriteBlock(UART_COM_MMI, ptr, len);
}

#if 0
/*******************************************************************
** 函数名:     YX_MMI_WriteChar
** 函数描述:   底层通道上传接收到数据
** 参数:       [in] com: 统一的通道编号
**             [in] ch:  数据
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN YX_MMI_WriteChar(INT8U com, INT8U ch)
{
    INT8U no;
    INT8U *ptr;

    #if DEBUG_MMI > 0
    //printf_com("<MMI接收,通道(%x),数据:%02x>\r\n", com, ch);
    #endif
    
    if (s_rcb.curcom != 0xff) {                                                /* 已经匹配到串口通道 */
        if (s_rcb.curcom == com) {
            YX_WriteRoundBuf(&s_rcb.recvround, ch);
            return true;
        }
    }
    
    no = com;
    
    OS_ASSERT((no < MAX_COM), RETURN_FALSE);
    
    ptr = s_rcb.rbuf[no];
    
    if (s_rcb.rlen[no] == 0) {
        if (ch == g_dvr_rules.c_flags) {                                         /* 检测协议头 */
            ptr[0] = 0;
            s_rcb.rlen[no]++;
        }
    } else {
        if (ch == g_dvr_rules.c_flags) {
            if (s_rcb.rlen[no] > 1) {
                s_rcb.rlen[no] = YX_DeassembleByRules(ptr, ptr + 1, s_rcb.rlen[no] - 1, &g_dvr_rules);
                
                if (s_rcb.rlen[no] >= 4) {
                    if ((ptr[2] == PE_TYPE_YXMMI) || (ptr[2] == PE_TYPE_YX110R) || (ptr[2] == PE_TYPE_JTMMI)) {/* 先判断外设类型，提高效率 */
                        if (ptr[0] == YX_GetChkSum(ptr + 1, s_rcb.rlen[no] - 1)) {/* 计算校验和 */
                            s_rcb.curtype = ptr[2];
                            if (YX_PE_InformPaserSuccess(no, PE_TYPE_DEF)) {
                                s_rcb.rlen[no] = 0;
                                return false;
                            }
                        }
                    }
                }
                s_rcb.rlen[no] = 0;
            }
        } else {
            if (s_rcb.rlen[no] >= SIZE_HDL) {
                s_rcb.rlen[no] = 0;
            } else {
                ptr[s_rcb.rlen[no]++] = ch;
            }
        }
    }
    return true;
}

/*******************************************************************
** 函数名:     YX_MMI_Inform
** 函数描述:   底层通道状态变化通知函数
** 参数:       [in] com:     统一的通道编号
**             [in] type:    外设类型
**             [in] result:  结果，true－通道连接，false－通道断开
** 返回:       无
********************************************************************/
void YX_MMI_Inform(INT8U com, INT8U type, INT8U result)
{
    INT8U i;
    
    OS_ASSERT((type == PE_TYPE_DEF), RETURN_VOID);
    
    if (result == PE_INFO_CONNECT) {                                           /* 通道连接 */
        if (s_rcb.curcom != com) {
            #if DEBUG_MMI > 0
            printf_com("<MMI已连接,通道(%x)>\r\n", com);
            #endif
            
            YX_ResetRoundBuf(&s_rcb.recvround);
            s_rcb.curcom = com;
            s_rcb.bakcom = (INT8U)(~s_rcb.curcom);
        }
    } else if (result == PE_INFO_DISCONNECT) {                                 /* 通道断开 */
        if (s_rcb.curcom == com) {
            #if DEBUG_MMI > 0
            printf_com("<MMI已断开,通道(%x)>\r\n", com);
            #endif
            
            YX_ResetRoundBuf(&s_rcb.recvround);
            s_rcb.curcom = 0xff;
            s_rcb.bakcom = (INT8U)(~s_rcb.curcom);
        }
    }
    
    for (i = 0; i < MAX_COM; i++) {                                            /* 清除通道，以便下次匹配 */
        s_rcb.rlen[i] = 0;
    }
}
#endif

/*******************************************************************
** 函数名:     YX_MMI_InitCom
** 函数描述:   MMI通信通道初始化
** 参数:       无
** 返回:       无
********************************************************************/
void YX_MMI_InitCom(void)
{
    //YX_MEMSET(&s_rcb, 0, sizeof(s_rcb));
    
    //s_rcb.curcom = 0xff;
    //s_rcb.bakcom = (INT8U)(~s_rcb.curcom);
    
    //YX_InitRoundBuf(&s_rcb.recvround, s_rcb.recvbuf, sizeof(s_rcb.recvbuf), 0);
}


#endif


