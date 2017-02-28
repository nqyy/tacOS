/* scheduler.c - functions related to Scheduler in C
 */

#include "scheduler.h"
#include "terminal.h"

#define VIRTUAL_ADDR	0x08048000
#define SHIFT_4MB 		22

/*
 * sched
 *   DESCRIPTION: sched is run every time the pit is fired: first if no process
 *                is running on that particular terminal, execute shell. Then,
 *                save the esp and ebp of the currently running process. Then, 
 *                change the processing terminal into the next terminal and 
 *                update to the esp, ebp, esp0 in TSS, as well as paging of the
 *                next terminal
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: prepares the stack for the next program in the sched queue
 */
void sched()
{
	cli();
	next_terminal = (processing_terminal+1)%3;
	// save esp and ebp
    asm volatile("movl %%esp, %0\n\t"
				"movl %%ebp, %1\n\t"
				:"=r"(terminal[processing_terminal].esp), "=r"(terminal[processing_terminal].ebp)
				);
	//open a new shell for each terminal
	if(terminal[next_terminal].num_process == 0)
	{
		processing_terminal = next_terminal;
		//terminal[processing_terminal].num_process++;
		sti();
		execute((uint8_t*)"shell");
		return;
	}

	processing_terminal = next_terminal;

	pcb_t* cur_pcb = get_pcb(terminal[processing_terminal].cur_pid);

	// change TSS esp0
	tss.esp0 = cur_pcb->esp0;
	// change PD
	uint32_t virt_addr = VIRTUAL_ADDR;
	uint32_t phys_addr = cur_pcb->pd_entry;
	set_pde(virt_addr >> SHIFT_4MB, phys_addr);
	// change esp and ebp
	asm volatile("movl %0, %%esp\n\t"
				"movl %1, %%ebp\n\t"
				:
				:"g"(terminal[processing_terminal].esp), "g"(terminal[processing_terminal].ebp)
				);
	sti();
}
