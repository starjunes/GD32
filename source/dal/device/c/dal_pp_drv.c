/********************************************************************************
**
** 文件名:     dal_pp_drv.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:    该模块主要实现公共参数文件存储驱动管理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2012/01/15 | 叶德焰 |  创建第一版本
*********************************************************************************/
#include "yx_include.h"
#include "yx_misc.h"
#include "hal_sflash_drv.h"
#include "dal_pp_drv.h"



/*
********************************************************************************
* 定义模块配置参数
********************************************************************************
*/
// para operation attrib
#define WF_                  0x01  //need write to file_xx
#define WS_                  0x02  //need save to simbak
#define RS_                  0x04  //need read from simbak
#define CL_                  0x08  //need clear para in file_xx

// pubpara file type
#define FILE_0               '0'
#define FILE_1               '1'

#define VALID_               'V'
#define BOUND_FLAG           'F'
#define PP_LOCK              'L'
#define PERIOD_UPDATE        _SECOND, 1

#define PP_ADDR_MAIN         0x0000              /* 主存储区起始地址 */
#define PP_ADDR_BACK         0x0080              /* 备份存储区起始地址 */

#define PP_MAX_LEN           512                 /* 存储区最大长度 */

/*
********************************************************************************
* 定义模块数据结构
********************************************************************************
*/
typedef struct {
    INT8U   id;
    void    (*informer)(INT8U reason);
} INFORM_REG_T;

typedef struct {
    INT8U           valid;             /* 参数有效 */
    INT8U           attrib;            /* 操作属性 */
    INT16U          space;             /* 一个记录占用的空间 */
    INT32U          offset;            /* 参数位置 */
    PP_REG_T const *preg;
} PP_T;

typedef struct {
    INT16U            ct_delay;
    INT32U            filesize;
    PP_CLASS_T const *pclass;
} CLASS_T;

typedef struct {
    INT8U        lock;
    PP_T         pp[MAX_PP_NUM];
    CLASS_T      cls[MAX_PP_CLASS_NUM];
    INFORM_REG_T inform[MAX_PP_INFORM];
} DCB_T;

/*
********************************************************************************
* 定义模块静态变量
********************************************************************************
*/
static DCB_T s_dcb;
static INT8U s_updatetmr;



/*******************************************************************
** 函数名:      CheckParaValid
** 函数描述:    判断参数是否正确
** 参数:        [in] ptr：参数指针
**              [in] len：参数长度
** 返回:        有效返回true，无效返回false
********************************************************************/
static BOOLEAN CheckParaValid(INT8U *ptr, INT16U len)
{
    PP_HEAD_T *phead;
	
	phead = (PP_HEAD_T *)ptr;
    if (phead->chksum[0] != YX_GetChkSum_N(ptr + sizeof(PP_HEAD_T), len - sizeof(PP_HEAD_T) - 1)) {
        return false;
    }
    
    if (phead->chksum[1] != YX_ChkSum_XOR(ptr + sizeof(PP_HEAD_T), len - sizeof(PP_HEAD_T) - 1)) {
        return false;
    }
    
    return TRUE;
}

/*******************************************************************
** 函数名:      CopyFileToRam
** 函数描述:    拷贝参数到RAM区
** 参数:        [in] cls：参数区,见PP_CLASS_ID_E
** 返回:        成功返回true，失败返回false
********************************************************************/
static BOOLEAN CopyFileToRam(INT8U cls)
{
    BOOLEAN result;
    INT8U i, id;
    //INT8U *memptr = 0;
    PP_REG_T const *preg;
    PP_CLASS_T const *pclass;

    OS_ASSERT((cls < DAL_PP_GetRegClassMax()), RETURN_FALSE);
    //memptr = YX_MemMalloc(s_dcb.cls[cls].filesize);
    //OS_ASSERT((memptr != 0), RETURN_FALSE);
    
    pclass = s_dcb.cls[cls].pclass;
    OS_ASSERT((s_dcb.cls[cls].filesize == pclass->memlen), RETURN_FALSE);
    
    /* 主文件 */
    result = HAL_SFLASH_Read(PP_ADDR_MAIN, pclass->memptr, pclass->memlen);
    OS_ASSERT((result != 0), RETURN_FALSE);
    
    /* 备份文件 */
    ;
    
    /* 判断各个参数 */
    preg   = pclass->preg;
    for (i = 0; i < pclass->nreg; i++) {
        id = preg->id;
        OS_ASSERT((s_dcb.pp[id].space == sizeof(PP_HEAD_T) + preg->rec_size + 1), RETURN_FALSE);
        
        *(pclass->memptr + s_dcb.pp[id].offset + s_dcb.pp[id].space - 1) = BOUND_FLAG;
        if (CheckParaValid(pclass->memptr + s_dcb.pp[id].offset, s_dcb.pp[id].space)) {/* 主参数 */
            s_dcb.pp[id].valid = VALID_;
        }
        
        preg++;
    }
    
    //if (memptr != 0) {
    //    YX_MemFree(memptr);
    //}

    return true;
}

/*******************************************************************
** 函数名:      InitPubPara
** 函数描述:    初始化各个公共参数
** 参数:        无
** 返回:        无
********************************************************************/
static BOOLEAN InitPubPara(void)
{
    INT8U i, j, nclass, id, maxid;
    INT32U offset;
    PP_REG_T const *preg;
    PP_CLASS_T const *pclass;
    
    maxid = DAL_PP_GetRegPPMax();
    OS_ASSERT((maxid <= MAX_PP_NUM), RETURN_FALSE);
    nclass = DAL_PP_GetRegClassMax();
    OS_ASSERT((nclass <= MAX_PP_CLASS_NUM), RETURN_FALSE);
    
    for (i = 0; i < nclass; i++) {
        pclass = DAL_PP_GetRegClassInfo(i);
        preg   = pclass->preg;
        offset = 0;
        for (j = 0; j < pclass->nreg; j++) {
            /* 获取注册表信息 */
            OS_ASSERT((preg != 0), RETURN_FALSE);
            OS_ASSERT((preg->id < maxid), RETURN_FALSE);
            OS_ASSERT((preg->type == pclass->type), RETURN_FALSE);
            OS_ASSERT((offset + sizeof(PP_HEAD_T) + preg->rec_size + 1 <= pclass->memlen), RETURN_FALSE);
            
            id = preg->id;
            
            s_dcb.pp[id].preg    = preg;
            s_dcb.pp[id].valid   = 0;
            s_dcb.pp[id].attrib  = 0;
            s_dcb.pp[id].offset  = offset;
            s_dcb.pp[id].space   = sizeof(PP_HEAD_T) + preg->rec_size + 1;
            
            offset += s_dcb.pp[id].space;
            preg++;
        }
        s_dcb.cls[i].pclass   = pclass;
        s_dcb.cls[i].filesize = offset;
        OS_ASSERT((s_dcb.cls[i].filesize == pclass->memlen), RETURN_FALSE);
        OS_ASSERT((s_dcb.cls[i].filesize <= PP_MAX_LEN), RETURN_FALSE);
        
        #if DEBUG_PP >  0
        printf_com("<InitPubPara, class:(%d), filesize:(%d)>\r\n", i, offset);
        #endif
    }
    
    for (i = nclass; i > 0; i--) {
        if (!CopyFileToRam(i - 1)) {
            return false;
        }
        #if DEBUG_PP >  0
        printf_com("<CopyFileToRam, class:(%d)>\r\n", i - 1);
        #endif
    }
    return true;
}

/*******************************************************************
** 函数名:      FlushRamToFile
** 函数描述:    存储参数到文件
** 参数:        [in] cls：参数区,见PP_CLASS_ID_E
** 返回:        成功返回true，失败返回false
********************************************************************/
static BOOLEAN FlushRamToFile(INT8U cls)
{
    BOOLEAN result;
    INT8U j, id;
    INT32U offset;
    PP_HEAD_T *phead;
    PP_REG_T const *preg;
    PP_CLASS_T const *pclass;
    
    OS_ASSERT((cls == s_dcb.cls[cls].pclass->type), RETURN_FALSE);
    
    /* 主文件 */
    pclass = s_dcb.cls[cls].pclass;
    preg   = pclass->preg;
    for (j = 0; j < pclass->nreg; j++) {
        OS_ASSERT((preg != 0), RETURN_FALSE);
            
        id = preg->id;
        offset = s_dcb.pp[id].offset;
        phead  = (PP_HEAD_T *)(pclass->memptr + offset);
        OS_ASSERT((*(pclass->memptr + offset + s_dcb.pp[id].space - 1) == BOUND_FLAG), RETURN_FALSE);
        
        if ((s_dcb.pp[id].attrib & CL_) == CL_) {
            OS_ASSERT((s_dcb.pp[id].valid != VALID_), RETURN_FALSE);
            OS_ASSERT((phead->chksum[0] == 0 && phead->chksum[1] == 0), RETURN_FALSE);
             
            result = HAL_SFLASH_Write(PP_ADDR_MAIN + offset, pclass->memptr + offset, s_dcb.pp[id].space);
            OS_ASSERT((result != 0), RETURN_FALSE);
        } else if ((s_dcb.pp[id].attrib & WF_) == WF_) {
            OS_ASSERT((s_dcb.pp[id].valid == VALID_), RETURN_FALSE);
            OS_ASSERT(CheckParaValid((INT8U *)phead, s_dcb.pp[id].space), RETURN_FALSE);
                
            result = HAL_SFLASH_Write(PP_ADDR_MAIN + offset, pclass->memptr + offset, s_dcb.pp[id].space);
            OS_ASSERT((result != 0), RETURN_FALSE);
        }
        preg++;
    }
    
    return true;
}
    

/*******************************************************************
** 函数名:     UpdateTmrProc
** 函数描述:   延时更新定时器函数
** 参数:       [in]  pdata:  保留未用
** 返回:       无
********************************************************************/
static void UpdateTmrProc(void *pdata)
{
    INT8U i, nclass;
    PP_CLASS_T const *pclass;
     
    OS_StartTmr(s_updatetmr, PERIOD_UPDATE);
    
    if (s_dcb.lock == PP_LOCK) {
        return;
    }
    
    nclass = DAL_PP_GetRegClassMax();
    OS_ASSERT((nclass <= MAX_PP_CLASS_NUM), RETURN_VOID);
    for (i = 0; i < nclass; i++) {
        pclass = DAL_PP_GetRegClassInfo(i);
        OS_ASSERT((pclass->dly != 0), RETURN_VOID);
        if (s_dcb.cls[i].ct_delay < pclass->dly) {
            if (++s_dcb.cls[i].ct_delay == pclass->dly) {
                if (!FlushRamToFile(i)) {
                    return;
                }
            }
        } else {
            /*if (i == PP_TYPE_DELAY) {
                s_dcb.cls[i].ct_delay = 0;
            }*/
        }
    }
}

/*******************************************************************
** 函数名:     ResetInform_PP
** 函数描述:   复位回调接口
** 参数:       [in] resettype：复位类型
**             [in] filename： 文件名
**             [in] line：     出错行号
**             [in] errid：    错误ID
** 返回:       无
********************************************************************/
static void ResetInform_PP(INT8U resettype, char *filename, INT32U line)
{
    INT8U i, nclass;
    
    if (s_dcb.lock == PP_LOCK) {
        return;
    }
        
    nclass = DAL_PP_GetRegClassMax();
    OS_ASSERT((nclass <= MAX_PP_CLASS_NUM), RETURN_VOID);
    for (i = 0; i < nclass; i++) {
        if (!FlushRamToFile(i)) {
            return;
        }
    }
}

/***************************************************************
*   函数名:    DiagnoseProc
*   功能描述:  诊断函数
*　 参数:      无
*   返回值:    无
***************************************************************/
static void DiagnoseProc(void)
{
    OS_ASSERT(OS_TmrIsRun(s_updatetmr), RETURN_VOID);
}

/*******************************************************************
** 函数名:      DAL_PP_InitDrv
** 函数描述:    公共参数存储驱动初始化
** 参数:        无
** 返回:        无
********************************************************************/
void DAL_PP_InitDrv(void)
{
    #if DEBUG_PP >  0
    printf_com("<DAL_PP_InitDrv>\r\n");
    #endif
    
    YX_MEMSET(&s_dcb, 0, sizeof(s_dcb));
    
    s_dcb.lock = PP_LOCK;
    if (!InitPubPara()) {
        return;
    }
    s_dcb.lock = 0;
    
    s_updatetmr = OS_CreateTmr(TSK_ID_DAL, 0, UpdateTmrProc);
    OS_StartTmr(s_updatetmr, PERIOD_UPDATE);
    
    OS_RegistResetInform(RESET_PRI_1, ResetInform_PP);
    OS_RegistDiagnoseProc(DiagnoseProc);
}

/*******************************************************************
** 函数名:     DAL_PP_DelParaByClass
** 函数描述:   删除指定参数区的参数
** 参数:       [in] cls：参数区,见PP_CLASS_ID_E
** 返回:       成功返回true，失败返回false
********************************************************************/
//#define DAL_PP_DelParaByClass(cls)    PP_DelParaByClass(0x55, cls)
BOOLEAN PP_DelParaByClass(INT8U flag, INT8U cls)
{
    BOOLEAN result;
    INT8U i, id;
    PP_HEAD_T *phead;
    PP_REG_T const *preg;
    PP_CLASS_T const *pclass;
    
    OS_ASSERT((flag == 0x55), RETURN_FALSE);
    OS_ASSERT((cls < DAL_PP_GetRegClassMax()), RETURN_FALSE);
    
    pclass = s_dcb.cls[cls].pclass;
    OS_ASSERT((s_dcb.cls[cls].filesize == pclass->memlen), RETURN_FALSE);
    
    preg   = pclass->preg;
    for (i = 0; i < pclass->nreg; i++) {
        id = preg->id;
        OS_ASSERT((s_dcb.pp[id].space == sizeof(PP_HEAD_T) + preg->rec_size + 1), RETURN_FALSE);
        
        phead  = (PP_HEAD_T *)(pclass->memptr + s_dcb.pp[id].offset);
        phead->chksum[0] = 0;
        phead->chksum[1] = 0;
    
        s_dcb.pp[id].valid  = 0;
        s_dcb.pp[id].attrib = 0;
        
        preg++;
    }
    /* 更新主文件 */
    OS_ASSERT((flag == 0x55), RETURN_FALSE);
    result = HAL_SFLASH_Write(PP_ADDR_MAIN, pclass->memptr, pclass->memlen);
    OS_ASSERT((result != 0), RETURN_FALSE);
    
    /* 更新备份文件 */
    
    //s_dcb.lock = PP_LOCK;
    OS_PostMsg(TSK_ID_DAL, MSG_DAL_PP_CHANGE, cls | 0xff00, PP_REASON_RESET);
    return true;
}

/*******************************************************************
** 函数名:     DAL_PP_DelAllPara
** 函数描述:   删除所有参数区的参数,程序可以继续正常运行
** 参数:       无
** 返回:       成功返回true，失败返回false
********************************************************************/
//#define DAL_PP_DelAllPara()    PP_DelAllPara(0x55)
BOOLEAN PP_DelAllPara(INT8U flag)
{
    INT8U i, nclass;
    
    OS_ASSERT((flag == 0x55), RETURN_FALSE);
    nclass = DAL_PP_GetRegClassMax();
    OS_ASSERT((nclass <= MAX_PP_CLASS_NUM), RETURN_FALSE);
    
    for (i = 0; i < nclass; i++) {
        if (!DAL_PP_DelParaByClass(i)) {
            return false;
        }
    }
    return true;
}

/*******************************************************************
** 函数名:     DAL_PP_DelAllParaAndRst
** 函数描述:   删除所有参数区的参数,必须复位重启设备后才能继续存储
** 参数:       无
** 返回:       无
********************************************************************/
//#define DAL_PP_DelAllParaAndRst()    PP_DelAllParaAndRst(0x55)
void PP_DelAllParaAndRst(INT8U flag)
{
    PP_DelAllPara(flag);
    s_dcb.lock = PP_LOCK;
}

/*******************************************************************
** 函数名:      DAL_PP_ClearParaByID
** 函数描述:    清除PP参数
** 参数:        [in] id:  参数编号，见PP_ID_E
** 返回:        有效返回true，无效返回false
********************************************************************/
BOOLEAN DAL_PP_ClearParaByID(INT8U id)
{
    PP_HEAD_T *phead;
    PP_CLASS_T const *pclass;
       
    if (s_dcb.lock == PP_LOCK) {
        return FALSE;
    }
    
    OS_ASSERT((id < DAL_PP_GetRegPPMax()), RETURN_FALSE);
    OS_ASSERT((id == s_dcb.pp[id].preg->id), RETURN_FALSE);
    
    pclass = DAL_PP_GetRegClassInfo(s_dcb.pp[id].preg->type);
    phead  = (PP_HEAD_T *)(pclass->memptr + s_dcb.pp[id].offset);
    
    OS_ASSERT((*(pclass->memptr + s_dcb.pp[id].offset + s_dcb.pp[id].space - 1) == BOUND_FLAG), RETURN_FALSE);
    
    YX_MEMSET(((INT8U *)phead), 0, s_dcb.pp[id].space - 1);
    s_dcb.pp[id].valid   = 0;
    s_dcb.pp[id].attrib |= CL_;
    
    s_dcb.cls[pclass->type].ct_delay = 0;
    OS_PostMsg(TSK_ID_DAL, MSG_DAL_PP_CHANGE, id, PP_REASON_STORE);
    return TRUE;
}

/*******************************************************************
** 函数名:      DAL_PP_GetParaPtr
** 函数描述:    读取PP参数指针(只能按字节访问)
** 参数:        [in] id:  参数编号，见PP_ID_E
**              [in] dptr:输出缓存
**              [in] rlen:缓存长度
** 返回:        有效返回true，无效返回false
********************************************************************/
INT8U *DAL_PP_GetParaPtr(INT8U id)
{
    PP_HEAD_T *phead;
    PP_CLASS_T const *pclass;
    
    pclass = DAL_PP_GetRegClassInfo(s_dcb.pp[id].preg->type);
    phead  = (PP_HEAD_T *)(pclass->memptr + s_dcb.pp[id].offset);
   
    if (s_dcb.pp[id].valid == VALID_) {
        if (CheckParaValid((INT8U *)phead, s_dcb.pp[id].space)) {
            return ((INT8U *)phead) + sizeof(PP_HEAD_T);
        } else {
            OS_ASSERT(0, RETURN_FALSE);
        }
    }
    
    if (s_dcb.pp[id].preg->i_ptr != 0) {
        return (INT8U *)s_dcb.pp[id].preg->i_ptr;
    }
    
    return ((INT8U *)phead) + sizeof(PP_HEAD_T);
}

/*******************************************************************
** 函数名:      DAL_PP_ReadParaByID
** 函数描述:    读取PP参数，判断PP有效性
** 参数:        [in] id:  参数编号，见PP_ID_E
**              [in] dptr:输出缓存
**              [in] rlen:缓存长度
** 返回:        有效返回true，无效返回false
********************************************************************/
BOOLEAN DAL_PP_ReadParaByID(INT8U id, INT8U *dptr, INT16U rlen)
{
    PP_HEAD_T *phead;
    PP_CLASS_T const *pclass;
    
    OS_ASSERT((id < DAL_PP_GetRegPPMax()), RETURN_FALSE);
    OS_ASSERT((rlen >= s_dcb.pp[id].preg->rec_size), RETURN_FALSE);
    OS_ASSERT((id == s_dcb.pp[id].preg->id), RETURN_FALSE);
    
    pclass = DAL_PP_GetRegClassInfo(s_dcb.pp[id].preg->type);
    phead  = (PP_HEAD_T *)(pclass->memptr + s_dcb.pp[id].offset);
    
    OS_ASSERT((*(pclass->memptr + s_dcb.pp[id].offset + s_dcb.pp[id].space - 1) == BOUND_FLAG), RETURN_FALSE);
   
    if (s_dcb.pp[id].valid == VALID_) {
        if (CheckParaValid((INT8U *)phead, s_dcb.pp[id].space)) {
            YX_MEMCPY(dptr, s_dcb.pp[id].preg->rec_size, ((INT8U *)phead) + sizeof(PP_HEAD_T), s_dcb.pp[id].preg->rec_size);
            return TRUE;
        } else {
            OS_ASSERT(0, RETURN_FALSE);
        }
    }
    if (s_dcb.pp[id].preg->i_ptr != 0) {
        YX_MEMCPY(dptr, s_dcb.pp[id].preg->rec_size, s_dcb.pp[id].preg->i_ptr, s_dcb.pp[id].preg->rec_size);
        return TRUE;
    }
    YX_MEMSET(dptr, 0, s_dcb.pp[id].preg->rec_size);
    return FALSE;
}

/*******************************************************************
** 函数名:      DAL_PP_StoreParaByID
** 函数描述:    存储PP参数，延时2秒存储到flash
** 参数:        [in] id:  参数编号，见PP_ID_E
**              [in] sptr:输入缓存
**              [in] slen:缓存长度
** 返回:        成功返回true，失败返回false
********************************************************************/
BOOLEAN DAL_PP_StoreParaByID(INT8U id, INT8U *sptr, INT16U slen)
{
    PP_HEAD_T *phead;
    PP_CLASS_T const *pclass;
       
    if (s_dcb.lock == PP_LOCK) {
        return FALSE;
    }
    
    OS_ASSERT((id < DAL_PP_GetRegPPMax()), RETURN_FALSE);
    OS_ASSERT((slen == s_dcb.pp[id].preg->rec_size), RETURN_FALSE);
    OS_ASSERT((id == s_dcb.pp[id].preg->id), RETURN_FALSE);
    
    pclass = DAL_PP_GetRegClassInfo(s_dcb.pp[id].preg->type);
    phead  = (PP_HEAD_T *)(pclass->memptr + s_dcb.pp[id].offset);
    
    OS_ASSERT((pclass->type == s_dcb.pp[id].preg->type), RETURN_FALSE);
    OS_ASSERT((*(pclass->memptr + s_dcb.pp[id].offset + s_dcb.pp[id].space - 1) == BOUND_FLAG), RETURN_FALSE);
    
    YX_MEMCPY(((INT8U *)phead) + sizeof(PP_HEAD_T), slen, sptr, slen);
    phead->chksum[0] = YX_GetChkSum_N(((INT8U *)phead) + sizeof(PP_HEAD_T), slen);
    phead->chksum[1] = YX_ChkSum_XOR(((INT8U *)phead) + sizeof(PP_HEAD_T), slen);
    
    s_dcb.pp[id].valid   = VALID_;
    s_dcb.pp[id].attrib |= WF_;
    
    //if (pclass->type != PP_TYPE_DELAY) {
        s_dcb.cls[pclass->type].ct_delay = 0;
        OS_PostMsg(TSK_ID_DAL, MSG_DAL_PP_CHANGE, id, PP_REASON_STORE);
    //}
    
    return TRUE;
}

/*******************************************************************
** 函数名:      DAL_PP_StoreParaInstantByID
** 函数描述:    存储PP参数,立即存储到flash中
** 参数:        [in] id:  参数编号，见PP_ID_E
**              [in] sptr:输入缓存
**              [in] slen:缓存长度
** 返回:        成功返回true，失败返回false
********************************************************************/
BOOLEAN DAL_PP_StoreParaInstantByID(INT8U id, INT8U *sptr, INT16U slen)
{
    BOOLEAN result;
    
    result = DAL_PP_StoreParaByID(id, sptr, slen);
    if (result) {
        if (FlushRamToFile(s_dcb.pp[id].preg->type)) {
            s_dcb.cls[s_dcb.pp[id].preg->type].ct_delay = 0;
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

/*******************************************************************
** 函数名:      DAL_PP_CheckParaValidByID
** 函数描述:    判断PP参数有效性
** 参数:        [in] id:  参数编号，见PP_ID_E
** 返回:        有效返回true，无效返回false
********************************************************************/
BOOLEAN DAL_PP_CheckParaValidByID(INT8U id)
{
    PP_HEAD_T *phead;
    PP_CLASS_T const *pclass;
    
    OS_ASSERT((id < DAL_PP_GetRegPPMax()), RETURN_FALSE);
    OS_ASSERT((id == s_dcb.pp[id].preg->id), RETURN_FALSE);
    
    pclass = DAL_PP_GetRegClassInfo(s_dcb.pp[id].preg->type);
    phead  = (PP_HEAD_T *)(pclass->memptr + s_dcb.pp[id].offset);
    
    OS_ASSERT((*(pclass->memptr + s_dcb.pp[id].offset + s_dcb.pp[id].space - 1) == BOUND_FLAG), RETURN_FALSE);
    
    if (s_dcb.pp[id].valid == VALID_) {
        if (CheckParaValid((INT8U *)phead, s_dcb.pp[id].space)) {
            return TRUE;
        } else {
            OS_ASSERT(0, RETURN_FALSE);
        }
    }
    if (s_dcb.pp[id].preg->i_ptr != 0) {
        return TRUE;
    }
    return FALSE;      
}

/*******************************************************************
** 函数名:      DAL_PP_GetParaByID
** 函数描述:    获取PP参数，不判断PP参数的有效性，直接获取
** 参数:        [in] id:  参数编号，见PP_ID_E
**              [in] dptr:输出缓存
**              [in] rlen:缓存长度
** 返回:        成功返回true，失败返回false
********************************************************************/
BOOLEAN DAL_PP_GetParaByID(INT8U id, INT8U *dptr, INT16U rlen)
{
    PP_HEAD_T *phead;
    PP_CLASS_T const *pclass;
    
    OS_ASSERT((id < DAL_PP_GetRegPPMax()), RETURN_FALSE);
    OS_ASSERT((rlen >= s_dcb.pp[id].preg->rec_size), RETURN_FALSE);
    OS_ASSERT((id == s_dcb.pp[id].preg->id), RETURN_FALSE);
    
    pclass = DAL_PP_GetRegClassInfo(s_dcb.pp[id].preg->type);
    phead  = (PP_HEAD_T *)(pclass->memptr + s_dcb.pp[id].offset);
    
    OS_ASSERT((*(pclass->memptr + s_dcb.pp[id].offset + s_dcb.pp[id].space - 1) == BOUND_FLAG), RETURN_FALSE);
    
    YX_MEMCPY(dptr, s_dcb.pp[id].preg->rec_size, ((INT8U *)phead) + sizeof(PP_HEAD_T), s_dcb.pp[id].preg->rec_size);
    return TRUE;        
}

/*******************************************************************
** 函数名:      DAL_PP_RegParaChangeInformer
** 函数描述:    注册参数变化通知函数的函数
** 参数:        [in] id:  参数编号，见PP_ID_E
**              [in] fp:  回调函数
** 返回:        成功返回true，失败返回false
********************************************************************/
BOOLEAN DAL_PP_RegParaChangeInformer(INT8U id, void (*fp)(INT8U reason))
{
    INT8U i;
    
    OS_ASSERT((id < DAL_PP_GetRegPPMax()), RETURN_FALSE);
    OS_ASSERT((id == s_dcb.pp[id].preg->id), RETURN_FALSE);
       
    for (i = 0; i < MAX_PP_INFORM; i++) {
        if (s_dcb.inform[i].informer == 0) {
            break;
        }
    }
    OS_ASSERT((i < MAX_PP_INFORM), RETURN_FALSE);
    
    s_dcb.inform[i].id       = id;
    s_dcb.inform[i].informer = fp;
    
    return TRUE;
}

/*******************************************************************
** 函数名:      DAL_PP_InformParaChangeByID
** 函数描述:    参数变化通知处理
** 参数:        [in] id:      参数编号，见PP_ID_E
**              [in] reason:  变化原因
** 返回:        无
********************************************************************/
void DAL_PP_InformParaChangeByID(INT16U id, INT8U reason)
{
    INT8U i, j, cls, maxid;
    PP_REG_T const *preg;
    PP_CLASS_T const *pclass;
    
    maxid = DAL_PP_GetRegPPMax();
    if (id > 0xff) {
        cls = id & 0xff;
        OS_ASSERT((cls < DAL_PP_GetRegClassMax()), RETURN_VOID);
        
        pclass = s_dcb.cls[cls].pclass;
        preg   = pclass->preg;
        for (j = 0; j < pclass->nreg; j++) {
            id = preg->id;
            for (i = 0; i < MAX_PP_INFORM; i++) {
                if (s_dcb.inform[i].informer != 0) {
                    OS_ASSERT((s_dcb.inform[i].id < maxid), RETURN_VOID);
                    if (id == s_dcb.inform[i].id) {
                         (*s_dcb.inform[i].informer)(reason);
                    }
                }
            }
            i = id - s_dcb.cls[cls].pclass->preg->id;
            preg++;
            
            #if DEBUG_PP >  0
            printf_com("<DAL_PP_InformParaChangeByID,cls(%d),id(%d),offset id:(%d),reason:(%d)>\r\n", cls, id, i, reason);
            #endif
        }
    } else {
        OS_ASSERT((id < maxid), RETURN_VOID);
        OS_ASSERT((id == s_dcb.pp[id].preg->id), RETURN_VOID);
        cls = s_dcb.pp[id].preg->type;
        
        for (i = 0; i < MAX_PP_INFORM; i++) {
            if (s_dcb.inform[i].informer != 0) {
                OS_ASSERT((s_dcb.inform[i].id < maxid), RETURN_VOID);
                if (id == s_dcb.inform[i].id) {
                    (*s_dcb.inform[i].informer)(reason);
                }
            }
        }
        i = id - s_dcb.cls[cls].pclass->preg->id;
        
        #if DEBUG_PP >  0
        printf_com("<DAL_PP_InformParaChangeByID,cls(%d),id(%d),offset id:(%d),reason:(%d)>\r\n", cls, id, i, reason);
        #endif
    }
}

