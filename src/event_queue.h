#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

typedef enum {
    EVENT_ARRIVAL,
    EVENT_CPU_BURST_DONE,
    EVENT_IO_DONE,
    EVENT_CONTEXT_SWITCH_DONE,
    EVENT_QUANTUM_EXPIRED
} EventType;

typedef struct {
    int time;
    EventType type;
    int process_index;  /* índice em Process[] */
    int device_id;       /* relevante só para EVENT_IO_DONE; -1 caso contrário */
    long seq;             /* número de sequência de inserção — usado como
                            * critério de desempate determinístico entre
                            * eventos com o mesmo `time` (ver docs/modelagem.md) */
} Event;

typedef struct {
    Event *data;
    int size;
    int capacity;
    long next_seq;
} EventQueue;

void eq_init(EventQueue *q, int initial_capacity);
void eq_free(EventQueue *q);

/* Insere um evento; `seq` é atribuído internamente (ordem de inserção). */
void eq_push(EventQueue *q, int time, EventType type, int process_index, int device_id);

/* Remove e retorna o evento de menor (time, seq). Comportamento indefinido
 * se a fila estiver vazia — sempre checar eq_empty() antes. */
Event eq_pop(EventQueue *q);

int eq_empty(const EventQueue *q);

#endif /* EVENT_QUEUE_H */
