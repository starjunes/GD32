/********************************************************************************
**
** 文件名:     mmi_resetcnt.c
** 版权所有:   (c) 2014-2020 厦门雅迅网络股份有限公司
** 文件描述:   复位统计模块,目前暂统计如下信息
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2016/1/4   | JUMP   | 创建本模块
**
*********************************************************************************/
#include  "dal_include.h"
#include  "mmi_resetcnt.h"
#include  "systime.h"
#include  "public.h"
#include  "database.h"
#include  "tools.h"
#include  "debug_print.h"
#include  "app_include.h"


/*
*****************************************************************
*   宏定义
*****************************************************************
*/


/*
*****************************************************************
*   静态变量定义
*****************************************************************
*/
static RESETREC_T s_resetcnt;


/*******************************************************************
** 函数名:     fill_reset_extraarea
** 函数描述:   存储信息
** 参数:       [in] err_id：   错误ID
**             [in] filename： 文件名
**             [in] line：     出错行号
** 返回:       无
********************************************************************/
static void fill_reset_extraarea(INT32U err_id, INT8U *filename, INT32U line)
{
    INT8U len;

    memmove(&s_resetcnt.rst_rec_ext[1], &s_resetcnt.rst_rec_ext[0], sizeof(REC_EXT_T) * (MAX_REC_EXT - 1));
    GetSysTime(&s_resetcnt.rst_rec_ext[0].start_time);
    len = CutRedundantLen(sizeof(s_resetcnt.rst_rec_ext[0].filename), strlen((char*)filename));
    MMI_MEMCPY(s_resetcnt.rst_rec_ext[0].filename, sizeof(s_resetcnt.rst_rec_ext[0].filename), filename, len);

    s_resetcnt.rst_rec_ext[0].err_id   = err_id;
    s_resetcnt.rst_rec_ext[0].line     = line;
}

/*******************************************************************
** 函数名:     mmi_resetinform
** 函数描述:   复位回调接口
** 参数:       [in] resettype：复位类型
**             [in] filename： 文件名
**             [in] line：     出错行号
**             [in] errid：    错误ID
** 返回:       无
********************************************************************/
void mmi_resetinform(INT8U resettype, INT8U *filename, INT32U line, INT32U errid)
{
    if (resettype == RESET_INITIATIVE) {
      #if DEBUG_RESET > 0
        Debug_SysPrint("内部复位\r\n");
      #endif
        if (ReadPubPara(RESETREC_, &s_resetcnt)) {
            s_resetcnt.rst_int++;
            s_resetcnt.rst_total = s_resetcnt.rst_int + s_resetcnt.rst_ext + s_resetcnt.rst_err + s_resetcnt.rst_link - 1;
        } else {
          #if DEBUG_RESET > 0
            Debug_SysPrint("ReadPubPara wrong\r\n");
          #endif
            memset(&s_resetcnt, 0, sizeof(s_resetcnt));
            s_resetcnt.rst_int  = 1;
            s_resetcnt.rst_ext  = 0;
            s_resetcnt.rst_err  = 0;
            s_resetcnt.rst_link = 0;
            s_resetcnt.rst_total= 0;                                            /* 与总和差1才能在下次启动时判断出不是外部复位 */
        }
        fill_reset_extraarea(errid, filename, line);
    } else if (resettype == RESET_ABNORMAL) {
      #if DEBUG_RESET > 0
        Debug_SysPrint("异常复位\r\n");
			  Debug_SysPrint(", File[%-15s]",  filename);
        Debug_SysPrint(", Line[%.4d]",   line);
        Debug_SysPrint(", ErrID[%.8X]",  errid);
        Debug_SysPrint("\r\n");
      #endif
        if (ReadPubPara(RESETREC_, &s_resetcnt)) {
            s_resetcnt.rst_err++;
            s_resetcnt.rst_total = s_resetcnt.rst_int + s_resetcnt.rst_ext + s_resetcnt.rst_err + s_resetcnt.rst_link - 1;
        } else {
          #if DEBUG_RESET > 0
            Debug_SysPrint("ReadPubPara wrong\r\n");
          #endif
            memset(&s_resetcnt, 0, sizeof(s_resetcnt));
            s_resetcnt.rst_int  = 0;
            s_resetcnt.rst_ext  = 0;
            s_resetcnt.rst_err  = 1;
            s_resetcnt.rst_link = 0;
            s_resetcnt.rst_total= 0;                                            /* 与总和差1才能在下次启动时判断出不是外部复位 */
        }
        fill_reset_extraarea(errid, filename, line);
    } else if (resettype == RESET_LINK) {
      #if DEBUG_RESET > 0
        Debug_SysPrint("链路复位\r\n");
      #endif
        if (ReadPubPara(RESETREC_, &s_resetcnt)) {
            s_resetcnt.rst_link++;
            s_resetcnt.rst_total = s_resetcnt.rst_int + s_resetcnt.rst_ext + s_resetcnt.rst_err + s_resetcnt.rst_link - 1;
        } else {
          #if DEBUG_RESET > 0
            Debug_SysPrint("ReadPubPara wrong\r\n");
          #endif
            memset(&s_resetcnt, 0, sizeof(s_resetcnt));
            s_resetcnt.rst_int  = 0;
            s_resetcnt.rst_ext  = 0;
            s_resetcnt.rst_err  = 0;
            s_resetcnt.rst_link = 1;
            s_resetcnt.rst_total= 0;                                            /* 与总和差1才能在下次启动时判断出不是外部复位 */
        }
        fill_reset_extraarea(errid, filename, line);
    } else {
        return;
    }
  #if EN_CHECKFLASH
  setflashflag();
  #endif
  StorePubPara(RESETREC_, &s_resetcnt);
  #if 0 /* RESETREC_不备份 */
  BakParaReqSingle(RESETREC_); /* 备份参数到车台 */
  #endif
  #if EN_CHECKFLASH
  clearflashflag();
  #endif
}

/*******************************************************************
** 函数名:     mmi_resetcnt_init
** 函数描述:   初始化出错统计模块
** 参数:       无
** 返回:       无
********************************************************************/
void mmi_resetcnt_init(void)
{
    INT32U tmpCnt;
    BOOLEAN store;
  #if DEBUG_RESET > 0
    INT8U i;
    INT8U temp[FILENAMELEN + 1];

    Debug_SysPrint("mmi_resetcnt_init\r\n");
  #endif

    store = false;
    if (ReadPubPara(RESETREC_, &s_resetcnt) == 0) {
        memset(&s_resetcnt, 0, sizeof(s_resetcnt));
        store = true;
    } else {
        tmpCnt = s_resetcnt.rst_int + s_resetcnt.rst_ext + s_resetcnt.rst_err + s_resetcnt.rst_link;
        if (tmpCnt == s_resetcnt.rst_total + 1) {                              /* 说明是已知的复位, 而不是外部的复位 */
            s_resetcnt.rst_total++;
            store = true;
        } else if (tmpCnt == s_resetcnt.rst_total) {                           /* 说明是外部复位. */
            /* 受到写flash次数限制，暂时不统计外部复位 */
            //s_resetcnt.rst_total++;
            //s_resetcnt.rst_ext++;
            //fill_reset_extraarea(ERR_ID_EXT, (INT8U *)__FILE__ + strlen(__FILE__) - FILENAMELEN, __LINE__);
            //store = true;
          #if DEBUG_RESET > 0
            Debug_SysPrint("外部复位重启\r\n");
          #endif
        } else {                                                               /* 说明统计和异常,重新加之 */
            s_resetcnt.rst_total = tmpCnt;
            store = true;
        }
    }
    if (store) {
      #if EN_CHECKFLASH
      setflashflag();
      #endif
      StorePubPara(RESETREC_, &s_resetcnt);
      #if 0 /* RESETREC_不备份 */
      BakParaReqSingle(RESETREC_); /* 备份参数到车台 */
      #endif
      #if EN_CHECKFLASH
      clearflashflag();
      #endif
    }

  #if DEBUG_RESET > 0
    Debug_SysPrint("\r\n总复位次数(%d) = 外部(%d) + 内部(%d) + 出错(%d) + 链路超时(%d)\r\n",
                 s_resetcnt.rst_total, s_resetcnt.rst_ext, s_resetcnt.rst_int, s_resetcnt.rst_err, s_resetcnt.rst_link);
    for (i = 0; i < MAX_REC_EXT; i++) {
        Debug_SysPrint("%d. ", i);
        Debug_SysPrint("Time[20%.2d-%.2d-%.2d %.2d:%.2d:%.2d]", s_resetcnt.rst_rec_ext[i].start_time.date.year,
                                                                s_resetcnt.rst_rec_ext[i].start_time.date.month,
                                                                s_resetcnt.rst_rec_ext[i].start_time.date.day,
                                                                s_resetcnt.rst_rec_ext[i].start_time.time.hour,
                                                                s_resetcnt.rst_rec_ext[i].start_time.time.minute,
                                                                s_resetcnt.rst_rec_ext[i].start_time.time.second);
        temp[FILENAMELEN] = '\0';
        MMI_MEMCPY(temp, sizeof(temp), s_resetcnt.rst_rec_ext[i].filename, FILENAMELEN);
        Debug_SysPrint(", File[%-15s]",                         temp);
        Debug_SysPrint(", Line[%.4d]",                          s_resetcnt.rst_rec_ext[i].line);
        Debug_SysPrint(", ErrID[%.8X]",                         s_resetcnt.rst_rec_ext[i].err_id);
        Debug_SysPrint("\r\n");
    }
  #endif
}

/*******************************************************************
** 函数名:     mmi_resetcnt_getptr
** 函数描述:   获取复位信息
** 参数:       无
** 返回:       无
********************************************************************/
RESETREC_T *mmi_resetcnt_getptr(void)
{
    if (ReadPubPara(RESETREC_, &s_resetcnt) == 0) {
        memset((INT8U*)&s_resetcnt, 0, sizeof(s_resetcnt));
    }
    return &s_resetcnt;
}


