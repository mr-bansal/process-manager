#ifndef _ALLOCATE_H_
#define _ALLOCATE_H_

#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
#include<assert.h>
#include<limits.h>
#include<math.h>
#include"queues.h"

#define SJF 0
#define RR 1
#define INFINITE 0
#define BESTFIT 1

#define MEMORY_MB 2048

typedef struct segment segment_t;

// doubly linked list
struct segment {
    int mem_start;
    int mem_end;
    segment_t *next_s;
    segment_t *prev_s;
    process_t *process;
};


void print_ready_process(int time, char *name, int mem_index);
void print_running_process(int time, char *name, int remaining_time);
void print_finished_process(int time, char* name, int count);

// Memory
void merge_memory_holes(segment_t *memory);
segment_t *make_new_segment(int from, int to);
segment_t* initialize_simulated_memory(int memory_strategy);
int segment_available_memory(segment_t *curr);

// Process scheduling
void allocate_input_processes(queue_t *input_q, segment_t *memory, queue_t *ready_q, int strategy, int simulated_time, int quantum);
segment_t* initialize_simulated_memory(int memory_strategy);
void schedule_processes(queue_t *infile_q, queue_t *input_q,
     queue_t *ready_q, int strategy, int quantum, segment_t *memory, int memory_strategy);
void add_to_ready_q_infinite(queue_t *input_q, queue_t *ready_q, int strategy, int quantum);
void add_to_input_q(queue_t *infile_q, queue_t *input_q,
     int simulated_time, int* num_processes);


#endif