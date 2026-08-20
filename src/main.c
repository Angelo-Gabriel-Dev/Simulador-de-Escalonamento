#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "process.h"
#include "rng.h"
#include "event_queue.h"
#include "workload_generator.h"
#include "io_manager.h"
#include "context_switch.h"
#include "scheduler.h"
#include "metrics.h"
#include "stats.h"
#include "cli.h"
#include "algorithms/fcfs.h"
#include "algorithms/round_robin.h"
#include "algorithms/priority.h"
#include "algorithms/custom_algo.h"

typedef struct {
    SimConfig cfg;
    Process *procs;
    int n;
    EventQueue eq;
    IoManager iom;
    ContextSwitchState cs;
    SchedulerAlgorithm *algo;

    int cpu_busy;
    int cpu_switching;
    long context_switch_count;
    int terminated_count;
} SimState;

static void start_running(SimState *S, int idx, int now) {
    Process *p = &S->procs[idx];
    if (p->start_time == -1) p->start_time = now;
    p->state = PROC_RUNNING;
    S->cpu_busy = 1;
    cs_set_running(&S->cs, p->pid);

    int rem = p->remaining_time;
    if (S->algo->quantum > 0 && rem > S->algo->quantum) {
        eq_push(&S->eq, now + S->algo->quantum, EVENT_QUANTUM_EXPIRED, idx, -1);
    } else {
        eq_push(&S->eq, now + rem, EVENT_CPU_BURST_DONE, idx, -1);
    }
}

static void try_dispatch(SimState *S, int now) {
    if (S->cpu_busy || S->cpu_switching) return;

    Process *next = S->algo->select_next(S->algo, now);
    if (next == NULL) {
        cs_mark_idle(&S->cs);
        return;
    }
    int idx = next->pid; /* pid == índice no array procs, ver workload_generator.c */

    if (cs_needs_switch(&S->cs, next->pid)) {
        S->context_switch_count++;
        if (S->cfg.context_switch_cost > 0) {
            S->cpu_switching = 1;
            eq_push(&S->eq, now + S->cfg.context_switch_cost, EVENT_CONTEXT_SWITCH_DONE, idx, -1);
        } else {
            start_running(S, idx, now);
        }
    } else {
        start_running(S, idx, now);
    }
}

static SchedulerAlgorithm *create_algorithm(const char *name, int n, const SimConfig *cfg) {
    if (strcmp(name, "fcfs") == 0) return fcfs_create(n);
    if (strcmp(name, "rr") == 0) return round_robin_create(n, cfg->quantum);
    if (strcmp(name, "priority") == 0) return priority_create(n);
    if (strcmp(name, "custom") == 0)
        return custom_algo_create(n, cfg->k1, cfg->k2, cfg->initial_burst_estimate);
    return NULL;
}

static void destroy_algorithm(const char *name, SchedulerAlgorithm *algo) {
    if (strcmp(name, "fcfs") == 0) { fcfs_destroy(algo); return; }
    if (strcmp(name, "rr") == 0) { round_robin_destroy(algo); return; }
    if (strcmp(name, "priority") == 0) { priority_destroy(algo); return; }
    if (strcmp(name, "custom") == 0) { custom_algo_destroy(algo); return; }
}

static void run_event_loop(SimState *S) {
    /* Pré-carrega todos os eventos de chegada, em ordem de pid (que já é
     * ordem não-decrescente de arrival_time por construção do gerador de
     * cargas). Isso NÃO viola a regra de "sem informação futura" para o
     * algoritmo: são eventos exógenos/ambientais na fila de eventos, não
     * dados visíveis ao algoritmo — cada algoritmo só enxerga um processo
     * via on_process_arrival quando o evento correspondente é processado
     * no instante `now` certo. */
    for (int i = 0; i < S->n; i++) {
        eq_push(&S->eq, S->procs[i].arrival_time, EVENT_ARRIVAL, i, -1);
    }

    while (!eq_empty(&S->eq)) {
        Event e = eq_pop(&S->eq);
        int now = e.time;
        Process *p = &S->procs[e.process_index];

        switch (e.type) {
            case EVENT_ARRIVAL: {
                p->state = PROC_READY;
                S->algo->on_process_arrival(S->algo, p, now);
                try_dispatch(S, now);
                break;
            }
            case EVENT_CONTEXT_SWITCH_DONE: {
                S->cpu_switching = 0;
                start_running(S, e.process_index, now);
                break;
            }
            case EVENT_QUANTUM_EXPIRED: {
                p->remaining_time -= S->algo->quantum;
                p->state = PROC_READY;
                S->cpu_busy = 0;
                S->algo->on_quantum_check(S->algo, p, now);
                try_dispatch(S, now);
                break;
            }
            case EVENT_CPU_BURST_DONE: {
                int actual_cpu_time = p->bursts[p->current_burst_index].cpu_time;
                custom_algo_observe_completed_burst(p, actual_cpu_time, S->cfg.ema_alpha);

                S->cpu_busy = 0;

                if (p->bursts[p->current_burst_index].io_time > 0) {
                    p->state = PROC_BLOCKED;
                    int started = 0;
                    int device = io_manager_request(&S->iom, e.process_index, &started);
                    if (started) {
                        int io_dur = p->bursts[p->current_burst_index].io_time;
                        eq_push(&S->eq, now + io_dur, EVENT_IO_DONE, e.process_index, device);
                    }
                } else {
                    p->state = PROC_TERMINATED;
                    metrics_finalize_process(p, now);
                    S->terminated_count++;
                }
                try_dispatch(S, now);
                break;
            }
            case EVENT_IO_DONE: {
                int next_proc = -1;
                io_manager_service_done(&S->iom, e.device_id, &next_proc);
                if (next_proc != -1) {
                    Process *np = &S->procs[next_proc];
                    int io_dur = np->bursts[np->current_burst_index].io_time;
                    eq_push(&S->eq, now + io_dur, EVENT_IO_DONE, next_proc, e.device_id);
                }

                p->current_burst_index++;
                p->remaining_time = p->bursts[p->current_burst_index].cpu_time;
                p->state = PROC_READY;
                S->algo->on_process_arrival(S->algo, p, now);
                try_dispatch(S, now);
                break;
            }
        }
    }
}

int main(int argc, char **argv) {
    SimConfig cfg;
    if (!cli_parse(argc, argv, &cfg)) {
        return 1;
    }

    const char *header =
        "scenario,algorithm,seed,n_processes,quantum,context_switch_cost,io_devices,"
        "k1,k2,avg_turnaround,context_switches,jain_slowdown,avg_slowdown,"
        "avg_turnaround_high_priority,avg_turnaround_low_priority\n";

    if (cfg.print_header) {
        printf("%s", header);
        return 0;
    }

    ScenarioConfig scen;
    if (!scenario_get_by_name(cfg.scenario, &scen)) {
        fprintf(stderr, "Erro: cenário desconhecido '%s'\n", cfg.scenario);
        return 1;
    }

    SchedulerAlgorithm *algo = create_algorithm(cfg.algorithm, cfg.n_processes, &cfg);
    if (algo == NULL) {
        fprintf(stderr, "Erro: algoritmo desconhecido '%s'\n", cfg.algorithm);
        return 1;
    }

    SimState S;
    S.cfg = cfg;
    S.n = cfg.n_processes;
    S.procs = generate_workload(&scen, cfg.seed, cfg.n_processes);
    eq_init(&S.eq, 4 * cfg.n_processes);
    io_manager_init(&S.iom, cfg.io_devices);
    cs_init(&S.cs, cfg.context_switch_cost);
    S.algo = algo;
    S.cpu_busy = 0;
    S.cpu_switching = 0;
    S.context_switch_count = 0;
    S.terminated_count = 0;

    run_event_loop(&S);

    assert(S.terminated_count == S.n);

    RunMetrics rm;
    metrics_compute(S.procs, S.n, S.context_switch_count, &rm);

    if (cfg.verbose) {
        fprintf(stderr,
            "[%s/%s seed=%llu] n=%d turnaround_medio=%.3f trocas_contexto=%ld "
            "jain_slowdown=%.2f%% slowdown_medio=%.3f (k1=%.4f k2=%.4f)\n",
            cfg.scenario, cfg.algorithm, (unsigned long long) cfg.seed, rm.n_processes,
            rm.avg_turnaround, rm.total_context_switches, rm.jain_slowdown, rm.avg_slowdown,
            cfg.k1, cfg.k2);
    }

    printf("%s,%s,%llu,%d,%d,%d,%d,%.6f,%.6f,%.6f,%ld,%.6f,%.6f,%.6f,%.6f\n",
           cfg.scenario, cfg.algorithm, (unsigned long long) cfg.seed, cfg.n_processes,
           cfg.quantum, cfg.context_switch_cost, cfg.io_devices, cfg.k1, cfg.k2,
           rm.avg_turnaround, rm.total_context_switches, rm.jain_slowdown, rm.avg_slowdown,
           rm.avg_turnaround_high_priority, rm.avg_turnaround_low_priority);

    workload_free(S.procs, S.n);
    eq_free(&S.eq);
    io_manager_free(&S.iom);
    destroy_algorithm(cfg.algorithm, algo);

    return 0;
}
