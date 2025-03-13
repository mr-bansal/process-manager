#ifndef _QUEUES_H_
#define _QUEUES_H_

#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
#include<assert.h>
#include<limits.h>
#include<math.h>

#define ARRIVED -1
#define READY 0
#define RUNNING 1
#define FINISHED 2

typedef struct node node_t;

typedef struct {
    uint32_t time_arrived; // first process always has time-arrived to 0
    char *process_name;
    uint32_t service_time;
    int memory_requirement;

    int time_ran;
    int state;
    int wait_time;
} process_t;

typedef struct {
    node_t *head;
    node_t *foot;
    int count;
} queue_t;

struct node {
    process_t *p;
    node_t *next;
};

// Queues and processes
int is_process_finished(process_t *p);
void increment_wait_time(process_t *p, int quantum);
int process_remaining_time(process_t *p);
process_t* build_process(char *lineptr, char *delim);
process_t* make_empty_process();
void free_process(process_t *p);
queue_t* make_empty_queue();
void enqueue_node_sjf(queue_t *q, node_t *n);
void enqueue_node(queue_t *q, node_t *n);
node_t* dequeue_node(queue_t *q);
void enqueue_process(queue_t *q, process_t *p);
process_t* dequeue_process(queue_t *q);
int queue_is_empty(queue_t *q);

#endif