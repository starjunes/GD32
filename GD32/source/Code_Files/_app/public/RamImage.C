/**************************************************************************************************
**                                                                                               **
**  文件名称:  RamImage.H                                                                        **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2010                                    **
**  创建信息:  Lantu.Cai -- 2010年12月28日                                                       **
**  文件描述:  RAM存储重启（非掉电）不丢失数据                                                   **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/
#include "tools.h"
#include "ramimage.h"

#pragma arm section zidata = "RAM_IMAGE"
static  INT8U RamImage_mem[SIZE_PARASECTOR];
#pragma arm section zidata

/**************************************************************************************************
**  函数名称:  RamImageValid
**  功能描述:  校验参数区数据是否正确
**  输入参数:  None
**  返回参数:  与校验码相等返回TRUE,反之返回FALSE
**************************************************************************************************/
BOOLEAN RamImageValid(void)
{
    INT8U   chksum;
    INT8U   *iptr;
    
    iptr = &RamImage_mem[1];
    chksum = RamImage_mem[ram_para_chkcode];
    if (chksum != Getchksum_n(iptr, SIZE_PARASECTOR - 1)) {
        return FALSE;
    } else {
        return TRUE;
    }
}

/**************************************************************************************************
**  函数名称:  ClearRamImage
**  功能描述:  参数区清零
**  输入参数:  None
**  返回参数:  None
**************************************************************************************************/
void ClearRamImage(void)
{
    INT8U i;
    INT8U *iptr;
    
    for(i = 1; i < SIZE_PARASECTOR; i++) {
         RamImage_mem[i] = 0;   
    }
    iptr = RamImage_mem;
    iptr++;                /* 第0字节是校验码，从第1字节开始到参数区结尾累加取反 */
    RamImage_mem[ram_para_chkcode] = Getchksum_n(iptr, SIZE_PARASECTOR - 1);
}

/**************************************************************************************************
**  函数名称:  StoreRamImage
**  功能描述:  往参数区写数据
**  输入参数:  ptr : 指向要存储的数据地址
**          :  len : 要存储的数据个数
**  返回参数:  TRUE 或者 FALSE
**************************************************************************************************/
BOOLEAN StoreRamImage(INT8U *ptr, INT8U len)
{
    INT8U *iptr;   

    iptr = &RamImage_mem[1];  
    memcpy(iptr, ptr, len);
    RamImage_mem[ram_para_chkcode] = Getchksum_n(iptr, SIZE_PARASECTOR - 1);  
    return TRUE;
}

/**************************************************************************************************
**  函数名称:  ReadRamImage
**  功能描述:  读参数区参数
**  输入参数:  ptr : 指向要读取的数据地址
**          :  len : 要读取的数据个数
**  返回参数:  None
**************************************************************************************************/
void ReadRamImage(INT8U *ptr, INT8U len)
{
    INT8U *iptr;
    iptr = &RamImage_mem[0];
    memcpy(ptr, iptr, len);  
}
/************************ (C) COPYRIGHT 2010 XIAMEN YAXON.LTD *******************END OF FILE******/

