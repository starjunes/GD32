/******************************************************************************
**
** Filename:     os_errman.c
** Copyright:    
** Description:  该模块主要实现系统运行出错管理
**
-------------------------------------------------------------------------------
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#include "hal_include.h"
#include "os_include.h"
#include "os_errman.h"
#include "yx_misc.h"

/*
********************************************************************************
* define config parameters
********************************************************************************
*/


/*
********************************************************************************
* define struct
********************************************************************************
*/
typedef struct {
    INT8U curindex;
    INT8U nused;
    INT8U nbackup;
    INT8U ct_reset;
    
    void (*diagproc[OS_MAX_DIAG])(void);
    void (*backproc[OS_MAX_DIAG])(void);
    
    void (*c_informer[RESET_PRI_MAX][RESET_REG_MAX])(INT8U event, char *filename, INT32U line);
    void (*b_informer[RESET_PRI_MAX][RESET_REG_MAX])(INT8U event, char *filename, INT32U line);
} ECB_T;

/*
********************************************************************************
* define module variants
********************************************************************************
*/
static ECB_T s_ecb;


/*******************************************************************
** 函数名称:   OS_InitErrMan
** 函数描述:   模块初始化函数
** 参数:       无
** 返回:       无
********************************************************************/
void OS_InitErrMan(void)
{
    YX_MEMSET(&s_ecb, 0, sizeof(s_ecb));
}

/*******************************************************************
** 函数名称:   OS_RegistResetInform
** 函数描述:   注册复位通知接口
** 参数:       [in]: prior:待处理事件的优先级,见RESET_PRI_E
**			   [in]: fp   :回调函数
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN OS_RegistResetInform(INT8U prior, void (*fp)(INT8U event, char *filename, INT32U line))
{
    INT8U i;

    if (fp == 0) {
        return false;
    }
    
    if (prior >= RESET_PRI_MAX) {
        return false;
    }
    
    for (i = 0; i < RESET_REG_MAX; i++) {
        if (s_ecb.c_informer[prior][i] == 0) {
            s_ecb.c_informer[prior][i] = fp;
            s_ecb.b_informer[prior][i] = fp;
            return true;
        }
    }
    OS_ASSERT(false, RETURN_FALSE);
}

/*******************************************************************
** 函数名称:   OS_Reset
** 函数描述:   复位设备
** 参数:       [in] event：    复位事件
**             [in] pfile：    出错文件名
**             [in] line：     出错行号
** 返回:       无
********************************************************************/
void OS_Reset(INT8U event, char *filename, INT32U line)
{
    INT8U i, j, len;
    INT8U *ptr;
    INT32U count = 0xfffff;
    
    len = YX_STRLEN(filename);
    ptr = (INT8U *)filename;
    for (i = len; i > 0; i--) {
        if (ptr[i - 1] == '\\') {
            break;
        }
    }
    ptr += i;
    len -= i;
    
    #if DEBUG_ERR > 0
    printf_com("<assert:file(%s), line(%d), event(%d)>\r\n", ptr, line, event);
    #endif
    
    s_ecb.ct_reset++;
    if (event != RESET_EVENT_DIRECT && s_ecb.ct_reset <= 1) {
        for (i = 0; i < RESET_PRI_MAX; i++) {
            for (j = 0; j < RESET_REG_MAX; j++) {
                if (s_ecb.c_informer[i][j] == s_ecb.b_informer[i][j] && s_ecb.c_informer[i][j] != 0) {
                    s_ecb.c_informer[i][j](event, (char *)ptr, line);
                }
            }
        }
    }

    for(;;) {
        if (count > 0) {
            if (--count == 0) {
                ClearWatchdog();
            }
        }
    }
    //PORT_ResetCPU();
}

/*******************************************************************
** 函数名称:   OS_ErrTskEntry
** 函数描述:   错误处理任务入口函数
** 参数:       无
** 返回:       无
********************************************************************/
void OS_ErrTskEntry(void)
{
    INT8U i;
    
    OS_ASSERT((s_ecb.nused == s_ecb.nbackup && s_ecb.nused <= OS_MAX_DIAG), RETURN_VOID);
    if (s_ecb.nused == 0) {
        return;
    }
    
    i = s_ecb.curindex++;
    if (i >= s_ecb.nused) {
        i = 0;
        s_ecb.curindex = 0;
    }
    OS_ASSERT((s_ecb.diagproc[i] == s_ecb.backproc[i] && s_ecb.diagproc[i] != 0), RETURN_VOID);
    s_ecb.diagproc[i]();
}

/*******************************************************************
** 函数名称:   OS_RegistDiagnoseProc
** 函数描述:   注册一个诊断函数
** 参数:       [in]  diagproc: 待安装的诊断函数
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN OS_RegistDiagnoseProc(void (*diagproc)(void))
{
    OS_ASSERT((s_ecb.nused < OS_MAX_DIAG && diagproc != 0), RETURN_FALSE);
    
    s_ecb.diagproc[s_ecb.nused++]   = diagproc;
    s_ecb.backproc[s_ecb.nbackup++] = diagproc;
    
    return true;
}
