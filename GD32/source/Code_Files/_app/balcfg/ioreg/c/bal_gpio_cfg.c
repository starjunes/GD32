/********************************************************************************
**
** æ–‡ä»¶å:     bal_gpio_cfg.c
** ç‰ˆæƒæ‰€æœ‰:   (c) 1998-2017 å¦é—¨é›…è¿…ç½‘ç»œè‚¡ä»½æœ‰é™å…¬å¸
** æ–‡ä»¶æè¿°:   è¯¥æ¨¡å—ä¸»è¦å®ç°GPIOæ¥å£çš„å®šä¹‰å’Œé…ç½®
**
*********************************************************************************
**                  ä¿®æ”¹å†å²è®°å½•
**===============================================================================
**| æ—¥æœŸ              | ä½œè€…        |  ä¿®æ”¹è®°å½•
**===============================================================================
**| 2017/05/12 | é»„è¿å³°    |  ç§»æ¤ã€ä¿®æ”¹ã€è§„èŒƒåŒ–
********************************************************************************/
#include "yx_includes.h"
#include "bal_gpio_cfg.h"
#include "port_gpio.h"
#include "port_adc.h"
/*
*********************************************************************************
* å®šä¹‰æ¨¡å—å˜é‡
*********************************************************************************
*/
static INT32U s_advalue, ct_readvin;

#if 0
#define ADC_18V               0x074A
#define ADC_20V               0x07D8
#define ADC_22V               0x08B0

#define ADC_8V                0x0314
#define ADC_10V               0x03EC
#define ADC_11V               0x0458
#else
/* 24vÏµÍ³ */
#define ADC_24V_LOW           1450
#define ADC_24V_RECV          1540

/* 12vÏµÍ³ */
#define ADC_12V_LOW           692
#define ADC_12V_RECV          740

#endif
/*
*********************************************************************************
*  çœ‹é—¨ç‹—ï¼š
*********************************************************************************
*/
static void InitWatchdog(void)
{
    // PORT_GpioConfigure(PIN_WATCHDOG, GPIO_DIR_OUT, 0);
}

// æ³¨ï¼šMCUè½¯ä»¶çœ‹é—¨ç‹—ç”±åº•å±‚è´Ÿè´£æ¸…ç‹—ï¼Œå¤–éƒ¨çœ‹é—¨ç‹—ç”±ä¸Šå±‚è´Ÿè´£æ¸…ç‹—
void bal_ClearWatchdog(void)
{
    PORT_ClearWatchdog();
#if 0
    if (PORT_GetGpioPinState(PIN_WATCHDOG)) {
        PORT_ClearGpioPin(PIN_WATCHDOG);
    } else {
        PORT_SetGpioPin(PIN_WATCHDOG);
    }
#endif
}

/* ¶¨Î»µÆ */
void bal_Init_PIN_GPSLED(void)
{
    PORT_GpioConfigure(PIN_GPSLED, GPIO_DIR_OUT, 1);
}
void bal_Pullup_GPSLED(void)
{
    PORT_SetGpioPin(PIN_GPSLED);
}
void bal_Pulldown_GPSLED(void)
{
    PORT_ClearGpioPin(PIN_GPSLED);
}
/* CANµÆ */
void bal_Init_PIN_CANLED(void)
{
    PORT_GpioConfigure(PIN_CANLED, GPIO_DIR_OUT, 0);
}
void bal_Pullup_CANLED(void)
{
    PORT_SetGpioPin(PIN_CANLED);
}
void bal_Pulldown_CANLED(void)
{
    PORT_ClearGpioPin(PIN_CANLED);
}
/*WIFIµÆ */
void bal_Init_PIN_WIFILED(void)
{
    PORT_GpioConfigure(PIN_WIFILED, GPIO_DIR_OUT, 0);
}
void bal_Pullup_WIFILED(void)
{
    PORT_SetGpioPin(PIN_WIFILED);
}
void bal_Pulldown_WIFILED(void)
{
    PORT_ClearGpioPin(PIN_WIFILED);
}




/********************************************************************************
*   æ¿ä¸Šç”µæºä½¿èƒ½å¼€å…³æ§åˆ¶
********************************************************************************/
// æ¿ä¸Šå¤–å›´3.3Vç”µæºå¼€å…³
static void Init_PIN_EXT3V(void)
{
	PORT_GpioConfigure(PIN_EXT3V, GPIO_DIR_OUT, 1);
}

void bal_Pullup_EXT3V(void)
{
    PORT_SetGpioPin(PIN_EXT3V);
}

void bal_PullDown_EXT3V(void)
{
    PORT_ClearGpioPin(PIN_EXT3V);
}

void bal_Pullup_EXT1V8(void)
{
    PORT_SetGpioPin(PIN_EXT1V8);
}

void bal_PullDown_EXT1V8(void)
{
    PORT_ClearGpioPin(PIN_EXT1V8);
}

// GSM4Vç”µæºDCèŠ¯ç‰‡ä½¿èƒ½
static void Init_PIN_GSM4VIC(void)
{
    PORT_GpioConfigure(PIN_GSM4V, GPIO_DIR_OUT, 1);
}

void bal_Pullup_GSM4VIC(void)
{
    PORT_SetGpioPin(PIN_GSM4V);
}

void bal_Pulldown_GSM4VIC(void)
{
    PORT_ClearGpioPin(PIN_GSM4V);
}

// GSMæ¨¡å—ç”µæºä½¿èƒ½å¼€å…³
static void Init_PIN_GSMPWR(void)
{
     PORT_GpioConfigure(PIN_GSMPWR, GPIO_DIR_OUT, 0);
}

void bal_Pullup_GSMPWR(void)
{
    PORT_SetGpioPin(PIN_GSMPWR);
    //PORT_ClearGpioPin(PIN_GSMPWR);
}

void bal_Pulldown_GSMPWR(void)
{
	PORT_ClearGpioPin(PIN_GSMPWR);
     //PORT_SetGpioPin(PIN_GSMPWR);
}

static void Init_PIN_GSMPWR2(void)
{
    PORT_GpioConfigure(PIN_GSMPU, GPIO_DIR_OUT, 0);
}

/* EC20´®¿ÚÍ¨µÀ¹¦ÄÜÇĞ»» */
void bal_Pulldown_232CON(void)
{
    //PORT_ClearGpioPin(PIN_232CON);
}
void bal_Pullup_232CON(void)
{
     //PORT_SetGpioPin(PIN_232CON);
}

/* USB¹©µç¿ØÖÆ */
void bal_Pulldown_VBUSCTL(void)
{
    PORT_ClearGpioPin(PIN_VBUSCTL);
}
void bal_Pullup_VBUSCTL(void)
{
     PORT_SetGpioPin(PIN_VBUSCTL);
}

/* ¼ÓÃÜĞ¾Æ¬»½ĞÑ */
void bal_Pulldown_HSMWK(void)
{
    //PORT_ClearGpioPin(PIN_HSMWK);
}
void bal_Pullup_HSMWK(void)
{
     //PORT_SetGpioPin(PIN_HSMWK);
}

/* ¼ÓÃÜĞ¾Æ¬µçÔ´¿ØÖÆ */
void bal_Pulldown_HSMPWR(void)
{
    PORT_ClearGpioPin(PIN_HSMPWR);
}
void bal_Pullup_HSMPWR(void)
{
     PORT_SetGpioPin(PIN_HSMPWR);
}

/* À¶ÑÀ/wifiµçÔ´¿ØÖÆ */
void bal_Pulldown_BTWLEN(void)
{
    //PORT_ClearGpioPin(PIN_BTWLEN);
}
void bal_Pullup_BTWLEN(void)
{
     //PORT_SetGpioPin(PIN_BTWLEN);
}

/* GPSµçÔ´¿ØÖÆ */
void bal_Pulldown_GPSPWR(void)
{
	PORT_ClearGpioPin(PIN_GPSPWR);
}
void bal_Pullup_GPSPWR(void)
{
	 PORT_SetGpioPin(PIN_GPSPWR);
}

/* PHYµçÔ´¿ØÖÆ */
void bal_Pulldown_PHYPWR(void)
{
    //PORT_ClearGpioPin(PIN_PHYPWR);
}
void bal_Pullup_PHYPWR(void)
{
     //PORT_SetGpioPin(PIN_PHYPWR);
}

/* ¸ß¾«¶È¶¨Î»Ä£¿éÈÈÆô¶¯¿ØÖÆ */
void bal_Pulldown_GPSBAT(void)
{
    PORT_ClearGpioPin(PIN_GPSBAT);
}
void bal_Pullup_GPSBAT(void)
{
     PORT_SetGpioPin(PIN_GPSBAT);
}

/* ÒôÆµ£¬¼ÓÃÜ£¬´®¿Ú×ª»»µçÔ´¿ØÖÆ */
void bal_Pulldown_5VEXT(void)
{
    PORT_ClearGpioPin(PIN_5VEXT);
}
void bal_Pullup_5VEXT(void)
{
     PORT_SetGpioPin(PIN_5VEXT);
}

/* ÍÓÂİÒÇµçÔ´¿ØÖÆ£¬À­¸ß¶Ïµç */
void bal_Pulldown_GYRPWR(void)
{
    PORT_ClearGpioPin(PIN_GYRPWR);
}
void bal_Pullup_GYRPWR(void)
{
     PORT_SetGpioPin(PIN_GYRPWR);
}
/* ±¸ÓÃµç³Ø³äµçÊ¹ÄÜ¿ØÖÆ */
void bal_Pulldown_CHGEN(void)
{
    /* 2022/07/15 Âß¼­·´Ïò À­¸ßÍ£Ö¹£¬À­µÍ³äµç*/
    PORT_SetGpioPin(PIN_CHGEN);
}
void bal_Pullup_CHGEN(void)
{
     /* 2022/07/15 Âß¼­·´Ïò À­¸ßÍ£Ö¹£¬À­µÍ³äµç*/
     PORT_ClearGpioPin(PIN_CHGEN);
}
/* CAN0 StandbyÄ£Ê½Ê¹ÄÜ¿ØÖÆ */
void bal_Pulldown_CAN0STB(void)
{
    PORT_ClearGpioPin(PIN_CAN0STB);
}
void bal_Pullup_CAN0STB(void)
{
     PORT_SetGpioPin(PIN_CAN0STB);
}

/* CAN1 StandbyÄ£Ê½Ê¹ÄÜ¿ØÖÆ */
void bal_Pulldown_CAN1STB(void)
{
    PORT_ClearGpioPin(PIN_CAN1STB);
}
void bal_Pullup_CAN1STB(void)
{
     PORT_SetGpioPin(PIN_CAN1STB);
}
/* CAN2 StandbyÄ£Ê½Ê¹ÄÜ¿ØÖÆ */
void bal_Pulldown_CAN2STB(void)
{
    PORT_ClearGpioPin(PIN_CAN2STB);
}
void bal_Pullup_CAN2STB(void)
{
     PORT_SetGpioPin(PIN_CAN2STB);
}

/* Í¨Ñ¶Ä£¿éµçÔ´µÍ¹¦ºÄÄ£Ê½¿ØÖÆ */
void bal_Pulldown_PSCTRL(void)
{
    PORT_ClearGpioPin(PIN_PSCTRL);
}
void bal_Pullup_PSCTRL(void)
{
     PORT_SetGpioPin(PIN_PSCTRL);
}

/* ±¸ÓÃµç³Ø×Ü¿ª¹Ø¿ØÖÆ */
void bal_Pulldown_BATSHUT(void)
{
    PORT_ClearGpioPin(PIN_BATSHUT);
}
void bal_Pullup_BATSHUT(void)
{
     PORT_SetGpioPin(PIN_BATSHUT);
}



/*
*********************************************************************************
**              è¾“å…¥ç®¡è„šæ“ä½œæ¥å£å®šä¹‰
*********************************************************************************
*/
/********************************************************************************
*   ACCå’ŒIGNçŠ¶æ€
********************************************************************************/
void bal_InitPort_ACC(void)
{
    PORT_GpioConfigure(PIN_ACCIN, GPIO_DIR_IN, 1);
}
BOOLEAN bal_ReadPort_ACC(void)
{
    if (PORT_GetGpioPinState(PIN_ACCIN)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

void bal_InitPort_ECALL(void)
{
    PORT_GpioConfigure(PIN_ECALL, GPIO_DIR_IN, 1);
}
BOOLEAN bal_ReadPort_ECALL(void)
{
    if (PORT_GetGpioPinState(PIN_ECALL)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

void bal_InitPort_BCALL(void)
{
    PORT_GpioConfigure(PIN_BCALL, GPIO_DIR_IN, 1);
}
BOOLEAN bal_ReadPort_BCALL(void)
{
    if (PORT_GetGpioPinState(PIN_BCALL)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

void bal_InitPort_PWRDECT(void)
{
     PORT_GpioConfigure(PIN_PWRDECT, GPIO_DIR_IN, 1);
}
BOOLEAN bal_ReadPort_PWRDECT(void)
{
    if (PORT_GetGpioPinState(PIN_PWRDECT)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

void bal_InitPort_GPSOPEN(void)
{
     PORT_GpioConfigure(PIN_GPSOPEN, GPIO_DIR_IN, 1);
}
BOOLEAN bal_ReadPort_GPSOPEN(void)
{
    if (PORT_GetGpioPinState(PIN_GPSOPEN)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

void bal_InitPort_GPSSHORT(void)
{
     PORT_GpioConfigure(PIN_GPSSHORT, GPIO_DIR_IN, 1);
}
BOOLEAN bal_ReadPort_GPSSHORT(void)
{
    if (PORT_GetGpioPinState(PIN_GPSSHORT)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

void bal_InitPort_CHG(void)
{
    PORT_GpioConfigure(PIN_CHGDECT, GPIO_DIR_IN, 1);
}

BOOLEAN bal_ReadPort_CHG(void)
{
    if (PORT_GetGpioPinState(PIN_CHGDECT)) {
        return TRUE;
    } else {
        return FALSE;
    }
}


/********************************************************************************
*   ä¸»ç”µæ¬ å‹çŠ¶æ€æ£€æµ‹
********************************************************************************/
void bal_InitPort_VINLOW(void)
{
    s_advalue = PORT_GetADCValue(ADC_MAINPWR);
}

/********************************************************************************
** å‡½æ•°å:     ReadPort_VINLOW
** å‡½æ•°æè¿°:   è¯»å–æ¬ å‹çŠ¶æ€
** å‚æ•°:       æ— 
** è¿”å›:       æ¬ å‹è¿”å›trueï¼Œæ­£å¸¸è¿”å›false
********************************************************************************/
BOOLEAN bal_ReadPort_VINLOW(void)
{
    if (++ct_readvin > 60) {
        ct_readvin = 0;
        s_advalue = PORT_GetADCValue(ADC_MAINPWR);
    }

    if (bal_ReadPort_PWRDECT()) {
        return false;
    }

    if (s_advalue == 0xffffffff) {
        return false;
    }
    if (s_advalue > ADC_24V_LOW) {
        if (s_advalue > ADC_24V_RECV) {
            return false;
        }else{
            return true;
        }
    } else if (s_advalue > ADC_12V_LOW) {
        if (s_advalue > ADC_12V_RECV) {
            return false;
        }else{
            return true;
        }
    }
    return false;
}


/********************************************************************************
** å‡½æ•°å:     ReadValue_VINLOW
** å‡½æ•°æè¿°:   è¯»å–ä¸»ç”µæºADCå€¼
** å‚æ•°:       æ— 
** è¿”å›:       è¿”å›ADCå€¼
********************************************************************************/
INT32U bal_ReadValue_VINLOW(void)
{
    return s_advalue;
}


/********************************************************************************
** å‡½æ•°å:     bal_gpio_InitCfg
** å‡½æ•°æè¿°:   åˆå§‹åŒ–GPIOé…ç½®
** å‚æ•°:       æ— 
** è¿”å›:       æ— 
** å¤‡æ³¨ï¼š      æœªæ³¨å†Œåˆ°input_regå’Œoutput_regä¸­çš„IOéƒ½è¦åœ¨æ­¤å‡½æ•°ä¸­åˆå§‹åŒ–
********************************************************************************/
void bal_gpio_InitCfg(void)
{
    InitWatchdog();
    Init_PIN_EXT3V();
    Init_PIN_GSM4VIC();
    Init_PIN_GSMPWR();
    Init_PIN_GSMPWR2();
}

//------------------------------------------------------------------------------
/* End of File */
