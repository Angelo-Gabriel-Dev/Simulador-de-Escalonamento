#include <stdlib.h>
#include "priority.h"

typedef struct {
    Process **items;
    int count, capacity;
} PriorityState;

static void prio_on_arrival(SchedulerAlgorithm *self, Process *p, int now) {
    (void) now;
    PriorityState *st = (PriorityState *) self->internal_state;
    st->items[st->count++] = p;
}

/* true se `a` deve rodar antes de `b`: menor prioridade numérica primeiro;
 * empate -> menor arrival_time; empate residual -> menor pid. */
static int better(const Process *a, const Process *b) {
    if (a->priority != b->priority) return a->priority < b->priority;
    if (a->arrival_time != b->arrival_time) return a->arrival_time < b->arrival_time;
    return a->pid < b->pid;
}

static Process *prio_select_next(SchedulerAlgorithm *self, int now) {
    (void) now;
    PriorityState *st = (PriorityState *) self->internal_state;
    if (st->count == 0) return NULL;

    int best_idx = 0;
    for (int i = 1; i < st->count; i++) {
        if (better(st->items[i], st->items[best_idx])) {
            best_idx = i;
        }
    }
    Process *chosen = st->items[best_idx];
    /* remove via swap com o último (ordem do array é irrelevante aqui,
     * já que a escolha é sempre por varredura completa) */
    st->items[best_idx] = st->items[st->count - 1];
    st->count--;
    return chosen;
}

static void prio_on_quantum_check(SchedulerAlgorithm *self, Process *running, int now) {
    (void) self; (void) running; (void) now; /* não preemptivo: no-op */
}

static void prio_reset(SchedulerAlgorithm *self) {
    ((PriorityState *) self->internal_state)->count = 0;
}

SchedulerAlgorithm *priority_create(int n_processes) {
    SchedulerAlgorithm *algo = (SchedulerAlgorithm *) malloc(sizeof(SchedulerAlgorithm));
    PriorityState *st = (PriorityState *) malloc(sizeof(PriorityState));
    st->capacity = n_processes > 0 ? n_processes : 1;
    st->items = (Process **) malloc((size_t) st->capacity * sizeof(Process *));
    st->count = 0;

    algo->name = "Priority";
    algo->quantum = 0;
    algo->on_process_arrival = prio_on_arrival;
    algo->select_next = prio_select_next;
    algo->on_quantum_check = prio_on_quantum_check;
    algo->reset = prio_reset;
    algo->internal_state = st;
    return algo;
}

void priority_destroy(SchedulerAlgorithm *algo) {
    PriorityState *st = (PriorityState *) algo->internal_state;
    free(st->items);
    free(st);
    free(algo);
}

