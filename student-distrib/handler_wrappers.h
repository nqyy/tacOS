/* handler_wrappers.h - assembly wrappers for the handlers
 */

#ifndef _HANDLER_WRAPPERS_H
#define _HANDLER_WRAPPERS_H


/* Wrapper for pit */
extern void pit_irq(void);
/* Wrapper for keyboard */
extern void keyboard_irq(void);
/* Wrapper for RTC */
extern void rtc_irq(void);
/* Wrapper for system calls */
extern void systemcall_wrapper(void);

#endif


