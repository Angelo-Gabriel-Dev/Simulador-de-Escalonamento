#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "workload_generator.h"
#include "rng.h"

/*
 * Os 4 cenários obrigatórios (Seção 6 do enunciado) como presets nomeados.
 * Os mesmos números aparecem em config/scenarios.json para documentação e
 * para que os scripts Python rotulem gráficos/tabelas sem duplicar valores
 * "no achismo" — ver docs/decisoes_projeto.md para a justificativa de manter
 * os presets no C (evita depender de um parser de JSON em C para algo que
 * não precisa ser dado dinâmico em tempo de execução).
 */
int scenario_get_by_name(const char *name, ScenarioConfig *out) {
    memset(out, 0, sizeof(ScenarioConfig));

    if (strcmp(name, "balanced") == 0) {
        out->name = "balanced";
        out->min_bursts = 1;  out->max_bursts = 5;
        out->cpu_min = 2;     out->cpu_max = 20;
        out->io_min = 2;      out->io_max = 15;
        out->priority_min = 1; out->priority_max = 10;
        out->high_priority_fraction = 0.0;
        out->arrival_model = ARRIVAL_EXPONENTIAL;
        out->arrival_mean_interval = 40.0;
        return 1;
    }
    if (strcmp(name, "io_bound") == 0) {
        out->name = "io_bound";
        out->min_bursts = 3;  out->max_bursts = 8;
        out->cpu_min = 1;     out->cpu_max = 5;
        out->io_min = 3;      out->io_max = 12;
        out->priority_min = 1; out->priority_max = 10;
        out->high_priority_fraction = 0.0;
        out->arrival_model = ARRIVAL_EXPONENTIAL;
        out->arrival_mean_interval = 45.0;
        return 1;
    }
    if (strcmp(name, "cpu_bound") == 0) {
        out->name = "cpu_bound";
        out->min_bursts = 1;  out->max_bursts = 3;
        out->cpu_min = 10;    out->cpu_max = 40;
        out->io_min = 1;      out->io_max = 5;
        out->priority_min = 1; out->priority_max = 10;
        out->high_priority_fraction = 0.0;
        out->arrival_model = ARRIVAL_EXPONENTIAL;
        out->arrival_mean_interval = 60.0;
        return 1;
    }
    if (strcmp(name, "priority_skew") == 0) {
        out->name = "priority_skew";
        out->min_bursts = 1;  out->max_bursts = 5;
        out->cpu_min = 2;     out->cpu_max = 20;
        out->io_min = 2;      out->io_max = 15;
        out->priority_min = 1; out->priority_max = 10;
        out->high_priority_fraction = 0.85;
        out->high_priority_min = 1; out->high_priority_max = 3;
        out->low_priority_min = 8;  out->low_priority_max = 10;
        out->arrival_model = ARRIVAL_EXPONENTIAL;
        out->arrival_mean_interval = 40.0;
        return 1;
    }
    return 0;
}

static int draw_priority(const ScenarioConfig *cfg, RngState *rng) {
    if (cfg->high_priority_fraction > 0.0) {
        double u = rng_next_double(rng);
        if (u < cfg->high_priority_fraction) {
            return rng_range_int(rng, cfg->high_priority_min, cfg->high_priority_max);
        } else {
            return rng_range_int(rng, cfg->low_priority_min, cfg->low_priority_max);
        }
    }
    return rng_range_int(rng, cfg->priority_min, cfg->priority_max);
}

Process *generate_workload(const ScenarioConfig *cfg, uint64_t seed, int n_processes) {
    RngState rng;
    rng_seed(&rng, seed);

    Process *procs = (Process *) malloc((size_t) n_processes * sizeof(Process));

    int cumulative_time = 0;
    int batch_counter = 0;

    for (int i = 0; i < n_processes; i++) {
        /* --- tempo de chegada (mesma ordem de draw para todos os cenários,
         * de forma que a sequência de números aleatórios consumida seja
         * estável e fácil de auditar) --- */
        int arrival_time;
        switch (cfg->arrival_model) {
            case ARRIVAL_ALL_AT_ZERO:
                arrival_time = 0;
                break;
            case ARRIVAL_FIXED_INTERVAL:
                arrival_time = (int) (i * cfg->arrival_mean_interval);
                break;
            case ARRIVAL_BATCH: {
                int bsize = cfg->batch_size > 0 ? cfg->batch_size : 10;
                batch_counter = i / bsize;
                arrival_time = (int) (batch_counter * cfg->arrival_mean_interval);
                break;
            }
            case ARRIVAL_EXPONENTIAL:
            default:
                if (i == 0) {
                    arrival_time = 0;
                } else {
                    double gap = rng_exponential(&rng, cfg->arrival_mean_interval);
                    cumulative_time += (int) (gap + 0.5);
                    arrival_time = cumulative_time;
                }
                break;
        }

        int num_bursts = rng_range_int(&rng, cfg->min_bursts, cfg->max_bursts);
        if (num_bursts < 1) num_bursts = 1;

        int priority = draw_priority(cfg, &rng);

        process_init(&procs[i], i, arrival_time, priority, num_bursts);

        for (int j = 0; j < num_bursts; j++) {
            int cpu_t = rng_range_int(&rng, cfg->cpu_min, cfg->cpu_max);
            if (cpu_t < 1) cpu_t = 1;
            procs[i].bursts[j].cpu_time = cpu_t;

            if (j == num_bursts - 1) {
                procs[i].bursts[j].io_time = 0; /* última rajada nunca tem E/S depois */
            } else {
                int io_t = rng_range_int(&rng, cfg->io_min, cfg->io_max);
                if (io_t < 1) io_t = 1;
                procs[i].bursts[j].io_time = io_t;
            }
        }

        process_compute_min_ideal(&procs[i]);
        procs[i].remaining_time = procs[i].bursts[0].cpu_time;
    }

    return procs;
}

void workload_free(Process *procs, int n_processes) {
    for (int i = 0; i < n_processes; i++) {
        process_free(&procs[i]);
    }
    free(procs);
}
