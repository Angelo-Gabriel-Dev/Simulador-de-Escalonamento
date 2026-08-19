#include <stdlib.h>
#include "fcfs.h"

typedef struct {
    Process **queue;
    int head, tail, count, capacity;
} FcfsState;

static void fcfs_on_arrival(SchedulerAlgorithm *self, Process *p, int now) {
    (void) now;
    FcfsState *st = (FcfsState *) self->internal_state;
    st->queue[st->tail] = p;
    st->tail = (st->tail + 1) % st->capacity;
    st->count++;
}

static Process *fcfs_select_next(SchedulerAlgorithm *self, int now) {
    (void) now;
    FcfsState *st = (FcfsState *) self->internal_state;
    if (st->count == 0) return NULL;
    Process *p = st->queue[st->head];
    st->head = (st->head + 1) % st->capacity;
    st->count--;
    return p;
}

static void fcfs_on_quantum_check(SchedulerAlgorithm *self, Process *running, int now) {
    (void) self; (void) running; (void) now; /* FCFS não é preemptivo: no-op */
}

static void fcfs_reset(SchedulerAlgorithm *self) {
    FcfsState *st = (FcfsState *) self->internal_state;
    st->head = st->tail = st->count = 0;
}

SchedulerAlgorithm *fcfs_create(int n_processes) {
    SchedulerAlgorithm *algo = (SchedulerAlgorithm *) malloc(sizeof(SchedulerAlgorithm));
    FcfsState *st = (FcfsState *) malloc(sizeof(FcfsState));
    st->capacity = n_processes > 0 ? n_processes : 1;
    st->queue = (Process **) malloc((size_t) st->capacity * sizeof(Process *));
    st->head = st->tail = st->count = 0;

    algo->name = "FCFS";
    algo->quantum = 0;
    algo->on_process_arrival = fcfs_on_arrival;
    algo->select_next = fcfs_select_next;
    algo->on_quantum_check = fcfs_on_quantum_check;
    algo->reset = fcfs_reset;
    algo->internal_state = st;
    return algo;
}

void fcfs_destroy(SchedulerAlgorithm *algo) {
    FcfsState *st = (FcfsState *) algo->internal_state;
    free(st->queue);
    free(st);
    free(algo);
}

