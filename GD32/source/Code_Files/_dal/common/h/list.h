/********************************************************************************
**
** 文件名:     list.h
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
#ifndef H_MMI_LIST_H
#define H_MMI_LIST_H

#ifdef __cplusplus
extern "C"
{
#endif

#define LISTMEM     INT8U
#define LISTNODE    struct node

/*************************************************************************************************/
/*                           节点结构体                                                          */
/*************************************************************************************************/
typedef struct node{
    LISTNODE  *prv;
    LISTNODE  *next;
} NODE;

/*************************************************************************************************/
/*                           链表结构体                                                          */
/*************************************************************************************************/
typedef struct {
    LISTNODE  *head;
    LISTNODE  *tail;
    INT32U     item;
} LIST_T;

BOOLEAN  CheckList(LIST_T *pl);
BOOLEAN  InitList(LIST_T *pl);
INT32U   GetListItem(LIST_T *pl);
LISTMEM *GetListHead(LIST_T *pl);
LISTMEM *GetListTail(LIST_T *pl);
LISTMEM *ListNextEle(LISTMEM *pb);
LISTMEM *ListPrvEle(LISTMEM *pb);
LISTMEM *DelListEle(LIST_T *pl, LISTMEM *pb);
LISTMEM *DelListHead(LIST_T *pl);
LISTMEM *DelListTail(LIST_T *pl);
BOOLEAN  AppendListEle(LIST_T *pl, LISTMEM *pb);
BOOLEAN  InsertListHead(LIST_T *pl, LISTMEM *pb);
BOOLEAN  ConnectHeadTail(LIST_T *pl);
BOOLEAN  BInsertListEle(LIST_T *pl, LISTMEM *curpb, LISTMEM *inspb);
BOOLEAN  InitMemList(LIST_T *mempl, LISTMEM *addr, INT32U nblks, INT32U blksize);

#ifdef __cplusplus
}
#endif

#endif

