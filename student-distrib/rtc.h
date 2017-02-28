/* rtc.h - Functions used for the RTC
 */

#ifndef _RTC_H
#define _RTC_H

#include "types.h"

#ifndef ASM

#define NUM_TERM 					3

/* Initialize RTC */
void rtc_init(void);

/* Change rate of RTC */
void rtc_change_rate(uint8_t rate);

/* RTC handler, called when interrupt is raised */
void rtc_handler(void);

/* Flag that indicates if an interrupt has occured */
volatile uint32_t rtc_int_flag[NUM_TERM];

/* Open rtc by initialization */
int32_t rtc_open(const uint8_t* filename);

/* Wait for one tick of the time that indicated by the rtc rate */
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes);

/* Write to rtc by changing the rate */
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes);

/* Close the rtc */
int32_t rtc_close(int32_t fd);


#endif

#endif



