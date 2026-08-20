#include <stdio.h>
#include <stdlib.h>
#include "process.h"
#include "algorithms/fcfs.h"
#include "algorithms/round_robin.h"
#include "algorithms/priority.h"

static void check(int cond, const char *msg) {
    if (!cond) {
        fprintf(stderr, "FALHOU: %s\n", msg);
        exit(1);
    }
    printf("OK: %s\n", msg);
}

static Process make_proc(int pid, int arrival, int priority) {
    Process p;
    process_init(&p, pid, arrival, priority, 1);
    p.bursts[0].cpu_time = 10;
    p.bursts[0].io_time = 0;
    return p;
}

static void test_fcfs(void) {
    SchedulerAlgorithm *algo = fcfs_create(10);
    check(algo->quantum == 0, "FCFS: quantum == 0 (nao preemptivo)");

    Process p0 = make_proc(0, 0, 5);
    Process p1 = make_proc(1, 1, 1); /* prioridade "melhor" mas chegou depois: FCFS ignora prioridade */
    Process p2 = make_proc(2, 2, 9);

    algo->on_process_arrival(algo, &p0, 0);
    algo->on_process_arrival(algo, &p1, 1);
    algo->on_process_arrival(algo, &p2, 2);

    Process *first = algo->select_next(algo, 5);
    Process *second = algo->select_next(algo, 5);
    Process *third = algo->select_next(algo, 5);
    Process *empty = algo->select_next(algo, 5);

    check(first->pid == 0 && second->pid == 1 && third->pid == 2,
          "FCFS: ordem de saida = ordem de entrada na fila de prontos, ignorando prioridade");
    check(empty == NULL, "FCFS: fila vazia retorna NULL");

    process_free(&p0); process_free(&p1); process_free(&p2);
    fcfs_destroy(algo);
}

static void test_priority(void) {
    SchedulerAlgorithm *algo = priority_create(10);

    Process p0 = make_proc(0, 0, 5);
    Process p1 = make_proc(1, 1, 1); /* menor valor = maior prioridade: deve sair primeiro */
    Process p2 = make_proc(2, 2, 9);
    Process p3 = make_proc(3, 3, 1); /* empate de prioridade com p1, mas chegou depois -> desempate por arrival_time */

    algo->on_process_arrival(algo, &p0, 0);
    algo->on_process_arrival(algo, &p1, 1);
    algo->on_process_arrival(algo, &p2, 2);
    algo->on_process_arrival(algo, &p3, 3);

    Process *first = algo->select_next(algo, 10);
    Process *second = algo->select_next(algo, 10);
    Process *third = algo->select_next(algo, 10);
    Process *fourth = algo->select_next(algo, 10);

    check(first->pid == 1, "Priority: processo de menor valor numerico (maior prioridade) sai primeiro");
    check(second->pid == 3, "Priority: empate de prioridade desempatado por menor arrival_time");
    check(third->pid == 0 && fourth->pid == 2, "Priority: restante em ordem crescente de prioridade");

    process_free(&p0); process_free(&p1); process_free(&p2); process_free(&p3);
    priority_destroy(algo);
}

static void test_round_robin_requeue(void) {
    SchedulerAlgorithm *algo = round_robin_create(10, 4);
    check(algo->quantum == 4, "RR: quantum configurado corretamente (4)");

    Process p0 = make_proc(0, 0, 5);
    Process p1 = make_proc(1, 0, 5);
    algo->on_process_arrival(algo, &p0, 0);
    algo->on_process_arrival(algo, &p1, 0);

    Process *first = algo->select_next(algo, 0);
    check(first->pid == 0, "RR: primeiro da fila sai primeiro (FIFO)");

    /* simula quantum de p0 expirando: main.c chamaria on_quantum_check,
     * devolvendo p0 ao FINAL da fila (atras de p1, que ja estava esperando) */
    algo->on_quantum_check(algo, first, 4);

    Process *second = algo->select_next(algo, 4);
    check(second->pid == 1, "RR: apos preempcao, quem estava esperando (p1) roda antes do preemptado (p0)");

    Process *third = algo->select_next(algo, 8);
    check(third->pid == 0, "RR: processo preemptado (p0) volta ao final da fila circular e roda depois");

    process_free(&p0); process_free(&p1);
    round_robin_destroy(algo);
}

static void test_round_robin_default_quantum(void) {
    SchedulerAlgorithm *algo = round_robin_create(5, 0); /* 0 -> deve cair no padrao sugerido (4) */
    check(algo->quantum == 4, "RR: quantum <= 0 no create cai para o padrao sugerido (4)");
    round_robin_destroy(algo);
}

int main(void) {
    test_fcfs();
    test_priority();
    test_round_robin_requeue();
    test_round_robin_default_quantum();
    printf("Todos os testes dos algoritmos classicos passaram.\n");
    return 0;
}

