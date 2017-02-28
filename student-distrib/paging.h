/* paging.h - functions related to paging
 */

#ifndef _PAGING_H
#define _PAGING_H

#include "types.h"

/* Initialize paging */
void paging_init(void);
/* Set a Page Directory Entry */
void set_pde(uint32_t idx, uint32_t entry);
/* Set up Page Table Entries for Vidmap */
void set_video();
/* Change Page Table Entries for Vidmap when Switching Terminals */
void change_vid(uint32_t idx, uint32_t entry);

#endif

