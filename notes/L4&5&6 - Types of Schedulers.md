
**CPU bound** process want a lot of CPU time
**I/O bound** process require a quick response from I/O devices
**Interactive** wait for user input

$T_{arrival}$ = Time when job first enters ready state
$T_{Completion}$ = Time when job finishes
$T_{firstrun}$ = time when job first starts its first run on the CPU

### Turnaround time
$T_{turnaround} = T_{completion} - T_{arrival}$
Tells Time to complete jobs
Good for CPU bound processes where CPU runtime is important
___________________________________
### Response Time
$T_{responsetime} = T_{firstrun} - T_{arrival}$
Tells how long to response to I/O
Good for I/O bound process which have short CPU burst and long I/O burst
_____________

## FIFO

**Implementation**: Queue
**PreEmption**: None
**Pros**: 
1. Easy to Implement
**Cons**
1. Performance depends on arrival order. Large CPU burst before many other process can increase turnaround time and response time

_________________________________

## Shortest Job First (SJF)
**Implementation**: Priority Queue sorted by Job length
**PreEmption**: If a new job arrives with a shorter time to completion, it preempts the current process
**Pros**: 
1. Short jobs don't wait for long jobs to complete
**Cons**
1. Since an entire process needs to run, the *Response time* and *turn around* time can still be bad
__________________________________

## **Shortest Time to Completion First** (**STCF**)

**Implementation**: Priority Queue sorted by time to completion
**PreEmption**: None
**Pros**: 
1. Optimal avg turnaround time when all jobs arrive at the same time
**Cons**
1. Bad if any jobs come after a long job has started execution
2. Can also starve a long job if short jobs keep coming
3.  Needs to know the total runtime of a process which requires an oracle


___________________________

## Round Robin

**Implementation**: FIFO Queue
**PreEmption**: Job on CPU gets a time slices
**Pros**: 
1. Low response time
**Cons**
1. Costly because frequent context changes make it inefficient

___________________________________________________

## Multi Level Feedback queue

**Rule 1:** Priority(A) > Priority(B), A runs
**Rule 2:** Priority(A) == Priority(B), A and B run in RR order

Processes that need Fast response but little CPU time should be  higher priority
Processes that require long CPU time and little I/O should be lower priority

Adding feedback

**Rule 3:** When a process enters the system, it is places in the highest priority
~~**Rule 4:** ~~
- ~~If a process uses its entire time slice when running, its priority is reduced
- ~~If a process gives up its time slice before time is up, it stays at the same level~~
**New Rule 4**:
- Once a process uses up its time allotment, regardless of how many times it has given up the CPU, Reduce its priority
**Rule 5:** After some time S, move all processes to top priority


**Implementation**: Round Robin but the time lsices slowly increase in length
**PreEmption**: Job on CPU gets a time slices
**Pros**: 
1. Combines RR, STCF, SJF
**Cons**
1. May not be fair

_____________________
### Lottery Scheduler

- Each process is assigned a number of tickets
- Every time slice, scheduler randomly picks a process and that process is run for that time slice
- The runtime closely resembles the number of tickets each process has

**How to assign tickets?**
- More tickets mean higher priority
- Faireer than priority because lower priority process cannot be starved

**Pros**: 
1. No job can be starved
**Cons**
1. A short job can be unlucky and take an uncharacteristically high response time
2. Thus guarantess fairness over long period but short term, might be unoptimal
__________________
### Stride Scheduling

- Stride scheduling is deterministic
- $\text{stride} = \frac{\text{large number}}{\text{num tickets}}$
- init pass counter to 0
- When chosen, stride is assigned as the pass of the process and it keeps getting added to the variable
- So largest priority get a smaller stride vlaue and over time, it grows. Since it's stride is smaller, it grows slower than the other processes, and thus runs more frequently
- **Cons** needs to store one more variable in the stuct proc

______________________
### Linux completely Fair Scheduling

- OS keeps track of the runtime of each process
- After every some time, the scheduler picks the process with the lowest runtime
- If time interval is not right
	- Too short = Context switching too frequently
	- Too long = Might not be fair in the short term

Two paramters: scheduler_latency and minimum granularity

- High priority gets smaller stride value
- Each process has a dynamic time slice
- Larger weight gets larger priority
- vruntime = runtime + weight_0/summ of all weights
- When a new process joins, It is assigned the vruntime of the shortest process


__________________________________
