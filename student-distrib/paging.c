/* paging.h - functions related to paging in C
 */

#include "paging.h"
#include "lib.h"

// Magic Numbers
#define VIDEO               0xB8000
#define PAGE_4KB            0x1000
#define NUM_ENTRIES         1024
#define SIZE_4KB            4096 
#define SHIFT_TO_10         22
#define SHIFT_TO_20         12
#define KERNEL_ADDRESS      0x400000

// global variables: Page Directory aligned to 4096 and Page Table aligned to 4096
static uint32_t pg_drct[NUM_ENTRIES] __attribute__((aligned (SIZE_4KB)));
static uint32_t pg_tbl_1[NUM_ENTRIES] __attribute__((aligned(SIZE_4KB)));
static uint32_t vid_pg_tbl_1[NUM_ENTRIES] __attribute__((aligned(SIZE_4KB)));

// Local functions
/* Helper function that writes to Control Registers to enable paging */
void enable_paging(void);

/*
 * paging_init
 *   DESCRIPTION: Initializes paging by setting bits in the PDE
 *                and PTE. For the Page Directory, the first
 *                4 MB should point to a Page Table (which points
 *                to 4 kB each), the second 4 MB points to the 
 *                kernels pages. In the Page Table, video memory
 *                is located at VIDEO and those pages are present.
 *                The other pages are marked as not present.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void paging_init(void) {
/* Initialize paging */
    int i;
    uint32_t tbl_adr = (uint32_t)&pg_tbl_1;

    // initialize all PDE to be "not present"
    for(i = 0; i < NUM_ENTRIES; i++) {
        pg_drct[i] = 0x2;                       // clear bit 0 (present), supervisor mode
    }

    // first 4 MB is broken into 4kB
    pg_drct[0] = tbl_adr | 0x3;
                                                // set page table base address
                                                // set bit 7 (PS) and bit 0 (present)
                                                // set bits 1 (R/W) and 2 (U/S)

    // initialize page table
    // initialize all PTE to be "not present"
    for(i = 0; i < NUM_ENTRIES; i++) {
        pg_tbl_1[i] = 0x2;                      // clear bit 0 (present), supervisor mode
    }
    // map video memory
    pg_tbl_1[VIDEO >> SHIFT_TO_20] = pg_tbl_1[VIDEO >> SHIFT_TO_20] | VIDEO | 0x3;
                                                // set video memory address VIDEO
                                                // set bit 0 (present)
                                                // set bits 1 (R/W) and 2 (U/S)
    // set video memory buffer for three terminal to present
    pg_tbl_1[(VIDEO+1*PAGE_4KB) >> SHIFT_TO_20] = pg_tbl_1[(VIDEO+1*PAGE_4KB) >> SHIFT_TO_20] | (VIDEO+1*PAGE_4KB) | 0x3;
    pg_tbl_1[(VIDEO+2*PAGE_4KB) >> SHIFT_TO_20] = pg_tbl_1[(VIDEO+2*PAGE_4KB) >> SHIFT_TO_20] | (VIDEO+2*PAGE_4KB) | 0x3;
    pg_tbl_1[(VIDEO+3*PAGE_4KB) >> SHIFT_TO_20] = pg_tbl_1[(VIDEO+3*PAGE_4KB) >> SHIFT_TO_20] | (VIDEO+3*PAGE_4KB) | 0x3;
    // 4-8 MB mapped to physical memory 4-8 MB (a single page)
    pg_drct[1] = KERNEL_ADDRESS | 0x83;
                                                // set kernel address 0x400000
                                                // set bit 7 (PS) and bit 0 (present)
                                                // set bit 1 (R/W) and clear bit 2 (U/S)
    set_video();
    // enable paging
    enable_paging();
}

/*
 * enable_paging
 *   DESCRIPTION: Enables paging by writing bits into the
 *                Control Registers. (CR0 must be last)
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void enable_paging(void) {
   asm volatile("mov %0, %%cr3":: "r"(pg_drct));

   uint32_t cr4;
   asm volatile("mov %%cr4, %0": "=r"(cr4));
   cr4 |= 0x00000010;
   asm volatile("mov %0, %%cr4":: "r"(cr4));

   uint32_t cr0;
   asm volatile("mov %%cr0, %0": "=r"(cr0));
   cr0 |= 0x80000001;
   asm volatile("mov %0, %%cr0":: "r"(cr0));
}

/*
 * set_pde
 *   DESCRIPTION: Sets a Page Directory entry to enable paging for
 *                that block of memory and flushes TLB
 *   INPUTS: idx--index of the Page Directory to be written
 *           entry--what the content of the entry of the Page Directory
 *                  should be
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: flushes TLB
 */
void set_pde(uint32_t idx, uint32_t entry) {
  pg_drct[idx] = entry;

  // Flush TLB by resetting CR3
  uint32_t cr3;
  asm volatile("mov %%cr3, %0":: "r"(cr3));
  asm volatile("mov %0, %%cr3": "=r"(cr3));
}

/*
 * change_vid
 *   DESCRIPTION: Changes the entries of the user virtual video memory
 *                for terminal switching and fishs
 *   INPUTS: idx--index of the page table entry to be written
 *           entry--what the content of the entry of the Page Directory
 *                  should be
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: flushes TLB
 */
void change_vid(uint32_t idx, uint32_t entry) {
  vid_pg_tbl_1[idx] = entry;

  // Flush TLB by resetting CR3
  uint32_t cr3;
  asm volatile("mov %%cr3, %0":: "r"(cr3));
  asm volatile("mov %0, %%cr3": "=r"(cr3));
}


/*
 * set_video
 *   DESCRIPTION: Sets the last entry of the Page Directory to be mapped
 *                to video memory, used for vidmap
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: flushes TLB
 */
void set_video() {
  int i;
  uint32_t vid_tbl_adr = (uint32_t)&vid_pg_tbl_1;

  pg_drct[NUM_ENTRIES-1] = vid_tbl_adr | 0x7;
                                            // set page table base address
                                            // set bit 7 (PS) and bit 0 (present)
                                            // set bits 1 (R/W) and 2 (U/S)
  // initialize all PTE to be "not present"
  for(i = 0; i < NUM_ENTRIES; i++) {
    vid_pg_tbl_1[i] = 0x6;                  // clear bit 0 (present), user mode
  }
  for(i = 0; i < 3; i++) {
    vid_pg_tbl_1[i] = VIDEO | 0x7;
                                            // set video memory address VIDEO
                                            // set bit 0 (present)
                                            // set bits 1 (R/W) and 2 (U/S)
  }

  uint32_t cr3;
  asm volatile("mov %%cr3, %0":: "r"(cr3));
  asm volatile("mov %0, %%cr3": "=r"(cr3));
}
