#ifndef _PICTRL_RING_BUFFER_H
#define _PICTRL_RING_BUFFER_H

#include <stddef.h>
#include <stdint.h>

typedef struct pictrl_rb_t {
    uint8_t *buffer_start;
    size_t num_bytes;

    uint8_t *data_start;
    size_t data_length;
} pictrl_rb_t;

typedef enum pictrl_read_flag {
    PICTRL_READ_PEEK,
    PICTRL_READ_CONSUME
} pictrl_read_flag;

pictrl_rb_t *pictrl_rb_init(pictrl_rb_t*, size_t);
void pictrl_rb_destroy(pictrl_rb_t*);
size_t pictrl_rb_insert(pictrl_rb_t*, void*, size_t);
size_t pictrl_rb_read(pictrl_rb_t*, pictrl_read_flag, void*, size_t);
void pictrl_rb_clear(pictrl_rb_t*);

#endif
