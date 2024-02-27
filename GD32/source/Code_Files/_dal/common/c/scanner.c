/**************************************************************************************************
**                                                                                               **
**  文件名称:  Scanner.c                                                                         **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2010                                    **
**  创建信息:  Lantu.Cai -- 2010年12月6日                                                        **
**  文件描述:  扫描模块接口函数                                                                  **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/
#include "scanner.h"
/*************************************************************************************************/
/*                           扫描模块入口结构体                                                  */
/*************************************************************************************************/
 
typedef struct {
   INT32U    maxcycles;                       /* wait for cycles */
   INT32U    cyclecnt;                        /* cycle count */
   SCANENTRY entry;                           /* program entry */
}SCANENTRY_T;
 
static  SCANENTRY_T  s_scanentry[MAX_SCANENTRY];
static  INT8U        s_entrynums;
 
/**************************************************************************************************
**  函数名称:  InitScanner
**  功能描述:  初始化扫描模块
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
void InitScanner(void)
{
    INT8U i;
    
    for (i=0; i<MAX_SCANENTRY; i++) {
        s_scanentry[i].maxcycles = 0;
        s_scanentry[i].cyclecnt  = 0;
        s_scanentry[i].entry     = 0;
    } 
    
    s_entrynums = 0;
}


/**************************************************************************************************
**  函数名称:  InstallScanner
**  功能描述:  安装扫描器接口
**  输入参数:  entry   : 扫描器的执行函数
**          :  cycles  : 需要等待的时间，0表示不需等待，执行扫描
**  返回参数:  安装成功返回TRUE,失败返回FALSE
**************************************************************************************************/
BOOLEAN  InstallScanner(SCANENTRY entry,INT32U cycles)
{
     DAL_ASSERT(entry != 0);
     
     if (s_entrynums >= MAX_SCANENTRY) return FALSE;
     
     s_scanentry[s_entrynums].maxcycles = cycles;
     s_scanentry[s_entrynums].cyclecnt  = cycles;
     s_scanentry[s_entrynums++].entry   = entry;
     
     return TRUE;
}

/**************************************************************************************************
**  函数名称:  ScannerRunning
**  功能描述:  执行扫描，遍历所有安装过的扫描器
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
void ScannerRunning(void)
{
    INT8U i;
    
    for (i=0; i<s_entrynums; i++) {
        if (s_scanentry[i].maxcycles == 0) {
            s_scanentry[i].entry();
        } else {
            if (s_scanentry[i].cyclecnt == 0) {
                s_scanentry[i].cyclecnt = s_scanentry[i].maxcycles;
                s_scanentry[i].entry();
            } else {
                s_scanentry[i].cyclecnt--;
            }
        } 
    }
}
/************************* (C) COPYRIGHT 2010 XIAMEN YAXON.LTD ********************END OF FILE******/

