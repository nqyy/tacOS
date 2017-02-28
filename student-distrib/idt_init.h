/* idt_init.h - Functions related to the initialization of
 * the IDT
 */

#ifndef _IDT_H
#define _IDT_H

#include "types.h"

/* Set up the IDT table with exceptions and interrupts */
void setup_idt();

/* 19 Exceptions */
int exception0();
int exception1();
int exception2();
int exception3();
int exception4();
int exception5();
int exception6();
int exception7();
int exception8();
int exception9();
int exception10();
int exception11();
int exception12();
int exception13();
int exception14();
int exception16();
int exception17();
int exception18();
int exception19();

#endif

