/******************************************************************************
**
** filename:     os_config.h
** copyright:    
** description:  该模块主要实现内核参数配置
**
-------------------------------------------------------------------------------
**             Revision history
**-----------------------------------------------------------------------------
**| 2014/03/09 | 叶德焰 |  创建文件
*******************************************************************************/
#ifndef OS_CONFIG_H
#define OS_CONFIG_H



/* Exported macro ------------------------------------------------------------*/
#if 0
/* define critical condition macro */
#define OS_ENTER_CRITICAL()            {NVIC_FLAG |= NVIC_REGS->ISER[1];\
                                        NVIC_REGS->ICER[1] |= NVIC_FLAG;\
                                        SysTick_REGS->CTRL &= ~SYSTICK_CTRL_TICKINT;}
                                        
#define OS_EXIT_CRITICAL()             {NVIC_REGS->ISER[1] |= NVIC_FLAG;\
                                        NVIC_FLAG = 0;\
                                        SysTick_REGS->CTRL |= SYSTICK_CTRL_TICKINT;}

INT32U __GetIPSR(void);
void __SETPRIMASK(void);
#define OS_ENTER_CRITICAL()   do {                              \
                                   if (!(__GetIPSR() & 0x1ff)) { \
                                      __SETPRIMASK();            \
                                   }                             \
                               } while(0)

#define OS_EXIT_CRITICAL()    do {                              \
                                   if (!(__GetIPSR() & 0x1ff)) { \
                                      __RESETPRIMASK();          \
                                   }                             \
                               } while(0)


#else

/*static __inline __asm void s777(void)
{
    cpsid i
}

static __inline __asm void s888(void)
{
    cpsie i
}*/

#define OS_ENTER_CRITICAL()    do {                                               \
                                   volatile register INT32U __regPriMask __asm("primask"); \
                                   cpu_sr = __regPriMask & 0x01;                  \
                                   if (cpu_sr == 0) {                             \
                                       __regPriMask |= 0x01;                 \
                                   }                                              \
                               } while(0)


#define OS_EXIT_CRITICAL()    do {                                       \
                                   volatile register INT32U __regPriMask __asm("primask"); \
                                   if (cpu_sr == 0) {                    \
                                       __regPriMask &= (~0x01);                 \
                                   }                                     \
                               } while(0)

//#define OS_ENTER_CRITICAL()            {}                      
//#define OS_EXIT_CRITICAL()             {}


#endif

void DAL_GPIO_ClearWatchdog(void);
#define ClearWatchdog()       DAL_GPIO_ClearWatchdog()

#endif
