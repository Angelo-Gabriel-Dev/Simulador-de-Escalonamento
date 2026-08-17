#include <stdio.h>
#include <stdlib.h>
#include "rng.h"

static void check(int cond, const char *msg) {
    if (!cond) {
        fprintf(stderr, "FALHOU: %s\n", msg);
        exit(1);
    }
    printf("OK: %s\n", msg);
}

int main(void) {
    RngState a, b;
    rng_seed(&a, 12345);
    rng_seed(&b, 12345);

    int all_equal = 1;
    for (int i = 0; i < 1000; i++) {
        if (rng_next_u32(&a) != rng_next_u32(&b)) { all_equal = 0; break; }
    }
    check(all_equal, "mesma seed produz a mesma sequência de u32");

    RngState c;
    rng_seed(&c, 54321);
    int any_diff = 0;
    for (int i = 0; i < 1000; i++) {
        RngState a2, c2;
        rng_seed(&a2, 12345);
        rng_seed(&c2, 54321);
        if (rng_next_u32(&a2) != rng_next_u32(&c2)) { any_diff = 1; break; }
    }
    check(any_diff, "seeds diferentes produzem sequências diferentes");

    RngState r;
    rng_seed(&r, 999);
    int in_range = 1;
    for (int i = 0; i < 10000; i++) {
        int v = rng_range_int(&r, 3, 12);
        if (v < 3 || v > 12) { in_range = 0; break; }
    }
    check(in_range, "rng_range_int respeita os limites [min, max] em 10k amostras");

    RngState d;
    rng_seed(&d, 1);
    int double_in_unit_interval = 1;
    for (int i = 0; i < 10000; i++) {
        double v = rng_next_double(&d);
        if (v < 0.0 || v >= 1.0) { double_in_unit_interval = 0; break; }
    }
    check(double_in_unit_interval, "rng_next_double sempre em [0,1) em 10k amostras");

    RngState e;
    rng_seed(&e, 2);
    double sum = 0.0;
    int nsamp = 200000;
    for (int i = 0; i < nsamp; i++) sum += rng_exponential(&e, 10.0);
    double mean = sum / nsamp;
    /* média amostral deve ficar razoavelmente perto de 10.0 (tolerância generosa,
     * é apenas um teste de sanidade, não um teste estatístico rigoroso) */
    check(mean > 9.0 && mean < 11.0, "rng_exponential(mean=10) tem media amostral perto de 10 em 200k amostras");

    printf("Todos os testes de rng.c passaram.\n");
    return 0;
}
