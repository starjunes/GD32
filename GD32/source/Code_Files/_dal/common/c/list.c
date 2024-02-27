/********************************************************************************
**
** 文件名:     list.c
** 版权所有:   (c) 2014-2020 厦门雅迅网络股份有限公司
** 文件描述:   链表结构
**
*********************************************************************************
**             修改历史记录
**===============================================================================
**| 日期       | 作者   |  修改记录
**===============================================================================
**| 2014/08/21 | 张鹏   |  创建该文件
**
*********************************************************************************/
#include "dal_include.h"
#include "list.h"

/**************************************************************************************************
**  函数名称:  CheckList
**  功能描述:  检查链表
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
BOOLEAN CheckList(LIST_T *pl)
{
    INT32U    count;
    LISTNODE *curnode;
    
    if (pl == 0) return false;
    
    count = 0;
    curnode = pl->head;
    while(curnode != 0) {
        if (++count > pl->item) return false;
        curnode = curnode->next;
    }
    if (count != pl->item) return false;
    
    count = 0;
    curnode = pl->tail;
    while(curnode != 0) {
        if (++count > pl->item) return false;
        curnode = curnode->prv;
    }
    if (count != pl->item) {
        return false;
    } else {
        return true;
    }
}

/**************************************************************************************************
**  函数名称:  InitList
**  功能描述:  初始化链表
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
BOOLEAN InitList(LIST_T *pl)
{
     if (pl == 0) return false;
    
     pl->head = 0;
     pl->tail = 0;
     pl->item = 0;
    
     return true;
}

/**************************************************************************************************
**  函数名称:  GetListItem
**  功能描述:  获取链表总数
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
INT32U GetListItem(LIST_T *pl)
{
     if (pl == 0) return 0;
     return pl->item;
}

/**************************************************************************************************
**  函数名称:  GetListHead
**  功能描述:  获取链表头节点
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
LISTMEM *GetListHead(LIST_T *pl)
{
    if (pl == 0 || pl->item == 0) {
        return 0;
    } else {
        return ((LISTMEM *)pl->head + sizeof(NODE));
    }
}

/**************************************************************************************************
**  函数名称:  GetListTail
**  功能描述:  获取链表尾节点
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
LISTMEM *GetListTail(LIST_T *pl)
{
    if (pl == 0 || pl->item == 0) {
        return 0;
    } else {
        return ((LISTMEM *)pl->tail + sizeof(NODE));
    }
}

/**************************************************************************************************
**  函数名称:  ListNextEle
**  功能描述:  获取链表下一个元素
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
LISTMEM *ListNextEle(LISTMEM *pb)
{
    LISTNODE *curnode;
    
    if (pb == 0) return 0;
    curnode = (LISTNODE *)(pb - sizeof(NODE));
    if ((curnode = curnode->next) == 0) {
        return 0;
    } else {
        return ((LISTMEM *)curnode + sizeof(NODE));
    }
}

/**************************************************************************************************
**  函数名称:  ListPrvEle
**  功能描述:  获取链表上一个元素
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
LISTMEM *ListPrvEle(LISTMEM *pb)
{
    LISTNODE *curnode;

    if (pb == 0) return 0;
    curnode = (LISTNODE *)(pb - sizeof(NODE));
    if ((curnode = curnode->prv) == 0) {
        return 0;
    } else {
        return ((LISTMEM *)curnode + sizeof(NODE));
    }
}

/**************************************************************************************************
**  函数名称:  DelListEle
**  功能描述:  从链表中删除指定节点
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
LISTMEM *DelListEle(LIST_T *pl, LISTMEM *pb)
{
    LISTNODE *curnode, *prvnode, *nextnode;

    if (pl == 0 || pb == 0) return 0;
    if (pl->item == 0) return 0;
    ENTER_CRITICAL();
    pl->item--;
    curnode  = (LISTNODE *)(pb - sizeof(NODE));
    prvnode  = curnode->prv;
    nextnode = curnode->next;
    if (prvnode == 0) {
        pl->head = nextnode;
    } else {
        prvnode->next = nextnode;
    }
    if (nextnode == 0) {
        pl->tail = prvnode;
        EXIT_CRITICAL();
        return 0;
    } else {
        nextnode->prv = prvnode;
        EXIT_CRITICAL();
        return ((LISTMEM *)nextnode + sizeof(NODE));
    }
}

/**************************************************************************************************
**  函数名称:  DelListHead
**  功能描述:  删除链表头节点
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
LISTMEM *DelListHead(LIST_T *pl)
{
    LISTMEM *pb;

    if (pl == 0 || pl->item == 0) return 0;
    ENTER_CRITICAL();
    pb = (LISTMEM *)pl->head + sizeof(NODE);
    DelListEle(pl, pb);
    EXIT_CRITICAL(); 
    return pb;
}

/**************************************************************************************************
**  函数名称:  DelListTail
**  功能描述:  删除链表尾节点
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
LISTMEM *DelListTail(LIST_T *pl)
{
    LISTMEM *pb;

    if (pl == 0 || pl->item == 0) return 0;

    pb = (LISTMEM *)pl->tail + sizeof(NODE);
    DelListEle(pl, pb);
    return pb;
}

/**************************************************************************************************
**  函数名称:  AppendListEle
**  功能描述:  增加链表节点
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
BOOLEAN AppendListEle(LIST_T *pl, LISTMEM *pb)
{
    LISTNODE *curnode;

    if (pl == 0 || pb == 0) return false;
    ENTER_CRITICAL();
    curnode = (LISTNODE *)(pb - sizeof(NODE));
    curnode->prv = pl->tail;
    if (pl->item == 0) {
        pl->head = curnode;
    } else {
        pl->tail->next = curnode;
    }
    curnode->next = 0;
    pl->tail = curnode;
    pl->item++;
    EXIT_CRITICAL();
    return true;
}

/**************************************************************************************************
**  函数名称:  InsertListHead
**  功能描述:  插入节点到链表头
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
BOOLEAN InsertListHead(LIST_T *pl, LISTMEM *pb)
{
    LISTNODE *curnode;

    if (pl == 0 || pb == 0) return false;

    curnode = (LISTNODE *)(pb - sizeof(NODE));
    curnode->next = pl->head;
    if (pl->item == 0) {
        pl->tail = curnode;
    } else {
        pl->head->prv = curnode;
    }
    curnode->prv = 0;
    pl->head = curnode;
    pl->item++;
    return true;
}

/**************************************************************************************************
**  函数名称:  ConnectHeadTail
**  功能描述:  将链表头尾连接起来
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
BOOLEAN ConnectHeadTail(LIST_T *pl)
{
    pl->head->prv  = pl->tail;
    pl->tail->next = pl->head;
    return true;
}

/**************************************************************************************************
**  函数名称:  BInsertListEle
**  功能描述:  在指定节点前插入节点
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
BOOLEAN BInsertListEle(LIST_T *pl, LISTMEM *curpb, LISTMEM *inspb)
{
    LISTNODE *curnode, *insnode;
    
    if (pl == 0 || curpb == 0 || inspb == 0) return false;
    if (pl->item == 0) return false;

    curnode  = (LISTNODE *)(curpb - sizeof(NODE));
    insnode  = (LISTNODE *)(inspb - sizeof(NODE));

    insnode->next = curnode;
    insnode->prv  = curnode->prv;
    if (curnode->prv == 0){
        pl->head = insnode;
    } else {
        curnode->prv->next = insnode;
    }
    curnode->prv = insnode;
    pl->item++;
    return true;
}

/**************************************************************************************************
**  函数名称:  InitMemList
**  功能描述:  初始化链表内存
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
BOOLEAN InitMemList(LIST_T *mempl, LISTMEM *addr, INT32U nblks, INT32U blksize)
{
    if (!InitList(mempl)) return false;

    addr += sizeof(NODE);
    for(; nblks > 0; nblks--){
        if (!AppendListEle(mempl, addr)) return false;
        addr += blksize;
    }
    return true;
}

