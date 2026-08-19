#pragma once

#include <uv.h>

#include <stdbool.h>
#include <stddef.h>

typedef struct {
  uv_mutex_t mutex;
  void **pool;
  size_t capacity;
  size_t top;
} pictrl_pool_t;

bool pictrl_pool_init(pictrl_pool_t *p, size_t capacity, size_t item_size);
bool pictrl_pool_checkin(pictrl_pool_t *p, void *item);
void *pictrl_pool_checkout(pictrl_pool_t *p);
void pictrl_pool_destroy(pictrl_pool_t *p);
