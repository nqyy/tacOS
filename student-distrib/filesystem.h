#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H

#include "types.h"
// magic numbers
#define NAME_LENGTH 32
#define RESERVED1 24
#define BLOCK_NUMS 1023
#define RESERVED2 52
#define DIR_ENTRIES_NUMS 63
#define BLOCK_SIZE 4096
#define FOUR 4
// structure of directory entry
typedef struct dentry_t{
    uint8_t file_name[NAME_LENGTH];
    uint32_t file_type;
    uint32_t inode;
    uint8_t reserved[RESERVED1];
}dentry_t;

/*structure of inode*/
typedef struct inode_t{
    uint32_t length;
    uint32_t block[BLOCK_NUMS];
}inode_t;

/*structure of boot block*/
typedef struct boot_block_t{
    uint32_t dir_entries_n;
    uint32_t inodes_n;
    uint32_t data_blocks;
    uint8_t reserved[RESERVED2];
    dentry_t dir_entries[DIR_ENTRIES_NUMS];
}boot_block_t;

// load fs function and the big three function
void load_filesystem(boot_block_t* start);
// load fs function and the big three function

int32_t read_dentry_by_name (const uint8_t* fname, dentry_t* dentry);
// load fs function and the big three function

int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry);
// load fs function and the big three function

int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);

// file system open close read and write
int32_t file_open(const uint8_t* filename);
// file system open close read and write

int32_t file_close(int32_t fd);
// file system open close read and write

int32_t file_read(int32_t fd, void* buf, int32_t nbytes);
// file system open close read and write

int32_t file_write(int32_t fd, const void* buf, int32_t nbytes);

// functions for directory
int32_t dir_open(const uint8_t* filename);
int32_t dir_close(int32_t fd);
int32_t dir_read(int32_t fd, void * buf, int32_t nbytes);
int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes);

inode_t* inode_find(dentry_t dentry);
int32_t dir_read_helper(uint8_t* buf, uint8_t* file_name, int32_t nbytes);

#endif
