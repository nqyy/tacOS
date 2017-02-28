/* syscall.c - functions related to system calls
 */

#include "syscall.h"
#include "paging.h"
#include "filesystem.h"
#include "x86_desc.h"
#include "lib.h"
#include "terminal.h"

// File Operations Definitions
fops_t stdin_func = {(read_t)terminal_read, NULL, NULL, NULL};
fops_t stdout_func = {NULL, (write_t)terminal_write, NULL, NULL};
fops_t rtc_func = {(read_t)rtc_read, (write_t)rtc_write, (open_t)rtc_open, (close_t)rtc_close};
fops_t file_func = {(read_t)file_read, (write_t)file_write, (open_t)file_open, (close_t)file_close};
fops_t dir_func = {(read_t)dir_read, (write_t)dir_write, (open_t)dir_open, (close_t)dir_close};

// Magic Numbers
#define USER_ESP 		0x083FFFFC			// 128 MB + 4 MB - 4
#define USER_MEM_START	0x0800000			// 8 MB
#define KERNEL_MEM_END	0x0800000			// 8 MB
#define PAGE_SIZE 		0x400000			// 4 MB
#define KRNL_STACK_SIZE 0x2000				// 8 KB
#define MAGIC0			0x7F
#define MAGIC1			0x45
#define MAGIC2 			0x4C
#define MAGIC3 			0x46
#define VIRTUAL_ADDR	0x08048000
#define ATTR			0x87
#define SHIFT_4MB 		22
#define IF_FLAG 		0x200
#define FILENAME_MAX 	32
#define MASK 			0xFF
#define MAX_PCB 		6
#define NUM_FILES	 	8
#define RTC_TYPE		0
#define DIR_TYPE		1
#define FILE_TYPE 		2
#define USER_BEGIN		0x8000000
#define	USER_VID		0xFFC00000
// #define PAGE_4KB        0x1000

void syscall_init() {
	int i;

	for(i = 0; i < 6; i++) {
		pcb_status[i] = 0;
	}
}

/*
 * halt
 *   DESCRIPTION: Terminates a process, returning the specified value to its parent
 *				  process, extending 8 bits to 32. Restores PCB to parent PCB, restores
 *                TSS, restores paging, and updates ESP and EBP values
 *   INPUTS: status--return value to parent process
 *   OUTPUTS: none
 *   RETURN VALUE: never returns
 *   SIDE EFFECTS: jumps to label in execute and returns value in EAX
 */
int32_t halt(uint8_t status) {
    //fetch current PCB
    pcb_t *current = get_pcb(terminal[processing_terminal].cur_pid);

    //find parent PCB
    uint32_t parent = current->parent;
    uint32_t esp, ebp;
    esp = current->esp;
    ebp = current->ebp;

    //destroy child PCB
    end_process(current->pid);

    terminal[processing_terminal].cur_pid = parent;
    terminal[processing_terminal].num_process--;

    if(parent==-1) execute((uint8_t*)"shell");

    pcb_t* parent_pcb = get_pcb(parent);

    //restore parents paging and flush TLB
    set_pde(VIRTUAL_ADDR >> SHIFT_4MB, parent_pcb->pd_entry);

    //restore parents data
    tss.esp0=parent_pcb->esp0;
    tss.ss0=parent_pcb->ss0;

    //jump to label halt_ret
    asm volatile("movl %0, %%esp\n\t"
				"movl %1, %%ebp\n\t"
				"movl %2, %%eax\n\t"
				"jmp halt_ret"
				:
				:"r"(esp), "r"(ebp), "r"((uint32_t)status & MASK)
				);

    // //jump to label halt_ret
    // asm volatile("movl %0, %%esp"::"r"(esp));
    // asm volatile("movl %0, %%ebp"::"r"(ebp));

    // //deal with return value
    // asm volatile("movl %0, %%eax"::"r"((uint32_t)status & MASK));
    // asm volatile("jmp halt_ret");

    return 0;
}

/*
 * execute
 *   DESCRIPTION: Attempts to load and execute a new program, handing off
 *                the processor to the new program until it terminates. First parses
 *                arguments, then checks the file for executable magic numbers,
 *                then creates the PCB for the current process, sets up TSS, sets up
 *                paging, loads program into memory and sets up stack for IRET
 *   INPUTS: command--space-separated sequence of words, first word is the file
 *           name of the program to be executed and the rest, stripped of leading
 *           spaces are the arguments
 *   OUTPUTS: none
 *   RETURN VALUE: -1 if command cannot be executed, 256 if program dies by exception,
 *                 or a value in the range 0 to 255 if the program executes a halt
 *                 system call (value passed from halt)
 *   SIDE EFFECTS: Goes to priority level 3 using IRET to run user code
 */
int32_t execute(const uint8_t* command) {
	uint8_t filename[FILENAME_MAX];
	int8_t args[95];				// 95 = KEYBOARD_BUFFER_LENGTH (128) - FILENAME_MAX (32) - 1 (SPACE)
	uint8_t buf[4];             	// the four magic numbers
	int i, k;
	k = 0;
	// Parse arguments
	if(command[k] == '\0' || command[k] == '\n' || command[k] == '\r') return -1;
	while(command[k] == ' ') {
		// strip leading spaces
		k++;
		if(command[k] == '\0' || command[k] == '\n' || command[k] == '\r') return -1;
	}
	i = 0;
	while(command[k] != '\0' && command[k] != '\n' && command[k] != '\r' && command[k] != ' ') {
		// executable name
		if(i == FILENAME_MAX) return -1;
		filename[i] = command[k];
		k++;
		i++;
	}
	if(i < FILENAME_MAX-1) filename[i] = '\0';
	while(command[k] == ' ') {
		// strip spaces
		k++;
	}
	i = 0;
	while(command[k] != '\0' && command[k] != '\n' && command[k] != '\r' && k < 128) {
		// get args
		args[i] = command[k];
		i++;
		k++;
	}
    args[i] = '\0';
	// Check File Type
	dentry_t dentry;
	if(read_dentry_by_name (filename, &dentry) == -1) return -1;
	if(read_data(dentry.inode, 0, buf, 4) != 4) return -1;
	if(!(buf[0] == MAGIC0 && buf[1] == MAGIC1 && buf[2] == MAGIC2 && buf[3] == MAGIC3)) {
		// Not executable file, fail
		return -1;
	}

	// Create PCB
	uint32_t pid = -1;
	for (i = 0; i < MAX_PCB; i++) {
		if (pcb_status[i] == 0) {
			pid = i;
			break;
		}
	}
	//return -1 upon failure of allocating space for pcb
	if (pid == -1) return -1;
	//indicate that this pid has existed
	pcb_status[pid] = 1;
	//specify address for new pid
	pcb_t *cur_pcb = (pcb_t *) (KERNEL_MEM_END - KRNL_STACK_SIZE * (pid + 1));
	cur_pcb->pid = pid;
	terminal[processing_terminal].num_process++;
	//save parent pid number
	if(terminal[processing_terminal].num_process == 1) cur_pcb->parent = -1;
	else cur_pcb->parent = terminal[processing_terminal].cur_pid;
	terminal[processing_terminal].cur_pid = pid;
    // set the arguments
    i = 0;
    while (args[i] != '\0') {
        cur_pcb -> arg[i] = args[i];
        i++;
    }
    cur_pcb -> arg[i] = '\0';
	// Set up TSS
	tss.esp0 = KERNEL_MEM_END - pid*KRNL_STACK_SIZE - 4;
	tss.ss0 = KERNEL_DS;
	//store the parent's esp0 and ss0
	cur_pcb->esp0 = tss.esp0;
	cur_pcb->ss0 = tss.ss0;
	strcpy(args, cur_pcb->arg);

	// Loader: set up paging
	uint32_t virt_addr = VIRTUAL_ADDR;
	uint32_t phys_addr = pid*PAGE_SIZE + USER_MEM_START;
	set_pde(virt_addr >> SHIFT_4MB, phys_addr | ATTR);

	// Continue PCB
	cur_pcb->pd_entry = phys_addr | ATTR;

	//initialization for stdin
	cur_pcb->f_array[0].fops = stdin_func;
	cur_pcb->f_array[0].inode = -1;
	cur_pcb->f_array[0].fpos = 0;
	cur_pcb->f_array[0].flags = 1;
	//initializaiton for stdout
	cur_pcb->f_array[1].fops = stdout_func;
	cur_pcb->f_array[1].inode = -1;
	cur_pcb->f_array[1].fpos = 0;
	cur_pcb->f_array[1].flags = 1;
    
	for(i = 2; i < NUM_FILES; i++) {
		cur_pcb->f_array[i].fops.read_func = NULL;
		cur_pcb->f_array[i].fops.write_func = NULL;
		cur_pcb->f_array[i].fops.open_func = NULL;
		cur_pcb->f_array[i].fops.close_func = NULL;
		cur_pcb->f_array[i].inode = -1;
		cur_pcb->f_array[i].fpos = 0;
		cur_pcb->f_array[i].flags = 0;
	}

	// Copy into memory
	inode_t* inodefind = inode_find(dentry);
	read_data(dentry.inode, 0, (uint8_t*)virt_addr, inodefind->length);
    
    uint8_t *tmp = (uint8_t *)virt_addr;
	// Entry point is 24, 25, 26, 27th bytes of the file, with bit shift 0 8 16 24
	uint32_t entry_point = tmp[24] | (tmp[25] << 8) | (tmp[26] << 16) | (tmp[27] << 24);
	// Equivalent: uint32_t entry_point = ((uint32_t*)tmp)[6];
	// Equivalent: uint32_t entry_point = *(((uint32_t *)tmp) + 6);

	// Push IRET context to stack
	// uint32_t eflags_reg;
	asm volatile("movl %0, %%eax":: "g"(USER_DS));		// Update DS register
	asm volatile("movw %ax, %ds");
	asm volatile("pushl %0":: "g"(USER_DS));			// SS
	asm volatile("pushl %0":: "g"(USER_ESP));			// ESP
	asm volatile("pushfl");								// EFLAGS
	asm volatile("pushl %0":: "g"(USER_CS));			// CS
	asm volatile("pushl %0":: "g"(entry_point));		// EIP

	asm volatile("movl %%esp, %0": "=r"(cur_pcb->esp));
	asm volatile("movl %%ebp, %0": "=r"(cur_pcb->ebp));

	// IRET
  	asm volatile("iret");
  	asm volatile("halt_ret:");

  	// Halt return value
  	int ret_value;
  	asm volatile("movl %%eax, %0": "=r"(ret_value));

  	return ret_value;
}

/*
 * read
 *   DESCRIPTION: Reads from file, uses a jump table
 *   INPUTS: fd--file descriptor (which file should be read)
 *           buf--buffer of where to copy the file to
 *           nbytes--number of bytes to read
 *   OUTPUTS: none
 *   RETURN VALUE: Number of bytes read, -1 on failure
 *   SIDE EFFECTS: none
 */
int32_t read(int32_t fd, void* buf, int32_t nbytes) {
    //get the current pcb
	pcb_t* curr_pcb = get_pcb(terminal[processing_terminal].cur_pid);
    // the error condition of fd
    if ((fd == 0) || (fd > 1 && fd < NUM_FILES && curr_pcb-> f_array[fd].flags != 0)) {
        // call the read function
        return curr_pcb -> f_array[fd].fops.read_func(fd, buf, nbytes);
    }
    return -1;
}

/*
 * write
 *   DESCRIPTION: Writes data to the file
 *   INPUTS: fd--file descriptor (which file should be written to)
 *           buf--buffer of where to copy the file to
 *           nbytes--number of bytes to write
 *   OUTPUTS: none
 *   RETURN VALUE: Number of bytes written, -1 on failure
 *   SIDE EFFECTS: none
 */
int32_t write(int32_t fd, const void* buf, int32_t nbytes) {
    // get the current pcb
	pcb_t* curr_pcb = get_pcb(terminal[processing_terminal].cur_pid);
    // the error condition of fd
    if (fd > 0 && fd < NUM_FILES && curr_pcb->f_array[fd].flags != 0){
        // call the write function
        return curr_pcb -> f_array[fd].fops.write_func(fd, buf, nbytes);
    }
    return -1;
}

/*
 * open
 *   DESCRIPTION: Opens a file (puts in file array if necessary)
 *   INPUTS: filename--name of the file to be opened
 *   OUTPUTS: none
 *   RETURN VALUE: file descriptor--the number it is stored on the
 *                 file array, -1 on failure
 *   SIDE EFFECTS: none
 */
int32_t open(const uint8_t* filename) {
    dentry_t fileopen;
    int i = 0;
    //get current pcb
	pcb_t* curr_pcb = get_pcb(terminal[processing_terminal].cur_pid);
    //fail condition
    if (read_dentry_by_name(filename, &fileopen) == -1){
        return -1;
    }
    // find the next available entry in fd
    for (i = 0; i < NUM_FILES; i++)
    {
        if (curr_pcb->f_array[i].flags == 0){
            break;
        }
    }
    //all full
    if(i == NUM_FILES) return -1;
    // wrong type
    if (fileopen.file_type < 0 || fileopen.file_type > 2) {
        return -1;
    }
    curr_pcb->f_array[i].flags = 1;
    curr_pcb->f_array[i].fpos = 0;
    // three type and set different function
    if (fileopen.file_type == RTC_TYPE)
    {
    	curr_pcb -> f_array[i].fops = rtc_func;
    	curr_pcb->f_array[i].inode = 0;
    }
    if (fileopen.file_type == DIR_TYPE)
    {
    	curr_pcb -> f_array[i].fops = dir_func;
    	curr_pcb->f_array[i].inode = 0;
    }
    if (fileopen.file_type == FILE_TYPE)
    {
    	curr_pcb -> f_array[i].fops = file_func;
    	curr_pcb->f_array[i].inode = fileopen.inode;
    }
    //open the function
    curr_pcb -> f_array[i].fops.open_func(filename);
    return i;
}

/*
 * close
 *   DESCRIPTION: Closes the file
 *   INPUTS: fd--file descriptor (which file should be closed)
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: none
 */
int32_t close(int32_t fd) {
    //get current pcb
	pcb_t* curr_pcb = get_pcb(terminal[processing_terminal].cur_pid);
    // error condition
    if (fd <= 1 || fd > 7) {
        return -1;
    }
    if (curr_pcb -> f_array[fd].flags == 0) {
        return -1;
    }
    // close the file
    curr_pcb -> f_array[fd].fops.close_func(fd);
    curr_pcb -> f_array[fd].flags = 0;
    return 0;
}

/*
 * getargs
 *   DESCRIPTION: reads the program's command line arguments into
 *                a user-level buffer
 *   INPUTS: buf--buffer to copy arguments into
 *           nbytes--maximum number of bytes
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: none
 */
int32_t getargs(uint8_t* buf, int32_t nbytes) {
    //error condition
    if (buf == NULL) {
        return -1;
    }
    int i;
    pcb_t* cur_pcb = get_pcb(terminal[processing_terminal].cur_pid);
    //copy the argument to the buffer
    if (strlen((int8_t*)cur_pcb -> arg) <= nbytes) {
        strcpy((int8_t*)buf, (int8_t*)cur_pcb -> arg);
    }else{
        for (i = 0; i < nbytes; i++) {
            buf[i] = cur_pcb->arg[i];
        }
    }
    
    return 0;
}

/*
 * vidmap
 *   DESCRIPTION: maps the text-mode video memory into user
 *                space at a pre-set virtual address (0XFFC00000)
 *   INPUTS: none
 *   OUTPUTS: screen_start--where the screens starts
 *   RETURN VALUE: 0 on success, always succeeds
 *   SIDE EFFECTS: none
 */

int32_t vidmap(uint8_t** screen_start) {
    if(screen_start < (uint8_t **)USER_BEGIN || screen_start >= (uint8_t **)(USER_BEGIN + PAGE_SIZE)) return -1;
    *screen_start = (uint8_t *)(USER_VID + processing_terminal*PAGE_4KB);
    return 0;
}

/*
 * set_handler
 *   DESCRIPTION: not implemented yet
 *   INPUTS: NA
 *   OUTPUTS: NA
 *   RETURN VALUE: NA
 *   SIDE EFFECTS: NA
 */
int32_t set_handler(int32_t signum, void* handler_address) {
	return -1;
}

/*
 * sigreturn
 *   DESCRIPTION: not implemented yet
 *   INPUTS: NA
 *   OUTPUTS: NA
 *   RETURN VALUE: NA
 *   SIDE EFFECTS: NA
 */
int32_t sigreturn(void){
	return -1;
}

/*
 * close
 *   DESCRIPTION: Deletes an existing process
 *   INPUTS: pid--process ID of the process to close
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: deletes the PCB associate with the PID
 */
int32_t end_process(uint32_t pid) {
	if(pid >= 6) return -1;


	pcb_status[pid] = 0;
	pcb_t* cur_pcb = get_pcb(pid);

	//close all the files if they could be closed
	int i;
	for(i = 0; i < NUM_FILES; i++) {
		if(cur_pcb->f_array[i].flags == 1 && cur_pcb->f_array[i].fops.close_func != NULL)
			cur_pcb->f_array[i].fops.close_func(i);
	}
	//clear all the initialization information
	cur_pcb->pid = -1;

	for(i = 0; i < NUM_FILES; i++) {
		if(cur_pcb->f_array[i].flags == 1) {
			cur_pcb->f_array[i].fops.read_func = NULL;
			cur_pcb->f_array[i].fops.write_func = NULL;
			cur_pcb->f_array[i].fops.open_func = NULL;
			cur_pcb->f_array[i].fops.close_func = NULL;

			cur_pcb->f_array[i].inode = 0;
			cur_pcb->f_array[i].fpos = 0;
			cur_pcb->f_array[i].flags = 0;
		}
	}


	cur_pcb->parent = NULL;
	cur_pcb->esp0 = 0;
	cur_pcb->esp = 0;
	cur_pcb->ebp = 0;
	cur_pcb->ss0 = 0;
	//cur_pcb->arg = {0};

	return 0;
}

/*
 * get_pcb
 *   DESCRIPTION: Gets the PCB related to teh PID
 *   INPUTS: pid--process ID
 *   OUTPUTS: none
 *   RETURN VALUE: PCB associated with the PID
 *   SIDE EFFECTS: none
 */
pcb_t* get_pcb(uint32_t pid){
	return (pcb_t*)(KERNEL_MEM_END - KRNL_STACK_SIZE * (pid + 1)) ;
}
