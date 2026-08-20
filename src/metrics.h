#ifndef METRICS_H
#define METRICS_H

#include "process.h"

typedef struct {
    double avg_turnaround;
    long total_context_switches;
    double jain_slowdown;    /* em % (0-100) */
    double avg_slowdown;     /* métrica auxiliar, não obrigatória, mas útil na discussão */
    int n_processes;

    /* Quebra por classe de prioridade (alta: priority<=3, baixa: priority>=8,
     * mesmos limiares usados no preset priority_skew) -- usada para checar
     * se um algoritmo realmente DIFERENCIA processos por prioridade ou nao.
     * n_high/n_low podem ser 0 em cenarios sem processos nessas faixas. */
    double avg_turnaround_high_priority;
    double avg_turnaround_low_priority;
    int n_high_priority;
    int n_low_priority;
} RunMetrics;

/* Calcula as métricas de UMA execução (uma seed) a partir dos processos já
 * finalizados. Espera que todo processo em `procs` tenha state ==
 * PROC_TERMINATED e slowdown_i já preenchido (ver metrics_finalize_process). */
void metrics_compute(const Process *procs, int n, long total_context_switches, RunMetrics *out);

/* Preenche completion_time/turnaround_time/waiting_time/slowdown de um
 * processo que acabou de terminar (chamado pelo motor de simulação). */
void metrics_finalize_process(Process *p, int completion_time);

#endif /* METRICS_H */
