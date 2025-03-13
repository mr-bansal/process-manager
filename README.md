# Process Manager


## Overview
This project simulates a **process manager** that handles both **memory allocation** and **process scheduling**. The simulation consists of two phases:
1. **CPU Scheduling Phase:** A scheduling algorithm assigns CPU time to processes.
2. **Memory Allocation Phase:** Processes are allocated memory before scheduling occurs.

The system assumes a **single CPU** environment, where scheduling follows a specified algorithm.

## Process Execution Cycle
The process manager operates in discrete cycles, each spanning a predefined quantum length. The simulation time increases in increments of the quantum value per cycle:

\[ T_s = N \times Q \]

Where:
- **T_s**: Current simulation time
- **N**: Number of elapsed cycles
- **Q**: Length of each quantum (integer between 1 and 3)

### Workflow Per Cycle
1. **Process Completion Check:** If the currently running process has finished execution, it is terminated and its memory is deallocated.
2. **Process Submission:** New processes that arrived since the last cycle are added to the input queue.
3. **Memory Allocation:** The system attempts to allocate memory to processes in the input queue.
   - **Infinite Memory Mode:** All processes automatically enter the READY state.
   - **Best Fit Memory Allocation:** Processes are allocated memory based on availability, and only successfully allocated processes enter the READY queue.
4. **Process Scheduling:** The scheduling algorithm determines which process runs in the current cycle.

## Process Lifecycle and States
- **NEW → READY:** A process enters the READY state once it has arrived and been allocated memory.
- **READY → RUNNING:** A scheduling algorithm selects a process for execution.
- **RUNNING → READY (Preemptive Mode):** The process may be suspended and placed back into the READY queue if another process is scheduled.
- **RUNNING → FINISHED:** Once a process completes execution, it terminates and releases allocated memory.

## Scheduling Algorithms
### Non-Preemptive Scheduling (Shortest Job First - SJF)
- The process with the shortest service time is selected from the READY queue.
- It runs to completion without interruption.
- If additional CPU time is needed after a quantum, the process continues execution.

### Preemptive Scheduling (Round Robin - RR)
- The process at the front of the READY queue runs for one quantum.
- After one quantum:
  - If the process is completed, it moves to the FINISHED state.
  - If more CPU time is needed and other READY processes exist, it is suspended and placed at the end of the queue.
  - If no other READY processes exist, it continues running for another quantum.

## Memory Allocation Strategy
Before scheduling, the manager simulates **Best Fit memory allocation** using a **doubly linked list** structure. Adjacent free blocks are merged upon process termination.

- **Total Available Memory:** 2048 MB
- **Fixed Memory Allocation:** A process receives a single contiguous block for its entire runtime.
- **Memory Release:** Upon process termination, its allocated memory is freed and merged with adjacent holes.

## Usage
Run the program using the following command:

```sh
./allocate -f <filename> -s (SJF | RR) -m (infinite | best-fit) -q (1 | 2 | 3)
```

### Arguments:
- `-f <filename>` → Path to the input file containing process details.
- `-s <scheduler>` → Scheduling algorithm (`SJF` for Shortest Job First or `RR` for Round Robin).
- `-m <memory-strategy>` → Memory allocation strategy (`infinite` or `best-fit`).
- `-q <quantum>` → Quantum value (1, 2, or 3).

### Example Command:
```sh
./allocate -f processes.txt -s RR -m best-fit -q 3
```
This command executes `processes.txt` using **Round Robin scheduling**, **Best Fit memory allocation**, and a **quantum of 3** seconds.

### Example Input File (`processes.txt`)
```
0 P4 30 16
29 P2 40 64
99 P1 20 32
```
- **P4** arrives at time **0**, requires **30 seconds** of CPU time, and **16 MB** of memory.
- **P2** arrives at time **29**, requires **40 seconds** of CPU time, and **64 MB** of memory.
- **P1** arrives at time **99**, requires **20 seconds** of CPU time, and **32 MB** of memory.

### Expected Output:
```
0,READY,process_name=P1,assigned_at=0
0,RUNNING,process_name=P1,remaining_time=50
12,READY,process_name=P2,assigned_at=8
12,RUNNING,process_name=P2,remaining_time=30
...
174,FINISHED,process_name=P5,proc_remaining=0
```
At the end of execution, performance statistics are displayed:

- **Turnaround Time:** Average duration from arrival to completion.
- **Time Overhead:** Maximum and average ratio of turnaround time to service time.
- **Makespan:** Total simulation runtime.

---
This project simulates **CPU scheduling and memory allocation**, providing insights into how processes are managed in an operating system environment.
