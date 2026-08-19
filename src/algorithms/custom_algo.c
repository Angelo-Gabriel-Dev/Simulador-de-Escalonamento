#include <stdlib.h>
#include "custom_algo.h"

typedef struct {
    Process **items;
    int count, capacity;
    double k1, k2;
    double initial_estimate;
} CustomState;

static double estimated_burst_of(const CustomState *st, const Process *p) {
    /* -1.0 é o sentinela de "ainda sem rajada observada" (ver process.c) */
    if (p->estimated_next_burst < 0.0) return st->initial_estimate;
    return p->estimated_next_burst;
}

static double effective_priority(const CustomState *st, const Process *p, int now) {
    double waited = (double) (now - p->ready_since);
    double burst_est = estimated_burst_of(st, p);
    return (double) p->priority - st->k1 * waited + st->k2 * burst_est;
}

static void custom_on_arrival(SchedulerAlgorithm *self, Process *p, int now) {
    CustomState *st = (CustomState *) self->internal_state;
    p->ready_since = now; /* reinicia contagem de espera/aging a cada vez que fica pronto */
    st->items[st->count++] = p;
}

static Process *custom_select_next(SchedulerAlgorithm *self, int now) {
    CustomState *st = (CustomState *) self->internal_state;
    if (st->count == 0) return NULL;

    int best_idx = 0;
    double best_eff = effective_priority(st, st->items[0], now);
    for (int i = 1; i < st->count; i++) {
        double eff = effective_priority(st, st->items[i], now);
        int is_better;
        if (eff != best_eff) {
            is_better = eff < best_eff;
        } else if (st->items[i]->ready_since != st->items[best_idx]->ready_since) {
            is_better = st->items[i]->ready_since < st->items[best_idx]->ready_since;
        } else {
            is_better = st->items[i]->pid < st->items[best_idx]->pid;
        }
        if (is_better) {
            best_idx = i;
            best_eff = eff;
        }
    }

    Process *chosen = st->items[best_idx];
    st->items[best_idx] = st->items[st->count - 1];
    st->count--;
    return chosen;
}

static void custom_on_quantum_check(SchedulerAlgorithm *self, Process *running, int now) {
    (void) self; (void) running; (void) now; /* não preemptivo: no-op */
}

static void custom_reset(SchedulerAlgorithm *self) {
    ((CustomState *) self->internal_state)->count = 0;
}

SchedulerAlgorithm *custom_algo_create(int n_processes, double k1, double k2,
                                        double initial_estimate) {
    SchedulerAlgorithm *algo = (SchedulerAlgorithm *) malloc(sizeof(SchedulerAlgorithm));
    CustomState *st = (CustomState *) malloc(sizeof(CustomState));
    st->capacity = n_processes > 0 ? n_processes : 1;
    st->items = (Process **) malloc((size_t) st->capacity * sizeof(Process *));
    st->count = 0;
    st->k1 = k1;
    st->k2 = k2;
    st->initial_estimate = initial_estimate;

    algo->name = "AdaptiveAgingEstimatedBurst";
    algo->quantum = 0;
    algo->on_process_arrival = custom_on_arrival;
    algo->select_next = custom_select_next;
    algo->on_quantum_check = custom_on_quantum_check;
    algo->reset = custom_reset;
    algo->internal_state = st;
    return algo;
}

void custom_algo_destroy(SchedulerAlgorithm *algo) {
    CustomState *st = (CustomState *) algo->internal_state;
    free(st->items);
    free(st);
    free(algo);
}

void custom_algo_observe_completed_burst(Process *p, int actual_cpu_time, double alpha) {
    if (p->estimated_next_burst < 0.0) {
        /* primeira observação: inicializa a EMA diretamente com o valor
         * observado, em vez de misturar com um "initial_estimate" que o
         * próprio algoritmo já usou como fallback para essa decisão. */
        p->estimated_next_burst = (double) actual_cpu_time;
    } else {
        p->estimated_next_burst = alpha * (double) actual_cpu_time
                                   + (1.0 - alpha) * p->estimated_next_burst;
    }
}
