/********************************************************************************
**
** 文件名:     yx_hearttable.c
** 版权所有:   (c) 2005-2012 厦门雅迅网络股份有限公司
** 文件描述:   该模块实现各心跳周期发送、延时等待、超时重传等心跳管理功能
** 创建人:     赖荣东, 2011年02月01日
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2011/02/01 | 赖荣东 |  创建第一版本
*********************************************************************************/
#include "yx_includes.h"
#include "yx_tools.h"
#include "yx_structs.h"
#include "yx_timer.h"
#include "dal_systime.h"
#include "yx_message.h"

/*
********************************************************************************
*                   DEFINE CONFIG PARA
********************************************************************************
*/
#define MAX_LINK                        5
#define PBHEART                         5
#define SEND_PERIOD                     SECOND, PBHEART, LOW_PRECISION_TIMER


/*
********************************************************************************
*                   DEFINE MODULE STRUCT
********************************************************************************
*/
typedef enum {
    IDLE_ = 0,
    PSEND_,
    WAITA_,
    WAITOVERTIME_
} HEART_STATUS_E;

typedef struct {
    INT8U   status;
    BOOLEAN ready;
    INT8U   mul;        
    INT16U  tcnt;
    INT16U  ncnt;    
    INT16U  period;
    INT16U  waittime;
    INT16U  sendnum;
    BOOLEAN (*chksend)(void);
    BOOLEAN (*sendheart)(INT8U *mul);
    void (*informer)(void);
} HCB_T;

/*
********************************************************************************
*                   DEFINE MODULE VARIANT
********************************************************************************
*/
static HCB_T    s_hcb[MAX_LINK];
static INT8U    s_hearttmr;
static BOOLEAN  s_tmrrun;

/*******************************************************************
*   函数名:    HeartTmrProc
*   功能描述:  心跳扫描函数入口
*　 参数:      [in]  pdata
*   返回:      无
********************************************************************/
static void HeartTmrProc(void *pdata)
{
    INT8U  	mul, i, j;
    BOOLEAN (*chksend)(void);
    BOOLEAN (*sendheart)(INT8U *mul);
    void (*informer)(void);
    
    pdata = pdata;
    
    YX_StartTmr(s_hearttmr, SEND_PERIOD);
    for (i = 0; i < MAX_LINK; i++) {   
        if (s_hcb[i].status == IDLE_ || s_hcb[i].status == WAITOVERTIME_) {
            continue;                    
        } else if (s_hcb[i].status == PSEND_) {
            s_hcb[i].tcnt += PBHEART;
            if (s_hcb[i].tcnt >= s_hcb[i].period && s_hcb[i].period != 0) {
                s_hcb[i].ready = true;     
             }          
        } else if (s_hcb[i].status == WAITA_) {     
            s_hcb[i].tcnt += PBHEART;
            if (s_hcb[i].tcnt >= s_hcb[i].waittime * s_hcb[i].mul && s_hcb[i].waittime != 0) {
                if (s_hcb[i].ncnt >= s_hcb[i].sendnum) {
                    s_hcb[i].status= WAITOVERTIME_;
                    s_hcb[i].ready = false;
                    informer = s_hcb[i].informer;
                    if (NULL != informer) {
                        informer();   
                    }
                } else {
                    if (!s_hcb[i].ready) {
                        s_hcb[i].ready = true;
                        s_hcb[i].ncnt++;
                    }
                }
             }
         } else {
             ASSERT(0, ERR_HEART_MEM);
         }         
    }
    
    j = 0;
    for (i = 0; i < MAX_LINK; i++) {   
        if (s_hcb[i].status != IDLE_) {        
            if (s_hcb[i].ready) {
                chksend = s_hcb[i].chksend;
                sendheart = s_hcb[i].sendheart;
                if (chksend()) {
                    if (sendheart(&mul)) {                                          /* 对于短信心跳, 考虑到短信延时，需要比GPRS增加等待延时倍数 */
                        s_hcb[i].status = WAITA_;
                        s_hcb[i].ready  = false;
                        s_hcb[i].tcnt   = 0;
                        s_hcb[i].mul    = mul;
                        continue;
                    } else {                   
                        YX_StartTmr(s_hearttmr, SECOND, 1, LOW_PRECISION_TIMER);          
                    }
                }
            }
        } else {
            j++;                
        }
    }

    if (j == MAX_LINK) {
        s_tmrrun = false;
        YX_StopTmr(s_hearttmr);    
    }
}

/*******************************************************************
*   函数名:    ResetHcb
*   功能描述:  初始化心跳表结构
*　 参数:      无
*   返回:      无
********************************************************************/
static void ResetHcb(void)
{
	INT8U i;
	
    YX_MEMSET(&s_hcb, 0, sizeof(s_hcb));
    
    for (i = 0; i < MAX_LINK; i++) {
    	s_hcb[i].mul = 1;
   	}
}

/*******************************************************************
** 函数名:     DiagnoseProc
** 函数描述:   诊断函数
** 参数:       无
** 返回:       无
********************************************************************/
static void DiagnoseProc(void)
{
    if (s_tmrrun) {
        ASSERT(YX_TmrIsRun(s_hearttmr), ERR_HEART_TABLE);        
    } else {
        ASSERT(!YX_TmrIsRun(s_hearttmr), ERR_HEART_TABLE);    
    }
}

/*******************************************************************
*   函数名:    YX_InitHeartTable
*   功能描述:  初始化心跳表
*　 参数:      无
*   返回:      无
********************************************************************/
void YX_InitHeartTable(void)
{
#if DEBUG_GSMSTATUS > 0
    printf_com("<YX_InitHeartTable>\r\n");
#endif 
    ResetHcb();
    s_tmrrun   = false;
    s_hearttmr = YX_InstallTmr(PRIO_OPTTASK, 0, HeartTmrProc);      
    OS_InstallDiag(DiagnoseProc);
}

/*******************************************************************
*   函数名:    YX_InstallHeart
*   功能描述:  安装心跳
*　 参数:      [in]  chksend:           检查心跳是非允许发送
               [in]  sendheart:         心跳发送函数
               [in]  informer:          发送超时无应答通知函数
*   返回:      心跳表id,若为0xff则表示安装失败
********************************************************************/
INT8U YX_InstallHeart(BOOLEAN (*chksend)(void), BOOLEAN (*sendheart)(INT8U *mul), void (*informer)(void))
{
	INT8U id;
	
    for (id = 0; id < MAX_LINK; id++) {
        if (NULL == s_hcb[id].sendheart) {
            break;    
        }
    }   
#if DEBUG_GSMSTATUS > 0
    //printf_com("<YX_InstallHeart,id:%d>\r\n", id);
#endif    
    if (id < MAX_LINK) {
        s_hcb[id].status  = IDLE_;
        s_hcb[id].ready   = false;          
        s_hcb[id].chksend  = chksend;
        s_hcb[id].sendheart= sendheart;
        s_hcb[id].informer = informer;
        return id;
    } else {
        return 0xff;    
    }
}

/*******************************************************************
*   函数名:    YX_StartHeart
*   功能描述:  启动心跳
*　 参数:      [in]  id:                心跳表id
               [in]  period:            心跳正常发送周期
               [in]  waittime:          心跳等待应答时间
               [in]  sendnum:           心跳异常时总共发送次数
               [in]  inst:              心跳立即发送标志
*   返回:      无
********************************************************************/
void YX_StartHeart(INT8U id, INT16U period, INT16U waittime, INT16U sendnum, BOOLEAN inst)
{    
    ASSERT(id < MAX_LINK, ERR_HEART_MEM);
    ASSERT(NULL != s_hcb[id].chksend, ERR_HEART_MEM);   
    ASSERT(NULL != s_hcb[id].sendheart, ERR_HEART_MEM);

#if DEBUG_GSMSTATUS > 0
    printf_com("<YX_StartHeartid:%d, period:%d, wtime:%d, snum(2B):%d, inst:%d>\r\n", id, period, waittime, sendnum, inst);
#endif

    s_hcb[id].period    = period;
    s_hcb[id].waittime  = waittime;
    s_hcb[id].sendnum   = sendnum;
    
    s_hcb[id].tcnt      = 0;
    s_hcb[id].ncnt      = 0;  
    if (inst) {
        s_hcb[id].status= PSEND_;
        s_hcb[id].ready = true;            
    } else {
        s_hcb[id].status= IDLE_;        
        s_hcb[id].ready = false;        
    }
    if (YX_TmrIsRun(s_hearttmr) != TRUE) {
        YX_StartTmr(s_hearttmr, SECOND, 1, LOW_PRECISION_TIMER);    
    }
    s_tmrrun = true;
}

/*******************************************************************
*   函数名:    YX_InformRecvHeartAck
*   功能描述:  通知接收到心跳应答
*　 参数:      [in]  id:                心跳表id
*   返回:      无
********************************************************************/
void YX_InformRecvHeartAck(INT8U id)
{    
    ASSERT(id < MAX_LINK, ERR_HEART_MEM);

#if DEBUG_GSMSTATUS > 0
    printf_com("<receive heartid(%d) ack>\r\n", id);
#endif    
    if (s_hcb[id].status == WAITA_) {
        s_hcb[id].status = PSEND_;
        s_hcb[id].ready  = false;
        s_hcb[id].tcnt   = 0;
        s_hcb[id].ncnt   = 0;
    }
    if (YX_TmrIsRun(s_hearttmr) != TRUE) {
        YX_StartTmr(s_hearttmr, SECOND, 1, LOW_PRECISION_TIMER);    
        s_tmrrun = true;
    }
}

/*******************************************************************
*   函数名:    YX_InformStopHeart
*   功能描述:  通知停止某心跳
*　 参数:      [in]  id:                心跳表id
*   返回:      无
********************************************************************/
void YX_InformStopHeart(INT8U id)
{    
    ASSERT(id < MAX_LINK, ERR_HEART_MEM);
    
    if (s_hcb[id].status != IDLE_) {
        s_hcb[id].status = IDLE_;
        s_hcb[id].ready  = false;
        s_hcb[id].tcnt   = 0;
        s_hcb[id].ncnt   = 0;
    }
}

/*******************************************************************
** 函数名:      YX_HeartIsRun
** 函数描述:    心跳定时器是否在运行
** 参数:        [in]  id:               心跳定时器id
** 返回:        TRUE/FALSE
********************************************************************/
BOOLEAN YX_HeartIsRun(INT8U id)
{
    ASSERT(id < MAX_LINK, ERR_HEART_MEM);
    if (!YX_TmrIsRun(s_hearttmr)) {
        return FALSE;
    }    
    if (s_hcb[id].status == IDLE_) {
        return FALSE;
    }
    return TRUE;
}

