/**************************************************************************************************
**                                                                                               **
**  文件名称:  Scanner.h                                                                         **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2010                                    **
**  创建信息:  Lantu.Cai -- 2010年12月6日                                                        **
**  文件描述:  扫描模块接口函数                                                                  **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/
#ifndef __SCANNER_H
#define __SCANNER_H
#include  "dal_include.h"

typedef void (*SCANENTRY)(void);

void InitScanner(void);
BOOLEAN  InstallScanner(SCANENTRY entry,INT32U cycles);
void ScannerRunning(void);

#endif /*__SCANNER_H*/

/************************ (C) COPYRIGHT 2010 XIAMEN YAXON.LTD ******************END OF FILE******/


