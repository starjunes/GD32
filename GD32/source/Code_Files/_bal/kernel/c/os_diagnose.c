/********************************************************************************
**
** 文件名:     os_diagnose.c
** 版权所有:   (c) 1998-2017 厦门雅迅网络股份有限公司
** 文件描述:   实现虚拟内核错误诊断管理功能
** 创建人：        陈从华
*********************************************************************************
**                  修改历史记录
**===============================================================================
**| 日期              | 作者        |  修改记录
**===============================================================================
**| 2017/05/15 | 黄运峰    |  移植、修改、规范化
********************************************************************************/
#include "yx_includes.h"
#include "port_tsk.h"
#include "port_timer.h"
#include "os_diagnose.h"
#include "debug_print.h"
//#include "bal_debug.h"

/*
*********************************************************************************
*                   模块数据类型、变量及宏定义
*********************************************************************************
*/
// 定义错误诊断管理结构体
typedef struct {
    INT8U curindex;
    INT8U nused;
    INT8U nbackup;
    INT8U ct_reset;
    
    void (*DiagProc[MAX_DIAG])(void);
    void (*BackProc[MAX_DIAG])(void);
    
    void (*c_informer[MAX_RESET_INFORM])(INT8U resettype, INT8U *filename, INT32U line, INT32U errid);
    void (*b_informer[MAX_RESET_INFORM])(INT8U resettype, INT8U *filename, INT32U line, INT32U errid);
} ECB_T;

// 定义模块局部变量
static ECB_T s_ecb;
/*
*********************************************************************************
*                   本地接口实现
*********************************************************************************
*/

/*
*********************************************************************************
*                   对外接口实现
*********************************************************************************
*/
/********************************************************************************
** 函数名:     OS_InitDiag
** 函数描述:   模块初始化函数
** 参数:       无
** 返回:       无
********************************************************************************/
void OS_InitDiag(void)
{
    YX_MEMSET(&s_ecb, 0, sizeof(s_ecb));
}

/********************************************************************************
** 函数名:     OS_RegResetHooker
** 函数描述:   复位注册接口,处理复位前的紧急事件,如参数保存等
** 参数:       [in]: prior，待处理事件的优先级
**			   [in]: fp，回调函数
** 返回:       成功返回true，失败返回false
********************************************************************************/
BOOLEAN OS_RegResetHooker(INT8U prior, void (*fp)(INT8U resettype, INT8U *filename, INT32U line, INT32U errid))
{
    INT8U i;

    if (fp == 0) {
        return false;
    }
    
    if (prior == RESET_HDL_PRIO_HIGH) {
        for (i = 0; i < MAX_RESET_INFORM / 2; i++) {
            if (s_ecb.c_informer[i] == 0) {
                s_ecb.c_informer[i] = fp;
                s_ecb.b_informer[i] = fp;
                return true;
            }
        }
        OS_ASSERT(false, RETURN_FALSE);
    } else if (prior == RESET_HDL_PRIO_LOW) {
        for (i = MAX_RESET_INFORM / 2; i < MAX_RESET_INFORM; i++) {
            if (s_ecb.c_informer[i] == 0) {
                s_ecb.c_informer[i] = fp;
                s_ecb.b_informer[i] = fp;
                return true;
            }
        }
        //OS_ASSERT(false, RETURN_FALSE); 
    } else {
        //OS_ASSERT(false, RETURN_FALSE); 
    }
    return false;
}

/********************************************************************************
** 函数名:     OS_Reset
** 函数描述:   复位,复位前是否会回调注册进来的复位通知函数，取决于RESET_E
** 参数:       [in] rsttype：  复位类型
**             [in] pfilename：文件名
**             [in] line：     出错行号
**             [in] errid：    错误ID
** 返回:       无
********************************************************************************/
void OS_Reset(INT8U rsttype, char *pfilename, INT32U line, INT32U errid)
{
    INT8U i, len;
    INT8U *ptr;
#if DEBUG_USE_SYSTEM > 0
    INT8U tempbuf[10];
    INT8U templen;
#endif
    
    len = YX_STRLEN(pfilename);
    ptr = (INT8U *)pfilename;
    for (i = len; i > 0; i--) {
        if (ptr[i - 1] == '\\') {
            break;
        }
    }
    ptr += i;
    len -= i;
    
#if DEBUG_ERR > 0
    if (!debug_printf_dir("<assertion failed:file(%s), line(%d), rsttype(%d), errid(%d)>\r\n", ptr, line, rsttype, errid)) {
#if DEBUG_USE_SYSTEM > 0
        PORT_Trace((INT8U *)"<assertion failed:file:", YX_STRLEN("<assertion failed:file:"));
        PORT_Trace(ptr, YX_STRLEN((char *)ptr));
        PORT_Trace((INT8U *)",line:", YX_STRLEN(",line:"));
        templen = bal_DecToAscii(tempbuf, line, 0);
        PORT_Trace(tempbuf, templen);
        PORT_Trace((INT8U *)">\r\n", YX_STRLEN(">\r\n"));
#endif
    }
#else
#if DEBUG_USE_SYSTEM > 0
        tempbuf[0] = 0;
        templen = 0;
        
        PORT_Trace((INT8U *)"<assertion failed:file:", YX_STRLEN("<assertion failed:file:"));
        PORT_Trace(ptr, YX_STRLEN((char *)ptr));
        PORT_Trace((INT8U *)",line:", YX_STRLEN(",line:"));
        templen = bal_DecToAscii(tempbuf, line, 0);
        PORT_Trace(tempbuf, templen);
        PORT_Trace((INT8U *)">\r\n", YX_STRLEN(">\r\n"));
#endif
#endif

    s_ecb.ct_reset++;
    if (rsttype != RESET_DIRECT && s_ecb.ct_reset <= 1) {                      /* 非直接复位，需要回调通知函数 */
        for (i = 0; i < MAX_RESET_INFORM; i++) {
            if (s_ecb.c_informer[i] == s_ecb.b_informer[i] && s_ecb.c_informer[i] != 0) {
                s_ecb.c_informer[i](rsttype, ptr, line, errid);
            }
        }
    }
    PORT_InstallCommonHdl(0);                                                  /* 注销消息回调 */
    PORT_InstallEventHdl(0);
    //PORT_PauseTimer(0);
    PORT_ResetCPU();
}

/********************************************************************************
** 函数名:     OS_DiagEntry
** 函数描述:   诊断模块的入口函数
** 参数:       无
** 返回:       无
********************************************************************************/
void OS_DiagEntry(void)
{
    INT8U tag;
    
    OS_ASSERT((s_ecb.nused == s_ecb.nbackup && s_ecb.nused <= MAX_DIAG), RETURN_VOID);
    if (s_ecb.nused == 0) {
        return;
    }
    
    if ((tag = s_ecb.curindex++) >= s_ecb.nused) {
        tag    = 0;
        s_ecb.curindex = 0;
    }
    OS_ASSERT((s_ecb.DiagProc[tag] == s_ecb.BackProc[tag] && s_ecb.DiagProc[tag] != 0), RETURN_VOID);
    s_ecb.DiagProc[tag]();
}

/********************************************************************************
** 函数名:     OS_InstallDiag
** 函数描述:   安装一个诊断函数
** 参数:       [in]  diagproc: 待安装的诊断函数
** 返回:       成功返回true，失败返回false
********************************************************************************/
BOOLEAN OS_InstallDiag(void (*diagproc)(void))
{
    OS_ASSERT((s_ecb.nused < MAX_DIAG && diagproc != 0), RETURN_FALSE);
    
    s_ecb.DiagProc[s_ecb.nused++]   = diagproc;
    s_ecb.BackProc[s_ecb.nbackup++] = diagproc;
    
    return true;
}

//------------------------------------------------------------------------------
/* End of File */
