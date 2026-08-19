#include "metrics.h"

void metrics_finalize_process(Process *p, int completion_time) {
    p->completion_time = completion_time;
    p->turnaround_time = completion_time - p->arrival_time;
    p->waiting_time = p->turnaround_time - (int) p->min_ideal_time;
    if (p->min_ideal_time > 0) {
        p->slowdown = (double) p->turnaround_time / (double) p->min_ideal_time;
    } else {
        /* min_ideal_time só seria 0 se todas as rajadas tivessem duração 0,
         * o que o gerador de cargas nunca produz (cpu_time >= 1 sempre). */
        p->slowdown = 1.0;
    }
}

void metrics_compute(const Process *procs, int n, long total_context_switches, RunMetrics *out) {
    double sum_turnaround = 0.0;
    double sum_slowdown = 0.0;
    double sum_slowdown_sq = 0.0;

    double sum_turnaround_high = 0.0;
    double sum_turnaround_low = 0.0;
    int n_high = 0, n_low = 0;

    for (int i = 0; i < n; i++) {
        sum_turnaround += procs[i].turnaround_time;
        sum_slowdown += procs[i].slowdown;
        sum_slowdown_sq += procs[i].slowdown * procs[i].slowdown;

        /* mesmos limiares do preset priority_skew (ver workload_generator.c) */
        if (procs[i].priority <= 3) {
            sum_turnaround_high += procs[i].turnaround_time;
            n_high++;
        } else if (procs[i].priority >= 8) {
            sum_turnaround_low += procs[i].turnaround_time;
            n_low++;
        }
    }

    out->n_processes = n;
    out->total_context_switches = total_context_switches;
    out->avg_turnaround = (n > 0) ? sum_turnaround / n : 0.0;
    out->avg_slowdown = (n > 0) ? sum_slowdown / n : 0.0;

    out->n_high_priority = n_high;
    out->n_low_priority = n_low;
    out->avg_turnaround_high_priority = (n_high > 0) ? sum_turnaround_high / n_high : 0.0;
    out->avg_turnaround_low_priority = (n_low > 0) ? sum_turnaround_low / n_low : 0.0;

    /* Índice de Jain do slowdown: (sum x_i)^2 / (n * sum x_i^2) * 100 */
    if (n > 0 && sum_slowdown_sq > 0.0) {
        out->jain_slowdown = (sum_slowdown * sum_slowdown) / (n * sum_slowdown_sq) * 100.0;
    } else {
        out->jain_slowdown = 100.0; /* caso degenerado (n=0 ou todos slowdown=0) */
    }
}
