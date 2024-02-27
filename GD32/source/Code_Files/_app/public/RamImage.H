/**************************************************************************************************
**                                                                                               **
**  文件名称:  RamImage.H                                                                        **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2010                                    **
**  创建信息:  Lantu.Cai -- 2010年12月28日                                                       **
**  文件描述:  RAM存储重启（非掉电）不丢失数据                                                   **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/
#ifndef DEF_RAMIMAGE
#define DEF_RAMIMAGE

#include  "app_include.h"

#define SIZE_PARASECTOR                 40  /* 40字节用来存储参数 */
#define RESTIMES_NORMAL                  1  /* 一天内常规复位次数 */
#define RESTIMES_NOACK                   4  /* 上电无应答复位次数 */
#define RESTIMES_ABNORMITY               5  /* 一天内被判断异常后被请求复位次数 */
#define RESTIMES_DATE                    10 /* 一天内复位的总次数*/


/*************************************************************************************************/
/*                                定义存储区参数位置枚举                                         */
/*依次：校验码、24小时内复位次数、上电无应答复位次数、常规复位次数、异常复位次数、年、月、日     */
/*************************************************************************************************/

typedef enum {
    ram_para_chkcode = 0x00,                 /* 存储校验码，参数累加和取反码 */
    ram_para_24HResTimes,                    /* 24小时内的复位总次数 */
    ram_para_NoAckResTimes,                  /* 因上电无应答导致复位的次数 */
    ram_para_NormalResTimes,                 /* 常规复位次数 */
    ram_para_AbNormalResTimes,               /* 异常复位次数 */
    ram_para_Year,                           /* 存储年 */
    ram_para_Month,                          /* 存储月 */
    ram_para_Date,                           /* 存储日 */
    ram_para_MAX
}RAM_PARA_E;


BOOLEAN RamImageValid(void);
void ClearRamImage(void);
BOOLEAN StoreRamImage(INT8U *ptr, INT8U len);
void ReadRamImage(INT8U *ptr, INT8U len);


#endif

/************************ (C) COPYRIGHT 2010 XIAMEN YAXON.LTD *******************END OF FILE******/

