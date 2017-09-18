/********************************************************************************
**
** 文件名:     dal_hit_drv.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现碰撞侧翻标定、检测功能
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
#include "hal_hit_drv.h"
#include "dal_hit_drv.h"
#include "dal_pp_drv.h"
#include "yx_debug.h"


#if EN_GSENSOR > 0

/*
********************************************************************************
* 定义配置参数
********************************************************************************
*/
#define PERIOD_SAMPLE        _TICK, 2

#define MAX_ERROR            5

#define SUB(a, b)            ((a) > (b) ? ((a) - (b)) : ((b) - (a)))
#define ABS(a)               ((a) >= 0 ? (a) : (0 - (a)))

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    INT8U status;
    INT8U ct_error;                    /* 重力传感器无效次数累计 */
    INT8U ct_rollover;                 /* 侧翻计数器 */
    INT8U ct_collision;                /* 碰撞计数器 */
    INT8U onoff;                       /* 开关 */
    INT8U acceleration;                /* 碰撞加速度阈值,单位:0.1g */
    INT8U angle;                       /* 侧翻角度阈值,单位:度 */
    GSENSOR_XYZ_T xyz;
    GSENSOR_CALIBRATION_T calibration; /* 标定值 */
    void (*handler)(INT8U event);
} DCB_T;

/*
********************************************************************************
* 定义模块变量
********************************************************************************
*/
static INT8U s_sampletmr;
static DCB_T s_dcb;



/*******************************************************************
** 函数名:     SampleTmrProc
** 函数描述:   采样周期定时器
** 参数:       无
** 返回:       无
********************************************************************/
static void SampleTmrProc(void *pdata)
{
    INT8U collision, rollover;
    GSENSOR_XYZ_T xyz;
    
    OS_StartTmr(s_sampletmr, PERIOD_SAMPLE);
    
    collision = false;
    rollover = false;
    if (HAL_HIT_GetValueXYZ(&xyz)) {
        if ((s_dcb.status & GSENSOR_EVENT_VALID) == 0) {                       /* 判断重力加速度传感器是否正常 */
            #if DEBUG_GSENSOR > 0
            printf_com("<gsensor is valid>\r\n");
            #endif
            
            s_dcb.status |= GSENSOR_EVENT_VALID;
            if (s_dcb.handler != 0) {
                s_dcb.handler(s_dcb.status);
            }
        }
        
        if (!s_dcb.onoff) {
            return;
        }
        
        if (s_dcb.xyz.x != 0 || s_dcb.xyz.y != 0 || s_dcb.xyz.z != 0) {
            /* 判断碰撞 */
            if (SUB(xyz.x, s_dcb.xyz.x) > (s_dcb.acceleration * 100)) {        /* X轴方向 */
                collision = true;
            }
            
            if (SUB(xyz.y, s_dcb.xyz.y) > (s_dcb.acceleration * 100)) {        /* Y轴方向 */
                collision = true;
            }
            
            //if (SUB(xyz.z, s_dcb.xyz.z) > (s_dcb.acceleration * 100)) {        /* Z轴方向 */
            //    collision = true;
            //}
            
            /* 判断侧翻 */
            if (s_dcb.calibration.isvalid) {
                if (SUB(xyz.x, s_dcb.calibration.x) > ((s_dcb.angle * 1000) / 90)) {/* x轴方向重力加速度偏离正常值太多 */
                    rollover = true;
                }
            
                if (SUB(xyz.y, s_dcb.calibration.y) > ((s_dcb.angle * 1000) / 90)) {/* y轴方向重力加速度偏离正常值太多 */
                    rollover = true;
                }
            
                if (SUB(xyz.z, s_dcb.calibration.z) > ((s_dcb.angle * 1000) / 90)) {/* z轴方向重力加速度偏离正常值太多 */
                    rollover = true;
                }
            } else {
                if (ABS(xyz.x) > ((s_dcb.angle * 1000) / 90)) {                    /* x轴方向重力加速度偏离正常值太多 */
                    rollover = true;
                }
            
                if (ABS(xyz.y) > ((s_dcb.angle * 1000) / 90)) {                    /* y轴方向重力加速度偏离正常值太多 */
                    rollover = true;
                }
            
                //if (SUB(ABS(xyz.z), 1000) > ((s_dcb.angle * 1000) / 90) || xyz.z > 0) {/* z轴方向重力加速度偏离正常值太多 */
                //    rollover = true;
                //}
            }
        }
    
        /* 碰撞事件通知 */
        if (collision) {
            s_dcb.ct_collision = 0;
            if ((s_dcb.status & GSENSOR_EVENT_COLLISION) == 0) {
                #if DEBUG_GSENSOR > 0
                printf_com("collision+xyz(%d)(%d)(%d),(%d)(%d)(%d),(%d)(%d)(%d)\r\n", xyz.x, xyz.y, xyz.z, 
                                                                                      s_dcb.xyz.x, s_dcb.xyz.y, s_dcb.xyz.z, 
                                                                                      SUB(xyz.x, s_dcb.xyz.x), SUB(xyz.y, s_dcb.xyz.y), SUB(xyz.z, s_dcb.xyz.z));
                #endif
                
                s_dcb.status |= GSENSOR_EVENT_COLLISION;
                if (s_dcb.handler != 0) {
                    s_dcb.handler(s_dcb.status);
                }
            }
        } else {
            if ((s_dcb.status & GSENSOR_EVENT_COLLISION) != 0) {
                if (++s_dcb.ct_collision > 125) {
                    #if DEBUG_GSENSOR > 0
                    printf_com("collision-xyz(%d)(%d)(%d),(%d)(%d)(%d),(%d)(%d)(%d)\r\n", xyz.x, xyz.y, xyz.z, 
                                                                                      s_dcb.xyz.x, s_dcb.xyz.y, s_dcb.xyz.z, 
                                                                                      SUB(xyz.x, s_dcb.xyz.x), SUB(xyz.y, s_dcb.xyz.y), SUB(xyz.z, s_dcb.xyz.z));
                    #endif
                    
                    s_dcb.ct_collision = 0;
                    s_dcb.status &= ~GSENSOR_EVENT_COLLISION;
                    if (s_dcb.handler != 0) {
                        s_dcb.handler(s_dcb.status);
                    }
                }
            }
        }
        /* 侧翻事件通知 */
        if (rollover) {
            if ((s_dcb.status & GSENSOR_EVENT_ROLLOVER) == 0) {
                if ((s_dcb.status & GSENSOR_EVENT_COLLISION) != 0 && ++s_dcb.ct_rollover > 100) {
                    #if DEBUG_GSENSOR > 0
                    printf_com("rollover+xyz(%d)(%d)(%d),(%d)(%d)(%d)\r\n", xyz.x, xyz.y, xyz.z, 
                                                                            s_dcb.xyz.x, s_dcb.xyz.y, s_dcb.xyz.z);
                    #endif
                    
                    s_dcb.ct_rollover = 0;
                    s_dcb.status |= GSENSOR_EVENT_ROLLOVER;
                    if (s_dcb.handler != 0) {
                        s_dcb.handler(s_dcb.status);
                    }
                }
            } else {
                s_dcb.ct_rollover = 0;
            }
        } else {
            if ((s_dcb.status & GSENSOR_EVENT_ROLLOVER) != 0) {
                if (++s_dcb.ct_rollover > 100) {
                    #if DEBUG_GSENSOR > 0
                    printf_com("rollover-xyz(%d)(%d)(%d):(%d)(%d)(%d)\r\n", xyz.x, xyz.y, xyz.z, 
                                                                            s_dcb.xyz.x, s_dcb.xyz.y, s_dcb.xyz.z);
                    #endif
                    
                    s_dcb.ct_rollover = 0;
                    s_dcb.status &= ~GSENSOR_EVENT_ROLLOVER;
                    if (s_dcb.handler != 0) {
                        s_dcb.handler(s_dcb.status);
                    }
                }
            } else {
                s_dcb.ct_rollover = 0;
            }
        } 
        
        YX_MEMCPY(&s_dcb.xyz, sizeof(s_dcb.xyz), &xyz, sizeof(xyz));
    } else {
        if (++s_dcb.ct_error >= MAX_ERROR) {
            s_dcb.ct_error = 0;
            //if ((s_dcb.status & GSENSOR_EVENT_VALID) != 0) {
                #if DEBUG_GSENSOR > 0
                printf_com("<gsensor is invalid>\r\n");
                #endif
                
                s_dcb.status &= ~GSENSOR_EVENT_VALID;
                if (s_dcb.handler != 0) {
                    s_dcb.handler(s_dcb.status);
                }
            //}
            
            OS_StopTmr(s_sampletmr);
        }
    }
}

/*******************************************************************
** 函数名:     DAL_HIT_InitDrv
** 函数描述:   模块初始化
** 参数:       无
** 返回:       无
********************************************************************/
void DAL_HIT_InitDrv(void)
{
    YX_MEMSET(&s_dcb, 0, sizeof(s_dcb));
    
    //DAL_PP_ReadParaByID(PP_GSENSOR_, (INT8U *)&s_dcb.calibration, sizeof(s_dcb.calibration));
    
    s_sampletmr = OS_CreateTmr(TSK_ID_DAL, (void *)0, SampleTmrProc);
    OS_StartTmr(s_sampletmr, PERIOD_SAMPLE);
}

/*******************************************************************
** 函数名:     DAL_HIT_SetGsensorPara
** 函数描述:   设置加速度传感器参数
**             [in]  acceleration:    加速度阈值,单位:0.1g
**             [in]  sensitivity:     时间灵敏度, 单位: ms
**             [in]  angle:           侧翻角度阈值
**             [in]  auto_report:     true:  开启自动定时上报,false: 关闭自动定时上报
**             [in]  period:          定时上报间隔周期, 单位: s
** 返回:       成功返回TRUE, 失败返回FALSE
********************************************************************/
BOOLEAN DAL_HIT_SetGsensorPara(INT8U acceleration, INT16U sensitivity, INT8U angle, BOOLEAN auto_report, INT8U period)
{
    s_dcb.acceleration = acceleration;
    s_dcb.angle        = angle;
    if (auto_report && acceleration != 0 && angle != 0) {
        s_dcb.onoff = true;
        if (!OS_TmrIsRun(s_sampletmr)) {
            OS_StartTmr(s_sampletmr, PERIOD_SAMPLE);
        }
    } else {
        s_dcb.onoff = false;
        s_dcb.status &= ~(GSENSOR_EVENT_ROLLOVER | GSENSOR_EVENT_COLLISION);
        OS_StopTmr(s_sampletmr);
    }
    
    return TRUE;
}

/*******************************************************************
** 函数名:     DAL_HIT_GetGsensorStatus
** 函数描述:   读取重力加速度传感器状态
** 参数:       [in] status: 重力传感器状态, GSENSOR_EVENT_E 事件组合
** 返回:       返回传感器状态, GSENSOR_EVENT_E 事件组合
********************************************************************/
INT8U DAL_HIT_GetGsensorStatus(void)
{
    return s_dcb.status;
}

/*******************************************************************
** 函数名:     DAL_HIT_RegistEventHandler
** 函数描述:   注册事件处理器
** 参数:       [in] handler: 事件处理器,event为GSENSOR_EVENT_E事件组合
** 返回:       成功返回true,失败返回false
********************************************************************/
BOOLEAN DAL_HIT_RegistEventHandler(void (*handler)(INT8U event))
{
    s_dcb.handler = handler;
    return TRUE;
}

/*******************************************************************
** 函数名:     DAL_HIT_StartCalibration
** 函数描述:   启动加速度传感器标定
** 参数:       无
** 返回:       成功返回true,失败返回false
********************************************************************/
BOOLEAN DAL_HIT_StartCalibration(void)
{
    GSENSOR_XYZ_T xyz;
    
    s_dcb.calibration.isvalid = false;
    if (HAL_HIT_GetValueXYZ(&xyz)) {
        s_dcb.calibration.x = xyz.x;
        s_dcb.calibration.y = xyz.y;
        s_dcb.calibration.z = xyz.z;
        
        //DAL_PP_StoreParaByID(PP_GSENSOR_, (INT8U *)&s_dcb.calibration, sizeof(s_dcb.calibration));
        return true;
    } else {
        return false;
    }
}

/*******************************************************************
** 函数名:     DAL_HIT_StopCalibration
** 函数描述:   停止加速度传感器标定
** 参数:       无
** 返回:       成功返回true,失败返回false
********************************************************************/
BOOLEAN DAL_HIT_StopCalibration(void)
{
    GSENSOR_XYZ_T xyz;
    
    if (HAL_HIT_GetValueXYZ(&xyz)) {
        if (SUB(xyz.x, s_dcb.calibration.x) > 50 || SUB(xyz.y, s_dcb.calibration.y) > 50 || SUB(xyz.z, s_dcb.calibration.z) > 50) {
            return false;
        }
        
        if (SUB(xyz.x, s_dcb.xyz.x) > 50 || SUB(xyz.y, s_dcb.xyz.y) > 50 || SUB(xyz.z, s_dcb.xyz.z) > 50) {
            return false;
        }
        
        s_dcb.calibration.isvalid = true;
        s_dcb.calibration.x = (xyz.x + s_dcb.xyz.x + s_dcb.calibration.x) / 3;
        s_dcb.calibration.y = (xyz.y + s_dcb.xyz.y + s_dcb.calibration.y) / 3;
        s_dcb.calibration.z = (xyz.z + s_dcb.xyz.z + s_dcb.calibration.z) / 3;
        //DAL_PP_StoreParaByID(PP_GSENSOR_, (INT8U *)&s_dcb.calibration, sizeof(s_dcb.calibration));
        return true;
    } else {
        return false;
    }
}

#endif

/************************** (C) COPYRIGHT 2010 XIAMEN YAXON.LTD *******************END OF FILE******/


