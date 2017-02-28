/* pit.c -- Programmable Interval Counter C files
 */

#include "pit.h"

#define CMD_Content	0x36
#define CMD_PORT 	0x43
#define TOTAL_FREQ	1193182
#define Channel0	0x40
#define LASTBYTE	0xFF
#define LEN_BYTE	0x8
#define PIT_IRQ 	0


/*
 * pit_init
 *   DESCRIPTION: Initialize the Programmable Interval Timer
 *   INPUTS: desired frequency
 *   OUTPUTS: pit that interrupts in our wanted frequency
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void pit_init(int32_t freq) {

	//first we tell the pit which channel we're setting
	outb(CMD_Content, CMD_PORT);

	int divisor = TOTAL_FREQ / freq;

	//we first send low byte
	outb((uint8_t)(divisor & LASTBYTE), Channel0);
	//then send high byte
	outb((uint8_t)((divisor >> LEN_BYTE) & LASTBYTE), Channel0);

	//finally we enable the pit interrupt
	enable_irq(PIT_IRQ);
}


/*
 * pit_handler
 *   DESCRIPTION: Handle the pit interrupt
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: take some actions specified by the scheduler for each interrupt
 */
void pit_handler(void) {
	//printf("a ");
	send_eoi(PIT_IRQ);
	//trigger the scheduler function for each interrupt
	sched();
}

