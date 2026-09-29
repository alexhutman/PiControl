#pragma once

#include <uv.h>

#include <stdbool.h>
#include <stdint.h>

// Thread-safe generic circular queue
typedef struct {
  uv_mutex_t mutex;
  uv_cond_t not_empty;
  uv_cond_t not_full;

  bool is_closed;

  uint8_t *items;
  size_t item_size;
  size_t capacity;

  size_t head;
  size_t tail;
} Queue;

bool pictrl_queue_init(Queue *q, size_t capacity, size_t item_size);
bool pictrl_queue_push(Queue *q, const void *item); // Blocks until not full
bool pictrl_queue_pop(Queue *q, void *out);         // Blocks until not empty
void pictrl_queue_close(Queue *q);
void pictrl_queue_destroy(Queue *q);
