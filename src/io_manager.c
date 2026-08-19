#include <stdlib.h>
#include "io_manager.h"

#define IO_QUEUE_INITIAL_CAP 8

void io_manager_init(IoManager *im, int n_devices) {
    if (n_devices < 1) n_devices = 1;
    im->n_devices = n_devices;
    im->rr_pointer = 0;
    im->devices = (IoDevice *) malloc((size_t) n_devices * sizeof(IoDevice));
    for (int i = 0; i < n_devices; i++) {
        im->devices[i].busy = 0;
        im->devices[i].current_process = -1;
        im->devices[i].queue_cap = IO_QUEUE_INITIAL_CAP;
        im->devices[i].queue_len = 0;
        im->devices[i].queue = (int *) malloc((size_t) IO_QUEUE_INITIAL_CAP * sizeof(int));
    }
}

void io_manager_free(IoManager *im) {
    for (int i = 0; i < im->n_devices; i++) {
        free(im->devices[i].queue);
    }
    free(im->devices);
    im->devices = NULL;
}

static void device_enqueue(IoDevice *dev, int process_index) {
    if (dev->queue_len == dev->queue_cap) {
        dev->queue_cap *= 2;
        dev->queue = (int *) realloc(dev->queue, (size_t) dev->queue_cap * sizeof(int));
    }
    dev->queue[dev->queue_len++] = process_index;
}

static int device_dequeue(IoDevice *dev) {
    int head = dev->queue[0];
    for (int i = 1; i < dev->queue_len; i++) {
        dev->queue[i - 1] = dev->queue[i];
    }
    dev->queue_len--;
    return head;
}

static int device_load(const IoDevice *dev) {
    return (dev->busy ? 1 : 0) + dev->queue_len;
}

int io_manager_request(IoManager *im, int process_index, int *started_immediately) {
    /* escolhe o dispositivo de menor carga; em empate, começa a busca a
     * partir de rr_pointer e avança -> comportamento round-robin entre
     * dispositivos empatados ao longo de várias chamadas. */
    int best = -1;
    int best_load = -1;
    for (int k = 0; k < im->n_devices; k++) {
        int idx = (im->rr_pointer + k) % im->n_devices;
        int load = device_load(&im->devices[idx]);
        if (best == -1 || load < best_load) {
            best = idx;
            best_load = load;
        }
    }
    im->rr_pointer = (best + 1) % im->n_devices;

    IoDevice *dev = &im->devices[best];
    if (!dev->busy) {
        dev->busy = 1;
        dev->current_process = process_index;
        *started_immediately = 1;
    } else {
        device_enqueue(dev, process_index);
        *started_immediately = 0;
    }
    return best;
}

void io_manager_service_done(IoManager *im, int device_id, int *next_process_index) {
    IoDevice *dev = &im->devices[device_id];
    dev->busy = 0;
    dev->current_process = -1;

    if (dev->queue_len > 0) {
        int next = device_dequeue(dev);
        dev->busy = 1;
        dev->current_process = next;
        *next_process_index = next;
    } else {
        *next_process_index = -1;
    }
}

