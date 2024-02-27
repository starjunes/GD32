/********************************************************************************
**
** 文件名:     dal_gsen_drv.c
** 版权所有:   (c) 1998-2017 厦门雅迅网络股份有限公司
** 文件描述:   g-sensor、gyro驱动，提供加速度、角速度数据查询及参数标定等接口
**
*********************************************************************************
**                  修改历史记录
**===============================================================================
**| 日期              | 作者        |  修改记录
**===============================================================================
**| 2017/05/27 | 黄运峰    |  创建第一版本
**| 2019/12/18 | 陈益民    |  添加ICM20648传感器驱动
********************************************************************************/
#include "dal_pinlist.h"
#include "dal_gpio.h"
#include "dal_simi2c_2.h"
#include  "dal_simi2c.h"
#include "man_timer.h"
#include "dal_exti.h"
#include "man_queue.h"
#include "port_msghdl.h"
#include "port_gpio.h"
#include "dal_gsen_drv.h"
#include "Dal_Include.h"
#include "os_reg.h"
//#define hal_i2c_ReadByte    dal_i2c_sim_ReadByte
//#define hal_i2c_WriteByte   dal_i2c_sim_WriteByte

/* 使能低功耗模式功能(只用于ICM20648) */
#define EN_GS_LOW_POWER_MODE 1

/*
*********************************************************************************
*                   定义模块数据类型、变量及宏
*********************************************************************************
*/
#define EN_DEBUG_GSENTMR    0
#if EN_DEBUG_GSEN == 0
#undef EN_DEBUG_GSENTMR
#define EN_DEBUG_GSENTMR    0
#endif

#if EN_DEBUG_GSEN > 1
#undef EN_DEBUG_GSEN
#define EN_DEBUG_GSEN 1
#endif

#define  I2C_IDX_0   0
#define  I2C_IDX_1   1
#define  _MILTICK    _TICK
#define configTICK_RATE_HZ  100
#define MSG_MAIN_APPEVNT    1

// I2C回调函数指针类型
typedef void (*I2C_CALBAK_PTR)(INT8U idx, BOOLEAN result);

// I2C读写操作参数结构体
// calbak：若为NULL，表示采用同步调用模式，函数返回成功即可得到结果；
//         若不为NULL，表示采用回调模式，要等驱动回调成功才可得到结果。
typedef struct {
    INT16U  devaddr;
    BOOLEAN is10bit;
    INT16U  addr;
    INT8U  *buf;
    INT16U  dlen;
    I2C_CALBAK_PTR calbak;
} I2C_OPPARA_T;

/* 六轴传感器从机地址 */
#define I2C_DEV_ADDR         0xD0
#define I2C_ADDR             I2C_IDX_0, I2C_DEV_ADDR, FALSE

/* SD2058传感器从机地址 */
#define I2C_COM_SD2058		 I2C_IDX_1
#define SD2058_ADDR         0x64


/* 传感器类型 */   
#define SENSER_MPU6500       1
#define SENSER_ICM20648      1

/********************************* MPU6500寄存器 *********************************/
#if (1 == SENSER_MPU6500)

/* MPU6500设备ID */
#define MPU_DEF_DEVICE_ID       0x70    // MPU6500's ID
#define MPU_DEVICE_INFOR_STR    "MPU-6500"

/* 震动唤醒状态位 */
#define MPU_WOM_INTSTAT_MASK    0x40

/* MPU6500寄存器地址 */
#define MPU_REG_SMPLRT_DIV      25      // 正常工作模式采样频率分频
#define MPU_REG_DLPF_CFG        26      // GYRO和TEMP低通滤波参数、FIFO数据覆盖方式
#define MPU_REG_GYRO_CFG        27      // GYRO参数，量程、FCHOICE_B
#define MPU_REG_ACCL_CFG        28      // ACCL参数配置1，量程,
#define MPU_REG_ACCL_CFG2       29      // ACCL参数配置2，FCHOICE_B、DLPF
#define MPU_REG_LPODR_CTRL      30      // 低功耗模式下的数据输出频率
#define MPU_REG_WOM_THRSHD      31      // 振动唤醒加速度阀值
#define MPU_REG_INTPARA_CFG     55      // 中断参数配置，有效电平、脉冲方式、状态清除方式等
#define MPU_REG_INTEN_CFG       56      // 中断使能设置
#define MPU_REG_INT_STAT        58      // 中断状态寄存器

#define MPU_REG_ACCL_XOUT_H     59      // ACCL数据输出：X轴H字节
#define MPU_REG_ACCL_XOUT_L     60      // ACCL数据输出：X轴L字节
#define MPU_REG_ACCL_YOUT_H     61      // ACCL数据输出：Y轴H字节
#define MPU_REG_ACCL_YOUT_L     62      // ACCL数据输出：Y轴L字节
#define MPU_REG_ACCL_ZOUT_H     63      // ACCL数据输出：Z轴H字节
#define MPU_REG_ACCL_ZOUT_L     64      // ACCL数据输出：Z轴L字节

#define MPU_REG_ACCL_XOFS_H     119     // ACCL偏移寄存器：X轴H字节
#define MPU_REG_ACCL_XOFS_L     120     // ACCL偏移寄存器：X轴L字节
#define MPU_REG_ACCL_YOFS_H     122     // ACCL偏移寄存器：Y轴H字节
#define MPU_REG_ACCL_YOFS_L     123     // ACCL偏移寄存器：Y轴L字节
#define MPU_REG_ACCL_ZOFS_H     125     // ACCL偏移寄存器：Z轴H字节
#define MPU_REG_ACCL_ZOFS_L     126     // ACCL偏移寄存器：Z轴L字节

#define MPU_REG_TEMP_OUT_H      65      // TEMP数据输出：H字节
#define MPU_REG_TEMP_OUT_L      66      // TEMP数据输出：L字节

#define MPU_REG_GYRO_XOUT_H     67      // GYRO数据输出：X轴H字节
#define MPU_REG_GYRO_XOUT_L     68      // GYRO数据输出：X轴L字节
#define MPU_REG_GYRO_YOUT_H     69      // GYRO数据输出：Y轴H字节
#define MPU_REG_GYRO_YOUT_L     70      // GYRO数据输出：Y轴L字节
#define MPU_REG_GYRO_ZOUT_H     71      // GYRO数据输出：Z轴H字节
#define MPU_REG_GYRO_ZOUT_L     72      // GYRO数据输出：Z轴L字节

#define MPU_REG_GYRO_XOFS_H     19      // GYRO偏移寄存器：X轴H字节
#define MPU_REG_GYRO_XOFS_L     20      // GYRO偏移寄存器：X轴L字节
#define MPU_REG_GYRO_YOFS_H     21      // GYRO偏移寄存器：X轴H字节
#define MPU_REG_GYRO_YOFS_L     22      // GYRO偏移寄存器：X轴L字节
#define MPU_REG_GYRO_ZOFS_H     23      // GYRO偏移寄存器：Z轴H字节
#define MPU_REG_GYRO_ZOFS_L     24      // GYRO偏移寄存器：Z轴L字节

#define MPU_REG_SIGPATH_RST     104     // 传感信号通道复位
#define MPU_REG_ACCL_INTCTL     105     // ACCL中断配置，WOM检测逻辑使能
#define MPU_REG_USER_CTRL       106     // 用户参数配置，各种使能和重置功能
#define MPU_REG_PWR_MGR1        107     // 功耗模式管理寄存器1
#define MPU_REG_PWR_MGR2        108     // 功耗模式管理寄存器2

#define MPU_REG_DEVICE_ID       117     // 设备ID，WHO AM I

#endif

/********************************* ICM20648寄存器 *********************************/

#if (1 == SENSER_ICM20648)

/* ICM20648设备ID */
#define ICM_DEF_DEVICE_ID       0xe0    // ICM20648's ID
#define ICM_DEVICE_INFOR_STR    "ICM20648"

/* 震动唤醒状态位 */
#define ICM_WOM_INTSTAT_MASK    0x08

/* 寄存器bank区值 */
#define BANK0 0x00
#define BANK1 0x10
#define BANK2 0x20
#define BANK3 0x30

/* 六轴传感器寄存器地址 */

/* bank区选择寄存器 */
#define REG_BANK_SEL 0x7F

/*************************Bank0************************** */
#define ICM_REG_WHO_AM_I 0x00

#define ICM_REG_USER_CTL 0x03

#define ICM_REG_LP_CONFIG 0x05

#define ICM_REG_POW_MGMT_1 0x06
#define ICM_REG_POW_MGMT_2 0x07

#define ICM_REG_INT_PIN_CFG 0x0f

#define ICM_REG_INT_ENABLE 0x10
#define ICM_REG_INT_ENABLE_1 0x11
#define ICM_REG_INT_ENABLE_2 0x12
#define ICM_REG_INT_ENABLE_3 0x13

#define ICM_REG_I2C_MST_STATUS 0x17

#define ICM_REG_INT_STATUS 0x19
#define ICM_REG_INT_STATUS_1 0x1a
#define ICM_REG_INT_STATUS_2 0x1b
#define ICM_REG_INT_STATUS_3 0x1c

#define ICM_REG_DELAY_TIME_H 0x28
#define ICM_REG_DELAY_TIME_L 0x29

#define ICM_REG_ACCEL_XOUT_H 0x2d
#define ICM_REG_ACCEL_XOUT_L 0x2e

#define ICM_REG_ACCEL_YOUT_H 0x2f
#define ICM_REG_ACCEL_YOUT_L 0x30

#define ICM_REG_ACCEL_ZOUT_H 0x31
#define ICM_REG_ACCEL_ZOUT_L 0x32

#define ICM_REG_GYRO_XOUT_H 0x33
#define ICM_REG_GYRO_XOUT_L 0x34

#define ICM_REG_GYRO_YOUT_H 0x35
#define ICM_REG_GYRO_YOUT_L 0x36

#define ICM_REG_GYRO_ZOUT_H 0x37
#define ICM_REG_GYRO_ZOUT_L 0x38

#define ICM_REG_TEMP_OUT_H 0x39
#define ICM_REG_TEMP_OUT_L 0x3a

/* 地址从0x3b~0x52 */
#define ICM_REG_EXT_SLV_SENS_DATA_00 0x3b
#define ICM_REG_EXT_SLV_SENS_DATA_23 0x52


#define ICM_REG_FIFO_EN_1 0x66
#define ICM_REG_FIFO_EN_2 0x67

#define ICM_REG_FIFO_RST 0x68
#define ICM_REG_FIFO_MODE 0x69
#define ICM_REG_FIFO_COUNT_H 0x70
#define ICM_REG_FIFO_COUNT_L 0x71
#define ICM_REG_FIFO_R_W 0x72

#define ICM_REG_DATA_RDY_STATUS 0x74
#define ICM_REG_FIFO_CFG 0x76

/*************************Bank1***************************/
#define ICM_REG_SELF_TEST_X_GYRO 0x02
#define ICM_REG_SELF_TEST_Y_GYRO 0x03
#define ICM_REG_SELF_TEST_Z_GYRO 0x04

#define ICM_REG_SELF_TEST_X_ACCEL 0x0e
#define ICM_REG_SELF_TEST_Y_ACCEL 0x0f
#define ICM_REG_SELF_TEST_Z_ACCEL 0x10

#define ICM_REG_XA_OFFS_H 0x14
#define ICM_REG_XA_OFFS_L 0x15

#define ICM_REG_YA_OFFS_H 0x17
#define ICM_REG_YA_OFFS_L 0x18

#define ICM_REG_ZA_OFFS_H 0x1a
#define ICM_REG_ZA_OFFS_L 0x1b

#define ICM_REG_TIMEBASE_CORRECTION_PLL 0x28

/*************************Bank2***************************/
#define ICM_REG_GYRO_SMPLR_DIV 0x00
 
#define ICM_REG_GYRO_CONFIG_1 0x01
#define ICM_REG_GYRO_CONFIG_2 0x02
 
#define ICM_REG_XG_OFFS_USR_H 0x03
#define ICM_REG_XG_OFFS_USR_L 0x04
 
#define ICM_REG_YG_OFFS_USR_H 0x05
#define ICM_REG_YG_OFFS_USR_L 0x06
 
#define ICM_REG_ZG_OFFS_USR_H 0x07
#define ICM_REG_ZG_OFFS_USR_L 0x08
 
#define ICM_REG_ODR_ALIGN_EN 0x09
 
#define ICM_REG_ACCEL_SMPLRT_DIV1 0x10
#define ICM_REG_ACCEL_SMPLRT_DIV2 0x11
 
#define ICM_REG_ACCEL_INTEL_CTRL 0x12
 
#define ICM_REG_ACCEL_WOM_THR 0x13
 
#define ICM_REG_ACCEL_CONFIG 0x14
#define ICM_REG_ACCEL_CONFIG_2 0x15
 
#define ICM_REG_FSYNC_CONFIG 0x52
 
#define ICM_REG_TEMP_CONFIG 0x53
 
#define ICM_REG_MOD_CTRL_USR 0x54

/*************************Bank3***************************/
#define ICM_REG_I2C_MST_ODR_CONFIG 0x00

#define ICM_REG_I2C_MST_CTRL 0x01

#define ICM_REG_I2C_MST_DELAY_CTRL 0x02

#define ICM_REG_I2C_SLV0_ADDR 0x03

#define ICM_REG_I2C_SLV0_REG 0x04

#define ICM_REG_I2C_SLV0_CTRL 0x05

#define ICM_REG_I2C_SLV0_DO 0x06

#define ICM_REG_I2C_SLV1_ADDR 0x07

#define ICM_REG_I2C_SLV1_REG 0x08

#define ICM_REG_I2C_SLV1_CTRL 0x09

#define ICM_REG_I2C_SLV1_DO 0x0a

#define ICM_REG_I2C_SLV2_ADDR 0x0b

#define ICM_REG_I2C_SLV2_REG 0x0c

#define ICM_REG_I2C_SLV2_CTRL 0x0d

#define ICM_REG_I2C_SLV2_DO 0x0e

#define ICM_REG_I2C_SLV3_ADDR 0x0f

#define ICM_REG_I2C_SLV3_REG 0x10

#define ICM_REG_I2C_SLV3_CTRL 0x11

#define ICM_REG_I2C_SLV3_DO 0x12

#define ICM_REG_I2C_SLV4_ADDR 0x13

#define ICM_REG_I2C_SLV4_REG 0x14

#define ICM_REG_I2C_SLV4_CTRL 0x15

#define ICM_REG_I2C_SLV4_DO 0x16

#define ICM_REG_I2C_SLV4_DI 0x17

#endif

// 定义g-sensor模块状态
#define STAT_INIT           0x01    // 模块初始化
#define STAT_HIT_THRS       0x02    // 已设置碰撞阀值
#define STAT_TEMP_EN        0x04    // 使能温度采集
#define STAT_CALIB          0x08    // 启动标定
#define STAT_LOWPWR         0x10    // 进入休眠模式
#define STAT_SET_GYRO       0x20
#define STAT_RESET          0x40    // 复位gsensor
#define STAT_REINIT         0x80    // 重新初始化

#define GSEN_SCAN_PRIOD_MS  10L
#if (GSEN_SCAN_PRIOD_MS * configTICK_RATE_HZ % 1000) != 0
#error "GSEN_SCAN_PRIOD_MS must be multiples of 10!"
#endif
#define GSEN_SCAN_PERIOD    _MILTICK, configTICK_RATE_HZ * GSEN_SCAN_PRIOD_MS / 1000//_MILTICK, 1
#define GSEN_SWITCH_TIME   100L/GSEN_SCAN_PRIOD_MS        //重启gsensor下电和上电时间 100ms

#define GSEN_CALIB_COUNT    100             // 标定平滑次数
#define HALF_CALIB_COUNT    GSEN_CALIB_COUNT / 2
#define ACCL_CAL_RATIO      1000.0f/4096    // 单位转换为mg
#define GYRO_CAL_RATIO      1000.0f/65.5    // 单位转换为1/1000 °/s
#define TEMP_CAL_RATIO      10.0f/333.87    // 单位转换为1/100 ℃

#define MAX_THRS_REGVAL     1020
#define MAX_BUSY_CNT        1000
// 可读取的数据枚举类型
typedef enum {
    TYPE_ACCL,
    TYPE_GYRO,
    TYPE_TEMP,
    MAX_TYPE
} GSEN_DATA_TYPE_E;

// 读取ACCL或GYRO数据控制结构体类型
typedef struct {
    BOOLEAN isbusy;         // 是否忙标志，同一时刻只能处理一类读取操作
    INT8U   rtype;          // 读取类型
    INT8U   hitreq;         // 中断触发读取请求标志
    INT8U   step;           // 读取步骤
    WORD_UNION rword;       // 读取半字联合体，用与保存读取到的高低字节数据
    void   *pdata;          // 读取数据最终保存的数据结构体指针，取决于读取的发起者
    I2C_OPPARA_T rpara;     // I2C读取参数
} GSEN_RCB_T;

// 坐标轴数据结构体类型,INT16S
typedef struct {
    INT16S axis_x;
    INT16S axis_y;
    INT16S axis_z;
} AXIS_DATA16_T;

#define FLAG_INT_RD    0x01
#define FLAG_INT_RDREQ   0x02

// 读取ACCL或GYRO数据步骤枚举类型
typedef enum {
    STEP_X_H,
    STEP_X_L,
    STEP_Y_H,
    STEP_Y_L,
    STEP_Z_H,
    STEP_Z_L,
    MAX_STEP
} GSEN_RSTETP_E;
//extern BOOLEAN debug_printf_dir(const char * fmt, ...);


typedef struct{
INT32U msgid;
 INT32U lpara;
 INT32U hpara;
}Q_MSG_GSENSER;

static void ReadSensorDataCalbak(INT8U idx, BOOLEAN result);

#ifdef HAL_PIO_DEF
#undef HAL_PIO_DEF
#endif
#define HAL_PIO_DEF(_PIO_ID_, _PORT_ID_, _PIN_POS_, _PIO_DIR_, _DEF_VALUE_, _INT_EN_, _INT_MODE_, _HANDLER_)  \
                             _PIO_ID_,
typedef enum {
    #include "hal_pio_drv.def"
    PIO_NUMMAX
} PIO_NUM_E;
                             
static BOOLEAN hal_pio_SetPinIntMode(PIO_NUM_E pid, BOOLEAN enable)
{
    return PORT_SetPinIntMode((INT32U)pid, enable);
}

static BOOLEAN hal_i2c_WriteByte(INT8U idx, INT8U devaddr, BOOLEAN is10bit, INT16U wraddr, INT8U wrbyte)
{
    return dal_i2c_sim_WriteByte(idx, devaddr, (INT8U)wraddr, wrbyte);
}

 static BOOLEAN hal_i2c_WriteData(INT8U idx, I2C_OPPARA_T *op_para)
 {
	 BOOLEAN ret;

    I2C_SIM_OPPARA_T simop_para;

    if(op_para->buf == NULL) return false;

    simop_para.addr     = op_para->addr;
    simop_para.buf      = op_para->buf;
    simop_para.devaddr  = op_para->devaddr;
    simop_para.dlen     = op_para->dlen;
    
    ret = dal_i2c_sim_WriteData(idx, &simop_para);
	return ret;
 }


 INT16S hal_i2c_ReadByte(INT8U idx, INT16U devaddr, BOOLEAN is10bit, INT16U readaddr)
{
    INT8U rd_buf;

    if(dal_i2c_sim_ReadByte(idx, devaddr, readaddr,&rd_buf)){
        return rd_buf;    //成功 
    }else{
        return -1;
    }
}
static BOOLEAN hal_i2c_ReadData(INT8U idx, I2C_OPPARA_T *op_para)
{

	BOOLEAN ret;

    I2C_SIM_OPPARA_T simop_para;

    if(op_para->buf == NULL) return false;

    simop_para.addr     = op_para->addr;
    simop_para.buf      = op_para->buf;
    simop_para.devaddr  = op_para->devaddr;
    simop_para.dlen     = op_para->dlen;
    
    ret = dal_i2c_sim_ReadData(idx, &simop_para);



	#if EN_DEBUG_GSEN > 0
		//debug_printf_dir("\r\n< op_para->addr:%x  op_para->devaddr:%x  ret: %x  >\r\n",op_para->addr,op_para->devaddr,ret);
	#endif	


	

	ReadSensorDataCalbak(idx,ret);

	return ret;
}

static void GSEN_OP_DELAY(INT16U dly)
 {
     INT16U i,j;
     for (i = 0, j = 0; i < dly; i++) {
         while(j++ < 3000) __asm__ volatile("NOP");
         j = 0;
     }
 }

/*
*********************************************************************************
*                   本地变量
*********************************************************************************
*/

static INT8U s_status = 0, s_rtype;
static INT16U s_ct_calib;
static GSEN_CALIB_CALBAK_PTR s_calib_calbak_fp;
static AXIS_DATA16_T s_accl_hitdata, s_accl_data, s_accl_tmpdata;
static AXIS_DATA16_T s_gyro_data, s_gyro_tmpdata;
static AXIS_DATA_T s_accl_calib, s_gyro_calib;
static INT16S s_temper_data, s_temper_tmpdata;      // 温度数据
static INT32U s_scan_tmr;
static INT16U s_hit_threshold;                      // 单位:mg，<=1020mg
static BOOLEAN s_hitdata_valid, s_accldata_valid, s_gyrodata_valid, s_tempdata_valid;
static BOOLEAN s_detect_int;
static GSEN_RCB_T s_rcb;
static INT16U s_ct_busy;
static INT8U s_delcyc_cout=0;
#if EN_DEBUG_GSENTMR > 0
static BOOLEAN s_gsen_tmr_running;
#endif

/* 设备类型: */
#define DEV_NONE        0x00  /* 错误设备类型 */
#define DEV_MPU_6500   0x01
#define DEV_ICM_20648  0x02
static INT8U s_dev_type = DEV_NONE;

/*
*********************************************************************************
*                   本地接口实现
*********************************************************************************
*/
static void StartReadSensorData(INT8U rtype, BOOLEAN hitreq);
static BOOLEAN GsenInit(void);
static BOOLEAN GsenReInit(void);

/********************************************************************************
** 函数名:     DeviceTypeCheck
** 函数描述:   设备类型判断
** 参数:       [out] dev_type: 输出设备类型
** 返回:       TRUE:执行成功，FALSE:执行失败    
********************************************************************************/
static BOOLEAN DeviceTypeCheck(INT8U* dev_type)
{
    INT8U i;
    INT16S rch1,rch2;
    
    for(i = 0; i < 10; i++){
        /* 读取设备ID */
        rch1 = hal_i2c_ReadByte(I2C_ADDR, 0x00);
        GSEN_OP_DELAY(5);
        
        if (rch1 == -1) {
            #if EN_DEBUG_GSEN > 0
            debug_printf_dir("\r\n<**** (1)读设备类型失败 ****>\r\n");
            #endif   
            continue;
        }
        #if EN_DEBUG_GSEN > 0
        debug_printf_dir("\r\n<**** (1)DEV:0x%x ****>\r\n",rch1);
        #endif  
        
        rch2 = hal_i2c_ReadByte(I2C_ADDR, 0x75);
        GSEN_OP_DELAY(5);
        
        if (rch2 == -1) {
            #if EN_DEBUG_GSEN > 0
            debug_printf_dir("\r\n<**** (2)读设备类型失败 ****>\r\n");
            #endif 
            continue;
        }   

        #if EN_DEBUG_GSEN > 0
        debug_printf_dir("\r\n<**** (2)DEV:0x%x ****>\r\n",rch2);
        #endif 
        
        if(rch1 == 0xe0){
            if(rch2 != 0x70){
                dev_type[0] = DEV_ICM_20648; //s_dev_type = DEV_ICM_20648;
                #if EN_DEBUG_GSEN > 0
                debug_printf_dir("\r\n<**** 设备类型:ICM_20648 ****>\r\n");
                #endif
                return TRUE;
            }else{
                dev_type[0] = DEV_NONE;
                return FALSE;
            }
        }
        
        if(rch2 == 0x70){
            if(rch1 != 0xe0){
                dev_type[0] = DEV_MPU_6500; // s_dev_type = DEV_MPU_6500;
                #if EN_DEBUG_GSEN > 0
                debug_printf_dir("\r\n<**** 设备类型:MPU_6500 ****>\r\n");
                #endif  
                return TRUE;
            }else{
                dev_type[0] = DEV_NONE;
                return FALSE;
            }
        }  
    }
    dev_type[0] = DEV_NONE;
    return FALSE;
}

/********************************************************************************
** 函数名:     SelectBankNum
** 函数描述:   bank区选择
** 参数:       [in] bank_n:bank区号
** 返回:       TRUE:执行成功，FALSE:执行失败    
********************************************************************************/
static BOOLEAN SelectBankNum(INT8U bank_n)
{
    BOOLEAN rt;
    INT16S rch;
    if(bank_n > BANK3) return FALSE;
    
    rch = hal_i2c_ReadByte(I2C_ADDR, REG_BANK_SEL);
    if (rch == -1) {       
    	#if EN_DEBUG_GSEN > 0
    	debug_printf_dir("\r\n<***** (0)SelectBankNum() Failed *****>\r\n");
    	#endif            
        return FALSE;
    }
    GSEN_OP_DELAY(5);   

    if( (rch &0x30) != bank_n ){
        rch &= 0xce;
        rch |= bank_n;
        rt = hal_i2c_WriteByte(I2C_ADDR, REG_BANK_SEL, rch);
        if(rt == FALSE){
            #if EN_DEBUG_GSEN > 0
            debug_printf_dir("\r\n<***** (1)SelectBankNum() Failed *****>\r\n");
            #endif        
            return FALSE;
        }

    }else{
        #if EN_DEBUG_GSEN > 0
        debug_printf_dir("\r\n<***** bank无需切换:0x%x *****>\r\n",bank_n);
        #endif       
    }   
    return TRUE;
}


/********************************************************************************
**  函数名称:  gsen_enable_temp
**  功能描述:  使能温度采集
**  输入参数: 无
**            
**  返回参数:  TRUE，设置成功；FALSE，设置失败
********************************************************************************/
static inline BOOLEAN gsen_enable_temp(void)
{
    INT16S rch;
    BOOLEAN ret;
	#if EN_DEBUG_GSEN > 0
	debug_printf_dir("\r\n<*****gsen_enable_temp()*****>\r\n");
	#endif
    if(s_dev_type == DEV_MPU_6500){
        rch = hal_i2c_ReadByte(I2C_ADDR, MPU_REG_PWR_MGR1);
        /* 使能temp */
        rch &= ~0x08;       
        ret = hal_i2c_WriteByte(I2C_ADDR, MPU_REG_PWR_MGR1, rch&0xff);
        GSEN_OP_DELAY(5);       
    }else if(s_dev_type == DEV_ICM_20648){
        rch = hal_i2c_ReadByte(I2C_ADDR, ICM_REG_POW_MGMT_1);
        /* 使能temp */
        rch &= ~0x08;       
        ret = hal_i2c_WriteByte(I2C_ADDR, ICM_REG_POW_MGMT_1, rch&0xff);
        GSEN_OP_DELAY(5);      
    }else{
        #if EN_DEBUG_GSEN > 0
        debug_printf_dir("\r\n<**** 无传感器 ****>\r\n");
        #endif
        return FALSE;
    }  
    
    return ret;
}

/********************************************************************************
**  函数名称:  gsen_reset_device
**  功能描述:  复位设备，恢复所有寄存器到默认值
**  输入参数: 无
**            
**  返回参数:  TRUE，设置成功；FALSE，设置失败
********************************************************************************/
static inline BOOLEAN gsen_reset_device(void)
{
    INT16S rch, i;
    BOOLEAN ret;
 	#if EN_DEBUG_GSEN > 0
	debug_printf_dir("\r\n<*****gsen_reset_device()*****>\r\n");
	#endif

    if(s_dev_type == DEV_MPU_6500){
        /* 复位device：重置内部寄存器并恢复默认设置 */
        ret = hal_i2c_WriteByte(I2C_ADDR, MPU_REG_PWR_MGR1, 0x80);
        GSEN_OP_DELAY(5);
        if(ret == FALSE) return FALSE;
        
        for (i = 0; i < 0x1000; i++) {
            /* 读取是否设置成功 */
            rch = hal_i2c_ReadByte(I2C_ADDR, MPU_REG_PWR_MGR1);
            if (rch == -1) return FALSE;
            if((rch & 0x80) == 0) break;    // 设备复位完毕
        }
        
        /* 复位gyro,accel,temp信号路径，寄存器值未清除，寄存器复位通过: SIG_COND_RST{在REG_USER_CTRL[0]} */
        ret = hal_i2c_WriteByte(I2C_ADDR, MPU_REG_SIGPATH_RST, 0x07);
        GSEN_OP_DELAY(5);      
        if(ret == FALSE) return FALSE;
    }else if(s_dev_type == DEV_ICM_20648){

        /* 先复位该寄存器，退出默认的低功耗使能 */
        ret = hal_i2c_WriteByte(I2C_ADDR, ICM_REG_POW_MGMT_1, 0x00);
        GSEN_OP_DELAY(10);
        if(ret == FALSE) return FALSE;
        
        /* 复位device：重置内部寄存器并恢复默认设置 */
        ret = hal_i2c_WriteByte(I2C_ADDR, ICM_REG_POW_MGMT_1, 0x80);
        GSEN_OP_DELAY(5);
        if(ret == FALSE) return FALSE;
            
        for (i = 0; i < 0x1000; i++) {
            /* 读取是否设置成功 */
            rch = hal_i2c_ReadByte(I2C_ADDR, ICM_REG_POW_MGMT_1);
            if (rch == -1) return FALSE;
            if((rch & 0x80) == 0) break;    // 设备复位完毕
        }
        GSEN_OP_DELAY(5);

        /* 低功耗模式下duty_cycled：i2c_master、accel、gryo*/
        ret = hal_i2c_WriteByte(I2C_ADDR, ICM_REG_LP_CONFIG,0x00);
        GSEN_OP_DELAY(5);
        if(ret == FALSE) return FALSE;
        
    #if 1
        /* 复位DMP，SRAM,I2C_MST */
        ret = hal_i2c_WriteByte(I2C_ADDR, ICM_REG_USER_CTL, 0x0e);
        GSEN_OP_DELAY(5);    
        if(ret == FALSE) return FALSE;
    #endif

        /* 设备复位后为sleep模式，清除sleep，唤醒device */
        ret = hal_i2c_WriteByte(I2C_ADDR, ICM_REG_POW_MGMT_1, 0x00);
        GSEN_OP_DELAY(10);
        if(ret == FALSE) return FALSE;

    #if 1
        /* 在BANK2区设置 */
        ret = SelectBankNum(BANK2);
        GSEN_OP_DELAY(5);
        if(ret == FALSE) return FALSE;

        /* 关闭DMP模式 */
        ret = hal_i2c_WriteByte(I2C_ADDR, ICM_REG_MOD_CTRL_USR, 0x00);
        GSEN_OP_DELAY(5);
        if(ret == FALSE) return FALSE;
        
        /* 在BANK0区设置 */
        ret = SelectBankNum(BANK0);
        GSEN_OP_DELAY(5);
        if(ret == FALSE) return FALSE;
    #endif
      
    }else{
        #if EN_DEBUG_GSEN > 0
        debug_printf_dir("\r\n<**** 无传感器 ****>\r\n");
        #endif
        return FALSE;
    }     
    return TRUE;
}

/********************************************************************************
**  函数名称:  gsen_enter_sleep
**  功能描述:  设置进入睡眠模式
**  输入参数: 无
**  
**  返回参数: 无
********************************************************************************/
static inline void gsen_enter_sleep(void)
{
    INT16U i;
    
    #if EN_DEBUG_GSEN > 0
    debug_printf_dir("\r\n<*****gsen_enter_sleep()*****>\r\n");
    #endif

    for (i = 0; i < 0x1000; i++) {
        if (s_rcb.isbusy == FALSE) break;
    }
    
    /* 关闭MCU INT中断 */
    hal_pio_SetPinIntMode(PIO_GYRINT, FALSE);               
    GSEN_OP_DELAY(5);
    
    if(s_dev_type == DEV_MPU_6500){
        /* 禁用ACCL和GYRO六轴 */
        hal_i2c_WriteByte(I2C_ADDR, MPU_REG_PWR_MGR2, 0x3F);        
        GSEN_OP_DELAY(5);
        
        /* 关闭WOM检测逻辑 */
        hal_i2c_WriteByte(I2C_ADDR, MPU_REG_ACCL_INTCTL,0x0);       
        GSEN_OP_DELAY(5);

        /* 禁用芯片所有中断 */
        hal_i2c_WriteByte(I2C_ADDR, MPU_REG_INTEN_CFG, 0x0);        
        GSEN_OP_DELAY(5);

        /* sleep=1,cycle=0,temp_dis=1 */
        hal_i2c_WriteByte(I2C_ADDR, MPU_REG_PWR_MGR1, 0x48);        
        GSEN_OP_DELAY(5);      
    }else if(s_dev_type == DEV_ICM_20648){
        /* 禁用ACCL和GYRO六轴 */
        hal_i2c_WriteByte(I2C_ADDR, ICM_REG_POW_MGMT_2, 0x3F);        
        GSEN_OP_DELAY(5);

        /* 在BANK2区设置 */
        SelectBankNum(BANK2);
        GSEN_OP_DELAY(5);
        
        /* 关闭WOM检测逻辑 */
        hal_i2c_WriteByte(I2C_ADDR, ICM_REG_ACCEL_INTEL_CTRL,0x0);       
        GSEN_OP_DELAY(5);

        /* 在BANK0区设置 */
        SelectBankNum(BANK0);
        GSEN_OP_DELAY(5);
        
        /* 禁用芯片所有中断 */
        hal_i2c_WriteByte(I2C_ADDR, ICM_REG_INT_ENABLE, 0x0);        
        GSEN_OP_DELAY(5);

        hal_i2c_WriteByte(I2C_ADDR, ICM_REG_INT_ENABLE_1, 0x0);        
        GSEN_OP_DELAY(5);
        
        hal_i2c_WriteByte(I2C_ADDR, ICM_REG_INT_ENABLE_2, 0x0);        
        GSEN_OP_DELAY(5);    
        
        hal_i2c_WriteByte(I2C_ADDR, ICM_REG_INT_ENABLE_3, 0x0);        
        GSEN_OP_DELAY(5);    

        /* sleep=1,cycle=0,temp_dis=1 */
        hal_i2c_WriteByte(I2C_ADDR, ICM_REG_POW_MGMT_1, 0x48);        
        GSEN_OP_DELAY(5);    
    }else{
        #if EN_DEBUG_GSEN > 0
        debug_printf_dir("\r\n<**** 无传感器 ****>\r\n");
        #endif
    }   
}

/********************************************************************************
**  函数名称:  gsen_enter_sleep
**  功能描述:  设置进入低功耗模式
**  输入参数: 无
**  返回参数: 无
********************************************************************************/
static inline void gsen_enter_lowpwr(void)
{
    INT16U i;
    #if EN_DEBUG_GSEN > 0
    debug_printf_dir("\r\n<*****gsen_enter_lowpwr()*****>\r\n");
    #endif

    for (i = 0; i < 0x1000; i++) {
        if (s_rcb.isbusy == FALSE) break;
    }
    /* 关闭MCU中断 */
    hal_pio_SetPinIntMode(PIO_GYRINT, FALSE);
    if(s_dev_type == DEV_MPU_6500){
        /* ACCL三轴使能, GYRO三轴禁用 */
        hal_i2c_WriteByte(I2C_ADDR, MPU_REG_PWR_MGR2, 0x07);        
        GSEN_OP_DELAY(5);

        /* 先关闭WOM检测逻辑 */
        hal_i2c_WriteByte(I2C_ADDR, MPU_REG_ACCL_INTCTL,0x40);      
        GSEN_OP_DELAY(5);

        /* 设置accel的x,y,z轴唤醒阈值 */
        /* 振动唤醒阀值,lsb = 4mg;100*4 = 400mg */
        hal_i2c_WriteByte(I2C_ADDR, MPU_REG_WOM_THRSHD, 100);        
        GSEN_OP_DELAY(5);

        /* 使能WOM检测逻辑，ACCEL_INTEL_EN=1，ACCEL_INTEL_MODE=1 */
        hal_i2c_WriteByte(I2C_ADDR, MPU_REG_ACCL_INTCTL,0xC0);      
        GSEN_OP_DELAY(5);

        /* 读一次，清原有中断标志，读取时，传感器自动清除状态 */
        hal_i2c_ReadByte(I2C_ADDR, MPU_REG_INT_STAT);                
        GSEN_OP_DELAY(5);

        hal_i2c_WriteByte(I2C_ADDR, MPU_REG_INTEN_CFG, MPU_WOM_INTSTAT_MASK);   // 使能WOM中断
        GSEN_OP_DELAY(5);
        
        /* sleep=0,cycle=1,temp_dis=1 */
        hal_i2c_WriteByte(I2C_ADDR, MPU_REG_PWR_MGR1, 0x28); 
        GSEN_OP_DELAY(5);
    
    }else if(s_dev_type == DEV_ICM_20648){
         /* ACCL三轴使能, GYRO三轴禁用 */
        hal_i2c_WriteByte(I2C_ADDR, ICM_REG_POW_MGMT_2, 0x07);        
        GSEN_OP_DELAY(5);

        /* 在BANK2区设置 */
        SelectBankNum(BANK2);
        GSEN_OP_DELAY(5);

        /* 先关闭WOM检测逻辑,ACCEL_INTEL_EN = 0,ACCEL_INTEL_MODE_INT=1 */
        hal_i2c_WriteByte(I2C_ADDR, ICM_REG_ACCEL_INTEL_CTRL,0x01);      
        GSEN_OP_DELAY(5);

        /* 设置accel的x,y,z轴唤醒阈值 */
        /* 振动唤醒阀值,lsb = 4mg;100 *4 = 400mg */
        hal_i2c_WriteByte(I2C_ADDR, ICM_REG_ACCEL_WOM_THR, 100);        
        GSEN_OP_DELAY(5);

        /* 使能WOM检测逻辑，ACCEL_INTEL_EN=1，ACCEL_INTEL_MODE=1 */
        hal_i2c_WriteByte(I2C_ADDR, ICM_REG_ACCEL_INTEL_CTRL,0x03);      
        GSEN_OP_DELAY(5);
        
        /* 在BANK0区设置 */
        SelectBankNum(BANK0);
        GSEN_OP_DELAY(5);

    #if EN_GS_LOW_POWER_MODE > 0
        /* 低功耗模式下：i2c_master = 0 、accel = 1、gryo = 0.(duty_cycled)*/
        hal_i2c_WriteByte(I2C_ADDR, ICM_REG_LP_CONFIG,0x20); //0x00
        GSEN_OP_DELAY(5);
    #else
        hal_i2c_WriteByte(I2C_ADDR, ICM_REG_LP_CONFIG,0x00); //0x00
        GSEN_OP_DELAY(5);        
    #endif
        
    #if EN_GS_LOW_POWER_MODE > 0
        /* 在BANK2区设置 */
        SelectBankNum(BANK2);
        GSEN_OP_DELAY(5);

        #if 0
        /* 打开DMP模式 */
        hal_i2c_WriteByte(I2C_ADDR, ICM_REG_MOD_CTRL_USR, 0x00);
        GSEN_OP_DELAY(5);
        #endif

        /* ACCL量程±8g,ACCEL_FCHOICE = 0,ACCEL_DLPFCFG = 2;BW:1209HZ,NBW:1248 HZ */
        hal_i2c_WriteByte(I2C_ADDR, ICM_REG_ACCEL_CONFIG, 0x14); //0x15     
        GSEN_OP_DELAY(5);

        /* 低功耗模式下（duty cycled）控制加速度计抽取器中的平均样本数：1~4 sample */
        hal_i2c_WriteByte(I2C_ADDR, ICM_REG_ACCEL_CONFIG_2, 0); // 1  
        GSEN_OP_DELAY(5);

        /* accel设置采样频率:102hz;ODR=1125/(127+1)=8.8HZ(主要是为了减低功耗) */
        hal_i2c_WriteByte(I2C_ADDR, ICM_REG_ACCEL_SMPLRT_DIV1, 0);
        GSEN_OP_DELAY(5);

        hal_i2c_WriteByte(I2C_ADDR, ICM_REG_ACCEL_SMPLRT_DIV2, 63);// 127
        GSEN_OP_DELAY(5);
    
        /* 在BANK0区设置 */
        SelectBankNum(BANK0);
        GSEN_OP_DELAY(5);  
    #endif

        /* sleep=0,cycle=1,temp_dis=1 */
        hal_i2c_WriteByte(I2C_ADDR, ICM_REG_POW_MGMT_1, 0x29); //0x29
        GSEN_OP_DELAY(5); 

        /* 读一次，清原有中断标志，读取后，传感器自动清除状态 */
        hal_i2c_ReadByte(I2C_ADDR, ICM_REG_INT_STATUS);                
        GSEN_OP_DELAY(5);
        hal_i2c_ReadByte(I2C_ADDR, ICM_REG_INT_STATUS_1);                
        GSEN_OP_DELAY(5);
        hal_i2c_ReadByte(I2C_ADDR, ICM_REG_INT_STATUS_2);                
        GSEN_OP_DELAY(5);
        hal_i2c_ReadByte(I2C_ADDR, ICM_REG_INT_STATUS_3);                
        GSEN_OP_DELAY(5);
        
        hal_i2c_WriteByte(I2C_ADDR, ICM_REG_INT_ENABLE, ICM_WOM_INTSTAT_MASK);   // 使能WOM中断
        GSEN_OP_DELAY(5);

        
    }else{
        #if EN_DEBUG_GSEN > 0
        debug_printf_dir("\r\n<**** 无传感器 ****>\r\n");
        #endif
    }  
    #if EN_DAL_GSEN_WK > 0
    /* 打开MCU中断 */
    hal_pio_SetPinIntMode(PIO_GYRINT, TRUE);
    #endif
    #if EN_DEBUG_GSENTMR > 0
    s_gsen_tmr_running = FALSE;
    #endif
}

// 注：因模块问题，休眠后唤醒gyro数据就会异常，需要复位芯片，理论上只要退出低功耗模式即可
/********************************************************************************
**  函数名称:  gsen_exit_lowpwr
**  功能描述:  设置退出低功耗模式
**  输入参数: 无
**  返回参数: 无
********************************************************************************/
static inline void gsen_exit_lowpwr(void)
{
    #if EN_DEBUG_GSEN > 0
    debug_printf_dir("\r\n<***** gsen_exit_lowpwr() *****>\r\n");
    #endif

    s_status |= STAT_REINIT;
    s_status &= ~STAT_INIT;
    s_status &= ~STAT_LOWPWR;
    //GsenReInit();
}

// 将gyro标定的偏移量值写入芯片寄存器
#if 0   // 芯片问题，自动修正无法使用，只好软件自行修正
static inline BOOLEAN WriteGyroCalibPara(void)
{
    BOOLEAN ret;
    WORD_UNION tmpword;

    tmpword.hword = (INT16U)(-s_gyro_calib.axis_x); // 作为偏移量需取反才可设置到寄存器
    ret = hal_i2c_WriteByte(I2C_ADDR, REG_GYRO_XOFS_H, tmpword.bytes.high);
    if (ret == FALSE) return FALSE;
    GSEN_OP_DELAY(5);
    ret = hal_i2c_WriteByte(I2C_ADDR, REG_GYRO_XOFS_L, tmpword.bytes.low);
    if (ret == FALSE) return FALSE;
    GSEN_OP_DELAY(5);
    tmpword.hword = (INT16U)(-s_gyro_calib.axis_y);
    ret = hal_i2c_WriteByte(I2C_ADDR, REG_GYRO_YOFS_H, tmpword.bytes.high);
    if (ret == FALSE) return FALSE;
    GSEN_OP_DELAY(5);
    ret = hal_i2c_WriteByte(I2C_ADDR, REG_GYRO_YOFS_L, tmpword.bytes.low);
    if (ret == FALSE) return FALSE;
    GSEN_OP_DELAY(5);
    tmpword.hword = (INT16U)(-s_gyro_calib.axis_z);
    ret = hal_i2c_WriteByte(I2C_ADDR, REG_GYRO_ZOFS_H, tmpword.bytes.high);
    if (ret == FALSE) return FALSE;
    GSEN_OP_DELAY(5);
    ret = hal_i2c_WriteByte(I2C_ADDR, REG_GYRO_ZOFS_L, tmpword.bytes.low);
    if (ret == FALSE) return FALSE;
    GSEN_OP_DELAY(5);

    return TRUE;
}
#endif

/********************************************************************************
**  函数名称:  GsenReInit
**  功能描述:  重新初始化配置g-sensor模块，主要用于从低功耗模式恢复或模块通信异常后重置恢复
**  输入参数: 无
**  返回参数: TRUE，设置成功；FALSE，设置失败
********************************************************************************/
static BOOLEAN GsenReInit(void)
{
    BOOLEAN ret;
    INT16U threshold;
    
    #if EN_DEBUG_GSEN > 0
	debug_printf_dir("\r\n<*****GsenReInit()*****>\r\n");
    #endif

    ret = GsenInit();
    if (ret == FALSE) return FALSE;
    if (s_status & STAT_TEMP_EN) {
        ret = gsen_enable_temp();
        if (ret == FALSE) return FALSE;
        GSEN_OP_DELAY(5);
    }
    
    if(s_dev_type == DEV_MPU_6500){
        if ((s_status & STAT_HIT_THRS) != 0) {
            threshold = (INT16U)(s_hit_threshold * ACCL_CAL_RATIO / 4);
            threshold = threshold > 255 ? 255 : threshold;

            /* 设置震动唤醒阈值 */
            ret = hal_i2c_WriteByte(I2C_ADDR, MPU_REG_WOM_THRSHD, (INT8U)(threshold));
            if (ret == FALSE) return FALSE;
            GSEN_OP_DELAY(5);
            
            /* 使能WOM检测逻辑 */
            ret = hal_i2c_WriteByte(I2C_ADDR, MPU_REG_ACCL_INTCTL,0xC0);
            if (ret == FALSE) return FALSE;
            GSEN_OP_DELAY(5);
            
            /* 使能WOM中断 */
            ret = hal_i2c_WriteByte(I2C_ADDR, MPU_REG_INTEN_CFG, MPU_WOM_INTSTAT_MASK);   
            if (ret == FALSE) return FALSE;
            GSEN_OP_DELAY(5);

            #if EN_DEBUG_GSEN > 0
            debug_printf_dir("\r\n<*****使能WOM中断(0),阈值:0x%x*****>\r\n",(INT8U)(threshold));
            #endif           
            /* 设置MCU中断脚 */
            hal_pio_SetPinIntMode(PIO_GYRINT, TRUE);
        } else {
            /* 关闭中断脚 */
            hal_pio_SetPinIntMode(PIO_GYRINT, FALSE);

            /* 禁用芯片所有中断 */
            hal_i2c_WriteByte(I2C_ADDR, MPU_REG_INTEN_CFG, 0);          
            if (ret == FALSE) return FALSE;
            GSEN_OP_DELAY(5);
            
            /* 关闭WOM检测逻辑 */
            hal_i2c_WriteByte(I2C_ADDR, MPU_REG_ACCL_INTCTL,0); 
            if (ret == FALSE) return FALSE;
            GSEN_OP_DELAY(5);
        }     
    }else if(s_dev_type == DEV_ICM_20648){
        if ((s_status & STAT_HIT_THRS) != 0) {
            threshold = (INT16U)(s_hit_threshold * ACCL_CAL_RATIO / 4);
            threshold = threshold > 255 ? 255 : threshold;
            
            /* 在BANK2区设置 */
            SelectBankNum(BANK2);
            GSEN_OP_DELAY(5);

            /* 设置震动唤醒阈值 */
            ret = hal_i2c_WriteByte(I2C_ADDR, ICM_REG_ACCEL_WOM_THR, (INT8U)(threshold));
            if (ret == FALSE) {goto ERR;}
            GSEN_OP_DELAY(5);
            
            /* 使能WOM检测逻辑 */
            ret = hal_i2c_WriteByte(I2C_ADDR, ICM_REG_ACCEL_INTEL_CTRL,0x03);
            if (ret == FALSE) {goto ERR;}
            GSEN_OP_DELAY(5);
            
            /* 在BANK0区设置 */
            SelectBankNum(BANK0);
            GSEN_OP_DELAY(5); 
            
            /* 使能WOM中断 */
            ret = hal_i2c_WriteByte(I2C_ADDR, ICM_REG_INT_ENABLE, ICM_WOM_INTSTAT_MASK);   
            if (ret == FALSE) {goto ERR;}
            GSEN_OP_DELAY(5);

            /* 读一次，清原有中断标志，读取后，传感器自动清除状态 */
            hal_i2c_ReadByte(I2C_ADDR, ICM_REG_INT_STATUS);                
            GSEN_OP_DELAY(5);
            
            #if EN_DEBUG_GSEN > 0
            debug_printf_dir("\r\n<*****使能WOM中断(1),阈值:0x%x*****>\r\n",(INT8U)(threshold));
            #endif

            /* 设置MCU中断脚 */
            hal_pio_SetPinIntMode(PIO_GYRINT, TRUE);
        } else {
            /* 关闭中断脚 */
            hal_pio_SetPinIntMode(PIO_GYRINT, FALSE);

            /* 在BANK0区设置 */
            SelectBankNum(BANK0);
            GSEN_OP_DELAY(5); 

            /* 禁用芯片所有中断 */
            hal_i2c_WriteByte(I2C_ADDR, ICM_REG_INT_ENABLE, 0);          
            if (ret == FALSE) {goto ERR;}
            GSEN_OP_DELAY(5);
            
            /* 在BANK2区设置 */
            SelectBankNum(BANK2);
            GSEN_OP_DELAY(5);      
            
            /* 关闭WOM检测逻辑 */
            hal_i2c_WriteByte(I2C_ADDR, ICM_REG_ACCEL_INTEL_CTRL,0); 
            if (ret == FALSE) {goto ERR;}
            GSEN_OP_DELAY(5);

            /* 在BANK0区设置 */
            SelectBankNum(BANK0);
            GSEN_OP_DELAY(5); 
            
        }      
    
    }else{
        #if EN_DEBUG_GSEN > 0
        debug_printf_dir("\r\n<**** 无传感器 ****>\r\n");
        #endif
		return FALSE;
    }      

    #if 0
    if ((s_status & STAT_SET_GYRO) != 0) {
        ret = WriteGyroCalibPara();
        if (ret == FALSE) return FALSE;
    }
    #endif

    s_status &= ~STAT_REINIT;
    s_status |= STAT_INIT;

    #if EN_DEBUG_GSEN > 0
    //调试温度
    //dal_gsen_EnableTemp();
    #endif
    return TRUE;

ERR:
    if(s_dev_type == DEV_ICM_20648){
        /* 在BANK0区设置 */
        SelectBankNum(BANK0);
        GSEN_OP_DELAY(5); 
    }    
    return FALSE;
    
}

/********************************************************************************
**  函数名称:  GsenInit
**  功能描述:  初始化配置g-sensor模块
**  输入参数: 无
**  返回参数: TRUE，设置成功；FALSE，设置失败
********************************************************************************/
static BOOLEAN GsenInit(void)
{
    INT16S rch;
    BOOLEAN ret;
    INT8U chk_dev_cnt;
    INT8U tmp_dev_type;
    ret = FALSE;
    
    #if EN_DEBUG_GSEN > 0
    debug_printf_dir("\r\n<*****GsenInit() *****>\r\n");
    
    #endif
    
    /* 检验设备类型 */
    for(chk_dev_cnt = 0; chk_dev_cnt < 5; chk_dev_cnt++){
        if(DeviceTypeCheck(&tmp_dev_type) == TRUE){
            #if EN_DEBUG_GSEN > 0
            debug_printf_dir("\r\n<***** 检验设备类型成功 *****>\r\n");
            #endif
            s_dev_type = tmp_dev_type;
            break;
        }
    }
    
    if(chk_dev_cnt == 5){
        s_dev_type = DEV_NONE;
        return FALSE;
    }
    
    if(s_dev_type == DEV_MPU_6500){
        /* 读取设备ID */
        rch = hal_i2c_ReadByte(I2C_ADDR, MPU_REG_DEVICE_ID);
        if (rch == -1 || rch != MPU_DEF_DEVICE_ID) {                    // 检查设备ID
            goto retp;
        }
         
        #if EN_DEBUG_GSEN > 0
        debug_printf_dir("\r\n<**** MPU6500 ****>\r\n");
        #endif    
        
        /* 复位设备 */
        ret = gsen_reset_device();
        if (ret == FALSE) goto retp;

        /* 设置采样频率:100hz;ODR=1000/(9+1)=100HZ */
        ret = hal_i2c_WriteByte(I2C_ADDR, MPU_REG_SMPLRT_DIV, 0x09);
        GSEN_OP_DELAY(5);
        if (ret == FALSE) goto retp;

        // hal_i2c_WriteByte(I2C_ADDR, REG_DLPF_CFG, 0x0);          // 默认就是0
        /* gyro量程配置,GYRO量程±500dps,陀螺仪、温度传感器的band width:250HZ */
        ret = hal_i2c_WriteByte(I2C_ADDR, MPU_REG_GYRO_CFG, 0x08);      
        GSEN_OP_DELAY(5);
        if (ret == FALSE) goto retp;

        /* ACCL量程±8g */
        ret = hal_i2c_WriteByte(I2C_ADDR, MPU_REG_ACCL_CFG, 0x10);      
        GSEN_OP_DELAY(5);
        if (ret == FALSE) goto retp;

        /* ACCEL,bandwidth 184HZ */
        ret = hal_i2c_WriteByte(I2C_ADDR, MPU_REG_ACCL_CFG2, 0x01);   
        GSEN_OP_DELAY(5);
        if (ret == FALSE) goto retp;

        /* 低功耗模式下：wackup freq 15.63HZ，太低会降低灵敏度 */
        ret = hal_i2c_WriteByte(I2C_ADDR, MPU_REG_LPODR_CTRL,0x06);      
        GSEN_OP_DELAY(5);
        if (ret == FALSE) goto retp;

        /* ACTL=0,OPEN=0,LATCH_INT_EN=0,INT_ANYRD_2CLEAR=0 */
        ret = hal_i2c_WriteByte(I2C_ADDR, MPU_REG_INTPARA_CFG,0x00);  
        GSEN_OP_DELAY(5);
        if (ret == FALSE) goto retp;

        /* 默认关闭TEMP */
        ret = hal_i2c_WriteByte(I2C_ADDR, MPU_REG_PWR_MGR1, 0x08);       
        GSEN_OP_DELAY(5);

        /* 默认使能ACCL和GYRO六轴，复位值就是0 */
        // hal_i2c_WriteByte(I2C_ADDR, REG_PWR_MGR2, 0);             
        // GSEN_OP_DELAY(5);            
            
    }else if(s_dev_type == DEV_ICM_20648){

        /* 读取设备ID */
        rch = hal_i2c_ReadByte(I2C_ADDR, ICM_REG_WHO_AM_I);
        if (rch == -1 || rch != ICM_DEF_DEVICE_ID) {                    // 检查设备ID
            goto retp;
        }
        
        #if EN_DEBUG_GSEN > 0
        debug_printf_dir("\r\n<**** ICM20648:0x%x ****>\r\n", rch);
        #endif    
        
        /* 复位设备 */
        ret = gsen_reset_device();
        if (ret == FALSE) goto retp;

        /* 低功耗模式下duty_cycled：i2c_master、accel、gryo*/
        ret = hal_i2c_WriteByte(I2C_ADDR, ICM_REG_LP_CONFIG,0x00);  //0x30    
        GSEN_OP_DELAY(5);
        if (ret == FALSE) goto retp;

        /* 六轴模式 */
        ret = hal_i2c_WriteByte(I2C_ADDR, ICM_REG_POW_MGMT_2, 0x00);//0x00      
        GSEN_OP_DELAY(5);  
        if (ret == FALSE) goto retp;
        
        /* 默认关闭TEMP,使用PLL=1（使用陀螺仪时钟需要配置成PLL模式） */
        ret = hal_i2c_WriteByte(I2C_ADDR, ICM_REG_POW_MGMT_1, 0x09);//0x08      
        GSEN_OP_DELAY(10);
        if (ret == FALSE) goto retp;

        /* 在BANK2区设置 */
        ret = SelectBankNum(BANK2);
        GSEN_OP_DELAY(5);

        /* gyro量程配置,GYRO量程±500dps,GYRO_FCHOICE = 1,GYRO_DLPFCFG = 3;band width:51.2HZ */
        ret = hal_i2c_WriteByte(I2C_ADDR, ICM_REG_GYRO_CONFIG_1, 0x1b); //0x1b     
        GSEN_OP_DELAY(5);
        if (ret == FALSE) goto retp;
       
        /* 低功耗模式(duty cycled)下的平均滤波器配置设置：1x averaging，NBW:773.5HZ */
        ret = hal_i2c_WriteByte(I2C_ADDR, ICM_REG_GYRO_CONFIG_2, 0); // 2     
        GSEN_OP_DELAY(5);
        if (ret == FALSE) goto retp;    
        
        /* gyro设置采样频率:ODR=1125/(3+1)=281.25HZ */
        ret = hal_i2c_WriteByte(I2C_ADDR, ICM_REG_GYRO_SMPLR_DIV, 3);// 10
        GSEN_OP_DELAY(5);
        if (ret == FALSE) goto retp;

        /* accel设置采样频率:102hz;ODR=1125/(5+1)=187.5HZ */
        ret = hal_i2c_WriteByte(I2C_ADDR, ICM_REG_ACCEL_SMPLRT_DIV1, 0);
        GSEN_OP_DELAY(5);
        if (ret == FALSE) goto retp;

        ret = hal_i2c_WriteByte(I2C_ADDR, ICM_REG_ACCEL_SMPLRT_DIV2, 5);
        GSEN_OP_DELAY(5);
        if (ret == FALSE) goto retp;

        /* ACCL量程±8g,ACCEL_FCHOICE = 1,ACCEL_DLPFCFG = 2;BW:111.4HZ,NBW:136.0 HZ */
        ret = hal_i2c_WriteByte(I2C_ADDR, ICM_REG_ACCEL_CONFIG, 0x15); //0x15     
        GSEN_OP_DELAY(5);
        if (ret == FALSE) goto retp;

        /* 低功耗模式下（duty cycled）控制加速度计抽取器中的平均样本数：1~4 sample */
        ret = hal_i2c_WriteByte(I2C_ADDR, ICM_REG_ACCEL_CONFIG_2, 0); // 1  
        GSEN_OP_DELAY(5);
        if (ret == FALSE) goto retp;

        /* 设置温度低通滤波,NBW:123.5HZ */
        ret = hal_i2c_WriteByte(I2C_ADDR, ICM_REG_TEMP_CONFIG, 2); 
        GSEN_OP_DELAY(5);
        if (ret == FALSE) goto retp;

        /* 设置ODR开始时间对齐 */
        ret = hal_i2c_WriteByte(I2C_ADDR, ICM_REG_ODR_ALIGN_EN, 0x01); 
        GSEN_OP_DELAY(5);
        if (ret == FALSE) goto retp;

        /* 在BANK0区设置 */
        SelectBankNum(BANK0);
        GSEN_OP_DELAY(5);

        /* ACTL=0,OPEN=0,LATCH_INT_EN=0,INT_ANYRD_2CLEAR=0 */
        ret = hal_i2c_WriteByte(I2C_ADDR, ICM_REG_INT_PIN_CFG,0x00);  
        GSEN_OP_DELAY(5);
        if (ret == FALSE) goto retp;
    
    }else{
        #if EN_DEBUG_GSEN > 0
        debug_printf_dir("\r\n<**** 无传感器 ****>\r\n");
        #endif
        return FALSE;
    }  

    return TRUE;

retp:

    #if EN_DEBUG_GSEN > 0
    debug_printf_dir("\r\n<**** 传感器初始化失败 ****>\r\n");
    #endif
    
    if(s_dev_type == DEV_ICM_20648){
        /* 在BANK0区设置 */
        SelectBankNum(BANK0);
        GSEN_OP_DELAY(5);
    }
    return ret;
}

/********************************************************************************
**  函数名称:  ReadSensorDataCalbak
**  功能描述:  读取传感器数据回调函数，在底层I2C中断中回调
**  输入参数:  [in] idx: I2C标号
**             [in] result:回调结果
**  返回参数:  无
********************************************************************************/
static void ReadSensorDataCalbak(INT8U idx, BOOLEAN result)
{
    BOOLEAN ret;
    INT8U rtype;

    if (idx != I2C_IDX_0) return;
    if (s_rcb.isbusy == FALSE) return;
    ret = TRUE;
    
    /* 如果准备进入低功耗模式，中止当前读取操作 */
    if (s_rcb.hitreq == 0 && (s_status & STAT_LOWPWR) != 0) {   
        s_rcb.isbusy = FALSE;
        return;
    }
    if (result != TRUE) {
        ret = FALSE;
        goto endp;
    }

    rtype = s_rcb.rtype;
    if(s_dev_type == DEV_MPU_6500){
         /* 读取ACCL或GYRO传感器 */
        if (rtype == TYPE_ACCL || rtype == TYPE_GYRO) {             
            switch (s_rcb.step) {
                case STEP_X_H:
                    s_rcb.step = STEP_X_L;
                    if (rtype == TYPE_ACCL) {
                        s_rcb.rpara.addr = MPU_REG_ACCL_XOUT_L;
                    } else {
                        s_rcb.rpara.addr = MPU_REG_GYRO_XOUT_L;
                    }
                    s_rcb.rpara.buf = &s_rcb.rword.bytes.low;
                    ret = hal_i2c_ReadData(I2C_IDX_0, &s_rcb.rpara);
                    break;
                case STEP_X_L:
                    s_rcb.step = STEP_Y_H;
                    if (rtype == TYPE_ACCL) {
                        s_rcb.rpara.addr = MPU_REG_ACCL_YOUT_H;
                    } else {
                        s_rcb.rpara.addr = MPU_REG_GYRO_YOUT_H;
                    }
                    s_rcb.rpara.buf = &s_rcb.rword.bytes.high;
                     /* 保存x轴数据 */
                    ((AXIS_DATA16_T *)s_rcb.pdata)->axis_x = (INT16S)s_rcb.rword.hword;  
                    ret = hal_i2c_ReadData(I2C_IDX_0, &s_rcb.rpara);
                    break;
                case STEP_Y_H:
                    s_rcb.step = STEP_Y_L;
                    if (rtype == TYPE_ACCL) {
                        s_rcb.rpara.addr = MPU_REG_ACCL_YOUT_L;
                    } else {
                        s_rcb.rpara.addr = MPU_REG_GYRO_YOUT_L;
                    }
                    s_rcb.rpara.buf = &s_rcb.rword.bytes.low;
                    ret = hal_i2c_ReadData(I2C_IDX_0, &s_rcb.rpara);
                    break;
                case STEP_Y_L:
                    s_rcb.step = STEP_Z_H;
                    if (rtype == TYPE_ACCL) {
                        s_rcb.rpara.addr = MPU_REG_ACCL_ZOUT_H;
                    } else {
                        s_rcb.rpara.addr = MPU_REG_GYRO_ZOUT_H;
                    }
                    s_rcb.rpara.buf = &s_rcb.rword.bytes.high;
                    /* 保存y轴数据 */
                    ((AXIS_DATA16_T *)s_rcb.pdata)->axis_y = (INT16S)s_rcb.rword.hword;   
                    ret = hal_i2c_ReadData(I2C_IDX_0, &s_rcb.rpara);
                    break;
                case STEP_Z_H:
                    s_rcb.step = STEP_Z_L;
                    if (rtype == TYPE_ACCL) {
                        s_rcb.rpara.addr = MPU_REG_ACCL_ZOUT_L;
                    } else {
                        s_rcb.rpara.addr = MPU_REG_GYRO_ZOUT_L;
                    }
                    s_rcb.rpara.buf = &s_rcb.rword.bytes.low;
                    ret = hal_i2c_ReadData(I2C_IDX_0, &s_rcb.rpara);
                    break;
                case STEP_Z_L:
                    /* 保存z轴数据 */
                    ((AXIS_DATA16_T *)s_rcb.pdata)->axis_z = (INT16S)s_rcb.rword.hword;   
                    if (rtype == TYPE_ACCL) {
                        if (s_rcb.hitreq == FLAG_INT_RD) {
                            /* 读取碰撞数据有效 */
                            s_hitdata_valid = TRUE;
                            
                            s_rcb.hitreq = 0;
                        } else {
                            s_accldata_valid = TRUE;
                        }
                    } else {
                        /* 读取碰撞数据有效 */
                        s_gyrodata_valid = TRUE;
                    }
                    goto endp;
                    //break;
            }
        } else if (rtype == TYPE_TEMP) {                                    
            /* 读取温度传感器 */
            switch (s_rcb.step) {
                case STEP_X_H:
                    s_rcb.step = STEP_X_L;
                    s_rcb.rpara.addr = MPU_REG_TEMP_OUT_L;
                    s_rcb.rpara.buf = &s_rcb.rword.bytes.low;
                    ret = hal_i2c_ReadData(I2C_IDX_0, &s_rcb.rpara);
                    break;
                case STEP_X_L:
                    /* 保存温度数据 */
                    *((INT16S *)s_rcb.pdata) = (INT16S)s_rcb.rword.hword;   
                    /* 读取温度数据有效 */
                    s_tempdata_valid = TRUE;
                    goto endp;  // 已经读取完成
                    //break;
            }
        }           
    
    }else if(s_dev_type == DEV_ICM_20648){

        /* 读取ACCL或GYRO传感器 */
        if (rtype == TYPE_ACCL || rtype == TYPE_GYRO) {             
            switch (s_rcb.step) {
                case STEP_X_H:
                    s_rcb.step = STEP_X_L;
                    if (rtype == TYPE_ACCL) {
                        s_rcb.rpara.addr = ICM_REG_ACCEL_XOUT_L;
                    } else {
                        s_rcb.rpara.addr = ICM_REG_GYRO_XOUT_L;
                    }
                    s_rcb.rpara.buf = &s_rcb.rword.bytes.low;
					#if EN_DEBUG_GSEN > 0
			        //debug_printf_dir("\r\n<**read XL***>\r\n");
			        #endif
                    ret = hal_i2c_ReadData(I2C_IDX_0, &s_rcb.rpara);
                    break;
                case STEP_X_L:
                    s_rcb.step = STEP_Y_H;
                    if (rtype == TYPE_ACCL) {
                        s_rcb.rpara.addr = ICM_REG_ACCEL_YOUT_H;
                    } else {
                        s_rcb.rpara.addr = ICM_REG_GYRO_YOUT_H;
                    }
                    s_rcb.rpara.buf = &s_rcb.rword.bytes.high;
                     /* 保存x轴数据 */
                    ((AXIS_DATA16_T *)s_rcb.pdata)->axis_x = (INT16S)s_rcb.rword.hword;  
                    ret = hal_i2c_ReadData(I2C_IDX_0, &s_rcb.rpara);
                    break;
                case STEP_Y_H:
                    s_rcb.step = STEP_Y_L;
                    if (rtype == TYPE_ACCL) {
                        s_rcb.rpara.addr = ICM_REG_ACCEL_YOUT_L;
                    } else {
                        s_rcb.rpara.addr = ICM_REG_GYRO_YOUT_L;
                    }
                    s_rcb.rpara.buf = &s_rcb.rword.bytes.low;
                    ret = hal_i2c_ReadData(I2C_IDX_0, &s_rcb.rpara);
                    break;
                case STEP_Y_L:
                    s_rcb.step = STEP_Z_H;
                    if (rtype == TYPE_ACCL) {
                        s_rcb.rpara.addr = ICM_REG_ACCEL_ZOUT_H;
                    } else {
                        s_rcb.rpara.addr = ICM_REG_GYRO_ZOUT_H;
                    }
                    s_rcb.rpara.buf = &s_rcb.rword.bytes.high;
                    /* 保存y轴数据 */
                    ((AXIS_DATA16_T *)s_rcb.pdata)->axis_y = (INT16S)s_rcb.rword.hword;   
                    ret = hal_i2c_ReadData(I2C_IDX_0, &s_rcb.rpara);
                    break;
                case STEP_Z_H:
                    s_rcb.step = STEP_Z_L;
                    if (rtype == TYPE_ACCL) {
                        s_rcb.rpara.addr = ICM_REG_ACCEL_ZOUT_L;
                    } else {
                        s_rcb.rpara.addr = ICM_REG_GYRO_ZOUT_L;
                    }
                    s_rcb.rpara.buf = &s_rcb.rword.bytes.low;
                    ret = hal_i2c_ReadData(I2C_IDX_0, &s_rcb.rpara);
                    break;
                case STEP_Z_L:
                    /* 保存z轴数据 */
                    ((AXIS_DATA16_T *)s_rcb.pdata)->axis_z = (INT16S)s_rcb.rword.hword;   
                    if (rtype == TYPE_ACCL) {
                        if (s_rcb.hitreq == FLAG_INT_RD) {
                            /* 读取碰撞数据有效 */
                            s_hitdata_valid = TRUE;
                            
                            s_rcb.hitreq = 0;
                        } else {
                            s_accldata_valid = TRUE;
                        }
                    } else {
                        /* 读取碰撞数据有效 */
                        s_gyrodata_valid = TRUE;
                    }
                    goto endp;
                    //break;
            }
        } else if (rtype == TYPE_TEMP) {                                    
            /* 读取温度传感器 */
            switch (s_rcb.step) {
                case STEP_X_H:
                    s_rcb.step = STEP_X_L;
                    s_rcb.rpara.addr = ICM_REG_TEMP_OUT_L;
                    s_rcb.rpara.buf = &s_rcb.rword.bytes.low;
                    ret = hal_i2c_ReadData(I2C_IDX_0, &s_rcb.rpara);
                    break;
                case STEP_X_L:
                    /* 保存温度数据 */
                    *((INT16S *)s_rcb.pdata) = (INT16S)s_rcb.rword.hword;   
                    /* 读取温度数据有效 */
                    s_tempdata_valid = TRUE;
                    goto endp;  // 已经读取完成
                    //break;
            }
        }          
    }else{
        #if EN_DEBUG_GSEN > 0
        debug_printf_dir("\r\n<**** 无传感器 ****>\r\n");
        #endif
    }     

    if (ret != TRUE) goto endp;
    return;

endp:

    // rval = OS_DisableSysINT(); // i2c中断已是系统级中断中最高优先级
    s_rcb.isbusy = FALSE;
    // OS_EnableSysINT(rval);
    if (ret != TRUE) {
        s_status |= STAT_REINIT;
        s_status &= ~STAT_INIT;
        s_rcb.hitreq = 0;
        return;
    }
    
    /* 读数据请求 */
    if (s_rcb.hitreq == FLAG_INT_RDREQ) {
        StartReadSensorData(TYPE_ACCL, TRUE);
    } else {
        if (s_rtype == TYPE_ACCL) {
            s_rtype = TYPE_GYRO;
        } else if (s_rtype == TYPE_GYRO) {

            if ((s_status & STAT_TEMP_EN) != 0) {
                s_rtype = TYPE_TEMP;

            } else {    // 一组数据已经读取完毕，读ACCL/GYRO
                s_rtype = TYPE_ACCL;
                return;
            }
        } else {        // 一组数据已经读取完毕，读ACCL/GYRO/TEMP
            s_rtype = TYPE_ACCL;
            return;
        }
        StartReadSensorData(s_rtype, FALSE);
    }
}

/********************************************************************************
**  函数名称:  StartReadSensorData
**  功能描述:  启动读取ACCL或GYRO三轴数据
**  输入参数:  [in] rtype: 读取数据类型
**             [in] hitreq:读数据请求状态
**  返回参数:  无
********************************************************************************/
static void StartReadSensorData(INT8U rtype, BOOLEAN hitreq)
{
    //INT32U rval;
    
    #if EN_DEBUG_GSEN > 1
    debug_printf_dir("\r\n<*****StartReadSensorData()*****>\r\n");
    #endif


    if ((s_status & STAT_INIT) == 0) return;
    if (rtype >= MAX_TYPE) return;
    if (rtype == TYPE_TEMP && (s_status & STAT_TEMP_EN) == 0) return;
    if (hitreq == FALSE && (s_status & STAT_LOWPWR) != 0) return;

	#if 1
    ENTER_CRITICAL();
    if (s_rcb.isbusy == TRUE) {
        if (hitreq == TRUE) s_rcb.hitreq = FLAG_INT_RDREQ;
        EXIT_CRITICAL();
        return;
    }
    s_rcb.isbusy = TRUE;
    EXIT_CRITICAL();
	#endif
    if (hitreq == TRUE) s_rcb.hitreq = FLAG_INT_RD;     // 已开始读，不需要后续再重新读
    s_rcb.step = STEP_X_H;
    s_rcb.rtype = rtype;
    
    if(s_dev_type == DEV_MPU_6500){
        if (rtype == TYPE_ACCL) {
            s_rcb.pdata = (void *)&s_accl_tmpdata;
            s_rcb.rpara.addr =  MPU_REG_ACCL_XOUT_H;
        } else if (rtype == TYPE_GYRO) {
            s_rcb.pdata = (void *)&s_gyro_tmpdata;
            s_rcb.rpara.addr =  MPU_REG_GYRO_XOUT_H;
        } else if (rtype == TYPE_TEMP){                     // 读取温度数据
            s_rcb.pdata = (void *)&s_temper_tmpdata;
            s_rcb.rpara.addr =  MPU_REG_TEMP_OUT_H;
        } else {
            return;
        }        
    }else if(s_dev_type == DEV_ICM_20648){
        if (rtype == TYPE_ACCL) {
            s_rcb.pdata = (void *)&s_accl_tmpdata;
            s_rcb.rpara.addr =  ICM_REG_ACCEL_XOUT_H;
        } else if (rtype == TYPE_GYRO) {
            s_rcb.pdata = (void *)&s_gyro_tmpdata;
            s_rcb.rpara.addr =  ICM_REG_GYRO_XOUT_H;
        } else if (rtype == TYPE_TEMP){                     // 读取温度数据
            s_rcb.pdata = (void *)&s_temper_tmpdata;
            s_rcb.rpara.addr =  ICM_REG_TEMP_OUT_H;
        } else {
            return;
        }      
    }else{
        #if EN_DEBUG_GSEN > 0
        debug_printf_dir("\r\n<**** 无传感器 ****>\r\n");
        #endif
        return;
    }     
    
    s_rcb.rpara.devaddr = I2C_DEV_ADDR;
    s_rcb.rpara.is10bit = FALSE;
    s_rcb.rpara.buf = &s_rcb.rword.bytes.high;
    s_rcb.rpara.dlen = 1;                               // 每次只能读取一个寄存器
    s_rcb.rpara.calbak = ReadSensorDataCalbak;

    hal_i2c_ReadData(I2C_IDX_0, &s_rcb.rpara);          // 启动第一步读操作
}

/********************************************************************************
**  函数名称:  CheckHitDataOverThreshold
**  功能描述:  判断accel是否超过阈值，用于判断碰撞
**  输入参数:  无
**  返回参数:  TRUE，设置成功；FALSE，设置失败
********************************************************************************/
static BOOLEAN CheckHitDataOverThreshold(void)
{
    INT32U tmp;

    tmp = abs((INT32S)s_accl_hitdata.axis_x - s_accl_calib.axis_x);
    if (tmp > s_hit_threshold) return TRUE;
    tmp = abs((INT32S)s_accl_hitdata.axis_y - s_accl_calib.axis_y);
    if (tmp > s_hit_threshold) return TRUE;
    tmp = abs((INT32S)s_accl_hitdata.axis_z - s_accl_calib.axis_z);
    if (tmp > s_hit_threshold) return TRUE;
    return FALSE;
}

/********************************************************************************
**  函数名称:  GsenCalCalibData
**  功能描述:  计算标定数据，并将gyro标定的偏移量写入芯片寄存器
**  输入参数:  [in] accl:加速度数据
**             [in] gyro:陀螺仪数据
**  返回参数:  无
********************************************************************************/
static void GsenCalCalibData(AXIS_DATA_T *accl, AXIS_DATA_T *gyro)
{
    FP32 tmp;

    tmp = (FP32)s_accl_calib.axis_x / GSEN_CALIB_COUNT;
    s_accl_calib.axis_x = (INT32S)tmp;
    tmp *= ACCL_CAL_RATIO;
    accl->axis_x = (INT32S)(tmp >= 0 ? tmp + 0.5 : tmp - 0.5);

    tmp = (s_accl_calib.axis_y / GSEN_CALIB_COUNT);
    s_accl_calib.axis_y = (INT32S)tmp;
    tmp *= ACCL_CAL_RATIO;
    accl->axis_y = (INT32S)(tmp >= 0 ? tmp + 0.5 : tmp - 0.5);

    tmp = s_accl_calib.axis_z / GSEN_CALIB_COUNT;
    s_accl_calib.axis_z = (INT32S)tmp;
    tmp *= ACCL_CAL_RATIO;
    accl->axis_z = (INT32S)(tmp >= 0 ? tmp + 0.5 : tmp - 0.5);

    tmp = s_gyro_calib.axis_x / GSEN_CALIB_COUNT;
    s_gyro_calib.axis_x = (INT32S)tmp;
    tmp *= GYRO_CAL_RATIO;
    gyro->axis_x = (INT32S)(tmp >= 0 ? tmp + 0.5 : tmp - 0.5);

    tmp = s_gyro_calib.axis_y / GSEN_CALIB_COUNT;
    s_gyro_calib.axis_y = (INT16U)tmp;
    tmp *= GYRO_CAL_RATIO;
    gyro->axis_y = (INT32S)(tmp >= 0 ? tmp + 0.5 : tmp - 0.5);

    tmp = s_gyro_calib.axis_z / GSEN_CALIB_COUNT;
    s_gyro_calib.axis_z = (INT16U)tmp;
    tmp *= GYRO_CAL_RATIO;
    gyro->axis_z = (INT32S)(tmp >= 0 ? tmp + 0.5 : tmp - 0.5);

    // WriteGyroCalibPara();
}

/********************************************************************************
**  函数名称:  PowDownGsensor
**  功能描述:  关闭传感器电源
**  输入参数:  无
**  返回参数:  无
********************************************************************************/
static void PowDownGsensor(void)
{
    WriteOutputPort(GPIOA, GPIO_Pin_10, TRUE);
}

/********************************************************************************
**  函数名称:  PowUpGsensor
**  功能描述:  打开传感器电源
**  输入参数:  无
**  返回参数:  无
********************************************************************************/
static void PowUpGsensor(void)
{
    WriteOutputPort(GPIOA, GPIO_Pin_10, FALSE);
}

/********************************************************************************
**  函数名称:  GsenScanTmrProc
**  功能描述:  定时扫描函数入口
**  输入参数:  无
**  返回参数:  无
********************************************************************************/
static void GsenScanTmrProc(void)
{
    INT16S rch;
    BOOLEAN ret;
    Q_MSG_GSENSER msg;
    
    #if EN_DEBUG_GSEN > 0
    static INT8U print_cnt = 0;
    static INT8U print_cnt1 = 0;
    static INT8U print_cnt2 = 0;
    #endif
    
    #if EN_DEBUG_GSENTMR > 0
    if (s_gsen_tmr_running == FALSE) return;
    #endif
    
    if (s_rcb.isbusy == TRUE && (s_status & STAT_REINIT) == 0) {
        if (++s_ct_busy > MAX_BUSY_CNT) {
            s_status |= STAT_REINIT;
            s_rcb.isbusy = FALSE;
        }
    } else {
        s_ct_busy = 0;
    }
    if ((s_status & STAT_RESET) != 0) {
    	s_delcyc_cout++;
    	if(s_delcyc_cout > GSEN_SWITCH_TIME) {
            /* 传感器上电 */
			PowUpGsensor();
			if(s_delcyc_cout > (GSEN_SWITCH_TIME * 2)){
				s_status &= ~STAT_RESET;
				s_ct_busy = 0;
				s_delcyc_cout=0;
			}

    	}
		return;
    }
    // if (s_detect_int == TRUE && (s_status & STAT_LOWPWR) != 0) {
    if ((s_status & STAT_LOWPWR) != 0) {
        if(s_dev_type == DEV_MPU_6500){
            /* 读取中断状态,读取后，自动清0 */
            rch = hal_i2c_ReadByte(I2C_ADDR, MPU_REG_INT_STAT);                         
        }else if(s_dev_type == DEV_ICM_20648){
            /* 读取中断状态,读取后，自动清0 */
            rch = hal_i2c_ReadByte(I2C_ADDR, ICM_REG_INT_STATUS);                             
        }

        #if EN_DEBUG_GSEN > 0
        debug_printf_dir("\r\n<*****int stat:%#x, s_status:%#x*****>\r\n",rch, s_status);
        #endif
        
        /* 退出低功耗 */
        gsen_exit_lowpwr();
        
        if(s_dev_type == DEV_MPU_6500){
            /* 查询震动唤醒标志是否有效，有效则发送震动唤醒消息 */
            if (rch != -1 && (rch & MPU_WOM_INTSTAT_MASK) != 0) {
                msg.msgid = MSG_OPT_OTWAKE_ENENT;
                msg.lpara= SHELL_MSG_MCU_WAKEUP;
                msg.hpara= SHELL_WKUP_EVNT_MOTION;//改GSEN_EVENT_MOTION;
                OS_PostMsg(0, msg.msgid, msg.lpara, msg.hpara);
                #if EN_DEBUG_GSEN > 0
                debug_printf_dir("\r\n<***** gs mes(1) *****>\r\n");
                #endif
            }        
        }else if(s_dev_type == DEV_ICM_20648){
            /* 查询震动唤醒标志是否有效，有效则发送震动唤醒消息 */
            if (rch != -1 && (rch & ICM_WOM_INTSTAT_MASK) != 0) {
                msg.msgid = MSG_OPT_GSEN_MOTION_EVNT;
                msg.lpara = SHELL_MSG_MCU_WAKEUP;
                msg.hpara = SHELL_WKUP_EVNT_MOTION;//改GSEN_EVENT_MOTION;
                OS_PostMsg(0, msg.msgid, msg.lpara, msg.hpara);
                #if EN_DEBUG_GSEN > 0
                debug_printf_dir("\r\n<***** gs mes(2) *****>\r\n");
                #endif                
            }          
        }  

    }

    if (s_detect_int == TRUE) {
        if(s_dev_type == DEV_MPU_6500){
            /* 读取中断状态,读取后，自动清0 */
            rch = hal_i2c_ReadByte(I2C_ADDR, MPU_REG_INT_STAT);                         
        }else if(s_dev_type == DEV_ICM_20648){
            /* 读取中断状态,读取后，自动清0 */
            rch = hal_i2c_ReadByte(I2C_ADDR, ICM_REG_INT_STATUS);                             
        }

        #if EN_DEBUG_GSEN > 0
        debug_printf_dir("\r\n<*****int stat:%#x, hitdata_valid:%d *****>\r\n",rch, s_hitdata_valid);
        #endif
        
        if (s_hitdata_valid == TRUE) {  // 应能可检测休眠模式下碰撞唤醒事件
            s_accl_hitdata = s_accl_tmpdata;
            #if EN_DEBUG_GSEN > 0
            debug_printf_dir("\r\n<*****hit accel data: x:%d, y:%d, z:%d *****>\r\n",s_accl_hitdata.axis_x,
            s_accl_hitdata.axis_y, s_accl_hitdata.axis_z);
            #endif
            s_accl_data = s_accl_tmpdata;
            s_accldata_valid = TRUE;
            if (CheckHitDataOverThreshold() == TRUE) {
                msg.msgid = MSG_OPT_GSEN_HIT_EVNT;
                msg.lpara = SHELL_MSG_GSENSOR_EVENT;
                msg.hpara = GSEN_EVENT_HIT;
                OS_PostMsg(0, msg.msgid, msg.lpara, msg.hpara);
                ret = TRUE;
                #if EN_DEBUG_GSEN > 0
                debug_printf_dir("\r\n<***** gs mes(3) *****>\r\n");
                #endif
            }
            memset(&s_accl_tmpdata, 0, sizeof(s_accl_tmpdata));
        }

       if(s_dev_type == DEV_MPU_6500){
            if (rch != -1 && (rch & MPU_WOM_INTSTAT_MASK) != 0 && ret == FALSE) {   // 主要针对非休眠模式下振动检测
                msg.msgid = MSG_OPT_GSEN_MOTION_EVNT;
                msg.lpara = SHELL_MSG_GSENSOR_EVENT;
                msg.hpara = GSEN_EVENT_MOTION;
                OS_PostMsg(0, msg.msgid, msg.lpara, msg.hpara);
                #if EN_DEBUG_GSEN > 0
                debug_printf_dir("\r\n<***** gs mes(4) *****>\r\n");
                #endif                
            }        
        }else if(s_dev_type == DEV_ICM_20648){
            if (rch != -1 && (rch & ICM_WOM_INTSTAT_MASK) != 0 && ret == FALSE) {   // 主要针对非休眠模式下振动检测
                msg.msgid = MSG_OPT_GSEN_MOTION_EVNT;
                msg.lpara = SHELL_MSG_GSENSOR_EVENT;
                msg.hpara = GSEN_EVENT_MOTION;
                OS_PostMsg(0, msg.msgid, msg.lpara, msg.hpara);
                #if EN_DEBUG_GSEN > 0
                debug_printf_dir("\r\n<***** gs mes(5) *****>\r\n");
                #endif
            }          
        }

        s_detect_int = FALSE;
        // if (rch != -1 && (rch & WOM_INTSTAT_MASK) == 0) s_detect_int = FALSE;
        // if ((s_status & STAT_HIT_THRS) == 0) s_detect_int = FALSE;
    } else {
        s_hitdata_valid = FALSE;
    }
    if ((s_status & STAT_REINIT) != 0) {

        ret = GsenReInit();
        #if EN_DEBUG_GSEN > 0
        debug_printf_dir("\r\n<*****Reinit GsensorDrv!! *****>\r\n");
        #endif

        if (ret == FALSE) {
            #if EN_DEBUG_GSEN > 0
            debug_printf_dir("\r\n<*****Reset Gsensor&I2C!! *****>\r\n");
            #endif
            //rval = OS_DisableSysINT();
            dal_i2c_sim_Close(I2C_IDX_0);
            dal_i2c_sim_Open(I2C_IDX_0);
            PowDownGsensor();
            s_status |= STAT_RESET;
            //OS_EnableSysINT(rval);
            return;
        }
    }
    if ((s_status & STAT_CALIB) != 0) {         // 处于标定状态
        if (s_accldata_valid == TRUE && s_gyrodata_valid == TRUE) {
            s_accl_calib.axis_x += s_accl_tmpdata.axis_x;
            s_accl_calib.axis_y += s_accl_tmpdata.axis_y;
            s_accl_calib.axis_z += s_accl_tmpdata.axis_z;

            s_gyro_calib.axis_x += s_gyro_tmpdata.axis_x;
            s_gyro_calib.axis_y += s_gyro_tmpdata.axis_y;
            s_gyro_calib.axis_z += s_gyro_tmpdata.axis_z;
            if (++s_ct_calib >= GSEN_CALIB_COUNT) {
                AXIS_DATA_T accl_calib, gyro_calib;
                GsenCalCalibData(&accl_calib, &gyro_calib);
                s_status |= STAT_SET_GYRO;  // gyro偏移量标定完成等效于设置
                if (s_calib_calbak_fp != NULL) {
                    (*s_calib_calbak_fp)(&accl_calib, &gyro_calib);
                }
                s_status &= ~STAT_CALIB;
            }
        }
    }
    if (s_accldata_valid == TRUE) {
        s_accl_data = s_accl_tmpdata;
        memset(&s_accl_tmpdata, 0, sizeof(s_accl_tmpdata));
        s_accldata_valid = FALSE;
        
        #if EN_DEBUG_GSEN > 0
        if(++print_cnt >= 200){
            print_cnt = 0;
            
            #if 0
            if(s_dev_type == DEV_ICM_20648){
                
                debug_printf_dir("\r\n<***** s_status:0x%x *****>\r\n",s_status);
                
                rch = hal_i2c_ReadByte(I2C_ADDR, ICM_REG_WHO_AM_I);
                if (rch == -1 ) {                    // 检查设备ID
                    debug_printf_dir("\r\n<***** err *****>\r\n");
                }
                debug_printf_dir("\r\n<***** ICM_REG_WHO_AM_I:0x%x *****>\r\n",rch);
                
                rch = hal_i2c_ReadByte(I2C_ADDR, ICM_REG_USER_CTL);
                if (rch == -1 ) {                    // 检查设备ID
                    debug_printf_dir("\r\n<***** err *****>\r\n");
                }
                debug_printf_dir("\r\n<***** ICM_REG_USER_CTL:0x%x *****>\r\n",rch);
                
      
                rch = hal_i2c_ReadByte(I2C_ADDR, ICM_REG_LP_CONFIG);
                if (rch == -1 ) {                    // 检查设备ID
                    debug_printf_dir("\r\n<***** err *****>\r\n");
                }
                debug_printf_dir("\r\n<***** ICM_REG_LP_CONFIG:0x%x *****>\r\n",rch);
                
                rch = hal_i2c_ReadByte(I2C_ADDR, ICM_REG_POW_MGMT_1);
                if (rch == -1 ) {                    // 检查设备ID
                    debug_printf_dir("\r\n<***** err *****>\r\n");
                }
                debug_printf_dir("\r\n<***** ICM_REG_POW_MGMT_1:0x%x *****>\r\n",rch);

                rch = hal_i2c_ReadByte(I2C_ADDR, ICM_REG_POW_MGMT_2);
                if (rch == -1 ) {                    // 检查设备ID
                    debug_printf_dir("\r\n<***** err *****>\r\n");
                }
                debug_printf_dir("\r\n<***** ICM_REG_POW_MGMT_2:0x%x *****>\r\n",rch);
                
            }
            #endif
            
            debug_printf_dir("\r\n<*****acc:x:%d, y:%d, z:%d*****>\r\n",s_accl_data.axis_x, s_accl_data.axis_y, s_accl_data.axis_z);
        }
        #endif
    }
    if (s_gyrodata_valid == TRUE) {
        s_gyro_data = s_gyro_tmpdata;
        memset(&s_gyro_tmpdata, 0, sizeof(s_gyro_tmpdata));
        s_gyrodata_valid = FALSE;
        #if EN_DEBUG_GSEN > 0
        if(++print_cnt1 >= 200){
            print_cnt1 = 0;
            //debug_printf_dir("\r\n<***** gryo:x:0x%x, y:0x%x, z:0x%x *****>\r\n",s_gyro_data.axis_x, s_gyro_data.axis_y, s_gyro_data.axis_z);
            debug_printf_dir("\r\n<***** gryo:x:%d, y:%d, z:%d *****>\r\n",s_gyro_data.axis_x, s_gyro_data.axis_y, s_gyro_data.axis_z);
        }
        #endif
    }
    if ((s_status & STAT_TEMP_EN) != 0 && s_tempdata_valid == TRUE) {
        s_temper_data = s_temper_tmpdata;
        s_temper_tmpdata = 0;
        s_tempdata_valid = FALSE;
        #if EN_DEBUG_GSEN > 0
        if(print_cnt2++ > 200){
            print_cnt2 = 0;
            debug_printf_dir("\r\n<***** temp:%d *****>\r\n",s_temper_data);
        }
        #endif        
    }
    StartReadSensorData(TYPE_ACCL, FALSE);
}

/*
*********************************************************************************
*                   对外接口实现
*********************************************************************************
*/

/********************************************************************************
** 函数名:     dal_gsen_EnableTemp
** 函数描述:   使能温度数据采集功能，默认初始化完是关闭
** 参数:       无
** 返回:       无
********************************************************************************/
BOOLEAN dal_gsen_EnableTemp(void)
{
    BOOLEAN ret;

    if ((s_status & STAT_INIT) == 0 || (s_status & STAT_LOWPWR) != 0) return FALSE;
    s_status |= STAT_TEMP_EN;
    #if EN_DEBUG_GSEN > 0
    debug_printf_dir("\r\n<*****使能温度检测*****>\r\n");
    #endif

    ret = gsen_enable_temp();
    GSEN_OP_DELAY(5);
    return ret;
}
/********************************************************************************
** 函数名:     dal_gsen_CalibStart
** 函数描述:   收到通知后进行ACCL的X/Y/Z轴的标定
** 参数:       标定完成回调函数，用以返回三轴标定值
** 返回:       无
********************************************************************************/
void dal_gsen_CalibStart(GSEN_CALIB_CALBAK_PTR calbak)
{
    if ((s_status & STAT_CALIB) != 0) return;    // 还未标定完，拒绝处理新的标定
    if (calbak != NULL) {
        s_status |= STAT_CALIB;
        memset(&s_accl_calib, 0, sizeof(s_accl_calib));
        memset(&s_gyro_calib, 0, sizeof(s_gyro_calib));
        s_ct_calib = 0;
        s_calib_calbak_fp = calbak;
    }
}
/********************************************************************************
**  函数名称:  dal_gsen_EnterLowPower
**  功能描述:  设置g-sensor模块进入低功耗模式(系统进入休眠模式前调用)
**  输入参数:  None
**  返回参数:  None
********************************************************************************/
void dal_gsen_EnterLowPower(void)
{
    if ((s_status & STAT_LOWPWR) != 0) return;
#if EN_DEBUG_GSEN > 0
    debug_printf_dir("\r\n<*****gsen enter lowpwr!! *****>\r\n");
#endif
    s_status |= STAT_LOWPWR;
    gsen_enter_lowpwr();
}

/********************************************************************************
**  函数名称:  dal_gsen_EnterSleep
**  功能描述:  设置g-sensor模块进入睡眠模式(系统进入运输模式前调用)
**  输入参数:  None
**  返回参数:  None
********************************************************************************/
void dal_gsen_EnterSleep(void)
{
    if ((s_status & STAT_LOWPWR) != 0) return;
    #if EN_DEBUG_GSEN > 0
    debug_printf_dir("\r\n<*****gsen enter sleep!! *****>\r\n");
    #endif
    s_status |= STAT_LOWPWR;
    gsen_enter_sleep();
    //hal_StopTmr(s_scan_tmr);
}

/********************************************************************************
**  函数名称:  dal_gsen_SetHitPara
**  功能描述:  设置碰撞加速度阀值和三轴轴标定值
**  输入参数: [in] ths_val: 碰撞加速度阀值,单位mg
**            [in] calib:  三轴标定值
**  返回参数:  TRUE，设置成功；FALSE，设置失败
********************************************************************************/
BOOLEAN dal_gsen_SetHitPara(INT16U ths_val, AXIS_DATA_T *calib)
{
    INT8U setval;
    BOOLEAN ret;
    INT16U i;
	INT16S rch;
    
    #if EN_DEBUG_GSEN > 0
    debug_printf_dir("\r\n<*****dal_gsen_SetHitPara():ths_val:%d, calib:x:%d, y:%d, z:%d,  *****>\r\n",ths_val, calib->axis_x, calib->axis_y, calib->axis_z);
    #endif

    if ((s_status & STAT_INIT) == 0 || (s_status & STAT_LOWPWR) != 0) return FALSE;
    for (i = 0; i < 0x1000; i++) {
        if (s_rcb.isbusy == FALSE) {
        	break;
        }
    }

    s_hit_threshold = (INT32U)(ths_val / (ACCL_CAL_RATIO) + 0.5);
    if (calib != NULL) {
        s_accl_calib.axis_x = (INT32S)(calib->axis_x / (ACCL_CAL_RATIO));
        s_accl_calib.axis_y = (INT32S)(calib->axis_y / (ACCL_CAL_RATIO));
        s_accl_calib.axis_z = (INT32S)(calib->axis_z / (ACCL_CAL_RATIO));
    }
    if (ths_val >= MAX_THRS_REGVAL) setval = 255;
    else setval = ths_val / 4;

    /* 使能碰撞检测阈值 */
    s_status |= STAT_HIT_THRS;

    if ((s_status & STAT_INIT) == 0) return FALSE;

    if(s_dev_type == DEV_MPU_6500){
         /* 设置碰撞检测ACCL阀值 */
        ret = hal_i2c_WriteByte(I2C_ADDR, MPU_REG_WOM_THRSHD, setval);    
        if (ret == FALSE) return FALSE;
        GSEN_OP_DELAY(5);

        /* ACCEL_INTEL_EN=1，ACCEL_INTEL_MODE=1，使能WOM检测逻辑 */
        ret =hal_i2c_WriteByte(I2C_ADDR, MPU_REG_ACCL_INTCTL,0xC0); 
        if (ret == FALSE) return FALSE;
        GSEN_OP_DELAY(5);

        /* 使能WOM中断 */
        ret = hal_i2c_WriteByte(I2C_ADDR, MPU_REG_INTEN_CFG, MPU_WOM_INTSTAT_MASK);   
        if (ret == FALSE) return FALSE;
        GSEN_OP_DELAY(5);   
    }else if(s_dev_type == DEV_ICM_20648){
        /* 在BANK2区设置 */
        SelectBankNum(BANK2);
        GSEN_OP_DELAY(5);   
        
         /* 设置碰撞检测ACCL阀值 */
        ret = hal_i2c_WriteByte(I2C_ADDR, ICM_REG_ACCEL_WOM_THR, setval);    
        if (ret == FALSE) return FALSE;
        GSEN_OP_DELAY(5);

        /* ACCEL_INTEL_EN=1，ACCEL_INTEL_MODE=1，使能WOM检测逻辑 */
        ret = hal_i2c_WriteByte(I2C_ADDR, ICM_REG_ACCEL_INTEL_CTRL,0x03); 
        if (ret == FALSE) return FALSE;
        GSEN_OP_DELAY(5);

        /* 在BANK0区设置 */
        ret = SelectBankNum(BANK0);
        if (ret == FALSE) return FALSE;
        GSEN_OP_DELAY(5);   

        /* 使能WOM中断 */
        ret = hal_i2c_WriteByte(I2C_ADDR, ICM_REG_INT_ENABLE, ICM_WOM_INTSTAT_MASK);   
        if (ret == FALSE) return FALSE;
        GSEN_OP_DELAY(5);

        /* 读一次，清原有中断标志，读取后，传感器自动清除状态 */
        rch = hal_i2c_ReadByte(I2C_ADDR, ICM_REG_INT_STATUS);  
        if (rch == -1) return FALSE;
        GSEN_OP_DELAY(5);
        
        #if EN_DEBUG_GSEN > 0
        debug_printf_dir("\r\n<*****使能WOM中断(2),阈值:0x%x*****>\r\n",setval);
        #endif    
    }else{
        #if EN_DEBUG_GSEN > 0
        debug_printf_dir("\r\n<**** 无传感器 ****>\r\n");
        #endif
        return FALSE;
    } 

    /* 使能MCU中断脚 */
    hal_pio_SetPinIntMode(PIO_GYRINT, TRUE);

    return TRUE;
}
/********************************************************************************
**  函数名称:  dal_gsen_SetGyroPara
**  功能描述:  设置标定的角速度三轴偏移值
**  输入参数: [in] calib: 角速度三轴标定偏移值
**
**  返回参数:  TRUE，设置成功；FALSE，设置失败
********************************************************************************/
BOOLEAN dal_gsen_SetGyroPara(AXIS_DATA_T *calib)
{
    BOOLEAN ret;

    ret = TRUE;
    if (calib != NULL) {
        s_gyro_calib.axis_x = (INT32S)(calib->axis_x / (GYRO_CAL_RATIO));
        s_gyro_calib.axis_y = (INT32S)(calib->axis_y / (GYRO_CAL_RATIO));
        s_gyro_calib.axis_z = (INT32S)(calib->axis_z / (GYRO_CAL_RATIO));
    }
    s_status |= STAT_SET_GYRO;
    if ((s_status & STAT_INIT) == 0) return FALSE;

    // ret = WriteGyroCalibPara();

    return ret;
}

/********************************************************************************
**  函数名称:  dal_gsen_GetWorkState
**  功能描述:  获取g-sensor工作状态
**  输入参数:  None
**  返回参数:  0 未运行  1 已运行
********************************************************************************/
INT8U dal_gsen_GetWorkState(void)
{
    if ((s_status & STAT_INIT) != 0 && (s_status & STAT_LOWPWR) == 0) return 1;
    return 0;   // 未初始化或处于休眠状态
}
/********************************************************************************
**  函数名称:  dal_gsen_GetState
**  功能描述:  获取g-sensor设备工作状态
**  输入参数:  idx:传感器序号
**  返回参数:  TRUE:准备就绪    FALSE:未准备好
********************************************************************************/
BOOLEAN dal_gsen_GetState(INT8U idx)
{
	return dal_i2c_GetState(idx);
}

/********************************************************************************
**  函数名称:  dal_gsen_ReadAcclData
**  功能描述:  获取g-sensor三轴加速度值，单位mg
**  输入参数:  [in] axis_val：用于返回三轴加速度值
**  返回参数:  TRUE，获取成功；FALSE，获取失败
********************************************************************************/
BOOLEAN dal_gsen_ReadAcclData(AXIS_DATA_T *axis_val)
{
    FP32 tmp;

    if ((s_status & STAT_INIT) == 0) return FALSE;
    if (axis_val == NULL) return FALSE;

    tmp = s_accl_data.axis_x * ACCL_CAL_RATIO;                      // 转换为mg单位
    axis_val->axis_x = (INT32S)(tmp >= 0 ? tmp + 0.5 : tmp - 0.5);  // 考虑四舍五入
    tmp = s_accl_data.axis_y * ACCL_CAL_RATIO;
    axis_val->axis_y = (INT32S)(tmp >= 0 ? tmp + 0.5 : tmp - 0.5);
    tmp = s_accl_data.axis_z * ACCL_CAL_RATIO;
    axis_val->axis_z = (INT32S)(tmp >= 0 ? tmp + 0.5 : tmp - 0.5);

    return TRUE;
}
/********************************************************************************
**  函数名称:  dal_gsen_ReadHitAcclData
**  功能描述:  获取碰撞中断时刻g-sensor三轴加速度值,单位mg
**  输入参数:  [in] axis_val[3],用于返回三轴加速度值
**  返回参数:  TRUE，获取成功；FALSE，获取失败
********************************************************************************/
BOOLEAN dal_gsen_ReadHitAcclData(AXIS_DATA_T *axis_val)
{
    FP32 tmp;

    if ((s_status & STAT_INIT) == 0) return FALSE;
    if (axis_val == NULL) return FALSE;

    tmp = s_accl_hitdata.axis_x * ACCL_CAL_RATIO;                   // 转换为mg单位
    axis_val->axis_x = (INT32S)(tmp >= 0 ? tmp + 0.5 : tmp - 0.5);  // 考虑四舍五入
    tmp = s_accl_hitdata.axis_y * ACCL_CAL_RATIO;
    axis_val->axis_y = (INT32S)(tmp >= 0 ? tmp + 0.5 : tmp - 0.5);
    tmp = s_accl_hitdata.axis_z * ACCL_CAL_RATIO;
    axis_val->axis_z = (INT32S)(tmp >= 0 ? tmp + 0.5 : tmp - 0.5);

    return TRUE;
}
/********************************************************************************
**  函数名称:  dal_gsen_ReadGyroData
**  功能描述:  获取g-sensor三轴角速度值，单位1/1000 °/s
**  输入参数:  [in] axis_val：用于返回三轴角速度值
**  返回参数:  TRUE，获取成功；FALSE，获取失败
********************************************************************************/
BOOLEAN dal_gsen_ReadGyroData(AXIS_DATA_T *axis_val)
{
    FP32 tmp;
    INT16S calib_x, calib_y, calib_z;

    if ((s_status & STAT_INIT) == 0) return FALSE;
    if (axis_val == NULL) return FALSE;

    if ((s_status & STAT_CALIB) != 0) {         // 标定状态下不进行静态漂移修正
        calib_x = calib_y = calib_z = 0;
    } else {
        calib_x = s_gyro_calib.axis_x;
        calib_y = s_gyro_calib.axis_y;
        calib_z = s_gyro_calib.axis_z;
    }
    tmp = (s_gyro_data.axis_x - calib_x) * GYRO_CAL_RATIO;              // 转换为mg单位
    axis_val->axis_x = (INT32S)(tmp >= 0 ? tmp + 0.5 : tmp - 0.5);      // 考虑四舍五入
    tmp = (s_gyro_data.axis_y - calib_y) * GYRO_CAL_RATIO;
    axis_val->axis_y = (INT32S)(tmp >= 0 ? tmp + 0.5 : tmp - 0.5);
    tmp = (s_gyro_data.axis_z - calib_z)  * GYRO_CAL_RATIO;
    axis_val->axis_z = (INT32S)(tmp >= 0 ? tmp + 0.5 : tmp - 0.5);

    return TRUE;
}

/********************************************************************************
**  函数名称:  dal_gsen_ReadTempData
**  功能描述:  获取温度传感器值
**  输入参数:  [in] rval：用于返回温度值，单位:1/10℃
**  返回参数:  TRUE，获取成功；FALSE，获取失败
********************************************************************************/
BOOLEAN dal_gsen_ReadTempData(INT16S *rval)
{
    if ((s_status & (STAT_INIT | STAT_TEMP_EN)) != (STAT_INIT | STAT_TEMP_EN)) return FALSE;
    if (rval == NULL) return FALSE;
    *rval = (INT16S)(s_temper_data * TEMP_CAL_RATIO + 21*10);

    return TRUE;
}

/********************************************************************************
**  函数名称:  dal_gsen_Init
**  功能描述:  初始化g-sensor模块
**  输入参数:  无
**  返回参数:  无
********************************************************************************/
void dal_gsen_Init(void)
{
    BOOLEAN ret;
	#if 0
    INT8U chk_dev_cnt;
    INT8U temp_dev_type;
	#endif
    // s_status = 0;
    s_detect_int = FALSE;
    s_hitdata_valid  = FALSE;
    s_accldata_valid = FALSE;
    s_gyrodata_valid = FALSE;
    s_tempdata_valid = FALSE;
    s_hit_threshold  = 0;
    s_ct_busy        = 0;
    memset(&s_accl_hitdata, 0, sizeof(s_accl_hitdata));
    memset(&s_accl_data,    0, sizeof(s_accl_data));
    memset(&s_accl_calib,   0, sizeof(s_accl_calib));
    memset(&s_accl_tmpdata, 0, sizeof(s_accl_tmpdata));
    memset(&s_gyro_data,    0, sizeof(s_gyro_data));
    memset(&s_gyro_tmpdata, 0, sizeof(s_gyro_tmpdata));
    memset(&s_gyro_calib,   0, sizeof(s_gyro_calib));
    memset(&s_rcb, 0, sizeof(s_rcb));

	PowDownGsensor();
    GSEN_OP_DELAY(10);
	PowUpGsensor();
	GSEN_OP_DELAY(10);

	
	dal_i2c_sim_Init();		// 6aix
    /*ret = dal_i2c_sim_Open(I2C_IDX_0);
	//ST_I2C_OpenCom(I2C_IDX_0);
    GSEN_OP_DELAY(5);

    if (ret == TRUE) {
        s_status |= STAT_REINIT;
        s_scan_tmr = CreateTimer(GsenScanTmrProc);
        StartTimer(s_scan_tmr, GSEN_SCAN_PERIOD,true);

        #if EN_DEBUG_GSENTMR > 0
        s_gsen_tmr_running = TRUE;
        #endif

        s_rtype = TYPE_ACCL;

        #if EN_DEBUG_GSEN > 0
        debug_printf_dir("\r\n<*****传感器初始化I2C Open Ok*****>\r\n");
        #endif
        #if 0
        for(chk_dev_cnt = 0; chk_dev_cnt < 5; chk_dev_cnt++){
            if(DeviceTypeCheck(&temp_dev_type) == TRUE){
                #if EN_DEBUG_GSEN > 0
                debug_printf_dir("\r\n<***** 上电检验设备类型成功 *****>\r\n");
                #endif
                s_dev_type = temp_dev_type;
                break;
            }
        }
        if(chk_dev_cnt == 5){
            s_dev_type = DEV_NONE;
        }
        #endif
    }*/
	
	ret = dal_i2c_sim_Open(I2C_IDX_1);
	GSEN_OP_DELAY(5);
    if(ret == TRUE){
		#if EN_DEBUG_GSEN > 0
		debug_printf("<*****SD2058传感器初始化I2C Open Ok*****>");
		#endif
	}
}

/********************************************************************************
**  函数名称:  dal_gsen_GetDevInfor
**  功能描述:  获取g-sensor设备型号信息
**  输入参数:  无
**  返回参数:  设备型号信息字符串
********************************************************************************/
char *dal_gsen_GetDevInfor(void)
{
    if(s_dev_type == DEV_MPU_6500){
        return MPU_DEVICE_INFOR_STR;
    }else if(s_dev_type == DEV_ICM_20648){
        return ICM_DEVICE_INFOR_STR;
    }else{
        return "Erro Gsenser Version";
    }
}
/*****************************sd2058相关函数*********************************/
/*****************************************************************************
** 函数:   sd_ReadData
** 描述: 读多个字节
** 参数: noe
** 返回: TRUE成功 FALSE失败
*****************************************************************************/
BOOLEAN sd_ReadData(INT8U regaddr, INT8U *rbuf, INT8U rlen)
{
	I2C_OPPARA_T sdop_para;
    if (!dal_gsen_GetState(I2C_COM_SD2058)) {
        return FALSE;
    }

	sdop_para.addr		= regaddr;
	sdop_para.buf		= rbuf;
	sdop_para.devaddr	= SD2058_ADDR;
	sdop_para.dlen		= rlen;
	return hal_i2c_ReadData(I2C_COM_SD2058,&sdop_para);
}

/*****************************************************************************
** 函数:   sd_ReadByte
** 描述: 读单个字节
** 参数: noe
** 返回: TRUE成功 FALSE失败
*****************************************************************************/
BOOLEAN sd_ReadByte(INT8U regaddr, INT8U *rbuf)
{
    return sd_ReadData(regaddr, rbuf, 1);
}

/*****************************************************************************
** 函数:   sd_WriteData
** 描述: 写多个字节
** 参数: noe
** 返回: TRUE成功 FALSE失败
*****************************************************************************/
BOOLEAN sd_WriteData(INT8U regaddr, INT8U* wbuf, INT8U wlen)
{
	I2C_OPPARA_T sdop_para;
    if (!dal_gsen_GetState(I2C_COM_SD2058)) {
        return FALSE;
    }
	sdop_para.addr		= regaddr;
	sdop_para.buf		= wbuf;
	sdop_para.devaddr	= SD2058_ADDR;
	sdop_para.dlen		= wlen;
	hal_i2c_WriteData(I2C_COM_SD2058, &sdop_para);
	return  TRUE;
}

/*****************************************************************************
** 函数:   sd_WriteByte
** 描述: 写单个字节
** 参数: noe
** 返回: TRUE成功 FALSE失败
*****************************************************************************/
BOOLEAN sd_WriteByte(INT8U regaddr, INT8U wbuf)
{
	return sd_WriteData(regaddr, &wbuf, 1);
}

/*
*********************************************************************************
**                      定义端口中断回调函数
*********************************************************************************
*/
// 在底层端口中断ISR中调用
void GsenIntCalbakFunc(void) //PRIVATE_FUNC
{
    if (s_detect_int == TRUE) return;
    s_detect_int = TRUE;
    if ((s_status & STAT_HIT_THRS) != 0) {
        s_hitdata_valid = FALSE;
        StartReadSensorData(TYPE_ACCL, TRUE);
    }
    StartTimer(s_scan_tmr, GSEN_SCAN_PERIOD,true);
#if EN_DEBUG_GSENTMR > 0
    s_gsen_tmr_running = TRUE;
#endif
}

//------------------------------------------------------------------------------
/* End of File */
