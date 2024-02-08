#ifndef _PICTRL_RING_BUFFER_H
#define _PICTRL_RING_BUFFER_H

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

// Types
typedef struct pictrl_rb_t {
    uint8_t *buffer_start; // TODO: rename to just `buffer`
    size_t num_bytes; // TODO: rename to `capacity`

    uint8_t *data_start; // TODO: just store the idx of it instead...
    size_t data_length; // TODO: rename to `num_items` or `length`, make atomic?
} pictrl_rb_t;

typedef enum pictrl_read_flag {
    PICTRL_READ_PEEK,
    PICTRL_READ_CONSUME
} pictrl_read_flag;

// Prototypes
pictrl_rb_t *pictrl_rb_init(pictrl_rb_t*, size_t);
void pictrl_rb_destroy(pictrl_rb_t*);
ssize_t pictrl_rb_read(int, size_t, pictrl_rb_t*, pictrl_read_flag);
ssize_t pictrl_rb_write(int, size_t, pictrl_rb_t*);
void pictrl_rb_clear(pictrl_rb_t*);
void pictrl_rb_copy(pictrl_rb_t *rb, void *dest);

// Static "methods"
static inline bool pictrl_rb_data_wrapped(pictrl_rb_t *rb) {
    const size_t data_start_abs_idx = rb->data_start - rb->buffer_start;
    const size_t data_end_idx = data_start_abs_idx + rb->data_length;
    return data_end_idx > rb->num_bytes;
}

/*
 * Indices wrap around modulo rb->data_length to make sure we only have access to what we're allowed
 * Should probably call the read lock that isn't implemented yet before calling this,
   it's not done here because:
   A.) I haven't gotten around to the r/w locks
   B.) that you can get multiple items if desired
 *
 * i.e., with the following setup, get(3) would normally return the value at absolute index 1 in the raw buffer,
   but it returns 0, since we only have 3 items in the ring buffer and we wrapped to the
   data_start:
 *
 *  |2--01|
 *      ^
 *      |_ data_start (val == idx)
*/
static inline uint8_t pictrl_rb_get(pictrl_rb_t *rb, size_t idx) {
    const size_t data_start_abs_idx = rb->data_start - rb->buffer_start;
    const size_t target_abs_idx = (data_start_abs_idx + (idx % rb->data_length)) % rb->num_bytes;
    return rb->buffer_start[target_abs_idx];
}
#endif
