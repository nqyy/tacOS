/* keyboard.c - the C code for the functions related to
 * interaction with the keyboard
 */
#include "keyboard.h"

#include "types.h"
#include "i8259.h"
#include "lib.h"
#include "terminal.h"
#include "syscall.h"

uint8_t status_ctrl;
uint8_t status_shift;
uint8_t status_alt;
uint8_t status_capslock;
uint8_t status_enter;
uint8_t status_clear;
uint8_t status_backspace;
uint8_t status_altf1;
uint8_t status_altf2;
uint8_t status_altf3;

// Magic Numbers
#define KBD_CMD_PORT		0x64
#define KBD_DATA_PORT		0x60
#define Bitmask_lastbit		0x01
#define TOTAL_CHAR 			89
#define ASCII_A 			0x41
#define ASCII_Z 			0x5A
#define ASCII_a 			0x61
#define ASCII_z 			0x7A
#define ASCII_GAP 			0x20

// scan_code_set: an array of the scan codes corresponding
// to the letters typed from the keyboard. Note: got this
// information from the osdev website 
uint8_t	scan_code_set[TOTAL_CHAR] = {
	0, ESC, '1', '2', 
	'3', '4', '5', '6',
	'7', '8', '9', '0',
	'-', '=', BACKSPACE, TAB,
	'q', 'w', 'e', 'r',
	't', 'y', 'u', 'i',
	'o', 'p', '[', ']',
	ENTER, 0/*L_CTRL*/, 'a', 's',
	'd', 'f', 'g', 'h',
	'j', 'k', 'l', ';',
	'\'', '`', 0/*L_SHIFT*/, '\\',
	'z', 'x', 'c', 'v',
	'b', 'n', 'm', ',',
	'.', '/', 0/*R_SHIFT*/, '*',
	0/*L_ALT*/, ' ', 0/*CAPSLOCK*/, F1,
	F2, F3, F4, F5,
	F6, F7, F8, F9,
	F10, NUMBERLOCK, SCROLLLOCK, HOME,
	CURSOR_UP, PAGE_UP, 0, CURSOR_LEFT,
	0, CURSOR_RIGHT, 0, END,
	CURSOR_DOWN, PAGE_DOWN, INSERT, DELETE,
	0, 0, 0, F11,
	F12
};

// scan_code_set: an array of the scan codes corresponding
// to the letters typed from the keyboard when shift or 
// capslock is pressed. Note: got this information from
// the osdev website 
uint8_t	scan_code_set_upcase[TOTAL_CHAR] = {
	0, ESC, '!', '@', 
	'#', '$', '%', '^', 
	'&', '*', '(', ')',
	'_', '+', BACKSPACE, TAB,
	'Q', 'W', 'E', 'R', 
	'T', 'Y', 'U', 'I', 
	'O', 'P', '{', '}',
	ENTER, 0/*L_CTRL*/, 'A', 'S',
	'D', 'F', 'G', 'H',
	'J', 'K', 'L', ':',
	'"', '~', 0/*L_SHIFT*/, '|',
	'Z', 'X', 'C', 'V',
	'B', 'N', 'M', '<',
	'>', '?', 0/*R_SHIFT*/, '*',
	0/*L_ALT*/, ' ', 0/*CAPSLOCK*/, F1,
	F2, F3, F4, F5,
	F6, F7, F8, F9,
	F10, NUMBERLOCK, SCROLLLOCK, HOME,
	CURSOR_UP, PAGE_UP, 0, CURSOR_LEFT,
	0, CURSOR_RIGHT, 0, END,
	CURSOR_DOWN, PAGE_DOWN, INSERT, DELETE,
	0, 0, 0, F11,
	F12
};



/*
 * keyboard_init
 *   DESCRIPTION: Initializes the keyboard by initializing
 *                the status bits and enabling IRQ for
 *                keyboard
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void
keyboard_init() {
    status_ctrl = 0;
    status_shift = 0;
    status_alt = 0;
    status_altf1 = 0;
    status_altf2 = 0;
    status_altf3 = 0;
    status_capslock = 0;
    status_enter = 0;
    status_clear = 0;
    status_backspace = 0;

	//enable the keyboard, set the corresponding interrupt line 1
	enable_irq(1);
}


/*
 * keyboard_handler
 *   DESCRIPTION: handler called when interrupt is generated
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void keyboard_handler() {
	uint8_t output = 0;
	int j;

	cli();
	output = keyboard_get_char();
	sti();

    // for mac user the terminal switch
    if (status_ctrl == 1 && inb(KBD_DATA_PORT) == 0x02){
        terminal_switch(0);
        send_eoi(1);
        return;
    }
    if (status_ctrl == 1 && inb(KBD_DATA_PORT) == 0x03){
        terminal_switch(1);
        send_eoi(1);
        return;
    }
    if (status_ctrl == 1 && inb(KBD_DATA_PORT) == 0x04){
        terminal_switch(2);
        send_eoi(1);
        return;
    }
    
    if(status_enter == 1) {
    	terminal_enter();

        status_enter = 0;
    }
    else if(status_clear == 1) {
	    terminal_clear();                       // Clear video memory
	    cursor_init();                          // Initialize cursor to top of page
	    clear_kbd_buf();                        // Clear keyboard buffer

	    // print start
	    uint8_t msg[7] = "391OS> ";
	    for(j = 0; j < 7; j++) {
	        terminal_putc(msg[j]);
	    }

        status_clear = 0;
    }
    else if(status_backspace == 1) {
		terminal_backspace();
		
        status_backspace = 0;
    }
    else if(status_altf1 == 1){
    	terminal_switch(0);
    	status_altf1 = 0;
    	//printf("alt f1");
    }
    else if(status_altf2 == 1){
    	terminal_switch(1);
    	status_altf2 = 0;
    	//printf("alt f2");
    }
    else if(status_altf3 == 1){
    	terminal_switch(2);
    	status_altf3 = 0;
    	//printf("alt f3");
    }
    else if(output != 0) {            // output != 0 when it is a valid char to print
        // place in buffer
		terminal_getc(output);
        // echo on screen
        terminal_putc(output);	                             // lib.c
    }

	send_eoi(1);
	// asm volatile("iret");
}


/*
 * keyboard_get_char
 *   DESCRIPTION: gets scan code from port and translates into
 *                ASCII for uppercase and lowercase as well as
 *                special characters
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: ASCII value of the character typed in
 *   SIDE EFFECTS: none
 */
uint8_t
keyboard_get_char() {
	//get our scan code from the port
	uint8_t scan_code = 0;
	if (inb(KBD_CMD_PORT) & Bitmask_lastbit) scan_code = inb(KBD_DATA_PORT);
	//printf("ENTERED GET CHAR, %d\n", scan_code);
	//initially initialize an output that contains nothing
	uint8_t output = 0;
    if (scan_code == ESC_pressed) {
        send_eoi(KEYBOARD_IRQ);
        terminal_gogogo((uint8_t*)"program terminated by keyboard interrupt");
        halt(0);
        return ESC;
    }
	//first worry about status changes
	if (scan_code == L_CTRL) status_ctrl = 1;
	else if (scan_code == L_CTRL_R) status_ctrl = 0;
	else if (scan_code == L_SHIFT || scan_code == R_SHIFT) status_shift = 1;
	else if (scan_code == L_SHIFT_R || scan_code == R_SHIFT_R) status_shift = 0;
	else if (scan_code == L_ALT) status_alt = 1;
	else if (scan_code == L_ALT_R) status_alt = 0;
	else if (status_alt==1 && scan_code == F1) status_altf1 = 1;
	else if (status_alt==1 && scan_code == F2) status_altf2 = 1;
	else if (status_alt==1 && scan_code == F3) status_altf3 = 1;
	// else if (status_alt==1 && scan_code == ONE) status_altf1 = 1;
	// else if (status_alt==1 && scan_code == TWO) status_altf2 = 1;
	// else if (status_alt==1 && scan_code == THREE) status_altf3 = 1;
    else if (scan_code == ENTER) status_enter = 1;
    else if (scan_code == BACKSPACE) status_backspace = 1;
	else if (scan_code == CAPSLOCK) {
		if (status_capslock) status_capslock = 0;
		else status_capslock = 1;
	}
	//do we need to handle cases about FX?
	//we care only about things we defined in the arrays now
	//so we cut everything not defined in the array
	else if (scan_code >= TOTAL_CHAR) return output;
	//do we need to worry about ctrl command like force quit?

	else {
		//properly set the naive translation to the output from scan code
		//if control is pressed, several keys pressed at the same time may have special meaning
		if (status_ctrl) {
			if (scan_code == L_pressed) return status_clear = 1;
		}
		//follow the behavior of linux terminal exactly about alphanumerics
		if (status_shift) output = scan_code_set_upcase[scan_code];
		else output = scan_code_set[scan_code];
		//letters are the only things we need to worry about when caps
		if (status_capslock) {
			//invert the case
			if ((output >= ASCII_a) && (output <= ASCII_z)) output -= ASCII_GAP;
			else if ((output >= ASCII_A) && (output <= ASCII_Z)) output += ASCII_GAP;
		}
	}

	return output;
}
