#include"queues.h"

int is_process_finished(process_t *p) {
    return p->time_ran >= p->service_time;
}

/* Adds remainder wait time if submitted before a quantum multiple */
void increment_wait_time(process_t *p, int quantum) {
    if (p->time_arrived > 0) {
        int time = quantum - (p->time_arrived % quantum);
        if (time != quantum) {
            p->wait_time += time;
        }
    }
}

int process_remaining_time(process_t *p) {
    return p->service_time - p->time_ran;
}


/* Creates a new process, and assigns input data to variables */
process_t* build_process(char *lineptr, char *delim) {
    process_t *p = make_empty_process();
    char *lineptr2 = lineptr;

    p->time_arrived = atoi(strsep(&lineptr2, delim));
    p->process_name = strdup(strsep(&lineptr2, delim));
    p->service_time = atoi(strsep(&lineptr2, delim));
    p->memory_requirement = atoi(strsep(&lineptr2, delim));


    return p;
}

process_t* make_empty_process() {
    process_t *p = malloc(sizeof(process_t));
    assert(p != NULL);

    p->memory_requirement = 1;
    p->process_name = NULL;
    p->service_time = 0;
    p->state = ARRIVED;
    p->time_arrived = 0;
    p->time_ran = 0;
    p->wait_time = 0;

    return p;
}

void free_process(process_t *p) {
    free(p->process_name);
    p->process_name = NULL;
    free(p);
}

queue_t* make_empty_queue() {
    queue_t *q = malloc(sizeof(queue_t));
    assert(q!=NULL);
    q->head = q->foot = NULL;
    q->count = 0;

    return q;
}

/* Adds node to queue by priority of remaining time - Shortest Job First */
void enqueue_node_sjf(queue_t *q, node_t *n) {
    assert(n!=NULL);    
    node_t *prev = NULL, *curr = q->head;

    if (queue_is_empty(q)) {
        enqueue_node(q, n);
        return;
    }

    // Inserts a node inbetween nodes
    while (curr) {
        if (n->p->service_time < curr->p->service_time) {
            if (prev == NULL) {
                q->head = n;
            } else {
                prev->next = n;
            }
            n->next = curr;
            q->count++;
            return;
        }
        prev = curr;
        curr = curr->next;
    }
    // Enqueue normally if reached end of queue
    enqueue_node(q, n);
}

/* Enqueues node to queue foot */
void enqueue_node(queue_t *q, node_t *n) {
    assert(n!=NULL);
    
    q->count++;
    if (q->foot==NULL) q->head = q->foot = n;
    else { q->foot->next = n; q->foot = n;}
}

node_t* dequeue_node(queue_t *q) {
    node_t *n = q->head;

    if (n) {
        q->head = n->next;
        n->next = NULL;
        q->count--;
    }
    if (!q->head) {
        q->foot = NULL;
    }

    return n;
}

/* Enqueues a process into a queue by putting it inside a queue node */
void enqueue_process(queue_t *q, process_t *p) {
    node_t *new = malloc(sizeof(node_t));
    assert(new!=NULL);
    q->count++;
    new->p = p;
    new->next = NULL;

    if (q->foot == NULL) {q->head = q->foot = new; }
    else { q->foot->next = new; q->foot = new; }
}

/* Dequeues head queue-node's process */
process_t* dequeue_process(queue_t *q) {
    process_t *p = q->head->p;
    q->count--;
    node_t *to_free = q->head;
    q->head = q->head->next;
    free(to_free);
    return p;
}

int queue_is_empty(queue_t *q) {
    return (q->head==NULL && q->count==0);
}