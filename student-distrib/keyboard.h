/* keyboard.h -  functions related to interaction with
 * the keyboard
 */

#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "types.h"

#ifndef ASM

#define KEYBOARD_IRQ 1
//scan code translation
#define L_CTRL		0x1D
#define L_CTRL_R	0x9D	

#define L_SHIFT		0x2A
#define L_SHIFT_R	0xAA

#define R_SHIFT		0x36
#define R_SHIFT_R	0xB6

#define L_ALT		0x38
#define L_ALT_R		0xB8

#define CAPSLOCK	0x3A

//ASCII values translation
#define ESC			0
#define BACKSPACE	0x0E
#define TAB			0x09
#define ENTER		0x1C
#define F1			0x3B
#define F2 	 		0x3C
#define F3			0x3D
#define F4			0
#define F5			0
#define F6			0
#define F7			0
#define F8			0
#define F9			0
#define F10			0
#define F11			0
#define F12			0
#define L_pressed	0x26
#define ESC_pressed 0x01

#define ONE			0x02
#define TWO			0x03
#define THREE 		0x04

#define NUMBERLOCK	0
#define SCROLLLOCK	0
#define HOME		0
#define CURSOR_UP	0
#define PAGE_UP		0
#define CURSOR_LEFT	0
#define CURSOR_RIGHT 0
#define END 		0
#define CURSOR_DOWN	0
#define PAGE_DOWN	0
#define INSERT		0
#define DELETE		0x7F

/* Initialize the keyboard */
extern void keyboard_init();
/* Helper function for keyboard handler that deals with scancode to ASCII */
extern uint8_t keyboard_get_char();
/* Handler for keyboard when interrupts are raised */
extern void keyboard_handler();

#endif

#endif



