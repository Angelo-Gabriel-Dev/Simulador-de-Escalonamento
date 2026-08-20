#ifndef FCFS_H
#define FCFS_H

#include "../scheduler.h"

/* First-Come, First-Served: fila FIFO simples por ordem de entrada na fila
 * de prontos (nova chegada OU retorno de E/S — ver scheduler.h). Critério
 * de desempate para chegadas simultâneas: ordem de inserção na fila de
 * eventos, que por construção preserva menor arrival_time e, em empate
 * residual, menor pid (ver docs/modelagem.md). */
SchedulerAlgorithm *fcfs_create(int n_processes);
void fcfs_destroy(SchedulerAlgorithm *algo);

#endif /* FCFS_H */

