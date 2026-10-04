#include "data_structures/pool.h"

#include <uv.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

bool pictrl_pool_init(Pool *p, const PoolOpts *opts) {
  void **pool = calloc(opts->capacity, sizeof(*(p->pool)));
  if (!pool)
    return false;

  if (uv_mutex_init(&p->mutex) < 0) {
    free(pool);
    return false;
  }

  for (size_t i = 0; i < opts->capacity; i++) {
    pool[i] = calloc(1, opts->item_size);
    if (!pool[i]) {
      for (size_t j = 0; j < i; j++)
        free(pool[j]);
      free(pool);
      uv_mutex_destroy(&p->mutex);
      return false;
    }

    if (opts->init_cb && opts->init_cb(pool[i], opts->user_data) < 0) {
      for (size_t j = 0; j <= i; j++) {
        if (opts->destroy_cb)
          opts->destroy_cb(pool[j], opts->user_data);
        free(pool[j]);
      }
      free(pool);
      uv_mutex_destroy(&p->mutex);
      return false;
    }
  }

  p->pool = pool;
  p->capacity = opts->capacity;
  p->top = opts->capacity;
  p->destroy_cb = opts->destroy_cb;
  p->user_data = opts->user_data;
  return true;
}

bool pictrl_pool_checkin(Pool *p, void *item) {
  if (!item)
    return false;

  uv_mutex_lock(&p->mutex);
  if (p->top < p->capacity) {
    p->pool[p->top] = item;
    p->top++;

    uv_mutex_unlock(&p->mutex);
    return true;
  }

  uv_mutex_unlock(&p->mutex);
  return false;
}

void *pictrl_pool_checkout(Pool *p) {
  void *item = NULL;
  uv_mutex_lock(&p->mutex);
  if (p->top > 0) {
    p->top--;
    item = p->pool[p->top];
  }
  uv_mutex_unlock(&p->mutex);
  return item;
}

void pictrl_pool_destroy(Pool *p) {
  if (!p)
    return;

  for (size_t i = 0; i < p->top; i++) {
    if (p->destroy_cb)
      p->destroy_cb(p->pool[i], p->user_data);
    free(p->pool[i]);
  }
  free(p->pool);
  uv_mutex_destroy(&p->mutex);
}
