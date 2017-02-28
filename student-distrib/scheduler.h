/* scheduler.h -- Scheduler H files
 */

#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#include "types.h"
#include "paging.h"
#include "x86_desc.h"
#include "i8259.h"
#include "filesystem.h"
#include "keyboard.h"
#include "rtc.h"
#include "lib.h"
#include "terminal.h"
#include "pit.h"
#include "syscall.h"

volatile int sched_term;

/* Main body of the scheduler program */
void sched();

#endif
