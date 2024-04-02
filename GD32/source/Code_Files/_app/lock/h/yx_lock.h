/**************************************************************************************************
**                                                                                               **
**  文件名称:  YX_LOCK.h                                                                         **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2011                                    **
**  创建信息:  阙存先 -- 2017年7月31日                                                       **
**  文件描述:  CAN应用层                                                                         **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/
#ifndef __YX_LOCK_H
#define __YX_LOCK_H
#include "yx_includes.h"
#include "yx_can_man.h"


#define  YC_HANDSK_PERIOD  800              /* 玉柴握手校验阶段请求报文的逻辑0x18EA0021发送周期(ms) */
#define	 LOCK_COLLECTION   0				// 锁车报文采集功能


/*************************************************************************************************/
/*                           ECU类型枚举                                                 */
/*************************************************************************************************/
typedef enum {
    ECU_WEICHAI,                    /* 潍柴 */
	ECU_KMS_Q6,						/*康明斯国六*/
	ECU_YUCHAI,						/*玉柴国六*/
    ECU_DACHAI,                     /* 大柴 */
    ECU_XICHAI,                  	/* 锡柴 */
    ECU_YUNNEI,                     /* 云内 */
    ECU_XICHAI_EMSVI,				/* 锡柴EMSVI_需加密 */
    ECU_XICHAI_EMSMDI,				/* 锡柴EMSMDI */
    ECU_XICHAI_EMSECO,				/* 锡柴EMSEcontrol */
    MAX_ECU_TYPE
} ECU_TYPE_E;



/*************************************************************************************************/
/*                           握手流程阶段                                                 */
/*************************************************************************************************/
typedef enum {
    CONFIG_REQ   = 0x00,        // 发送控制命令
    RAND_CODE_REC      ,        // 获取随机码
    CHECK_CODE_SEND    ,        // 发送校验码
    HAND_SEND1,					// 发送握手加密报文第一包
    HAND_SEND2,					// 发送握手加密报文第二包
    HAND_SEND3,					// 发送握手加密报文第三包
    CONFIG_CONFIRM_REC ,        // 获取控制验证信息
    CONFIG_OVER        ,        // 控制结束
} SC_LOCK_STEP_E;

/*************************************************************************************************/
/*                           康明斯握手流程阶段                                                    */
/*************************************************************************************************/
typedef enum {
    KMS_REQ_SEED         = 0x00,        // 发送请求SEED命令
    KMS_RESP_SEED        = 0x01,        // 获取随机码
    KMS_SEND_KEY         = 0x02,        // 发送校验码
    KMS_ECU_REC          = 0x03,        // 获取控制验证信息
    KMS_OVER             = 0x04,        // 控制结束
} KMS_LOCK_STEP_E;


/* 终端上报平台握手结果 */
typedef enum {
    HANDSHAKE_OK,                   /*  0-ECU握手成功 */
    HANDSHAKE_CHECKERR,				/*  1-握手校验失败 */
    HANDSHAKE_ERR,                  /*  2-ECU握手失败 10s内未收到ECU握手校验报文 */
    HANDSHAKE_OVER,                 /*  3-ECU超时 */
    HANDSHAKE_UNKNOWN,              /*  4-ECU状态未知 */
    HANDSHAKE_NOACK,                /*  5-ECU未反馈握手结果 */
    HANDSHAKE_NOSEED,               /*  6-ECU未响应随机数请求 */
    HANDSHAKE_BUSEXCEPTION,			/*  7-总线异常 */
    MAX_STAT
} HANDSHAKE_ACK_E;



/* 终端锁车控制命令类型定义 */
typedef enum {
    LC_CMD_UNBIND,                  /* 解绑 */
    LC_CMD_BIND,                    /* 绑定 */
   	LC_CMD_UNLOCK,                  /* 解锁 */
    LC_CMD_LOCK,                    /* 锁车 */
    //LC_CMD_HKEY,                    /* 握手HKEY校验 */
    //LC_CMD_LSPEED1,                 /* 限速1 */
    //LC_CMD_LSPEED2,                 /* 限速2 */
    //LC_CMD_LTORQUE1,                /* 限扭1 */
    //LC_CMD_LTORQUE2,                /* 限扭2 */
    MAX_LC_CMD
} LC_CMD_E;

/**    康明斯锁车定义*/
#define KMS_CANID_BIND                 0x18FE08EE						/* 绑定/解绑指令 */
#define KMS_CANID_LOCK                 0x18FFCB4A                    	/* 锁车/解锁指令 */
#define KMS_CANID_ACKSTATUS            0x18FFCA00                    	/* ECU响应状态报文 */
/**    潍柴锁车定义*/
#define WC_CANID_BIND                  0x18FE08EE
#define WC_CANID_LOCK                  0x18FE0AEE
#define WC_CANID_ACKSTATUS             0x18FF0800
/**    玉柴锁车定义*/
#define YC_GROUPID                     0xFD01
#define YC_CANID_REQ                   0x18EA002						/* 请求指令 */
#define YC_CANID_BIND                  0x18FE01FB                      /* 绑定/解绑指令 */
#define YC_CANID_LOCK                  0x18FE03FB                      /* 锁车/解锁指令 */
#define YC_CANID_ACKSTATUS             0x18FD0100                     	/* ECU响应状态报文 */

#if LOCK_COLLECTION > 0
/* 平台D008监控数据 */
typedef struct{
	INT16U	restarttime;				//	重新点火后报文采集时间
	INT8U	restarten;					//  重新点火后报文采集发送标志
	INT8U	restarttotal;				//	重新点火后采集到报文总条数
	INT8U	restartdata[80][12];		//	重新点火后8s内锁车相关数据 ID+报文
	INT16U	lockcmdtime;				//	收到锁车指令后报文采集时间
	INT8U	lockcmden;					//	收到锁车指令后报文发送标志
	INT8U	lockcmdtotal;				//	收到锁车指令后采集到报文总条数
	INT8U	lockcmddata[20][12];		//	收到锁车指令后5s内锁车相关数据 ID+报文
}D008_DATA_T;
#endif


void    Lock_Init(void);
BOOLEAN LockParaBak(INT8U *userdata, INT8U userdatalen);
BOOLEAN GetFiltenableStat(void);
void    HandShakeMsgAnalyze(CAN_DATA_HANDLE_T *CAN_msg, INT16U datalen);
void    GetGpsID(INT8U *gpsid);
void    ACCON_HandShake(void);
void    ACCOFF_HandShake(void);
void    LockParaStore(INT8U *userdata, INT8U userdatalen);
void    Lock_RecordReqHdl(void);
void    BakParaSave(INT8U locken);
static void KMS_HandShake(INT8U *data, INT8U len);
void 	CanBaudSet(INT16U baud,INT8U channel);


/**************************************************************************************************
**  函数名称:  Lock_KmsG5Cmd
**  功能描述:  柳汽康明斯国5锁车指令
**  输入参数:  无
**  输出参数:  无
**  返回参数:  无
**************************************************************************************************/
void Lock_KmsG5Cmd(INT8U* data, INT16U len);

/**************************************************************************************************
**  函数名称:  KMS_Hand_Send_Set
**  功能描述:  康明斯握手发送使能
**  输入参数:  en:使能/失能
**  返回参数:  None
**************************************************************************************************/
void KMS_Hand_Send_Set(BOOLEAN en);

/**************************************************************************************************
**  函数名称:  SetSpeedFlag
**  功能描述:  设置转速报文标志
**  输入参数:  flag:标志
**  返回参数:  无
**************************************************************************************************/
void SetSpeedFlag(BOOLEAN flag);

/**************************************************************************************************
**  函数名称:  GetSpeedFlag
**  功能描述:  获取转速报文标志
**  输入参数:  flag:标志
**  返回参数:  无
**************************************************************************************************/
BOOLEAN GetSpeedFlag(void);

/**************************************************************************************************
**  函数名称:  YX_LOCKCMD_SetPara
**  功能描述:  设置锁车参数
**  输入参数:  canbuf:CAN数据
**  输出参数:  无
**  返回参数:  true:填充成功 false:填充失败
**************************************************************************************************/
void YX_LOCKCMD_SetPara(INT8U *buf);

/**************************************************************************************************
**  函数名称:  YX_StartDataCollection
**  功能描述:  开始数据采集
**  输入参数:  type:采集数据类型  0x00:前8s数据  0x01:指令数据
**  输出参数:  无
**  返回参数:  无
**************************************************************************************************/
void YX_StartDataCollection(INT8U type);

/**************************************************************************************************
**  函数名称:  YX_AddDataCollection
**  功能描述:  填充采集信息
**  输入参数:  canbuf:CAN数据
**  输出参数:  无
**  返回参数:  true:填充成功 false:填充失败
**************************************************************************************************/
BOOLEAN YX_AddDataCollection(INT8U *canbuf);

/**************************************************************************************************
**  函数名称:  YX_IsLOCKCMD
**  功能描述:  是否为锁车指令
**  输入参数:  id
**  输出参数:  无
**  返回参数:  true:是 false:否
**************************************************************************************************/
BOOLEAN YX_IsLOCKCMD(INT32U id);

/**************************************************************************************************
**  函数名称:  XC_SecretDataTran
**  功能描述:  锡柴数据加密转发
**  输入参数:  data:参数内容
			   datalen:数据长度
**  返回参数:  false:失败 true:成功
**************************************************************************************************/
BOOLEAN XC_SecretDataTran(INT8U* data, INT8U datalen);

/**************************************************************************************************
**  函数名称:  XC_ParaSet
**  功能描述:  锡柴参数同步
**  输入参数:  data:参数内容
			   datalen:数据长度
**  返回参数:  false:失败 true:成功
**************************************************************************************************/
BOOLEAN XC_ParaSet(INT8U* data, INT8U datalen);



#endif
/**************************** (C) COPYRIGHT 2011  XIAMEN YAXON.LTD **************END OF FILE******/

