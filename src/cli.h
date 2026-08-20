#ifndef CLI_H
#define CLI_H

#include <stdint.h>

typedef struct {
    char scenario[64];       /* balanced | io_bound | cpu_bound | priority_skew */
    char algorithm[64];      /* fcfs | rr | priority | custom */
    uint64_t seed;
    int n_processes;
    int quantum;              /* usado só pelo RR */
    int context_switch_cost;
    int io_devices;
    double k1, k2;             /* pesos do algoritmo próprio */
    double initial_burst_estimate;
    double ema_alpha;
    int print_header;         /* se 1, imprime só o cabeçalho CSV e sai */
    int verbose;               /* se 1, imprime também um resumo legível em stderr */
} SimConfig;

/* Preenche `cfg` com os padrões e depois aplica argv. Retorna 1 em
 * sucesso, 0 se algum argumento obrigatório estiver faltando/inválido
 * (nesse caso já imprime a mensagem de uso em stderr). */
int cli_parse(int argc, char **argv, SimConfig *cfg);

void cli_print_usage(const char *prog_name);

#endif /* CLI_H */
