#ifndef ROUND_ROBIN_H
#define ROUND_ROBIN_H

#include "../scheduler.h"

/* Round Robin com quantum configurável. Fila circular FIFO idêntica à do
 * FCFS; a diferença é que o processo em execução é preemptado quando seu
 * quantum expira e devolvido ao FINAL da fila (main.c agenda
 * EVENT_QUANTUM_EXPIRED sempre que remaining_time > quantum no despacho). */
SchedulerAlgorithm *round_robin_create(int n_processes, int quantum);
void round_robin_destroy(SchedulerAlgorithm *algo);

#endif /* ROUND_ROBIN_H */

