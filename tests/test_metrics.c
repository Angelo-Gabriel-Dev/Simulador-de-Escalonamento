#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "process.h"
#include "metrics.h"

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

static void test_finalize_process(void) {
    Process p;
    process_init(&p, 0, 10, 5, 1);
    p.bursts[0].cpu_time = 20;
    p.bursts[0].io_time = 0;
    process_compute_min_ideal(&p); /* min_ideal_time = 20 */

    metrics_finalize_process(&p, 50); /* completion_time = 50, arrival = 10 */

    check(p.turnaround_time == 40, "turnaround_time = completion - arrival (50-10=40)");
    check(close_enough(p.slowdown, 40.0 / 20.0, 1e-9),
          "slowdown = turnaround / min_ideal_time (40/20=2.0)");
    check(p.waiting_time == 40 - 20, "waiting_time = turnaround - min_ideal_time");

    process_free(&p);
}

static void test_jain_all_equal(void) {
    /* todos os slowdowns iguais -> Jain deve ser exatamente 100% */
    int n = 5;
    Process procs[5];
    for (int i = 0; i < n; i++) {
        process_init(&procs[i], i, 0, 1, 1);
        procs[i].bursts[0].cpu_time = 10;
        procs[i].bursts[0].io_time = 0;
        process_compute_min_ideal(&procs[i]); /* min_ideal = 10 */
        metrics_finalize_process(&procs[i], 30); /* turnaround = 30, slowdown = 3.0 para todos */
    }

    RunMetrics rm;
    metrics_compute(procs, n, 42, &rm);

    check(close_enough(rm.jain_slowdown, 100.0, 1e-6),
          "Jain = 100% quando todos os processos tem o mesmo slowdown");
    check(rm.total_context_switches == 42, "metrics_compute repassa total_context_switches corretamente");
    check(close_enough(rm.avg_turnaround, 30.0, 1e-9), "avg_turnaround correto quando todos iguais");

    for (int i = 0; i < n; i++) process_free(&procs[i]);
}

static void test_jain_known_case(void) {
    /* caso conhecido: slowdowns = {1, 1, 1, 4} (n=4)
     * sum = 7, sum_sq = 1+1+1+16 = 19
     * Jain = 7^2 / (4*19) * 100 = 49/76*100 = 64.473684...% */
    int n = 4;
    double slowdowns[4] = {1.0, 1.0, 1.0, 4.0};
    Process procs[4];
    for (int i = 0; i < n; i++) {
        process_init(&procs[i], i, 0, 1, 1);
        int cpu = 10;
        procs[i].bursts[0].cpu_time = cpu;
        procs[i].bursts[0].io_time = 0;
        process_compute_min_ideal(&procs[i]); /* min_ideal = 10 */
        int completion = (int) (slowdowns[i] * 10.0); /* turnaround = slowdown * min_ideal */
        metrics_finalize_process(&procs[i], completion); /* arrival=0 => turnaround=completion */
    }

    RunMetrics rm;
    metrics_compute(procs, n, 0, &rm);

    double expected = (7.0 * 7.0) / (4.0 * 19.0) * 100.0;
    char msg[128];
    snprintf(msg, sizeof(msg), "Jain do caso {1,1,1,4} bate com valor calculado a mao (%.4f%%)", expected);
    check(close_enough(rm.jain_slowdown, expected, 1e-6), msg);

    for (int i = 0; i < n; i++) process_free(&procs[i]);
}

int main(void) {
    test_finalize_process();
    test_jain_all_equal();
    test_jain_known_case();
    printf("Todos os testes de metrics.c passaram.\n");
    return 0;
}
