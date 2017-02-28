/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */
//reference from http://lxr.free-electrons.com/source/arch/x86/kernel/i8259.c
#include "i8259.h"
#include "lib.h"

// Magic Numbers
#define MASK_INIT           0XFF
#define MAX_PIC             8
#define SLAVE_PORT          2

/* Interrupt masks to determine which interrupts
 * are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7 */
uint8_t slave_mask; /* IRQs 8-15 */

/*
 * i8259_init
 *   DESCRIPTION: initializes the 8259 PIC & send all ICW
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: PICs are initialized
 */
void
i8259_init(void)
{
    /* Initialize the 8259 PIC */
    //int i = 0;
    master_mask = MASK_INIT;
    slave_mask = MASK_INIT;
    outb(master_mask, MASTER_8259_PORT_DAT); // mask all master IMR
    outb(slave_mask, SLAVE_8259_PORT_DAT);	 // mask all slave IMR
    
    outb(ICW1, MASTER_8259_PORT_CMD);		 // ICW1 to master cmd
    outb(ICW2_MASTER, MASTER_8259_PORT_DAT); // ICW2 to master data
    outb(ICW3_MASTER, MASTER_8259_PORT_DAT); // ICW3 to master data
    outb(ICW4, MASTER_8259_PORT_DAT);		 // ICW4 to master data
    outb(ICW1, SLAVE_8259_PORT_CMD);		 // ICW1 to slave cmd
    outb(ICW2_SLAVE, SLAVE_8259_PORT_DAT);	 // ICW2 to slave data
    outb(ICW3_SLAVE, SLAVE_8259_PORT_DAT);   // ICW3 to slave data
    outb(ICW4, SLAVE_8259_PORT_DAT);		 // ICW4 to slave data
    //wait for initialize
    //whatever the number appears on the right will be an arbitray time for initialization
    // while (i < 1000) {
    //     i++;
    // }
}

/*
 * enable_irq
 *   DESCRIPTION: enable (unmask) the specific IRQ
 *   INPUTS: irq_num--the enabled irq number
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void
enable_irq(uint32_t irq_num)
{
    /* Enable (unmask) the specified IRQ */
    //printf("enable irq\n");
    uint8_t mask;
    if (irq_num & MAX_PIC)
    {
        //slave PIC:
        mask = ~(1 << (irq_num - MAX_PIC));
        slave_mask &= mask;
        // write the mask to slave port data
        outb(slave_mask, SLAVE_8259_PORT_DAT);
        mask = ~(1 << SLAVE_PORT);
        master_mask &= mask;
        // write the mask to the master port data
        outb(master_mask, MASTER_8259_PORT_DAT);
    }else{
        //master PIC:
        mask = ~(1 << irq_num);
        master_mask &= mask;
        outb(master_mask, MASTER_8259_PORT_DAT);
    }
}

/*
 * disable_irq
 *   DESCRIPTION: disable (mask) the specific IRQ.
 *   INPUTS: irq_num--the disabled irq number
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void
disable_irq(uint32_t irq_num)
{
    /* Disable (mask) the specified IRQ */
    uint8_t mask;
    
    if (irq_num & MAX_PIC)
    {
        //slave PIC:
        mask = (1 << (irq_num & MAX_PIC));
        slave_mask |= mask;
        // write the mask to slave port data
        outb(slave_mask, SLAVE_8259_PORT_DAT);
    } else
    {
        mask = (1 << irq_num);
        master_mask |= mask;
        // write the mask to the master port data
        outb(master_mask, MASTER_8259_PORT_DAT);
    }
}

/*
 * send eoi
 *   DESCRIPTION: sends EOI of the specified IRQ end of interrupt
 *   INPUTS: irq_num--the finished irq number
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void
send_eoi(uint32_t irq_num)
{
    /* Send end-of-interrupt signal of the specified IRQ */
    if (irq_num < MAX_PIC)
    {
        //send master
        outb(EOI + irq_num, MASTER_8259_PORT_CMD);
    }else{
        //send slave
        outb(EOI + irq_num - MAX_PIC, SLAVE_8259_PORT_CMD);
        outb(EOI + SLAVE_PORT, MASTER_8259_PORT_CMD);
    }
}


