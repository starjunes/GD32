/****************************************************************
**                                                              *
**  FILE         :  YX_ErrCode.H                                *
**  COPYRIGHT    :  (c) 2001 .Xiamen Yaxon NetWork CO.LTD       *
**                                                              *
**                                                              *
**  By : CCH 2002.1.15                                          *
****************************************************************/

#ifndef DEF_ERRCODE
#define DEF_ERRCODE
/*
********************************************************************************
*                  DEFINE ERRCODE
********************************************************************************
*/
#define BASE_CODE_SEG1                  0x00
#define CODE_SEG1(ERRID)                ( BASE_CODE_SEG1 + ERRID )

#define BASE_CODE_SEG2                  0x100
#define CODE_SEG2(ERRID)                ( BASE_CODE_SEG2 + ERRID )

#define BASE_CODE_SEG3                  0x200
#define CODE_SEG3(ERRID)                ( BASE_CODE_SEG3 + ERRID )

#define BASE_CODE_SEG4                  0x300
#define CODE_SEG4(ERRID)                ( BASE_CODE_SEG4 + ERRID )

#define BASE_CODE_RESET                  0x400
#define CODE_SEG5(ERRID)                ( BASE_CODE_RESET + ERRID )

#define BASE_CODE_SEG6                   0x500
#define CODE_SEG6(ERRID)                ( BASE_CODE_SEG6 + ERRID )

#define BASE_CODE_SEG7                   0x600
#define CODE_SEG7(ERRID)                ( BASE_CODE_SEG7 + ERRID )

// 下面的定义后续需要整理
#define ERR_MEMCPY_LEN                  CODE_SEG1(0x00)
#define ERR_ERRTASK_INSTALL             CODE_SEG1(0x01)
#define ERR_ERRDIAGNOSE_MEM             CODE_SEG1(0x02)
#define ERR_SMDRV_STATUS                CODE_SEG1(0x03)
#define ERR_SMDRV_TMR                   CODE_SEG1(0x04)
#define ERR_SMLIST_TMR                  CODE_SEG1(0x05)
#define ERR_SMLIST_MEM                  CODE_SEG1(0x06)
#define ERR_SMRECV_MEM                  CODE_SEG1(0x07)
#define ERR_GSMCORE_TMR                 CODE_SEG1(0x08)
#define ERR_GPRSDRV_TMR                 CODE_SEG1(0x09)
#define ERR_GPRSDRV_STATUS              CODE_SEG1(0x0A)
#define ERR_GPRSRECV_TMR                CODE_SEG1(0x0B)
#define ERR_GPRSLIST_TMR                CODE_SEG1(0x0C)
#define ERR_GPRSLIST_MEM                CODE_SEG1(0x0D)
#define ERR_MSGMAN_TASKID               CODE_SEG1(0x0E)
#define ERR_MSGMAN_HANDLER              CODE_SEG1(0x0F)              

#define ERR_MSGMAN_MEM                  CODE_SEG1(0x10)
#define ERR_DIAGNOSE_MEM                CODE_SEG1(0x11)
#define ERR_DIAGNOSE_ENTRY_LIST         CODE_SEG1(0x12)
#define ERR_DIAGNOSE_ENTRY_NULL         CODE_SEG1(0x13)
#define ERR_DIAGNOSE_ENTRY_INSTALL      CODE_SEG1(0x14)
#define ERR_TIMER_MAX_TASK_NUM          CODE_SEG1(0x15)
#define ERR_TIMER_INSTALL               CODE_SEG1(0x16)
#define ERR_TIMER_INSTALL_FIND          CODE_SEG1(0x17)
#define ERR_TIMER_REMOVE                CODE_SEG1(0x18)
#define ERR_TIMER_START                 CODE_SEG1(0x19)
#define ERR_TIMER_PRECISION             CODE_SEG1(0x1A)
#define ERR_TIMER_STOP                  CODE_SEG1(0x1B)
#define ERR_TIMER_LEFT                  CODE_SEG1(0x1C)
#define ERR_TIMER_ISRUN                 CODE_SEG1(0x1D)
#define ERR_MINPUT_CFG                  CODE_SEG1(0x1E)
#define ERR_MINPUT_READ                 CODE_SEG1(0x1F)

#define ERR_MINPUT_DIAG                 CODE_SEG1(0x20)
#define ERR_OUTPUT_CTLPORT              CODE_SEG1(0x21)
#define ERR_OUTPUT_INIT                 CODE_SEG1(0x22)
#define ERR_MSGMAN_POSTMSG              CODE_SEG1(0x23)
#define ERR_MSGMAN_REG                  CODE_SEG1(0x24)
#define ERR_PHONEDRV_REGIST             CODE_SEG1(0x25)
#define ERR_PHONEDRV_STATUS             CODE_SEG1(0x26)
#define ERR_PHONEDRV_TMR                CODE_SEG1(0x27)
#define ERR_PHONEDRV_PRIO               CODE_SEG1(0x28)
#define ERR_RXFRAME_LIST                CODE_SEG1(0x29)
#define ERR_RXFRAME_MEM                 CODE_SEG1(0x2A)
#define ERR_RXFRAME_CLOG                CODE_SEG1(0x2B)
#define ERR_GPSDRV_TMR                  CODE_SEG1(0x2C)
#define ERR_GPSDRV_OPT                  CODE_SEG1(0x2D)
#define ERR_GPSDRV_APPLY                CODE_SEG1(0x2E)
#define ERR_GPSMSG_REG                  CODE_SEG1(0x2F)

#define ERR_GPSEVENT_TYPE               CODE_SEG1(0x30)
#define ERR_GPSEVENT_REG                CODE_SEG1(0x31)
#define ERR_GPSMSG_POINTER              CODE_SEG1(0x32)
#define ERR_GPSMSG_FILTER               CODE_SEG1(0x33)
#define ERR_ATSIM_MEM                   CODE_SEG1(0x34)
#define ERR_ATCORE_NVPARA               CODE_SEG1(0x35)
#define ERR_ATRESET_MEM                 CODE_SEG1(0x36)
#define ERR_ODOMETER_ACC                CODE_SEG1(0x37)
#define ERR_ODOMETER_TMR                CODE_SEG1(0x38)
#define ERR_ODOMETER_INSTALL            CODE_SEG1(0x39)
#define ERR_ODOMETER_ODOMETER           CODE_SEG1(0x3A)
#define ERR_ODOMETER_ID                 CODE_SEG1(0x3B)
#define ERR_ODOMETER_ATTRIB             CODE_SEG1(0x3C)
#define ERR_ODOMETER_PTRNULL            CODE_SEG1(0x3D)
#define ERR_NVITEM_READ                 CODE_SEG1(0x3E)
#define ERR_NVITEM_WRITE                CODE_SEG1(0x3F)

#define ERR_MONITOR_GPSTYPE             CODE_SEG1(0x40)
#define ERR_MONITOR_UPDATEMODE          CODE_SEG1(0x41)
#define ERR_MONITOR_PERIOD              CODE_SEG1(0x42)
#define ERR_MONITOR_MEM                 CODE_SEG1(0x43)
#define ERR_MONITOR_TMR                 CODE_SEG1(0x44)
#define ERR_MONITOR_STATUS              CODE_SEG1(0x45)
#define ERR_SENSOR_USER                 CODE_SEG1(0x46)
#define ERR_SENSOR_TMR                  CODE_SEG1(0x47)
#define ERR_SENSOR_TYPE                 CODE_SEG1(0x48)
#define ERR_SENSOR_REGPARA              CODE_SEG1(0x49)
#define ERR_SENSOR_MODE                 CODE_SEG1(0x4A)
#define ERR_ALARMER_STATUS              CODE_SEG1(0x4B)
#define ERR_ALARMER_CONFIG              CODE_SEG1(0x4C)
#define ERR_ALARMER_TMR                 CODE_SEG1(0x4D)
#define ERR_ALARMER_ALARM               CODE_SEG1(0x4E)


#define ERR_PORT_NV_READ                CODE_SEG1(0x50)
#define ERR_PORT_NV_WRITE               CODE_SEG1(0x51)
#define ERR_PORT_INSTALL_HOOK           CODE_SEG1(0x52)
#define ERR_PORT_CREATE_TIMER           CODE_SEG1(0x53)
#define ERR_PORT_TIMER_1                CODE_SEG1(0x54)
#define ERR_PORT_TIMER_2                CODE_SEG1(0x55)
#define ERR_PORT_TIMER_3                CODE_SEG1(0x56)
#define ERR_PORT_TIMER                  CODE_SEG1(0x57)
#define ERR_PORT_STARTTIMER_1           CODE_SEG1(0x58)
#define ERR_PORT_STARTTIMER_2           CODE_SEG1(0x59)
#define ERR_PORT_STARTTIMER_3           CODE_SEG1(0x5A)
#define ERR_PORT_STARTTIMER             CODE_SEG1(0x5B)
#define ERR_PORT_PAUSETIMER             CODE_SEG1(0x5C)
#define ERR_PORTDRV_TMR                 CODE_SEG1(0x5D)
#define ERR_PORTDRV_TYPE                CODE_SEG1(0x5E)
#define ERR_PORTDRV_MODE                CODE_SEG1(0x5F)

#define ERR_SYSTIME_TMR                 CODE_SEG1(0x60)
#define ERR_TIMETASK_CREATE             CODE_SEG1(0x61)
#define ERR_TIMETASK_START              CODE_SEG1(0x62)
#define ERR_TIMETASK_STOP               CODE_SEG1(0x63)
#define ERR_TIMETASK_REMOVE             CODE_SEG1(0x64)
#define ERR_TIMETASK_SWITCH             CODE_SEG1(0x65)
#define ERR_TIMETASK_MEM                CODE_SEG1(0x66)
#define ERR_TIMETASK_HANDLER            CODE_SEG1(0x67)
#define ERR_TMRTSK_NUM                  CODE_SEG1(0x68)
#define ERR_SIM_OVERADDRESS             CODE_SEG1(0x69)
#define ERR_SIM_NULLINFORMER            CODE_SEG1(0x6A)
#define ERR_GPRSDRV_TEST                CODE_SEG1(0x6B)
#define ERR_LCDLIST_WAIT                CODE_SEG1(0x6C)
#define ERR_LCDLIST_MEM                 CODE_SEG1(0x6D)
#define ERR_LCDLIST_TMR                 CODE_SEG1(0x6E)
#define ERR_LCDLIST_ASM                 CODE_SEG1(0x6F)

#define ERR_GPRSSEND_SEND               CODE_SEG1(0xA0)
#define ERR_GPRSSEND_MEM                CODE_SEG1(0xA1)
#define ERR_GAUDER_MODE                 CODE_SEG1(0xA2)
#define ERR_GAUDER_TMR                  CODE_SEG1(0xA3)
#define ERR_GAUDER_LEARN                CODE_SEG1(0xA4)
#define ERR_FLASH_WRITEBYTE             CODE_SEG1(0xA5)
#define ERR_FLASH_ERASE                 CODE_SEG1(0xA6)
#define ERR_FLASH_WRITEADDR        		CODE_SEG1(0xA7)
#define ERR_FLASH_WRITELEN           	CODE_SEG1(0xA8)


#define ERR_NVRAMITEM_ID                CODE_SEG1(0xC0)
#define ERR_NVRAMITEM_PARAM             CODE_SEG1(0xC1)
#define ERR_NVRAMITEM_TYPE              CODE_SEG1(0xC2)
#define ERR_NVRAMITEM_SIZE              CODE_SEG1(0xC3)
#define ERR_NVRAMITEM_ISO               CODE_SEG1(0xC4)
#define ERR_NVRAMITEM_WRITE             CODE_SEG1(0xC5)
#define ERR_NVRAMITEM_READ              CODE_SEG1(0xC6)
#define ERR_NVRAMITEM_ITEM              CODE_SEG1(0xC7)
#define ERR_NVRAMITEM_NUM               CODE_SEG1(0xC8)
#define ERR_NVRAMREC_LIST               CODE_SEG1(0xC9)
#define ERR_NVRAMREC_MEM                CODE_SEG1(0xCA)
#define ERR_NVRAMMONI_TMR               CODE_SEG1(0xCB)


#define ERR_RECMEM_FSEEK                CODE_SEG1(0xD0)
#define ERR_RECMEM_ITEM                 CODE_SEG1(0xD1)
#define ERR_RECMEM_SIZE                 CODE_SEG1(0xD2)
#define ERR_RECMEM_WRITE                CODE_SEG1(0xD3)
#define ERR_RECMEM_READ                 CODE_SEG1(0xD4)
#define ERR_RECMEM_FFLUSH               CODE_SEG1(0xD5)
#define ERR_RECMEM_FCLOSE               CODE_SEG1(0xD6)
#define ERR_RECMEM_FOPEN                CODE_SEG1(0xD7)
#define ERR_RECMEM_REMOVE               CODE_SEG1(0xD8)
#define ERR_RECMEM_NAMLEN               CODE_SEG1(0xD9)
#define ERR_RECMEM_SUBSAV               CODE_SEG1(0xDA)


#define ERR_PORT_SM_REQPARSESM          CODE_SEG1(0xE0)
#define ERR_PORT_SM_GETSMSNUM           CODE_SEG1(0xE1)
#define ERR_PORT_SM_EPTR                CODE_SEG1(0xE2)
#define ERR_GPRSMAN_MEM                 CODE_SEG1(0xE3)
#define ERR_GPRSMAN_ID                  CODE_SEG1(0xE4)
#define ERR_GPRSMAN_TMR                 CODE_SEG1(0xE5)
#define ERR_RESETCPU_USER               CODE_SEG1(0xE6)


//////////////////////////////////////////////////////////////////
#define ERR_TIMETASK_TMRCOUNT           CODE_SEG2(0x00)
#define ERR_AUDIOTASK_INIT			    CODE_SEG2(0x01)
#define ERR_PHONETASK_INIT			    CODE_SEG2(0x02)
#define ERR_ALARMTASK_INIT			    CODE_SEG2(0x03)


#define ERR_WATCHDOG_MEM                CODE_SEG2(0x10)
#define ERR_WATCHDOG_OVERFLOW           CODE_SEG2(0x11)
#define ERR_WATCHDOG_APPLY              CODE_SEG2(0x12)
#define ERR_WATCHDOG_ID                 CODE_SEG2(0x13)
#define ERR_HSTIODRV_SEND               CODE_SEG2(0x14)
#define ERR_OPEN_PUBFILE                CODE_SEG2(0x15)
#define ERR_MEMPOOL_FLAG                CODE_SEG2(0x16)
#define ERR_MEMPOOL_TYPE                CODE_SEG2(0x17)
#define ERR_MEMPOOL_NUM                 CODE_SEG2(0x18)
#define ERR_MMIUARTDRV_REG              CODE_SEG2(0x19)
#define ERR_MMIUARTLIST_MEM             CODE_SEG2(0x1A)
#define ERR_MMIUARTLIST_TMR             CODE_SEG2(0x1B)
#define ERR_MMIUARTLIST_ASM             CODE_SEG2(0x1C)
#define ERR_MMIUARTLIST_CTWAIT          CODE_SEG2(0x1D)
#define ERR_MMIUARTRECV_REG             CODE_SEG2(0x1E)
#define ERR_EXTUARTLIST_WAIT            CODE_SEG2(0x1F)

#define ERR_POSITION_SET                CODE_SEG2(0x50)
#define ERR_POSITION_REG                CODE_SEG2(0x51)
#define ERR_RANGEPOS_SET                CODE_SEG2(0x52)
#define ERR_TIMEPOS_SET                 CODE_SEG2(0x53)
#define ERR_VTPOS_SET                   CODE_SEG2(0x54)
#define ERR_INTPOS_SET                  CODE_SEG2(0x55)
#define ERR_MONITOR_ID                  CODE_SEG2(0x56)
#define ERR_EGCODE_READ                 CODE_SEG2(0x57)
#define ERR_DRAGALARM_TMR               CODE_SEG2(0x58)
#define ERR_ALARMCONFIG_READ            CODE_SEG2(0x59)
#define ERR_VTPOSPARA_READ              CODE_SEG2(0x5A)
#define ERR_ALARMER_TYPE                CODE_SEG2(0x5B)
#define ERR_SPEEDALM_SET                CODE_SEG2(0x5C)
#define ERR_DEFENCER_STATUS             CODE_SEG2(0x5D)
#define ERR_DEFENCER_TMR                CODE_SEG2(0x5E)


#define ERR_BLKBOX_WRITE                CODE_SEG2(0x70)
#define ERR_BLKBOX_READ                 CODE_SEG2(0x71)
#define ERR_BLK_GPSNUM                  CODE_SEG2(0x72)
#define ERR_BLKBOX_QUERY                CODE_SEG2(0x73)


#define ERR_DCDRV_INITTMR               CODE_SEG2(0x81)
#define ERR_DCDRV_STOPPREVIEW           CODE_SEG2(0x82)
#define ERR_DCDRV_REMOVE_FOLDER         CODE_SEG2(0x83)
#define ERR_DCDRV_FILE_RECYCLE          CODE_SEG2(0x84)
#define ERR_MFSEND_LEN_ERROR            CODE_SEG2(0x85)
#define ERR_CEDRV_FILE_RECYCLE          CODE_SEG2(0x86)

#define ERR_HST_SCREEN_VALID            CODE_SEG3(0x00)
#define ERR_HST_SCREEN_REVERSE          CODE_SEG3(0x01)
#define ERR_HST_REG_CTL_CLASS           CODE_SEG3(0x02)
#define ERR_HST_CTL_CLASS_UNEXIST       CODE_SEG3(0x03)
#define ERR_HST_MMI_TASK                CODE_SEG3(0x04)
#define ERR_HST_FIRSTWIN_CREATE         CODE_SEG3(0x05)
#define ERR_HST_CNT_OVERFLOW            CODE_SEG3(0x06)
#define ERR_HST_CLOSE_TILLWIN           CODE_SEG3(0x07)
#define ERR_HST_PHONEWIN_CREATE         CODE_SEG3(0x08)
#define ERR_PHONEWIN_STORE              CODE_SEG3(0x09)
#define ERR_TELBOOKWIN_HDLMSG           CODE_SEG3(0x0A)
#define ERR_TELBOOKPOP_OPEN             CODE_SEG3(0x0B)
#define ERR_TELBOOKWIN_VIEW             CODE_SEG3(0x0C)
#define ERR_TELRECORD_LIST_HDLMSG       CODE_SEG3(0x0D)
#define ERR_TELRECORD_LIST_READ         CODE_SEG3(0x0E)
#define ERR_EDITBOX_INS_CHN             CODE_SEG3(0x0F)

#define ERR_EDITBOX_CHECK               CODE_SEG3(0x10)
#define ERR_EDITBOX_SETTEXT             CODE_SEG3(0x11)
#define ERR_EDITBOX_DESTROY             CODE_SEG3(0x12)
#define ERR_EDITBOX_HDLMSG              CODE_SEG3(0x13)
#define ERR_LISTBOX_CHECK               CODE_SEG3(0x14)
#define ERR_LISTBOX_SHOW                CODE_SEG3(0x15)
#define ERR_LISTBOX_CHANGEPARA          CODE_SEG3(0x16)
#define ERR_LISTBOX_SETTEXT             CODE_SEG3(0x17)
#define ERR_LISTBOX_DESTROY             CODE_SEG3(0x18)
#define ERR_LISTBOX_HDLMSG              CODE_SEG3(0x19)
#define ERR_MENUBOX_SHOW                CODE_SEG3(0x1A)
#define ERR_MENUBOX_JUMP                CODE_SEG3(0x1B)
#define ERR_MENUBOX_DESTROY             CODE_SEG3(0x1C)
#define ERR_MENUBOX_HDLMSG              CODE_SEG3(0x1D)
#define ERR_TEXTBOX_SHOW                CODE_SEG3(0x1E)
#define ERR_TEXTBOX_SETTEXT             CODE_SEG3(0x1F)

#define ERR_TEXTBOX_DESTROY             CODE_SEG3(0x20)
#define ERR_TEXTBOX_HDLMSG              CODE_SEG3(0x21)
#define ERR_WINMAN_CTL_PROC             CODE_SEG3(0x22)
#define ERR_WINMAN_WINKEY               CODE_SEG3(0x23)
#define ERR_WINMAN_CREATEWIN            CODE_SEG3(0x24)
#define ERR_WINMAN_REFRESHWIN           CODE_SEG3(0x25)
#define ERR_WINMAN_GETCTL               CODE_SEG3(0x26)
#define ERR_WINPARSE_TAB                CODE_SEG3(0x27)
#define ERR_MMIMSG_FREE                 CODE_SEG3(0x28)
#define ERR_MMIMSG_ASYNC                CODE_SEG3(0x29)
#define ERR_MMITIMER_CREATE             CODE_SEG3(0x2A)
#define ERR_MMITIMER_CREATE_START       CODE_SEG3(0x2B)
#define ERR_MMITIMER_DESTROY_CID        CODE_SEG3(0x2C)
#define ERR_MMITIMER_DESTROY_WIN        CODE_SEG3(0x2D)
#define ERR_MMITIMER_START              CODE_SEG3(0x2E)
#define ERR_MMITIMER_STOP               CODE_SEG3(0x2F)

#define ERR_ALMWAY_HDLMSG               CODE_SEG3(0x30)
#define ERR_ALMWAY_TRIG_HDLMSG          CODE_SEG3(0x31)
#define ERR_ASET_SMSPS_HDLMSG           CODE_SEG3(0x32)
#define ERR_ASET_TOWALM_HDLMSG          CODE_SEG3(0x33)
#define ERR_ASET_TESTIN_HDLMSG          CODE_SEG3(0x34)
#define ERR_GSET_APN_HDLMSG             CODE_SEG3(0x35)
#define ERR_GSET_IP_HDLMSG              CODE_SEG3(0x36)
#define ERR_GSET_PORT_HDLMSG            CODE_SEG3(0x37)
#define ERR_GSET_MYTEL_HDLMSG           CODE_SEG3(0x38)
#define ERR_GSET_SMSC_HDLMSG            CODE_SEG3(0x39)
#define ERR_GSET_CTEL_HDLMSG            CODE_SEG3(0x3A)
#define ERR_GSET_AUTEL_HDLMSG           CODE_SEG3(0x3B)
#define ERR_MENUSET_HDLMSG              CODE_SEG3(0x3C)
#define ERR_MENUSET_CONTRAST_HDLMSG     CODE_SEG3(0x3D)
#define ERR_MENUSET_RING_HDLMSG         CODE_SEG3(0x3E)
#define ERR_MENUSET_KBS_HDLMSG          CODE_SEG3(0x3F)

#define ERR_MENUSET_PKUP_HDLMSG         CODE_SEG3(0x40)
#define ERR_MENUSET_REMOTE_HDLMSG       CODE_SEG3(0x41)
#define ERR_PHONEWIN_HDLMSG             CODE_SEG3(0x42)
#define ERR_PHONEWIN_OPEN               CODE_SEG3(0x43)
#define ERR_PROMPT_HDLMSG               CODE_SEG3(0x44)
#define ERR_NEWSMS_TEL                  CODE_SEG3(0x45)
#define ERR_NEWSMS_CONTENT              CODE_SEG3(0x46)
#define ERR_SMSWIN_OPEN                 CODE_SEG3(0x47)
#define ERR_SMSLIST_HDLMSG              CODE_SEG3(0x48)
#define ERR_SMSVIEWWIN_HDLMSG           CODE_SEG3(0x49)
#define ERR_SMSVIEW_MALLOC              CODE_SEG3(0x4A)
#define ERR_PINYINBOX_DESTROY           CODE_SEG3(0x4B)
#define ERR_PINYINBOX_HDLMSG            CODE_SEG3(0x4C)
#define ERR_PINYINBOX_CHECK             CODE_SEG3(0x4D)
#define ERR_PINYINBOX_INIT              CODE_SEG3(0x4E)
#define ERR_PINYINBOX_HZ_MATCH_LEN      CODE_SEG3(0x4F)

#define ERR_VOL_STEPWIN_HDLMSG          CODE_SEG3(0x50)
#define ERR_PHONEWIN_VOL_DISP           CODE_SEG3(0x51)
#define ERR_PHONEWIN_TALKTIME           CODE_SEG3(0x52)
#define ERR_WINMAN_CLOSE_ALLWIN         CODE_SEG3(0x53)
#define ERR_SMS_POPMENU_OPEN            CODE_SEG3(0x54)
#define ERR_REPLYSMS_CONTENT            CODE_SEG3(0x55)
#define ERR_REPLYSMS_TEL                CODE_SEG3(0x56)
#define ERR_OILSET_AUTOCUT_HDLMSG       CODE_SEG3(0x57)
#define ERR_GPRSLINK_STATUS             CODE_SEG3(0x58)
#define ERR_GPRSLINK_TMR                CODE_SEG3(0x59)
#define ERR_SMSFRAME_USER               CODE_SEG3(0x5A)
#define ERR_SMSFRAME_TMR                CODE_SEG3(0x5B)
#define ERR_SMSFRAME_MEM                CODE_SEG3(0x5C)
#define ERR_RX_FRAME_MEM                CODE_SEG3(0x5D)
#define ERR_SYSFRAME_REG                CODE_SEG3(0x5E)
#define ERR_SYSFRAME_SEND               CODE_SEG3(0x5F)

#define ERR_SYSFRAME_LEN                CODE_SEG3(0x60)


#define ERR_PPPDRV_STATUS               CODE_SEG3(0x70)
#define ERR_PPPDRV_TMR                  CODE_SEG3(0x71)
#define ERR_TCPDRV_TMR                  CODE_SEG3(0x72)
#define ERR_TCPDRV_LIST                 CODE_SEG3(0x73)
#define ERR_TCPDRV_SOCKETNO             CODE_SEG3(0x74)
#define ERR_UDPDRV_TMR                  CODE_SEG3(0x75)
#define ERR_UDPDRV_LIST                 CODE_SEG3(0x76)
#define ERR_UDPDRV_SOCKETNO             CODE_SEG3(0x77)
#define ERR_ASET_SENSORSET_HDLMSG       CODE_SEG3(0x78)
#define ERR_ASET_LAMPSET_HDLMSG         CODE_SEG3(0x79)
#define ERR_ASET_LAMPFUNCSET_HDLMSG     CODE_SEG3(0x7a)
#define ERR_ASET_LAMPVOLTAGE_HDLMSG     CODE_SEG3(0x7b)
#define ERR_ASET_DOORSET_HDLMSG         CODE_SEG3(0x7c)
#define ERR_ASET_DOORFUNCSET_HDLMSG     CODE_SEG3(0x7d)
#define ERR_ASET_DOORVOLTAGE_HDLMSG     CODE_SEG3(0x7e)
#define ERR_ASET_ALMINDISET_HDLMSG      CODE_SEG3(0x7f)

#define ERR_ASET_ALMINDIFUNCSET_HDLMSG  CODE_SEG3(0x80)
#define ERR_ASET_ALMINDIVOLTAGE_HDLMSG  CODE_SEG3(0x81)

#define ERR_DSET_PROVINCE_HDLMSG        CODE_SEG3(0x82)
#define ERR_DSET_CITY_HDLMSG            CODE_SEG3(0x83)
#define ERR_DSET_MANUFAC_HDLMSG         CODE_SEG3(0x84)
#define ERR_DSET_DEVICETYPE_HDLMSG      CODE_SEG3(0x85)
#define ERR_DSET_DEVICEID_HDLMSG        CODE_SEG3(0x86)
#define ERR_DSET_COLOUR_HDLMSG          CODE_SEG3(0x87)
#define ERR_DSET_CARLICENSE_HDLMSG      CODE_SEG3(0x88)

#define ERR_DSET_REGOUT_HDLMSG          CODE_SEG3(0x89)

//pubpara_core
#define ERR_PUBPARA_FOPEN               CODE_SEG4(0x01)
#define ERR_PUBPARA_FCLOSE              CODE_SEG4(0x02)
#define ERR_PUBPARA_FSEEK               CODE_SEG4(0x03)
#define ERR_PUBPARA_FREAD               CODE_SEG4(0x04)
#define ERR_PUBPARA_FWRITE              CODE_SEG4(0x05)
#define ERR_PUBPARA_MAXID               CODE_SEG4(0x06)
#define ERR_PUBPARA_REGINFOR            CODE_SEG4(0x07)
#define ERR_PUBPARA_RAMIMG              CODE_SEG4(0x08)
#define ERR_PUBPARA_FSIZE               CODE_SEG4(0x09)
#define ERR_PUBPARA_REGTBL              CODE_SEG4(0x0A)
#define ERR_PUBPARA_INFORMER            CODE_SEG4(0x0b)
#define ERR_PUBPARA_WRITELEN            CODE_SEG4(0x0c)
#define ERR_PUBPARA_READLEN             CODE_SEG4(0x0d)
#define ERR_PUBPARA_CFGCHK              CODE_SEG4(0x0e)


//pubpara_simbak
#define ERR_PPBAK_OPID                  CODE_SEG4(0x11)
#define ERR_PPBAK_RDPTR                 CODE_SEG4(0x12)
#define ERR_PPBAK_RDLEN                 CODE_SEG4(0x13)
#define ERR_PPBAK_MAXNUM                CODE_SEG4(0x14)
#define ERR_PPBAK_OPATTR                CODE_SEG4(0x15)
#define ERR_PPBAK_WRADDR                CODE_SEG4(0x16)
#define ERR_PPBAK_RDADDR                CODE_SEG4(0x17)
#define ERR_PPBAK_REGINFOR              CODE_SEG4(0x18)
#define ERR_PPBAK_RAMIMG                CODE_SEG4(0x19)
#define ERR_PPBAK_CFGCHK                CODE_SEG4(0x1a)

//fileman
#define ERR_FILEMAN_FILESIZE             CODE_SEG4(0x21)
#define ERR_FILEMAN_OPENDIR              CODE_SEG4(0x22)
#define ERR_FILEMAN_FNAMEPTR             CODE_SEG4(0x23)
#define ERR_FILEMAN_FNAMELEN             CODE_SEG4(0x24)
#define ERR_FILEMAN_REGNUM               CODE_SEG4(0x25)
#define ERR_FILEMAN_FORMAT               CODE_SEG4(0x26)

//database
#define ERR_DBCORE_FSIZE                 CODE_SEG4(0x31)
#define ERR_DBCORE_FSEEK                 CODE_SEG4(0x32)
#define ERR_DBCORE_FREAD                 CODE_SEG4(0x33)
#define ERR_DBCORE_FWRITE                CODE_SEG4(0x34)
#define ERR_DBCORE_DBNUM                 CODE_SEG4(0x35)
#define ERR_DBCORE_REGINFOR              CODE_SEG4(0x36)
#define ERR_DBCORE_FNAME                 CODE_SEG4(0x37)
#define ERR_DBCORE_MAXITEM               CODE_SEG4(0x38)
#define ERR_DBCORE_FOPEN                 CODE_SEG4(0x39)
#define ERR_DBCORE_FCLOSE                CODE_SEG4(0x3A)

//track
#define ERR_TRACK_READREC                CODE_SEG4(0x41)
#define ERR_TRACK_TRIGETYPE              CODE_SEG4(0x42)
#define ERR_TRACK_TRIGEMODE              CODE_SEG4(0x43)
#define ERR_TRACK_LIST                   CODE_SEG4(0x44)
#define ERR_TRACK_LISTNUM                CODE_SEG4(0x45)
#define ERR_TRACK_READPARA               CODE_SEG4(0x46)

////for test initiative reset
#define ERR_GPRSLINK_PPPFAILURE          CODE_SEG5(0x00)   
#define ERR_GPRSLINK_ATTACHGPRS          CODE_SEG5(0x01)
#define ERR_GPRSLINK_LOGGPRS             CODE_SEG5(0x02)
#define ERR_GSMCORE_RESETTMR             CODE_SEG5(0x03)
#define ERR_GSMCORE_SENDSMFAILURE        CODE_SEG5(0x04)         
#define ERR_GSMCORE_DELSMFAILURE         CODE_SEG5(0x05)
#define ERR_GSMCORE_GUARD_MSG_ERROR      CODE_SEG5(0x06)
#define ERR_GPRSDRIVER_STATEERR          CODE_SEG5(0x07)
#define ERR_INITIATIVE_RESETTMR          CODE_SEG5(0x08)

// port sensor
#define ERR_THREAD_EXISTED               CODE_SEG5(0x20)
#define ERR_THREAD_CREATED               CODE_SEG5(0x21)
#define ERR_YXDC_SIGNAL                  CODE_SEG5(0x22)

// for wireless download
#define ERR_WDLOAD_MALLOCMEM             CODE_SEG5(0x30)    
#define ERR_WDLOAD_DATAPTR               CODE_SEG5(0x31) 
#define ERR_WDLOAD_WRITESIZE             CODE_SEG5(0x32)
#define ERR_WDLOAD_FILEOPEN              CODE_SEG5(0x33)
#define ERR_WDOWNMAN_UPDATASUCCESS       CODE_SEG5(0x34)
#define ERR_WDOWNMAN_FILELEN             CODE_SEG5(0x35)
#define ERR_WDOWNRECV_PARA               CODE_SEG5(0x36)
#define ERR_WDOWNFRAMEPARSE_MYTEL        CODE_SEG5(0x37)
#define ERR_WDOWNMAN_FSEEK               CODE_SEG5(0x38)
#define ERR_WDOWNMAN_FWRITE              CODE_SEG5(0x39)
#define ERR_WDLOAD_FILECLOSE             CODE_SEG5(0x3A)
#define ERR_WDLOAD_FILEREMOVE            CODE_SEG5(0x3B)

#if EN_HANDSET > 0 && EN_HST_SYSMSG > 0
#define ERR_DISPAVIEWWIN_HDLMSG          CODE_SEG5(0x50)
#define ERR_DISPAVIEW_MALLOC             CODE_SEG5(0x51)
#define ERR_DISPALIST_HDLMSG             CODE_SEG5(0x52)
#define ERR_BROADLIST_HDLMSG             CODE_SEG5(0x53)
#define ERR_BROADAVIEW_MALLOC            CODE_SEG5(0x54)
#define ERR_SYSMSG                       CODE_SEG5(0x58)
#endif

#if EN_HST_DICTION > 0
#define ERR_DICTION_DEL                  CODE_SEG5(0x60)
#define ERR_DICTION_SENF                 CODE_SEG5(0x61)
#define ERR_DICTION_EDIT                 CODE_SEG5(0x62)
#define ERR_DICTION_MENU_POP             CODE_SEG5(0x63)
#define ERR_DICTION_MENU                 CODE_SEG5(0x64)
#define ERR_DICTION                      CODE_SEG5(0x68)
#endif

#if EN_RUNRECORD > 0
#define ERR_RUNTIRED_TMR                 CODE_SEG5(0x70)
#define ERR_RUNTIRED_QUERY               CODE_SEG5(0x71)
#define ERR_RUNDOUBT_QUERY               CODE_SEG5(0x72)
#define ERR_RUNDOUBT_TMR                 CODE_SEG5(0x73)
#define ERR_USB_OUTPUT                   CODE_SEG5(0x74)
#endif 
#define ERR_MEMPOOL_ALLOC                CODE_SEG5(0x75)
#define ERR_CELLDRV_MEMORY               CODE_SEG5(0x76)

#define ERR_AUDIOCHANNEL_REGIST          CODE_SEG5(0x77)
#define ERR_LOGINFORM_REGIST             CODE_SEG5(0x78)

#define ERR_FILEDOWNMAN_MAX              CODE_SEG6(0xE4)
#define ERR_FILEDOWNMAN_TYPE             CODE_SEG6(0xE5)
#define ERR_CELLFILEDOWNMAN_DATALEN      CODE_SEG6(0xE6)
#define ERR_CELLDRV_MAXTYPEID            CODE_SEG6(0xE7)
#define ERR_CELLDRV_INFORMER             CODE_SEG6(0xE8)
#define ERR_CELDRV_FILE_RECYCLE          CODE_SEG6(0xE9)
#define ERR_VECTOR_HANDLER               CODE_SEG6(0xEA)
#define ERR_VECTOR_NUM                   CODE_SEG6(0xEB)
#define ERR_VECTOR_ID                    CODE_SEG6(0xEC)
#define ERR_YAW_FILE_RECYCLE             CODE_SEG6(0xED)
#define ERR_LIST_CELLFILEDOWN            CODE_SEG6(0xEE)
#define ERR_HEART_TABLE                  CODE_SEG6(0xEF)

/*------------------------------BASE_CODE_SEG7-------------------------------------*/
enum {
    ERR_PE_RECV = BASE_CODE_SEG7,
    ERR_PE_DRV,
    ERR_PE_SEND,
    
    ERR_LCD_COM,
    ERR_LCD_SEND,
    ERR_LCD_RECV,
    
    ERR_BLK_COM,
    
    ERR_150G3_COM,
    ERR_150G3_DRV,
    ERR_150G3_RECV,
    ERR_150G3_SEND,
    
    ERR_HST_COM,
    
    ERR_120ND_COM,
    ERR_120ND_RECV,
    ERR_120ND_SEND,
    ERR_120ND_DRV,

    ERR_150TR_COM,
    ERR_150TR_RECV,
    ERR_150TR_SEND,
    ERR_150TR_DRV,
    
    ERR_120R_COM,
    ERR_120R_RECV,
    ERR_120R_SEND,
    ERR_120R_DRV,
    
	ERR_RUN_COM,
    ERR_RUN_RECV,
    ERR_RUN_SEND,
    ERR_RUN_DRV,	
    
#if EN_150KG > 0
    ERR_150KG_COM,
    ERR_150KG_RECV,
    ERR_150KG_SEND,
    ERR_150KG_DRV,
#endif
    ERR_HEART_MEM,

    ERR_EXTUP_COM,
    ERR_EXTUP_RECV,
    ERR_EXTUP_SEND,
    ERR_EXTUP_DRV,

	ERR_ST_HANDLER,
	ERR_ST_ID,
	ERR_VT_TYPE,
	ERR_ROADVECTOR_DIRECITON,
	ERR_MEMFREE_LEN,
	ERR_RGDRV_FILE_RECYCLE,
	ERR_RGDRV_FILE_RECYCLE1,
	ERR_RGDRV_FILE_RECYCLE2,
	ERR_AREATYPE_ID,
	ERR_SOCKET_RECVLEN,
	ERR_ETOS_LEN,

	ERR_FILENAME_LEN,
	ERR_MFILELIST_MEM,

	ERR_RLDRV_FILE_RECYCLE1,
	ERR_RLDRV_FILE_RECYCLE2,
	ERR_RLDRV_FILE_RECYCLE,
	ERR_COMMON_FILE_RECYCLE,
	ERR_RL_FULL,
#if EN_VCAMERA > 0   
    ERR_VCAMERA_COM,
    ERR_VCAMERA_RECV,
    ERR_VCAMERA_SEND,
    ERR_VCAMERA_DRV, 
#endif    
	ERR_LOAD_INPUT,
	ERR_RLFILE_WR,
	ERR_CEFILE_WR,
	ERR_RGFILE_WR,
    	
#if EN_CR606_OILWEAR > 0
    ERR_CR606_COM,
    ERR_CR606_RECV,
#endif
    ERR_120ND70_COM,
    ERR_120ND70_RECV,
    ERR_120ND70_SEND,
    ERR_120ND70_DRV,
	ERR_MAX_COM
};

#endif

