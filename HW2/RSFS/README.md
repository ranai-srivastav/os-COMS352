## HW 2 Ridiculously Simple File System (In memory FS)

In this assignment, I made an InMemory filessytem that supports a max of 8 files of 256 Bytes each.
The FS supports concurrency and deals with the Readers-Writers problem using 2 binary semaphores. 

To compile the code, run `make` in the RSFS folder in the terminal.
To run the application, run `./app` in the terminal.

**These following files are responsible for the following tasks:**
  - `def.h`
    - The definition for all the structs used in the code and the initialization of all the api calls for RSFS
  - `data_block.c`
    - Has methods that allow for the creation and deletion of datablocks
  - `inode.c`
    - Has methods that allow for the creation and deletion of inodes
  - `open_file_table.c`
    - Has methods that allow for the creation and deletion of open_file_table_entries
  - `api.c`
    - `int RSFS_create(char *file_name)`
      - Creates a file with name `*filename` and initializes the inodes and semaphores
    - `int RSFS_open(char *file_name, int access_flag)`
      - Opens a file that exists in thedirectory structure
    - `int RSFS_read(int fd, void *buf, const int size)`
      - Reads min(size, file_length) bytes into buf
    - `int RSFS_write(int fd, void *buf, const int size)`
      - writes min(size, MAX_FILE_SIZE) bytes from buf
    - `int RSFS_fseek(int fd, int offset, int whence)`
      - Sets the current position of the "cursor" to offset OR sets it to the end of the file OR sets it to the beginning of the file depending on the whence flag
    - `int RSFS_close(int fd)`
      - Closes the file and changes the semaphores to appropriate things 
    - `int RSFS_delete(char *file_name);`
      - Deletes a file
      - 
    More detailed documentation can be found on the method stubs as docstrings



