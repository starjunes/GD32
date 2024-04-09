/**************************************************************************************************
**                                                                                               **
**  文件名称:  YX_LOCK.c                                                                         **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2011                                    **
**  创建信息:  阙存先 -- 2017年7月31日                                                       **
**  文件描述:  锁车流程                                                                         **
**  ===========================================================================================  **
**  修改信息:  2011-10-25 By clt: 两个控制开关管脚均默认关闭                                     **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/
#include "yx_lock.h"
#include "yx_includes.h"
#include "yx_can_man.h"
#include "yx_com_send.h"
#include "yx_com_recv.h"
#include "yx_protocal_hdl.h"
#include "yx_debugcfg.h"
#include "port_gpio.h"
#include "bal_pp_drv.h"
#include "port_timer.h"
#include "port_adc.h"

#include "yx_md5.H"
#include "yx_lc_md5.h"
#include "yx_signal_man.h"
#include "appmain.h"
#include "bal_input_reg.h"
#include "bal_input_drv.h"
#include "yx_power_man.h"
#include "yx_com_man.h"
#include "xichai_ems_seedkey.h"
#include "tbox_seed2key.h"
#include "sha-256.h"
#if EN_UDS > 0
#include "yx_uds_drv.h"
#include "yx_uds_did.h"
#endif
#include "MDP.h"
#if 0
#ifdef DEBUG_LOCK
#undef DEBUG_LOCK
#endif

#define DEBUG_LOCK 1
#endif
#define               LOCK_CAN_CH       CAN_CHN_1//CAN_CHN_2



#define KMS_Q6_KEY_ID           0x18FFCA00          /* 康明斯ECU to 记录仪 */
        
static REUPSAFE_DATA_T*         s_lockmsg_buf;      // 锁车上报数据内存
static INT8U                    s_req = 0;          // 上报流水号
static INT32U                   s_igoncnt = 0;      // igon后时间戳/10ms
typedef struct {
    INT8U  chn;
    INT32U filtrid;
    INT32U filtrid_mask;
} FILTER_ID_CFG_T;
/* 锁车参数 */
static SCLOCKPARA_T s_sclockpara;

/* 锁车日志 */
static LOCK_RECORD     s_lockrecord;
/* 锁车参数备份 */
static SCLOCKPARABAK_T s_sclockparabak;

static INT8U           s_lockstatid[12];
static INT8U           s_seed[4];		// 随机数种子
static FILTER_ID_CFG_T const    s_filtid[] = {
						LOCK_CAN_CH, 0x0cfffd00,      0xffffffff,
						LOCK_CAN_CH, 0x18fd0100,      0xffffffff,
						LOCK_CAN_CH, 0x18ff0800,      0xffffffff,     
						LOCK_CAN_CH, 0x18fff400,      0xffffffff,
						LOCK_CAN_CH, KMS_Q6_KEY_ID,   0xffffffff,
						#if EN_UDS > 0
						UDS_CAN_CH,  FUNC_REQID,      0xffffffff,
						UDS_CAN_CH,	 UDS_PHSCL_REQID, 0xffffffff,
						#endif
						LOCK_CAN_CH, 0x18FEAF00,      0xFBFFB9FF,     /* 油气请求判读id 0x18FEAF00,0x1CFEAF00,0x18FEE900 */
						LOCK_CAN_CH, 0x18FEF000,      0xEBD00000,     /* DTC丢失节点 0x18FEF000,0x0CFE6C17,0x18D00021,0x18FF6003,0x18F00010,0x18F0010B,0x18FEF433,0x18FE582F,0x18FEFCC6,0x18FC08F4,0x18FF0241,0x18FF4FF4 */
};
static SC_LOCK_STEP_E  s_sclockstep = CONFIG_OVER;
static INT8U           s_key[8];


static BOOLEAN         s_idfiltenable;
static BOOLEAN         s_handskenable;
static INT16U          s_idfiltcnt;
static BOOLEAN         s_parasendstat;
static INT8U           s_parasendcnt;
static INT8U           s_md5source[8];
static INT8U           s_md5result[16];
static INT8U           s_lock_tmr;
static INT16U          s_reqcnt;
static INT16U          s_poweron_times_cnt = 0;                             /* 上电2分钟内读取油耗报文 */
static BOOLEAN         s_oilsumreq = FALSE;                                  /* FALSE:表示不发送油耗请求，TRUE：表示发送油耗请求 */
static INT8U           s_ycseed[4];
static BOOLEAN         f_handsk = FALSE;						// 是否握手成功
static BOOLEAN         f_handskcnt = 0;
static INT32U 		   s_securitykey[4] = {0x32B162CD,0x729F6E13,0x5FAE2E12,0x12747A3E};
#if LOCK_COLLECTION > 0
static D008_DATA_T	   *s_d008data;				// 记录监控数据
static INT16U		   s_d008locktime = 600;			// 收到指令后采集报文时间
#endif

static BOOLEAN		  	s_ishandover	= FALSE;	   			/* 握手结束标志 */
static HANDSHAKE_ACK_E	s_handshake_ack = HANDSHAKE_UNKNOWN;    // 上报平台握手结果

//潍柴
#define ACK_HANFSHARK	0x18FE02FB  //握手回复报文
static INT8U s_wc_state = 0, s_wc_ack = 0;
static BOOLEAN s_wc_0100recv = FALSE;		// 是否收到握手校验报文
static INT16U s_wc_0100cnt = 0;				// 握手报文超时时间
static BOOLEAN s_wc_0800recv = FALSE;		// 是否收到消贷报文
static INT16U s_wc_0800cnt = 0;				// 消贷报文超时时间

/* ECU反馈 byte3 */
#define WC_ACTIVE_BIT (1)    //激活位
#define WC_LOCK_BIT   (1<<1) //锁车位
#define WC_KEY_BIT    (1<<2) //KEY位
#define WC_GID_BIT    (1<<3) //GPS ID

// 玉柴
static INT8U s_yc_state = 0;		// 握手状态


// 康明斯

static KMS_LOCK_STEP_E s_kmslockstep = KMS_REQ_SEED;
static BOOLEAN s_kms_hand_send_enable = TRUE;//康明斯握手报文发送使能标志
static INT8U   s_kms_delay_cnt = 0;//康明斯延时计数

#define KMS_CMD_NONE      0x00
#define KMS_CMD_ACTIVE    0x01
#define KMS_CMD_LOCK      0x02
#define KMS_CMD_UNLOCK    0x03
#define KMS_CMD_CLOSE     0x04

#define CAN_SPEED_ID	  0x0CF00400	//转速报文
static BOOLEAN can_speed_flag =TRUE;	//收到转速报文标识

static KMS_LOCK_OBJ_T s_kms_obj = {KMS_CMD_NONE, {0xff, 0xff}};

/* 锡柴 */
static XCLOCKPARA_T s_xclockpara;
static INT8U s_xichai_seed[8];              // 锡柴随机数
static INT8U hash[SIZE_OF_SHA_256_HASH];    // 计算的哈希值
static INT8U hash_cnt = 0;                  // 发送哈希值计数

/**************************************************************************************************
**  函数名称:  LockSafeDataAdd
**  功能描述:  锁车安全数据填充
**  输入参数:  type--数据类型
              len--数据长度
              buf--数据内容
**  输出参数:  无
**  返回参数:  true:填充成功 false：数据缓存已满，填充失败
**************************************************************************************************/
static BOOLEAN LockSafeDataAdd(INT8U type, INT8U len, INT8U* buf)
{
    INT8U i = 0;
    if ((type == 0x00) && (buf[0] <  MAX_STAT)) {
        i = buf[0];
		if (s_sclockpara.unbindstat == 1) {
			return FALSE;
		}
		if (s_lockmsg_buf->active[i] == FALSE) {
			s_lockmsg_buf->active[i] = TRUE;
			s_lockmsg_buf->buf[i][0] = s_req++;
		}
        s_lockmsg_buf->buf[i][1] = type;
        s_lockmsg_buf->buf[i][2] = len;
        memcpy(&s_lockmsg_buf->buf[i][3], buf, len);
    } else {
        for (i = MAX_STAT; i < HANDDATANUM;i++) {
            if (s_lockmsg_buf->active[i] == FALSE) {
                s_lockmsg_buf->buf[i][0] = s_req++;
                s_lockmsg_buf->buf[i][1] = type;
                s_lockmsg_buf->buf[i][2] = len;
                s_lockmsg_buf->active[i] = TRUE;
                memcpy(&s_lockmsg_buf->buf[i][3], buf, len);
                break;
            }
        }
    }
    #if DEBUG_LOCK > 0
    debug_printf("LockSafeDataAdd type:%d result:%d\r\n", type, buf[0]);
    #endif
    if (i >= HANDDATANUM) {
        return FALSE;
    } else {
        return TRUE;
    }
}

/**************************************************************************************************
**  函数名称:  KmsG5LockMsgSend
**  功能描述:  柳汽康明斯国5锁车指令发送
**  输入参数:  无
**  输出参数:  无
**  返回参数:  无
**************************************************************************************************/
static void KmsG5LockMsgSend(void)
{
    static INT8U send_period_cnt = 0;
    INT8U senddata[13] = {0x18, 0xff, 0xf9, 0x4b, 0x08, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}; /* 0x18fff94b */ 

    #if DEBUG_LOCK > 1
    static INT8U printf_cnt = 0;
    if (++printf_cnt >= 100) {
        printf_cnt = 0;
        debug_printf("<KmsG5LockMsgSend,cmd:0x%x, speed[0]:0x%x,speed[1]:0x%x>\r\n",s_kms_obj.kms_cmd, s_kms_obj.rota_speed[0], s_kms_obj.rota_speed[1]);
    }
    #endif

    if ((s_kms_obj.kms_cmd >= KMS_CMD_ACTIVE) && (s_kms_obj.kms_cmd <= KMS_CMD_CLOSE)) {
        /* 50ms */
        if (++send_period_cnt >= 5) {
            send_period_cnt = 0;
            if (s_kms_obj.kms_cmd == KMS_CMD_ACTIVE) {
                senddata[5] = 0x00;
            } else if (s_kms_obj.kms_cmd == KMS_CMD_LOCK) {
                senddata[5] = 0x01;
                /* 转速 */
                senddata[6] = s_kms_obj.rota_speed[0];
                senddata[7] = s_kms_obj.rota_speed[1];
            } else if (s_kms_obj.kms_cmd == KMS_CMD_UNLOCK) {
                senddata[5] = 0x00;
            } else {
                return;
            }
            
            CAN_TxData(senddata, false, LOCK_CAN_CH);
        }
    } else {
        send_period_cnt = 0;
    }
}

#if LOCK_COLLECTION > 0
/**************************************************************************************************
**  函数名称:  YX_StartDataCollection
**  功能描述:  开始数据采集
**  输入参数:  type:采集数据类型  0x00:前8s数据  0x01:指令数据
**  输出参数:  无
**  返回参数:  无
**************************************************************************************************/
void YX_StartDataCollection(INT8U type)
{
	#if DEBUG_LOCK > 0
		debug_printf("YX_StartDataCollection type:%d s_d008locktime:%d\r\n", type, s_d008locktime);
	#endif
	if (type == 0x00) {
		s_wc_ack 	= 0;
		s_wc_state	= 0;
		memset(s_d008data->restartdata, 0, sizeof(s_d008data->restartdata));
		s_d008data->restarttime		= 0;
		s_d008data->restarttotal 	= 0;
		s_d008data->restarten		= FALSE;
	} else if (type == 0x01) {
		s_wc_ack 	= 0;
		s_wc_state	= 0;
		memset(s_d008data->lockcmddata, 0, sizeof(s_d008data->lockcmddata));
		s_d008data->lockcmdtime		= 0;
		s_d008data->lockcmdtotal	= 0;
		s_d008data->lockcmden		= FALSE;
	}
}

/**************************************************************************************************
**  函数名称:  YX_AddDataCollection
**  功能描述:  填充采集信息
**  输入参数:  canbuf:CAN数据
**  输出参数:  无
**  返回参数:  true:填充成功 false:填充失败
**************************************************************************************************/
BOOLEAN YX_AddDataCollection(INT8U *canbuf)
{
	INT8U cnt = 0;
	if (s_d008data->lockcmdtime <= s_d008locktime) {
		cnt = s_d008data->lockcmdtotal;
		if (cnt >= 20) {
			return FALSE;
		}
		memcpy(s_d008data->lockcmddata[cnt], canbuf, 12);	// CANDATA
		s_d008data->lockcmdtotal++;
	}

	if (s_d008data->restarttime <= 800) {
		cnt = s_d008data->restarttotal;
		if (cnt >= 80) {
			return FALSE;
		}
		memcpy(s_d008data->restartdata[cnt], canbuf, 12);	// CANDATA
		s_d008data->restarttotal++;
	}
	return TRUE;
}

/**************************************************************************************************
**  函数名称:  SendDataCollection
**  功能描述:  发送采集信息
**  输入参数:  canbuf:CAN数据
**  输出参数:  无
**  返回参数:  true:填充成功 false:填充失败
**************************************************************************************************/
static void SendDataCollection(void)
{
	INT8U senddata[1024] = {0};
	INT16U datalen = 0, i;
	static INT16U delay_lockcmd = 0, delay_restart = 0;
	/* 锁车报文处理 */
	s_d008data->lockcmdtime++;
	if (s_d008data->lockcmdtime == s_d008locktime) {
		if ((s_d008data->lockcmdtotal > 0) && (s_d008data->lockcmdtotal < 20)) {
			s_d008data->lockcmden = TRUE;
			/*memcpy(s_d008data->lockcmddata[s_d008data->lockcmdtotal], s_d008data->lockcmddata[s_d008data->lockcmdtotal-1], 12);
			s_d008data->lockcmdtotal++;*/
		}
	}
	if (s_d008data->lockcmden == TRUE) {
		if (YX_COM_Islink() == TRUE) {
			if (delay_lockcmd++ >= 200) {									// 延迟2s发送
				senddata[datalen++] = 0x09;							// 客户编码
			    senddata[datalen++] = 0x18;							//
			    senddata[datalen++] = 0x32;							// 指令类型
				senddata[datalen++] = 0x01;							// 数据类型
				senddata[datalen++] = s_d008data->lockcmdtotal;		// 数据包数
				for (i = 0;i < s_d008data->lockcmdtotal; i++){
					memcpy(&senddata[datalen], s_d008data->lockcmddata[i], 12);
					datalen += 12;
				}
				YX_COM_DirSend(CLIENT_FUNCTION_DOWN_REQ_ACK, senddata, datalen);
				#if DEBUG_LOCK > 0
					debug_printf("SendDataCollection num:%d lockcmdbuf:", s_d008data->lockcmdtotal);
					Debug_PrintHex(TRUE, senddata, datalen);
				#endif
				s_d008data->lockcmden	= FALSE;
				//s_d008data->lockcmdtime 	= 0;
				s_d008data->lockcmdtotal = 0;
				memset(s_d008data->lockcmddata, 0 , sizeof(s_d008data->lockcmddata));
			}
		}
	}
	if (s_d008data->lockcmdtime > s_d008locktime) {
		s_d008data->lockcmdtime = s_d008locktime + 100; //防止越界
	}
	datalen = 0;
	
	/* 重新点火报文处理 */
	s_d008data->restarttime++;
	if (s_d008data->restarttime == 800) {
		#if DEBUG_LOCK > 0
		debug_printf("restarttotal:%d\r\n",s_d008data->restarttotal);
		#endif
		if ((s_d008data->restarttotal > 0) && (s_d008data->restarttotal < 80)) {
			s_d008data->restarten = TRUE;
			/*memcpy(s_d008data->restartdata[s_d008data->restarttotal], s_d008data->restartdata[s_d008data->restarttotal-1], 12);
			s_d008data->restarttotal++;*/
		}
	}
	if (s_d008data->restarten == TRUE) {
		if (YX_COM_Islink() == TRUE) {
			if (delay_restart++ >= 200) {									// 延迟2s发送
				senddata[datalen++] = 0x09;							// 客户编码
			    senddata[datalen++] = 0x18;							//
			    senddata[datalen++] = 0x32;							// 指令类型
				senddata[datalen++] = 0x00;							// 数据类型
				senddata[datalen++] = s_d008data->restarttotal;		// 数据包数
				for (i = 0;i < s_d008data->restarttotal; i++){
					memcpy(&senddata[datalen], s_d008data->restartdata[i], 12);
					datalen += 12;
				}
				YX_COM_DirSend(CLIENT_FUNCTION_DOWN_REQ_ACK, senddata, datalen);
				#if DEBUG_LOCK > 0
					debug_printf("SendDataCollection num:%d rstartcmdbuf:", s_d008data->restarttotal);
					Debug_PrintHex(TRUE, senddata, datalen);
				#endif
				s_d008data->restarten	= FALSE;
				//s_d008data->restarttime	= 0;
				s_d008data->restarttotal	= 0;
				memset(s_d008data->restartdata, 0 , sizeof(s_d008data->restartdata));
			}
		}
	}
	if (s_d008data->restarttime > 800) {
		s_d008data->restarttime = 900; //防止越界
	}
}


/**************************************************************************************************
**  函数名称:  YX_LOCKCMD_SetPara
**  功能描述:  设置锁车参数
**  输入参数:  canbuf:CAN数据
**  输出参数:  无
**  返回参数:  true:填充成功 false:填充失败
**************************************************************************************************/
void YX_LOCKCMD_SetPara(INT8U *buf)
{
	
	if (buf[1] > 10) {
		s_d008locktime = 1000;	// 最大10s
	} else {
		s_d008locktime = buf[1] * 100;
	}
	#if DEBUG_LOCK > 0
		debug_printf("锁车命令同步 cmdtype:%d %d秒\r\n", buf[0], s_d008locktime/100);
	#endif
	YX_StartDataCollection(0x01);	// 开始采集锁车报文
}

/**************************************************************************************************
**  函数名称:  YX_IsLOCKCMD
**  功能描述:  是否为锁车指令
**  输入参数:  id
**  输出参数:  无
**  返回参数:  true:是 false:否
**************************************************************************************************/
BOOLEAN YX_IsLOCKCMD(INT32U id)
{
	if ((id == KMS_CANID_BIND) || (id == KMS_CANID_LOCK) || (id == KMS_CANID_ACKSTATUS) ||
		(id == WC_CANID_BIND) || (id == WC_CANID_LOCK) || (id == WC_CANID_ACKSTATUS) ||
		(id == YC_CANID_BIND) || (id == YC_CANID_LOCK) || (id == YC_CANID_ACKSTATUS)) {
		return TRUE;
	}
	return FALSE;
}


#endif

/**************************************************************************************************
**  函数名称:  SetSpeedFlag
**  功能描述:  设置转速报文标志
**  输入参数:  flag:标志
**  返回参数:  无
**************************************************************************************************/
void SetSpeedFlag(BOOLEAN flag)
{
	can_speed_flag = flag;
}

/**************************************************************************************************
**  函数名称:  GetSpeedFlag
**  功能描述:  获取转速报文标志
**  输入参数:  flag:标志
**  返回参数:  无
**************************************************************************************************/
BOOLEAN GetSpeedFlag(void)
{
	return can_speed_flag;
}

/**************************************************************************************************
**  函数名称:  GetFiltenableStat
**  功能描述:  获取CAN总线配置阻止状态
**  输入参数:  None
**  返回参数:  开关状态值 TRUE or FALSE
**************************************************************************************************/
BOOLEAN GetFiltenableStat(void)
{
    return s_idfiltenable;
}

/**************************************************************************************************
**  函数名称:  DC_CanDelayTmr
**  功能描述:  大柴锁车延时处理(MPU端处理，功能预留)
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
void DC_CanDelayTmr(void)
{

}
/**************************************************************************************************
**  函数名称:  XC_SecretDataTran
**  功能描述:  锡柴数据加密转发
**  输入参数:  data:参数内容
			   datalen:数据长度
**  返回参数:  false:失败 true:成功
**************************************************************************************************/
BOOLEAN XC_SecretDataTran(INT8U* data, INT8U datalen)
{
	INT8U key[8], ch, xc_checkcounter;
	INT32U retlen, id;
    INT8U senddata[13] = {0};
	if (datalen != 9) {
		#if DEBUG_LOCK > 0
		debug_printf("XC_SecretDataTran datalen:%d err\r\n", datalen);
		#endif
		return FALSE;
	}
    id = bal_chartolong(data);
	switch (s_sclockpara.ecutype) 
	{
		case ECU_XICHAI_EMSVI:
			seedToKey(&data[5], 4, key, &retlen);
			memcpy(&senddata[8], key, 4);
			senddata[4] = retlen+3;
			senddata[5] = 0x06;
			senddata[6] = 0x27;
			senddata[7] = 0x02;
			break;
		case ECU_XICHAI_EMSMDI:
			seedToKey_EControl_app(&data[5], 4, key, &retlen);
			memcpy(&senddata[8], key, 4);
			senddata[4] = retlen+3;
			senddata[5] = 0x06;
			senddata[6] = 0x27;
			senddata[7] = 0x02;
			break;
		case ECU_XICHAI_EMSECO:
			MessageProcessEnCode(s_xclockpara.ctr_mode, s_xclockpara.limitLR, s_xclockpara.limitHR, s_xclockpara.checkcode, s_xclockpara.msgID, &senddata[3], &xc_checkcounter);
			senddata[0] = 0x0A;
            senddata[1] = 0x27;
            senddata[2] = 0x04;
            YX_MMI_CanSendMul(data[4]-1, id, senddata, 11);
            #if DEBUG_LOCK > 0
            debug_printf("XC_SecretDataTran ecutype:%d--", s_sclockpara.ecutype);
            Debug_PrintHex(TRUE, senddata, 11);
            #endif
			return TRUE;
		default:
			return FALSE;
	}
	memcpy(senddata, data, 4);		// canid
    ch = data[4];
	if ((ch > 0) && (ch < CAN_CHN_3)) {
        ch -= 1;
    } else {
        #if DEBUG_LOCK > 0
        debug_printf("XC_SecretDataTran ch:%d err\r\n", ch);
        #endif
        return FALSE;
    }
	#if DEBUG_LOCK > 0
	debug_printf("XC_SecretDataTran ecutype:%d ch:%d ", s_sclockpara.ecutype, ch);
	Debug_PrintHex(TRUE, senddata, 13);
	#endif
	CAN_TxData(senddata, false, ch);
	return TRUE;
}

/**************************************************************************************************
**  函数名称:  XC_ParaSet
**  功能描述:  锡柴参数同步
**  输入参数:  data:参数内容
			   datalen:数据长度
**  返回参数:  false:失败 true:成功
**************************************************************************************************/
BOOLEAN XC_ParaSet(INT8U* data, INT8U datalen)
{
    BOOLEAN change = FALSE;
    INT32U checkcode, msgid;
	if (datalen != 11) {
		return FALSE;
	}
    if (s_xclockpara.ctr_mode != data[0]) {
        s_xclockpara.ctr_mode = data[0];
        change = TRUE;
    }
    if (s_xclockpara.limitLR != data[1]) {
        s_xclockpara.limitLR = data[1];
        change = TRUE;
    }
    if (s_xclockpara.limitHR != data[2]) {
        s_xclockpara.limitHR = data[2];
        change = TRUE;
    }
	checkcode	= bal_chartolong(&data[3]);
	msgid		= bal_chartolong(&data[6]);
    if (s_xclockpara.checkcode != checkcode) {
        s_xclockpara.checkcode = checkcode;
        change = TRUE;
    }
    if (s_xclockpara.msgID != msgid) {
        s_xclockpara.msgID = msgid;
        change = TRUE;
    }
    if (change) {
        #if DEBUG_LOCK > 0
        debug_printf("XC_ParaSet 状态改变\r\n");
        #endif
        bal_pp_StoreParaByID(XCLOCKPARA_, (INT8U *)&s_xclockpara, sizeof(XCLOCKPARA_T));
    }
	return TRUE;
}
/**************************************************************************************************
**  函数名称:  XC_HandShake
**  功能描述:  锡柴握手（随机数请求）
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
void XC_HandShake(void)
{
	INT8U senddata[13] = {0x0C, 0x00, 0x00, 0x4A, 0x08, 0xc0, 0xff, 0xfa, 0xfa, 0xff, 0xf0, 0xff, 0xff};//周期报文
	if ((s_sclockpara.ecutype != ECU_XICHAI_EMSMDI) || (s_sclockpara.ecutype != ECU_XICHAI_EMSVI)) return;
    
	#if DEBUG_LOCK > 0
	debug_printf("XC_HandShake step:%d\r\n", s_sclockstep);
	#endif
    CAN_TxData(senddata, false, LOCK_CAN_CH);
}

/**************************************************************************************************
**  函数名称:  XC_Hashcal
**  功能描述:  计算哈希值
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
static void XC_Hashcal(void)
{
    INT8U input[32] = {0x00};
    UDS_DID_DATA_E2ROM_T uds_data;
    bal_pp_StoreParaByID(UDS_DID_PARA_, (INT8U *)&uds_data, sizeof(UDS_DID_DATA_E2ROM_T));
    
    memcpy(input, s_xichai_seed, 7);
    input[7] = 0xFF;
    input[8] = 0xFF;
    input[9] = 0xFF;
    memcpy(&input[10], uds_data.DID_F190, sizeof(uds_data.DID_F190));
    memcpy(&input[27], s_xichai_seed, 4);
    input[31] = s_xichai_seed[5];
    calc_sha_256(hash, input, 32);
}
/**************************************************************************************************
**  函数名称:  XC_HandShake
**  功能描述:  锡柴发送检验码
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
void XC_Checkcode(void)
{
    static INT8U xc_checkcounter = 0;// 锡柴周期报文(5s)
    INT8U perioddata[13] = {0x0C,0x00,0x00,0x4A,0x08,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
	
	if ((s_sclockpara.ecutype != ECU_XICHAI_EMSECO) || (s_sclockpara.ecutype != ECU_XICHAI_EMSMDI) || (s_sclockpara.ecutype != ECU_XICHAI_EMSVI)) {
        #if DEBUG_LOCK > 0
        debug_printf("XC_Checkcode ECU类型错误:%d\r\n", s_sclockpara.ecutype);
        #endif
        return;
    }
    switch (s_sclockpara.ecutype)
    {
        case ECU_XICHAI_EMSVI:
            MessageProcessEnCode(s_xclockpara.ctr_mode, s_xclockpara.limitLR, s_xclockpara.limitHR, s_xclockpara.checkcode, s_xclockpara.msgID, &perioddata[5], &xc_checkcounter);
            CAN_TxData(perioddata, FALSE, LOCK_CAN_CH);
            break;
        case ECU_XICHAI_EMSMDI:
        case ECU_XICHAI_EMSECO:
            perioddata[0] = 0xF0;
            perioddata[1] = s_xclockpara.limitLR;
            perioddata[2] = s_xclockpara.limitHR;
            perioddata[3] = 0x7D;
            perioddata[4] = 0xFF;
            perioddata[5] = 0xF0;
            perioddata[6] = 0xFF;
            perioddata[7] = hash[hash_cnt++];
            if (hash_cnt >= SIZE_OF_SHA_256_HASH) {
                hash_cnt = 0;
                memset(hash, 0xFF, SIZE_OF_SHA_256_HASH);
                s_ishandover = TRUE;
                s_handshake_ack = HANDSHAKE_OK;
                f_handsk = TRUE; 
                LockSafeDataAdd(0x00, 1, &s_handshake_ack);
            }
            break;
        default:

            break;
    }
	
	#if DEBUG_LOCK > 0
	debug_printf("XC_Checkcode step:%d data:\r\n", s_sclockstep);
    Debug_PrintHex(TRUE, perioddata, 13);
	#endif
    CAN_TxData(perioddata, false, LOCK_CAN_CH);
}

/**************************************************************************************************
**  函数名称:  XC_CanDelayTmr
**  功能描述:  锡柴锁车延时处理(MPU端处理，功能预留)
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
void XC_CanDelayTmr(void)
{
	//static INT8U xc_cnt = 0;//上电发送3次
    static INT8U xc_period_cnt = 0,xc_checkcounter = 0;// 锡柴周期报文(5s)
    //INT8U senddata[13]   = {0x18,0xda,0x00,0xf1,0x08,0x22,0x02,0x9c,0xff,0xff,0xff,0xff,0xff};//握手报文
    INT8U perioddata[13] = {0x0C, 0x00, 0x00, 0x41, 0x08, 0xc0, 0xff, 0xfa, 0xfa, 0xff, 0xf0, 0xff, 0xff};//周期报文
    switch (s_sclockstep) {
        case RAND_CODE_REC:         // 随机数请求
            if ((xc_period_cnt++ % 500) == 0) {
                xc_period_cnt = 0;
                XC_HandShake();
            }
            break;
        case CHECK_CODE_SEND:       // 发送加密数据
            if ((xc_period_cnt++ % 500) == 0) {
                xc_period_cnt = 0;
                XC_Checkcode();
            }
            break;
        default:
            xc_period_cnt = 0;
            break;
    }
}

/**************************************************************************************************
**  函数名称:  WC_HandShake
**  功能描述:  潍柴握手
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
void WC_HandShake(void)
{
    INT8U senddata[13] = {0x00,0x00,0x00,0x00,0x08,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
	
	if (s_sclockpara.ecutype != ECU_WEICHAI) return;
    
	senddata[0]	= (ACK_HANFSHARK>>24) & 0xFF;
	senddata[1]	= (ACK_HANFSHARK>>16) & 0xFF;
	senddata[2]	= (ACK_HANFSHARK>>8)  & 0xFF;
	senddata[3]	= (ACK_HANFSHARK)	  & 0xFF;
    senddata[5] = s_md5result[0];
    senddata[6] = s_md5result[1];
    senddata[7] = s_md5result[2];
    senddata[8] = s_md5result[3];
    senddata[9] = s_md5result[4];
    senddata[10] = s_md5result[5];
    senddata[11] = s_md5result[6];
    senddata[12] = s_md5result[7];
	#if DEBUG_LOCK > 0
	debug_printf("WC_HandShake step:%d\r\n", s_sclockstep);
	#endif
    CAN_TxData(senddata, false, LOCK_CAN_CH);
}

/**************************************************************************************************
**  函数名称:  WC_CanDelayTmr
**  功能描述:  潍柴锁车延时处理
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
void WC_CanDelayTmr(void)
{
	static INT8U delay_cnt = 0;
	switch (s_sclockstep) {
		case HAND_SEND1:
			delay_cnt = 0;
			s_sclockstep = HAND_SEND2;
			WC_HandShake();
		break;
		case HAND_SEND2:
			if (++delay_cnt >= 5) {
				delay_cnt = 0;
				WC_HandShake();
				s_sclockstep = HAND_SEND3;
			}
		break;
		case HAND_SEND3:
			if (++delay_cnt >= 5) {
				delay_cnt = 0;
				WC_HandShake();
				s_wc_0100recv = FALSE;
				s_wc_0100cnt = 0;
				s_sclockstep = CONFIG_CONFIRM_REC;
			}
		break;
		case CONFIG_CONFIRM_REC:
			if ((s_wc_0100cnt >= 300) && (s_ishandover == FALSE)) {
				s_sclockstep = CONFIG_OVER;
				f_handsk = FALSE;		// 握手失败
				s_handshake_ack = HANDSHAKE_CHECKERR;
				s_ishandover = TRUE;
				#if DEBUG_LOCK > 0
				debug_printf("WC HANDSHAKE_CHECKERR\r\n");
				#endif
                LockSafeDataAdd(0x00, 1, &s_handshake_ack);
			}
		default:
		break;
	}

	if ((s_wc_0800recv == FALSE) && (s_wc_0800cnt < 102)) {
		if (++s_wc_0800cnt >= 100) {
			s_wc_0800cnt = 102;
			f_handsk = FALSE;
			s_handshake_ack = HANDSHAKE_BUSEXCEPTION;
			s_ishandover = TRUE;
			#if DEBUG_LOCK > 0
			debug_printf("WC HANDSHAKE_BUSEXCEPTION\r\n");
			#endif
            LockSafeDataAdd(0x00, 1, &s_handshake_ack);
		}
	}
	if ((s_wc_0100recv == FALSE) && ((s_wc_0100cnt < 1002))) {
		if (++s_wc_0100cnt >= 1000) {
			s_wc_0100cnt = 1002;
			s_handshake_ack = HANDSHAKE_ERR;
			s_ishandover = TRUE;
			#if DEBUG_LOCK > 0
			debug_printf("WC HANDSHAKE_ERR s_wc_0100cnt:%d\r\n", s_wc_0100cnt);
			#endif
            LockSafeDataAdd(0x00, 1, &s_handshake_ack);
		}
	}
}


/**************************************************************************************************
**  函数名称:  YC_HandShake
**  功能描述:  玉柴握手
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
void YC_HandShake(void)
{
    INT8U senddata[13] = {0x18,0xea,0x00,0x21,0x08,0x01,0xfd,0x00,0x00,0x00,0x00,0x00,0x00};//0x18ea0021
    INT32U id;
	#if LOCK_COLLECTION > 0
	INT8U canbuf[12] = {0};
	#endif

    if (s_sclockpara.ecutype != ECU_YUCHAI) return;
    switch(s_sclockstep) {
        case CONFIG_REQ:
            #if DEBUG_LOCK > 0
                debug_printf("配置发送\r\n");
            #endif
            CAN_TxData(senddata, false, LOCK_CAN_CH);
			#if LOCK_COLLECTION > 0
			memcpy(canbuf, senddata, 4);	// id
			memcpy(&canbuf[4], &senddata[5], 8);
			YX_AddDataCollection(canbuf);
			#endif
            id = bal_chartolong(senddata);
            SetCANMsg_Period(id, &senddata[4], YC_HANDSK_PERIOD / 10, LOCK_CAN_CH); /* 发送周期1000ms -> 800ms */
            s_sclockstep = RAND_CODE_REC;
            break;
        case CHECK_CODE_SEND:
            senddata[1] = 0xfe;         /* 发送 0x18fe02fb 报文 */
            senddata[2] = 0x02;
            senddata[3] = 0xfb;
            memcpy(&senddata[5],s_key,8);
            CAN_TxData(senddata, false, LOCK_CAN_CH);
            #if DEBUG_LOCK > 0
                debug_printf("校验码s_key发送  ");
                printf_hex(&senddata[5], 8);
            #endif
            //id = bal_chartolong(senddata);
            //SetCANMsg_Period(id, &senddata[4], YC_HANDSK_PERIOD/10, LOCK_CAN_CH); /* 发送周期1000ms -> 800ms */
            s_sclockstep = CONFIG_CONFIRM_REC;
            break;
        default:
            break;
    }
}

/**************************************************************************************************
**  函数名称:  YC_CanDelayTmr
**  功能描述:  玉柴锁车延时处理
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
void YC_CanDelayTmr(void)
{
	static INT16U ycdelay_cnt = 0;
	switch (s_sclockstep) {
		case RAND_CODE_REC:
			if (++ycdelay_cnt >= 800) {
				ycdelay_cnt = 0;
				s_sclockstep = CONFIG_OVER;
				s_ishandover = TRUE;
				f_handsk	= FALSE;
				s_handshake_ack = HANDSHAKE_BUSEXCEPTION;
                LockSafeDataAdd(0x00, 1, &s_handshake_ack);
			}
			break;
		case CONFIG_CONFIRM_REC:
			if (++ycdelay_cnt >= 300) {
				if (s_yc_state == 0x00) {
					s_handshake_ack = HANDSHAKE_CHECKERR;	// 校验失败
				} else if (s_yc_state == 0x11) {
					s_handshake_ack = HANDSHAKE_OVER;	// 校验失败
				}
				ycdelay_cnt = 0;
				s_sclockstep = CONFIG_OVER;
				s_ishandover = TRUE;
				f_handsk	= FALSE;
                LockSafeDataAdd(0x00, 1, &s_handshake_ack);
			}
			break;
		default:
			ycdelay_cnt = 0;
			break;
	}
}


/**************************************************************************************************
**  函数名称:  YC_Set18ea0021Period
**  功能描述:  玉柴设置周期发送请求
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
static void YC_Set18ea0021Period(void)
{
    INT8U senddata[13] = {0x18,0xea,0x00,0x21,0x08,0x01,0xfd,0x00,0x00,0x00,0x00,0x00,0x00};//0x18ea0021
    INT32U id;

    if (s_sclockpara.ecutype != ECU_YUCHAI) return;

    id = bal_chartolong(senddata);
    SetCANMsg_Period(id, &senddata[4], YC_HANDSK_PERIOD / 10, LOCK_CAN_CH); /* 发送周期1000ms -> 800ms */

}

/**************************************************************************************************
**  函数名称:  YN_HandShake
**  功能描述:  云内握手(做在MPU端)
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
void YN_HandShake(void)
{

}
/**************************************************************************************************
**  函数名称:  YN_CanDelayTmr
**  功能描述:  云内锁车延时处理(MPU端处理，功能预留)
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
void YN_CanDelayTmr(void)
{

}

/**************************************************************************************************
**  函数名称:  HandShakeMsgAnalyze
**  功能描述:  CAN锁车相关报文解析处理函数,放在中断中，及时处理
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
void HandShakeMsgAnalyze(CAN_DATA_HANDLE_T *CAN_msg, INT16U datalen)
{
    INT32U id;
    INT32U seed;
    INT8U can_seed[4];
    INT8U  senddata[13] = {0x18,0xfe,0x02,0xfb,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; // 0x18fe02fb
    #if LOCK_COLLECTION > 0
    INT8U can_buf[12] = {0};
	#endif


     id = bal_chartolong(CAN_msg->id);
	 seed = bal_chartolong(CAN_msg->databuf);
     
	if (id == CAN_SPEED_ID){
		SetSpeedFlag(TRUE);
	}
    if (s_sclockpara.ecutype == ECU_YUCHAI) {
		
        if (id == 0x18fd0100) {
			s_yc_state = (CAN_msg->databuf[5] & 0x30) >> 4;
			#if DEBUG_LOCK > 0
		        debug_printf("收到CANid:%x ecutype:%d s_sclockstep:%d buf\r\n", id, s_sclockpara.ecutype, s_sclockstep);
			 	Debug_PrintHex(true, CAN_msg->databuf, CAN_msg->len);
		    #endif
			#if LOCK_COLLECTION > 0
			memcpy(can_buf, CAN_msg->id, 4);
			memcpy(&can_buf[4], CAN_msg->databuf, CAN_msg->len);
			YX_AddDataCollection(can_buf);
			#endif
			if (s_sclockstep == RAND_CODE_REC) {
                #if DEBUG_LOCK > 0
                    debug_printf("随机码seed接收\r\n");
                #endif
                if (((CAN_msg->databuf[5] & 0x70) == 0x70) && (seed != 0) && ((CAN_msg->databuf[7] & 0x0f) == 0x0d)) {
                    #if DEBUG_LOCK > 0
                        debug_printf("随机码帧校验成功，计算key\r\n");
                    #endif
                    memcpy(s_seed, CAN_msg->databuf, 4);
                    YX_LC_InputSeed(s_seed,4);
                    YX_LC_GetHKey(s_key);
                    s_sclockstep = CHECK_CODE_SEND;
                    YC_HandShake();
                } else if (((CAN_msg->databuf[5] & 0x70) == 0x50) && (seed != 0)) {
                    s_sclockstep = CONFIG_OVER;
                    s_idfiltenable = TRUE;
                    f_handsk = true;
					s_handshake_ack = HANDSHAKE_OK;
					s_ishandover = TRUE;
                    LockSafeDataAdd(0x00, 1, &s_handshake_ack);
                    //StopCANMsg_Period(0x18fe02fb, LOCK_CAN_CH);
                    s_handskenable = FALSE;
                    /* 握手成功，发送全0清除标志到0x18fe02fb */
                    CAN_TxData(senddata, false, LOCK_CAN_CH);
                    #if DEBUG_LOCK > 0
                    debug_printf("已握手，结束流程\r\n");
					#endif
                }
           } else if (s_sclockstep == CONFIG_CONFIRM_REC) {
                if( (CAN_msg->databuf[5]& 0x30 ) == 0x10){//校验通过
                    s_sclockstep = CONFIG_OVER;
                    s_idfiltenable = TRUE;
                    f_handsk = true;
                    //StopCANMsg_Period(0x18fe02fb, LOCK_CAN_CH);
                    s_ishandover = TRUE;
                    s_handskenable = FALSE;
                    s_handshake_ack = HANDSHAKE_OK;
                    LockSafeDataAdd(0x00, 1, &s_handshake_ack);
                    /* 握手成功，发送全0清除标志到0x18fe02fb */
                    CAN_TxData(senddata, false, LOCK_CAN_CH);
                    #if DEBUG_LOCK > 0
                        debug_printf("握手成功\r\n");
                    #endif
                }/*else{
                    if((f_handskcnt++) == 3){
                        f_handskcnt = 0;
                        s_sclockstep = CONFIG_OVER;
                        StopCANMsg_Period(0x18fe02fb, LOCK_CAN_CH);
                        s_idfiltenable = TRUE;
                        f_handsk = false;
                        s_handskenable = FALSE;
                    }
                }*/
         	}


            memcpy(s_lockstatid, CAN_msg->id, 4);
            memcpy(s_lockstatid + 4,CAN_msg->databuf,8);
            //memcpy(s_seed, CAN_msg->databuf, 4);
            #if DEBUG_LOCK > 0
                debug_printf("id = %x\r\n",id);
            #endif

            memcpy(can_seed, CAN_msg->databuf, 4);
			#if DEBUG_TEMP > 1
			debug_printf("can_seed:");
			Debug_PrintHex(TRUE,can_seed,4);
			debug_printf("s_ycseed:");
			Debug_PrintHex(TRUE,s_ycseed,4);
			debug_printf("f_handsk:%d  unbindstat:%d\r\n",f_handsk,s_sclockpara.unbindstat);
			#endif
			#if 0
            /* 心跳校验 */
            if ((TRUE == f_handsk) && (!s_sclockpara.unbindstat)) {

                if( can_seed[0] != s_ycseed[0] ||
                    can_seed[1] != s_ycseed[1] ||
                    can_seed[2] != s_ycseed[2] ||
                    can_seed[3] != s_ycseed[3]) {
                    /* seed 更新，启动新的握手检验流程 */
                    if(can_seed[0] == 0&& can_seed[1] == 0 && can_seed[2] == 0&&can_seed[3] == 0){

                    }else{
                       // if ((CAN_msg->databuf[5] & 0x30) == 0x30) { /* 未校验 */
                            f_handsk     = FALSE;
                            s_sclockstep = CONFIG_REQ;
                            YC_HandShake();
                            #if DEBUG_LOCK > 0
                                debug_printf("YC_EcuAnalyze - seed 更新，启动新的握手检验流程  ");
                            #endif
							#if DEBUG_TEMP > 0
                                debug_printf("YC_EcuAnalyze - seed 更新，启动新的握手检验流程  ");
                            #endif
                       // }
                   }
                }
            }
			#else
			//激活未验证
			if (s_sclockstep == CONFIG_OVER){
				if (CAN_msg->databuf[5] == 0x70  && (seed != 0)){
					s_sclockstep = CONFIG_REQ;
	                YC_HandShake();
				}
			}
			#endif

            /* 保存seed */
            s_ycseed[0] = can_seed[0];
            s_ycseed[1] = can_seed[1];
            s_ycseed[2] = can_seed[2];
            s_ycseed[3] = can_seed[3];


        }
    }
	/**************************潍柴**********************/
    if (s_sclockpara.ecutype == ECU_WEICHAI) {
		#if DEBUG_LOCK > 0
		if ((id == 0x18fd0100) || (id == 0x18FF0800)) {
	        debug_printf("收到CANid:%x ecutype:%d s_sclockstep:%d s_sclockpara.unbindstat:%d buf\r\n", id, s_sclockpara.ecutype, s_sclockstep, s_sclockpara.unbindstat);
		 	Debug_PrintHex(true, CAN_msg->databuf, CAN_msg->len);
		}
	    #endif
        if (id == 0x18fd0100) {
			if (s_wc_0800recv == FALSE) {
				return;		// 未收到消贷报文，不响应握手
			}
			s_ishandover = FALSE;
            if(!s_sclockpara.unbindstat){
                s_md5source[0] = s_sclockpara.firmcode[0];
                s_md5source[1] = s_sclockpara.firmcode[1];
                s_md5source[2] = s_sclockpara.firmcode[2];
                s_md5source[3] = CAN_msg->databuf[0];
                s_md5source[4] = CAN_msg->databuf[1];
                s_md5source[5] = CAN_msg->databuf[2];
                s_md5source[6] = CAN_msg->databuf[3];
                s_md5source[7] = CAN_msg->databuf[4];
				#if DEBUG_LOCK > 0
			        debug_printf("\r\n", id, s_sclockpara.ecutype, s_sclockstep, s_sclockpara.unbindstat);
				 	Debug_PrintHex(true, CAN_msg->databuf, CAN_msg->len);
			    #endif
                MDString(s_md5source,s_md5result,sizeof(s_md5source));
                s_sclockstep = HAND_SEND1;
           }
			s_wc_0100recv = TRUE;
			s_wc_0100cnt  = 0;
        }
        if (id == 0x18FF0800) {
			s_wc_0800recv = TRUE; 
			s_wc_0800cnt  = 0;
            if (s_sclockpara.ecutype == ECU_WEICHAI) {
                if ((s_ishandover == FALSE) && ((CAN_msg->databuf[2] & WC_KEY_BIT) == WC_KEY_BIT)) {
                    s_idfiltenable = TRUE;
                    s_sclockstep = CONFIG_OVER;
					s_ishandover = TRUE;
					s_handshake_ack = HANDSHAKE_OK;
					f_handsk = TRUE;
					#if DEBUG_LOCK > 0
					debug_printf("WC HANDSHAKE_OK\r\n");
					#endif
                    LockSafeDataAdd(0x00, 1, &s_handshake_ack);
                }
                memcpy(s_lockstatid,CAN_msg->id,4);
                memcpy(s_lockstatid + 4,CAN_msg->databuf,8);
            }
        }
    }
	/*************************康明斯**********************/
	if (KMS_Q6_KEY_ID == id) {
		#if LOCK_COLLECTION > 0
		memcpy(can_buf, CAN_msg->id, 4);
		memcpy(&can_buf[4], CAN_msg->databuf, CAN_msg->len);
		YX_AddDataCollection(can_buf);
		#endif
		#if DEBUG_LOCK > 0
			debug_printf("%s id:%08x buf:",__FUNCTION__,id);
			Debug_PrintHex(true, CAN_msg->databuf,CAN_msg->len);
		#endif
		s_kms_delay_cnt = 200;	// 收到BCM回复后不发送周期报文
        if (CAN_msg->databuf[0] == 0xA1) {
            s_kmslockstep = KMS_SEND_KEY;
            KMS_HandShake(&CAN_msg->databuf[1], 7);
			
        }
		if ((CAN_msg->databuf[0] == 0xB1) && ((CAN_msg->databuf[1] & 0xC1) == 0xC1)){
			#if DEBUG_LOCK > 0
				debug_printf("databuf[0]:%02x databuf[1] & 0xC1=%02x\r\n",CAN_msg->databuf[0],CAN_msg->databuf[1] & 0xC1);
			#endif
			s_kmslockstep = KMS_REQ_SEED;
            KMS_HandShake(NULL, 0);
			KMS_Hand_Send_Set(false);
		}
        if (s_sclockpara.ecutype == ECU_KMS_Q6) {
            memcpy(s_lockstatid, CAN_msg->id, 4);
            memcpy(s_lockstatid + 4, CAN_msg->databuf, 8);
        }
    }
    if ((s_sclockpara.ecutype == ECU_XICHAI_EMSECO) || (s_sclockpara.ecutype == ECU_XICHAI_EMSMDI) || (s_sclockpara.ecutype == ECU_XICHAI_EMSVI)) {
        if (id == 0x18FF7800) {
            memcpy(s_xichai_seed, CAN_msg->databuf, 8);
            s_sclockstep = CHECK_CODE_SEND;
            XC_Hashcal();
            if (hash_cnt != 0) {
                s_ishandover = TRUE;
                s_handshake_ack = HANDSHAKE_CHECKERR;
                f_handsk = TRUE; 
                LockSafeDataAdd(0x00, 1, &s_handshake_ack);
            }
            XC_Checkcode();
        }
    }
}
/**************************************************************************************************
**  函数名称:  SendLockPara
**  功能描述:  发送锁车同步帧
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/

void SendLockPara(void)
{
    INT8U senddata[200];
    INT8U len = 0;

    bal_pp_ReadParaByID(SCLOCKPARA_,(INT8U *)&s_sclockpara, sizeof(SCLOCKPARA_T));
    bal_shorttochar(senddata, CLIENT_CODE);
    senddata[2] = 0x01;
    len = 3;
    senddata[len++] = s_sclockpara.ecutype;
    senddata[len++] = s_sclockpara.firmcodelen;
    memcpy(&senddata[len], s_sclockpara.firmcode, s_sclockpara.firmcodelen);
    len += s_sclockpara.firmcodelen;
    senddata[len++] = s_sclockpara.srlnumberlen;
    memcpy(&senddata[len], s_sclockpara.serialnumber, s_sclockpara.srlnumberlen);
    len += s_sclockpara.srlnumberlen;
    senddata[len++] = s_sclockpara.unbindstat;
    senddata[len++] = s_sclockpara.limitfunction;
    memcpy(&senddata[len], s_lockstatid, 12);
    if(GetAccState() == false){
        memset(&s_lockstatid, 0, sizeof(s_lockstatid));
    }
    YX_COM_DirSend(CLIENT_FUNCTION_DOWN_REQ_ACK, senddata, len + 12);

}

/**************************************************************************************************
**  函数名称:  Lock_RecordReqHdl
**  功能描述:  车台获取M3锁车日志
**  输入参数:
**  返回参数:  None
**************************************************************************************************/
void Lock_RecordReqHdl(void)
{
    INT8U senddata[150];
    INT8U datalen = 0, i;

    senddata[datalen++] = 0x09;
    senddata[datalen++] = 0x18;
    senddata[datalen++] = 0x02;
    senddata[datalen++] = s_lockrecord.storenum;
    if (s_lockrecord.storenum < 10) {
        for (i = 0;i < s_lockrecord.storenum; i++) {
            senddata[datalen++] = 8;
            memcpy(senddata + datalen, &s_lockrecord.lockrecord[i].locktime, 6);
            datalen += 6;
            senddata[datalen++] = s_lockrecord.lockrecord[i].locktype;
            senddata[datalen++] = 0;
        }
    } else {
        for (i = 0; i < 10; i++) {
            senddata[datalen++] = 8;
            memcpy(senddata + datalen, &s_lockrecord.lockrecord[(s_lockrecord.storeadd + i)%10].locktime, 6);
            datalen += 6;
            senddata[datalen++] = s_lockrecord.lockrecord[(s_lockrecord.storeadd + i)%10].locktype;
            senddata[datalen++] = 0;
        }
    }

    YX_COM_DirSend(CLIENT_FUNCTION_DOWN_REQ_ACK, senddata, datalen);
    #if DEBUG_CAN > 0
        debug_printf("锁车日志");
        printf_hex(senddata, datalen);
    #endif

}

/**************************************************************************************************
**  函数名称:  SecurityKeySet
**  功能描述:  秘钥设置
**  输入参数:  buf--秘钥数组
**  输出参数:  无
**  返回参数:  无
**************************************************************************************************/
static void SecurityKeySet(INT8U *buf,INT8U len)
{
	if(len != 16){
		return ;
	}
	#if 0
	s_securitykey[0] = (buf[3] 	<< 24) + (buf[2]  << 16) + (buf[1]  << 8) + buf[0];
	s_securitykey[1] = (buf[7] 	<< 24) + (buf[6]  << 16) + (buf[5]  << 8) + buf[4];
	s_securitykey[2] = (buf[11] << 24) + (buf[10] << 16) + (buf[9]  << 8) + buf[8];
	s_securitykey[3] = (buf[15] << 24) + (buf[14] << 16) + (buf[13] << 8) + buf[12];
	#else
	for (INT8U i =0; i < len/4; i++){
		s_securitykey[i] = (buf[i*4] << 24) + (buf[i*4+1] << 16) + (buf[i*4+2] << 8) + buf[i*4+3];
	}
	#endif
	#if DEBUG_TEMP > 0
		debug_printf("s_securitykey--0x:%x  0x:%x 0x:%x 0x:%x\r\n",s_securitykey[0],s_securitykey[1],s_securitykey[2],s_securitykey[3]);
	#endif
}


/**************************************************************************************************
**  函数名称:  LockParaStore
**  功能描述:  锁车参数存储，用于同步锁车状态
**  输入参数:  ECU类型（1）+密钥（3）+GPSid（3）+锁车类型（1）
**  输出参数:  无
**  返回参数:  无
**************************************************************************************************/
void LockParaStore(INT8U *userdata, INT8U userdatalen)
{
    BOOLEAN change = FALSE;
    BOOLEAN unbindchange = FALSE;
    INT8U len = 0,len1 = 0, i = 0;
	INT8U buf[16] = {0};

    //if (userdatalen != 8) return;

    #if 0 /* 对于潍柴锁车，10s后就停止了；对于玉柴锁车，由车台控制何时停止 */
    if (s_sclockpara.ecutype != ECU_YUCHAI) { /* 玉柴, 由车台控制何时停止 */
        if (s_parasendstat) {
            StopCANMsg_Period(0x18ea0021,0);  /* 第一次收到车台同步帧(FDH)，停止发送0x18ea0021 */
        }
    }
    #endif

    s_parasendstat = FALSE;
    #if DEBUG_LOCK > 0
        debug_printf("下发锁车参数FD-01:");
        printf_hex(userdata,userdatalen);
        debug_printf("\r\n");
    #endif

	if (userdata[0] != ECU_KMS_Q6){						//康明斯长度不一样
		if (userdatalen != 13) {                        // 总长度不符合
			#if EN_DEBUG > 0
	            debug_printf("总长度不符合");
	        #endif
	        SendLockPara();
	        return;
	    }
	}

    if (s_sclockpara.ecutype != userdata[len]) {
        //if (userdata[len] >= MAX_ECU_TYPE) {        // 发动机类型非法
        //    SendLockPara();
        //    return;
        //}
        change = TRUE;
        s_sclockpara.ecutype = userdata[len];
    }
    len++;
    len1 = userdata[len];
	if (s_sclockpara.ecutype == ECU_YUCHAI) {           // 玉柴及潍柴的固定密钥和GPSID长度分别为(3和3)及(2和4)
        if (len1 != 2) {                                // 固定密钥长度非法
            SendLockPara();
            return;
        }
    } else if (s_sclockpara.ecutype == ECU_WEICHAI) {
        if (len1 != 3) {                                // 固定密钥长度非法
            SendLockPara();
            return;
        }
    }

    if (s_sclockpara.firmcodelen != len1) {
        change = TRUE;
        s_sclockpara.firmcodelen = len1;
        memcpy(s_sclockpara.firmcode, &userdata[len + 1], len1);
		/*for (i = 0; i < len1; i++){
			s_sclockpara.firmcode[i] = userdata[len + 1 + len1 - i - 1];
		}*/
    } else {
    	/*for (i = 0; i < len1; i++){
			buf[i] = userdata[len + 1 + len1 - i - 1];
		}*/
        if (STR_EQUAL != bal_ACmpString(FALSE, s_sclockpara.firmcode, buf, s_sclockpara.firmcodelen, len1)) {
            change = TRUE;
            memcpy(s_sclockpara.firmcode, &userdata[len + 1], len1);
            //for (i = 0; i < len1; i++){
			//	s_sclockpara.firmcode[i] = userdata[len + 1 + len1 - i - 1];
			//}
        }
    }
    len += (len1 + 1);

    len1 = userdata[len];
    if (s_sclockpara.ecutype == ECU_YUCHAI) {
        if (len1 != 4) {                                // GPSID度非法
            SendLockPara();
            return;
        }
    } else if (s_sclockpara.ecutype == ECU_WEICHAI) {
        if (len1 != 3) {                                // GPSID非法
            SendLockPara();
            return;
        }
    } 
    if (s_sclockpara.srlnumberlen != len1) {
        change = TRUE;
        s_sclockpara.srlnumberlen = len1;
        memcpy(s_sclockpara.serialnumber, &userdata[len + 1], len1);
        /*for (i = 0; i < len1; i++){
			s_sclockpara.serialnumber[i] = userdata[len + 1 + len1 - i - 1];
		}*/
    } else {
	    /*for (i = 0; i < len1; i++){
			buf[i] = userdata[len + 1 + len1 - i - 1];
		}*/
        if (STR_EQUAL != bal_ACmpString(FALSE, s_sclockpara.serialnumber, buf, s_sclockpara.srlnumberlen, len1)) {
            change = TRUE;
            memcpy(s_sclockpara.serialnumber, &userdata[len + 1], len1);
            //for (i = 0; i < len1; i++){
			//	s_sclockpara.serialnumber[i] = userdata[len + 1 + len1 - i - 1];
			//}
        }
    }
    len += (len1 + 1);

	len1 = userdata[len];
	if (s_sclockpara.ecutype == ECU_KMS_Q6) {		//康明斯securityKey
		if(len1 != 16 || len1 == 0){
			SendLockPara();
            return;
		}
	}
	if (s_sclockpara.securityKeylen != len1) {
        change = TRUE;
        s_sclockpara.securityKeylen = len1;
        memcpy(s_sclockpara.securityKey, &userdata[len + 1], len1);
    } else {
    	for (i = 0; i < len1; i++){
			buf[i] = userdata[len + 1 + len1 - i - 1];
		}
        if (STR_EQUAL != bal_ACmpString(FALSE, s_sclockpara.securityKey, buf, s_sclockpara.securityKeylen, len1)) {
            change = TRUE;
            memcpy(s_sclockpara.securityKey, &userdata[len + 1], len1);
        }
    }
	SecurityKeySet(s_sclockpara.securityKey, s_sclockpara.securityKeylen);
	len += (len1 + 1);

	len1 = userdata[len];
	if (s_sclockpara.ecutype == ECU_KMS_Q6) {		//康明斯deratespeed
		if(len1 != 2 || len1 == 0){
			SendLockPara();
            return;
		}
	}
	if (s_sclockpara.deratespeedlen!= len1) {
        change = TRUE;
        s_sclockpara.deratespeedlen = len1;
        //memcpy(s_sclockpara.deratespeed, &userdata[len + 1], len1);
        for (i = 0; i < len1; i++){
			s_sclockpara.deratespeed[i] = userdata[len + 1 + len1 - i - 1];
		}
    } else {
    	for (i = 0; i < len1; i++){
			buf[i] = userdata[len + 1 + len1 - i - 1];
		}
        if (STR_EQUAL != bal_ACmpString(FALSE, s_sclockpara.deratespeed, buf, s_sclockpara.deratespeedlen, len1)) {
            change = TRUE;
            //memcpy(s_sclockpara.deratespeed, &userdata[len + 1], len1);
            for (i = 0; i < len1; i++){
				s_sclockpara.deratespeed[i] = userdata[len + 1 + len1 - i - 1];
			}
        }
    }
	len += (len1 + 1);
	
    if (s_sclockpara.unbindstat != userdata[len]) {
        s_sclockpara.unbindstat = userdata[len];
        change = TRUE;
        unbindchange = TRUE;
        if(!s_sclockpara.unbindstat){
            s_sclockstep = CONFIG_REQ;
			#if DEBUG_LOCK > 0
				debug_printf("%s s_sclockstep set CONFIG_REQ\r\n",__FUNCTION__);
			 #endif
            YC_HandShake();
        }else{
            f_handsk = FALSE;
            StopCANMsg_Period(0x18ea0021, LOCK_CAN_CH);
        }

        #if EN_DEBUG > 0
            debug_printf("解绑状态改变:%d \r\n", s_sclockpara.unbindstat);
        #endif
    }
    len++;
    if (s_sclockpara.limitfunction != userdata[len]) {
        change = TRUE;

        if(userdata[len] == LC_CMD_BIND){
           YC_Set18ea0021Period();
        }

        if (++s_lockrecord.storenum >= 10) {
            s_lockrecord.storenum = 10;
        }
        PORT_GetSysTime1((SYSTIME_T *)&s_lockrecord.lockrecord[s_lockrecord.storeadd].locktime);
        s_lockrecord.lockrecord[s_lockrecord.storeadd].locktype = userdata[len];
        if (++s_lockrecord.storeadd >= 10) {
            s_lockrecord.storeadd = 0;
        }

        s_sclockpara.configsuccess = FALSE;
        s_sclockpara.limitfunction = userdata[len];
    }
    len++;

    if (change) {
        #if EN_DEBUG > 0
            debug_printf("同步状态改变 \r\n");
            debug_printf("LockParaStore ecutype:%d unbindstat:%d\r\n", s_sclockpara.ecutype, s_sclockpara.unbindstat);
        #endif


        bal_pp_StoreParaByID(SCLOCKPARA_, (INT8U *)&s_sclockpara, sizeof(SCLOCKPARA_T));
        bal_pp_StoreParaByID(LOCKRECORD_, (INT8U *)&s_lockrecord, sizeof(LOCK_RECORD));

        if (unbindchange) {
            BakParaSave(s_sclockparabak.lock_en);
        }

    }
    SendLockPara();
}

void GetGpsID(INT8U *gpsid)
{
    memcpy(gpsid, s_sclockpara.serialnumber,s_sclockpara.srlnumberlen);
}

/**************************************************************************************************
**  函数名称:  LockParaBak
**  功能描述:  锁车解绑状态备份，锁车功能开关同步
**  输入参数:  锁车功能开关（1）
**  输出参数:  TRUE-保存成功  FALSE-未保存
**  返回参数:  无
**************************************************************************************************/
BOOLEAN LockParaBak(INT8U *userdata, INT8U userdatalen)
{
    #if EN_DEBUG > 0
        debug_printf("锁车解绑状态备份，锁车功能开关同步 len:%d  data:", userdatalen);
        printf_hex(userdata,userdatalen);
    #endif

    if (userdatalen != 1) {                        // 总长度不符合
        return FALSE;
    }

    bal_pp_ReadParaByID(SCLOCKPARABAK_,(INT8U *)&s_sclockparabak, sizeof(SCLOCKPARABAK_T));

    if (s_sclockparabak.lock_en != userdata[0]) {
        s_sclockparabak.lock_en = userdata[0];
        BakParaSave(s_sclockparabak.lock_en);
    } else {
        return FALSE;
    }

    return TRUE;
}

/**************************************************************************************************
**  函数名称:  BakParaSave
**  功能描述:  锁车解绑状态备份，锁车功能开关同步
**  输入参数:  locken: 锁车功能开关
**  输出参数:  无
**  返回参数:  无
**************************************************************************************************/
void BakParaSave(INT8U locken)
{
    INT8U sum;

    bal_pp_ReadParaByID(SCLOCKPARA_,(INT8U *)&s_sclockpara, sizeof(SCLOCKPARA_T));

    s_sclockparabak.lock_en = locken;
    s_sclockparabak.lock_en_bak1 = locken;
    s_sclockparabak.lock_en_bak2 = locken;
    s_sclockparabak.unbindstat_bak1 = s_sclockpara.unbindstat;
    s_sclockparabak.unbindstat_bak2 = s_sclockpara.unbindstat;
    sum = s_sclockparabak.lock_en + s_sclockparabak.lock_en_bak1 + s_sclockparabak.lock_en_bak1
        + s_sclockparabak.unbindstat_bak1 + s_sclockparabak.unbindstat_bak2;
    s_sclockparabak.chksum = sum;

    #if EN_DEBUG > 0
        debug_printf("BakParaSave \r\n");
        debug_printf("lock_en:%d lock_en bak1:%d lock_en bak2:%d\r\n", s_sclockparabak.lock_en, s_sclockparabak.lock_en_bak1, s_sclockparabak.lock_en_bak2);
        debug_printf("unbindstat:%d unbindstat bak1:%d unbindstat bak2:%d\r\n", s_sclockpara.unbindstat, s_sclockparabak.unbindstat_bak1, s_sclockparabak.unbindstat_bak2);
        debug_printf("s_sclockparabak.chksum:0x%02x\r\n\r\n", s_sclockparabak.chksum);
    #endif

}


/**************************************************************************************************
**  函数名称:  IsBind
**  功能描述:  判断是否为绑定状态
**  输入参数:  无
**  输出参数:  无
**  返回参数:  无
**************************************************************************************************/
BOOLEAN IsShakeEn(void)
{
    INT8U chksum;

    bal_pp_ReadParaByID(SCLOCKPARABAK_,(INT8U *)&s_sclockparabak, sizeof(SCLOCKPARABAK_T));

    chksum = s_sclockparabak.lock_en + s_sclockparabak.lock_en_bak1 + s_sclockparabak.lock_en_bak1
           + s_sclockparabak.unbindstat_bak1 + s_sclockparabak.unbindstat_bak2;
    if (chksum != s_sclockparabak.chksum) {
        #if EN_DEBUG > 0
            debug_printf("IsShakeEn FALSE (校验和错误) \r\n");
        #endif
        return FALSE;
    }


    #if EN_DEBUG > 0
        debug_printf("s_sclockpara.unbindstat:%d bak1:%d bak2:%d\r\n", s_sclockpara.unbindstat, s_sclockparabak.unbindstat_bak1, s_sclockparabak.unbindstat_bak2);
        debug_printf("s_sclockparabak.lock_en:%d bak1:%d bak2:%d\r\n", s_sclockparabak.lock_en, s_sclockparabak.lock_en_bak1, s_sclockparabak.lock_en_bak2);
    #endif

    if ((!s_sclockpara.unbindstat)
     && (!s_sclockparabak.unbindstat_bak1)
     && (!s_sclockparabak.unbindstat_bak2)
     && (s_sclockparabak.lock_en)
     && (s_sclockparabak.lock_en_bak1)
     && (s_sclockparabak.lock_en_bak2)){
        #if EN_DEBUG > 0
            debug_printf("IsShakeEn TRUE \r\n");
        #endif
        return TRUE;
    } else {
        #if EN_DEBUG > 0
            debug_printf("IsShakeEn FALSE (条件不满足) \r\n");
        #endif
        return FALSE;
    }
}

/**************************************************************************************************
**  函数名称:  CanLocateFiltidSet
**  功能描述:  锁车开机握手ID配置
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
void CanLocateFiltidSet(void)
{
    IDPARA_T canid;
    INT8U i,idnum,j;

    canid.isused = FALSE;
    for (i = 0; i < MAX_CANIDS; i++) {                      /* 清除所有配置过的ID滤波对象 */
        SetIDPara(&canid, i, 1);
    }

    PORT_ClearCanFilter(LOCK_CAN_CH);                           /* 清除消息对象 */

    idnum = sizeof(s_filtid)/sizeof(s_filtid[0]);
    #if DEBUG_CAN > 0
        debug_printf("idnum = %d\r\n",idnum);
    #endif
    //idnum = 4;
    for (i = 0; i < idnum; i++) {
        PORT_SetCanFilter(s_filtid[i].chn, 1,s_filtid[i].filtrid, s_filtid[i].filtrid_mask);
        canid.id = s_filtid[i].filtrid;
        canid.stores = 1;
        canid.isused = TRUE;
        for (j = 0; j < MAX_CANIDS; j++) {
            if (FALSE == GetIDIsUsed(j, s_filtid[i].chn)) break;        /* 查找到未被配置的，可以配置 */
        }
        SetIDPara(&canid, j, s_filtid[i].chn);
    }
}

/**************************************************************************************************
**  函数名称:  GetEcuType
**  功能描述:  获取ECU锁车类型
**  输入参数:
**  返回参数:  None
**************************************************************************************************/
ECU_TYPE_E GetEcuType(void)
{
    return s_sclockpara.ecutype;
}


/**************************************************************************************************
**  函数名称:  CanSendTmr
**  功能描述:  锁车延时处理
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
void CanDelayTmr(void)
{
	INT8U ecutype = GetEcuType();
	switch (ecutype) {
		case ECU_WEICHAI:
			WC_CanDelayTmr();
			break;
		case ECU_YUCHAI:
			YC_CanDelayTmr();
			break;
		case ECU_DACHAI:
			DC_CanDelayTmr();
			break;
		case ECU_XICHAI:
			XC_CanDelayTmr();
			break;
        case ECU_YUNNEI:
            YN_CanDelayTmr();
            break;
		default:
			break;
	}
    if (++s_igoncnt > 100*1000) {   // 暂定最大值1000s
        s_igoncnt    = 100*1000;
    }
}

/**************************************************************************************************
**  函数名称:  LockSafeDataTran
**  功能描述:  锁车安全数据循环上报
**  输入参数:  无
**  输出参数:  无
**  返回参数:  无
**************************************************************************************************/
static void LockSafeDataTran(void)
{
    static INT8U delay = 0;
    INT8U sendbuf[64] = {0};
    INT8U i = 0;
    if (++delay < 100) {    // 周期1s
        return;
    }
    delay = 0;

    bal_shorttochar(sendbuf, CLIENT_CODE);      // 客户编号
    sendbuf[2] = 0x35;                          // 协议命令
    for (i = 0; i < HANDDATANUM;i++) {
        if (s_lockmsg_buf->active[i] == TRUE) {
            sendbuf[3] = s_lockmsg_buf->buf[i][0];      // 流水号
            sendbuf[4] = s_lockmsg_buf->buf[i][1];      // 参数类型
            memcpy(&sendbuf[5], &s_lockmsg_buf->buf[i][3], s_lockmsg_buf->buf[i][2]);
            YX_COM_DirSend(CLIENT_FUNCTION_DOWN_REQ, sendbuf, s_lockmsg_buf->buf[i][2]+5);
			#if DEBUG_LOCK > 0
			debug_printf("LockSafeDataTran %d\r\n", i);
			Debug_PrintHex(TRUE, sendbuf, s_lockmsg_buf->buf[i][2]+5);
			#endif
        }
    }
}


/**************************************************************************************************
**  函数名称:  LockSafeDataTran
**  功能描述:  锁车安全数据回复解析
**  输入参数:  无
**  输出参数:  无
**  返回参数:  无
**************************************************************************************************/
void LockSafeDataAck(INT8U *userdata, INT8U userdatalen)
{
    INT8U seq, result, i;
    #if DEBUG_LOCK > 0
    debug_printf("LockSafeDataAck data:");
    Debug_PrintHex(TRUE, userdata, userdatalen);
    #endif
    seq     = userdata[0];
    result  = userdata[1];
    for (i = 0; i < HANDDATANUM; i++) {
        if (s_lockmsg_buf->active[i] == TRUE) {
            if ((s_lockmsg_buf->buf[i][0] == seq) && (result == 0x00)) {
               s_lockmsg_buf->active[i] = FALSE;
               break;
            }
        }
    }
}
/**************************************************************************************************
**  函数名称:  LockTmrProc
**  功能描述:  锁车定时器 10ms
**  输入参数:  无
**  输出参数:  无
**  返回参数:  无
**************************************************************************************************/
static void LockTmrProc(void*index)
{
    INT8U acc_state,emstype;
    #if EN_KMS_LOCK > 0
    static INT16U acc_off_delay = 0;
    #endif
    INT8U senddata[13] = {0x18,0xEA,0x00,0x17,0x08,0xAF,0xFE,0x00,0xFF,0xFF,0xFF,0xFF,0xFF}; /* 0x18ea0017 */
	INT32U accpwrad;
    //acc_state = bal_input_ReadSensorFilterStatus(TYPE_ACC);
	accpwrad	 = PORT_GetADCValue(ADC_ACCPWR);
	if(accpwrad >= ADC_ACC_VALID){
		acc_state = TRUE;
	}else{
		acc_state = FALSE;
	}

	CanDelayTmr();
	LockSafeDataTran();
    #if EN_KMS_LOCK > 0
    if (!acc_state) {
        acc_off_delay = 3000;    /* 延时30秒 */
        KmsG5LockMsgSend();
    } else {
        /* acc off延时30秒停止发送 */
        if (acc_off_delay) {
            acc_off_delay--;
            KmsG5LockMsgSend();
        }
    }
    #endif
	// 康明斯 2s 延时发送握手报文
	if(GetEcuType() == ECU_KMS_Q6){
		if ((s_kms_delay_cnt++ % 10) == 0){
			if(s_kms_delay_cnt < 200){
				if (s_sclockpara.limitfunction < MAX_LC_CMD) {
					s_kmslockstep = KMS_REQ_SEED;
	            	KMS_HandShake(NULL, 0);
		        }
			}else {
				s_kms_delay_cnt = 201;
			}
			
		}
	}

    #if 0
    if (++s_reqcnt >= 3000/*2500*/) {                /* 2017-06-12 修改油耗请求周期为30秒 */
        s_reqcnt = 0;
        senddata[5] = 0xe9;                        /* 0xe9 请求油耗 */
        CAN_TxData(senddata, false, 1);
    }
    #else
    if(acc_state == TRUE){    //ACC ON
        if(s_poweron_times_cnt++ < 6000 ){// 6000    /* 上电1分钟内检测有没有收到油耗报文ID:0x18FEE900 */
            s_oilsumreq = GetOilMsg_State();
            s_reqcnt = 0;                             /* 如果没读到该ID,可以马上上报油耗请求 */
        } else {
            s_poweron_times_cnt = 6000;
            if(s_oilsumreq == TRUE){
                if (++s_reqcnt >= 1000) {                /* 油耗请求周期为1秒 */
                    s_reqcnt = 0;
										emstype = YX_Get_EMSType();
										if((emstype == 0x01) || (emstype == 0x02) || (emstype == 0x03) || (emstype == 0x06)
										|| (emstype == 0x08) || (emstype == 0x0A) || (emstype == 0x0C) || (emstype == 0x0D)
										|| (emstype == 0x0F) || (emstype == 0x11) || (emstype == 0x12) || (emstype == 0x14)
										|| (emstype == 0x16) || (emstype == 0x18)) {
										     senddata[5] = 0xE9;
										}

										if((emstype == 0x09) || (emstype == 0x0E) || (emstype == 0x15) || (emstype == 0x17)) {
										     senddata[3] = 0x21;
										}
                    CAN_TxData(senddata, false, LOCK_CAN_CH);
                }
            }
        }
    }
    else{    //ACC OFF
        s_poweron_times_cnt = 0;
        ResetOilMsg_State();
    }
    #endif


    if (++s_idfiltcnt >= 900) {                    /* 10秒后允许车台进行总线配置 */
        s_idfiltenable = TRUE;
        if(TRUE == s_handskenable){
            s_handskenable = FALSE;
            if(s_sclockstep == RAND_CODE_REC){
                s_sclockstep = CONFIG_OVER;
            }
            if (s_sclockpara.ecutype != ECU_YUCHAI) {/* 玉柴10s超时不停止发送，由车台控制何时停止 */
                StopCANMsg_Period(0x18ea0021, LOCK_CAN_CH);
            }else{
                if(s_sclockpara.unbindstat == 0){
					#if DEBUG_LOCK > 0
						debug_printf("%s stop 0x18ea0021\r\n",__FUNCTION__);
					#endif
                    StopCANMsg_Period(0x18ea0021, LOCK_CAN_CH);
                }
            }
        }
    }
	#if LOCK_COLLECTION > 0
	SendDataCollection();			// 发送采集数据
	#endif
}

//加密函数
void KMS_TEA_encrypt(INT8U *v, const INT32U *k)
{
    INT32U v0, v1, sum = 0, i;                         /* set up */
    INT32U delta = 0x9E3779B9;                         /* a key schedule constant */
    INT32U k0 = k[0], k1 = k[1], k2 = k[2], k3 = k[3]; /* cache key */

    v0 = (v[0] << 24) | (v[1] << 16) | (v[2] << 8) | (v[3] << 0);
    v1 = (v[4] << 24) | (v[5] << 16) | (v[6] << 8) | (v[7] << 0);
    for (i = 0; i < 32; ++i) {                           /* basic cycle start */
        sum += delta;
        v0 += ((v1<<4) + k0) ^ (v1 + sum) ^ ((v1>>5) + k1);
        v1 += ((v0<<4) + k2) ^ (v0 + sum) ^ ((v0>>5) + k3);
    }                                                    /* end cycle */

    v[0] = (v0 & 0xFF000000) >> 24;
    v[1] = (v0 & 0x00FF0000) >> 16;
    v[2] = (v0 & 0x0000FF00) >> 8;
    v[3] = (v0 & 0x000000FF) >> 0;
    v[4] = (v1 & 0xFF000000) >> 24;
    v[5] = (v1 & 0x00FF0000) >> 16;
    v[6] = (v1 & 0x0000FF00) >> 8;
    v[7] = (v1 & 0x000000FF) >> 0;
}
//解密函数
void KMS_TEA_decrypt(INT8U *v, const INT32U *k)
{
    INT32U v0 = v[0], v1 = v[1], sum = 0xC6EF3720, i;  /* set up */
    INT32U delta = 0x9E3779B9;                         /* a key schedule constant */
    INT32U k0 = k[0], k1 = k[1], k2 = k[2], k3 = k[3]; /* cache key */

    v0 = (v[0] << 24) | (v[1] << 16) | (v[2] << 8) | (v[3] << 0);
    v1 = (v[4] << 24) | (v[5] << 16) | (v[6] << 8) | (v[7] << 0);
    for (i = 0; i < 32; ++i) {                           /* basic cycle start */
        v1 -= ((v0<<4) + k2) ^ (v0 + sum) ^ ((v0>>5) + k3);
        v0 -= ((v1<<4) + k0) ^ (v1 + sum) ^ ((v1>>5) + k1);
        sum -= delta;
    }                                                    /* end cycle */

    v[0] = (v0 & 0xFF000000) >> 24;
    v[1] = (v0 & 0x00FF0000) >> 16;
    v[2] = (v0 & 0x0000FF00) >> 8;
    v[3] = (v0 & 0x000000FF) >> 0;
    v[4] = (v1 & 0xFF000000) >> 24;
    v[5] = (v1 & 0x00FF0000) >> 16;
    v[6] = (v1 & 0x0000FF00) >> 8;
    v[7] = (v1 & 0x000000FF) >> 0;
}

//握手加密
void KMS_HandshakeEncrypt(const INT8U iseed[7], INT8U deactive, 
    const INT8U gpsid[4], INT16U deratespeed, INT8U content[8])
{
    INT8U i = 0;
    INT8U x3send[8] = {0};
    INT8U plaintext[8] = {0};

    memcpy(x3send, iseed, 7);
    x3send[7] = 0xFF;
    
    plaintext[0] = deactive;
    memcpy(&plaintext[1], gpsid, 4);
    plaintext[5] = (deratespeed & 0x00FF) >> 0;
    plaintext[6] = (deratespeed & 0xFF00) >> 8;
    plaintext[7] = 0xFF;

    for (i = 0; i < sizeof(x3send); ++i) {
        content[i] = x3send[i] ^ plaintext[i];
    }

    KMS_TEA_encrypt(content, s_securitykey);
}
//激活加密
void KMS_ActiveEncrypt(const INT8U iseed[7], const INT8U mkey[8], 
    const INT8U gpsid[4], INT8U content[16])
{
    INT8U i = 0;
    INT8U x3send[8] = {0};
    INT8U plaintext1[8] = {0};
    INT8U plaintext2[8] = {0};
    INT8U xorval[8] = {0};

    memcpy(x3send, iseed, 7);
    x3send[7] = 0xFF;
    
	memcpy(plaintext1, mkey, sizeof(plaintext1));
	
    for (i = 0; i < sizeof(x3send); ++i) {
        xorval[i] = x3send[i] ^ plaintext1[i];
    }
    memcpy(content, xorval, sizeof(xorval));
    KMS_TEA_encrypt(content, s_securitykey);

    memcpy(&plaintext2[0], gpsid, 4);
    memcpy(&plaintext2[4], x3send, 4);
    
    for (i = 0; i < sizeof(xorval); ++i) {
        xorval[i] = xorval[i] ^ content[i];
    }
    for (i = 0; i < sizeof(xorval); ++i) {
        xorval[i] = xorval[i] ^ plaintext2[i];
    }
    memcpy(&content[8], xorval, sizeof(xorval));
    KMS_TEA_encrypt(&content[8], s_securitykey);
}

/**************************************************************************************************
**  函数名称:  KMS_Hand_Send_Set
**  功能描述:  康明斯握手发送使能
**  输入参数:  en:使能/失能
**  返回参数:  None
**************************************************************************************************/
void KMS_Hand_Send_Set(BOOLEAN en)
{
	s_kms_hand_send_enable = en;
}


/**************************************************************************************************
**  函数名称:  KMS_HandShake
**  功能描述:  康明斯握手
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
static void KMS_HandShake(INT8U *data, INT8U len)
{
    INT8U binddata[13] = {0x18,0xff,0xc9,0x4a,0x08,0xa0,0x3D,0xD1,0x0A,0xE0,0x53,0xD3,0x49};
    INT8U lockdata[13] = {0x18,0xff,0xc9,0x4a,0x08,0xb1,0x3D,0xD1,0xC6,0x25,0x27,0xE9,0x5B};
    INT8U senddata[13] = {0x18,0xff,0xcb,0x4a,0x08,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
    INT8U key[16];
	INT16U seppd = (s_sclockpara.deratespeed[0] << 8) + s_sclockpara.deratespeed[1];
	#if LOCK_COLLECTION > 0
	INT8U canbuf[12];
	#endif

    if (s_sclockpara.ecutype != ECU_KMS_Q6) return;
    if (s_sclockpara.limitfunction >= MAX_LC_CMD) return;
    switch(s_kmslockstep) {
        case KMS_REQ_SEED:
            #if DEBUG_LOCK > 0
                Debug_SysPrint("配置发送  limitfunction:%d\r\n",s_sclockpara.limitfunction);
            #endif
            if (s_sclockpara.limitfunction == LC_CMD_BIND) {
				#if LOCK_COLLECTION > 0
					memcpy(canbuf, binddata, 4);	// id
					memcpy(&canbuf[4], &binddata[5], 8);
					YX_AddDataCollection(canbuf);	// 添加采集数据
				#endif
                CAN_TxData(binddata, false, LOCK_CAN_CH);
            } else {
            	if(s_kms_hand_send_enable == true){
					if (s_sclockpara.unbindstat != 1){
						#if LOCK_COLLECTION > 0
							memcpy(canbuf, lockdata, 4);	// id
							memcpy(&canbuf[4], &lockdata[5], 8);
							YX_AddDataCollection(canbuf);	// 添加采集数据
						#endif
						CAN_TxData(lockdata, false, LOCK_CAN_CH);
					}
					//KMS_Hand_Send_Set(FALSE);
				}
            }
            s_kmslockstep = KMS_RESP_SEED;
            break;
        case KMS_SEND_KEY:
            if (data == NULL) {
                return;
            }
            switch (s_sclockpara.limitfunction) {
                case LC_CMD_BIND:
                    KMS_ActiveEncrypt(data, s_sclockpara.firmcode, s_sclockpara.serialnumber, key);
                    senddata[5] = 0x01;
                    memcpy(&senddata[6],key, 7);
					#if LOCK_COLLECTION > 0
						memcpy(canbuf, senddata, 4);	// id
						memcpy(&canbuf[4], &senddata[5], 8);
						YX_AddDataCollection(canbuf);	// 添加采集数据
					#endif
                    CAN_TxData(senddata, false, LOCK_CAN_CH);
                    {
                        INT32U  delaycnt;
                        
                        delaycnt = 0xA000 * 2;                                          /* 约150ms */////////////////////////要去掉
                        while (delaycnt--) {
                        }

                    }

                    senddata[5] = 0x02;
                    memcpy(&senddata[6],&key[7], 7);
					#if LOCK_COLLECTION > 0
						memcpy(canbuf, senddata, 4);	// id
						memcpy(&canbuf[4], &senddata[5], 8);
						YX_AddDataCollection(canbuf);	// 添加采集数据
					#endif
                    CAN_TxData(senddata, false, LOCK_CAN_CH);
                    {
                        INT32U  delaycnt;
                        
                        delaycnt = 0xA000 * 2;                                          /* 约150ms */////////////////////////要去掉
                        while (delaycnt--) {
                        }

                    }

                    senddata[5] = 0x03;
                    memcpy(&senddata[6],&key[14], 2);
					memset(&senddata[8], 0, 5);
					#if LOCK_COLLECTION > 0
						memcpy(canbuf, senddata, 4);	// id
						memcpy(&canbuf[4], &senddata[5], 8);
						YX_AddDataCollection(canbuf);	// 添加采集数据
					#endif
                    CAN_TxData(senddata, false, LOCK_CAN_CH);
                    break;
                case LC_CMD_UNLOCK:
                    KMS_HandshakeEncrypt(data, 0, s_sclockpara.serialnumber, seppd, &senddata[5]);
					#if DEBUG_LOCK > 0
					debug_printf("LC_CMD_UNLOCK speed:%04x\r\n",seppd);
					#endif 
					#if LOCK_COLLECTION > 0
						memcpy(canbuf, senddata, 4);	// id
						memcpy(&canbuf[4], &senddata[5], 8);
						YX_AddDataCollection(canbuf);	// 添加采集数据
					#endif
					CAN_TxData(senddata, false, LOCK_CAN_CH);
                    break;
                case LC_CMD_LOCK:
					#if DEBUG_LOCK > 0
					debug_printf("LC_CMD_LOCK speed:%04x\r\n",seppd);
					#endif 
                    KMS_HandshakeEncrypt(data, 0, s_sclockpara.serialnumber, seppd, &senddata[5]);
					#if LOCK_COLLECTION > 0
						memcpy(canbuf, senddata, 4);	// id
						memcpy(&canbuf[4], &senddata[5], 8);
						YX_AddDataCollection(canbuf);	// 添加采集数据
					#endif
                    CAN_TxData(senddata, false, LOCK_CAN_CH);
                    break;
                case LC_CMD_UNBIND:
					#if DEBUG_LOCK > 0
					debug_printf("LC_CMD_UNBIND speed:%04x\r\n",seppd);
					#endif
                    KMS_HandshakeEncrypt(data, 1, s_sclockpara.serialnumber, seppd, &senddata[5]);
					#if LOCK_COLLECTION > 0
						memcpy(canbuf, senddata, 4);	// id
						memcpy(&canbuf[4], &senddata[5], 8);
						YX_AddDataCollection(canbuf);	// 添加采集数据
					#endif
                    CAN_TxData(senddata, false, LOCK_CAN_CH);
                    break;
                default:
                    break;
            }

            #if DEBUG_LOCK > 0
                Debug_SysPrint("校验码key发送  ");
                Debug_PrintHex(TRUE, &senddata[5], 8);
            #endif

            s_kmslockstep = KMS_OVER;
            break;
        default:
            break;
    }
}


/**************************************************************************************************
**  函数名称:  ACCON_HandShake
**  功能描述:  ACC上电握手处理
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
void ACCON_HandShake(void)
{
    s_parasendstat = TRUE;
    s_handskenable = TRUE;
    s_idfiltenable = FALSE;
    s_idfiltcnt    = 0;
    s_parasendcnt  = 0;
	s_yc_state	   = 0;
    s_igoncnt      = 0;
	s_wc_0100cnt   = 0;
	s_wc_0100recv  = 0;
	s_wc_0800cnt   = 0;
	s_wc_0800recv  = 0;
    s_igoncnt      = 0;
    memset(&s_ycseed, 0, sizeof(s_ycseed));

    OS_StartTmr(s_lock_tmr, MILTICK, 1);
	#if DEBUG_LOCK > 0
		debug_printf("%s s_sclockpara.ecutype:%d limitfunction:%d\r\n",__FUNCTION__,s_sclockpara.ecutype,s_sclockpara.limitfunction);
	 #endif
    if (s_sclockpara.ecutype == ECU_YUCHAI){
        if (!s_sclockpara.unbindstat) {
            f_handsk = FALSE;
            s_sclockstep = CONFIG_REQ;
            #if DEBUG_LOCK> 0
            debug_printf("ACCON_HandShake s_sclockstep set CONFIG_REQ\r\n",__FUNCTION__);
            #endif
            YC_HandShake();
            #if DEBUG_LOCK > 0
                debug_printf("App_CAN_Init YC_HandShake\r\n");
            #endif
        }
    } else if ((s_sclockpara.ecutype == ECU_XICHAI_EMSMDI) || (s_sclockpara.ecutype == ECU_XICHAI_EMSVI)) {
        if (!s_sclockpara.unbindstat) {
            f_handsk = FALSE;
            s_sclockstep = RAND_CODE_REC;
            #if DEBUG_LOCK> 0
            debug_printf("%s s_sclockstep 请求随机数\r\n");
            #endif
            XC_HandShake();
        }
    }

	s_kms_delay_cnt = 0;

	
	f_handsk = FALSE;
	s_ishandover = FALSE;
	s_handshake_ack = HANDSHAKE_UNKNOWN;
	
	#if LOCK_COLLECTION > 0
	YX_StartDataCollection(0x00);
	#endif
}

/**************************************************************************************************
**  函数名称:  ACCOFF_HandShake
**  功能描述:  ACC上电握手处理
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
void ACCOFF_HandShake(void)
{
	if (s_sclockpara.ecutype == ECU_YUCHAI){
		#if DEBUG_LOCK > 0
		debug_printf("stop\r\n");
		#endif
		s_idfiltenable = FALSE;
		StopCANMsg_Period(0x18ea0021, LOCK_CAN_CH);
		StopCANMsg_Period(0x18fe02fb, LOCK_CAN_CH);
	}
   
	s_sclockstep = CONFIG_OVER;
}

#if EN_KMS_LOCK > 0
/**************************************************************************************************
**  函数名称:  Lock_KmsG5Cmd
**  功能描述:  柳汽康明斯国5锁车指令
**  输入参数:  无
**  输出参数:  无
**  返回参数:  无
**************************************************************************************************/
void Lock_KmsG5Cmd(INT8U* data, INT16U len) 
{
    BOOLEAN is_change;
    INT8U ack[10];
    /* client code */
    ack[0] = 0x09;
    ack[1] = 0x18;

    /* 指令 */
    ack[2] = 0x2e;

    /* 流水号 */
    ack[3] = data[0];

    ack[4] = 0x00;

    is_change = FALSE;

    #if DEBUG_LOCK > 1
    debug_printf("<Lock_KmsG5Cmd,cmd:0x%x, speed[0]:0x%x,speed[1]:0x%x>\r\n",s_kms_obj.kms_cmd, s_kms_obj.rota_speed[0], s_kms_obj.rota_speed[1]);
    printf_hex(data, len);
    #endif
    
    if ((data[1] >= KMS_CMD_ACTIVE) && (data[1] <= KMS_CMD_CLOSE)) {

        if (data[1] == KMS_CMD_LOCK) {
            if ((data[2] != s_kms_obj.rota_speed[0]) || (data[3] != s_kms_obj.rota_speed[1])) {
                s_kms_obj.rota_speed[0] = data[2];
                s_kms_obj.rota_speed[1] = data[3];
                is_change = TRUE;
            }
        }
        
        if (data[1] != s_kms_obj.kms_cmd) {
            s_kms_obj.kms_cmd = data[1];
            is_change = TRUE;
        } 
    } else {
        ack[4] = 0x01;  /* 失败 */
    }

    if (is_change) {
        is_change = FALSE;
        bal_pp_StoreParaByID(KMS_LOCK_PARA_,(INT8U *)&s_kms_obj, sizeof(KMS_LOCK_OBJ_T));
    }
    
    YX_COM_DirSend( CLIENT_FUNCTION_DOWN_REQ_ACK, ack, 5);
}
#endif

/**************************************************************************************************
**  函数名称:  Lock_Init
**  功能描述:  锁车初始化
**  输入参数:  无
**  输出参数:  无
**  返回参数:  无
**************************************************************************************************/
void Lock_Init(void)
{
    INT32U id;
    bal_pp_ReadParaByID(LOCKRECORD_,(INT8U *)&s_lockrecord, sizeof(LOCK_RECORD));
    bal_pp_ReadParaByID(SCLOCKPARA_,(INT8U *)&s_sclockpara, sizeof(SCLOCKPARA_T));
    bal_pp_ReadParaByID(XCLOCKPARA_,(INT8U *)&s_xclockpara, sizeof(XCLOCKPARA_T));
    #if EN_KMS_LOCK > 0
    bal_pp_ReadParaByID(KMS_LOCK_PARA_,(INT8U *)&s_kms_obj, sizeof(KMS_LOCK_OBJ_T));
    #endif
    CanLocateFiltidSet();
    
    s_lock_tmr = OS_InstallTmr(TSK_ID_OPT, 0, LockTmrProc);
    SecurityKeySet(s_sclockpara.securityKey, s_sclockpara.securityKeylen);
    ACCON_HandShake();

    s_lockmsg_buf = (REUPSAFE_DATA_T*)YX_MemMalloc(sizeof(REUPSAFE_DATA_T));
    #if LOCK_COLLECTION > 0
    //memset(&g_d008data, 0, sizeof(D008_DATA_T));
    s_d008data = (D008_DATA_T*)YX_MemMalloc(sizeof(D008_DATA_T));
    if (s_d008data != NULL) {
        memset(s_d008data, 0, sizeof(D008_DATA_T));
        s_d008data->lockcmdtime = 1100;
    }
	#endif
}
/**************************************************************************************************
**  函数名称:  CanBaudSet
**  功能描述:  can波特率设置
**  输入参数:  cahnnel:通道号
**  输出参数:  无
**  返回参数:  无
**************************************************************************************************/
void CanBaudSet(INT16U baud,INT8U channel)
{
    if(s_sclockpara.canbaud[channel] != baud) {
	      s_sclockpara.canbaud[channel] = baud;
	      bal_pp_StoreParaByID(SCLOCKPARA_, (INT8U *)&s_sclockpara, sizeof(SCLOCKPARA_T));

    }
}

