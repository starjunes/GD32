/**************************************************************************************************
**                                                                                               **
**  文件名称:  configurepg.c                                                                           **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2010                                    **
**  创建信息:  jump -- 2011年1月20日                                                             **
**  文件描述:  系统参数页面                                                                      **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/

#define  CONFIGURE_GLOBALS

#include "yx_includes.h"
#include "yx_com_send.h"
#include "bal_tools.h"
#include "tools.h"
#include "bal_stream.h"
#include "yx_structs.h"
#include "yx_configure.h"
#include "yx_lock.h"
#include "appmain.h"
#include "bal_gpio_cfg.h"
#include "bal_output_drv.h"
#include "yx_com_man.h"
#include "yx_encrypt_man.h"
#include "bal_pp_drv.h"
#include "public.h"
#include "yx_encrypt_man.h"
#include "app_update.h"

static STREAM_T       strm;
static GPS_STATE_E    GPRSstatus = GPS_NO_STATU;
static INT8U          GsmNetIntension;                                      /* 网络信号强度 */
static GPS_STRUCT     GpsLocation = {
	.hbit = 0xff,
};                                     /* 收到的GPS数据信息 */
static INT32U         AlarmStatus;
static INT32U         sensorstatus = 0;
static INT8U		  wifistatus;		//wifi状态

static INT16U         ProvinceID, CityID;
static INT8U          ManufactureID[5], TerminalType[20], TerminalID[7];
static INT8U          ManufactureIDLen, TerminalTypeLen, TerminalIDLen;
static INT8U          TerminalTypeLenMax = 20;
static INT8U          platecolor;
static INT8U          CarID[12], CarIDLen;

//static BOOLEAN        speedflag = false;
//static INT32U         gps_speed, pulse_speed;
//static INT8U          reqparatype;


struct {
    INT8U setparatype;
    INT8U paralen;
    INT8U para[50];
}GprsPara;

/**************************************************************************************************
**  函数名称:  SetGPRSStatus
**  功能描述:  设置GPRS状态
**  输入参数:
**  返回参数:
**************************************************************************************************/
void SetGPRSStatus(GPS_STATE_E status)
{
    GPRSstatus = status;
}

/**************************************************************************************************
**  函数名称:  SetGsmNetIntension
**  功能描述:  设置信号强度
**  输入参数:
**  返回参数:
**************************************************************************************************/
void SetGsmNetIntension(INT8U intension)
{
    GsmNetIntension = intension & 0x07;
}

/**************************************************************************************************
**  函数名称:  IsGpsValid
**  功能描述:  获取Gps位置信息是否定位
**  输入参数:
**  返回参数:
**************************************************************************************************/
INT8U GetGPSStatus(void)
{
    return GpsLocation.hbit;
}

/**************************************************************************************************
**  函数名称:  SetGpsLocation
**  功能描述:  设置GPS位置信息
**  输入参数:
**  返回参数:
**************************************************************************************************/
void SetGpsLocation(INT8U *locationdata)
{
    memcpy((INT8U*)&GpsLocation, locationdata, sizeof(GPS_STRUCT));
}

/**************************************************************************************************
**  函数名称:  Setsensorstatus
**  功能描述:  设置传感器状态
**  输入参数:
**  返回参数:
**************************************************************************************************/
void Setsensorstatus(INT32U status)
{
    sensorstatus = status;
}

/**************************************************************************************************
**  函数名称:  Getwifistatus
**  功能描述:  获取WIFI状态
**  输入参数:
**  返回参数:	wifi状态
**************************************************************************************************/
INT8U Getwifistatus(void)
{
    return wifistatus;
}


/**************************************************************************************************
**  函数名称:  EquipmentStatusReq
**  功能描述:  接收车台状态处理函数
**  输入参数:
**  返回参数:
**************************************************************************************************/
void EquipmentStatusReq(INT8U mancode, INT8U command, INT8U *data, INT16U datalen)
{
    INT8U signal, time[6], temp[4],type,mileage_show;
    INT8U level,type_num,count;
    INT32U sensor;
	static INT8U rtc_cnt = 0;
	//Debug_PrintHex(TRUE, data, datalen);
    if (datalen < 28) return;
    bal_InitStrm(&strm, data, datalen);
    ParaStatus.olstatus = bal_ReadBYTE_Strm(&strm);     /* 是否建立网络 */
    signal = bal_ReadBYTE_Strm(&strm);
    if ((level = (signal + 5) / 6) >= 6) {
        level = 5;
    }
    SetGsmNetIntension(level);
    ParaStatus.gprstatus = bal_ReadBYTE_Strm(&strm);

    if ((ParaStatus.gprstatus & 0x18) && (ParaStatus.gprstatus & 0x03)) {
        SetGPRSStatus(GPS_12_STATU);
    } else if (ParaStatus.gprstatus & 0x18){
        SetGPRSStatus(GPS_2_STATU);
    } else if (ParaStatus.gprstatus & 0x03){
        SetGPRSStatus(GPS_1_STATU);
    } else {
       SetGPRSStatus(GPS_NO_STATU);
    }

    bal_ReadDATA_Strm(&strm, time, 6);
	if (++rtc_cnt > 30) {
		PORT_SetSysTime((SYSTIME_T *)time);
		rtc_cnt = 0;
	}
	
	
    SetGpsLocation(bal_GetStrmPtr(&strm));
    ParaStatus.situate = GetGPSStatus();
    #if EN_DEBUG > 1
    debug_printf("定位状态:%02X\r\n", ParaStatus.situate);
    #endif

    bal_MovStrmPtr(&strm, 11);
    ParaStatus.almstatus[0] = bal_ReadBYTE_Strm(&strm);
    ParaStatus.almstatus[1] = bal_ReadBYTE_Strm(&strm);
    ParaStatus.almstatus[2] = bal_ReadBYTE_Strm(&strm);
    ParaStatus.almstatus[3] = bal_ReadBYTE_Strm(&strm);
    AlarmStatus = ((INT32U)ParaStatus.almstatus[0] << 24) | ((INT32U)ParaStatus.almstatus[1] << 16)
                | ((INT32U)ParaStatus.almstatus[2] << 8) | ParaStatus.almstatus[3];

    temp[0] = bal_ReadBYTE_Strm(&strm);
    temp[1] = bal_ReadBYTE_Strm(&strm);
    temp[2] = bal_ReadBYTE_Strm(&strm);
    temp[3] = bal_ReadBYTE_Strm(&strm);
    sensor = ((INT32U)temp[0] << 24) | ((INT32U)temp[1] << 16) | ((INT32U)temp[2] << 8) | temp[3];

    Setsensorstatus(sensor);

	mileage_show = bal_ReadBYTE_Strm(&strm);
   	bal_MovStrmPtr(&strm, 21);//里程和驾驶时间略过
	
	type_num = bal_ReadBYTE_Strm(&strm);
	for(count = 0; count < type_num; count++){
		type = bal_ReadBYTE_Strm(&strm);
		if(type == 0x23){
			wifistatus = bal_ReadBYTE_Strm(&strm);//wifi状态
			//debug_printf("wifistatus:%d\r\n",wifistatus);
			return ;
		}else{
			bal_MovStrmPtr(&strm, bal_ReadBYTE_Strm(&strm)+1);
		}
	}
	
    #if 0
    if (bal_GetStrmLeftLen(&strm) > 0) {
        type = bal_ReadBYTE_Strm(&strm);
        if (type == 0) {
            Change_time_Mileage(0, 0, type);
        } else {
            if (type == 0x01) {
                bal_ReadDATA_Strm(&strm, time, 6);
                bal_ReadDATA_Strm(&strm, temp, 4);
                bal_MovStrmPtr(&strm, 10);
            } else if (type == 0x02) {
                bal_MovStrmPtr(&strm, 10);
                bal_ReadDATA_Strm(&strm, time, 6);
                bal_ReadDATA_Strm(&strm, temp, 4);
            } else {
                return;
            }
            Change_time_Mileage(time, temp, type);
        }
    }
    #endif
}
#if 0
/**************************************************************************************************
**  函数名称:  CutRedundantLen
**  功能描述:  取较短的长度
**  输入参数:
**  返回参数:
**************************************************************************************************/
INT8U  CutRedundantLen(INT8U len,INT8U standardlen)
{
      if (len < standardlen) {
         return len;
      } else {
         return standardlen;
      }
}
#endif
/**************************************************************************************************
**  函数名称:  GprsQueAns
**  功能描述:  GPRS参数请求应答
**  输入参数:
**  返回参数:
**************************************************************************************************/
void GprsQueAns(INT8U mancode, INT8U command, INT8U *data, INT16U datalen)
{
    INT8U count, type, typelen;


    YX_ConfirmSend(EQUIPMENT_PARA_REQ);
    bal_InitStrm(&strm, data, datalen);
    count = bal_ReadBYTE_Strm(&strm);
    if (count == 0) {

        return;
    }
    while (count-- > 0)
    {
        type = bal_ReadBYTE_Strm(&strm);
        typelen = bal_ReadBYTE_Strm(&strm);
        switch(type)
        {

            case 0x03:
                GprsPara.paralen = CutRedundantLen(typelen, MAXMYTELLEN);
                memset(GprsPara.para, '0', sizeof(GprsPara.para));
                bal_ReadDATA_Strm(&strm, GprsPara.para, GprsPara.paralen);
                break;
            case 0x61:
                if (typelen != 7) {
                    bal_MovStrmPtr(&strm, typelen);

                } else {
                }
                break;
            case 0x62:

                break;
            case 0x63:

                break;
            case 0x64:
                break;
            case 0x65:
                break;

            default:
                bal_MovStrmPtr(&strm, typelen);
                break;
        }
    }
}

/**************************************************************************************************
**  函数名称:  GprsSetAns
**  功能描述:  GPRS参数设置的应答
**  输入参数:
**  返回参数:
**************************************************************************************************/
void GprsSetAns(INT8U mancode, INT8U command, INT8U *data, INT16U datalen)
{

    YX_ConfirmSend(EQUIPMENT_PARA_SET);
}

/**********************************************************************************
**  函数名称:  Client_FuntionUpAck_Hdl
**  功能描述:  客户特殊功能上行请求应答
**  输入参数:  version
            :  command
            :  userdata
            :  userdatalen
**  返回参数:  None
**********************************************************************************/

void Client_FuntionUpAck_Hdl(INT8U mancode, INT8U command, INT8U *data, INT16U datalen)
{
    INT8U cmdtye;
    INT16U cno;


    #if EN_DEBUG > 1
        debug_printf("FC指令应答数据 len:%d  data: ", datalen);
        printf_hex( data, datalen);
    #endif

    bal_InitStrm(&strm, data, datalen);
    cno = bal_ReadHWORD_Strm(&strm);
    if (cno != CLIENT_CODE) return;

    cmdtye = bal_ReadBYTE_Strm(&strm);
    if ((cmdtye == 0x0f) && (bal_ReadBYTE_Strm(&strm) == 0x01) && (bal_ReadBYTE_Strm(&strm) == 0x02)) {
        ProvinceID = bal_ReadHWORD_Strm(&strm);
        CityID = bal_ReadHWORD_Strm(&strm);
        ManufactureIDLen = bal_ReadBYTE_Strm(&strm);
        if (ManufactureIDLen != 5) {
            bal_MovStrmPtr(&strm, ManufactureIDLen);
            ManufactureIDLen = 0;
            memset(ManufactureID, 0, 5);
        } else {
            bal_ReadDATA_Strm(&strm, ManufactureID, 5);
            ManufactureIDLen = bal_CutFromRight(ManufactureID, 0x00, 5);
            if (ManufactureIDLen == 0xff) {
                ManufactureIDLen = 0;
            }
            ManufactureIDLen = bal_CutFromRight(ManufactureID, 0x20, ManufactureIDLen);
            if (ManufactureIDLen == 0xff) {
                ManufactureIDLen = 0;
            }
        }
        TerminalTypeLen = bal_ReadBYTE_Strm(&strm);
        if (TerminalTypeLen == 8) {
            bal_ReadDATA_Strm(&strm, TerminalType, 8);
            TerminalTypeLenMax = 8;
            TerminalTypeLen = bal_CutFromRight(TerminalType, 0x00, 8);
            if (TerminalTypeLen == 0xff) {
                TerminalTypeLen = 0;
            }
            TerminalTypeLen = bal_CutFromRight(TerminalType, 0x20, TerminalTypeLen);
            if (TerminalTypeLen == 0xff) {
                TerminalTypeLen = 0;
            }
        } else if (TerminalTypeLen == 20) {
            bal_ReadDATA_Strm(&strm, TerminalType, 20);
            TerminalTypeLenMax = 20;
            TerminalTypeLen = bal_CutFromRight(TerminalType, 0x00, 20);
            if (TerminalTypeLen == 0xff) {
                TerminalTypeLen = 0;
            }
            TerminalTypeLen = bal_CutFromRight(TerminalType, 0x20, TerminalTypeLen);
            if (TerminalTypeLen == 0xff) {
                TerminalTypeLen = 0;
            }
        } else {
            bal_MovStrmPtr(&strm, TerminalTypeLen);
            TerminalTypeLen = 0;
            memset(TerminalType, 0, 20);
        }
        TerminalIDLen = bal_ReadBYTE_Strm(&strm);
        if (TerminalIDLen != 7) {
            bal_MovStrmPtr(&strm, TerminalIDLen);
            TerminalIDLen = 0;
            memset(TerminalID, 0, 7);
        } else {
            bal_ReadDATA_Strm(&strm, TerminalID, 7);
            TerminalIDLen = bal_CutFromRight(TerminalID, 0x00, 7);
            if (TerminalIDLen == 0xff) {
                TerminalIDLen = 0;
            }
            TerminalIDLen = bal_CutFromRight(TerminalID, 0x20, TerminalIDLen);
            if (TerminalIDLen == 0xff) {
                TerminalIDLen = 0;
            }
        }
        platecolor = bal_ReadBYTE_Strm(&strm);
        CarIDLen = bal_ReadBYTE_Strm(&strm);
        if (CarIDLen > 12) {
            bal_MovStrmPtr(&strm, CarIDLen);
            CarIDLen = 0;
        } else {
            memset(CarID, 0, sizeof(CarID));
            bal_ReadDATA_Strm(&strm, CarID, CarIDLen);
        }
        //InputProvinceID();
    }
}

/**********************************************************************************
**  函数名称:  exio_output
**  功能描述:  扩展IO输出控制
**  输入参数:  type  IO类型
            :  value IO电平
**  返回参数:  None
**********************************************************************************/
static void exio_output(INT8U type, INT8U value)
{
    switch (type) {
        case 0x00:
            if (value) {
                bal_Pullup_232CON();
                #if DEBUG_EXIO > 0
                debug_printf("EC20调试串口通道切换到主接插件串口2\r\n");
                #endif
            } else {
                bal_Pulldown_232CON();
                #if DEBUG_EXIO > 0
                debug_printf("EC20调试串口通道切换到加密芯片UARTB\r\n");
                #endif
            }
            break;
        case 0x01:
            if (value) {
                bal_Pullup_VBUSCTL();
                #if DEBUG_EXIO > 0
                debug_printf("EC20 USB供电控制-高\r\n");
                #endif
            } else {
                bal_Pulldown_VBUSCTL();
                #if DEBUG_EXIO > 0
                debug_printf("EC20 USB供电控制-低\r\n");
                #endif
            }
            break;
        case 0x02:
            if (value) {
                bal_Pullup_HSMPWR();
                #if DEBUG_EXIO > 0
                debug_printf("加密芯片电源控制-高\r\n");
                #endif
            } else {
                bal_Pulldown_HSMPWR();
                #if DEBUG_EXIO > 0
                debug_printf("加密芯片电源控制-低\r\n");
                #endif
            }
            break;
        case 0x03:
            if (value) {
                bal_Pullup_HSMWK();
                #if DEBUG_EXIO > 0
                debug_printf("加密芯片唤醒-高\r\n");
                #endif
            } else {
                bal_Pulldown_HSMWK();
                #if DEBUG_EXIO > 0
                debug_printf("加密芯片唤醒-低\r\n");
                #endif
            }
            break;
        case 0x04:
            if (value) {
                bal_Pulldown_BTWLEN();
                #if DEBUG_EXIO > 0
                debug_printf("wifi电源控制-高\r\n");
                #endif
            } else {
                bal_Pulldown_BTWLEN();
                #if DEBUG_EXIO > 0
                debug_printf("wifi电源控制-低\r\n");
                #endif
            }
            break;
        case 0x05:
            if (value) {
                bal_Pullup_GPSPWR();
                #if DEBUG_EXIO > 0
                debug_printf("gps电源控制-高\r\n");
                #endif
            } else {
                bal_Pulldown_GPSPWR();
                #if DEBUG_EXIO > 0
                debug_printf("gps电源控制-低\r\n");
                #endif
            }
            break;
        case 0x06:
            if (value) {
                bal_Pullup_PHYPWR();
                #if DEBUG_EXIO > 0
                debug_printf("phy电源控制-高\r\n");
                #endif
            } else {
                bal_Pulldown_PHYPWR();
                #if DEBUG_EXIO > 0
                debug_printf("phy电源控制-低\r\n");
                #endif
            }
            break;
        case 0x07:
            if (value) {
                //bal_Pullup_GPSBAT();
                #if DEBUG_EXIO > 0
                debug_printf("高精度定位模块热启动控制-高\r\n");
                #endif
            } else {
                //bal_Pulldown_GPSBAT();
                #if DEBUG_EXIO > 0
                debug_printf("高精度定位模块热启动控制-低\r\n");
                #endif
            }
            break;
        case 0x08:
            if (value) {
                bal_Pullup_5VEXT();
                #if DEBUG_EXIO > 0
                debug_printf("外围芯片5V电源使能控制-高\r\n");
                #endif
            } else {
                bal_Pulldown_5VEXT();
                #if DEBUG_EXIO > 0
                debug_printf("外围芯片5V电源使能控制-低\r\n");
                #endif
            }
            break;
        default:
            break;
    }
}

/**********************************************************************************
**  函数名称:  Client_FuntionDown_Hdl
**  功能描述:  客户特殊功能下行命令
**  输入参数:  version
            :  command
            :  userdata
            :  userdatalen
**  返回参数:  None
**********************************************************************************/
void Client_FuntionDown_Hdl(INT8U mancode, INT8U command, INT8U *data, INT16U datalen)
{
    INT8U i;
    INT16U code;
    INT8U cmdtye;
    INT8U ionum, iotype, iovalue;
    STREAM_T rstrm;
    INT8U ack[32];

    ack[0] = data[0];
    ack[1] = data[1];
    ack[2] = data[2];
	
    bal_InitStrm(&rstrm, data, datalen);
    code = bal_ReadHWORD_Strm(&rstrm);
    if ((code != CLIENT_CODE) || (datalen < 3)) {
        return;
    }

    cmdtye = bal_ReadBYTE_Strm(&rstrm);
	//debug_printf("Client_FuntionDown_Hdl cmdtye:%X\r\n",cmdtye);
    switch (cmdtye) {
         case 0x01:
            LockParaStore(&data[3], datalen-3);
            break;
        case 0x02:
            Lock_RecordReqHdl();
            break;
        case 0x05:
            if (LockParaBak(&data[3], datalen-3)) {
               ack[2] = 0x05;
               ack[3] = 0x01;
            } else {
               ack[2] = 0x05;
               ack[3] = 0x02;
            }
            YX_COM_DirSend(CLIENT_FUNCTION_DOWN_REQ_ACK, ack, 4);
            #if EN_DEBUG > 0
            debug_printf("指令FD-05应答%2x: ", data[2]);
            printf_hex(ack, 4);
            #endif
            break;  		
        case 0x20:
            ionum = bal_ReadBYTE_Strm(&rstrm);
            for (i = 0; i < ionum; i++) {
                iotype = bal_ReadBYTE_Strm(&rstrm);
                iovalue = bal_ReadBYTE_Strm(&rstrm);
                exio_output(iotype, iovalue);
            }
            ack[3] = 0x00;
            YX_COM_DirSend( CLIENT_FUNCTION_DOWN_REQ_ACK, ack, 4);
            break;
        case 0x21:
            break;
        case 0x24:
            YX_COM_SetOTA();
            ack[3] = 0x00;
            YX_COM_DirSend( CLIENT_FUNCTION_DOWN_REQ_ACK, ack, 4);
            break;
        case 0x26: /* 加密算法 */
            YX_Encrypt_Func(&data[3], datalen - 3);
			ack[3] = 0x00;
            break;
        case 0x2e:  /* 柳汽康明斯国五锁车指令 */
            #if EN_KMS_LOCK > 0
            Lock_KmsG5Cmd(&data[3], datalen - 3);
            #endif
            break;
		case 0x31:  /* 锁车命令通知 */
			#if LOCK_COLLECTION > 0
			YX_LOCKCMD_SetPara(&data[3]);
			#endif
            ack[3] = 0x00;
			YX_COM_DirSend( CLIENT_FUNCTION_DOWN_REQ_ACK, ack, 4);
            break;
		case 0x32:  /* 锁车采集报文同步应答 */
            #if LOCK_COLLECTION > 0
            Lock_KmsG5Cmd(&data[3], datalen - 3);
            #endif
            break;
        default:
            break;
      }

}
/**************************************************************************************************
**  函数名称:  ClearDataRequest
**  功能描述:  处理主机清空调度屏数据请求
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
void ClearDataRequest(INT8U mancode, INT8U command, INT8U *data, INT16U userdatalen)
{
    INT8U   temp[2];
    //INT16U  i;
    
    mancode = mancode;
    command = command;
    userdatalen = userdatalen;
    
    temp[0] = data[0];
    temp[1] = 0x01;
    switch (data[0]) {
        case 0x01:    
            #if 1
            #if EN_DEBUG > 1
            debug_printf("清除康明斯锁车pp参数\r\n");
            printf_hex((INT8U*)GetDefaulValue(KMS_LOCK_PARA_), sizeof(KMS_LOCK_OBJ_T));
            #endif
            ResumeAllPubPara();
            #else
            if (bal_pp_StoreParaByID(AXISCALPARA_, (INT8U*)GetDefaulValue(AXISCALPARA_), sizeof(AXISPARA_T)) == FALSE) {
                temp[1] = 0x02;
            }
            if (bal_pp_StoreParaByID(SCLOCKPARA_, (INT8U*)GetDefaulValue(SCLOCKPARA_), sizeof(SCLOCKPARA_T)) == FALSE) {
                temp[1] = 0x02;
            }				
            if (bal_pp_StoreParaByID(LOCKRECORD_, (INT8U*)GetDefaulValue(LOCKRECORD_), sizeof(LOCK_RECORD)) == FALSE) {
                temp[1] = 0x02;
            }					
            if (bal_pp_StoreParaByID(SCLOCKPARABAK_, (INT8U*)GetDefaulValue(SCLOCKPARABAK_), sizeof(SCLOCKPARABAK_T)) == FALSE) {
                temp[1] = 0x02;
            }
            #if EN_KMS_LOCK > 0
            #if EN_DEBUG > 1
            debug_printf("清除康明斯锁车pp参数\r\n");
            printf_hex((INT8U*)GetDefaulValue(KMS_LOCK_PARA_), sizeof(KMS_LOCK_OBJ_T));
            #endif
            if (bal_pp_StoreParaByID(KMS_LOCK_PARA_, (INT8U*)GetDefaulValue(KMS_LOCK_PARA_), sizeof(KMS_LOCK_OBJ_T)) == FALSE) {
                temp[1] = 0x02;
            }
            #endif
            #endif
            break;
        default:
            temp[1] = 0x02;
            break;
    }
	
    YX_COM_DirSend(CLEAR_DATA_REQ_ACK, temp, 2);
    ResetMcuDelay(5);
}

