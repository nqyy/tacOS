/* syscall.h - system call header file
 */

#ifndef _SYSCALL_H
#define _SYSCALL_H

#include "keyboard.h"
#include "rtc.h"
#include "types.h"
#include "i8259.h"
#include "lib.h"
#include "terminal.h"

// Global Variables
uint32_t pcb_status[6];
//uint32_t cur_pid;

void syscall_init(void);
/* Halt current program */
int32_t halt(uint8_t status);
/* Execute a program */
int32_t execute(const uint8_t* command);
/* Read a file */
int32_t read(int32_t fd, void* buf, int32_t nbytes);
/* Write to a file */
int32_t write(int32_t fd, const void* buf, int32_t nbytes);
/* Open a file */
int32_t open(const uint8_t* filename);
/* Close a file */
int32_t close(int32_t fd);
/* Get the arguments */
int32_t getargs(uint8_t* buf, int32_t nbytes);
/* Set video map */
int32_t vidmap(uint8_t** screen_start);
/* Set handler */
int32_t set_handler(int32_t signum, void* handler_address);
/* Signal return */
int32_t sigreturn(void);

// Function Pointer Definitions
typedef int32_t (*read_t) (int32_t fd, void* buf, int32_t nbytes);
typedef int32_t (*write_t) (int32_t fd, const void* buf, int32_t nbytes);
typedef int32_t (*open_t) (const uint8_t* filename);
typedef int32_t (*close_t) (int32_t fd);

// FOps Struct
typedef struct fops_t {
	read_t read_func;
	write_t write_func;
	open_t open_func;
	close_t close_func;
}fops_t;

// FD Struct
typedef struct fd_t{
	fops_t fops;
	uint32_t inode;
	uint32_t fpos;
	uint32_t flags;
}fd_t;

// PCB truct
typedef struct pcb_t {
	uint32_t pid;
	fd_t f_array[8]; 	//each task can have up to 8 open files. see doc7.2
	uint32_t parent;		//used in halt. see doc5.3.5
	uint32_t esp0;
	uint32_t esp;
	uint32_t ebp;
	uint16_t ss0;
	int8_t arg[128];
	uint32_t pd_entry;
}pcb_t;

/* Close PCB */
int32_t end_process(uint32_t pid);
/* Helper Function--gets pcb given pid */
pcb_t* get_pcb(uint32_t pid);

#endif

