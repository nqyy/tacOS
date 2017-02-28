#define ASM 1

#include "keyboard.h"
#include "rtc.h"
#include "pit.h"

.globl pit_irq
.globl keyboard_irq
.globl rtc_irq
.globl systemcall_wrapper


# pit_irq: assembly wrapper for keyboard handler
pit_irq:
	#cli
	pushl %ebp 					# callee setup
	movl %esp, %ebp
	pushal
	call pit_handler
	popal
	leave
	iret
	#sti

# keyboard_irq: assembly wrapper for keyboard handler
keyboard_irq:
	#cli
	pushl %ebp 					# callee setup
	movl %esp, %ebp
	pushal
	call keyboard_handler
	popal
	leave
	iret
	#sti

# rtc_irq: assembly wrapper for RTC handler
rtc_irq:
	#cli
	pushl %ebp 					# callee setup
	movl %esp, %ebp
	pushal
	call rtc_handler
	popal
	leave
	iret
	#sti

#systemcall_wrapper: assembly wrapper for system call
systemcall_wrapper:
	cli
	pushl %ebp				#callee setup
	movl %esp, %ebp
	pushl %ebx
	pushl %esi
	pushl %edi					#save all registers				#save all flags

	cmpl $1, %eax			#system call value checking
	jl INVALID_ARG
	cmpl $10, %eax
	jg INVALID_ARG

	#caller preparation
	#push caller saved registers
	pushl %edx
	pushl %ecx
	#push arguments
	pushl %edx
	pushl %ecx
	pushl %ebx
	subl $1, %eax			#index is actually 0-9 instead of 1-10
	sti
	call *systemcall_table(,%eax,4)
	addl $12, %esp
	popl %ecx
	popl %edx
	jmp DONE

INVALID_ARG:
	xorl %eax, %eax
	subl $1, %eax

DONE:
	popl %edi
	popl %esi
	popl %ebx
	leave
	iret

systemcall_table:
	.long halt, execute, read, write, open, close, getargs, vidmap, set_handler, sigreturn


