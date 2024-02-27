/**************************************************************************************************
**                                                                                               **
**  文件名称:  APP_update.c                                                                      **
**  版权所有:  CopyRight @ Xiamen Yaxon NetWork CO.LTD. 2011                                     **
**  创建信息:  2011-12-23 By clt: 创建本文件                                                     **
**  文件描述:  无线升级应用层接口函数                                                            **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/
#include "yx_includes.h"
#include  "app_update.h"
#include  "appmain.h"
#include  "dal_flash.h"
#include  "tools.h"
//#include  "protocaldata_send.h"
//#include  "systemack_handle.h"
#include  "s_flash.h"
#include  "database.h"
#if DEBUG_APP_UPDATE > 0
#include  "debug_print.h"
#endif
#include "yx_com_send.h"

#include "yx_version.h"
#include "port_plat.h"
#include "port_timer.h"

#if EN_DEBUG > 1
#undef DEBUG_APP_UPDATE
#define DEBUG_APP_UPDATE 1
#endif

#if SWITCH_DEVICE == GK_110R
/*************************************************************************************************/
/*数据帧格式枚举(若包含在头文件中则与数据导出的一个枚举类型成员名称相同,同在AppMain.c中被包含)   */
/*************************************************************************************************/
typedef enum {
    FRAME_TYPE   = 0x00,                                                       /* 数据帧: 传输类型 1个字节 */
    FRAME_STATUS = 0x01,                                                       /* 数据帧: 传输状态 1个字节 */
    FRAME_SEQUE  = 0x02,                                                       /* 数据帧: 包序号   2个字节 */
    FRAME_LEN    = 0x04,                                                       /* 数据帧: 数据长度 2个字节 */
    FRAME_DATA   = 0x06,                                                       /* 数据帧: 数据内容         */
} UPDATE_FRAME_E;
/*===============================================================================================*/

#define  UPDATE_PROC_ING        0x01                                           /* 数据包是中间的一包，数据导出中   */
#define  UPDATE_PROC_END        0x02                                           /* 数据包是最后的一包，数据导出结束 */
#define  ONEPACKET_MAXLEN       1024                                           /* 一包数据的最长长度512            */



UPDATE_STATUS_E s_upstatus = UPSTATUS_NONE;                                    /* 初始化升级阶段为未进入升级流程      */

static INT8U    s_cnt_error;                                                   /* 用于计数出错的次数，最多不超过3次 */
static INT16U   s_num_packets;                                                 /* 包序号           */
static INT32U   s_newchcksum = 0xffffffff;                                     /* 新程序代码校验和 */
static INT32U   s_codelen;                                                     /* 记录接收到的数据帧的长度的累加和 */
static INT8U    s_newchck_a;
static INT8U    s_newchck_b;

static INT8U    s_chcksum_a;                                                   /* 升级的程序代码的A校验和 */
static INT8U    s_chcksum_b;                                                   /* 升级的程序代码的B校验和 */
static INT32U   s_filelen;                                                     /* 升级的程序代码的长度    */
static INT32U   s_rstmcu_tmr;

/**************************************************************************************************
 **  函数名称:  Update_RstMcuTmr
 **  功能描述:  升级文件存储完毕，软件复位MCU
 **  输入参数:
 **  返回参数:
 **************************************************************************************************/
static void	Update_RstMcuTmr(void *pdata)
{
	#if DEBUG_WIRELESS > 0
		debug_printf("复位\r\n");
	#endif

	PORT_ResetCPU();                       //远程升级协议解析完成，并成功将数据写入CFlash后，复位MCU
}

/********************************************************************************
* Function Name  : Chartolong_Small
* Description    : 小端模式 
* Arguments      : [in] dest
*                : [in] src
* Return         : None
********************************************************************************/
static void Chartolong_Small(INT32U *dest, INT8U *src)
{
    *dest  = (INT32U)(src[0]);
    *dest |= (INT32U)(src[1]) << 8;
    *dest |= (INT32U)(src[2]) << 16;
    *dest |= (INT32U)(src[3]) << 24;
}

/**************************************************************************************************
**  函数名称:  UpDateMode_Init
**  功能描述:  升级模块初始化，主要是将APROM的后面的备份区配置成DATA FLASH属性，才可以进行数据擦写
**  输入参数:  无
**  输出参数:  无
**  返回参数:  无
**************************************************************************************************/
void UpDateMode_Init(void)
{
    s_upstatus    = UPSTATUS_NONE;
    s_cnt_error   = 0;                                                   
    s_num_packets = 0;                                                 
    s_newchcksum  = 0xffffffff;                                     
    s_codelen     = 0;
    s_newchck_a   = 0;
    s_newchck_b   = 0;
    s_chcksum_a   = 0;                                              
    s_chcksum_b   = 0;                                             
    s_filelen     = 0; 
    s_rstmcu_tmr = OS_InstallTmr(TSK_ID_OPT, 0, Update_RstMcuTmr);
    OS_StopTmr(s_rstmcu_tmr);    
}

/**************************************************************************************************
**  函数名称:  StartUpdateReq_Hdl
**  功能描述:  请求开始进入无线升级流程处理函数
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
void StartUpdateReq_Hdl(INT8U mancode, INT8U command, INT8U *userdata, INT16U userdatalen) 
{
    INT8U  temp, temp1;
    INT8U  ack;
    INT8U  start;
    INT16U i;
    BOOLEAN flag_update;
    
    mancode = mancode;
    command = command;
    userdatalen = userdatalen;

    temp = userdata[0];                                                        /* 外设编码 */

    #if DEBUG_APP_UPDATE > 0
    debug_printf_dir("<**** (1)StartUpdateReq_Hdl ****>\r\n");
    #endif
    
    if (DEV_CODE != temp) {
        ack = 0x03;    /* 外设类型错误 */
        s_upstatus = UPSTATUS_NONE;
        YX_COM_DirSend(UPDATE_START_REQ_ACK, &ack, 1);
        return;
    }
	
    temp = userdata[1];                                                        /* 文件类型：HEX或者BIN */
    if (BIN_TYPE != temp) {
        ack = 0x04;    /* 文件类型错误 */
        s_upstatus = UPSTATUS_NONE;
        YX_COM_DirSend(UPDATE_START_REQ_ACK, &ack, 1);
        return;
    }

    /* 替换个数 */
    temp = userdata[2];
    start = 3;
    flag_update = FALSE;
	
    for (; temp > 0; temp--) {
        temp1 = userdata[start++];
        if ((STR_EQUAL == AcmpString(FALSE, APP_VERSION_NUM, &userdata[start], strlen(APP_VERSION_NUM), temp1))
            || (STR_EQUAL == AcmpString(FALSE, "yx-test", &userdata[start], strlen("yx-test"), temp1))
            || (STR_EQUAL == AcmpString(FALSE, APP_VERSION_TYPE, &userdata[start], strlen(APP_VERSION_TYPE), temp1))) {
            flag_update = TRUE;
        }
        start += temp1;
    }
    if (!flag_update) {
        ack = 0x05;                                                  /* 本身版本号不属于被替代的版本号失败 */
        s_upstatus = UPSTATUS_NONE;
        YX_COM_DirSend(UPDATE_START_REQ_ACK, &ack, 1);
        return;
    }
    temp1 = userdata[start++];                                       /* 升级版本号长度 */
    start += temp1;
    s_chcksum_a = userdata[start++];                                           /* A校验和 */
    s_chcksum_b = userdata[start++];                                           /* B校验和 */
    Chartolong_Small(&s_filelen, &userdata[start]);                            /* 文件长度--小端模式 */

    Flash_Unlock();

    #if DEBUG_APP_UPDATE > 0
    debug_printf_dir("<**** (1)StartUpdateReq_Hdl:擦除pp参数 ****>\r\n");
    #endif

    ClearWatchdog();
    if (!Flash_ErasePage(E_PARA1_PAGE)) {                                  /* 擦除失败，返回不可进入流程 */
        Flash_Lock(); 
        ack = 0x02;    /* 擦除失败 */
        s_upstatus = UPSTATUS_NONE;
        YX_COM_DirSend(UPDATE_START_REQ_ACK, &ack, 1);
        return;
    }
    
    s_upstatus = UPSTATUS_START;
    s_cnt_error = 0;
    s_codelen = 0;
    s_newchcksum = 0xffffffff;
    s_num_packets = 0;
    s_newchck_a = 0;
    s_newchck_b = 0;
    ack = 0x01;    /* 请求成功 */
    YX_COM_DirSend(UPDATE_START_REQ_ACK, &ack, 1);
    
    #if DEBUG_APP_UPDATE > 0
    debug_printf_dir("<**** (1)StartUpdateReq_Hdl:擦除代码备份区:start ****>\r\n");
    #endif
    
    #if EN_CHECKFLASH
    setflashflag(); 
    #endif
    /* 要循环到包括E_PARA1_PAGE，不然在接收函数的最后一包中会出现写参数错误，因为那页从没被擦除过，就会写失败 */
    for (i = B_BACK_PAGE; i <= E_BACK_PAGE; i++){                          /* 擦除data flash */
        BlockErase(i);
        ClearWatchdog();
    }
    #if EN_CHECKFLASH
    clearflashflag();
    #endif
    
    #if DEBUG_APP_UPDATE > 0
    debug_printf_dir("<**** (1)StartUpdateReq_Hdl:擦除代码备份区:end ****>\r\n");
    #endif
}

/**************************************************************************************************
**  函数名称:  DataRecv_ErrorHdl
**  功能描述:  用于处理在数据传输过程遇到需要返回，回馈信息的时候统一处理函数
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
static void DataRecv_ReturnHdl(UPDATE_RESULE_E resulttype,  INT8U* ackdata)
{
    INT8U ack[5];

    ack[0] = ackdata[0];
    ack[1] = ackdata[1];
    ack[2] = ackdata[2];
    ack[3] = ackdata[3];
    ack[4] = (INT8U)resulttype;
    s_upstatus = UPSTATUS_NONE;
    s_cnt_error = 0;
    s_num_packets = 0;
    s_newchcksum = 0xffffffff;
    s_codelen = 0;
    s_newchck_a = 0;
    s_newchck_b = 0;
    s_chcksum_a = 0;                                               /* 升级的程序代码的A校验和 */
    s_chcksum_b = 0;                                               /* 升级的程序代码的A校验和 */
    s_filelen = 0;                                                 /* 升级的程序代码的长度 */
    
    //RequestProtocolDataSend(NORMAL, UPDATE_DATA_RECV_ACK, ack, 5);
    YX_COM_DirSend(UPDATE_DATA_RECV_ACK, ack, 5);
}

/**************************************************************************************************
**  函数名称:  NextPacketOverTime
**  功能描述:  下一包超时处理函数：关闭文件，提醒数据可能不完整。
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
static void NextPacketOverTime(void)
{
    INT8U ack[4];

    ack[0] = 0x01;
    Shorttochar(&ack[1], s_num_packets);
    ack[3] = (INT8U)UP_RESULT_UNKNOWERROR;
    s_upstatus = UPSTATUS_NONE;
    s_cnt_error = 0;
    s_num_packets = 0;
    s_newchcksum = 0xffffffff;
    s_codelen = 0;
    //RequestProtocolDataSend(NORMAL, UPDATE_DATA_RECV_ACK, ack, 4);
    YX_COM_DirSend(UPDATE_DATA_RECV_ACK, ack, 4);
}

/**************************************************************************************************
**  函数名称:  GetChksum_Flash
**  功能描述:  获取校验和
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
static void GetChksum_Flash(INT32U addr, INT32U len)
{
    INT32U i;
    INT16U temp_o, temp_n;
    INT8U data;

    s_newchck_a = 0;
    s_newchck_b = 0;
    temp_o = 0;
    temp_n = 0;
	
    for (i = 0; i < len; i++) {                                      /* 计算原码的校验和 */
        AbsRead(&data, 1, addr++);
        
        temp_o += (INT8U)data;
        s_newchck_a = temp_o & 0xff;
        if (temp_o >= 0x100) {
            s_newchck_a++;
            temp_o = s_newchck_a;
        }

        temp_n += (INT8U)(~data);
        s_newchck_b = temp_n & 0xff;
        if (temp_n >= 0x100) {
            s_newchck_b++;
            temp_n = s_newchck_b;
        }
        
        if ((i & 0x07ff) == 0) {                                     /* 定时清狗 */
            ClearWatchdog();
        }
    }

    s_newchck_a++;
    s_newchck_b++;
}

/**************************************************************************************************
**  函数名称:  UpdateDadaRecv_Hdl
**  功能描述:  无线升级数据接收处理函数
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
void UpdateDadaRecv_Hdl(INT8U mancode, INT8U command, INT8U *userdata, INT16U userdatalen) 
{
    INT8U   ack[5];
    INT16U  i;
    INT16U  temppackets;
    INT16U  packetlen;
    INT32U  addr;
    INT32U  para[PARA_MAX];
    INT8U   buffer[ONEPACKET_MAXLEN];
    INT16U  lentemp;
    INT8U   num;

    mancode = mancode;
    command = command;

    ack[0] = userdata[FRAME_TYPE];      /* 文件类型 */
    ack[1] = userdata[FRAME_STATUS];    /* 获取传输状态 */
    ack[2] = userdata[FRAME_SEQUE];     /* 获取包序号 2个字节 */
    ack[3] = userdata[FRAME_SEQUE + 1];
    
    if (UPSTATUS_START != s_upstatus) {                                        /* 状态不对 */
        DataRecv_ReturnHdl(UP_RESULT_BUSY, ack);                               /* 发设备忙碌，终止 */ 
        return;
    }
    if (s_num_packets != 0) {                                                  /* 第一包不需要清重发，中间过程都需要清 */
        //ConfirmSend(UPDATE_DATA_RECV_ACK);                                     /* 清除重发机制 */
    }
    //ConfirmAck(UPDATE_DATA_RECV_ACK);                                          /* 清除上一帧的超时机制 */
    /* 包序号 */
    Chartoshort(&temppackets, &userdata[FRAME_SEQUE]);

    #if DEBUG_APP_UPDATE > 0
    debug_printf_dir("<**** (1)UpdateDadaRecv_Hdl:包序号:%d ****>\r\n",temppackets);
    #endif
    
    ClearWatchdog();
    if ((ack[1] == UPDATE_PROC_END) && (s_num_packets != 0)) {                 /* 最后一包 */
			
        if (s_codelen != s_filelen) {    /* 长度与和与原先的不一样 */
            #if DEBUG_APP_UPDATE > 0
            Debug_SysPrint("s_codelen != s_filelen\r\n");
            Debug_SysPrint("s_codelen = %x, s_filelen = %x\r\n", s_codelen, s_filelen);
            #endif
            DataRecv_ReturnHdl(UP_RESULT_UNKNOWERROR, ack); 
            return;
        }
        /* 校验计算 */
        GetChksum_Flash((INT32U)B_BACK_PAGE * s_switch_flash.block_nsize, s_codelen);
        #if DEBUG_APP_UPDATE > 0
        Debug_SysPrint("s_newchck_a = %x\r\n", s_newchck_a);
        Debug_SysPrint("s_newchck_b = %x\r\n", s_newchck_b);
        #endif
        
        if ((s_newchck_a != s_chcksum_a) || (s_newchck_b != s_chcksum_b)) {    /* 校验和与原先的不一样 */
            #if DEBUG_APP_UPDATE > 0
            Debug_SysPrint("\r\ns_newchck_a != s_chcksum_a) || (s_newchck_b != s_chcksum_b\r\n");
            Debug_SysPrint("s_newchck_a = %x, s_chcksum_a = %x\r\n", s_newchck_a, s_chcksum_a);
            Debug_SysPrint("s_newchck_b = %x, s_chcksum_b = %x\r\n", s_newchck_b, s_chcksum_b);
            Debug_SysPrint("s_codelen = %x, s_filelen = %x\r\n", s_codelen, s_filelen);
            #endif
            DataRecv_ReturnHdl(UP_RESULT_UNKNOWERROR, ack); 
            return;
        }
		Flash_Unlock();
        /*更新参数区*/
        addr = GetFlashAbsAddr(B_PARA1_PAGE, 0);                               /* 获取参数区地址 */
        for (i = 0; i < PARA_MAX; i++) {
            para[i] = (*(volatile INT32U *)(addr + i * 4));
        }

        if (0xffffffff == s_newchcksum) {
            s_newchcksum = SPECIAL_CHCKSUM;                                    /* 如果校验和刚巧=0xffffffff，则赋值一个特定值 */
        }
        para[PARA_BACKCHKSUM] = s_newchcksum;                                  /* 更新参数 */
        para[PARA_BACKLEN] = s_codelen;
        para[PARA_MODE] = UPDATE_WIRELESS;
		Flash_ErasePage(E_PARA1_PAGE);
        #if DEBUG_APP_UPDATE > 0
        Debug_SysPrint("para %x,%x,%x\r\n",para[PARA_BACKCHKSUM],para[PARA_BACKLEN],para[PARA_MODE]);
        #endif
        for (i = 0; i < PARA_MAX; i++) {                                       /* 将参数回写回参数区 */
            ClearWatchdog();
            if (FALSE == Flash_WriteWord((addr + i * 4), para[i]) ) {
                Flash_Lock();
                #if DEBUG_APP_UPDATE > 0
                Debug_SysPrint("para write err\r\n");
                #endif
                DataRecv_ReturnHdl(UP_RESULT_UNKNOWERROR, ack);                /* 参数写入出错 */
                return;
            }
        }

		#if DEBUG_APP_UPDATE > 0
        for (i = 0; i < PARA_MAX; i++) {                                       /* 将参数回写回参数区 */
            ClearWatchdog();
            para[i] = (*(volatile INT32U *)(addr + i * 4));
			debug_printf("para[%d]:%d  ",i, para[i]);
        }
        #endif

        Flash_Lock();
        #if DEBUG_APP_UPDATE > 0
        Debug_SysPrint("<**** 复位mcu ****>\r\n");
        #endif        
        DataRecv_ReturnHdl(UP_RESULT_END, ack);                                /* 返回下载成功结束 */
        Call_Circulate(1000);
        PORT_ResetCPU();
        //mmi_resetinform(RESET_INITIATIVE, (INT8U *)__FILE__ + strlen(__FILE__) - FILENAMELEN, __LINE__, ERR_ID_INT);
        //Turnoff_ReStartDevice();
        return;
    }

    /****************以下都属于中间包的过程......... */
      /* 1、判断如果是上一包的重发，由于之前已经写成功了，直接返回成功 */
    if (temppackets == s_num_packets) {                                        /* 判断是否是上一包重发 */  
        ack[4] = (INT8U)UP_RESULT_SUCESS; 
        s_cnt_error = 0;
        //RequestProtocolDataSend(NORMAL, UPDATE_DATA_RECV_ACK, ack, 5);         /* 中间包过程中，需要有应答，返回导出一包成功 */
        //RegisterAckList(UPDATE_DATA_RECV_ACK, 10, NextPacketOverTime);         /* 注册超时无应答函数 */
        YX_COM_DirSend(UPDATE_DATA_RECV_ACK, ack, 5);
        #if DEBUG_APP_UPDATE > 0
        debug_printf_dir("<**** (1)UpdateDadaRecv_Hdl:包序号重复 ****>\r\n");
        #endif
        return;
    }
      /* 2、判断不是上一包的重发，进行正常的导出数据流程 */
    Chartoshort(&packetlen, &userdata[FRAME_LEN]);                             /* 获取数据包长度 */
    if ((packetlen > ONEPACKET_MAXLEN) || (packetlen % 4)) {                   /* 数据包长度过长或者没有双字对齐(4字节倍数) */
        s_cnt_error++;
        if (s_cnt_error < 3) {
            ack[4] = (INT8U)UP_RESULT_REJECT;
            YX_COM_DirSend(UPDATE_START_REQ_ACK, ack, 5);
        } else {
            #if DEBUG_APP_UPDATE > 0
            Debug_SysPrint("over 3 times\r\n");
            #endif
            Flash_Lock();
            DataRecv_ReturnHdl(UP_RESULT_UNKNOWERROR, ack);                    /* 错误超过三次，导出终止 */ 
        }
        #if DEBUG_APP_UPDATE > 0
        debug_printf_dir("<**** (1)UpdateDadaRecv_Hdl:包长度错误 ****>\r\n");
        #endif
        return;
    }
    if (packetlen == 0 && userdatalen != 5) {
        #if DEBUG_APP_UPDATE > 0
        Debug_SysPrint("packetlen == 0 && userdatalen != 5\r\n");
        #endif
        Flash_Lock();
        DataRecv_ReturnHdl(UP_RESULT_UNKNOWERROR, ack);                        /* 数据长度为0，导出终止 */ 
        return;
    }
    if (packetlen != 0) {
        memcpy(buffer, &userdata[FRAME_DATA], packetlen);
        /*写数据到代码备份区*/
        #if EN_CHECKFLASH
        setflashflag(); 
        #endif
        addr = B_BACK_PAGE * s_switch_flash.block_nsize;             /* 获取备份区地址 */
        addr += s_codelen;
        num = ((addr % s_switch_flash.flashpage) + packetlen + s_switch_flash.flashpage - 1) / s_switch_flash.flashpage;/* 所需写入次数 */
        lentemp = 0;
        for (i = 0; i < num; i++) {
            if (i == num - 1) {
                PageWrite(buffer + lentemp, packetlen - lentemp, (addr / s_switch_flash.flashpage) + i, 0);
            } else if (i == 0) {
                lentemp = s_switch_flash.flashpage - (addr % s_switch_flash.flashpage);
                PageWrite(buffer, lentemp, (addr / s_switch_flash.flashpage), (addr % s_switch_flash.flashpage));
            } else {
                PageWrite(buffer + lentemp, s_switch_flash.flashpage, (addr / s_switch_flash.flashpage + i), 0);
                lentemp += s_switch_flash.flashpage;
            }
        }
        #if DEBUG_APP_UPDATE > 0
        debug_printf_dir("<**** (1)UpdateDadaRecv_Hdl:写入数据到备份区****>\r\n");
        #endif
    } 
    ClearWatchdog();
    for (i = 0; i < packetlen; i++) {
        s_newchcksum += buffer[i];
    }
    ClearWatchdog();
    
    s_codelen += packetlen;
    ack[4] = (INT8U)UP_RESULT_SUCESS; 
    s_cnt_error = 0;
    s_num_packets = temppackets;
    //RequestProtocolDataSend(NEED_ACK, UPDATE_DATA_RECV_ACK, ack, 5);           /* 中间包过程中，需要有应答，返回导出一包成功 */
    //RegisterAckList(UPDATE_DATA_RECV_ACK, 10, NextPacketOverTime);             /* 注册超时无应答函数 */
    YX_COM_DirSend(UPDATE_DATA_RECV_ACK, ack, 5);
}

/*******************************************************************************
 **  函数名称:  ResetReq_Hdl
 **  功能描述:  复位请求
 **  输入参数:  [IN]	*data:	数据指针
 **  		  [IN]	datelen:数据长度
 **  返回参数:  无
 ******************************************************************************/
void ResetReq_Hdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen)
{
    INT8U ack[2];

	#if DEBUG_WIRELESS > 0
		debug_printf("复位请求\r\n");
	#endif

    ack[0] = 1;
    YX_COM_DirSend( RESET_REQ_ACK, ack, 1);
    OS_StartTmr(s_rstmcu_tmr, SECOND, 10);
}

/*******************************************************************************
**  函数名称:  ResetMcuDelay
**  功能描述:  复位MCU延时
**  输入参数:  [IN]	sec:	秒
**  返回参数:  无
******************************************************************************/
void ResetMcuDelay(INT32U sec) 
{
    OS_StartTmr(s_rstmcu_tmr, SECOND, sec);
}
#endif
/************************ (C) COPYRIGHT 2011 XIAMEN YAXON.LTD *******************END OF FILE******/

