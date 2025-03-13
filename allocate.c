#include "allocate.h"
#include "queues.h"

int main (int argc, char *argv[]) {
    int opt, strategy = SJF, memory_strategy = INFINITE, quantum;
    FILE *inFile;

    while ((opt = getopt(argc, argv, "f:s:m:q:")) != -1) {
        switch (opt) {
            case 'f': // Open and read input file
                inFile = fopen(optarg, "r");
                break;
            case 's': // strategy
                // 0 for SJF, 1 for RR
                if (strcmp(optarg, "RR") == 0) strategy = RR;
                break;
            case 'm': // allocate memory
                // 0 for infinite, 1 for best fit
                if (strcmp(optarg, "best-fit") == 0) memory_strategy = BESTFIT;
                break;
            case 'q': // quantum
                quantum = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Missing argument\n");
                exit(EXIT_FAILURE);
        }
    } 

    if (inFile == NULL) {
        fprintf(stderr, "Input file error\n");
        exit(EXIT_FAILURE);
    }

    queue_t *infile_q = make_empty_queue();
    queue_t *input_q = make_empty_queue();
    queue_t *ready_q = make_empty_queue();
    segment_t *simulated_memory = initialize_simulated_memory(memory_strategy);

    char *lineptr = NULL;
    size_t lineBufferLength = 0;
    char *delim = " ";

    while(getline(&lineptr, &lineBufferLength, inFile) != -1) {
        process_t *process = build_process(lineptr, delim);
        enqueue_process(infile_q, process);
    }
    free(lineptr);
    lineptr = NULL;

    fclose(inFile);

    schedule_processes(infile_q, input_q, ready_q, strategy, quantum, simulated_memory, memory_strategy);

    free(infile_q);
    free(input_q);
    free(ready_q);

    segment_t *head = simulated_memory, *to_free;
    while (head) {
        to_free = head;
        head = head->next_s;
        free(to_free);
    }

    return 0;
}

/* Process scheduling */
void schedule_processes(queue_t *infile_q, queue_t *input_q, queue_t *ready_q, 
    int strategy, int quantum, segment_t *memory, int memory_strategy) {

    int num_cycles = 0;
    int simulated_time = 0;
    int turnaround_time = 0;
    int num_processes = 0;
    double total_overhead = 0, max_overhead = 0;

    process_t *running_process = NULL;

    // Each loop is 1 cycle
    do {
        // 1 - if running process is completed, terminate and deallocate memory
        if (running_process) {
            if (is_process_finished(running_process)) {
                // terminate program, move to finished state
                turnaround_time += running_process->time_ran;
                turnaround_time += running_process->wait_time;
                
                double temp_overhead = (running_process->time_ran
                 + running_process->wait_time)
                 /(double)running_process->service_time;
                total_overhead += temp_overhead;
                max_overhead = fmax(temp_overhead, max_overhead);

                running_process->state = FINISHED;
                print_finished_process(simulated_time, running_process->process_name, ready_q->count + input_q->count);

                // Merge holes
                merge_memory_holes(memory);
                
                free_process(running_process);
                running_process = NULL;
            }
        }

        // 2 - Identify new processes to add to input queue
        // only when arrival time <= simulated time
        add_to_input_q(infile_q, input_q, simulated_time, &num_processes);

        // 3 - Move process from input to ready queue upon successful memory allocation
        // Depending on memory strategy - infinite or best fit
        if (memory_strategy == BESTFIT) {
            allocate_input_processes(input_q, memory, ready_q, strategy, simulated_time, quantum);
        } else {
            add_to_ready_q_infinite(input_q, ready_q, strategy, quantum);
        }

        // 4 - Process scheduling
        if (strategy == SJF) { // SJF
            // if no running process and ready_q has process, schedule next ready
            if (!running_process && !queue_is_empty(ready_q)) {
                node_t *to_free = dequeue_node(ready_q);
                running_process = to_free->p;
                free(to_free);
                print_running_process(simulated_time, running_process->process_name, 
                    process_remaining_time(running_process));
                
            }
        }
        else { // RR
            if (!queue_is_empty(ready_q)) {
                node_t *to_free = dequeue_node(ready_q);
                // No running process, like start of the program or when there is a gap between processes
                if (!running_process) {
                    running_process = to_free->p;
                    running_process->state = RUNNING;
                }
                // If running process, switch out
                else {
                    running_process->state = READY;
                    enqueue_process(ready_q, running_process);
                    running_process = to_free->p;
                    running_process->state = RUNNING;
                }
                free(to_free);
                print_running_process(simulated_time, running_process->process_name, 
                    process_remaining_time(running_process));
            }
        }

        if (running_process) {
            running_process->time_ran += quantum;
        }

        // increment wait time in ready_q
        node_t *curr = ready_q->head;
        while(curr) {
            curr->p->wait_time += quantum;
            curr = curr->next;
        }

        // increment wait time in input_q for processes that have not been allocated memory yet
        curr = input_q->head;
        while(curr) {
            curr->p->wait_time += quantum;
            curr = curr->next;
        }

        simulated_time += quantum;
        num_cycles++;

    } while (!queue_is_empty(infile_q) || !queue_is_empty(input_q) || !queue_is_empty(ready_q) || running_process);
    
    // Print performance statistics
    printf("Turnaround time %.0f\n", ceil((double)turnaround_time/num_processes));
    printf("Time overhead %.2f %.2f\n", max_overhead, total_overhead/num_processes);
    printf("Makespan %d\n", simulated_time-quantum);
}

/* If best-fit, allocate enough memory to processes */
void allocate_input_processes(queue_t *input_q, segment_t *memory, queue_t *ready_q, 
    int strategy, int simulated_time, int quantum) {
    node_t *curr_q_node = input_q->head;
    
    // traverse queue by popping and repushing into queue(s)
    // and attempt to allocate memory to each
    // if unavailable memory - repush in input_q
    // if available memory - push in ready_q
    while(curr_q_node) {
        node_t *tmp_node_next = curr_q_node->next;
        segment_t *curr_segment = memory;
        segment_t *best_fit_segment = NULL;
        
        // Loop through all memory segments
        while(curr_segment) {
            // if current segment has no process
            if (curr_segment->process == NULL) {
                // if segment can hold process in memory
                if (segment_available_memory(curr_segment) >= curr_q_node->p->memory_requirement) {
                    // if no best_fit_segment, curr is best available
                    if(!best_fit_segment) best_fit_segment = curr_segment;
                    // get lowest segment sequence number in case on equal size blocks
                    else {
                        if (segment_available_memory(best_fit_segment) > segment_available_memory(curr_segment)) {
                            best_fit_segment = curr_segment;
                        }
                    }
                }
            }
            // move on to next memory segment. Best fit searches all segments
            curr_segment = curr_segment->next_s;
        }
                
        // upon successful memory allocation, move to ready queue
        // Fix input queue, Enqueue process to ready queue, Split memory segment

        // Fixing input queue
        // Input queue - pop all queue nodes, and reenqueue
        // If successful allocation, queue current node into ready queue
        if (best_fit_segment) {

            increment_wait_time(curr_q_node->p, quantum);
            
            int times = input_q->count;
            int i = 0;
            node_t *curr = input_q->head;

            // dequeue input_q input_q->times times to pop all nodes, if curr is curr_q_node (the node we want)
            // then push into ready_q depending on Shortest Job First or Round Robin. Otherwise push
            // back into input_q
            while(i < times) {
                node_t *next = curr->next;
                curr = dequeue_node(input_q);

                // enqueue to ready_q if successful memory allocation
                if (curr == curr_q_node && best_fit_segment) {
                    if (strategy == SJF) enqueue_node_sjf(ready_q, curr);
                    else enqueue_node(ready_q, curr);
                    curr->p->state = READY;
                }
                else {
                    enqueue_node(input_q, curr);
                }
                curr = next;
                i++;
            }
        
            // Split memory segment to accomodate process
            int remaining_memory = best_fit_segment->mem_end - curr_q_node->p->memory_requirement;

            // assumes process memory req <= best fit segment size
            if (remaining_memory > 0) {
                // best_fit_segment reduces its size down to process->memory_requirement, and
                // next will be inserted into linked list after best_fit_segment
                segment_t *next = make_new_segment(best_fit_segment->mem_start + curr_q_node->p->memory_requirement, 
                    best_fit_segment->mem_end);
                // insertion into doubly linked list
                best_fit_segment->mem_end = best_fit_segment->mem_start + curr_q_node->p->memory_requirement - 1;
                next->next_s = best_fit_segment->next_s;
                next->prev_s = best_fit_segment;
                if (best_fit_segment->next_s) {
                    best_fit_segment->next_s->prev_s = next;
                } 
                best_fit_segment->next_s = next;
            }
            // assign process to memory segment
            best_fit_segment->process = curr_q_node->p;
            print_ready_process(simulated_time, curr_q_node->p->process_name, best_fit_segment->mem_start);
        }
        
        // move on to next process in input_q
        curr_q_node = tmp_node_next;
    }
}

void print_ready_process(int time, char *name, int mem_index) {
    printf("%d,READY,process_name=%s,assigned_at=%d\n",
        time, name, mem_index);
}

void print_running_process(int time, char *name, int remaining_time) {
    printf("%d,RUNNING,process_name=%s,remaining_time=%d\n",
        time, name, remaining_time);
}

void print_finished_process(int time, char* name, int count) {
    printf("%d,FINISHED,process_name=%s,proc_remaining=%d\n",
        time, name, count);
}

/* Merges two adjacent empty memory holes into one, after a process is terminated */
void merge_memory_holes(segment_t *memory) {
    segment_t *curr = memory;

    // Loop through all memory segments
    while(curr) {
        // Upon process finish, find the hole the recently finished process was in, or a hole
        if (!curr->process || (curr->process && curr->process->state == FINISHED)) {
            curr->process = NULL;
            segment_t *nexthole = curr->next_s;
            segment_t *to_free;
            // Loop through potential next segments that are also holes
            while(nexthole) {
                if (nexthole->process && nexthole->process->state != FINISHED) break;
                // merge nexthole into curr, then free
                // assert(!nexthole->process);
                curr->mem_end = nexthole->mem_end;
                curr->next_s = nexthole->next_s;
                to_free = nexthole;
                nexthole = nexthole->next_s;
                free(to_free);
            }
        }
        curr = curr->next_s;
    }
}

/* For infinite memory, add any nodes from input to ready instantly */
void add_to_ready_q_infinite(queue_t *input_q, queue_t *ready_q, int strategy, int quantum) {
    node_t *curr = input_q->head, *next;

    while(curr) {
        next = curr->next;
        node_t *to_enq = dequeue_node(input_q);
        increment_wait_time(to_enq->p, quantum);

        if (strategy==SJF) enqueue_node_sjf(ready_q, to_enq);
        else enqueue_node(ready_q, to_enq);
        to_enq->p->state = READY;

        curr = next;
    }
}

/* Adds any ready processes from infile_q to input_q */
void add_to_input_q(queue_t *infile_q, queue_t *input_q, int simulated_time, int* num_processes) {
    node_t *curr = infile_q->head;
    node_t *next;

    while (curr) {
        next = curr->next;
        if (curr->p->time_arrived <= simulated_time) {
            process_t *to_enq = dequeue_process(infile_q);
            enqueue_process(input_q, to_enq);
            (*num_processes)++;
        }
        curr = next;
    }

}

/* Creates a new memory segment to fit a process into */
segment_t *make_new_segment(int from, int to) {
    segment_t *s = malloc(sizeof(segment_t));
    assert(s!=NULL);

    s->mem_start = from;
    s->mem_end = to;
    s->process = NULL;
    s->next_s = NULL;
    s->prev_s = NULL;

    return s;
}

int segment_available_memory(segment_t *curr) {
    return curr->mem_end - curr->mem_start + 1;
}

/* Creates an empty memory hole from 0 to 2048 */
segment_t* initialize_simulated_memory(int memory_strategy) {
    segment_t *s = malloc(sizeof(segment_t));
    assert(s!=NULL);
    s->prev_s = NULL;
    s->next_s = NULL;

    s->mem_start = 0;
    s->mem_end = MEMORY_MB;
    
    s->process = NULL;

    return s;
}