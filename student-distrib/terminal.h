/* terminal.h - functions related to terminal input and output
 */

#ifndef _TERMINAL_H
#define _TERMINAL_H

#include "types.h"

// Magic Numbers
#define VIDEO                   0xB8000
#define NUM_COLS                80
#define NUM_ROWS                24
#define ATTRIB                  0x7
#define KBD_BUF_LEN             128
#define MASK_LAST2BYTES         0xFF
#define VGA_PORT_ADDR           0x03D4
#define VGA_PORT_DATA           0x03D5
#define PAGE_4KB                0X1000
#define NUM_TERMINAL 			3
#define SPLIT_ONE 80 * 24 + 26
#define SPLIT_TWO 80 * 24 + 52

typedef struct terminal{
	int id;
	// Cursor Position
	int cursor_x;
	int cursor_y;
	
	// Keyboard buffer
	uint8_t kbd_buf[KBD_BUF_LEN];
	uint8_t kbd_buf_copy[KBD_BUF_LEN];
	int kbd_buf_count;
	int cur_pid;

	//enter flag
	volatile uint8_t enter;
	//process num
	int num_process;
	//save esp, ebp
	uint32_t esp;
	uint32_t ebp;
}terminal_t;

terminal_t terminal[NUM_TERMINAL];
volatile int running_terminal;
volatile int processing_terminal;
volatile int next_terminal;

/* Open the terminal */
int32_t terminal_open(const uint8_t* filename);
/* Close the terminal */
int32_t terminal_close(int32_t fd);
/* Initialize the terminal */
void terminal_init(void);
/* Clear the terminal */
void terminal_clear(void);
/* Initialize cursor to top of page */
void cursor_init(void);
/* Clear the keyboard buffer */
void clear_kbd_buf(void);
/* Read character from terminal */
void terminal_getc(uint8_t c);
/* Print character on terminal */
void terminal_putc(uint8_t c);
/* Deal with backspace */
void terminal_backspace(void);
/* Deal with enter */
void terminal_enter(void);
/* Scroll up */
void terminal_scroll_up(int i);
/* Update cursor */
void terminal_update_cursor(int i);
/* Read from terminal */
int32_t terminal_read(int32_t fd, uint8_t *buf, int32_t nbytes);
/* Write to terminal nbytes */
int32_t terminal_write(int32_t fd, const uint8_t *buf, int32_t nbytes);
/* Write to terminal ended with '\0' */
uint32_t terminal_gogogo(const uint8_t *buf);
/* Change terminal*/
void terminal_switch(int terminal_id);
// status bar
void bar_on();

#endif


