# OperatingSystem
## 2023 Operating System Term project
Implement a simulator for RR scheduling, including the ready queue and I/O queue. (The parent process acts as the scheduler)
***
### 1. About Round-Robin CPU Scheduling
Round Robin (RR) scheduling algorithm is designed specifically for time-sharing systems. It is a preemptive version of first-come, first-served scheduling. Processes are dispatched in a first-in-first-out sequence, but each process can run for only a limited amount of time. This time interval is known as a time-slice or quantum. It is similar to FIFO scheduling, but preemption added to switches between processes.

### 2. Basic explanations and requirements
- **Parent process** :  
  - Parent process creates 10 child processes.  
  - Assume your own scheduling parameters: e.g., time quantum, and timer tick interval.  
  - Parent process periodically receives ALARM signal by registering timer event. The ALARM signal serves as periodic timer interrupt (or time tick).  
  - Parent process manages the ready queue and I/O queue.  
    - Elements of the queue should be pointer variables to structures holding information about each child process (e.g. pid, remaining CPU burst time) (think of PCB, though simpler).  
  - The parent process performs scheduling of its child processes: The parent process accounts for the remaining time quantum of all the child processes.  
  - The parent process gives time slice to the child process by sending IPC message through msgq.  
    - Please note that there is msgget, msgsnd, msgrcv system calls, and IPC_NOWAIT flag.  
  - Decreases the remaining i/o burst time of processes in the i/o queue with every time tick.  
  - Total running time should be more than 1 minute.   
- **Child process** :     
  - A child process simulates the execution of a user process.  
  - Workload consists of infinite loop of dynamic CPU-burst and I/O-burst.  
  - CPU burst is randomly determined at the time of child process creation.  
  - When a child process receives a start IPC message from the parent process, it is considered to be running.  
  - The CPU burst time decreases as much as the execution time.  
  - About I/O operation:  
    - At each dispatch, the 'creation' of I/O burst is determined randomly by a certain probability.  
    - If an i/o burst is created,  
      - The 'start time of I/O' is also randomly determined, as the 'I/O duration'.  
      - The'starttimeofI/O'come before the end of the time quantum while the process is in running state.
      - When 'start time of I/O' occurs, the next child process in the ready queue executes.  
      - Information about the i/o is sent to the parent process via IPC.  
    - There is a option for creating I/O burst time:  
When child process enters running state, 'whether to perform i/o', 'i/o duration', and 'i/o start time' are randomly determined for the process. When the i/o start time is reached, the process must enter the i/o queue, and the parent process immediately dispatches another process from the ready queue.  
  - If the CPU burst time of a child process ends before the time quantum, an IPC message is sent to the parent.  
- **Logging**:  
  - The following contents are output for every time tick t:  
  - (1) pid of the child process in running state, (2) list of processes in the ready queue and i/o queue, (3) remaining cpu burst time and i/o burst time for each process.  
  - Print out all the operations to the following file: schedule_dump.txt.  
  - Students would like to refer to the following C-library function and system call: sprintf, open, write, close.  

### 3. Optional requirements
- **Implementing priority queue**:  
• Implement multi-level queues based on priority.  
• Different policies can be applied to each queue.  
- **Writing experimental results in graphs and tables**
***
## How to Implement
> For the source code, implementation in C/C++, Building through gcc or g++ compiler in a Linux or Windows WSL (WSL2) environment.  

### RR_scheduler  
`g++ -o main multi_level_queue.cpp` -> `./main`  
### Multi-level queue
`g++ -o main multi_level_queue.cpp` -> `./main`  


