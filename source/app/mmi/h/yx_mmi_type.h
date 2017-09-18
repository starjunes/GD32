/********************************************************************************
**
** ÎÄ¼þÃû:     yx_mmi_ptype.h
** °æÈ¨ËùÓÐ:   (c) 2007-2008 ÏÃÃÅÑÅÑ¸ÍøÂç¹É·ÝÓÐÏÞ¹«Ë¾
** ÎÄ¼þÃèÊö:   MMIÐ­ÒéÃüÁîºêÎÄ¼þ
**
*********************************************************************************
**             ÐÞ¸ÄÀúÊ·¼ÇÂ¼
**===============================================================================
**| ÈÕÆÚ       | ×÷Õß   |  ÐÞ¸Ä¼ÇÂ¼
**===============================================================================
**| 2014/05/02 | Ò¶µÂÑæ |  ´´½¨µÚÒ»°æ±¾
*********************************************************************************/
#ifndef H_YX_MMI_PTYPE
#define H_YX_MMI_PTYPE          1


/* ÃüÁî´¦Àí½á¹¹Ìå */
typedef struct {
    INT8U cmd;
    void  (*entryproc)(INT8U cmd, INT8U *data, INT16U datalen);
} FUNCENTRY_MMI_T;

#define PE_TYPE_YXMMI      0x19      /* ÍâÉè±àºÅ */
#define COM_VER_MMI        0x01      /* Í¨ÐÅÐ­Òé°æ±¾ */

#define PE_ACK_MMI         0x01
#define PE_NAK_MMI         0x02

/* ¸´Î»ÀàÐÍ */
typedef enum {
    MMI_RESET_EVENT_NULL = 0,
    MMI_RESET_EVENT_NORMAL,         /* ³£¹æ¸´Î»,ÈçÖ÷¶¯¸´Î» */
    MMI_RESET_EVENT_ERROR,          /* Òì³£¸´Î»£¬Èç³öÏÖASSERTµÈÒì³£ÎÊÌâ */
    MMI_RESET_EVENT_WDG,            /* ¿´ÃÅ¹·Òì³£¸´Î» */
    MMI_RESET_EVENT_SLEEP,          /* ½øÈëÊ¡µç£¬¹Ø±ÕÖ÷»ú */
    MMI_RESET_EVENT_UPDATE,         /* ³ÌÐòÉý¼¶¸´Î»£¬ÈçÆô¶¯ÍâÉè³ÌÐòÉý¼¶£¬ÍâÉè³ÌÐòÉý¼¶Íê±Ï */
    MMI_RESET_EVENT_POWERDOWN,      /* Ö÷µçÔ´¶Ïµç */
    MMI_RESET_EVENT_MAX
} MMI_RESET_EVENT_E;

/*************************************************************************************************/
/*                           ¶¨Òå¹¦ÄÜÃüÁî±àÂë                                                    */
/*************************************************************************************************/
typedef enum {
    /* Í¨ÓÃ¹ÜÀíÐ­Òé */ 
    UP_PE_CMD_LINK_REQ                  = 0x01,                /* ÉÏµçÖ¸Ê¾ÇëÇó (UP) */
    DN_PE_ACK_LINK_REQ                  = 0x01,                /* ÉÏµçÖ¸Ê¾ÇëÇóÓ¦´ð (DOWN) */
    
    UP_PE_CMD_BEAT_REQ                  = 0x02,                /* Á´Â·Î¬»¤ÇëÇó (UP) */
    DN_PE_ACK_BEAT_REQ                  = 0x02,                /* Á´Â·Î¬»¤ÇëÇóÓ¦´ð (DOWN)*/
    
    DN_PE_CMD_VERSION_REQ               = 0x04,                /* °æ±¾²éÑ¯ÇëÇó(DOWN) */
    UP_PE_ACK_VERSION_REQ               = 0x04,                /* °æ±¾²éÑ¯ÇëÇóÓ¦´ð (UP) */ 
    
    DN_PE_CMD_RESET_INFORM              = 0x05,                /* Ö÷»ú¼´½«¸´Î»Í¨ÖªÇëÇó (DOWN) */
    UP_PE_ACK_RESET_INFORM              = 0x05,                /* Ö÷»ú¼´½«¸´Î»Í¨ÖªÇëÇóÓ¦´ð (UP) */
    
    UP_PE_CMD_PE_RESET_INFORM           = 0x06,                /* ÍâÉèÍ¨ÖªÖ÷»ú£¬ÍâÉè¼´½«¸´Î»Í¨ÖªÇëÇó (UP) */
    DN_PE_ACK_PE_RESET_INFORM           = 0x06,                /* ÍâÉèÍ¨ÖªÖ÷»ú£¬ÍâÉè¼´½«¸´Î»Í¨ÖªÇëÇó (DOWN)*/
    
    DN_PE_CMD_RESTART_REQ               = 0x07,                /* ¸´Î»ÖØÆôÇëÇó (DOWN) */
    UP_PE_ACK_RESTART_REQ               = 0x07,                /* ¸´Î»ÖØÆôÇëÇóÓ¦´ð (UP) */
    
    UP_PE_CMD_HOST_RESET_INFORM         = 0x08,                /* ÍâÉèÍ¨ÖªÖ÷»ú£¬ÍâÉè¼´½«¹Ø±Õ»òÖØÆôÖ÷»úÍ¨ÖªÇëÇó (UP) */
    DN_PE_ACK_HOST_RESET_INFORM         = 0x08,                /* ÍâÉèÍ¨ÖªÖ÷»ú£¬ÍâÉè¼´½«¹Ø±Õ»òÖØÆôÖ÷»úÍ¨ÖªÇëÇóÓ¦´ð (DOWN)*/
    
    DN_PE_CMD_GET_RESET_REC             = 0x09,                /* ¸´Î»¼ÇÂ¼²éÑ¯ÇëÇó (DOWN) */
    UP_PE_ACK_GET_RESET_REC             = 0x09,                /* ¸´Î»¼ÇÂ¼²éÑ¯ÇëÇóÓ¦´ð (UP) */
    
    /* »ù´¡ÒµÎñÐ­Òé */
    DN_PE_CMD_CTL_GPIO                  = 0x41,                /* ¿ØÖÆGPIOÊä³ö (DOWN) */
    UP_PE_ACK_CTL_GPIO                  = 0x41,                /* ¿ØÖÆGPIOÊä³öÓ¦´ð (UP) */
    
    DN_PE_CMD_CLEAR_WATCHDOG            = 0x42,                /* ¿´ÃÅ¹·Î¹¹· (DOWN) */
    UP_PE_ACK_CLEAR_WATCHDOG            = 0x42,                /* ¿´ÃÅ¹·Î¹¹·Ó¦´ð (UP) */
    
    DN_PE_CMD_SET_REALCLOCK             = 0x43,                /* ÉèÖÃÊµÊ±Ê±ÖÓ (DOWN) */
    UP_PE_ACK_SET_REALCLOCK             = 0x43,                /* ÉèÖÃÊµÊ±Ê±ÖÓÓ¦´ð (UP) */
    
    UP_PE_CMD_REPORT_REALCLOCK          = 0x44,                /* ÉÏ±¨ÊµÊ±Ê±ÖÓ (DOWN) */
    DN_PE_ACK_REPORT_REALCLOCK          = 0x44,                /* ÉÏ±¨ÊµÊ±Ê±ÖÓÓ¦´ð (UP) */
    
    DN_PE_CMD_READ_REALCLOCK            = 0x45,                /* ¶ÁÈ¡ÊµÊ±Ê±ÖÓ (DOWN) */
    UP_PE_ACK_READ_REALCLOCK            = 0x45,                /* ¶ÁÈ¡ÊµÊ±Ê±ÖÓÓ¦´ð (UP) */
    
    DN_PE_CMD_SET_PARA                  = 0x46,                /* ÉèÖÃÍ¨ÓÃ²ÎÊý (DOWN) */
    UP_PE_ACK_SET_PARA                  = 0x46,                /* ÉèÖÃÍ¨ÓÃ²ÎÊýÓ¦´ð (UP) */
    
    UP_PE_CMD_GET_PARA                  = 0x47,                /* ´Ó»ú²éÑ¯Í¨ÓÃ²ÎÊý */
    DN_PE_ACK_GET_PARA                  = 0x47,                /* ´Ó»ú²éÑ¯Í¨ÓÃ²ÎÊýÓ¦´ð */
    
    DN_PE_CMD_CTL_FUNCTION              = 0x48,                /* ¹¦ÄÜ¿ØÖÆ */
    UP_PE_ACK_CTL_FUNCTION              = 0x48,                /* ¹¦ÄÜ¿ØÖÆÓ¦´ð */
    
    /* ÐÐÊ»¼ÇÂ¼ÒÇÒµÎñÐ­Òé */
    DN_PE_CMD_GET_ICCARD_INFO           = 0x61,                /* ¶Á¿¨ÇëÇó(DOWN) */
    UP_PE_ACK_GET_ICCARD_INFO           = 0x61,                /* ¶Á¿¨ÇëÇóÓ¦´ð (UP) */
    
    UP_PE_CMD_REPORT_ICCARD_INFO        = 0x62,                /* Ö÷¶¯ÉÏ±¨IC¿¨ÐÅÏ¢(UP) */
    DN_PE_ACK_REPORT_ICCARD_INFO        = 0x62,                /* Ö÷¶¯ÉÏ±¨IC¿¨ÐÅÏ¢Ó¦´ð(DOWN) */
    
    UP_PE_CMD_REPORT_ICCARD_DATA        = 0x63,                /* Ö÷¶¯ÉÏ±¨IC¿¨Ô­Ê¼Êý¾Ý(UP) */
    DN_PE_ACK_REPORT_ICCARD_DATA        = 0x63,                /* Ö÷¶¯ÉÏ±¨IC¿¨Ô­Ê¼Êý¾ÝÓ¦´ð(DOWN) */
    
    DN_PE_CMD_WRITE_ICCARD              = 0x64,                /* Ð´IC¿¨Êý¾Ý (DOWN) */
    UP_PE_ACK_WRITE_ICCARD              = 0x64,                /* Ð´IC¿¨Êý¾ÝÓ¦´ð (UP) */
    
    /* ´«¸ÐÆ÷ÐÅºÅÐ­Òé */
    UP_PE_CMD_REPORT_SENSOR_STATUS      = 0x71,                /* Ö÷¶¯ÉÏ±¨GPIO´«¸ÐÆ÷×´Ì¬(UP) */
    DN_PE_ACK_REPORT_SENSOR_STATUS      = 0x71,                /* Ö÷¶¯ÉÏ±¨GPIO´«¸ÐÆ÷×´Ì¬Ó¦´ð(DOWN) */
    DN_PE_CMD_SET_SENSOR_FILTER         = 0x72,                /* ÉèÖÃGPIOÂË²¨²ÎÊý(DOWN) */
    UP_PE_ACK_SET_SENSOR_FILTER         = 0x72,                /* ÉèÖÃGPIOÂË²¨²ÎÊýÓ¦´ð (UP) */
    DN_PE_CMD_SET_SENSOR_PARA           = 0x73,                /* ÉèÖÃGPIO´«¸ÐÆ÷²ÎÊý(DOWN) */
    UP_PE_ACK_SET_SENSOR_PARA           = 0x73,                /* ÉèÖÃGPIO´«¸ÐÆ÷²ÎÊýÓ¦´ð (UP) */
    
    UP_PE_CMD_REPORT_ODOPULSE           = 0x74,                /* Ö÷¶¯ÉÏ±¨Àï³ÌÂö³å(UP) */
    DN_PE_ACK_REPORT_ODOPULSE           = 0x74,                /* Ö÷¶¯ÉÏ±¨Àï³ÌÂö³åÓ¦´ð(DOWN) */
    DN_PE_CMD_SET_ODOPULSE_PARA         = 0x75,                /* ÉèÖÃÀï³ÌÂö³å²ÎÊý(DOWN) */
    UP_PE_ACK_SET_ODOPULSE_PARA         = 0x75,                /* ÉèÖÃÀï³ÌÂö³å²ÎÊýÓ¦´ð (UP) */
    
    DN_PE_CMD_SET_AD_PARA               = 0x78,                /* ÉèÖÃAD²É¼¯²ÎÊý(DOWN) */
    UP_PE_ACK_SET_AD_PARA               = 0x78,                /* ÉèÖÃAD²É¼¯²ÎÊýÓ¦´ð (UP) */
    DN_PE_CMD_GET_AD                    = 0x79,                /* »ñÈ¡ADÖµ(DOWN) */
    UP_PE_ACK_GET_AD                    = 0x79,                /* »ñÈ¡ADÖµÓ¦´ð (UP) */
    UP_PE_CMD_REPORT_AD                 = 0x7A,                /* Ö÷¶¯ÉÏ±¨ADÖµ(UP) */
    DN_PE_ACK_REPORT_AD                 = 0x7A,                /* Ö÷¶¯ÉÏ±¨ADÖµÓ¦´ð(DOWN) */
    
    UP_PE_CMD_REPORT_KEY                = 0x7B,                /* Ö÷¶¯ÉÏ±¨°´¼üÖµ(UP) */
    DN_PE_ACK_REPORT_KEY                = 0x7B,                /* Ö÷¶¯ÉÏ±¨°´¼üÖµÓ¦´ð(DOWN) */
    
    /* À©Õ¹´®¿ÚÒµÎñ */
    DN_PE_CMD_SET_UART_PARA             = 0x81,                /* ÉèÖÃÀ©Õ¹´®¿Ú²ÎÊý (DOWN) */
    UP_PE_ACK_SET_UART_PARA             = 0x81,                /* ÉèÖÃÀ©Õ¹´®¿Ú²ÎÊýÓ¦´ð (UP) */
    
    DN_PE_CMD_GET_UART_PARA             = 0x82,                /* »ñÈ¡À©Õ¹´®¿Ú²ÎÊý (DOWN) */
    UP_PE_ACK_GET_UART_PARA             = 0x82,                /* »ñÈ¡À©Õ¹´®¿Ú²ÎÊýÓ¦´ð (UP) */
    
    DN_PE_CMD_CTL_UART_POWER            = 0x83,                /* ¿ØÖÆÀ©Õ¹´®¿ÚµçÔ´ (DOWN) */
    UP_PE_ACK_CTL_UART_POWER            = 0x83,                /* ¿ØÖÆÀ©Õ¹´®¿ÚµçÔ´Ó¦´ð (UP) */
    
    UP_PE_CMD_UART_DATA_SEND            = 0x84,                 /* UARTÊý¾ÝÉÏÐÐÍ¸´«ÇëÇó(UP) */
    DN_PE_ACK_UART_DATA_SEND            = 0x84,                 /* UARTÊý¾ÝÉÏÐÐÍ¸´«ÇëÇóÓ¦´ð(DOWN) */
    DN_PE_CMD_UART_DATA_SEND            = 0x85,                 /* UARTÊý¾ÝÏÂÐÐÍ¸´«ÇëÇó (DOWN) */
    UP_PE_ACK_UART_DATA_SEND            = 0x85,                 /* UARTÊý¾ÝÏÂÐÐÍ¸´«ÇëÇóÓ¦´ð(UP) */
    
    
    UP_PE_CMD_RETIM_STATUS_REPORT       = 0x41,                /* µ÷¶ÈÆÁÊµÊ±×´Ì¬ÉÏ±¨ (UP)  */ 
    DN_PE_ACK_RETIM_STATUS_REPORT       = 0x41,                /* ¶Ôµ÷¶ÈÆÁÊµÊ±×´Ì¬ÉÏ±¨µÄÓ¦´ð (DOWN)  */
    

    UP_PE_CMD_EQUIPMENT_STATUS          = 0x44,                /* Ö÷»ú×Ô¼ì(UP) */
    DN_PE_ACK_EQUIPMENT_STATUS          = 0x44,                /* Ö÷»ú×Ô¼ìÓ¦´ð(DOWN) */

    DN_PE_CMD_EQUIPMENT_CLRDB_REQ       = 0x48,                /* »Ö¸´µÄÊý¾ÝÇøÇëÇó(DOWN) */
    UP_PE_ACK_EQUIPMENT_CLRDB_REQ       = 0x48,                /* »Ö¸´µÄÊý¾ÝÇøÓ¦´ð(UP) */
    UP_PE_CMD_EQUIPMENT_STATUS_GET      = 0x49,                /* ¿ª»úÇëÇóÖ÷»ú×´Ì¬ÇëÇó */
    DN_PE_ACK_EQUIPMENT_STATUS_GET      = 0x49,                /* ¿ª»úÇëÇóÖ÷»ú×´Ì¬Ó¦´ð */

    DN_PE_CMD_EQUIPMENT_STATUS_CHANGE   = 0x4A,                /* Ö÷»ú×´Ì¬ÇÐ»»¸æÖª(DOWN) */
    UP_PE_ACK_EQUIPMENT_STATUS_CHANGE   = 0x4A,                /* Ö÷»ú×´Ì¬ÇÐ»»Ó¦´ð(UP) */




    /* ÐÐÊ»¼ÇÂ¼ */
    DN_PE_CMD_DRIVER_LOGIN_STA          = 0x89,                 /* Ë¾»úµÇÂ¼×´¿ö¸æÖª(DOWN) */
    UP_PE_ACK_DRIVER_LOGIN_STA          = 0x89,                 /* Ë¾»úµÇÂ¼×´¿öÓ¦´ð(UP) */
    UP_PE_CMD_DRIVER_LOGIN_REQ          = 0x8a,                 /* Ë¾»úË¢¿¨½á¹û¸æÖª(UP) */
    DN_PE_ACK_DRIVER_LOGIN_REQ          = 0x8a,                 /* Ë¾»úË¢¿¨½á¹ûÓ¦´ð(DOWN) */
    UP_PE_CMD_CAR_CHECK_REQ             = 0x8b,                 /* ³µÁ¾ÌØÕ÷ÏµÊý²éÑ¯¡¢ÉèÖÃÇëÇó(UP) */
    DN_PE_ACK_CAR_CHECK_REQ             = 0x8b,                 /* ³µÁ¾ÌØÕ÷ÏµÊý²éÑ¯¡¢ÉèÖÃÓ¦´ð(DOWN) */
    UP_PE_CMD_SPEED_AVERAGE_REQ         = 0x8c,                 /* 15·ÖÖÓÆ½¾ùËÙ¶È(UP) */
    DN_PE_ACK_SPEED_AVERAGE_REQ         = 0x8c,                 /* 15·ÖÖÓÆ½¾ùËÙ¶ÈÓ¦´ð(DOWN) */
    UP_PE_CMD_DRIVE_RECORD_REQ          = 0x8d,                 /* ½üÁ½ÌìÁ¬Ðø¼ÝÊ»¼ÇÂ¼ÇëÇó(UP) */
    DN_PE_ACK_DRIVE_RECORD_REQ          = 0x8d,                 /* ½üÁ½ÌìÁ¬Ðø¼ÝÊ»¼ÇÂ¼Ó¦´ð(DOWN) */
    UP_PE_CMD_DRIVER_LOGIN_STA_REQ      = 0x8e,                 /* Ë¾»úµÇÂ¼×´¿öÇëÇó(UP) */
    DN_PE_ACK_DRIVER_LOGIN_STA_REQ      = 0x8e,                 /* Ë¾»úµÇÂ¼×´¿ö¸æÖª(DOWN) */
    
    DN_PE_CMD_COMBUS_MODE_SET           = 0xD0,                /* Í¨ÐÅ×ÜÏßÄ£Ê½ÉèÖÃÇëÇó(DOWN) */
    UP_PE_ACK_COMBUS_MODE_SET           = 0xD0,                /* Í¨ÐÅ×ÜÏßÄ£Ê½ÉèÖÃÇëÇóÓ¦´ð(UP) */
    DN_PE_CMD_COMBUS_ONOFF_SET          = 0xD1,                /* Í¨ÐÅ×ÜÏß¿ª¹ØÉèÖÃÇëÇó(DOWN) */
    UP_PE_ACK_COMBUS_ONOFF_SET          = 0xD1,                /* Í¨ÐÅ×ÜÏß¿ª¹ØÉèÖÃÇëÇóÓ¦´ð(UP) */
    DN_PE_CMD_COMBUS_RESET              = 0xD2,                /* Í¨ÐÅ×ÜÏß¸´Î»ÇëÇó(DOWN) */
    UP_PE_ACK_COMBUS_RESET              = 0xD2,                /* Í¨ÐÅ×ÜÏß¸´Î»ÇëÇóÓ¦´ð(UP) */


    UP_PE_CMD_TRANSMITION_UP            = 0xA0,                 /* Êý¾ÝÍ¸´«ÉÏÐÐÐ­Òé */ 
    DN_PE_ACK_TRANSMITION_UP            = 0xA0,                 /* Êý¾ÝÍ¸´«ÉÏÐÐÓ¦´ð */ 
    UP_PE_ACK_TRANSMITION_DOWN          = 0xA1,                 /* Êý¾ÝÍ¸´«ÏÂÐÐÓ¦´ð */    
    DN_PE_CMD_TRANSMITION_DOWN          = 0xA1,                 /* Êý¾ÝÍ¸´«ÏÂÐÐÐ­Òé */ 
    

    /* ÎÞÏßÏÂÔØ */
    DN_PE_CMD_WDOWN_REQ                 = 0xA4,                 /* ÎÞÏßÉý¼¶Ö÷³ÌÐò¿ªÊ¼ÇëÇó(DOWN) */
    UP_PE_ACK_WDOWN_REQ                 = 0xA4,                 /* ÎÞÏßÉý¼¶Ö÷³ÌÐò¿ªÊ¼ÇëÇóµÄÓ¦´ð (UP) */
    DN_PE_CMD_WDOWN_DATA_SEND           = 0xA5,                 /* ÎÞÏßÉý¼¶Ö÷³ÌÐòÊý¾Ý´«ÊäÇëÇó(DOWN) */
    UP_PE_ACK_WDOWN_DATA_SEND           = 0xA5,                 /* ÎÞÏßÉý¼¶Ö÷³ÌÐòÊý¾Ý´«ÊäÇëÇóµÄÓ¦´ð (UP) */


    DN_PE_CMD_RPEXT_PARA_SET            = 0xC3,                 /* ÎïÀíÀ©Õ¹´®¿Ú²ÎÊýÉèÖÃÇëÇó(DOWN) */
    UP_PE_ACK_RPEXT_PARA_SET            = 0xC3,                 /* ÎïÀíÀ©Õ¹´®¿Ú²ÎÊýÉèÖÃÇëÇóÓ¦´ð(UP) */
    DN_PE_CMD_PEXT_PARA_QUER           = 0xC4,                 /* ÎïÀíÀ©Õ¹´®¿Ú²ÎÊý²éÑ¯ÇëÇó(DOWN) */
    UP_PE_ACK_RPEXT_PARA_QUER           = 0xC4,                 /* ÎïÀíÀ©Õ¹´®¿Ú²ÎÊý²éÑ¯ÇëÇóÓ¦´ð(UP) */
    DN_PE_CMD_PEXT_POWER_SET           = 0xC5,                 /* ÎïÀíÀ©Õ¹´®¿ÚµçÔ´ÉèÖÃÇëÇó(DOWN) */
    UP_PE_ACK_RPEXT_POWER_SET           = 0xC5,                 /* ÎïÀíÀ©Õ¹´®¿ÚµçÔ´ÉèÖÃÇëÇóÓ¦´ð(UP) */
    

    //UP_PE_CMD_TRIG_WIRELESS_REQ         = 0xF9,                 /* Ö¸Ê¾³µÌ¨Ô¶³ÌÉý¼¶ÇëÇó(UP) */
    //DN_PE_ACK_TRIG_WIRELESS_REQ         = 0xF9                /* Ö¸Ê¾³µÌ¨Ô¶³ÌÉý¼¶ÇëÇóÓ¦´ð(DOWN) */
    
    /* CANÍ¨Ñ¶ */
    DN_PE_CMD_CAN_TRANS_DATA            = 0x90,                /* CANÒµÎñÐ­ÒéÍ¸´«ÇëÇó(DOWN) */
    UP_PE_CMD_CAN_TRANS_DATA            = 0x90,                /* CANÒµÎñÐ­ÒéÍ¸´«ÇëÇó(UP) */
    
    DN_PE_CMD_CAN_SET_PARA              = 0x91,                /* CANÍ¨ÐÅ²ÎÊýÉèÖÃÇëÇó(DOWN) */
    UP_PE_ACK_CAN_SET_PARA              = 0x91,                /* CANÍ¨ÐÅ²ÎÊýÉèÖÃÇëÇóµÄÓ¦´ð(UP) */
    
    DN_PE_CMD_CAN_CLOSE                 = 0x92,                /* CANÍ¨ÐÅ¹Ø±ÕÇëÇó(DOWN) */
    UP_PE_ACK_CAN_CLOSE                 = 0x92,                /* CANÍ¨ÐÅ¹Ø±ÕÇëÇóµÄÓ¦´ð(UP) */
    
    DN_PE_CMD_CAN_RESET                 = 0x93,                /* CANÍ¨ÐÅ×ÜÏß¸´Î»ÇëÇó(DOWN) */
    UP_PE_ACK_CAN_RESET                 = 0x93,                /* CANÍ¨ÐÅ×ÜÏß¸´Î»ÇëÇóµÄÓ¦´ð(UP) */
    
    DN_PE_CMD_CAN_SET_FILTER_ID_LIST    = 0x94,                /* CANÂË²¨IDÉèÖÃ,ÁÐ±íÊ½(DOWN) */
    UP_PE_ACK_CAN_SET_FILTER_ID_LIST    = 0x94,                /* CANÂË²¨IDÉèÖÃ,ÁÐ±íÊ½µÄÓ¦´ð(UP) */
    
    DN_PE_CMD_CAN_SET_FILTER_ID_MASK    = 0x95,                /* CANÂË²¨IDÉèÖÃ,ÑÚÂëÊ½(DOWN) */
    UP_PE_ACK_CAN_SET_FILTER_ID_MASK    = 0x95,                /* CANÂË²¨IDÉèÖÃ,ÑÚÂëÊ½µÄÓ¦´ð(UP) */
    
    UP_PE_CMD_CAN_DATA_REPORT           = 0x98,                /* Ö÷¶¯ÉÏ±¨CANÊý¾ÝÇëÇó(UP) */
    DN_PE_ACK_CAN_DATA_REPORT           = 0x98,                /* Ö÷¶¯ÉÏ±¨CANÊý¾ÝÇëÇóµÄÓ¦´ð(DOWN)*/
    
    DN_PE_CMD_CAN_SEND_DATA             = 0x99,                /* ·¢ËÍCANÊý¾ÝÇëÇó(UP) */
    UP_PE_ACK_CAN_SEND_DATA             = 0x99,                /* ·¢ËÍCANÊý¾ÝÇëÇóµÄÓ¦´ð(DOWN)*/
    
    UP_PE_CMD_CAN_BUS_STATUS_REPORT     = 0x9A,                /* Ö÷¶¯ÉÏ±¨CAN×ÜÏß×´Ì¬ó(UP) */
    DN_PE_ACK_CAN_BUS_STATUS_REPORT     = 0x9A,                /* Ö÷¶¯ÉÏ±¨CAN×ÜÏß×´Ì¬µÄÓ¦´ð(DOWN) */
    

    /* Åö×²²à·­ */
    DN_PE_CMD_HITCK_DMC_START           = 0xA1,                 /* Æô¶¯Åö×²¼ì²â±ê¶¨ÇëÇó(DOWN) */
    UP_PE_ACK_HITCK_DMC_START           = 0xA1,                 /* Æô¶¯Åö×²¼ì²â±ê¶¨Ó¦´ð(UP) */
    
    DN_PE_CMD_HITCK_DMC_STOP            = 0xA2,                 /* Í£Ö¹Åö×²¼ì²â±ê¶¨ÇëÇó(DOWN) */
    UP_PE_ACK_HITCK_DMC_STOP            = 0xA2,                 /* Í£Ö¹Åö×²¼ì²â±ê¶¨Ó¦´ð(UP) */
    
    DN_PE_CMD_HITCK_PARA_SET            = 0xA3,                 /* Åö×²¼ì²â²ÎÊýÉèÖÃÇëÇó(DOWN) */
    UP_PE_ACK_HITCK_PARA_SET            = 0xA3,                 /* Åö×²¼ì²â²ÎÊýÉèÖÃÓ¦´ð(UP) */
    
    UP_PE_CMD_HITCK_REPORT              = 0xA4,                 /* Åö×²¼ì²âÐÅºÅÉÏ±¨(UP) */
    DN_PE_ACK_HITCK_REPORT              = 0xA4,                 /* Åö×²¼ì²âÐÅºÅÉÏ±¨Ó¦´ð(DOWN) */
    
    /* GPSÄ£¿é */
    DN_PE_CMD_SET_GPS_UART              = 0xB1,                 /* ÉèÖÃGPS´®¿ÚÍ¨ÐÅ²ÎÊýÇëÇó (DOWN) */
    UP_PE_ACK_SET_GPS_UART              = 0xB1,                 /* ÉèÖÃGPS´®¿ÚÍ¨ÐÅ²ÎÊýÇëÇóÓ¦´ð(UP) */
    DN_PE_CMD_CTL_GPS_POWER             = 0xB2,                 /* ¿ØÖÆGPSÄ£¿éµçÔ´ÇëÇó (DOWN) */
    UP_PE_ACK_CTL_GPS_POWER             = 0xB2,                 /* ¿ØÖÆGPSÄ£¿éµçÔ´ÇëÇóÓ¦´ð(UP) */
    UP_PE_CMD_GPS_DATA_SEND             = 0xB3,                 /* GPSÊý¾ÝÉÏÐÐÍ¸´«ÇëÇó(UP) */
    DN_PE_ACK_GPS_DATA_SEND             = 0xB3,                 /* GPSÊý¾ÝÉÏÐÐÍ¸´«ÇëÇóÓ¦´ð(DOWN) */
    DN_PE_CMD_GPS_DATA_SEND             = 0xB4,                 /* GPSÊý¾ÝÏÂÐÐÍ¸´«ÇëÇó (DOWN) */
    UP_PE_ACK_GPS_DATA_SEND             = 0xB4,                 /* GPSÊý¾ÝÏÂÐÐÍ¸´«ÇëÇóÓ¦´ð(UP) */
    
    /* ¹Ì¼þÉý¼¶ */
    UP_PE_CMD_FIRMWARE_UPDATE_REQ       = 0xE1,                 /* ´Ó»úÇëÇó¹Ì¼þÉý¼¶(UP) */
    DN_PE_ACK_FIRMWARE_UPDATE_REQ       = 0xE1,                 /* ´Ó»úÇëÇó¹Ì¼þÉý¼¶Ó¦´ð(DOWN) */
    
    DN_PE_CMD_FIRMWARE_UPDATE_REQ       = 0xE2,                 /* Ö÷»úÏÂ·¢¹Ì¼þÉý¼¶ÇëÇó (DOWN) */
    UP_PE_ACK_FIRMWARE_UPDATE_REQ       = 0xE2,                 /* Ö÷»úÏÂ·¢¹Ì¼þÉý¼¶ÇëÇóÓ¦´ð(UP) */
    
    UP_PE_CMD_FIRMWARE_DATA_REQ         = 0xE3,                 /* ´Ó»úÇëÇó¹Ì¼þÊý¾Ý(UP) */
    DN_PE_ACK_FIRMWARE_DATA_REQ         = 0xE3,                 /* ´Ó»úÇëÇó¹Ì¼þÊý¾ÝÓ¦´ð(DOWN) */
    
    UP_PE_CMD_INFORM_UPDATE_RESULT      = 0xE4,                 /* ¹Ì¼þ¸üÐÂ½á¹ûÍ¨Öª(UP) */
    DN_PE_ACK_INFORM_UPDATE_RESULT      = 0xE4,                 /* ¹Ì¼þ¸üÐÂ½á¹ûÍ¨ÖªÓ¦´ð(DOWN) */
    
    PROTOCOL_COMMAND_MAX
} PROTOCOL_COMMAND_E;
 
/* Í¨ÓÃ²ÎÊý¶¨Òå */
typedef enum {
    /* »ù±¾²ÎÊýÀà */
    PARA_BASE_START,
    PARA_MYTEL            = 0x01,                /* ±¾»úºÅÂë */
    PARA_SMSC             = 0x02,                /* ¶ÌÐÅ·þÎñÖÐÐÄºÅÂë */
    PARA_ALARMTEL         = 0x03,                /* ±¨¾¯ºÅÂë */
    
    PARA_SERVER1_MAIN     = 0x04,                /* µÚÒ»·þÎñÆ÷Í¨ÐÅ²ÎÊý£¨Ö÷£© */
    PARA_SERVER1_BACK     = 0x05,                /* µÚÒ»·þÎñÆ÷Í¨ÐÅ²ÎÊý£¨¸±£© */
    PARA_SERVER1_ATTRIB   = 0x06,                /* µÚÒ»·þÎñÆ÷Í¨ÐÅÊôÐÔ */
    PARA_SERVER1_AUTHCODE = 0x07,                /* µÚÒ»·þÎñÆ÷¼øÈ¨Âë */
    PARA_SERVER1_LINK     = 0x08,                /* µÚÒ»·þÎñÆ÷Á´Â·Î¬»¤²ÎÊý */
    PARA_SERVER2_MAIN     = 0x09,                /* µÚ¶þ·þÎñÆ÷Í¨ÐÅ²ÎÊý£¨Ö÷£© */
    PARA_SERVER2_BACK     = 0x0A,                /* µÚ¶þ·þÎñÆ÷Í¨ÐÅ²ÎÊý£¨¸±£© */
    PARA_SERVER2_ATTRIB   = 0x0B,                /* µÚ¶þ·þÎñÆ÷Í¨ÐÅÊôÐÔ */
    PARA_SERVER2_AUTHCODE = 0x0C,                /* µÚ¶þ·þÎñÆ÷¼øÈ¨Âë */
    PARA_SERVER2_LINK     = 0x0D,                /* µÚ¶þ·þÎñÆ÷Á´Â·Î¬»¤²ÎÊý */
    
    PARA_VEHICHE_PROVINCE = 0x0E,                /* ³µÁ¾¹éÊôµØ */
    PARA_VEHICHE_CODE     = 0x0F,                /* ³µÅÆºÅ */
    PARA_VEHICHE_COLOUR   = 0x10,                /* ³µÁ¾ÑÕÉ« */
    PARA_VEHICHE_BRAND    = 0x11,                /* ³µÁ¾·ÖÀà */
    PARA_VEHICHE_VIN      = 0x12,                /* ³µÁ¾VIN */
    
    PARA_DEVICEINFO       = 0x13,                /* Éè±¸ÐÅÏ¢ */
    PARA_SLEEP            = 0x14,                /* Ê¡µç²ÎÊý */
    PARA_AUTOREPT         = 0x15,                /* Ö÷¶¯ÉÏ±¨²ÎÊý */
    PARA_BASE_END,
    
    /* Êý¾ÝÀà */
    PARA_DATA_START = 0x80,
    PARA_DATA_GPS         = 0x81,                /* GPSÊý¾Ý */
    PARA_DATA_END,
    
    PARA_MAX
} PARA_E;

#endif
