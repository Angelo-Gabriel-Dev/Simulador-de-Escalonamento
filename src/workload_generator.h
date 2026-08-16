#ifndef WORKLOAD_GENERATOR_H
#define WORKLOAD_GENERATOR_H

#include <stdint.h>
#include "process.h"

typedef enum {
    ARRIVAL_ALL_AT_ZERO,
    ARRIVAL_FIXED_INTERVAL,
    ARRIVAL_EXPONENTIAL,
    ARRIVAL_BATCH
} ArrivalModel;

/*
 * Parâmetros de um cenário. Os 4 cenários obrigatórios (Seção 6) são
 * expostos como presets nomeados em workload_generator.c e documentados,
 * com os mesmos valores, em config/scenarios.json (ver docs/decisoes_projeto.md
 * para a justificativa de por que os presets vivem no C em vez de serem
 * lidos de JSON em tempo de execução).
 */
typedef struct {
    const char *name;

    int min_bursts, max_bursts;   /* nº de rajadas de CPU por processo (uniforme) */
    int cpu_min, cpu_max;         /* duração de cada rajada de CPU (uniforme) */
    int io_min, io_max;           /* duração de cada requisição de E/S (uniforme) */

    int priority_min, priority_max;      /* faixa geral de prioridade */
    double high_priority_fraction;        /* fração de processos "prioridade alta" */
    int high_priority_min, high_priority_max;
    int low_priority_min, low_priority_max;

    ArrivalModel arrival_model;
    double arrival_mean_interval; /* média do intervalo entre chegadas (exponencial/fixo) */
    int batch_size;                /* usado só em ARRIVAL_BATCH */
} ScenarioConfig;

/* Retorna o preset de configuração para um dos 4 cenários obrigatórios.
 * Nomes aceitos: "balanced", "io_bound", "cpu_bound", "priority_skew".
 * Retorna 1 em sucesso, 0 se o nome não for reconhecido. */
int scenario_get_by_name(const char *name, ScenarioConfig *out);

/* Gera `n_processes` processos para o cenário `cfg`, deterministicamente a
 * partir de `seed` (mesma seed + mesmo cenário + mesmo n => mesma carga,
 * sempre). O array retornado é alocado com malloc; o chamador é dono da
 * memória (usar workload_free). */
Process *generate_workload(const ScenarioConfig *cfg, uint64_t seed, int n_processes);

void workload_free(Process *procs, int n_processes);

#endif /* WORKLOAD_GENERATOR_H */
