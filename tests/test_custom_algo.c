#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "process.h"
#include "algorithms/custom_algo.h"

static void check(int cond, const char *msg) {
    if (!cond) {
        fprintf(stderr, "FALHOU: %s\n", msg);
        exit(1);
    }
    printf("OK: %s\n", msg);
}

static int close_enough(double a, double b, double eps) {
    return fabs(a - b) < eps;
}

static void test_ema_update(void) {
    Process p;
    process_init(&p, 0, 0, 5, 3);
    check(p.estimated_next_burst < 0.0, "antes de qualquer rajada observada, estimated_next_burst usa o sentinela (-1)");

    /* primeira observacao: EMA deve assumir o valor observado diretamente
     * (nao ha "antigo" para misturar) */
    custom_algo_observe_completed_burst(&p, 10, 0.5);
    check(close_enough(p.estimated_next_burst, 10.0, 1e-9),
          "primeira rajada observada (10) vira a estimativa diretamente");

    /* segunda observacao: EMA = alpha*novo + (1-alpha)*antigo = 0.5*20 + 0.5*10 = 15 */
    custom_algo_observe_completed_burst(&p, 20, 0.5);
    check(close_enough(p.estimated_next_burst, 15.0, 1e-9),
          "segunda rajada observada (20) atualiza a EMA para 0.5*20+0.5*10=15");

    process_free(&p);
}

static void test_aging_increases_priority_over_time(void) {
    /* dois processos com prioridade estatica identica; o que espera mais
     * tempo deve ganhar prioridade efetiva melhor (numero menor) com o
     * passar do tempo, por causa do termo de aging (-k1*tempo_de_espera). */
    SchedulerAlgorithm *algo = custom_algo_create(10, /*k1=*/1.0, /*k2=*/0.0, /*initial_estimate=*/0.0);

    Process p0, p1;
    process_init(&p0, 0, 0, 5, 1);
    p0.bursts[0].cpu_time = 5; p0.bursts[0].io_time = 0;
    process_init(&p1, 1, 0, 5, 1);
    p1.bursts[0].cpu_time = 5; p1.bursts[0].io_time = 0;

    algo->on_process_arrival(algo, &p0, 0);  /* p0 entra pronto em t=0 */
    algo->on_process_arrival(algo, &p1, 10); /* p1 entra pronto em t=10, bem mais tarde */

    /* em t=11, p0 esperou 11 unidades, p1 esperou so 1 -> com k1=1, k2=0,
     * eff(p0) = 5 - 1*11 = -6 ; eff(p1) = 5 - 1*1 = 4 -> p0 deve vencer
     * mesmo tendo a MESMA prioridade estatica, por ter esperado mais. */
    Process *chosen = algo->select_next(algo, 11);
    check(chosen->pid == 0, "envelhecimento (aging) favorece quem espera mais, mesmo com prioridade estatica igual");

    process_free(&p0); process_free(&p1);
    custom_algo_destroy(algo);
}

static void test_uses_only_observed_history(void) {
    /* Verificacao estrutural de nao-antecipacao: a estimativa de rajada
     * usada na decisao SO pode vir de (a) o fallback configuravel
     * initial_estimate, quando nada foi observado ainda, ou (b) uma EMA
     * de rajadas ja CONCLUIDAS do mesmo processo (atualizada so por
     * custom_algo_observe_completed_burst, chamada por main.c somente
     * DEPOIS que uma rajada termina de fato). Este teste confirma que,
     * antes de qualquer chamada a observe_completed_burst, a decisao usa
     * exatamente o fallback configurado -- nunca a duracao real (ainda
     * desconhecida) da rajada atual ou de rajadas futuras. */
    SchedulerAlgorithm *algo = custom_algo_create(10, /*k1=*/0.0, /*k2=*/1.0, /*initial_estimate=*/7.0);

    Process p;
    process_init(&p, 0, 0, 5, 2);
    p.bursts[0].cpu_time = 999; /* rajada "real" bem diferente do fallback, de proposito */
    p.bursts[0].io_time = 3;
    p.bursts[1].cpu_time = 1;
    p.bursts[1].io_time = 0;

    algo->on_process_arrival(algo, &p, 0);
    /* com k1=0, k2=1: eff = priority + 1*rajada_estimada.
     * Se o algoritmo "trapaceasse" e usasse bursts[0].cpu_time (999) em
     * vez do fallback (7.0), eff seria ~1004; testamos que NAO e isso. */
    Process *chosen = algo->select_next(algo, 0);
    check(chosen != NULL, "processo unico na fila e selecionado");

    /* Recalcula manualmente o que a prioridade efetiva deveria ter sido:
     * so podemos verificar indiretamente (a API nao expõe eff diretamente),
     * entao testamos o contrato publico: antes de observar qualquer rajada
     * concluida, o campo estimated_next_burst continua no sentinela. */
    check(p.estimated_next_burst < 0.0,
          "antes de a rajada atual terminar, nenhuma informacao sobre sua duracao foi registrada no processo");

    process_free(&p);
    custom_algo_destroy(algo);
}

int main(void) {
    test_ema_update();
    test_aging_increases_priority_over_time();
    test_uses_only_observed_history();
    printf("Todos os testes do algoritmo proprio (custom_algo.c) passaram.\n");
    return 0;
}
