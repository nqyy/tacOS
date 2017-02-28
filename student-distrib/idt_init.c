#include "x86_desc.h"
#include "idt_init.h"
#include "lib.h"
#include "keyboard.h"
#include "rtc.h"
#include "handler_wrappers.h"

#define EXP_END      0x1F
#define INT_START    0x20
#define IDT_END      0xFF
#define INT_PIT      0x20
#define INT_KBD      0x21
#define INT_RTC      0x28

/*
 * Setup IDT
 *   DESCRIPTION: Initializes the IDT table by setting the
 *                IDT entries to point to the corresponding
 *                handlers and intitalizes the interrupt
 *                gates with the right values
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Initializes the IDT with exceptions 1 ~
 *                 19 and keyboard interrupt (0x21) as well
 *                 as real time clock (0x28)
 */
void setup_idt(){
    //idt[256] idt entries
    int i;
    // 0xff: 256 entries for IDT
    for (i = 0; i < IDT_END; i++) {
        //set up all the idt with appropriate value from the INTEL manual
        idt[i].seg_selector = KERNEL_CS;
        //exception from 0 to 0x1f
        if (i <= EXP_END) {
            idt[i].reserved4 = 0;
            idt[i].reserved3 = 1;
            idt[i].reserved2 = 1;
            idt[i].reserved1 = 1;
            idt[i].size = 1;
            idt[i].reserved0 = 0;
            idt[i].dpl = 0;
            idt[i].present = 1;
            // all the exception from #1 to #19
            if (i == 0) {
                SET_IDT_ENTRY(idt[i], exception0);
            }
            if (i == 1) {
                SET_IDT_ENTRY(idt[i], exception1);
            }
            if (i == 2) {
                SET_IDT_ENTRY(idt[i], exception2);
            }
            if (i == 3) {
                SET_IDT_ENTRY(idt[i], exception3);
            }
            if (i == 4) {
                SET_IDT_ENTRY(idt[i], exception4);
            }
            if (i == 5) {
                SET_IDT_ENTRY(idt[i], exception5);
            }
            if (i == 6) {
                SET_IDT_ENTRY(idt[i], exception6);
            }
            if (i == 7) {
                SET_IDT_ENTRY(idt[i], exception7);
            }
            if (i == 8) {
                SET_IDT_ENTRY(idt[i], exception8);
            }
            if (i == 9) {
                SET_IDT_ENTRY(idt[i], exception9);
            }
            if (i == 10) {
                SET_IDT_ENTRY(idt[i], exception10);
            }
            if (i == 11) {
                SET_IDT_ENTRY(idt[i], exception11);
            }
            if (i == 12) {
                SET_IDT_ENTRY(idt[i], exception12);
            }
            if (i == 13) {
                SET_IDT_ENTRY(idt[i], exception13);
            }
            if (i == 14) {
                SET_IDT_ENTRY(idt[i], exception14);
            }
            if (i == 16) {
                SET_IDT_ENTRY(idt[i], exception16);
            }
            if (i == 17) {
                SET_IDT_ENTRY(idt[i], exception17);
            }
            if (i == 18) {
                SET_IDT_ENTRY(idt[i], exception18);
            }
            if (i == 19) {
                SET_IDT_ENTRY(idt[i], exception19);
            }
        }
        //interrupt from 0x20 to 0x2f
        if (i >= INT_START && i <= IDT_END) {
            idt[i].reserved4 = 0;
            idt[i].reserved3 = 0;
            idt[i].reserved2 = 1;
            idt[i].reserved1 = 1;
            idt[i].size = 1;
            idt[i].reserved0 = 0;
            idt[i].dpl = 0;
            idt[i].present = 1;
            if (i == INT_PIT) {
                SET_IDT_ENTRY(idt[i], pit_irq);
            }
            if (i == INT_KBD) {
                SET_IDT_ENTRY(idt[i], keyboard_irq);
            }
            if (i == INT_RTC)
            {
                SET_IDT_ENTRY(idt[i], rtc_irq);
            }
        }
        //system call 0x80
        if (i == 0x80) {
            idt[i].reserved4 = 0;
            idt[i].reserved3 = 1;           // for trap gate
            idt[i].reserved2 = 1;
            idt[i].reserved1 = 1;
            idt[i].size = 1;
            idt[i].reserved0 = 0;
            idt[i].dpl = 3;
            idt[i].present = 1;
            SET_IDT_ENTRY(idt[i],systemcall_wrapper);
        }
        
    }
}


/*
 * Exception 0 ~ Exception 19
 *   DESCRIPTION: the each exception should be printed out
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: print the exception out on the console
 */

int exception0(){
    cli();
    printf("EXCEPTION: Divide Error\n");
    while(1);
    sti();
}
int exception1(){
    cli();
    printf("EXCEPTION: RESERVED\n");
    while(1);
    sti();
}
int exception2(){
    cli();
    printf("EXCEPTION: NMI Interrupt\n");
    while(1);
    sti();
}
int exception3(){
    cli();
    printf("EXCEPTION: Breakpoint\n");
    while(1);
    sti();
}
int exception4(){
    cli();
    printf("EXCEPTION: Overflow\n");
    while(1);
    sti();
}
int exception5(){
    cli();
    printf("EXCEPTION: BOUND Range Exceeded\n");
    while(1);
    sti();
}
int exception6(){
    cli();
    printf("EXCEPTION: Invalid Opcode\n");
    while(1);
    sti();
}
int exception7(){
    cli();
    printf("EXCEPTION: Device Not Available\n");
    while(1);
    sti();
}
int exception8(){
    cli();
    printf("EXCEPTION: Double Fault\n");
    while(1);
    sti();
}
int exception9(){
    cli();
    printf("EXCEPTION: Coprocessor Segment Overrun\n");
    while(1);
    sti();
}
int exception10(){
    cli();
    printf("EXCEPTION: Invalid TSS\n");
    while(1);
    sti();
}
int exception11(){
    cli();
    printf("EXCEPTION: Segment Not Present\n");
    while(1);
    sti();
}
int exception12(){
    cli();
    printf("EXCEPTION: Stack-Segment Fault\n");
    while(1);
    sti();
}
int exception13(){
    cli();
    printf("EXCEPTION: General Protection\n");
    while(1);
    sti();
}
int exception14(){
    cli();
    int fault;
    asm volatile("movl %%cr2, %0" \
                 :  \
                 :"r"(fault)    \
                 :"memory");
    
    printf("EXCEPTION: Page Fault at 0x%x", fault);
    while(1);
    sti();
}
int exception16(){
    cli();
    printf("EXCEPTION: x87 FPU Floating-Point Error\n");
    while(1);
    sti();
}
int exception17(){
    cli();
    printf("EXCEPTION: Alignment Check\n");
    while(1);
    sti();
}
int exception18(){
    cli();
    printf("EXCEPTION: Machine Check");
    while(1);
    sti();
}
int exception19(){
    cli();
    printf("EXCEPTION: SIMD Floating-Point Exception\n");
    while(1);
    sti();
}
