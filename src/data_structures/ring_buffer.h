#pragma once

#include <unistd.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Types
typedef struct RingBuffer {
  uint8_t *buffer;
  size_t capacity;

  size_t data_start; // index of start of the data section
  size_t num_items;  // TODO: make atomic?
} RingBuffer;

typedef enum RBReadFlag { PICTRL_READ_PEEK, PICTRL_READ_CONSUME } RBReadFlag;

// Prototypes
RingBuffer *pictrl_rb_init(RingBuffer *, size_t);
void pictrl_rb_destroy(RingBuffer *);
ssize_t pictrl_rb_read(int, size_t, RingBuffer *, RBReadFlag);
ssize_t pictrl_rb_write(int, size_t, RingBuffer *);
void pictrl_rb_clear(RingBuffer *);
void pictrl_rb_copy(RingBuffer *rb, void *dest);

void print_ring_buffer(RingBuffer *);
void print_rb_in_order(RingBuffer *);
void print_raw_buf(RingBuffer *);
void print_buf(void *, size_t);

// Static "methods"
static inline bool pictrl_rb_data_wrapped(RingBuffer *rb) {
  const size_t data_end_idx = rb->data_start + rb->num_items;
  return data_end_idx > rb->capacity;
}

/*
 * Indices wrap around modulo rb->num_items to make sure we only have access to
 what we're allowed
 * Should probably call the read lock that isn't implemented yet before calling
 this, it's not done here because: A.) I haven't gotten around to the r/w locks
   B.) that you can get multiple items if desired
 *
 * i.e., with the following setup, get(3) would normally return the value at
 absolute index 1 in the raw buffer, but it returns 0, since we only have 3
 items in the ring buffer and we wrapped to the data_start:
 *
 *  |2--01|
 *      ^
 *      |_ data_start (val == idx)
*/
static inline uint8_t pictrl_rb_get(RingBuffer *rb, size_t idx) {
  const size_t target_abs_idx = (rb->data_start + (idx % rb->num_items)) % rb->capacity;
  return rb->buffer[target_abs_idx];
}

static inline uint8_t *pictrl_rb_data_start_address(RingBuffer *rb) {
  return &rb->buffer[rb->data_start];
}
