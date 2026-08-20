#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cli.h"

void cli_print_usage(const char *prog_name) {
    fprintf(stderr,
        "Uso: %s --scenario <balanced|io_bound|cpu_bound|priority_skew> \\\n"
        "        --algorithm <fcfs|rr|priority|custom> --seed <N> [opções]\n\n"
        "Opções:\n"
        "  --scenario NOME            (obrigatório)\n"
        "  --algorithm NOME           (obrigatório)\n"
        "  --seed N                   (obrigatório) seed de 64 bits\n"
        "  --n-processes N            padrão 1000\n"
        "  --quantum N                padrão 4 (só usado pelo algoritmo rr)\n"
        "  --context-switch-cost N    padrão 1\n"
        "  --io-devices N             padrão 1\n"
        "  --k1 X                     padrão 0.05 (peso de aging, algoritmo custom)\n"
        "  --k2 X                     padrão 0.3 (peso da rajada estimada, algoritmo custom)\n"
        "  --initial-burst-estimate X padrão 10.0 (algoritmo custom)\n"
        "  --ema-alpha X              padrão 0.5 (suavização da EMA de rajada)\n"
        "  --print-header             imprime só o cabeçalho CSV e sai\n"
        "  --verbose                  imprime um resumo legível em stderr\n"
        "  --help                     mostra esta mensagem\n",
        prog_name);
}

static int parse_uint64(const char *s, uint64_t *out) {
    char *end;
    unsigned long long v = strtoull(s, &end, 10);
    if (*end != '\0') return 0;
    *out = (uint64_t) v;
    return 1;
}

int cli_parse(int argc, char **argv, SimConfig *cfg) {
    memset(cfg, 0, sizeof(SimConfig));
    cfg->scenario[0] = '\0';
    cfg->algorithm[0] = '\0';
    cfg->seed = 0;
    cfg->n_processes = 1000;
    cfg->quantum = 4;
    cfg->context_switch_cost = 1;
    cfg->io_devices = 1;
    cfg->k1 = 0.05;
    cfg->k2 = 0.3;
    cfg->initial_burst_estimate = 10.0;
    cfg->ema_alpha = 0.5;
    cfg->print_header = 0;
    cfg->verbose = 0;

    int have_scenario = 0, have_algorithm = 0, have_seed = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            cli_print_usage(argv[0]);
            exit(0);
        } else if (strcmp(argv[i], "--print-header") == 0) {
            cfg->print_header = 1;
        } else if (strcmp(argv[i], "--verbose") == 0) {
            cfg->verbose = 1;
        } else if (strcmp(argv[i], "--scenario") == 0 && i + 1 < argc) {
            strncpy(cfg->scenario, argv[++i], sizeof(cfg->scenario) - 1);
            have_scenario = 1;
        } else if (strcmp(argv[i], "--algorithm") == 0 && i + 1 < argc) {
            strncpy(cfg->algorithm, argv[++i], sizeof(cfg->algorithm) - 1);
            have_algorithm = 1;
        } else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
            if (!parse_uint64(argv[++i], &cfg->seed)) {
                fprintf(stderr, "Erro: --seed precisa ser um inteiro >= 0\n");
                return 0;
            }
            have_seed = 1;
        } else if (strcmp(argv[i], "--n-processes") == 0 && i + 1 < argc) {
            cfg->n_processes = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--quantum") == 0 && i + 1 < argc) {
            cfg->quantum = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--context-switch-cost") == 0 && i + 1 < argc) {
            cfg->context_switch_cost = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--io-devices") == 0 && i + 1 < argc) {
            cfg->io_devices = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--k1") == 0 && i + 1 < argc) {
            cfg->k1 = atof(argv[++i]);
        } else if (strcmp(argv[i], "--k2") == 0 && i + 1 < argc) {
            cfg->k2 = atof(argv[++i]);
        } else if (strcmp(argv[i], "--initial-burst-estimate") == 0 && i + 1 < argc) {
            cfg->initial_burst_estimate = atof(argv[++i]);
        } else if (strcmp(argv[i], "--ema-alpha") == 0 && i + 1 < argc) {
            cfg->ema_alpha = atof(argv[++i]);
        } else {
            fprintf(stderr, "Argumento desconhecido ou incompleto: %s\n", argv[i]);
            cli_print_usage(argv[0]);
            return 0;
        }
    }

    if (cfg->print_header) return 1; /* não exige --scenario/--algorithm/--seed */

    if (!have_scenario || !have_algorithm || !have_seed) {
        fprintf(stderr, "Erro: --scenario, --algorithm e --seed são obrigatórios.\n");
        cli_print_usage(argv[0]);
        return 0;
    }
    if (cfg->n_processes < 1) {
        fprintf(stderr, "Erro: --n-processes deve ser >= 1.\n");
        return 0;
    }
    return 1;
}
