/******************************************************************************
**
** Filename:     os_qmsg.c
** Copyright:    
** Description:  该模块主要实现系统消息管理
**
-------------------------------------------------------------------------------
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#include "os_include.h"
#include "os_qmsg.h"


/*
********************************************************************************
* define config parameters
********************************************************************************
*/


/*
********************************************************************************
* define struct
********************************************************************************
*/
typedef struct {
    INT8U       flag;
    INT16U      readpos;
    INT16U      nummsg;
    OS_MSG_T    msgbox[OS_MAX_MSG];
} MCB_T;

/*
********************************************************************************
* define module variants
********************************************************************************
*/
static MCB_T s_mcb;

/*******************************************************************
** 函数名称:   MsgIsExist
** 函数描述:   测试在消息队列中是否存在指定的消息ID
** 参数:       [in]  msgid: 消息ID
** 返回:       true-在消息队列中存在,false-不在消息队列中存在
********************************************************************/
static BOOLEAN MsgIsExist(INT8U tskid, INT32U msgid)
{
    INT16U pos, i;

    pos = s_mcb.readpos;
    for (i = 0; i < s_mcb.nummsg; i++, pos++) {
        if (pos >= OS_MAX_MSG) {
            pos -= OS_MAX_MSG;
        }
        if ((s_mcb.msgbox[pos].msgid == msgid) && (s_mcb.msgbox[pos].tskid == tskid)) {
            return true;
        }
    }
    return false;
}

/*******************************************************************
** 函数名称:   OS_PostMsg
** 函数描述:   发送系统消息
** 参数:       [in]  msgid: 消息ID
**             [in]  para1: 消息参数1
**             [in]  para2: 消息参数2
** 返回:       成功返回true,失败返回false
********************************************************************/
BOOLEAN OS_PostMsg(INT8U tskid, INT32U msgid, INT32U para1, INT32U para2)
{
    INT16U pos;

    OS_ASSERT((s_mcb.nummsg < OS_MAX_MSG), RETURN_FALSE);
    
    pos = s_mcb.readpos + s_mcb.nummsg;
    if (pos >= OS_MAX_MSG) {
        pos -= OS_MAX_MSG;
    }
    s_mcb.msgbox[pos].tskid = tskid;
    s_mcb.msgbox[pos].msgid = msgid;
    s_mcb.msgbox[pos].para1 = para1;
    s_mcb.msgbox[pos].para2 = para2;
    if (s_mcb.flag == 0) {
        s_mcb.flag = 1;
        //PORT_PostCommonMsg();
    }
    s_mcb.nummsg++;
    return true;
}

/*******************************************************************
** 函数名称:   OS_PostMsgEx
** 函数描述:   发送消息,增强型接口
** 参数:       [in]  overlay: true-检查是否已有存在该消息，false-不检查
**             [in]  tskid:   消息所属任务ID
**             [in]  msgid:   系统消息ID
**             [in]  para1:   低32位消息参数
**             [in]  para2:   高32位消息参数
** 返回:       成功返回true,失败返回false
********************************************************************/
BOOLEAN OS_PostMsgEx(BOOLEAN overlay, INT8U tskid, INT32U msgid, INT32U para1, INT32U para2)
{
    if (overlay) {
        if (!MsgIsExist(tskid, msgid)) {
            return OS_PostMsg(tskid, msgid, para1, para2);
        }
        return true;
    } else {
        return OS_PostMsg(tskid, msgid, para1, para2);
    }
}

/*******************************************************************
** 函数名称:   OS_MsgSchedEntry
** 函数描述:   处理系统消息
** 参数:       无
** 返回:       无
********************************************************************/
void OS_MsgSchedEntry(void)
{
    INT8U tskid, ntsk;
    INT32U msgid, para1, para2, num, nmsg;
    OS_TSK_TBL_T *ptsk;
    OS_MSG_TBL_T *pmsg;
    
    num = s_mcb.nummsg;
    s_mcb.flag = 0;
    for (;;) {
        if (num == 0) {
            break;
        }
        tskid = s_mcb.msgbox[s_mcb.readpos].tskid;
        msgid = s_mcb.msgbox[s_mcb.readpos].msgid;
        para1 = s_mcb.msgbox[s_mcb.readpos].para1;
        para2 = s_mcb.msgbox[s_mcb.readpos].para2;
        if (++s_mcb.readpos >= OS_MAX_MSG) {
            s_mcb.readpos = 0;
        }
        s_mcb.nummsg--;
        num--;
        
        ntsk = OS_GetRegTskMax();
        if (tskid < ntsk) {
            ptsk = OS_GetRegTskInfo(tskid);
            pmsg = ptsk->pmsg;
            nmsg = ptsk->nmsg;
            
            if (msgid >= pmsg->msgid && msgid < pmsg->msgid + nmsg) {
                OS_ASSERT((pmsg[msgid - pmsg->msgid].msgid == msgid), RETURN_VOID);
                OS_ASSERT((pmsg[msgid - pmsg->msgid].tskid == tskid), RETURN_VOID);
                
                if (pmsg[msgid - pmsg->msgid].proc != 0) {
                    pmsg[msgid - pmsg->msgid].proc(tskid, msgid, para1, para2);
                }
            } else if (msgid == 0) {
                OS_ASSERT((pmsg[0].tskid == tskid), RETURN_VOID);
                
                if (pmsg[0].proc != 0) {
                    pmsg[0].proc(tskid, pmsg->msgid, para1, para2);
                }
            }
        }
    }
}

/*******************************************************************
** 函数名称:   OS_InitQMsg
** 函数描述:   初始化函数
** 参数:       无
** 返回:       无
********************************************************************/
void OS_InitQMsg(void)
{
    INT8U i, j, ntsk, ninit;
    OS_TSK_TBL_T *ptsk;
    OS_INIT_TBL_T *pinit;
    
    s_mcb.flag    = 0;
    s_mcb.readpos  = 0;
    s_mcb.nummsg  = 0;
    
    ntsk = OS_GetRegTskMax();
    for (i = 0; i < ntsk; i++) {
        ptsk = OS_GetRegTskInfo(i);
        pinit = ptsk->pinit;
        ninit = ptsk->ninit;
        
        for (j = 0; j < ninit; j++) {
            if (pinit->init != 0) {
                pinit->init();
            }
            pinit++;
        }
    }
}

