/**
  ******************************************************************************
  * @file    gd32f30x_def.h 
  * @author  LEON
  * @version V3.1.2
  * @date    11/25/2010
  * @brief   file to be included by _dal layer and _app layer
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2009 STMicroelectronics</center></h2>
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __gd32f30x_DEF_H
#define __gd32f30x_DEF_H

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

/* typedef -------------------------------------------------------------------*/
typedef unsigned char        BOOLEAN;
typedef unsigned char        INT8U;              /* Unsigned  8 bit quantity  */
typedef signed   char        INT8S;              /* Signed    8 bit quantity  */
typedef unsigned short       INT16U;             /* Unsigned 16 bit quantity  */
typedef signed   short       INT16S;             /* Signed   16 bit quantity  */
typedef unsigned int         INT32U;             /* Unsigned 32 bit quantity  */
typedef signed   int         INT32S;             /* Signed   32 bit quantity  */
typedef float                FP32;               /* Single precision floating */
typedef double               FP64;               /* Double precision floating */

/* value define --------------------------------------------------------------*/
#ifndef True
#define True                 1
#endif
#ifndef False
#define False                0
#endif

#ifndef TRUE
#define TRUE                 1
#endif
#ifndef FALSE
#define FALSE                0
#endif

#ifndef true
#define true                 1
#endif
#ifndef false
#define false                0
#endif

#ifndef _FAILURE
#define _FAILURE             1
#endif
#ifndef _SUCCESS
#define _SUCCESS             2
#endif
#ifndef _OVERTIME
#define _OVERTIME            2
#endif

#ifndef ON
#define ON                   1
#endif
#ifndef OFF
#define OFF                  0
#endif

#ifndef NULL
#define NULL                 ((void*)0)
#endif

#endif /* __STM32F10x_DEF_H */

/******************* (C) COPYRIGHT 2010 YAXON NETWORK ***** END OF FILE ****/
