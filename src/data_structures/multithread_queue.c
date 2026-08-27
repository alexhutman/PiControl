#include "data_structures/multithread_queue.h"

#include <uv.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

bool pictrl_queue_init(Queue *q, size_t capacity, size_t item_size) {
  if (!q)
    return false;
  void *items = malloc(capacity * item_size);

  if (uv_mutex_init(&q->mutex) < 0) {
    free(items);
    return false;
  }
  if (uv_cond_init(&q->not_empty) < 0) {
    free(items);
    uv_mutex_destroy(&q->mutex);
    return false;
  }
  if (uv_cond_init(&q->not_full) < 0) {
    free(items);
    uv_mutex_destroy(&q->mutex);
    uv_cond_destroy(&q->not_empty);
    return false;
  }

  q->items = items;
  q->capacity = capacity;
  q->item_size = item_size;
  q->head = 0;
  q->tail = 0;
  return true;
}

bool pictrl_queue_push(Queue *q, const void *item) {
  uv_mutex_lock(&q->mutex);

  if (q->is_closed) {
    uv_mutex_unlock(&q->mutex);
    return false;
  }

  // Wait if full, but break if closed (while, not if, because of "spurious wakeups")
  while ((((q->tail + 1) % q->capacity) == q->head) && !q->is_closed) {
    uv_cond_wait(&q->not_full, &q->mutex);
  }

  // Closed while we were waiting
  if (q->is_closed) {
    uv_mutex_unlock(&q->mutex);
    return false;
  }

  const size_t next = (q->tail + 1) % q->capacity;
  uint8_t *dest = q->items + (q->tail * q->item_size);
  memcpy(dest, item, q->item_size);
  q->tail = next;

  uv_cond_signal(&q->not_empty);
  uv_mutex_unlock(&q->mutex);
  return true;
}

bool pictrl_queue_pop(Queue *q, void *out) {
  uv_mutex_lock(&q->mutex);

  // Wait if empty, but break if closed (while, not if, because of "spurious wakeups")
  while ((q->head == q->tail) && !q->is_closed) {
    uv_cond_wait(&q->not_empty, &q->mutex);
  }

  // Allow for draining of items before closing
  if ((q->head == q->tail) && q->is_closed) {
    uv_mutex_unlock(&q->mutex);
    return false;
  }

  uint8_t *src = q->items + (q->head * q->item_size);
  memcpy(out, src, q->item_size);
  q->head = (q->head + 1) % q->capacity;

  uv_cond_signal(&q->not_full);
  uv_mutex_unlock(&q->mutex);
  return true;
}

void pictrl_queue_close(Queue *q) {
  uv_mutex_lock(&q->mutex);

  q->is_closed = true;

  // Notify all blocking producers/consumers
  uv_cond_broadcast(&q->not_empty);
  uv_cond_broadcast(&q->not_full);

  uv_mutex_unlock(&q->mutex);
}

void pictrl_queue_destroy(Queue *q) {
  if (!q)
    return;

  if (q->items != NULL)
    free(q->items);
  uv_mutex_destroy(&q->mutex);
  uv_cond_destroy(&q->not_empty);
  uv_cond_destroy(&q->not_full);
}
