#include <stdlib.h>
#include "round_robin.h"

typedef struct {
    Process **queue;
    int head, tail, count, capacity;
} RrState;

static void rr_enqueue(RrState *st, Process *p) {
    st->queue[st->tail] = p;
    st->tail = (st->tail + 1) % st->capacity;
    st->count++;
}

static void rr_on_arrival(SchedulerAlgorithm *self, Process *p, int now) {
    (void) now;
    rr_enqueue((RrState *) self->internal_state, p);
}

static Process *rr_select_next(SchedulerAlgorithm *self, int now) {
    (void) now;
    RrState *st = (RrState *) self->internal_state;
    if (st->count == 0) return NULL;
    Process *p = st->queue[st->head];
    st->head = (st->head + 1) % st->capacity;
    st->count--;
    return p;
}

static void rr_on_quantum_check(SchedulerAlgorithm *self, Process *running, int now) {
    (void) now;
    /* Quantum expirou e o processo ainda tem trabalho restante na rajada
     * atual (isso já foi garantido por main.c antes de agendar o evento):
     * volta para o final da fila. */
    rr_enqueue((RrState *) self->internal_state, running);
}

static void rr_reset(SchedulerAlgorithm *self) {
    RrState *st = (RrState *) self->internal_state;
    st->head = st->tail = st->count = 0;
}

SchedulerAlgorithm *round_robin_create(int n_processes, int quantum) {
    SchedulerAlgorithm *algo = (SchedulerAlgorithm *) malloc(sizeof(SchedulerAlgorithm));
    RrState *st = (RrState *) malloc(sizeof(RrState));
    st->capacity = n_processes > 0 ? n_processes : 1;
    st->queue = (Process **) malloc((size_t) st->capacity * sizeof(Process *));
    st->head = st->tail = st->count = 0;

    algo->name = "RoundRobin";
    algo->quantum = quantum > 0 ? quantum : 4; /* padrão sugerido = 4 */
    algo->on_process_arrival = rr_on_arrival;
    algo->select_next = rr_select_next;
    algo->on_quantum_check = rr_on_quantum_check;
    algo->reset = rr_reset;
    algo->internal_state = st;
    return algo;
}

void round_robin_destroy(SchedulerAlgorithm *algo) {
    RrState *st = (RrState *) algo->internal_state;
    free(st->queue);
    free(st);
    free(algo);
}

