#ifndef CUSTOM_ALGO_H
#define CUSTOM_ALGO_H

#include "../scheduler.h"

/*
 * Algoritmo próprio: "Prioridade Dinâmica com Estimativa de Rajada e
 * Envelhecimento" (Adaptive Aging + Estimated Burst). Ver
 * docs/algoritmo_proprio.md para motivação, inspiração e limitações
 * completas (Seção 7.4 do prompt mestre).
 *
 * prioridade_efetiva(p, now) = prioridade_estatica(p)
 *                               - k1 * tempo_de_espera(p, now)
 *                               + k2 * rajada_estimada(p)
 * (menor valor => executa primeiro). Não preemptivo: a cada CPU livre,
 * escolhe o processo pronto com menor prioridade_efetiva no instante
 * `now`; quem já está rodando não é interrompido.
 *
 * rajada_estimada(p) é uma média móvel exponencial (EMA) das rajadas de
 * CPU JÁ CONCLUÍDAS daquele mesmo processo (nunca a duração real da
 * rajada atual/futura — só histórico observado, ver Seção 2 do prompt
 * mestre sobre não usar informação futura). Antes da primeira rajada
 * concluída, usa `initial_estimate` (hiperparâmetro do algoritmo).
 */
SchedulerAlgorithm *custom_algo_create(int n_processes, double k1, double k2,
                                        double initial_estimate);
void custom_algo_destroy(SchedulerAlgorithm *algo);

/* Chamado por main.c toda vez que UMA rajada de CPU de `p` termina de
 * fato (para qualquer algoritmo — é só bookkeeping no processo, inofensivo
 * para quem não olha o campo), atualizando p->estimated_next_burst via
 * EMA: novo = alpha*observado + (1-alpha)*antigo. */
void custom_algo_observe_completed_burst(Process *p, int actual_cpu_time, double alpha);

#endif /* CUSTOM_ALGO_H */
