/**************************************************************************************************
**                                                                                               **
**  文件名称:  configurepg.h                                                                     **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2010                                    **
**  创建信息:  jump -- 2011年1月20日                                                             **
**  文件描述:  系统参数页面                                                                      **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/

#ifndef DEF_CONFIGURE
#define DEF_CONFIGURE

#ifdef  CONFIGURE_GLOBALS
#define CONFIGURE_EXT
#else
#define CONFIGURE_EXT extern
#endif

#define  MAXMYTELLEN     20

typedef enum {
    YU_NET_PASSWORD,            /* 网络参数 */
    YU_PROJECTMENU_PASSWORD,    /* 工程菜单 */
}PSTYPE_E;// 密码类型

typedef enum
{
    ORIENTED_GPS = 0x01,
    ORIENTED_BEIDOU = 0x02,
    ORIENTED_MIX = 0x03,
    ORIENTED_MAX
} SATORIENTED_E;


typedef enum {
  GPS_NO_STATU,                                                                /* 都未连接*/
  GPS_1_STATU,                                                                 /* GPS1已联 */
  GPS_2_STATU,                                                                 /* GPS2已联 */
  GPS_12_STATU,                                                                /* GPS1与GPS2已联 */
}GPS_STATE_E;

typedef struct {
    INT8U tfnumlen;                      /* 特服号长度 */
    INT8U tfnum[MAXMYTELLEN];
    INT8U olstatus;                      /* 建立网络 */
    INT8U gprstatus;                     /* GPRS在线状态 */
    INT8U situate;                       /* 定位状态 */
    INT8U almstatus[4];                  /* 报警状态 */
    INT8U phonestatus;                   /* 呼入呼出限制状态 */
}PARA_STRUCT;

typedef enum
{
    SERVICE_ONE = 0x01,
    SERVICE_TWO = 0x02,
    SERVICE_MAX
} REGSERVICE_E;



CONFIGURE_EXT PSTYPE_E g_passwordtype;
CONFIGURE_EXT PARA_STRUCT  ParaStatus;

INT8U GetGPSStatus(void);


/**************************************************************************************************
**  函数名称:  Getwifistatus
**  功能描述:  获取WIFI状态
**  输入参数:
**  返回参数:	wifi状态
**************************************************************************************************/
INT8U Getwifistatus(void);

/**************************************************************************************************
**  函数名称:  SetGpsLocation
**  功能描述:  设置GPS位置信息
**  输入参数:
**  返回参数:
**************************************************************************************************/
void SetGpsLocation(INT8U *locationdata);



#endif

