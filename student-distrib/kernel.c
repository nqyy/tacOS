/* kernel.c - the C part of the kernel
 * vim:ts=4 noexpandtab
 */

#include "multiboot.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "debug.h"
#include "idt_init.h"
#include "paging.h"
#include "keyboard.h"
#include "rtc.h"
#include "filesystem.h"
#include "terminal.h"
#include "syscall.h"
#include "pit.h"

/* Macros. */
/* Check if the bit BIT in FLAGS is set. */
#define CHECK_FLAG(flags,bit)   ((flags) & (1 << (bit)))
#define FILE_BUF_SIZE 40000
#define OUTPUT_FILE_SIZE 1000
#define TEN_DECIMAL 10
#define TWELVE 12

/* Check if MAGIC is valid and print the Multiboot information structure
   pointed by ADDR. */
void
entry (unsigned long magic, unsigned long addr)
{
	multiboot_info_t *mbi;

	/* Clear the screen. */
	clear();
    bar_on();
	/* Am I booted by a Multiboot-compliant boot loader? */
	if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
	{
		printf ("Invalid magic number: 0x%#x\n", (unsigned) magic);
		return;
	}

	/* Set MBI to the address of the Multiboot information structure. */
	mbi = (multiboot_info_t *) addr;

	/* Print out the flags. */
	printf ("flags = 0x%#x\n", (unsigned) mbi->flags);

	/* Are mem_* valid? */
	if (CHECK_FLAG (mbi->flags, 0))
		printf ("mem_lower = %uKB, mem_upper = %uKB\n",
				(unsigned) mbi->mem_lower, (unsigned) mbi->mem_upper);

	/* Is boot_device valid? */
	if (CHECK_FLAG (mbi->flags, 1))
		printf ("boot_device = 0x%#x\n", (unsigned) mbi->boot_device);

	/* Is the command line passed? */
	if (CHECK_FLAG (mbi->flags, 2))
		printf ("cmdline = %s\n", (char *) mbi->cmdline);

	uint32_t fs_addr;
	if (CHECK_FLAG (mbi->flags, 3)) {
		int mod_count = 0;
		int i;
		module_t* mod = (module_t*)mbi->mods_addr;
		while(mod_count < mbi->mods_count) {
			printf("Module %d loaded at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_start);
			fs_addr = mod->mod_start;
			printf("Module %d ends at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_end);
			printf("First few bytes of module:\n");
			for(i = 0; i<16; i++) {
				printf("0x%x ", *((char*)(mod->mod_start+i)));
			}
			printf("\n");
			mod_count++;
			mod++;
		}
	}
	/* Bits 4 and 5 are mutually exclusive! */
	if (CHECK_FLAG (mbi->flags, 4) && CHECK_FLAG (mbi->flags, 5))
	{
		printf ("Both bits 4 and 5 are set.\n");
		return;
	}

	/* Is the section header table of ELF valid? */
	if (CHECK_FLAG (mbi->flags, 5))
	{
		elf_section_header_table_t *elf_sec = &(mbi->elf_sec);

		printf ("elf_sec: num = %u, size = 0x%#x,"
				" addr = 0x%#x, shndx = 0x%#x\n",
				(unsigned) elf_sec->num, (unsigned) elf_sec->size,
				(unsigned) elf_sec->addr, (unsigned) elf_sec->shndx);
	}

	/* Are mmap_* valid? */
	if (CHECK_FLAG (mbi->flags, 6))
	{
		memory_map_t *mmap;

		printf ("mmap_addr = 0x%#x, mmap_length = 0x%x\n",
				(unsigned) mbi->mmap_addr, (unsigned) mbi->mmap_length);
		for (mmap = (memory_map_t *) mbi->mmap_addr;
				(unsigned long) mmap < mbi->mmap_addr + mbi->mmap_length;
				mmap = (memory_map_t *) ((unsigned long) mmap
					+ mmap->size + sizeof (mmap->size)))
			printf (" size = 0x%x,     base_addr = 0x%#x%#x\n"
					"     type = 0x%x,  length    = 0x%#x%#x\n",
					(unsigned) mmap->size,
					(unsigned) mmap->base_addr_high,
					(unsigned) mmap->base_addr_low,
					(unsigned) mmap->type,
					(unsigned) mmap->length_high,
					(unsigned) mmap->length_low);
	}

	/* Construct an LDT entry in the GDT */
	{
		seg_desc_t the_ldt_desc;
		the_ldt_desc.granularity    = 0;
		the_ldt_desc.opsize         = 1;
		the_ldt_desc.reserved       = 0;
		the_ldt_desc.avail          = 0;
		the_ldt_desc.present        = 1;
		the_ldt_desc.dpl            = 0x0;
		the_ldt_desc.sys            = 0;
		the_ldt_desc.type           = 0x2;

		SET_LDT_PARAMS(the_ldt_desc, &ldt, ldt_size);
		ldt_desc_ptr = the_ldt_desc;
		lldt(KERNEL_LDT);
	}

	/* Construct a TSS entry in the GDT */
	{
		seg_desc_t the_tss_desc;
		the_tss_desc.granularity    = 0;
		the_tss_desc.opsize         = 0;
		the_tss_desc.reserved       = 0;
		the_tss_desc.avail          = 0;
		the_tss_desc.seg_lim_19_16  = TSS_SIZE & 0x000F0000;
		the_tss_desc.present        = 1;
		the_tss_desc.dpl            = 0x0;
		the_tss_desc.sys            = 0;
		the_tss_desc.type           = 0x9;
		the_tss_desc.seg_lim_15_00  = TSS_SIZE & 0x0000FFFF;

		SET_TSS_PARAMS(the_tss_desc, &tss, tss_size);

		tss_desc_ptr = the_tss_desc;

		tss.ldt_segment_selector = KERNEL_LDT;
		tss.ss0 = KERNEL_DS;
		tss.esp0 = 0x800000;
		ltr(KERNEL_TSS);
	}

	/* Initialize devices, memory, filesystem, enable device interrupts on the
	 * PIC, any other initialization stuff... */
	
	boot_block_t *filesystem_loaded = (boot_block_t *)fs_addr;

	/* IDT Initialization*/
	setup_idt();


	/* PIC Initialization */
	i8259_init();


	/* Hardware Initialization*/
	rtc_init();				// Initialize RTC
	keyboard_init(); 		// Initialize Keyboard


	/*Paging Initialization*/
	paging_init();

    load_filesystem((boot_block_t*)filesystem_loaded);
    //load_filesystem((boot_block_t*)((module_t*)mbi -> mods_addr) -> mod_start);
    
	/* CP1 Test Field */
	// int * a = 0x0;
	// printf("%d\n",5/0);

	/* Enable interrupts */
	/* Do not enable the following until after you have set up your
	 * IDT correctly otherwise QEMU will triple fault and simple close
	 * without showing you any output */
	printf("Enabling Interrupts\n");
	sti();

    terminal_init();

	/* CP2 Test Field */

/* ================ test for rtc ================== */
	// terminal_init();

	// int test_freq, i;
	// for (test_freq = 2; test_freq <= 1024; test_freq *= 2) {
	// 	rtc_write(0, &test_freq, 0);
	// 	for (i = 0; i < 3*test_freq; i++) {
	// 		rtc_read(0, NULL, 0);
	// 		terminal_putc('1');
	// 	}
	// 	terminal_init();
	// }
    
/* ================ start of test for the filesystem ================== */
  
//    terminal_init();
//    boot_block_t* bootblock = (boot_block_t*)filesystem_loaded;

/* ================ test for the all print ================== */
   // int i, j;
   // uint8_t output[OUTPUT_FILE_SIZE];
   // inode_t* inode_out;
   // for (i = 0; i < bootblock -> dir_entries_n; i++) {
   //     //bootblock -> dir_entries[i].file_name
   //     terminal_gogogo((uint8_t*)("file name: "));
   //     j = 0;
   //     while (bootblock -> dir_entries[i].file_name[j] != '\0' && j < NAME_LENGTH) {
   //         terminal_putc(bootblock -> dir_entries[i].file_name[j]);
   //         j++;
   //     }
   //     while (j < NAME_LENGTH) {
   //         terminal_putc(' ');
   //         j++;
   //     }
   //     //terminal_write(bootblock -> dir_entries[i].file_name);
   //     terminal_gogogo((uint8_t*)(" file type: "));
   //     itoa(bootblock -> dir_entries[i].file_type, (int8_t*)output, TEN_DECIMAL);
      
   //     terminal_gogogo(output);
   //     terminal_gogogo((uint8_t*)(" file size: "));
      
   //     inode_out =(inode_t*)((uint32_t)bootblock + (bootblock -> dir_entries[i].inode + 1) * BLOCK_SIZE);
      
   //     itoa(inode_out -> length, (int8_t*)output, TEN_DECIMAL);
      
   //     terminal_gogogo(output);
   //     if (i != bootblock -> dir_entries_n - 1) {
   //         terminal_enter();
   //     }
   // }
    
/* ============== test for read dentry by name ================== */
//    dentry_t dentrytest;
//    uint8_t buffer[FILE_BUF_SIZE];
//    inode_t* inode_out;
//    uint32_t bytes;
//
//    uint32_t test_index = 11; //index the file number
//    
//    read_dentry_by_name(bootblock -> dir_entries[test_index].file_name, &dentrytest); //bootblock -> dir_entries[test_index].file_name
//    inode_out = (inode_t*)((uint32_t)bootblock + (dentrytest.inode + 1) * BLOCK_SIZE);
//    bytes = read_data(dentrytest.inode, 0, buffer, inode_out -> length);
//    terminal_write(0, buffer, bytes);
//    terminal_write(0, (uint8_t*)"\nfile name:", TWELVE);
//    int j = 0;
//    while (dentrytest.file_name[j] != '\0' && j < NAME_LENGTH) {
//        terminal_putc(dentrytest.file_name[j]);
//        j++;
//    }

/* ============== test for read dentry by index ================== */
   // dentry_t dentrytest;
   // uint8_t buffer[FILE_BUF_SIZE];
   // inode_t* inode_out;
   // uint32_t bytes;

   // uint32_t test_index = 11; //index the file number
  
   // read_dentry_by_index(test_index, &dentrytest);
   // inode_out = (inode_t*)((uint32_t)bootblock + (dentrytest.inode + 1) * BLOCK_SIZE);
   // bytes = read_data(dentrytest.inode, 0, buffer, inode_out -> length);
   // terminal_write(0, buffer, bytes);
   // terminal_write(0, (uint8_t*)"\nfile name:", TWELVE);
   // int j = 0;
   // while (dentrytest.file_name[j] != '\0' && j < NAME_LENGTH) {
   //     terminal_putc(dentrytest.file_name[j]);
   //     j++;
   // }

/* ============== test for terminal read and write ================== */
    // while (1) {
	   //  uint8_t read[127] = {0};
	   //  int num = terminal_read(0, read, 127);
	   //  terminal_write(0, read, num);
    // }

	/* Execute the first program (`shell') ... */
	//execute((uint8_t*)"shell");
	syscall_init();
	pit_init(100);

	/* Spin (nicely, so we don't chew up cycles) */
	asm volatile(".1: hlt; jmp .1;");
}

