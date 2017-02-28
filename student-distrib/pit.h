/* pit.h -- Programmable Interval Counter H files
 */

#ifndef _PIT_H
#define _PIT_H

#ifndef ASM

#include "types.h"
#include "paging.h"
#include "x86_desc.h"
#include "i8259.h"
#include "filesystem.h"
#include "keyboard.h"
#include "rtc.h"
#include "lib.h"
#include "terminal.h"
#include "scheduler.h"

/* Initialize the Programmable Interval Timer */
void pit_init(int32_t freq);

/* Handle the pit interrupt */
void pit_handler(void);

#endif

#endif

