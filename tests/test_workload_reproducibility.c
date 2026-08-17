#include <stdio.h>
#include <stdlib.h>
#include "workload_generator.h"

static void check(int cond, const char *msg) {
    if (!cond) {
        fprintf(stderr, "FALHOU: %s\n", msg);
        exit(1);
    }
    printf("OK: %s\n", msg);
}

static int workloads_equal(Process *a, Process *b, int n) {
    for (int i = 0; i < n; i++) {
        if (a[i].pid != b[i].pid) return 0;
        if (a[i].arrival_time != b[i].arrival_time) return 0;
        if (a[i].priority != b[i].priority) return 0;
        if (a[i].num_bursts != b[i].num_bursts) return 0;
        for (int j = 0; j < a[i].num_bursts; j++) {
            if (a[i].bursts[j].cpu_time != b[i].bursts[j].cpu_time) return 0;
            if (a[i].bursts[j].io_time != b[i].bursts[j].io_time) return 0;
        }
    }
    return 1;
}

static void run_for_scenario(const char *scen_name) {
    ScenarioConfig cfg;
    check(scenario_get_by_name(scen_name, &cfg), "preset de cenário reconhecido");

    Process *w1 = generate_workload(&cfg, 777, 500);
    Process *w2 = generate_workload(&cfg, 777, 500);
    Process *w3 = generate_workload(&cfg, 778, 500);

    char msg1[128];
    snprintf(msg1, sizeof(msg1), "[%s] mesma seed (777) gera carga IDENTICA em duas chamadas", scen_name);
    check(workloads_equal(w1, w2, 500), msg1);

    char msg2[128];
    snprintf(msg2, sizeof(msg2), "[%s] seeds diferentes (777 vs 778) geram cargas DIFERENTES", scen_name);
    check(!workloads_equal(w1, w3, 500), msg2);

    /* nenhuma rajada de CPU pode ter duração <= 0, e apenas a última rajada
     * de cada processo pode ter io_time == 0 */
    int bursts_ok = 1;
    for (int i = 0; i < 500 && bursts_ok; i++) {
        for (int j = 0; j < w1[i].num_bursts; j++) {
            if (w1[i].bursts[j].cpu_time < 1) { bursts_ok = 0; break; }
            if (j < w1[i].num_bursts - 1 && w1[i].bursts[j].io_time < 1) { bursts_ok = 0; break; }
            if (j == w1[i].num_bursts - 1 && w1[i].bursts[j].io_time != 0) { bursts_ok = 0; break; }
        }
    }
    char msg3[128];
    snprintf(msg3, sizeof(msg3), "[%s] rajadas de CPU sempre >=1 e so a ultima rajada tem io_time==0", scen_name);
    check(bursts_ok, msg3);

    /* tempos de chegada não-decrescentes (modelo de chegada exponencial acumulado) */
    int nondecreasing = 1;
    for (int i = 1; i < 500; i++) {
        if (w1[i].arrival_time < w1[i - 1].arrival_time) { nondecreasing = 0; break; }
    }
    char msg4[128];
    snprintf(msg4, sizeof(msg4), "[%s] tempos de chegada nao-decrescentes em pid", scen_name);
    check(nondecreasing, msg4);

    workload_free(w1, 500);
    workload_free(w2, 500);
    workload_free(w3, 500);
}

int main(void) {
    run_for_scenario("balanced");
    run_for_scenario("io_bound");
    run_for_scenario("cpu_bound");
    run_for_scenario("priority_skew");

    ScenarioConfig unknown;
    check(scenario_get_by_name("nao_existe", &unknown) == 0,
          "nome de cenario desconhecido retorna 0 (falha controlada)");

    printf("Todos os testes de workload_generator.c passaram.\n");
    return 0;
}
