#include "data_structures/multithread_pool.h"

#include <uv.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

bool pictrl_pool_init(pictrl_pool_t *p, size_t capacity, size_t item_size) {
    p->top = 0;
    
    p->pool = calloc(capacity, sizeof(*(p->pool)));
    if (!p->pool) return false;
    
    if (uv_mutex_init(&p->mutex) < 0) {
        free(p->pool);
        return false;
    }
    
    for (size_t i = 0; i < capacity; i++) {
        p->pool[i] = calloc(1, item_size);
        if (!p->pool[i]) {
            for (size_t j = 0; j < i; j++) free(p->pool[j]);
            free(p->pool);
            uv_mutex_destroy(&p->mutex);
            return false;
        }
    }
    
    p->capacity = capacity;
    p->top = capacity;
    return true;
}

bool pictrl_pool_checkin(pictrl_pool_t *p, void *item) {
    if (!item) return false;

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

void *pictrl_pool_checkout(pictrl_pool_t *p) {
    void *item = NULL;
    uv_mutex_lock(&p->mutex);
    if (p->top > 0) {
        p->top--;
        item = p->pool[p->top];
    }
    uv_mutex_unlock(&p->mutex);
    return item;
}

void pictrl_pool_destroy(pictrl_pool_t *p) {
    for (size_t i = 0; i < p->top; i++) {
        free(p->pool[i]);
    }
    free(p->pool);
    uv_mutex_destroy(&p->mutex);
}
