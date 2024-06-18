/**************************************************************************************************
**                                                                                               **
**  文件名称:  Infrared.H                                                                        **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2010                                    **
**  创建信息:  jump -- 2011年1月20日                                                             **
**  文件描述:  红外遥控接口                                                                      **
**  ===========================================================================================  **
**  修改信息:  单击此处添加....                                                                  **
**************************************************************************************************/


#ifndef  DEF_INFRARED
#define  DEF_INFRARED

#ifdef   INFRARED_GLOBALS
#define  INFRARED_EXT
#else
#define  INFRARED_EXT  extern
#endif


#define	START_FLAG	1
#define BEGIN_FLAG	2
#define	DATA_FLAG	3

//#define	OSC_4L		2*1185	//四分频的时候，TH1 TL1从0000变成FFFF总共需要11.85mS
//#define	OSC_16L		2*4741	//十六分频时，TH1 TL1从0000变成FFFF总共需要47.41mS
//#define	OSC_64L		2*18963	//六四分频时，TH1 TL1从0000变成FFFF总共需要189.63mS

void dal_CanSeqCFSendCallbakFunc(void (* handle) (void));
void dal_CanSTminTimeoutCallbakFunc(void (* handle) (void));
void   InitFrKb(void);
#endif
