/********************************************************************************
**
** 文件名:     yx_ic_drv.h
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   该模块主要实现IC卡刷卡功能管理
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2014/04/09 | 叶德焰 |  创建第一版本
*********************************************************************************/
#ifndef DAL_IC_DRV_H
#define DAL_IC_DRV_H          1

#if EN_ICCARD > 0

/* IC卡状态 */
typedef enum {
    IC_STATUS_INIT,
    IC_STATUS_SUCC,                    /* IC卡已读取成功 */
    IC_STATUS_ERROR,                   /* IC卡已插入,读取失败 */
    IC_STATUS_NOCARD,                  /* IC卡已拔出 */
    IC_STATUS_MAX
} IC_STATUS_E;

/* IC卡类型 */
typedef enum {
    IC_TYPE_UNKNOW,                    /* IC卡未知型号 */
    IC_TYPE_SLE4442,                   /* IC卡已读取成功 */
    IC_TYPE_24CXX,                     /* IC卡已插入,读取失败 */
    IC_TYPE_MAX
} IC_TYPE_E;

/* 司机信息配置 */
#define DRIVER_NAME_LEN       20       /* 驾驶员姓名最大长度 */
#define IDENTITY_LEN          18       /* 驾驶员身份证最大长度 */
#define QUALIFICATION_LEN     18       /* 从业资格证号码最大长度 */
#define INSTITUTION_LEN       20       /* 从业资格证发证机构名称最大长度 */
#define LICENSE_LEN           18       /* 驾驶证号码最大长度 */
#define DRIVERID_LEN          8        /* 司机代码最大长度 */
#define PASSWORD_LEN          8        /* 密码长度 */

typedef struct{
#if EN_ICCARD_PASER > 0
    INT8U status;                           /* IC卡状态,IC_STATUS_E */
    INT8U type;                             /* IC卡类型,见IC_TYPE_E */
    
    INT8U passwordlen;                      /* 登入密码长度 */
    INT8U password[PASSWORD_LEN];           /* 登入密码 */
    
    INT8U namelen;                          /* 驾驶员姓名长度 */
    INT8U name[DRIVER_NAME_LEN];            /* 驾驶员姓名 */
    
    INT8U driveridlen;                      /* 司机代码长度 */
    INT8U driverid[DRIVERID_LEN];           /* 司机代码 */
    
    INT8U identitylen;                      /* 身份证编码长度 */
    INT8U identity[IDENTITY_LEN];           /* 身份证编码 */
    
    INT8U qualificationlen;                 /* 从业资格证号码长度 */
    INT8U qualification[QUALIFICATION_LEN]; /* 从业资格证号码 */
    
    INT8U driverlicenselen;                 /* 驾驶证号码长度 */
    INT8U driverlicense[LICENSE_LEN];       /* 驾驶证号码 */
    
    INT8U institutionlen;
    INT8U institution[INSTITUTION_LEN];     /* 从业资格证发证机构名称，以"/0"为结束符 */
    
    DATE_T date;                            /* 驾驶证有效期，年月日，BCD码 */
#else
    INT8U status;                           /* IC卡状态,IC_STATUS_E */
    INT8U type;                             /* IC卡类型,见IC_TYPE_E */
#endif
} DRIVER_T;





/*******************************************************************
** 函数名:     DAL_IC_InitDrv
** 函数描述:   模块初始化
** 参数:       无
** 返回:       无
********************************************************************/
void DAL_IC_InitDrv(void);

/*******************************************************************
** 函数名:     DAL_IC_RegistIccardProc
** 函数描述:   注册IC卡事件通知函数,最大可注册个数为 MAX_REG_ICCARD
** 参数:       [in] handler: 通知器
** 返回:       成功返回TRUE, 失败返回FALSE
********************************************************************/
BOOLEAN DAL_IC_RegistIccardProc(void (*handler)(DRIVER_T *info, INT8U *data, INT32U datalen));

/*******************************************************************
** 函数名:     DAL_IC_GetDriverInfo
** 函数描述:   获取驾驶员刷卡信息
** 参数:       无
** 返回:       成功返回IC卡信息
********************************************************************/
DRIVER_T *DAL_IC_GetDriverInfo(void);

/*******************************************************************
** 函数名:     DAL_IC_WriteData
** 函数描述:   写卡数据
** 参数:       [in] offset: 偏移地址
**             [in] sptr：  数据指针
**             [in] slen:   数据长度
** 返回:       写入数据长度
********************************************************************/
INT32U DAL_IC_WriteData(INT32U offset, INT8U *sptr, INT16U slen);

/*******************************************************************
** 函数名:     DAL_IC_RereadIccardData
** 函数描述:   重新读取IC卡中的数据
** 参数:       无
** 返回:       无
********************************************************************/
void DAL_IC_RereadIccardData(void);

/*******************************************************************
** 函数名:     DAL_IC_GetData
** 函数描述:   获取IC卡原始数据
** 参数:       无
** 返回:       成功返回数据指针
********************************************************************/
INT8U *DAL_IC_GetData(void);

#endif
#endif
/************************** (C) COPYRIGHT 2010 XIAMEN YAXON.LTD *******************END OF FILE******/


