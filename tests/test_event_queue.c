#include <stdio.h>
#include <stdlib.h>
#include "event_queue.h"

static void check(int cond, const char *msg) {
    if (!cond) {
        fprintf(stderr, "FALHOU: %s\n", msg);
        exit(1);
    }
    printf("OK: %s\n", msg);
}

int main(void) {
    EventQueue q;
    eq_init(&q, 4); /* capacidade pequena de propósito, para forçar eq_grow */

    eq_push(&q, 50, EVENT_ARRIVAL, 1, -1);
    eq_push(&q, 10, EVENT_ARRIVAL, 2, -1);
    eq_push(&q, 30, EVENT_ARRIVAL, 3, -1);
    eq_push(&q, 10, EVENT_ARRIVAL, 4, -1); /* empate de tempo com o processo 2, inserido depois */
    eq_push(&q, 20, EVENT_ARRIVAL, 5, -1);

    check(!eq_empty(&q), "fila não está vazia após 5 inserções");

    Event e1 = eq_pop(&q);
    check(e1.time == 10 && e1.process_index == 2,
          "primeiro evento é o de menor tempo (10, processo 2)");

    Event e2 = eq_pop(&q);
    check(e2.time == 10 && e2.process_index == 4,
          "empate de tempo (10) resolvido por ordem de inserção (FIFO): processo 4 depois do 2");

    Event e3 = eq_pop(&q);
    check(e3.time == 20 && e3.process_index == 5, "terceiro evento é tempo 20, processo 5");

    Event e4 = eq_pop(&q);
    check(e4.time == 30 && e4.process_index == 3, "quarto evento é tempo 30, processo 3");

    Event e5 = eq_pop(&q);
    check(e5.time == 50 && e5.process_index == 1, "quinto evento é tempo 50, processo 1");

    check(eq_empty(&q), "fila vazia após remover todos os eventos");

    /* teste de carga: insere em ordem aleatória, verifica saída sempre não-decrescente em (time, seq) */
    EventQueue q2;
    eq_init(&q2, 16);
    srand(7);
    int n = 5000;
    for (int i = 0; i < n; i++) {
        int t = rand() % 1000;
        eq_push(&q2, t, EVENT_ARRIVAL, i, -1);
    }
    int last_time = -1;
    int ordered = 1;
    int count = 0;
    while (!eq_empty(&q2)) {
        Event e = eq_pop(&q2);
        if (e.time < last_time) { ordered = 0; break; }
        last_time = e.time;
        count++;
    }
    check(ordered && count == n, "5000 eventos em ordem aleatória saem em ordem não-decrescente de tempo");

    eq_free(&q);
    eq_free(&q2);

    printf("Todos os testes de event_queue.c passaram.\n");
    return 0;
}
