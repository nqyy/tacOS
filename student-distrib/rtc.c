/* rtc.c - functions related to RTC in C
 */

#include "rtc.h"
#include "terminal.h"

#include "types.h"
#include "i8259.h"
#include "lib.h"


// Magic Numbers
#define RTC_CMD_PORT 				0x70
#define RTC_DATA_PORT 				0x71
#define Bitmask_sixth				0x40
#define Bitmask_highbits 			0xF0
#define Bitmask_lowbits 			0x0F
#define RTC_REGA 					0x8A
#define RTC_REGB 					0x8B
#define RTC_REGC 					0x8C
#define Test_rate					0x0F
#define RTC_IRQ						8
#define UPPER_FREQ					1024
#define LOWER_FREQ					2
#define FAIL 						-1
#define DOUBLE 						2
#define NUM_TERM 					3


/* Flag that indicates if an interrupt has occured */
volatile uint32_t rtc_int_flag[NUM_TERM];

/*
 * rtc_init
 *   DESCRIPTION: initializes the RTC and changes the rate
 *                also enables IRQs for RTC
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: ASCII value of the character typed in
 *   SIDE EFFECTS: none
 */
void rtc_init(void) {
	//mask the interrupt for initialization
	cli();
	int i;
	//change our flag to 0 during initialization
	for (i = 0; i < NUM_TERM; i++) rtc_int_flag[i] = 0;

	uint8_t val = 0;
	//according to osdev, we have to disable NMI here
	outb(RTC_REGB, RTC_CMD_PORT);
	//get the value of reg B and store it
	val = inb(RTC_DATA_PORT);
	//turn on the 6th bit of reg B
	val = val | Bitmask_sixth;
	//select reg B and send again
	outb(RTC_REGB, RTC_CMD_PORT);
	outb(val, RTC_DATA_PORT);


	//change the interrupt rate to 2 here
	rtc_change_rate(Test_rate);

	enable_irq(RTC_IRQ);

	//set the interrupt
	sti();
}

/*
 * rtc_change_rate
 *   DESCRIPTION: changes the rate for the RTC
 *   INPUTS: rate--rate to change to. 0xF is the slowest.
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void
rtc_change_rate(uint8_t rate) {
	//we are not allowed interrupt here, so we don't have to disable again
	//rate is only valid for bottom 4 bits
	rate = rate & Bitmask_lowbits;

	uint8_t val = 0;
	//disable NMI like before
	outb(RTC_REGA, RTC_CMD_PORT);
	//get the value of reg A and store it
	val = inb(RTC_DATA_PORT);
	//get the high bits
	val = val & Bitmask_highbits;
	//select reg A and send again
	outb(RTC_REGA, RTC_CMD_PORT);
	outb(val | rate, RTC_DATA_PORT); 
}


/*
 * rtc_handler
 *   DESCRIPTION: handler for when interrupt for RTC is 
 *                raised (calls test_interrupts program for
 *                testing) and sends EOI when finished
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void
rtc_handler(void) {
	cli();

	outb(RTC_REGC, RTC_CMD_PORT);
	inb(RTC_DATA_PORT);
	//check point 1 test 
	//test_interrupts();

	//clear the flag to indicate a new interrupt has occured
	int i;
	for (i = 0; i < NUM_TERM; i++) rtc_int_flag[i] = 0;
	send_eoi(RTC_IRQ);

	sti();
}


/*
 * rtc_open
 *   DESCRIPTION: open rtc by initialization
 *   INPUTS: filename pointer - do nothing here
 *   OUTPUTS: a number indicating if we succeed
 *   RETURN VALUE: 0 - success
 *   SIDE EFFECTS: open rtc by initialization
 */
int32_t rtc_open(const uint8_t* filename) {
	//we really have nothing to do except initialization
	rtc_init();
	return 0;
}


/*
 * rtc_read
 *   DESCRIPTION: wait for one tick of the time that indicated
 *				  by the rtc rate
 *   INPUTS: int fd - do nothing here
 *			 buf pointer - do nothing here
 *			 int nbytes - do nothing here
 *   OUTPUTS: a number indicating if we succeed
 *   RETURN VALUE: 0 - success
 *   SIDE EFFECTS: wait for one tick of time depending on rtc rate
 */
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes) {
	//set a flag to show that we are waiting for an interrupt
	rtc_int_flag[processing_terminal] = 1;
	//wait for next interrupt changes it back to 0
	while (rtc_int_flag[processing_terminal]);
	return 0;
}


/*
 * rtc_write
 *   DESCRIPTION: write to rtc by changing the rate
 *   INPUTS: int fd - do nothing here
 *			 buf pointer - points to a number indicating the new frequency
 *			 int nbytes - do nothing here
 *   OUTPUTS: a number indicating if we succeed
 *   RETURN VALUE: 0 - success
 *   SIDE EFFECTS: change the rate of the rtc
 */
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes) {
	//sanity check, complain if null is passed
	if (buf == NULL) return FAIL;
	//documentation tells us the thing in buf is a 4-byte int
	int32_t freq = *((int32_t*)buf);
	//check if the freq we get is within the range
	if (freq > UPPER_FREQ || freq < LOWER_FREQ) return FAIL;
	//initially start from 15
	uint8_t rate = Test_rate;
	//formulas from osdev to transfer freq to rate
	while (freq != DOUBLE) {
		freq /= DOUBLE;
		rate--;
		//protect the situation where 3 appears
		if (freq < DOUBLE) return FAIL;
	}

	//here our rate argument is guaranteed to make sense
	rtc_change_rate(rate);

	//return 0 upon sucess
	return 0;
}


/*
 * rtc_close
 *   DESCRIPTION: close the rtc
 *   INPUTS: int fd - do nothing here
 *   OUTPUTS: a number indicating if we succeed
 *   RETURN VALUE: 0 - success
 *   SIDE EFFECTS: none
 */
int32_t rtc_close(int32_t fd) {
	return 0;
}



