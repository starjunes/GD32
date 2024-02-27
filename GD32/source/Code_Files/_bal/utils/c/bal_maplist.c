/********************************************************************************
**
** 文件名:     YX_List.C
** 版权所有:   (c) 2007-2008 厦门雅迅网络股份有限公司
** 文件描述:   实现链表数据结构
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2007/04/17 | 陈从华 |  创建该文件
**| 2008/10/13 | 任赋   |  移植车台程序到该文件,接口使用原车台接口
*********************************************************************************/
#include "yx_includes.h"
#include "bal_maplist.h"

/*******************************************************************
** 函数名:     bal_CheckMapList
** 函数描述:   检测链表的有效性
** 参数:       [in] lp:      链表指针
**             [in] memptr:  映射表起始地址
**             [in] max_num: 最大记录数
** 返回:       有效返回true，无效返回false
********************************************************************/
BOOLEAN bal_CheckMapList(MAP_LIST_T *lp, INT8U *memptr, INT16U max_num)
{
    INT32U count;
    INT16S nextnum;
    MAPNODE *curnode;
	
    if (lp == 0) {
        return FALSE;
    }
    
    count = 0;
    nextnum = lp->head;
    while (nextnum != -1) {
        if (nextnum > max_num) {
            return FALSE;
        }
        
        if (++count > lp->item) {
            return FALSE;
        }
        
        curnode = (MAPNODE *)(memptr + nextnum * sizeof(MAPNODE));
        nextnum = curnode->next;
    }
    if (count != lp->item) {
        return FALSE;
    }
	
    count = 0;
    nextnum = lp->tail;
    while (nextnum != -1) {
        if (nextnum > max_num) {
            return FALSE;
        }
        
        if (++count > lp->item) {
            return FALSE;
        }
        curnode = (MAPNODE *)(memptr + nextnum * sizeof(MAPNODE));
        nextnum = curnode->prev;
    }
    if (count != lp->item) {
        return FALSE;
    }
    return TRUE;
}

/*******************************************************************
** 函数名:     bal_InitMapList
** 函数描述:   初始化链表
** 参数:       [in] lp:      链表指针
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN bal_InitMapList(MAP_LIST_T *lp)
{
    if (lp == 0) {
        return FALSE;
    }
	
    lp->head = -1;
    lp->tail = -1;
    lp->item = 0;
    return TRUE;
}

/*******************************************************************
** 函数名:     bal_MapListItem
** 函数描述:   获取链表节点个数
** 参数:       [in]  lp:        链表
** 返回:       链表节点个数
********************************************************************/
INT32U bal_MapListItem(MAP_LIST_T *lp)
{
    if (lp == 0) {
        return 0;
    } else {
        return (lp->item);
    }
}

/*******************************************************************
** 函数名:     bal_GetMapListHead
** 函数描述:   获取链表头节点
** 参数:       [in]  lp:        链表
** 返回:       返回记录位置编号，返回-1表示无头节点
********************************************************************/
INT16S bal_GetMapListHead(MAP_LIST_T *lp)
{
    if (lp == 0 || lp->item == 0) {
        return -1;
    } else {
        return (lp->head);
    }
}

/*******************************************************************
** 函数名:     bal_GetMapListTail
** 函数描述:   获取链表尾节点
** 参数:       [in]  lp:        链表
** 返回:       返回记录位置编号，返回-1表示无尾节点记录
********************************************************************/
INT16S bal_GetMapListTail(MAP_LIST_T *lp)
{
    if (lp == 0 || lp->item == 0) {
        return -1;
    } else {
        return (lp->tail);
    }
}

/*******************************************************************
** 函数名:     bal_MapListNextEle
** 函数描述:   获取指定节点的后一节点
** 参数:       [in] memptr:  映射表起始地址
**             [in] pos:     当前记录位置编号
** 返回:       返回下一个记录位置编号，返回-1表示无后节点记录
********************************************************************/
INT16S bal_MapListNextEle(INT8U *memptr, INT16S pos)
{
    MAPNODE *curnode;
	
    if (pos == -1) {
        return -1;
    }
    curnode = (MAPNODE *)(memptr + pos * sizeof(MAPNODE));
    return (curnode->next);
}

/*******************************************************************
** 函数名:     bal_MapListPrvEle
** 函数描述:   获取指定节点的前一节点
** 参数:       [in] memptr:  映射表起始地址
**             [in] pos:     当前记录位置编号
** 返回:       返回前一个记录位置编号，-1表示无前节点记录
********************************************************************/
INT16S bal_MapListPrvEle(INT8U *memptr, INT16S pos)
{
    MAPNODE *curnode;
	
    if (pos == -1) {
        return -1;
    }

    curnode = (MAPNODE *)(memptr + pos * sizeof(MAPNODE));
    return (curnode->prev);
}

/*******************************************************************
** 函数名:     bal_DelMapListEle
** 函数描述:   删除指定节点，并返回下一个记录的位置编号
** 参数:       [in] lp:  链表
**             [in] memptr:  映射表起始地址
**             [in] pos: 当前记录位置编号
** 返回:       返回记录pos的下一个记录位置编号，返回-1表示无后节点记录
********************************************************************/
INT16S bal_DelMapListEle(MAP_LIST_T *lp, INT8U *memptr, INT16S pos)
{
    MAPNODE *curnode, *prvnode, *nextnode;
    INT16S prevpos, nextpos;

    if (pos == -1) {
        return -1;
    }
    if (lp->item == 0) {
        return -1;
    }

    lp->item--;
    curnode  = (MAPNODE *)(memptr + pos * sizeof(MAPNODE));
    prevpos = curnode->prev;
    nextpos = curnode->next;
    
    if (prevpos == -1) {
        lp->head = nextpos;
    } else {
        prvnode  = (MAPNODE *)(memptr + prevpos * sizeof(MAPNODE));
        prvnode->next = nextpos;
    }
    
    if (nextpos == -1) {
        lp->tail = prevpos;
    } else {
        nextnode = (MAPNODE *)(memptr + nextpos * sizeof(MAPNODE));
        nextnode->prev = prevpos;
    }
    return nextpos;
}

/*******************************************************************
** 函数名:     bal_DelMapListHead
** 函数描述:   删除链表头节点
** 参数:       [in] lp:      链表
**             [in] memptr:  映射表起始地址
** 返回:       链表头节点; 如返回-1, 则表示不存在链表头节点
********************************************************************/
INT16S bal_DelMapListHead(MAP_LIST_T *lp, INT8U *memptr)
{
    INT16S pos;

    if (lp == 0 || lp->item == 0) {
        return -1;
    }
    pos = lp->head;
    bal_DelMapListEle(lp, memptr, pos);
    return pos;
}

/*******************************************************************
** 函数名:     bal_DelMapListTail
** 函数描述:   删除链表尾节点
** 参数:       [in] lp:      链表
**             [in] memptr:  映射表起始地址
** 返回:       链表头节点; 如返回-1, 则表示不存在链表尾节点
********************************************************************/
INT16S bal_DelMapListTail(MAP_LIST_T *lp, INT8U *memptr)
{
    INT16S pos;

    if (lp == 0 || lp->item == 0) {
        return -1;
    }
    pos = lp->tail;
    bal_DelMapListEle(lp, memptr, pos);
    return pos;
}

/*******************************************************************
** 函数名:     bal_AppendMapListEle
** 函数描述:   在链表尾上追加一个节点
** 参数:       [in] lp:      链表
**             [in] memptr:  映射表起始地址
**             [in] pos:     待追加节点编号
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN bal_AppendMapListEle(MAP_LIST_T *lp, INT8U *memptr, INT16S pos)
{
    MAPNODE *curnode, *tailnode;
    INT16S tailpos;

    if (lp == 0 || pos == -1) {
        return FALSE;
    }
    curnode  = (MAPNODE *)(memptr + pos * sizeof(MAPNODE));
    curnode->prev = lp->tail;
    if (lp->item == 0) {
        lp->head = pos;
    } else {
        tailpos = lp->tail;
        tailnode = (MAPNODE *)(memptr + tailpos * sizeof(MAPNODE));
        tailnode->next = pos;
    }
    curnode->next = -1;
    lp->tail = pos;
    lp->item++;
    return TRUE;
}

/*******************************************************************
** 函数名:     bal_InsertMapListHead
** 函数描述:   在链表头插入一个节点
** 参数:       [in] lp:      链表
**             [in] memptr:  映射表起始地址
**             [in] pos:     待插入的节点位置编号
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN bal_InsertMapListHead(MAP_LIST_T *lp, INT8U *memptr, INT16S pos)
{
    MAPNODE *curnode, *headnode;
    INT16S headpos;

    if (lp == 0 || pos == -1) {
        return FALSE;
    }

    curnode  = (MAPNODE *)(memptr + pos * sizeof(MAPNODE));
    curnode->next = lp->head;
    if (lp->item == 0) {
        lp->tail = pos;
    } else {
        headpos = lp->head;
        headnode = (MAPNODE *)(memptr + headpos * sizeof(MAPNODE));
        headnode->prev = pos;
    }
    curnode->prev = -1;
    lp->head = pos;
    lp->item++;
    return TRUE;
}

/*******************************************************************
** 函数名:     bal_BInsertMapListEle
** 函数描述:   在指定节点前插入一个新节点
** 参数:       [in] lp:      链表
**             [in] memptr:  映射表起始地址
**             [in] curpos:  当前节点位置编号
**             [in] inspos:  待插入的节点位置编号
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN bal_BInsertMapListEle(MAP_LIST_T *lp, INT8U *memptr, INT16S curpos, INT16S inspos)
{
    MAPNODE *curnode, *insnode, *prenode;
    INT16S prevpos;
	
    if (lp == 0 || curpos == -1 || inspos == -1) {
        return FALSE;
    }
    if (lp->item == 0) {
        return FALSE;
    }
    curnode = (MAPNODE *)(memptr + curpos * sizeof(MAPNODE));
    insnode = (MAPNODE *)(memptr + inspos * sizeof(MAPNODE));

    insnode->next = curpos;
    insnode->prev = curnode->prev;
    if (curnode->prev == -1){
        lp->head = inspos;
    } else {
        prevpos = curnode->prev;
        prenode = (MAPNODE *)(memptr + prevpos * sizeof(MAPNODE));
        prenode->next = inspos;
    }
    curnode->prev = inspos;
    lp->item++;
    return TRUE;
}

/*******************************************************************
** 函数名:     bal_AInsertMapListEle
** 函数描述:   在指定节点前插入一个新节点
** 参数:       [in] lp:      链表
**             [in] memptr:  映射表起始地址
**             [in] curpos:  当前节点位置编号
**             [in] inspos:  待插入的节点位置编号
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN bal_AInsertMapListEle(MAP_LIST_T *lp, INT8U *memptr, INT16S curpos, INT16S inspos)
{
    MAPNODE *curnode, *insnode, *nextnode;
    INT16S nextpos;

    if (lp == 0 || curpos == -1 || inspos == -1) {
        return FALSE;
    }
    if (lp->item == 0) {
        return FALSE;
    }
    curnode = (MAPNODE *)(memptr + curpos * sizeof(MAPNODE));
    insnode = (MAPNODE *)(memptr + inspos * sizeof(MAPNODE));
    
    insnode->next = curnode->next;
    insnode->prev = curpos;
    if(curnode->next == -1) {
        lp->tail = inspos;
    } else {
        nextpos = curnode->next;
        nextnode = (MAPNODE *)(memptr + nextpos * sizeof(MAPNODE));
        nextnode->prev = inspos;
    }
    curnode->next = inspos;
    lp->item++;
    return TRUE;
}

/*******************************************************************
** 函数名:     bal_InitMemMapList
** 函数描述:   将一块内存初始化成链表缓冲区
** 参数:       [in] lp:      链表
**             [in] memptr:  映射表起始地址
**             [in] max_num: 最大记录数
** 返回:       成功返回true，失败返回false
********************************************************************/
BOOLEAN bal_InitMemMapList(MAP_LIST_T *lp, INT8U *memptr, INT16U max_num)
{
    INT16U i;
    
    if (!bal_InitMapList(lp)) {
        return FALSE;
    }

    for(i = 0; i < max_num; i++) {
        if (!bal_AppendMapListEle(lp, memptr, i)) {
            return FALSE;
        }
    }
    return TRUE;
}
