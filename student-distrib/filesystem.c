#include "filesystem.h"
#include "lib.h"
#include "syscall.h"
#include "terminal.h"

//global variable
boot_block_t* bootblock;
static int32_t cur_dir;

/*
 * terminal_init
 *   DESCRIPTION: Initializes terminal by clearing screen initializing
 *                cursor to top of the page and clearing the keyboard
 *                buffer
 *   INPUTS: start --- the start the file system
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void load_filesystem(boot_block_t* start){
    bootblock = start;
    cur_dir = 0;
    printf("file system: 0x%x\n", bootblock);
}

/*
 * read_dentry_by_name
 *   DESCRIPTION: fill in the dentry t block passed as their second argument with the file name
 *   INPUTS: fname -- the file name passed in
 *           dentry -- the dentry_t structure to be changed
 *   OUTPUTS: none
 *   RETURN VALUE: success 0, fail -1
 *   SIDE EFFECTS: dentry changed by the thing we want
 */
int32_t read_dentry_by_name (const uint8_t* fname, dentry_t* dentry){
    int i;
    int length = 0;
    int fname_len = 0;
    // if null pointer, fail
    if (fname == NULL) {
        return -1;
    }
    // put the length in the variabe
    fname_len = strlen((int8_t *) fname);
    if(fname_len >= NAME_LENGTH)
        fname_len = NAME_LENGTH - 1;
    
    for (i = 0; i < bootblock -> dir_entries_n; i++) {
        length = strlen((int8_t *) bootblock->dir_entries[i].file_name);
        if (length == NAME_LENGTH + 1) {
            length = NAME_LENGTH - 1;
        }
        if (strncmp((int8_t*) fname, (int8_t*) bootblock -> dir_entries[i].file_name, fname_len) == 0 && fname_len == length) {
                //copy the string
            strcpy((int8_t *) dentry -> file_name, (int8_t *) bootblock -> dir_entries[i].file_name);
                // file type and inode changed
            dentry -> file_type = bootblock -> dir_entries[i].file_type;
            dentry -> inode = bootblock -> dir_entries[i].inode;
            return 0;
        }
    }
    return -1;
}

/*
 * read_dentry_by_index
 *   DESCRIPTION: fill in the dentry t block passed as their second argument with the file index
 *   INPUTS: index -- the index passed in
 *           dentry -- the dentry_t structure to be changed
 *   OUTPUTS: none
 *   RETURN VALUE: success 0, fail -1
 *   SIDE EFFECTS: dentry changed by the thing we want
 */
int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry){
    // if index is invalid return -1
    if (index < 0 || index >= bootblock -> dir_entries_n) {
        return -1;
    }
    // do the appropriate change in the dentry
    strcpy((int8_t *) dentry -> file_name, (int8_t *) bootblock -> dir_entries[index].file_name);
    dentry -> file_type = bootblock -> dir_entries[index].file_type;
    dentry -> inode = bootblock -> dir_entries[index].inode;
    
    return 0;
}

/*
 * read_data
 *   DESCRIPTION: reading up to length bytes starting from position offset in the file with inode
 *   number inode and returning the number of bytes read and placed in the buffer
 *   INPUTS: inode --- inode number
 *           offset --- the offset of the file system
 *           buf --- to be placed in the buffer
 *           length --- length if the bytes
 *   OUTPUTS: none
 *   RETURN VALUE: bytes read and placed
 *   SIDE EFFECTS: put the thing we want in the buffer
 */
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length){
    
    if (inode > bootblock -> inodes_n || buf == NULL) {
        return -1;
    }
    
    uint32_t i;
    uint32_t* datastart;
    inode_t* inode_go;
    uint32_t inode_length, index;
    
    //start address of the datablock
    datastart = (uint32_t*)((uint32_t)bootblock + (bootblock -> inodes_n + 1) * BLOCK_SIZE);
    // the specific inode address
    inode_go = (inode_t*)((uint32_t)bootblock + (inode + 1) * BLOCK_SIZE);
    //the length of the inode
    inode_length = inode_go -> length;
    //error condition
    if (offset >= inode_length) {
        return 0;
    }
    for (i = 0; i < length; i++) {
        if (offset + i >= inode_length) {
            break;
        }
        // the index of data
        index = *(uint32_t *)((uint32_t) inode_go + (1 + (i + offset)/BLOCK_SIZE) * FOUR);
        // load the right thing in the buffer
        buf[i] = *(uint8_t *)((uint32_t) datastart + index * BLOCK_SIZE + (i + offset) % BLOCK_SIZE);
    }
    // end with null terminator
    if(i < length) buf[i] = '\0';
    
    return i;
    
}


/*
 * filesystem_open
 *   DESCRIPTION: open the file system (for next system call checkpoint)
 *   INPUTS: filename --- the name of the file
 *   OUTPUTS: none
 *   RETURN VALUE: success 0, fail -1
 *   SIDE EFFECTS: none
 */
int32_t file_open(const uint8_t* filename){
    if (filename == NULL) return -1;
    
    return 0;
}

/*
 * filesystem_close
 *   DESCRIPTION: close the filesystem
 *   INPUTS: fd --- file descriptor
 *   OUTPUTS: none
 *   RETURN VALUE: 0
 *   SIDE EFFECTS: none
 */
int32_t file_close(int32_t fd){
    return 0;
}

/*
 * filesystem_read
 *   DESCRIPTION: read the file, same with the read_data
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: number of bytes read and placed
 *   SIDE EFFECTS: none
 */
int32_t file_read(int32_t fd, void* buf, int32_t nbytes){
    pcb_t* curr_pcb = get_pcb(terminal[processing_terminal].cur_pid);
    int32_t bytes = read_data(curr_pcb -> f_array[fd].inode, curr_pcb -> f_array[fd].fpos, buf, nbytes);
    curr_pcb -> f_array[fd].fpos += bytes;
    return bytes;
}

/*
 * filesystem_write
 *   DESCRIPTION: return fail because the file system is write only
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: -1 fail
 *   SIDE EFFECTS: none
 */
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes){
    //read only
    return -1;
}


/*
 * dir_open:
 *   DESCRIPTION: open the directory
 *   INPUTS: filename --- the file name
 *   OUTPUTS: none
 *   RETURN VALUE: success 0, fail -1
 *   SIDE EFFECTS: none
 */
int32_t dir_open(const uint8_t* filename)
{
    if (filename == NULL) return -1;
    cur_dir = 0;
    
    return 0;
}

/*
 * dir_close
 *   DESCRIPTION: close the directory
 *   INPUTS: fd - file descriptor
 *   OUTPUTS: none
 *   RETURN VALUE: success 0
 *   SIDE EFFECTS: none
 */

int32_t dir_close(int32_t fd)
{
    return 0;
}

/*
 * dir_read
 *   DESCRIPTION: read the dir
 *   INPUTS: fd - file descriptor
 *          buf - the data buffer
 *          nbytes - the number of bytes to read into the buffer
 *   OUTPUTS: none
 *   RETURN VALUE: bytes length copied into the buffer
 *   SIDE EFFECTS: none
 */

int32_t dir_read(int32_t fd, void * buf, int32_t nbytes)
{
    if(cur_dir >= bootblock->dir_entries_n) {
        cur_dir = 0;
        return 0;
    }

    // pcb_t* curr_pcb = get_pcb(cur_pid);
    // the file name
    // uint8_t* name = bootblock -> dir_entries[curr_pcb -> f_array[fd].fpos].file_name;
    uint8_t* name = bootblock -> dir_entries[cur_dir].file_name;
    
    // if(strlen((int8_t *) name) == 0){
    //     cur_dir = 0;
    //     return 0;
    // }
    // position increment
    // curr_pcb -> f_array[fd].fpos++;
    cur_dir++;
    //copy the name to the buffer
    return dir_read_helper(buf, name, nbytes);
    // return strlen((char*)buf);
}

/*
 * dir_read_helper
 *   DESCRIPTION: the helper function for dir read
 *   INPUTS:
 *          buf - the data buffer
 *          name - the file name
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
int32_t dir_read_helper(uint8_t* buf, uint8_t* name, int32_t nbytes){
    int j = 0;
    while (name[j] != '\0' && j < nbytes){
        buf[j] = name[j];
        j++;
    }
    if(j != nbytes) {
        buf[j] = '\0';
        j++;
    }
    return j;
}

/*
 * dir_write
 *   DESCRIPTION: write the directory
 *   INPUTS: fd - file descriptor
 *          buf - the data buffer
 *          nbytes - the number of bytes to read into the buffer
 *   OUTPUTS: none
 *   RETURN VALUE: failure: -1
 *   SIDE EFFECTS: none
 */

int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes)
{
    //read only
    return -1;
}

/*
 * inode_find
 *   DESCRIPTION: the helper function, given the dentry return inode structure
 *   INPUTS: dentry --- the directory entry
 *   OUTPUTS: none
 *   RETURN VALUE: the inode structure
 *   SIDE EFFECTS: none
 */
inode_t* inode_find(dentry_t dentry){
    return (inode_t*)((uint32_t)bootblock + (dentry.inode + 1) * BLOCK_SIZE);
}

