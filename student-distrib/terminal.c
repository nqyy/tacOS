/* terminal.c - the C code for the functions related to
 * printing and reading from terminal
 */

#include "terminal.h"
#include "lib.h"
#include "types.h"
#include "paging.h"

// Cursor Position
// static int cursor_x;
// static int cursor_y;
// Video memory position
static char* video_mem = (char *)VIDEO;

// Keyboard buffer
// static uint8_t kbd_buf[KBD_BUF_LEN];
// static uint8_t kbd_buf_copy[KBD_BUF_LEN];
// static int kbd_buf_count;

// Enter flag
// volatile uint8_t enter;

/*
 * terminal_open
 *   DESCRIPTION: Opens the terminal and initializes it
 *   INPUTS: filename--not used
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
int32_t terminal_open(const uint8_t* filename)
{
    terminal_init();
    return 0;
}

/*
 * terminal_close
 *   DESCRIPTION: Closes terminal--does nothing
 *   INPUTS: fd--not used
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
int32_t terminal_close(int32_t fd)
{
    return 0;
}

/*
 * terminal_init
 *   DESCRIPTION: Initializes terminal by clearing screen initializing
 *                cursor to top of the page and clearing the keyboard
 *                buffer
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void terminal_init(void) {
    int i;
    for(i=0; i<NUM_TERMINAL; i++)
    {
        running_terminal = i;
        terminal_clear();                       // Clear video memory
        cursor_init();                          // Initialize cursor to top of page
        clear_kbd_buf();                        // Clear keyboard buffer
        terminal[i].id = i;
        terminal[i].enter = 0;
        terminal[i].num_process = 0;
        terminal[i].cur_pid = -1;
    }
    running_terminal = 0;
    processing_terminal = 0;
    next_terminal = 0;
}

/*
 * terminal_clear
 *   DESCRIPTION: Clears the terminal screen by printing spaces
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void terminal_clear(void) {
    int32_t j;
    for(j=0; j<NUM_ROWS*NUM_COLS; j++) {
        *(uint8_t *)(video_mem + (j << 1)) = ' ';
        *(uint8_t *)(video_mem + (j << 1) + 1) = ATTRIB;

        *(uint8_t *)(video_mem + (running_terminal+1)*PAGE_4KB  + (j << 1)) = ' ';
        *(uint8_t *)(video_mem + (running_terminal+1)*PAGE_4KB  + (j << 1) + 1) = ATTRIB;
    }
}

/*
 * cursor_init
 *   DESCRIPTION: Initialize the cursor to the top of the page
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: changes position of cursor
 */
void cursor_init(void) {
    terminal[running_terminal].cursor_x = 0;                            // Initialize cursor to top of page (lib.c)
    terminal[running_terminal].cursor_y = 0;

    terminal_update_cursor(running_terminal);
}

/*
 * clear_kbd_buf
 *   DESCRIPTION: Clears the keyboard buffer by changing all the 
 *                content to '\0' and setting count to 0
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void clear_kbd_buf(void) {
    int j;                                    // Clear buffer and set number of chars in buffer to 0
    for(j = 0; j < KBD_BUF_LEN; j++) {
        terminal[running_terminal].kbd_buf[j] = '\0';
    }

    terminal[running_terminal].kbd_buf_count = 0;
}

/*
 * terminal_getc
 *   DESCRIPTION: Puts character read from the terminal into the keyboard
 *                buffer
 *   INPUTS: Character from terminal
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void terminal_getc(uint8_t c) {
    if(terminal[running_terminal].kbd_buf_count < KBD_BUF_LEN) {
        terminal[running_terminal].kbd_buf[terminal[running_terminal].kbd_buf_count] = c;
        terminal[running_terminal].kbd_buf_count++;
    }
}

/*
 * terminal_putc
 *   DESCRIPTION: Puts the character onto the screen, changes line and
 *                scrolls if necessary
 *   INPUTS: Character to put on the screen
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Updates cursor
 */
void terminal_putc(uint8_t c) {
    //printf("%d %d, ",terminal[running_terminal].cursor_x, terminal[running_terminal].cursor_y);
    if(terminal[running_terminal].cursor_x >= NUM_COLS) {
        if(terminal[running_terminal].cursor_y == NUM_ROWS - 1) {
            terminal_scroll_up(running_terminal);
            terminal[running_terminal].cursor_y--;
        }
        terminal[running_terminal].cursor_x = 0;
        terminal[running_terminal].cursor_y++;
    }

    if(terminal[running_terminal].kbd_buf_count < KBD_BUF_LEN) {
        *(uint8_t *)(video_mem + ((NUM_COLS*terminal[running_terminal].cursor_y + terminal[running_terminal].cursor_x) << 1)) = c;
        *(uint8_t *)(video_mem + ((NUM_COLS*terminal[running_terminal].cursor_y + terminal[running_terminal].cursor_x) << 1) + 1) = ATTRIB;
        terminal[running_terminal].cursor_x++;
    }
    
    terminal_update_cursor(running_terminal);
}

/*
 * terminal_backspace
 *   DESCRIPTION: Deals with what happens when backspace is pressed, prints spaces to
 *                erase from screen and removes from keyboard buffer, also deals with 
 *                erasing from second line
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: updates cursor
 */
void terminal_backspace(void) {
    if(terminal[running_terminal].kbd_buf_count == 0) return;

    if(terminal[running_terminal].cursor_x != 0) {
        terminal[running_terminal].kbd_buf_count--;
        terminal[running_terminal].kbd_buf[terminal[running_terminal].kbd_buf_count] = '\0';
        terminal[running_terminal].cursor_x--;

        *(uint8_t *)(video_mem + ((NUM_COLS*terminal[running_terminal].cursor_y + terminal[running_terminal].cursor_x) << 1)) = ' ';
        *(uint8_t *)(video_mem + ((NUM_COLS*terminal[running_terminal].cursor_y + terminal[running_terminal].cursor_x) << 1) + 1) = ATTRIB;
    }
    else if(terminal[running_terminal].cursor_x == 0 && terminal[running_terminal].kbd_buf_count != 0) {
        terminal[running_terminal].kbd_buf_count--;
        terminal[running_terminal].kbd_buf[terminal[running_terminal].kbd_buf_count] = '\0';
        terminal[running_terminal].cursor_x = NUM_COLS-1;
        terminal[running_terminal].cursor_y--;

        *(uint8_t *)(video_mem + ((NUM_COLS*terminal[running_terminal].cursor_y + terminal[running_terminal].cursor_x) << 1)) = ' ';
        *(uint8_t *)(video_mem + ((NUM_COLS*terminal[running_terminal].cursor_y + terminal[running_terminal].cursor_x) << 1) + 1) = ATTRIB;
    }

    terminal_update_cursor(running_terminal);
}

/*
 * terminal_enter
 *   DESCRIPTION: Deals with what happens when enter is pressed, scrolls if necessary,
 *                updates cursor, makes a copy of the keyboard buffer and sets the enter
 *                flag to high so that terminal_read jumps out of the while loop
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: updates buffer, clears keyboard buffer
 */
void terminal_enter(void) {
    if(terminal[running_terminal].cursor_y == NUM_ROWS - 1) {
        terminal_scroll_up(running_terminal);
        terminal[running_terminal].cursor_y--;
    }
    terminal[running_terminal].cursor_x = 0;
    terminal[running_terminal].cursor_y++;

    terminal_update_cursor(running_terminal);

    int i;
    for(i = 0; i < KBD_BUF_LEN; i++) {
        terminal[running_terminal].kbd_buf_copy[i] = terminal[running_terminal].kbd_buf[i];
    }

    clear_kbd_buf();

    terminal[running_terminal].enter = 1;
}

/*
 * terminal_scroll_up
 *   DESCRIPTION: Screen scrolls up by shifting everything up and clearing last line
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void terminal_scroll_up(int j) {
    int32_t i;
    if(running_terminal == j)
    {
        // shift up
        for(i=0; i<(NUM_ROWS*NUM_COLS - NUM_COLS); i++) {
            *(uint8_t *)(video_mem + (i << 1)) = *(uint8_t *)(video_mem + (i << 1) + 2*NUM_COLS);
            *(uint8_t *)(video_mem + (i << 1) + 1) = *(uint8_t *)(video_mem + (i << 1) + 1 + 2*NUM_COLS);
        }
        // clear line
        for(i = (NUM_ROWS*NUM_COLS - NUM_COLS); i < NUM_ROWS*NUM_COLS; i++) {
            *(uint8_t *)(video_mem + (i << 1)) = ' ';
            *(uint8_t *)(video_mem + (i << 1) + 1) = ATTRIB;
        }
    }

    // shift up
    for(i=0; i<(NUM_ROWS*NUM_COLS - NUM_COLS); i++) {
        *(uint8_t *)((video_mem + (j+1)*PAGE_4KB) + (i << 1)) = *(uint8_t *)((video_mem + (j+1)*PAGE_4KB) + (i << 1) + 2*NUM_COLS);
        *(uint8_t *)((video_mem + (j+1)*PAGE_4KB) + (i << 1) + 1) = *(uint8_t *)((video_mem + (j+1)*PAGE_4KB) + (i << 1) + 1 + 2*NUM_COLS);
    }
    // clear line
    for(i = (NUM_ROWS*NUM_COLS - NUM_COLS); i < NUM_ROWS*NUM_COLS; i++) {
        *(uint8_t *)((video_mem + (j+1)*PAGE_4KB) + (i << 1)) = ' ';
        *(uint8_t *)((video_mem + (j+1)*PAGE_4KB) + (i << 1) + 1) = ATTRIB;
    }
    
}

/*
 * terminal_update_cursor
 *   DESCRIPTION: Updates position of cursor displayed (taken from osdev)
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void terminal_update_cursor(int i)
{
    if(i!=running_terminal) return;
    unsigned short position=(terminal[i].cursor_y*NUM_COLS) + terminal[i].cursor_x;
    //printf("%d %d, ",terminal[i].cursor_x, terminal[i].cursor_y);

    // cursor LOW port to vga INDEX register
    outb(0x0F, VGA_PORT_ADDR);
    outb((unsigned char)(position&MASK_LAST2BYTES), VGA_PORT_DATA);
    // cursor HIGH port to vga INDEX register
    outb(0x0E, VGA_PORT_ADDR);
    outb((unsigned char)((position>>8)&MASK_LAST2BYTES), VGA_PORT_DATA);
}

/*
 * terminal_read
 *   DESCRIPTION: Copies content of keyboard buffer (which is saved in kbd_buf_copy)
 *                into the buf passed in (waits for enter to be pressed)
 *   INPUTS: fd--file descriptor, not used
 *           buf--buffer to copy keyboard buffer into
 *           nbytes--number of bytes to copy
 *   OUTPUTS: none
 *   RETURN VALUE: number of characters in buffer returned
 *   SIDE EFFECTS: clears keyboard buffer again
 */
int32_t terminal_read(int32_t fd, uint8_t *buf, int32_t nbytes) {
    int i;

    while(!terminal[processing_terminal].enter);
    terminal[processing_terminal].enter = 0;

    cli();
    if(nbytes > KBD_BUF_LEN-1) nbytes = KBD_BUF_LEN-1;
    for(i = 0; i < nbytes && terminal[processing_terminal].kbd_buf_copy[i] != '\0'; i++) {
        buf[i] = terminal[processing_terminal].kbd_buf_copy[i];
    }
    buf[i] = '\n';
    sti();

    clear_kbd_buf();

    return i + 1;
}

/*
 * terminal_write
 *   DESCRIPTION: Writes the content of buf to the terminal
 *   INPUTS: fd--file descriptor, not used
 *           buf--buffer to write onto screen
 *           nbytes--number of bytes to write
 *   OUTPUTS: none
 *   RETURN VALUE: number of characters to write
 *   SIDE EFFECTS: updates cursor
 */
int32_t terminal_write(int32_t fd, const uint8_t *buf, int32_t nbytes) {
    int i = 0;
    cli();
    if(running_terminal == processing_terminal)
    {
        for (i = 0; i < nbytes; i++){
            /* revised verison of "putc" */
            if(buf[i] == '\n' || buf[i] == '\r') 
            {
                terminal[running_terminal].cursor_y += 1;
                if(terminal[running_terminal].cursor_y >= NUM_ROWS)
                {
                    terminal[running_terminal].cursor_y -= 1;
                    terminal_scroll_up(processing_terminal);
                }
                terminal[running_terminal].cursor_x = 0;
                terminal_update_cursor(processing_terminal);
            }
            else 
            {
                *(uint8_t *)(video_mem + ((NUM_COLS*terminal[running_terminal].cursor_y + terminal[running_terminal].cursor_x) << 1)) = buf[i];
                *(uint8_t *)(video_mem + ((NUM_COLS*terminal[running_terminal].cursor_y + terminal[running_terminal].cursor_x) << 1) + 1) = ATTRIB;
                *(uint8_t *)((video_mem + (processing_terminal+1)*PAGE_4KB) + ((NUM_COLS*terminal[processing_terminal].cursor_y + terminal[processing_terminal].cursor_x) << 1)) = buf[i];
                *(uint8_t *)((video_mem + (processing_terminal+1)*PAGE_4KB) + ((NUM_COLS*terminal[processing_terminal].cursor_y + terminal[processing_terminal].cursor_x) << 1) + 1) = ATTRIB;

                terminal[running_terminal].cursor_x += 1;
                if(terminal[running_terminal].cursor_x >= NUM_COLS){
                    terminal[running_terminal].cursor_x = 0;
                    terminal[running_terminal].cursor_y += 1;
                    if(terminal[running_terminal].cursor_y >= NUM_ROWS){
                        terminal[running_terminal].cursor_y -= 1;
                        terminal_scroll_up(processing_terminal);
                        }
                    }
                    terminal_update_cursor(processing_terminal);
            }
        }
    }
    else
    {
        for (i = 0; i < nbytes; i++){
            /* revised verison of "putc" */
            if(buf[i] == '\n' || buf[i] == '\r') 
            {
                terminal[processing_terminal].cursor_y += 1;
                if(terminal[processing_terminal].cursor_y >= NUM_ROWS)
                {
                    terminal[processing_terminal].cursor_y -= 1;
                    terminal_scroll_up(processing_terminal);
                }
                terminal[processing_terminal].cursor_x = 0;
                terminal_update_cursor(processing_terminal);
            }
            else 
            {
                *(uint8_t *)((video_mem + (processing_terminal+1)*PAGE_4KB) + ((NUM_COLS*terminal[processing_terminal].cursor_y + terminal[processing_terminal].cursor_x) << 1)) = buf[i];
                *(uint8_t *)((video_mem + (processing_terminal+1)*PAGE_4KB) + ((NUM_COLS*terminal[processing_terminal].cursor_y + terminal[processing_terminal].cursor_x) << 1) + 1) = ATTRIB;

                terminal[processing_terminal].cursor_x += 1;
                if(terminal[processing_terminal].cursor_x >= NUM_COLS){
                    terminal[processing_terminal].cursor_x = 0;
                    terminal[processing_terminal].cursor_y += 1;
                    if(terminal[processing_terminal].cursor_y >= NUM_ROWS){
                        terminal[processing_terminal].cursor_y -= 1;
                        terminal_scroll_up(processing_terminal);
                        }
                    }
                    terminal_update_cursor(processing_terminal);
            }
        }
    }
    sti();
    return i;
}

/*
 * terminal_gogogo
 *   DESCRIPTION: Same as terminal write, but ends with '\0'
 *   INPUTS: buf--buffer to write onto screen
 *   OUTPUTS: none
 *   RETURN VALUE: number of characters written
 *   SIDE EFFECTS: none
 */
uint32_t terminal_gogogo(const uint8_t *buf) {
    int i = 0;
    
    while(buf[i] != '\0') {
        terminal_putc(buf[i]);
        i++;
    }
    
    if(terminal[running_terminal].cursor_y == NUM_ROWS - 1)
    {
        terminal_scroll_up(running_terminal);
        terminal[running_terminal].cursor_y--;
    }
    terminal[running_terminal].cursor_x = 0;
    terminal[running_terminal].cursor_y++;
    
    terminal_update_cursor(running_terminal);
    
    return i;
}

/*
 * terminal_switch
 *   DESCRIPTION: switch to the particular terminal
 *   INPUTS: terminal_id --- the index of the terminal to switch to
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void terminal_switch(int terminal_id){
    if(terminal_id == running_terminal) return;
    // copy video memory buffer
    memcpy((void*)(VIDEO + (running_terminal+1)*PAGE_4KB), (void*) VIDEO, PAGE_4KB);
    change_vid(running_terminal, (VIDEO + (running_terminal + 1)*PAGE_4KB) | 0x7);
    memcpy((void*) VIDEO, (void*)(VIDEO + (terminal_id+1)*PAGE_4KB), PAGE_4KB);  
    change_vid(terminal_id, VIDEO | 0x7);
    // change the global variable
    running_terminal = terminal_id;
    terminal_update_cursor(running_terminal);
    bar_on();
    return;
}

/*
 * bar_on
 *   DESCRIPTION: show the status bar
 *   INPUTS: none
 *   OUTPUTS: output status bar to the screen
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void bar_on()
{
    int32_t i;
    // status bar loop
    for (i = NUM_COLS * NUM_ROWS; i < NUM_COLS * (NUM_ROWS + 1); i++) {
        if (i == SPLIT_ONE || i == SPLIT_TWO) {
            // char " "
            *(uint8_t *)(video_mem + (i << 1)) = ' ';
        }else{
            // char "="
            *(uint8_t *)(video_mem + (i << 1)) = '=';
            if (running_terminal == 0) {
                if (i < SPLIT_ONE) {
                    // two colors: 0x7, 0x8
                    *(uint8_t *)(video_mem + (i << 1) + 1) = 0x7;
                }else{
                    *(uint8_t *)(video_mem + (i << 1) + 1) = 0x8;
                }
            }
            if (running_terminal == 1) {
                if (i < SPLIT_TWO && i > SPLIT_ONE) {
                    *(uint8_t *)(video_mem + (i << 1) + 1) = 0x7;
                }else{
                    *(uint8_t *)(video_mem + (i << 1) + 1) = 0x8;
                }
            }
            if (running_terminal == 2) {
                if (i > SPLIT_TWO) {
                    *(uint8_t *)(video_mem + (i << 1) + 1) = 0x7;
                }else{
                    *(uint8_t *)(video_mem + (i << 1) + 1) = 0x8;
                }
            }
        }
    }
}

