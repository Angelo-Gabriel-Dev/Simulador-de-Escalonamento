#ifndef PROCESS_H
#define PROCESS_H

/*
 * Modelo de processo do simulador.
 *
 * Convenção de prioridade: MENOR valor numérico = MAIOR prioridade
 * (mesma convenção do `nice` do Unix). Ver docs/modelagem.md.
 */

typedef enum {
    PROC_NEW,
    PROC_READY,
    PROC_RUNNING,
    PROC_BLOCKED,
    PROC_TERMINATED
} ProcessState;

/* Uma rajada: tempo de CPU seguido (opcionalmente) de uma requisição de E/S.
 * io_time == 0 significa "sem E/S após esta rajada" — por construção do
 * gerador de cargas, isso só acontece na ÚLTIMA rajada do processo. */
typedef struct {
    int cpu_time;
    int io_time;
} Burst;

typedef struct {
    int pid;
    int arrival_time;
    int priority;           /* menor valor = maior prioridade */
    int num_bursts;
    Burst *bursts;           /* sequência CPU -> E/S -> CPU -> E/S -> ... -> CPU */
    int current_burst_index;
    int remaining_time;      /* tempo restante da rajada ATUAL (CPU ou E/S) */
    ProcessState state;

    /* usado pelo algoritmo próprio: quando o processo entrou na fila de
     * prontos pela última vez (para calcular tempo de espera / aging) e
     * a estimativa corrente (EMA) da próxima rajada de CPU. Mantidos aqui
     * (em vez de só no internal_state do algoritmo) porque precisam
     * sobreviver entre chamadas de on_process_arrival/select_next. */
    int ready_since;
    double estimated_next_burst;

    /* métricas coletadas ao longo da simulação */
    int start_time;          /* primeira vez que rodou na CPU; -1 se nunca rodou */
    int completion_time;
    int waiting_time;        /* turnaround - min_ideal_time (overhead de espera/troca) */
    int turnaround_time;
    long min_ideal_time;     /* soma de todos os cpu_time + io_time (tempo mínimo ideal) */
    double slowdown;
} Process;

/* Aloca `bursts` e preenche pid/arrival/priority/estado inicial. */
void process_init(Process *p, int pid, int arrival_time, int priority,
                   int num_bursts);

void process_free(Process *p);

/* Soma cpu_time + io_time de todas as rajadas -> min_ideal_time. Deve ser
 * chamado depois que os valores de Burst já foram preenchidos pelo gerador. */
void process_compute_min_ideal(Process *p);

#endif /* PROCESS_H */
