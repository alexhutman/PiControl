#pragma once

#include <uv.h>

#include <stdbool.h>

// Thread-safe generic circular queue
typedef struct {
  uv_mutex_t mutex;
  uv_cond_t cond;

  uint8_t *items;
  size_t item_size;
  size_t capacity;

  size_t head;
  size_t tail;
} pictrl_queue_t;

bool pictrl_queue_init(pictrl_queue_t *q, size_t capacity, size_t item_size);
bool pictrl_queue_push(pictrl_queue_t *q, const void *item);
void pictrl_queue_pop(pictrl_queue_t *q, void *out); // Blocks until queue not empty
void pictrl_queue_destroy(pictrl_queue_t *q);
