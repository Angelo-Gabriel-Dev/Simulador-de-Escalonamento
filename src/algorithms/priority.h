#ifndef PRIORITY_H
#define PRIORITY_H

#include "../scheduler.h"

/* Escalonamento por prioridade não preemptivo: sempre escolhe o processo
 * de MENOR valor numérico de prioridade (= maior prioridade) disponível na
 * fila de prontos; não interrompe quem já está executando. Desempate:
 * menor arrival_time, depois menor pid. */
SchedulerAlgorithm *priority_create(int n_processes);
void priority_destroy(SchedulerAlgorithm *algo);

#endif /* PRIORITY_H */

