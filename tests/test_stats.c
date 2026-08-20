#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "stats.h"

static void check(int condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "FALHOU: %s\n", message);
        exit(1);
    }
    printf("OK: %s\n", message);
}

static int close_enough(double actual, double expected, double tolerance) {
    return fabs(actual - expected) <= tolerance;
}

static void test_empty_sample(void) {
    double mean = -1.0;
    double ci95 = -1.0;

    stats_ci95(NULL, 0, &mean, &ci95);

    check(mean == 0.0, "amostra vazia retorna media zero");
    check(ci95 == 0.0, "amostra vazia retorna IC95% zero");
}

static void test_single_sample(void) {
    const double values[] = {42.5};
    double mean = 0.0;
    double ci95 = -1.0;

    stats_ci95(values, 1, &mean, &ci95);

    check(mean == 42.5, "uma amostra preserva o valor como media");
    check(ci95 == 0.0, "uma amostra retorna IC95% zero");
}

static void test_known_sample(void) {
    const double values[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    const double expected_ci95 = 1.96 * sqrt(2.5) / sqrt(5.0);
    double mean = 0.0;
    double ci95 = 0.0;

    stats_ci95(values, 5, &mean, &ci95);

    check(close_enough(mean, 3.0, 1e-12),
          "media do caso conhecido {1,2,3,4,5} e igual a 3");
    check(close_enough(ci95, expected_ci95, 1e-12),
          "IC95% usa desvio padrao amostral com denominador n-1");
}

static void test_constant_sample(void) {
    const double values[] = {7.0, 7.0, 7.0, 7.0};
    double mean = 0.0;
    double ci95 = -1.0;

    stats_ci95(values, 4, &mean, &ci95);

    check(mean == 7.0, "amostra constante preserva a media");
    check(ci95 == 0.0, "amostra constante tem IC95% zero");
}

int main(void) {
    test_empty_sample();
    test_single_sample();
    test_known_sample();
    test_constant_sample();
    printf("Todos os testes de stats.c passaram.\n");
    return 0;
}
