#pragma once

#include <uv.h>

#include <stdbool.h>
#include <stddef.h>

typedef int (*pool_item_init_cb)(void *item, void *user_data);
typedef int (*pool_item_destroy_cb)(void *item, void *user_data);

typedef struct {
  uv_mutex_t mutex;
  void **pool;
  size_t capacity;
  size_t top;

  pool_item_destroy_cb destroy_cb;
  void *user_data;
} pictrl_pool_t;

typedef struct {
  size_t capacity;
  size_t item_size;
  pool_item_init_cb init_cb;
  pool_item_destroy_cb destroy_cb;
  void *user_data;
} pictrl_pool_opts;

bool pictrl_pool_init(pictrl_pool_t *p, const pictrl_pool_opts *opts);
bool pictrl_pool_checkin(pictrl_pool_t *p, void *item);
void *pictrl_pool_checkout(pictrl_pool_t *p);
void pictrl_pool_destroy(pictrl_pool_t *p);
