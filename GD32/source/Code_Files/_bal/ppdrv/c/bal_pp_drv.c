/*******************************************************************************
**
** 文件名:     bal_pp_drv.c
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:    该模块主要实现公共参数文件存储驱动管理
**
********************************************************************************
**             修改历史记录
**==============================================================================
**| 日期       | 作者   |  修改记录
**==============================================================================
**| 2017/06/07 | 谢金成 |  创建第一版本
*******************************************************************************/
//#include "yx_includes.h"
#include "bal_pp_drv.h"
//#include "bal_pp_reg.h"
//#include "port_nvrec.h"
/*
********************************************************************************
* 定义模块配置参数
********************************************************************************
*/
//#define VALID_               0x08
//#define FALUT_               0x00

/*
********************************************************************************
* 定义模块静态变量
********************************************************************************
*/
#if 0
static    const   PP_REG_T  *s_preg;
static    INT8U   s_startpos = PP_RECNUM; //计数的起始端从 头 结束 位置开始
static    INT8U   s_endpos   = PP_RECNUM;

#if DEBUG_PUBPARA >  0
static INT8U s_testpptmrid;
static  void test_pp_rw(void *pdata)
{
	static AXISCALCONFIG_T  s_ax={-1,-2,-900};
	static AXISCALCONFIG_T  s_ax1={0x11111111,0x22222222,0x33333333};

	bal_pp_StoreParaByID(AXISCALPARA_,(INT8U *)&s_ax, sizeof(AXISCALCONFIG_T));
	bal_pp_ReadParaByID(AXISCALPARA_, (INT8U *)&s_ax1, sizeof(AXISCALCONFIG_T));
	printf_hex((INT8U *)&s_ax1 , sizeof(AXISCALCONFIG_T));
}
#endif
#endif
/*******************************************************************************
** 函数名:      bal_pp_Init
** 函数描述:    公共参数存储驱动初始化
** 参数:        无
** 返回:        无
*******************************************************************************/
void bal_pp_Init(void)
{
#if 0
	INT8U *mptr;
    INT8U i=0,m=0,maxid=0,chksum=0;
	INT8U mrecd=0,nrecd=0;
    INT16U nvlen=0; //获取表头的len
    BOOLEAN chklenflg = true;

    #if DEBUG_PUBPARA >  0
	INT8U testary[10*32]={0};
    debug_printf("<bal_pp_Init>\r\n");
    PORT_WriteNvRecord(PP_HEAD_START, 10, testary);
    #endif

    maxid = bal_pp_GetRegPPMax();
    if(maxid > MAX_PP_NUM ) { //如果注册的参数大于定义的最大个数，舍去后面的
    	maxid = MAX_PP_NUM ;
    }

    mptr = PORT_Malloc(PP_RECNUM * NV_RECORD_LEN);
    PORT_ReadNvRecord(PP_HEAD_START, PP_RECNUM, mptr);//读出存储表头
	chksum = bal_u_chksum_1(mptr+1, PP_RECNUM * NV_RECORD_LEN-2);

    for(m=0; m < maxid; m++) { //校验长度和记录号的正确性
        s_preg = bal_pp_GetRegInfo(m);
        nvlen  = mptr[1 + (m * PP_HEAD_LEN + 5)];
        if(nvlen != s_preg->ppsize) {
            chklenflg = false;
            break;
        }

        if((((mptr[1 + (m * PP_HEAD_LEN + 4)])- (mptr[1 + (m * PP_HEAD_LEN + 3)]))*32) < s_preg->ppsize) {
            chklenflg = false;
            break;
        }
    }

    if((chksum == mptr[0]) && (true == chklenflg)) { //头部检验正确
//		printf_hex(mptr, PP_RECNUM * NV_RECORD_LEN);
    } else { //头部检验错误，重新写入表头
        for(i=0; i <maxid; i++) {
            s_preg = bal_pp_GetRegInfo(i);
        	mptr[1 + i*PP_HEAD_LEN] = s_preg->id;
        	mptr[2 + i*PP_HEAD_LEN] = FALUT_;
        	mptr[3 + i*PP_HEAD_LEN] = 0;

        	s_startpos = s_endpos;
        	mrecd = (s_preg->ppsize + NV_RECORD_LEN) / NV_RECORD_LEN ; //除数
        	nrecd = s_preg->ppsize % NV_RECORD_LEN ;                   //余数
        	s_endpos += mrecd;
        	if(nrecd >= (NV_RECORD_LEN -3)) { //如果接近标准记录长度，多开辟一条记录的空间
        		s_endpos += 1;
        	}
        	mptr[4 + i*PP_HEAD_LEN] = s_startpos;
        	mptr[5 + i*PP_HEAD_LEN] = s_endpos;
        	mptr[6 + i*PP_HEAD_LEN] = s_preg->ppsize;
        	mptr[7 + i*PP_HEAD_LEN] = 0; //预留字节
        }
        mptr[0] = bal_u_chksum_1(mptr+1, PP_RECNUM * NV_RECORD_LEN-2);
        PORT_WriteNvRecord(PP_HEAD_START, PP_RECNUM, mptr);
	}
	if(mptr != NULL) {
		PORT_Free(mptr);
	}

#if DEBUG_PUBPARA >  0
	s_testpptmrid = OS_InstallTmr(PRIO_COMMONTASK, 0, test_pp_rw);
    OS_StartTmr(s_testpptmrid, SECOND, 5);
#endif
#endif
}
/*******************************************************************************
** 函数名:      bal_pp_ReadParaByID
** 函数描述:    读取PP参数，判断PP有效性
** 参数:        [in] id:  参数编号，见PP_ID_E
**              [out] dptr:输出缓存
**              [out] rlen:缓存长度
** 返回:        有效返回true，无效返回false
*******************************************************************************/
INT8U bal_pp_ReadParaByID(INT8U id, INT8U *dptr, INT16U rlen)
{
    return ReadPubPara(id, dptr);
}

/*******************************************************************
** 函数名:      bal_pp_StoreParaByID
** 函数描述:    存储PP参数，延时2秒存储到flash
** 参数:        [in] id:  参数编号，见PP_ID_E
**              [in] sptr:输入缓存
**              [in] slen:缓存长度
** 返回:        成功返回true，失败返回false
********************************************************************/
BOOLEAN bal_pp_StoreParaByID(INT8U id, INT8U *sptr, INT16U slen)
{
    StorePubPara(id, sptr);
    return true;
}
