An operating system has two modes

- **User Mode:**
	- Each process executes in kernel ode and has access to its own memory space
- **Kernel Mode:** 
	- All instructions are allowed. Only the OS can execute in this mode 
	- Allocates registers, clears memory, load the program into memory, etc.

## Trap
#trap is a software generated interrupt. In xv6, generated using `ecall()`
A trap table stores the address of the syscall handler

