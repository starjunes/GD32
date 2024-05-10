/*******************************************************************************
**
** 文件名:     yx_hit_com.c
** 版权所有:   (c) 1998-2017 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要负责碰撞模块的处理和协议对接
**
********************************************************************************
**                  修改历史记录
**==============================================================================
**| 日期              | 作者        |  修改记录
**==============================================================================
**| 2017/06/16 |  谢金成    |  创建第一版本
*******************************************************************************/
#include "yx_includes.h"
#include "yx_protocal_hdl.h"
#include "yx_com_send.h"
#include "yx_hit_man.h"
#include "port_gsensor.h"
#include "bal_pp_reg.h"
#include "bal_pp_drv.h"
#include "bal_pp_misc.h"
#include "port_gpio.h"
#include "hal_exrtc_sd2058_drv.h"
#if 0
#undef DEBUG_GSEN_STATUS
#define DEBUG_GSEN_STATUS 1
#endif

/******************************************************************************/
/*                           定义参数设置结构体                                                                */
/******************************************************************************/
typedef enum {
	G_X,
	G_Y,
	G_Z,
	G_ERR
} G_DIR_E;

typedef enum {
    GSEN_STATUS_NONE = 0,
    GSEN_STATUS_HIT,
    GSEN_STATUS_ROV
} GSEN_STATUS_E;

typedef enum {
	BD_IDLE = 0x00,
	BD_GET_CORRECTVALUE,  /* 状态:获取修订值 */
	BD_JUDGEISSUCCESS,    /* 状态：判断标定是否能成功 */
	BD_SUCCESSEND,        /* 标定成功状 */
	BD_SAVEPARA           /* 保存参数状态 */
} BD_STATUS_E;

typedef struct {
	INT16S gthreshold;    /* 加速度阈值 */
	INT8U anglethreshold; /* 侧翻角度阈值 */
	INT8U timethreshold;  /* 灵敏度时间阈值 */
	BOOLEAN isreport;     /* 主动上报开关 */
	INT8U reporttime;     /* 上报时间间隔 */
} SENSOR_PART_T;

static BD_STATUS_E s_bd_status;       /* 标定阶段 */
static BOOLEAN s_isbd=false;          /* 是否标定过 */
static BOOLEAN s_setparastu=false;          /* 是否标定过 */
static AXIS_DAT_T  s_axis_bddata, s_axis_rdata;
static SENSOR_PART_T s_paraseted;
static INT8U s_rovtmrcyc,s_rovtmrcnt; /*侧翻累计时间 */
static INT8U s_reporttmrid;           /* 定时上报定时 */
static INT8U s_rovtmrid;              /* 定时检测侧翻 */
static INT8U s_waitdalinittmr;
static INT8U s_curstatu;              /* 模块状态，00无报警，01碰撞，02侧翻，03两者都有 */
static INT8U s_prestatu;
static G_DIR_E s_gdir;

/*******************************************************************************
 **  函数名称:  max
 **  功能描述:  比较三个数大小
 **  输入参数:  三个整数
 **  输出参数:  无
 **  返回参数:  得到最大数
 ******************************************************************************/
static G_DIR_E max(INT32U a, INT32U b, INT32U c)
{
	if((a >= b) && (a >= c))return G_X;
	if((b >= a) && (b >= c))return G_Y;
	if((c >= a) && (c >= b))return G_Z;
	return G_ERR;
}

/*******************************************************************************
 **  函数名称:  BDGsenCalbakValue
 **  功能描述:  标定回调函数
 **  输入参数:  无
 **  输出参数:  标定值
 **  返回参数:  无
 ******************************************************************************/
static void BDGsenCalbakValue(AXIS_DAT_T  *axis_bddata ,AXIS_DAT_T *gyro_bddata)
{
	INT8U ack[2] = { 0 };

	ack[0] = 0x01;
	ack[1] = 0x01;
	s_axis_bddata.axis_x = axis_bddata->axis_x;
	s_axis_bddata.axis_y = axis_bddata->axis_y;
	s_axis_bddata.axis_z = axis_bddata->axis_z;
	s_gdir =max(abs(s_axis_bddata.axis_x), abs(s_axis_bddata.axis_y),
					abs(s_axis_bddata.axis_z));
	s_bd_status = BD_SUCCESSEND;
	#if DEBUG_GSEN_STATUS > 0
		debug_printf("标定成功\r\n");
	#endif
	YX_COM_DirSend( GSENSOR_BD_REQ_ACK , ack, 2);
}
/*******************************************************************************
 **  函数名称:  BD_Start
 **  功能描述:  开始标定
 **  输入参数:  无
 **  输出参数:  无
 **  返回参数:  无
 ******************************************************************************/
static void BD_Start(void)
{
	INT8U ack[2] = { 0 };
	ack[0] = 0x01;

	#if DEBUG_GSEN_STATUS > 0
		debug_printf("开始标定\r\n");
	#endif
	if (BD_IDLE == s_bd_status || BD_SUCCESSEND == s_bd_status) { /* 只有在空闲状态 或 标定成功需要重新标定 */

		#if DEBUG_GSEN_STATUS > 0
			debug_printf("标定中\r\n");
		#endif
		s_bd_status = BD_GET_CORRECTVALUE;
		s_isbd = false;
		PORT_GsenCalibStart(BDGsenCalbakValue);
	} else {
		#if DEBUG_GSEN_STATUS > 0
			debug_printf("标定失败\r\n");
		#endif
		ack[1] = 0x02;
		YX_COM_DirSend( GSENSOR_BD_REQ_ACK , ack, 2);
	}
}

/*******************************************************************************
 **  函数名称:  BD_Stop
 **  功能描述:  标定结束，保存参数
 **  输入参数:  无
 **  输出参数:  无
 **  返回参数:  无
 ******************************************************************************/
static void BD_Stop(void) {
	INT8U ack[2] = { 0 };
	AXISPARA_T  axis_readpara;

	if (BD_SUCCESSEND != s_bd_status) {
		s_bd_status = BD_IDLE;
		ack[0] = 0x02;
		ack[1] = 0x02;
		YX_COM_DirSend( GSENSOR_BD_REQ_ACK , ack, 2);
		//YX_COM_DirSend( GSENSOR_BDRESULT_REQ_ACK , ack, 2);
		return;
	}
	s_bd_status = BD_SAVEPARA;

	if (!bal_pp_ReadParaByID(AXISCALPARA_, (INT8U*)&axis_readpara, sizeof(AXISPARA_T))) {
		ack[0] = 0x02;
		ack[1] = 0x02;
		YX_COM_DirSend( GSENSOR_BD_REQ_ACK , ack, 2);
		s_isbd = false;
	    s_bd_status = BD_IDLE;
	    return;
	}

	axis_readpara.is_cali = true;
	axis_readpara.calvalue_x = s_axis_bddata.axis_x;
	axis_readpara.calvalue_y = s_axis_bddata.axis_y;
	axis_readpara.calvalue_z = s_axis_bddata.axis_z;
	axis_readpara.setvalue_g = 4000;

	if (bal_pp_StoreParaByID(AXISCALPARA_, (INT8U*)&axis_readpara, sizeof(AXISPARA_T)))  {
		ack[0] = 0x02;
		ack[1] = 0x01;
		YX_COM_DirSend( GSENSOR_BD_REQ_ACK , ack, 2);
	    s_isbd = true;
	} else {
		ack[0] = 0x02;
		ack[1] = 0x02;
		YX_COM_DirSend( GSENSOR_BD_REQ_ACK , ack, 2);
		s_isbd = false;
	}
	s_bd_status = BD_IDLE;
}
/*******************************************************************************
 **  函数名称:  CrashBDHdl
 **  功能描述:  车台标定请求
 **  输入参数:  无
 **  输出参数:  无
 **  返回参数:  无
 ******************************************************************************/
void CrashBDHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen)
{
	INT8U ack[2] = { 0 };

	ack[0] = data[0];
	switch (data[0]) {
		case 0x01: /* 开始标定 */
			BD_Start();
			break;
		case 0x02: /* 结束标定 */
			BD_Stop();
			break;
		default:
			ack[1] = 0x02;
			YX_COM_DirSend( GSENSOR_BD_REQ_ACK, ack, 2);
			break;
	}
}
#if 0
/*******************************************************************************
 **  函数名称:  CrashBDResult
 **  功能描述:  标定情况查询
 **  输入参数:  无
 **  输出参数:  无
 **  返回参数:  无
 ******************************************************************************/
void CrashBDResult(INT8U version, INT8U command,INT8U *data, INT16U datalen)
{
	INT8U ack;
    #if 0
    version     = version;
    command     = command;
    data        = data;
    datalen     = datalen;
    #endif
	if(s_isbd == FALSE) {
		ack = 0; //未标定
		YX_COM_DirSend( GSENSOR_BDRESULT_REQ_ACK, &ack, 1);
	} else {
		ack = 2;//已经标定
		YX_COM_DirSend( GSENSOR_BDRESULT_REQ_ACK, &ack, 1);
	}
}
#endif
/*******************************************************************************
 **  函数名称:  CrashSetParaHdl
 **  功能描述:  参数设置 加速度阈值、角度阈值、灵敏时间、自动上报开关、上报时间间隔
 **  输入参数:  None
 **  返回参数:  None
 ******************************************************************************/
void CrashSetParaHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen) {
	INT8U ack = 0;
	AXISPARA_T  axis_readpara;

	if (data[0] > 60 || s_isbd == FALSE) {
		#if DEBUG_GSEN_STATUS > 0
			debug_printf("碰撞参数有误 %d isbd:%d\r\n", data[0], s_isbd);
		#endif
		ack = 0x02;
		YX_COM_DirSend( CRASH_SET_PARA_ACK, &ack, 1);
		return;
	}

	if(!PORT_GsenSetHitPara(data[0]*1000, &s_axis_bddata)) {
		#if DEBUG_GSEN_STATUS > 0
			debug_printf("设置碰撞参数失败\r\n");
		#endif
		ack = 0x02;
		YX_COM_DirSend( CRASH_SET_PARA_ACK, &ack, 1);
		return;
	}

	if(bal_pp_ReadParaByID(AXISCALPARA_, (INT8U*)&axis_readpara, sizeof(AXISPARA_T))) {
	    if (axis_readpara.setvalue_g != data[0]*1000) {
        	axis_readpara.setvalue_g = data[0]*1000;
        	if (!bal_pp_StoreParaByID(AXISCALPARA_, (INT8U*)&axis_readpara, sizeof(AXISPARA_T))){
				#if DEBUG_GSEN_STATUS > 0
					debug_printf("存储碰撞参数失败\r\n");
				#endif
        		ack = 0x02;
        		YX_COM_DirSend( CRASH_SET_PARA_ACK, &ack, 1);
        		return;
        	}
	    }
	} else {
		#if DEBUG_GSEN_STATUS > 0
			debug_printf("读取碰撞参数失败\r\n");
		#endif
		ack = 0x02;
		YX_COM_DirSend( CRASH_SET_PARA_ACK, &ack, 1);
		return;
	}

	if (data[1] == 0) {
		s_paraseted.timethreshold = 1;       /* 默认采用10ms(灵敏度) */
	} else {
		s_paraseted.timethreshold = data[2]; /* 时间阈值(灵敏度) */
	}

	if (0 == data[2]) {                      /* 是否上报标志 */
		s_paraseted.isreport = FALSE;
		OS_StopTmr(s_reporttmrid);
	} else if (1 == data[2]) {
		s_paraseted.isreport = TRUE;
	} else {
		ack = 0x02;
		YX_COM_DirSend( CRASH_SET_PARA_ACK, &ack, 1);
		return;
	}

	if (data[3] == 0) {
		s_paraseted.reporttime = 1;           /* 上报时间间隔 */
	} else {
		s_paraseted.reporttime = data[3];     /* 上报时间间隔 */
	}

	if(data[4] > 90) {
		ack = 0x02;
		YX_COM_DirSend( CRASH_SET_PARA_ACK, &ack, 1);
		return;
	} else {
		s_paraseted.anglethreshold = data[4];
	}
	ack = 0x01;
	YX_COM_DirSend( CRASH_SET_PARA_ACK, &ack, 1);
	s_setparastu = true;
	#if DEBUG_GSEN_STATUS > 0
		debug_printf("碰撞参数设置成功\r\n");
	#endif
}

/*******************************************************************************
** 函数名:     CrashReportAckHdl
** 函数描述:    已收到应答，停止上报
** 参数:       [in]data:数据指针
**             [in]datalen:数据长度
** 返回:       无
*******************************************************************************/
void CrashReportAckHdl(INT8U mancode, INT8U command,INT8U *data, INT16U datalen)
{
//	s_curstatu = GSEN_STATUS_NONE;// 清掉当前状态，重新获取

	#if DEBUG_GSEN_STATUS > 0
		if(s_prestatu == 1) {
			debug_printf("碰撞应答\r\n");
		} else if(s_prestatu == 2) {
			debug_printf("侧翻应答\r\n");
		} else if(s_prestatu == 3) {
			debug_printf("碰撞并侧翻应答\r\n");
		} else{
			debug_printf("车辆正常应答\r\n");
		}
	#endif

	switch ( data[0] ) {
		case  GSEN_STATUS_HIT:
			s_curstatu &= ~GSEN_STATUS_HIT;
			break;
		case  GSEN_STATUS_ROV:
			s_curstatu &= ~GSEN_STATUS_ROV;
			break;
		case  GSEN_STATUS_HIT+GSEN_STATUS_ROV:
			s_curstatu &= ~(GSEN_STATUS_HIT+GSEN_STATUS_ROV);
			break;
		default :
		    OS_StopTmr(s_reporttmrid);    //已经收到正常应答，停止上报
			break;
	}
}

/*******************************************************************************
 ** 函数名:     ReportTmrProc
 ** 函数描述:   定时上报状态
 ** 参数:       [in]  timer:             定时器
 **             [in]  pdata:             定时器私有数据指针
 ** 返回:       无
 ******************************************************************************/
static void ReportTmrProc(void *pdata)
{
	if (true == s_paraseted.isreport){
		YX_COM_DirSend( CRASH_REPORT, &s_prestatu, 1);
		#if DEBUG_GSEN_STATUS > 0
			if(s_prestatu == 1) {
				debug_printf("碰撞定时上报\r\n");
			} else if(s_prestatu == 2) {
				debug_printf("侧翻定时上报\r\n");
			} else if(s_prestatu == 3) {
				debug_printf("碰撞并侧翻定时上报\r\n");
			} else{
				debug_printf("车辆正常定时上报\r\n");
			}
		#endif
	}
}

/*******************************************************************************
 ** 函数名:     RovTmrProc
 ** 函数描述:   侧翻情况定时检测和上报
 ** 参数:       [in]  timer:             定时器
 **             [in]  pdata:             定时器私有数据指针
 ** 返回:       无
 ******************************************************************************/
static void RovTmrProc(void *pdata)
{
	FP32 a, b, c;
	//debug_printf("s_isbd=%d s_setparastu=%d\r\n",s_isbd,s_setparastu);
    if((false == s_isbd) || (false == s_setparastu))return;

	PORT_ReadAcclData(&s_axis_rdata);
	//debug_printf("axis_x=%d,axis_y=%d,axis_z=%d\r\n",s_axis_rdata.axis_x,s_axis_rdata.axis_y,s_axis_rdata.axis_z);
	if(s_gdir == G_X) {
		a = (FP32) (s_axis_rdata.axis_z * s_axis_rdata.axis_z
						+ s_axis_rdata.axis_y * s_axis_rdata.axis_y)
						/ (s_axis_rdata.axis_x * s_axis_rdata.axis_x);
		b = pow(a, 0.5);
		c = atan(b);
		c *= 180 / 3.14159;
		if (c >= s_paraseted.anglethreshold) {
			s_rovtmrcnt++;
		} else {
		    if(s_axis_bddata.axis_x > 0) {
		    	if(s_axis_rdata.axis_x <0) {
		    		s_rovtmrcnt++;
		    	}
		    } else if (s_axis_bddata.axis_x < 0) {
		    	if(s_axis_rdata.axis_x > 0) {
		    		s_rovtmrcnt++;
		    	}
		    }
		}
	} else if(s_gdir == G_Y) {
		a = (FP32) (s_axis_rdata.axis_x * s_axis_rdata.axis_x
						+ s_axis_rdata.axis_z * s_axis_rdata.axis_z)
						/ (s_axis_rdata.axis_y * s_axis_rdata.axis_y);
		b = pow(a, 0.5);
		c = atan(b);
		c *= 180 / 3.14159;
		if (c >= s_paraseted.anglethreshold) {
			s_rovtmrcnt++;
		} else {
		    if(s_axis_bddata.axis_y > 0) {
		    	if(s_axis_rdata.axis_y <0) {
		    		s_rovtmrcnt++;
		    	}
		    } else if (s_axis_bddata.axis_y < 0) {
		    	if(s_axis_rdata.axis_y > 0) {
		    		s_rovtmrcnt++;
		    	}
		    }
		}
	} else if(s_gdir == G_Z) {
		a = (FP32) (s_axis_rdata.axis_x * s_axis_rdata.axis_x
						+ s_axis_rdata.axis_y * s_axis_rdata.axis_y)
						/ (s_axis_rdata.axis_z * s_axis_rdata.axis_z);
		b = pow(a, 0.5);
		c = atan(b);
		c *= 180 / 3.14159;
		if (c >= s_paraseted.anglethreshold) {
			s_rovtmrcnt++;
		} else {
		    if(s_axis_bddata.axis_z > 0) {
		    	if(s_axis_rdata.axis_z <0) {
		    		s_rovtmrcnt++;
		    	}
		    } else if (s_axis_bddata.axis_z < 0) {
		    	if(s_axis_rdata.axis_z > 0) {
		    		s_rovtmrcnt++;
		    	}
		    }
		}
	}
	#if DEBUG_CRASH_STATUS > 0
		//debug_printf("c=%d\r\n",(INT16U)c);
	#endif
	if(s_rovtmrcyc >= 30) {         //验证30次是否处理侧翻状态
		if(s_rovtmrcnt > 25) {      //如果大于25次则判断侧翻
			s_curstatu |= GSEN_STATUS_ROV;
		} else if (s_rovtmrcnt <3) {//如果只有一两次则判断正常
			s_curstatu &= ~GSEN_STATUS_ROV;
		}
		if(s_prestatu != s_curstatu) {
		    OS_StartTmr(s_reporttmrid, SECOND, s_paraseted.reporttime);
		}
        s_rovtmrcyc=0;
        s_rovtmrcnt=0;
	}
	s_prestatu = s_curstatu;    // 保存状态

	s_rovtmrcyc++;
}

/*******************************************************************************
 ** 函数名:     WaitDalgenInitProc
 ** 函数描述:   等待底层dal_gsensor 先初始化完，再上层初始化
 ** 参数:       [in]  timer:             定时器
 **             [in]  pdata:             定时器私有数据指针
 ** 返回:       无
 ******************************************************************************/
static void WaitDalgenInitProc(void *pdata)
{
	AXISPARA_T  axis_readpara;

	if(!PORT_GsenGetWorkState())return;

	OS_StopTmr(s_waitdalinittmr);

	s_paraseted.timethreshold = 1;
	s_paraseted.reporttime = 1;
	s_bd_status = BD_IDLE;
	s_curstatu = GSEN_STATUS_NONE;
	s_setparastu = false;

	if(bal_pp_ReadParaByID(AXISCALPARA_, (INT8U*)&axis_readpara, sizeof(AXISPARA_T))) {
		if(axis_readpara.is_cali == true) {
			s_axis_bddata.axis_x = axis_readpara.calvalue_x;
			s_axis_bddata.axis_y = axis_readpara.calvalue_y;
			s_axis_bddata.axis_z = axis_readpara.calvalue_z;

			if(PORT_GsenSetHitPara(axis_readpara.setvalue_g, &s_axis_bddata)) {
				s_gdir =max(abs(s_axis_bddata.axis_x), abs(s_axis_bddata.axis_y),
								abs(s_axis_bddata.axis_z));
				s_isbd = true;
			} else {
				#if DEBUG_GSEN_STATUS > 0
					debug_printf("参数写入失败\r\n");
				#endif
				s_isbd = false;
			}
		} else {
			#if DEBUG_GSEN_STATUS > 0
				debug_printf("参数是未标定的\r\n");
			#endif
			s_isbd = false;
		}
	}

	s_reporttmrid = OS_InstallTmr(TSK_ID_OPT, 0, ReportTmrProc);
//    OS_StartTmr(s_reporttmrid, SECOND, s_paraseted.reporttime);

	s_rovtmrid = OS_InstallTmr(TSK_ID_OPT, 0, RovTmrProc);
	OS_StartTmr(s_rovtmrid, MILTICK , s_paraseted.timethreshold);
}


static void sd_2058rtc(void * pdata)
{
	INT8U time_data[7],temp,  alarmflag,ctr1, ctr2;;
	INT8U data[7] = {15,15,17,2,7,9,21};
	static INT8U i = 0;
	if(i == 0){
		i = 1;
		HAL_sd2058_Open();
 		HAL_sd2058_SetCalendar(data);
		HAL_sd2058_writebyte(0x14,0x15);
		data[0] = 20;
		HAL_sd2058_SetAlarm(0x01, data);
	}
	HAL_sd2058_ReadCalendar(time_data);
	HAL_sd2058_readbyte(0x14, &temp);
	HAL_sd2058_readbyte(0x0E, &alarmflag);
	HAL_sd2058_readbyte(0x0F, &ctr1);
	HAL_sd2058_readbyte(0x10, &ctr2);
	#if DEBUG_GSEN_STATUS > 0
	debug_printf("temp=%x alarmflag:%02x ctr1:%02x ctr2:%02x time:%d-%d-%d %d:%d:%d  %d\r\n",temp, alarmflag, ctr1, ctr2, time_data[6],time_data[5],\
					time_data[4],time_data[2],time_data[1],time_data[0],time_data[3]);
	#endif
}
/*******************************************************************************
 **  函数名称:  YX_Hit_Init
 **  功能描述:  碰撞模块初始化
 **  输入参数:  无
 **  输出参数:  无
 **  返回参数:  无
 ******************************************************************************/
void YX_Hit_Init(void)
{
	PORT_Gsensor_Init();

	//s_waitdalinittmr = OS_InstallTmr(TSK_ID_OPT, 0, WaitDalgenInitProc);
	//OS_StartTmr(s_waitdalinittmr, SECOND, 1);
	//s_sd2058tmr = OS_InstallTmr(TSK_ID_OPT, 0, sd_2058rtc);
	//OS_StartTmr(s_sd2058tmr,SECOND, 1);
}

/*******************************************************************************
 **  函数名称:  YX_Hit_GetGsenStatus
 **  功能描述:  获取gsensor当前状态
 **  输入参数:  无
 **  输出参数:  无
 **  返回参数:  返回状态
 ******************************************************************************/
INT8U YX_Hit_GetGsenStatus(void)
{
	return s_curstatu;
}

/*******************************************************************************
 **  函数名称:  YX_InformHitAlm
 **  功能描述:  碰撞发生后 底层通过消息传递返回来的处理函数
 **  输入参数:  无
 **  输出参数:  无
 **  返回参数:  无
 ******************************************************************************/
void YX_InformHitAlm(void)
{
	s_curstatu |= GSEN_EVT_HIT;
	#if DEBUG_GSEN_STATUS > 0
		debug_printf("碰撞中断:%d\r\n", s_curstatu);
	#endif
	if ((true == s_paraseted.isreport) && (true == s_isbd)) {
		YX_COM_DirSend( CRASH_REPORT, &s_curstatu, 1);
		OS_StartTmr(s_reporttmrid, SECOND, s_paraseted.reporttime);
	}
}
