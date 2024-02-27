/****************************************************************
**                                                              *
**  FILE         :  yx_message.h                                   *
**  COPYRIGHT    :  (c) 2001 .Xiamen Yaxon NetWork CO.LTD       *
**                                                              *
**                                                              *
**  By : CCH 2001.1.15                                          *
****************************************************************/

#ifndef	DEF_MESSAGE
#define DEF_MESSAGE

// This file is remained just for compatibility.
#include "os_reg.h"

#if 0
/*
********************************************************************************
*                  DEFINE GENERAL MESSAGE
********************************************************************************
*/
typedef enum {
    MSG_GENERAL_START = 0x00,
    MSG_TMRTSK_TMRRUN,
    MSG_ERRDIOG_RUN
} GENERAL_MSG_E;
/*
********************************************************************************
*                  DEFINE GSMTASK MESSAGE
********************************************************************************
*/

typedef enum {
    MSG_GSMTASK_START = 0x10,
    MSG_SMDRV_FREE,
    MSG_SMDRV_UNCLOGSM,
    MSG_SMDRV_TSK,
    MSG_SMLIST_TSK,
    MSG_SMLIST_FREE,
    MSG_SMDRV_SENDSHORTMSG,
    MSG_SMRECV_RECVSHORTMSG,

    MSG_GSMCORE_ENDINIT,
    MSG_GSMCORE_SIGNAL,
    MSG_GSMCORE_DETECTSIM,
    MSG_GSMCORE_NOSIMCARD,
    MSG_GSMCORE_NONETWORK,
    MSG_GSMCORE_SEARCHNETWORK,
    MSG_GSMCORE_VOICEDISCONNECT,
    MSG_GSMCORE_NEEDGPRS,
    MSG_GSMCORE_DELETESM,
    MSG_GSMCORE_PSREADY,
    MSG_GPRSLINK_ACTIVEGPRS,

    MSG_UDPDRV_COMOPEN,
    MSG_UDPDRV_COMCLOSE,
    MSG_TCPDRV_SOCKETOPEN,
    MSG_TCPDRV_SOCKETCLOSE,
    MSG_GPRSDRV_ACTIVED,
    MSG_GPRSDRV_DEACTIVED,

    MSG_WDOWNRECV_TSK,
    MSG_WDOWNRESULT_ENTRY,

    MSG_GPRSMAN_DEACTIVATED,
    MSG_GPRSMAN_ACTIVATED,

    MSG_ATCORE_INIT,

    MSG_ATCORE_UNPROHIBITSM,
    MSG_ATCORE_ENDINITSIM,
    MSG_ATRECV_RECVSHORTMSG,
    MSG_RXFRANE_RECV,
    MSG_ATGPRS_TSK,
    MSG_ATGPRS_FREE,


    MSG_ATTCPIP_FREE,
    MSG_ATTCPIP_TSK,
    MSG_ATSIM_TSK,

    MSG_MTRECV_TSK
} GSMTSK_MSG_E;
/*
********************************************************************************
*                  DEFINE OPTTASK MESSAGE
********************************************************************************
*/
typedef enum {
    MSG_OPTTASK_START = 0x40,
    MSG_GPRSDRV_PPPESTABLISHD,
    MSG_GPRSDRV_PPPBROKEN,
    MSG_GPRSDRV_PPPESTABLISHFAILURE,

    MSG_UDPDRV_TSK,
    MSG_UDPDRV_SENDFLOW,


    MSG_TCPDRV_TSK,
    MSG_TCPDRV_SENDFLOW,
    MSG_TCPDRV_SENDOK,

    MSG_UDPLINK_CONNECTERR,
    MSG_UDPLINK_REGERR,
    MSG_UDPLINK_LOGERR,
    MSG_UDPLINK_REGED,
    MSG_UDPLINK_LOGED,

    MSG_TCPLINK_CONNECTERR,
    MSG_TCPLINK_REGED_INF,
    MSG_TCPLINK_REGERR,
    MSG_TCPLINK_LOGERR,
    MSG_TCPLINK_REGED,
    MSG_TCPLINK_LOGED,

    MSG_GPRSRECV_TSK,

    MSG_GPRSCOMM_ERR,

    MSG_MONITOR_TSK,

    MSG_OILCUT_IDENTIFYRESULT,
    MSG_ROADVECTOR_CHECK,
    MSG_ROADAUDIO_CHECK,
    MSG_YAW_CHECK,
    MSG_RANGE_CHECK,
    MSG_AUDIO_TSK,
    MSG_ROADLINE_PROTOCOL,
    MSG_TCPBACKLINK_LOGED,
    MSG_TCPBACKLINK_REGED,
    MSG_TCPBACKLINK_LOGERR
} OPTTSK_MSG_E;


/*
********************************************************************************
*                  DEFINE ALMTASK MESSAGE
********************************************************************************
*/

typedef enum {
    MSG_ALMTASK_START = 0x70,

    MSG_ALARMER_CANCEL,
    MSG_ALARMER_CONFIG,
    MSG_SENSOR_TRIGGERROBWARN,
    MSG_SENSOR_RELEASEROBWARN,
    MSG_SENSOR_ALARMCHANGE,
    MSG_GAUDER_ALARMCHANGE,
    MSG_SENSOR_RELEASEVIND,
    MSG_SENSOR_RELEASEPOWDECT
} ALMTSK_MSG_E;

/*
********************************************************************************
*                  DEFINE COMMONTASK MESSAGE
********************************************************************************
*/
typedef enum {
    MSG_COMMOMTASK_START = 0xc0,

    MSG_GPS_EVENT,
    MSG_TTS_TSK
} COMTSK_MSG_E;

/*
********************************************************************************
*                  DEFINE HANDSET MESSAGE
********************************************************************************
*/
typedef enum {
    MSG_HSTTASKSTART = 0xe0,

    MSG_HSTTASKKEY,
    MSG_HSTTASKTIMER,
    MSG_HSTTASKMSG,
    MSG_HSTTASKSIGNAL,
    MSG_HSTTASKOILPSWD
} HSTTSK_MSG_E;
#endif

#endif
