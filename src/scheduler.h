#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"

/*
 * Interface comum a todos os algoritmos de escalonamento (Seção 11 do
 * prompt mestre). Cada algoritmo implementa estes callbacks e guarda seu
 * próprio estado (fila de prontos, quantum, EMA de rajadas etc.) em
 * `internal_state`.
 *
 * Decisão de projeto (registrada também em docs/decisoes_projeto.md):
 * `on_process_arrival` é chamado toda vez que um processo ENTRA na fila
 * de prontos — tanto em uma chegada nova (EVENT_ARRIVAL) quanto em um
 * retorno de E/S (EVENT_IO_DONE). O enunciado original só menciona
 * "chegada", mas do ponto de vista do escalonador as duas situações são
 * idênticas: um processo passou a estar apto a usar a CPU agora. Usar o
 * mesmo hook para as duas evita duplicar lógica de fila em cada algoritmo.
 *
 * O campo `quantum` é uma extensão pequena e deliberada sobre o contrato
 * mínimo do enunciado: permite que main.c decida, de forma genérica e sem
 * checar o nome do algoritmo, se deve armar um temporizador de quantum
 * (RR) ou não (algoritmos não-preemptivos, quantum = 0).
 */
typedef struct SchedulerAlgorithm {
    const char *name;
    int quantum; /* 0 = não-preemptivo / não usa quantum */

    /* Processo passou a estar pronto para executar (chegada OU retorno de E/S). */
    void (*on_process_arrival)(struct SchedulerAlgorithm *self, Process *p, int now);

    /* Escolhe e REMOVE o próximo processo a rodar da estrutura interna.
     * Retorna NULL se a fila de prontos estiver vazia (CPU fica ociosa). */
    Process *(*select_next)(struct SchedulerAlgorithm *self, int now);

    /* Chamado quando o temporizador de quantum do processo em execução
     * expira. Só tem efeito real no Round Robin (decide se re-enfileira
     * o processo); no-op nos demais algoritmos. */
    void (*on_quantum_check)(struct SchedulerAlgorithm *self, Process *running, int now);

    /* Reinicia o estado interno (usado nos testes unitários). */
    void (*reset)(struct SchedulerAlgorithm *self);

    void *internal_state;
} SchedulerAlgorithm;

#endif /* SCHEDULER_H */
