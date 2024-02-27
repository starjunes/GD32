/**************************************************************************************************
**                                                                                               **
**  文件名称:  DataBase.C                                                                        **
**  版权所有:  CopyRight ⊙ Xiamen Yaxon NetWork CO.LTD. 2010                                    **
**  创建信息:  jump -- 2011年1月20日                                                             **
**  文件描述:  数据库底层管理                                                                    **
**  ===========================================================================================  **
**  修改信息:  recsize原先为INT8U，即不得大于255，后根据需要修改为INT16U                         **
**             但是单一表项仍不可大于255 (FIELD_STRUCT中len为INT8U)                              **
**************************************************************************************************/

#define DATABASE_C
#include "app_include.h"
#include "dal_include.h"
#include "tools.h"
#include "s_flash.h"
#include "list.h"
#include "database.h"

/*
********************************************************************************
* define DB_STRUCT
********************************************************************************
*/
/* sizeof(DB_STRUCT) = 32 */
typedef struct {
    LIST_T          usedlist;
    LIST_T          freelist;
    INT16U          recsize;        /* 一个记录的大小 */
    INT8U           maxrecord;      /* 最大记录数 */
    INT8U           ordermode;      /* 排序规则 */
    INT8U           orderfield;     /* 用来排序的表项 */
    INT8U           maxfield;       /* 一个记录总共几个表项 */
    INT8U           chksum;         /* 用累加和计算 */
} DB_STRUCT;


/*
********************************************************************************
* define module variants
********************************************************************************
*/
//static DB_STRUCT    *image_db;    /* database在内存映射中的地址 */
//static FIELD_STRUCT *image_field;
//static INT8U        *image_rec;
//static INT16U readpara;
//static INT8U maxnum;

#if EN_CHECKFLASH > 0
static INT8U flashcheck[50];


/**************************************************************************************************
**  函数名称:  setflashflag
**  功能描述:  置/清除闪存标识，用于存储数据的保护，若checkflashflag()发现没有置标识，则不允许修改存储数据。
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
void setflashflag(void)
{
    INT8U i;
    for(i = 0; i < sizeof(flashcheck); i++) {
        flashcheck[i] = i;
    }
}

void clearflashflag(void)
{
    INT8U i;
    for (i = 0; i < sizeof(flashcheck); i++) {
        flashcheck[i] = 0x00;
    }
}

BOOLEAN checkflashflag(void)
{
    INT8U i;
    for (i = 0; i < sizeof(flashcheck); i++) {
        if (flashcheck[i] != i) {
            return FALSE;
        }
    }
    return TRUE;				
}
#endif

#if 0
/**************************************************************************************************
**  函数名称:  GetFieldOffset
**  功能描述:  获取记录中第numfield个表项的所在位置偏移量，参考结构体UNIMSG
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
static INT16U GetFieldOffset(INT8U numfield, FIELD_STRUCT *field)
{
    INT16U offset = 0;
    
    for (; numfield > 0; field++, numfield--) {
        offset += field->len;
        if ((field->attrib & FIELD_FIXED) == 0) {
            offset++;
        }
    }
    return offset;
}

/**************************************************************************************************
**  函数名称:  GetImageDBPara
**  功能描述:  给全局变量赋值，初始 image_db、image_field、image_rec的位置。
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
static void GetImageDBPara(INT16U offset)
{
    image_db    = (DB_STRUCT *)(flash_image_mem + offset);      /* 镜像地址加偏移量。 */
    image_field = (FIELD_STRUCT *)((INT8U*)image_db + sizeof(DB_STRUCT));
    image_rec   = (INT8U*)image_field + image_db->maxfield * sizeof(FIELD_STRUCT);/* 第一个记录地址 */
}

/**************************************************************************************************
**  函数名称:  ReadDB
**  功能描述:  读取闪存到flash_image_mem[4224]中，记录读取的类型和扇区，并调用GetImageDBPara()初始化。
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
static void ReadDB(INT8U type, INT16U nsector,INT16U offset)
{
    InitImageSram(nsector, type, flash_image_mem);              /* init image memory */
    readpara = nsector;
    GetImageDBPara(offset);
}

/**************************************************************************************************
**  函数名称:  WriteDB
**  功能描述:  写入闪存
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
void WriteDB(INT16U nsector, INT16U offset)
{
    if (readpara != nsector) {
        return;
    }
    image_db->chksum = Getchksum((INT8U*)image_db, sizeof(DB_STRUCT) - 2) + nsector + (nsector >> 8) + offset + (offset >> 8); //由于是32位系统，系统在DB_STRUCT后面补了两个字节。
    
    UpdateImageFlash(nsector, flash_image_mem);                   /* update flash */
    readpara = 0xffff;
}


/**************************************************************************************************
**  函数名称:  DBListHead
**  功能描述:  读取记录链表头
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
static INT8U *DBListHead(void)
{
    return GetListHead(&image_db->usedlist);
}


/**************************************************************************************************
**  函数名称:  DBListNextEle
**  功能描述:  读取下一个记录链表指针
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
static INT8U *DBListNextEle(INT8U *eleptr)
{
    return ListNextEle(eleptr);
}

/**************************************************************************************************
**  函数名称:  GetRecord
**  功能描述:  找到第numrec条记录,numrec越小越新
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
static INT8U *GetRecord(INT8U numrec)
{
    INT8U *ptr;
    
    ptr = DBListHead();
    for (;;) {
        if (ptr == 0 || numrec == 0) {
            break;
        }
        ptr = DBListNextEle(ptr);
        numrec--;
    }
    return ptr;
}


/**************************************************************************************************
**  函数名称:  CmpChar
**  功能描述:  比较字符大小写
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
static INT8U CmpChar(BOOLEAN matchcase, INT8U ch1, INT8U ch2)
{
    if (!matchcase) {
        ch1 = UpperChar(ch1);
        ch2 = UpperChar(ch2);
    }
    if (ch1 > ch2) {
        return STR_GREAT;
    } else if (ch1 < ch2) {
        return STR_LESS;
    } else {
        return STR_EQUAL;
    }
}
/**************************************************************************************************
**  函数名称:  ACmpString
**  功能描述:  比较字符串
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
static INT8U ACmpString(BOOLEAN matchcase, INT8U *ptr1, INT8U *ptr2, INT8U len1, INT8U len2)
{
    INT8U result;
    
    for (;;) {
        if (len1 == 0 && len2 == 0) {
            return STR_EQUAL;
        }
        if (len1 == 0) {
            return STR_LESS;
        }
        if (len2 == 0) {
            return STR_GREAT;
        }
        result = CmpChar(matchcase, *ptr1++, *ptr2++);
        if (result != STR_EQUAL) {
            return result;
        } else {
            len1--;
            len2--;
        }
    }
}
/**************************************************************************************************
**  函数名称:  SearchRecord
**  功能描述:  搜索记录
**  输入参数:  
**  返回参数:  每找不到匹配记录，则返回0、recordnum = 0xff
**************************************************************************************************/
static INT8U *SearchRecord(INT8U *recordnum, INT8U findmode, INT8U findresult, INT8U fieldnum, INT8U len, INT8U *ptr)
{
    BOOLEAN matchcase;
    INT8U   *eleptr, *cmpptr;
    INT8U   cmplen;
    INT16U  offset;
    
    if (findmode & FIND_MATCHCASE) {
        matchcase = TRUE;
    } else {
        matchcase = FALSE;
    }

    offset     = GetFieldOffset(fieldnum, image_field);
    *recordnum = 0;
    eleptr     = DBListHead();
    for (;;) {
        if (eleptr == 0) {
            break;
        }
        cmpptr = eleptr + offset;
        if (image_field[fieldnum].attrib & FIELD_FIXED) {
            cmplen = image_field[fieldnum].len;
        } else {
            cmplen = *cmpptr++;
        }
        if ((findmode & FIND_HALFMATCH) && len < cmplen) {
            cmplen = len;
        }
        if ((findmode & FIND_REVERSE) && cmplen > len) {
            cmpptr += (cmplen - len);
            cmplen  = len;
        }
        
        if (ACmpString(matchcase, cmpptr, ptr, cmplen, len) == findresult) {
            return eleptr;
        }
        eleptr = DBListNextEle(eleptr);
        *recordnum = *recordnum + 1;
        if (*recordnum > maxnum) {
            break;
        }
        ClearWatchdog();
    }
    *recordnum = 0xff;
    return 0;
}

/**************************************************************************************************
**  函数名称:  DB_Init
**  功能描述:  给image_db、image_field赋值  //增加一个type,来表示type类型
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
void DB_Init(INT8U type, INT16U nsector, INT16U offset, BOOLEAN needclear, INT8U maxfield, FIELD_STRUCT *field, INT8U maxrecord, INT8U ordermode, INT8U orderfield)
{
    INT16U recsize;
    INT8U  *ptr;
    INT8U  i;
    
    ReadDB(type, nsector, offset);
    //image_db    = (DB_STRUCT   *)(flash_image_mem + offset);
    //image_field = (FIELD_STRUCT   *)((INT8U   *)image_db + sizeof(DB_STRUCT));
    image_rec   = (INT8U*)image_field + maxfield * sizeof(FIELD_STRUCT);
    memcpy(image_field, field, maxfield * sizeof(FIELD_STRUCT));
    
    image_db->recsize    = recsize = GetFieldOffset(maxfield, image_field);
    image_db->maxrecord  = maxrecord;
    image_db->ordermode  = ordermode;
    image_db->orderfield = orderfield;
    image_db->maxfield   = maxfield;
    
    InitList(&image_db->usedlist);
    InitMemList(&image_db->freelist, image_rec, maxrecord, recsize + sizeof(NODE));/* maxrecord不得大于255 */
    
    if (needclear) {
        for (i = 0; i < maxrecord; i++) {
            ptr = DelListHead(&image_db->freelist);
            if (ptr == 0) {
                break;
            }
            memset(ptr, 0, recsize);
            AppendListEle(&image_db->freelist, ptr);
        }
    }
    WriteDB(nsector, offset);                                        /* write database */
}
/*
//删除整张表
void DB_Del(INT8U type, INT16U nsector, INT16U offset)
{
    ReadDB(type, nsector,offset);//GetImageDBPara(offset);
    DB_Init(type, nsector, offset, FALSE, image_db->maxfield, image_field, image_db->maxrecord, image_db->ordermode, image_db->orderfield);
}
*/
/**************************************************************************************************
**  函数名称:  DB_IsValid
**  功能描述:  检查DB校验码
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
BOOLEAN DB_IsValid(INT8U type, INT16U nsector, INT16U offset, INT8U maxfield, FIELD_STRUCT *field, INT8U maxrecord, INT8U ordermode, INT8U orderfield)
{
    INT8U chksum, chksum_real;
    INT8U imagebuf_db[30], *ptr;
    
    if (!ImageFlashValid(nsector)) {
        return FALSE;
    }
    
    ReadDB(type, nsector, offset);                                   /* GetImageDBPara(offset); */
    ptr = (INT8U*)image_db;
    chksum = Getchksum(ptr, sizeof(DB_STRUCT) - 2) + nsector + (nsector >> 8) + offset + (offset >> 8);/* 由于是32位系统，系统在DB_STRUCT后面补了两个字节。 */
    if (ACmpString(true, (INT8U *)image_field, (INT8U *)field, image_db->maxfield * sizeof(FIELD_STRUCT), maxfield * sizeof(FIELD_STRUCT)) != STR_EQUAL) {
        return FALSE;
    }
    if ((image_db->freelist.item != 0) && (((INT32U)image_db->freelist.head - (INT32U)image_rec) % (image_db->recsize + sizeof(NODE)) != 0)) {
        return FALSE;
    }
    if ((image_db->usedlist.item != 0) && (((INT32U)image_db->usedlist.head - (INT32U)image_rec) % (image_db->recsize + sizeof(NODE)) != 0)) {
        return FALSE;
    }
    
    memcpy(imagebuf_db, ptr, 24);                                    /* 将现在的数据库结构体求检验和进行比较 */
    imagebuf_db[24] = GetFieldOffset(maxfield, field);
    imagebuf_db[25] = GetFieldOffset(maxfield, field) >> 8;
    imagebuf_db[26] = maxrecord;
    imagebuf_db[27] = ordermode;
    imagebuf_db[28] = orderfield;
    imagebuf_db[29] = maxfield;
    chksum_real = Getchksum(imagebuf_db, sizeof(imagebuf_db)) + nsector + (nsector >> 8) + offset + (offset >> 8);
    
    if ((image_db->chksum != chksum) || (chksum != chksum_real)) {
        return FALSE;
    }
    
    if ((GetListItem(&image_db->usedlist) + GetListItem(&image_db->freelist)) != maxrecord) {
        return FALSE;
    }

    return TRUE;
}

/**************************************************************************************************
**  函数名称:  DB_GetItem
**  功能描述:  读取链表数
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
INT8U DB_GetItem(INT8U type, INT16U nsector, INT16U offset)
{
    ReadDB(type, nsector, offset);                                   /* GetImageDBPara(offset); */
    return (INT8U)(GetListItem(&image_db->usedlist));
}

/**************************************************************************************************
**  函数名称:  DB_GetBlankItem
**  功能描述:  读取空链表数
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
INT8U DB_GetBlankItem(INT8U type, INT16U nsector, INT16U offset)
{
    ReadDB(type, nsector, offset);                                   /* GetImageDBPara(offset); */
    return (INT8U)(GetListItem(&image_db->freelist));
}

/**************************************************************************************************
**  函数名称:  DB_GetRec
**  功能描述:  读取第numrec条记录到*ptr中
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
INT16U DB_GetRec(INT8U type, INT16U nsector, INT16U offset, INT8U numrec, INT8U *ptr)
{
    INT8U   *eleptr;
    
    ReadDB(type, nsector, offset);
    eleptr = GetRecord(numrec);
    if (eleptr == NULL) {
        return 0;
    } else {
        memcpy(ptr, eleptr, image_db->recsize);
        return image_db->recsize;
    }
}

/**************************************************************************************************
**  函数名称:  DB_GetRecField
**  功能描述:  把第numrec条记录Field里的第fieldnum个表项读到*ptr中
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
INT8U DB_GetRecField(INT8U type, INT16U nsector, INT16U offset, INT8U numrec, INT8U fieldnum, INT8U *ptr)
{
    INT8U  len;
    INT8U  *eleptr;
    
    ReadDB(type, nsector, offset);
    eleptr = GetRecord(numrec);
    if (eleptr == 0) {
        return 0;
    } else {
        eleptr += GetFieldOffset(fieldnum, image_field);
        if (image_field[fieldnum].attrib & FIELD_FIXED) {
            len = image_field[fieldnum].len;
        } else {
            len = *eleptr++;
        }
        memcpy(ptr, eleptr, len);
        return len;
    }
}

/**************************************************************************************************
**  函数名称:  DB_FindRec
**  功能描述:  查找符合的记录。
**  输入参数:  
**  返回参数:  如找不到匹配得记录，则返回0xff
**************************************************************************************************/
INT8U DB_FindRec(INT8U type, INT16U nsector, INT16U offset, INT8U findmode, INT8U fieldnum, INT8U len, INT8U *ptr)
{
    INT8U recordnum;
    ReadDB(type, nsector, offset);
    SearchRecord(&recordnum, findmode, STR_EQUAL, fieldnum, len, ptr);
    return recordnum;
}

/**************************************************************************************************
**  函数名称:  DB_DelRec
**  功能描述:  删除记录
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
BOOLEAN DB_DelRec(INT8U type, INT16U nsector, INT16U offset, INT8U numrec)
{
    INT8U *eleptr;

    ReadDB(type, nsector, offset);                                   /* read database parameters */
    eleptr = GetRecord(numrec);
    if (eleptr == 0) {
        return FALSE;
    } else {
        DelListEle(&image_db->usedlist, eleptr);
        AppendListEle(&image_db->freelist, eleptr);

        WriteDB(nsector, offset);                                    /* write database */
        return TRUE;
    }
}

/**************************************************************************************************
**  函数名称:  DB_AddRec
**  功能描述:  添加记录
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
BOOLEAN DB_AddRec(INT8U type, INT16U nsector, INT16U offset, BOOLEAN updateimage, INT8U *recptr)
{
    INT8U *blankptr, *ptr;
    INT8U findmode, findresult;
    INT8U recordnum;
    INT8U len;
    
    if (type == IMAGE_TEL && offset < DB_TB_OFFSET) maxnum = MAX_TALKREC;
    if (type == IMAGE_TEL && offset == DB_TB_OFFSET) maxnum = MAX_TBREC;
    if (type == IMAGE_DICT) maxnum = MAX_GROUP_CNT;
    if (type == IMAGE_SENDSMS || type == IMAGE_RECVSMS || type == IMAGE_DRAFTSMS) maxnum = MAX_SMSMSG;
    if (type >= IMAGE_BROADMSG && type <= IMAGE_TRACKMSG) maxnum = MAX_UNIMSG;
    if (type == IMAGE_QUESTION) maxnum = MAX_QUEMSG;
#if EN_POPMSG > 0
    if (type == IMAGE_BUFFER) maxnum = MAX_UNIMSG;
#endif
    ReadDB(type, nsector, offset);                                   /* read database parameters */
    if (GetListItem(&image_db->usedlist) >= image_db->maxrecord) {
        blankptr = GetListTail(&image_db->usedlist);
        DelListEle(&image_db->usedlist, blankptr);
    } else {
        blankptr = DelListHead(&image_db->freelist);
    }
    if (blankptr == 0) {
        return FALSE;
    }
    memcpy(blankptr, recptr, image_db->recsize);
    if (image_db->ordermode & ORDER_ORDER) {
        if (image_db->ordermode & ORDER_MATCHCASE) {
            findmode = FIND_MATCHCASE;
        } else {
            findmode = 0;
        }
        if (image_db->ordermode & ORDER_UP) {
            findresult = STR_GREAT;
        } else {
            findresult = STR_LESS;
        }
        ptr = recptr + GetFieldOffset(image_db->orderfield, image_field);
        if (image_field[image_db->orderfield].attrib & FIELD_FIXED) {
            len = image_field[image_db->orderfield].len;
        } else {
            len = *ptr++;
        }
        ptr = SearchRecord(&recordnum, findmode, findresult, image_db->orderfield, len, ptr);
    } else {
        ptr = GetListHead(&image_db->usedlist);
    }
    if (ptr == 0) {
        AppendListEle(&image_db->usedlist, blankptr);
    } else {
        BInsertListEle(&image_db->usedlist, ptr, blankptr);
    }
    if (updateimage) {
        WriteDB(nsector, offset);                                    /* write database */
    }
    return TRUE;
}

/**************************************************************************************************
**  函数名称:  DB_ModifyRec
**  功能描述:  修改记录
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
BOOLEAN DB_ModifyRec(INT8U type, INT16U nsector, INT16U offset, INT8U numrec, INT8U *recptr)
{
    INT8U *ptr;
    
    ReadDB(type, nsector, offset);                                   /* read database parameters */
    
    if ((ptr = GetRecord(numrec)) == 0) {
        return FALSE;
    }
    if (image_db->ordermode & ORDER_ORDER) {
        DelListEle(&image_db->usedlist, ptr);
        AppendListEle(&image_db->freelist, ptr);
        DB_AddRec(type, nsector, offset, FALSE, recptr);
    } else {
        memcpy(ptr, recptr, image_db->recsize);
    }

    WriteDB(nsector, offset);                                        /* write database */
    return TRUE;
}

/**************************************************************************************************
**  函数名称:  DB_ModifyRecField
**  功能描述:  修改记录表项
**  输入参数:  
**  返回参数:  
**************************************************************************************************/
BOOLEAN DB_ModifyRecField(INT8U type, INT16U nsector, INT16U offset, INT8U numrec, INT8U numfield, INT8U len, INT8U *ptr)
{
    INT8U  *recptr, *fieldptr;
    
    ReadDB(type, nsector, offset);                                   /* read database parameters */
    if ((recptr = GetRecord(numrec)) == 0) {
        return FALSE;
    }
    fieldptr = recptr + GetFieldOffset(numfield, image_field);
    if (image_field[numfield].attrib & FIELD_FIXED) {
        memcpy(fieldptr, ptr, image_field[numfield].len);
    } else {
        if (len > image_field[numfield].len) {
            return FALSE;
        }
        *fieldptr++ = len;
        memcpy(fieldptr, ptr, len);
    }
    if (image_db->ordermode & ORDER_ORDER) {
        DelListEle(&image_db->usedlist, recptr);
        AppendListEle(&image_db->freelist, recptr);
        DB_AddRec(type, nsector, offset, FALSE, recptr);
    }
    WriteDB(nsector, offset);                                        /* write database */
    return TRUE;
}
#endif

