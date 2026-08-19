#include "data_structures/multithread_queue.h"

#include <stddef.h>
#include <uv.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

bool pictrl_queue_init(pictrl_queue_t *q, size_t capacity, size_t item_size) {
    q->items = malloc(capacity * item_size);
    if (!q) return false;

    if (uv_mutex_init(&q->mutex) < 0) {
        free(q->items);
        return false;
    }
    if (uv_cond_init(&q->cond) < 0) {
        free(q->items);
        uv_mutex_destroy(&q->mutex);
        return false;
    }

    q->capacity = capacity;
    q->item_size = item_size;
    q->head = 0;
    q->tail = 0;
    return true;
}

bool pictrl_queue_push(pictrl_queue_t *q, const void *item) {
    // TODO: Keep mutex lock/unlocking here and in dequeue()?
    uv_mutex_lock(&q->mutex);

    const size_t next = (q->tail + 1) % q->capacity;
    if (next == q->head) { // Full
        uv_mutex_unlock(&q->mutex);
        return false;
    }

    uint8_t *dest = q->items + (q->tail * q->item_size);
    memcpy(dest, item, q->item_size);
    q->tail = next;

    uv_mutex_unlock(&q->mutex);
    uv_cond_signal(&q->cond);
    return true;
}

void pictrl_queue_pop(pictrl_queue_t *q, void *out) {
    uv_mutex_lock(&q->mutex);

    // `while` instead of `if` because of "spurious wakeups"
    while (q->head == q->tail) { // Empty
        uv_cond_wait(&q->cond, &q->mutex);
    }

    uint8_t *src = q->items + (q->head * q->item_size);
    memcpy(out, src, q->item_size);
    q->head = (q->head + 1) % q->capacity;

    uv_mutex_unlock(&q->mutex);
}

void pictrl_queue_destroy(pictrl_queue_t *q) {
    if (!q) return;

    if (q->items != NULL) free(q->items);
    uv_mutex_destroy(&q->mutex);
    uv_cond_destroy(&q->cond);
}
