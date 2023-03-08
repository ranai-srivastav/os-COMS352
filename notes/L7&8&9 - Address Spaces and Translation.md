**Address Space** is a process' view of memory
- Starts at 0
- is contiguous
- Heap and stack are plced at opposite ends

**Software-Based Translation**
- Performed once (statically) before the program begins execution


### Base and Bounds
- **Base** - points to the start of the physical memory
- **Bounds** - maximum legal address space
-  physical = virtual address + base
- No need for program to know what/where, OS takes care of it all
	- Easy to reallocate

How to code
1. Check if the program is in bounds
2. Check if all memory access are in bounds

**CONS**
- growing Dynamicallys is hard
- If Stack.Heap is the one that grow from the bottom up, all pointer snned to updated

### Segmentation
Locate parts of the address space independently in physical memory

For 14 bit address, Segment, get segment base, add offset


### Paging

**Cons**: Page table needs to be looked at

