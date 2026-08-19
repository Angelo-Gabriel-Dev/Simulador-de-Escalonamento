#ifndef IO_MANAGER_H
#define IO_MANAGER_H

/*
 * Modelagem de E/S (decisão de projeto documentada em docs/modelagem.md,
 * Seção 4.2 do prompt mestre):
 *  - N_IO_DEVICES dispositivos configuráveis (padrão = 1).
 *  - Cada dispositivo tem sua própria fila FIFO.
 *  - Um processo que solicita E/S é atribuído ao dispositivo com MENOR
 *    carga no momento (carga = 1 se ocupado + tamanho da fila de espera);
 *    empate é resolvido em round-robin entre os dispositivos empatados.
 *  - Dentro de um mesmo dispositivo a E/S NÃO é paralela (um processo por
 *    vez); dispositivos diferentes podem estar servindo em paralelo.
 *  - Essa modelagem é idêntica para todos os algoritmos avaliados.
 */

typedef struct {
    int busy;              /* 0/1 */
    int current_process;   /* índice do processo em atendimento, -1 se nenhum */
    int *queue;             /* fila FIFO de índices de processo aguardando */
    int queue_len;
    int queue_cap;
} IoDevice;

typedef struct {
    IoDevice *devices;
    int n_devices;
    int rr_pointer; /* para desempate round-robin entre dispositivos com carga igual */
} IoManager;

void io_manager_init(IoManager *im, int n_devices);
void io_manager_free(IoManager *im);

/* Processo `process_index` solicita E/S com duração `io_time` no instante
 * `now`. Retorna o id do dispositivo escolhido. Se o dispositivo estava
 * livre, a E/S começa imediatamente e `*started_immediately` é setado a 1
 * (o chamador deve então agendar EVENT_IO_DONE em now+io_time). Se o
 * dispositivo estava ocupado, o processo só é enfileirado e
 * `*started_immediately` é 0 (o evento IO_DONE será agendado depois, por
 * io_manager_service_done, quando chegar a vez desse processo). */
int io_manager_request(IoManager *im, int process_index, int *started_immediately);

/* Chamado quando a E/S em `device_id` termina. Marca o dispositivo como
 * livre e, se houver alguém na fila desse dispositivo, remove o próximo
 * processo da fila, marca o dispositivo como ocupado por ele e retorna o
 * índice desse próximo processo em `*next_process_index` (o chamador deve
 * agendar o EVENT_IO_DONE dele em now+io_time). Se a fila estava vazia,
 * `*next_process_index` é setado a -1. */
void io_manager_service_done(IoManager *im, int device_id, int *next_process_index);

#endif /* IO_MANAGER_H */

