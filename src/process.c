#include <stdlib.h>
#include "process.h"

void process_init(Process *p, int pid, int arrival_time, int priority,
                   int num_bursts) {
    p->pid = pid;
    p->arrival_time = arrival_time;
    p->priority = priority;
    p->num_bursts = num_bursts;
    p->bursts = (Burst *) calloc((size_t) num_bursts, sizeof(Burst));
    p->current_burst_index = 0;
    p->remaining_time = 0;
    p->state = PROC_NEW;

    p->ready_since = arrival_time;
    p->estimated_next_burst = -1.0; /* sentinela: "nenhuma rajada observada ainda" */

    p->start_time = -1;
    p->completion_time = -1;
    p->waiting_time = 0;
    p->turnaround_time = 0;
    p->min_ideal_time = 0;
    p->slowdown = 0.0;
}

void process_free(Process *p) {
    free(p->bursts);
    p->bursts = NULL;
}

void process_compute_min_ideal(Process *p) {
    long total = 0;
    int i;
    for (i = 0; i < p->num_bursts; i++) {
        total += p->bursts[i].cpu_time;
        total += p->bursts[i].io_time;
    }
    p->min_ideal_time = total;
}
