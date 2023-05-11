/*
    Implementation of API.
    Treat the comments inside a function as friendly hints; but you are free to implement in your own way as long as it meets the functionality expectation.
*/

#include "def.h"

pthread_mutex_t mutex_for_fs_stat;

//initialize file system - should be called as the first thing in application code
int RSFS_init(){

    //initialize data blocks
    for(int i=0; i<NUM_DBLOCKS; i++){
        void *block = malloc(BLOCK_SIZE); //a data block is allocated from memory
        if(block==NULL){
            printf("[sys_init] fails to init data_blocks\n");
            return -1;
        }
        data_blocks[i] = block;
    }

    //initialize bitmaps
    for(int i=0; i<NUM_DBLOCKS; i++) data_bitmap[i]=0;
    pthread_mutex_init(&data_bitmap_mutex,NULL);
    for(int i=0; i<NUM_INODES; i++) inode_bitmap[i]=0;
    pthread_mutex_init(&inode_bitmap_mutex,NULL);

    //initialize inodes
    for(int i=0; i<NUM_INODES; i++){
        inodes[i].length=0;
        for(int j=0; j<NUM_POINTER; j++)
            inodes[i].block[j]=-1; //pointer value -1 means the pointer is not used

    }
    pthread_mutex_init(&inodes_mutex,NULL);

    //initialize open file table
    for(int i=0; i<NUM_OPEN_FILE; i++){
        struct open_file_entry entry=open_file_table[i];
        entry.used=0; //each entry is not used initially
        pthread_mutex_init(&entry.entry_mutex,NULL);
        entry.position=0;
        entry.access_flag=-1;
    }
    pthread_mutex_init(&open_file_table_mutex,NULL);

    //initialize root directory
    root_dir.head = root_dir.tail = NULL;

    //initialize mutex_for_fs_stat
    pthread_mutex_init(&mutex_for_fs_stat,NULL);

    //return 0 means success
    return 0;
}


//create file with the provided file name
//if file does not exist, create the file and return 0;
//if file_name already exists, return -1;
//otherwise, return -2.
int RSFS_create(char *file_name){

    //search root_dir for dir_entry matching provided file_name
    struct dir_entry *dir_entry = search_dir(file_name);

    if(dir_entry){//already exists
        printf("[create] file (%s) already exists.\n", file_name);
        return -1;
    }else{

        if(DEBUG) printf("[create] file (%s) does not exist.\n", file_name);

        //construct and insert a new dir_entry with given file_name
        dir_entry = insert_dir(file_name);
        if(DEBUG) printf("[create] insert a dir_entry with file_name:%s.\n", dir_entry->name);

        //access inode-bitmap to get a free inode
        int inode_number = allocate_inode();
        if(inode_number < 0){
            printf("[create] fail to allocate an inode.\n");
            return -2;
        }
        if(DEBUG) printf("[create] allocate inode with number:%d.\n", inode_number);

        struct inode* curr_inode = &inodes[inode_number];
        curr_inode->reader_count = 0;
        sem_init(&curr_inode->rw_mutex, 0, 1);
        sem_init(&curr_inode->mutex, 0, 1);

        //save inode-number to dir-entry
        dir_entry->inode_number = inode_number;

        return 0;
    }
}


/**
 * open a file with RSFS_RDONLY or RSFS_RDWR flags only.
 * @param file_name The name of the file
 * @param access_flag the access permissions
 * @return the file descriptor (i.e., the index of the open file entry in the open file table) if succeed, or -1 in case of error
 */
int RSFS_open(char *file_name, int access_flag)
{
    if(DEBUG)
    {
        printf("\n ---------- F_OPEN DEBUG -----------------\n");
        printf("File %s\n", file_name);
        printf("Access Flag %d\n", access_flag);
    }

    // Ensure that the file has the correct access rights, if not we return error == -1
    if(access_flag != RSFS_RDONLY && access_flag != RSFS_RDWR)
    {
        return -1;
    }

    // Look for the file in the list of directories
    struct dir_entry* dir_index = search_dir(file_name);
    struct inode* curr_inode = &inodes[dir_index->inode_number];

    // If the file is not found, we return -1 == error, else we continue with execution
    if(dir_index == NULL)
    {
        return -1;
    }

    if(access_flag == RSFS_RDONLY)
    {
        if(DEBUG) printf("[RDR]Attempting to get mutex\n");
        sem_wait(&curr_inode->mutex);
        if(DEBUG) printf("[RDR]Got mutex\n");
        curr_inode->reader_count = curr_inode->reader_count + 1;
        if(DEBUG) printf("[RDR]Reader count updated to %d\n", curr_inode->reader_count);

        if(curr_inode->reader_count == 1)
        {
            if(DEBUG) printf("[RDR]Attempting to get rw_mutex\n");
            sem_wait(&curr_inode->rw_mutex);
            if(DEBUG) printf("[RDR]Got rw_mutex\n");
        }
        sem_post(&curr_inode->mutex);
    }
    else
    {
        if(DEBUG) printf("[WRTR]Attempting to get rw_mutex\n");
        sem_wait(&curr_inode->rw_mutex);
        if(DEBUG) printf("[WRTR]Got rw_mutex\n");
    }

    //file descriptor = The index of the file in the open_file_table
    int fd =  allocate_open_file_entry(access_flag, dir_index);
    if(DEBUG) printf("---- FILE OPEN END ---- \n");
    return fd;
}


//read from file: read up to size bytes from the current position of the file of descriptor fd to buf;
//read will not go beyond the end of the file;
//return the number of bytes actually read if succeed, or -1 in case of error.
/** read from file: read up to size bytes from the current position of the file of descriptor fd to buf;
 *
 * @param fd The file descriptor, an index in the open_file_table
 * @param buf Where to but the read contents
 * @param size The number of bytes that we should read
 * @return return the number of bytes actually read if succeed, or -1 in case of error.
 */
int RSFS_read(int fd, void *buf, const int size)
{
    //sanity test of fd and size
    if(fd >= NUM_OPEN_FILE || fd < 0)
    {
        return -1;
    }

    if (size > BLOCK_SIZE * NUM_POINTER || size < 0)
    {
        return -1;
    }

    //get the open file entry of fd
    struct open_file_entry* curr_file = &open_file_table[fd];

    //lock the open file entry
    pthread_mutex_lock(&curr_file->entry_mutex);

    //get the dir entry
    struct dir_entry* curr_dir_entry = curr_file->dir_entry;

    //get the inode
    struct inode* curr_inode = &inodes[curr_dir_entry->inode_number];

    // Assume a read to the end of the file
    int num_bytes_read = 0;
    int num_bytes_to_read = curr_inode->length;

    // if we need to read less than the end of file, then put that here
    if (curr_inode->length > size)
    {
        num_bytes_read = size;
    }

    const int MAX_FILE_SIZE_IN_BYTES = BLOCK_SIZE * NUM_POINTER;

    // if reading will make me go over the length of the file, then truncate how much I can read
    if(curr_file->position + size > curr_inode->length)
    {
        if(DEBUG)
        {
            printf("Protecting you against overflows\n");
            printf("Current Position + size > inode_len? \n");
            printf("%d + %d > %d? \n", curr_file->position, size, curr_inode->length);
        }
        // calculating how much I will be over by
        int over = (curr_file->position + size) - curr_inode->length;
        num_bytes_to_read = size - over;
    }

    // Keep reading until the number of bytes you were supposed to read is smaller than the number of bytes read AND
    // Keep reading until your current position is less than the length of the file
    while(num_bytes_read < num_bytes_to_read && curr_file->position < curr_inode->length)
    {
        //copy data from the data block(s) to buf and update current position
        int inode_read_index = (int) (curr_file->position / BLOCK_SIZE);

        // If the index you are trying to access is greater than the total number of inodes for this file, DON'T read anymore
        if(inode_read_index >= NUM_POINTER)
        {
            if(DEBUG) printf("[WRITE] EXIT COND: INode Read Index %d was greater than 7\n", inode_read_index);
            pthread_mutex_unlock(&curr_file->entry_mutex);
            return curr_file->position;
        }

        // How much into a datablock are we?
        int dblock_read_offset = curr_file->position % BLOCK_SIZE;

        // which datablock to read from
        int dblock_index = curr_inode->block[inode_read_index];

        if(DEBUG)
        {
            printf("[READING]: Size: %d\n", size);
            printf("[READING]: The datablock index: %d\n", inode_read_index);
            printf("[READING]: The datablock that exists in memory %s\n\n", (char*) data_blocks[dblock_index]);
//             printf("[READING]: The NEXT datablock %s\n", (char*) data_blocks[dblock_index+1]);
            printf("[READING]: numBytesRead %d numBytesToRead: %d, \n currFilePosition %d, inode_len: %d\n", num_bytes_read, num_bytes_to_read, curr_file->position, curr_inode->length);

        }

        // Will I ever actually have this?
        if(dblock_index < 0)
        {
            printf("[READING]: NEGATIVE DBLOCK INDEX %d in read\n", dblock_index);
            if(DEBUG)
            {
                printf("\n\n");
                printf("[READING]: The inode index: %d\n", inode_read_index);
                printf("[READING]: Position %d\n", curr_file->position);
                printf("[READING]: NumBytesToRead: %d", num_bytes_to_read);
                RSFS_stat();
                printf("\n\n");
            }
            exit(-5);
        }

        void* dblock = data_blocks[dblock_index];
        void* read_addr = dblock + dblock_read_offset;
        int size_to_read = BLOCK_SIZE - dblock_read_offset;

        if(DEBUG)
        {
            printf("[READING] ---- BEFORE ---- \n");
            printf("FILE %d\n", fd);
            printf("input size %d \n", size);
            printf("DBlock Index: %d, \ndblockReadOffset: %d\n", inode_read_index, dblock_read_offset);
            printf("currFilePos: %d: \n", curr_file->position);
            printf("INode length %d \n", curr_inode->length);
        }

        // If the amount I have to read is lesser than the size of a block, then truncate the num bytes to read
        if(num_bytes_to_read - num_bytes_read < BLOCK_SIZE)
        {
            size_to_read = num_bytes_to_read - num_bytes_read;
        }

        memcpy(buf + num_bytes_read, read_addr, size_to_read);

        curr_file->position = curr_file->position + size_to_read;
        num_bytes_read += size_to_read;

        if(DEBUG)
        {
            printf("[READING]: ---- AFTER ---- \n");
            printf("Buffer: %s \n", (char*) buf + num_bytes_read);
            printf("currFilePos: %d: \n", curr_file->position);
            printf("Done updating. Next Iteration --------------\n\n");
//             printf("numBytesRead %d numBytesToRead: %d, currFilePosition %d, inode_len: %d\n", num_bytes_read, num_bytes_to_read, curr_file->position, curr_inode->length);

        }
    }

    pthread_mutex_unlock(&curr_file->entry_mutex);
    return num_bytes_read;
}

//write file: write size bytes from buf to the file with fd
//return the number of bytes that have been written if succeed; return -1 in case of error
/** write file: write size bytes from buf to the file with fd
 *
 * @param fd The index of the file in the OpenFileTable
 * @param buf The contents of this array need to be written to a file
 * @param size The number of bytes to write
 * @return the number of bytes that have been written if succeed; return -1 in case of error
 */
int RSFS_write(int fd, void *buf, const int size)
{
    if(DEBUG)
    {
        printf("----- IN WRITE -----\n");
        printf("Input Size %d\n", size);
    }
    //sanity test of fd and size
    if(fd >= NUM_OPEN_FILE || fd < 0)
    {
        if(DEBUG) printf("[WRITE] !!!! fd was not between 0 and 7\n");
        return -1;
    }

    if (size < 0)
    {
        if(DEBUG) printf("[WRITE] !!!! size was less than 0 \n");
        return -1;
    }

    //get the open file entry
    struct open_file_entry* curr_file = &open_file_table[fd];

    //lock the open file entry
    pthread_mutex_lock(&curr_file->entry_mutex);

    //get the dir entry
    struct dir_entry* curr_dir_entry = curr_file->dir_entry;

    //get the inode
    struct inode* curr_inode = &inodes[curr_dir_entry->inode_number];

    //copy data from buf to the data block(s) and update current position;
    int num_bytes_to_write = size;
    int num_bytes_written = 0;

    const int MAX_FILE_SIZE = NUM_POINTER * BLOCK_SIZE;

    // If the number of bytes we are asked to write are greater than the MAX_FILE_SIZE
    if(MAX_FILE_SIZE - curr_file->position < size)
    {
        num_bytes_to_write = MAX_FILE_SIZE - curr_file->position;
    }

    int s = size;
    while(num_bytes_to_write > num_bytes_written)
    {
        if(DEBUG)
        {
            if(curr_file->position > MAX_FILE_SIZE)
            {
                printf("!!!!!!!!!!!!!!!!!!!!!!!!!Position exceeded Max File Size while writing!!!!!!!!!!!!!!!!!!!!!!!!!\n");
                exit(-2);
            }
            if(curr_inode->length > MAX_FILE_SIZE)
            {
                printf("!!!!!!!!!!!!!!!!!!!!! inode length Exceeded Max File Size while writing!!!!!!!!!!!!!\n");
                exit(-2);
            }
            if(s <= 0)
            {
                printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! var s is less equal to than 0 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
                exit(-2);
            }
        }


        int inode_index = curr_file->position / BLOCK_SIZE;

        //preventing an array index out of bounds if curr_position goes over accidentally
        if(inode_index >= NUM_POINTER)
        {
            pthread_mutex_unlock(&curr_file->entry_mutex);
            return curr_file->position;
        }

        int dblock_offset = curr_file->position % BLOCK_SIZE;

        int dblock_index = curr_inode->block[inode_index];

        //check to see if data block is allocated
        if (dblock_index < 0)
        {
            dblock_index = allocate_data_block();

            //Was allocation successful?
            if (dblock_index < 0)
            {
                if(DEBUG) printf("----[WRITE] Cannot allocate aby more DBLOCKS ----\n");
                //release our lock
                pthread_mutex_unlock(&curr_file->entry_mutex);
                return -1;
            }
            else
            {
                pthread_mutex_lock(&inodes_mutex);
                if(DEBUG)
                {
                    printf("\n\n+!+!+!+!+!+!+!+!+!+!+!!!!!  Assigning Inode block a number  !!!!!!!!+!+!+!+!+!+!+!+!+!+!+!+!+!+!+\n");
                    printf("InodeIndex = %d\n", inode_index);
                    printf("DBlock Index = %d\n\n", dblock_index);

                }
                curr_inode->block[inode_index] = dblock_index;
                pthread_mutex_unlock(&inodes_mutex);
            }
        }


        void* dblock = data_blocks[dblock_index];
        void* read_addr = dblock + dblock_offset;
        int block_write_size = BLOCK_SIZE - dblock_offset;

//        if(num_bytes_to_write - num_bytes_written < BLOCK_SIZE)
//        {
//            block_write_size = num_bytes_to_write - num_bytes_written;
//        }

        // If (the current length of the file + how much we are attempting to write) exceeds the MAX_FILE_SIZE
        if(curr_inode->length + block_write_size > MAX_FILE_SIZE)
        {
            block_write_size = MAX_FILE_SIZE - curr_inode->length;
            if(DEBUG)
            {
                printf("\n\n-------------------------------------------------------------------------------\n");
                printf("TRYING TO WRITE MORE BYTES THAN CAN FIT. Truncate to %d.\n", block_write_size);
                printf("NumBytesToWrite %d\n", num_bytes_to_write);
                printf("NumBytesWritten %d \n", num_bytes_written);
                printf("-------------------------------------------------------------------------------\n\n");
            }

            //If we cannot write any more
            if(block_write_size <= 0)
            {
                pthread_mutex_unlock(&curr_file->entry_mutex);
                return num_bytes_written;
            }
        }

        if(block_write_size - s > 0)
        {
            block_write_size = s;
        }

        if(DEBUG)
        {
            printf("File number fd: %d\n", fd);
            printf("Buffer: %s \n", (char*) buf + num_bytes_written);
            printf("Size %d\n", size);
            printf("INode index %d \n", inode_index);
            printf("INode length %d > %d?\n", curr_inode->length, MAX_FILE_SIZE);
            printf("Dblock offset %d \n", dblock_offset);
            printf("File Position %d \n", curr_file->position);
            printf("!!!!!!!------------------!!!!!!Dblock index %d \n", dblock_index);
            printf("block_write_size: %d \n", block_write_size);
            printf("NumBytesToWrite %d\n", num_bytes_to_write);
            printf("NumBytesWritten %d \n", num_bytes_written);
            printf("---------- MEMCPY ------------------\n");
        }

        memcpy(read_addr, buf + num_bytes_written, block_write_size);

        if(DEBUG)
        {
            printf("[WRITING]: The datablock that was just written: %s\n", (char*) data_blocks[dblock_index]);
        }

        num_bytes_written += block_write_size;
//        buf = buf + block_write_size;
        curr_file->position = curr_file->position + block_write_size;
        s -= block_write_size;
        if(DEBUG)
        {
            printf("File number fd: %d\n", fd);
            printf("Buffer: %s \n", (char*) buf + num_bytes_written);
            printf("INode index %d \n", inode_index);
            printf("File Position %d \n", curr_file->position);
            printf("Dblock index %d \n", dblock_index);
            printf("block_write_size: %d \n", block_write_size);
            printf("NumBytesWritten %d \n\n\n", num_bytes_written);
        }
    }
//    if(DEBUG) printf(" ----- WRITE END ----- \n");

    pthread_mutex_unlock(&curr_file->entry_mutex);

    curr_inode->length += num_bytes_written;
    return num_bytes_written;

}

//update current position: return the current position; if the position is not updated, return the original position
//if whence == RSFS_SEEK_SET, change the position to offset
//if whence == RSFS_SEEK_CUR, change the position to position+offset
//if whence == RSFS_SEEK_END, change hte position to END-OF-FILE-Position + offset
//position cannot be out of the file's range; otherwise, it is not updated
int RSFS_fseek(int fd, int offset, int whence)
/** update current position: return the current position; if the position is not updated, return the original position
 *  if whence == RSFS_SEEK_SET, change the position to offset
 *  if whence == RSFS_SEEK_CUR, change the position to position+offset
 *  if whence == RSFS_SEEK_END, change hte position to END-OF-FILE-Position + offset
 *
 * @param fd the file to seek in
 * @param offset How much to go into the file
 * @param whence FLAGS Choose one from the three mentioned in the description above
 * @return
 */
{

    if(DEBUG) printf(" ----- FSEEK START ----- \n");

    //sanity test of fd and whence
    if(fd >= NUM_OPEN_FILE)
    {
        return -1;
    }

    if(whence != RSFS_SEEK_CUR && whence != RSFS_SEEK_END && whence != RSFS_SEEK_SET)
    {
        return -1;
    }

    //get the open file entry of fd
    struct open_file_entry* curr_file = &open_file_table[fd];

    //TODO should there be a Mutex here and should it be this Mutex?
    //lock the entry
    pthread_mutex_lock(&curr_file->entry_mutex);

    //get the current position
    int curr_pos_in_file = curr_file->position;

    //get the dir entry
    struct dir_entry* curr_dir = curr_file->dir_entry;

    //get the inode
    struct inode curr_file_inode = inodes[curr_dir->inode_number];

    //change the position
    if(whence == RSFS_SEEK_SET)
    {
        curr_file->position = offset;
    }
    else if(whence == RSFS_SEEK_CUR)
    {
        curr_file->position = curr_pos_in_file + offset;
    }
    else
    {
        curr_file->position = curr_file_inode.length + offset;
    }

    if(curr_file->position < 0)
    {
        curr_file->position = 0;
    }
    else if(curr_file->position > curr_file_inode.length)
    {
        curr_file->position = curr_file_inode.length;
    }
    //unlock the entry
    pthread_mutex_unlock(&curr_file->entry_mutex);

    if(DEBUG) printf(" ----- END FSEEK with seek at pos %d ----- \n", curr_file->position);

    //return the current position
    return curr_file->position;

}


//close file: return 0 if succeed, or -1 if fd is invalid
int RSFS_close(int fd){

    if(fd >= NUM_OPEN_FILE)
    {
        return -1;
    }

    struct open_file_entry* curr_file = &open_file_table[fd];
    struct inode* curr_inode = &inodes[curr_file->dir_entry->inode_number];

    if(curr_file->access_flag == RSFS_RDONLY)
    {
        sem_wait(&curr_inode->mutex);
        curr_inode->reader_count--;
        if(curr_inode->reader_count == 0)
        {
            sem_post(&curr_inode->rw_mutex);
        }
        sem_post(&curr_inode->mutex);
    }
    else if(curr_file->access_flag == RSFS_RDWR)
    {
        sem_post(&curr_inode->rw_mutex);
    }

    free_open_file_entry(fd);
    return 0;
}


//delete file with provided file name: return 0 if succeed, or -1 in case of error
int RSFS_delete(char *file_name)
{
    //find the dir_entry; if find, continue, otherwise, return -1.
    struct dir_entry* curr_dir = search_dir(file_name);

    if(curr_dir == NULL)
    {
        return -1;
    }

    //find the inode
    struct inode* curr_inode = &inodes[curr_dir->inode_number];

    //find the data blocks, free them in data-bitmap
    int i = 0;
    for(i = 0; i < NUM_POINTER; i++)
    {
        int dblock_index = curr_inode->block[i];
        if(dblock_index > -1)
        {
            free_data_block(dblock_index);
        }
    }


    //free the inode in inode-bitmap
    free_inode(curr_dir->inode_number);

    //free the dir_entry
    delete_dir(file_name);


    return 0;
}


//print status of the file system
void RSFS_stat(){

    pthread_mutex_lock(&mutex_for_fs_stat);

    printf("\nCurrent status of the file system:\n\n %16s%10s%10s\n", "File Name", "Length", "iNode #");

    //list files
    struct dir_entry *dir_entry = root_dir.head;
    while(dir_entry!=NULL){

        int inode_number = dir_entry->inode_number;
        struct inode *inode = &inodes[inode_number];

        printf("%16s%10d%10d\n", dir_entry->name, inode->length, inode_number);
        dir_entry = dir_entry->next;
    }

    //data blocks
    int db_used=0;
    for(int i=0; i<NUM_DBLOCKS; i++) db_used+=data_bitmap[i];
    printf("\nTotal Data Blocks: %4d,  Used: %d,  Unused: %d\n", NUM_DBLOCKS, db_used, NUM_DBLOCKS-db_used);

    //inodes
    int inodes_used=0;
    for(int i=0; i<NUM_INODES; i++) inodes_used+=inode_bitmap[i];
    printf("Total iNode Blocks: %3d,  Used: %d,  Unused: %d\n", NUM_INODES, inodes_used, NUM_INODES-inodes_used);

    //open files
    int of_num=0;
    for(int i=0; i<NUM_OPEN_FILE; i++) of_num+=open_file_table[i].used;
    printf("Total Opened Files: %3d\n\n", of_num);

    pthread_mutex_unlock(&mutex_for_fs_stat);
}
