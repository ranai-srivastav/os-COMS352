**Threads**: Concurrent points of execution with their own stack and program counter but shared heap and program

**Thread Control Board**
- Thread ID
- State
- Program Counter
- CPU Register
- Stack pointer


### pthread(thread, attr, start_routine(), arg)
- Thread = points to a buffer that stores the ID of the new thread
- attr = struct with the attributes
- arg = arguments of start routine
- **Returns** => 0 on success, returns an error number

- exit() kills all thread
- pthread_exit() returns only from that thread

### pthread_mutex_init(&mutex, attr)
- `phtread_t` mutex variable address

### pthread_mutex_destroy(&mutex)
- mutex must be open when destroying, else, it is undefined behavior

### pthread_mutex_lock(&mutex)
- Locks the mutex so only the owning thread can call
- If another thread calls, the calling thread is blocked

### pthread_mutex_unlock(&mutex)
- Error if a thread different from the one that owns it calls it


