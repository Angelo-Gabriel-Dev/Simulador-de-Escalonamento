#include <stdlib.h>
#include "event_queue.h"

/* Min-heap binário clássico em array, comparando por (time, seq).
 * Comparar por `seq` como critério de desempate garante que, quando dois
 * eventos caem exatamente no mesmo instante, eles são processados na
 * ordem em que foram inseridos na fila — isso é o que torna a simulação
 * 100% determinística e reprodutível por seed, mesmo em casos de empate
 * de tempo (ex.: duas chegadas no mesmo instante, ou uma chegada e um
 * término de E/S no mesmo tick). */

static int event_less(const Event *a, const Event *b) {
    if (a->time != b->time) return a->time < b->time;
    return a->seq < b->seq;
}

void eq_init(EventQueue *q, int initial_capacity) {
    if (initial_capacity < 16) initial_capacity = 16;
    q->data = (Event *) malloc((size_t) initial_capacity * sizeof(Event));
    q->size = 0;
    q->capacity = initial_capacity;
    q->next_seq = 0;
}

void eq_free(EventQueue *q) {
    free(q->data);
    q->data = NULL;
    q->size = 0;
    q->capacity = 0;
}

static void eq_grow(EventQueue *q) {
    q->capacity *= 2;
    q->data = (Event *) realloc(q->data, (size_t) q->capacity * sizeof(Event));
}

static void swap_events(Event *a, Event *b) {
    Event tmp = *a;
    *a = *b;
    *b = tmp;
}

static void sift_up(EventQueue *q, int i) {
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (event_less(&q->data[i], &q->data[parent])) {
            swap_events(&q->data[i], &q->data[parent]);
            i = parent;
        } else {
            break;
        }
    }
}

static void sift_down(EventQueue *q, int i) {
    for (;;) {
        int left = 2 * i + 1;
        int right = 2 * i + 2;
        int smallest = i;

        if (left < q->size && event_less(&q->data[left], &q->data[smallest]))
            smallest = left;
        if (right < q->size && event_less(&q->data[right], &q->data[smallest]))
            smallest = right;

        if (smallest == i) break;
        swap_events(&q->data[i], &q->data[smallest]);
        i = smallest;
    }
}

void eq_push(EventQueue *q, int time, EventType type, int process_index, int device_id) {
    if (q->size == q->capacity) eq_grow(q);
    Event e;
    e.time = time;
    e.type = type;
    e.process_index = process_index;
    e.device_id = device_id;
    e.seq = q->next_seq++;
    q->data[q->size] = e;
    sift_up(q, q->size);
    q->size++;
}

Event eq_pop(EventQueue *q) {
    Event top = q->data[0];
    q->size--;
    q->data[0] = q->data[q->size];
    if (q->size > 0) sift_down(q, 0);
    return top;
}

int eq_empty(const EventQueue *q) {
    return q->size == 0;
}
