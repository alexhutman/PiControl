#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "data_structures/ring_buffer.h"


pictrl_rb_t *pictrl_rb_init(pictrl_rb_t *rb, size_t num_bytes) {
    if (num_bytes == 0) {
        fprintf(stderr, "Make a ring buffer bigger than 0 bytes.\n");
        return NULL; // Error (perror?) or something?
    }

    uint8_t *buf = calloc(num_bytes, sizeof(uint8_t)); // TODO: change to malloc?
    if (!buf) {
        fprintf(stderr, "Allocation of %zu bytes failed\n", num_bytes);
        return NULL;
    }
    rb->buffer_start = buf;
    rb->num_bytes = num_bytes;
    rb->data_start = buf;
    rb->data_length = 0;

    return rb;
}


void pictrl_rb_destroy(pictrl_rb_t *rb) {
    free(rb->buffer_start);

    rb->buffer_start = NULL;
    rb->num_bytes = 0;
    rb->data_start = NULL;
    rb->data_length = 0;
}


// Insert `num` bytes from `src_start` into the ring buffer
size_t pictrl_rb_insert(pictrl_rb_t *rb, int fd, size_t num) {
    size_t available_bytes = rb->num_bytes - rb->data_length;
    if (num == 0 || available_bytes == 0) {
        return 0;
    }

    // if not enough space, write as much as we can
    const size_t num_bytes_to_write = (num > available_bytes) ? available_bytes : num;
    const size_t data_offset_from_buf_start = rb->data_start - rb->buffer_start; // TODO: ASSERT THIS IS ALWAYS POSITIVE (how to even detect negative with size_t?)
    const size_t insertion_offset = (data_offset_from_buf_start + rb->data_length) % rb->num_bytes; // next slot after data_end. offset from buffer_start
    const size_t insertion_end_offset = (insertion_offset + num_bytes_to_write - 1) % rb->num_bytes; // end of data that is to be inserted. offset from buffer_start

    const bool insertion_wrapped = insertion_end_offset < insertion_offset; // (case 2)
    if (insertion_wrapped) {
        // insert from the insertion point to the end
        const size_t num_bytes_first_pass = rb->num_bytes - insertion_offset;
        read(fd, rb->buffer_start + insertion_offset, num_bytes_first_pass);

        // then from the beginning to insertion_end
        read(fd, rb->buffer_start, num_bytes_to_write - num_bytes_first_pass);
    } else {
        read(fd, rb->buffer_start + insertion_offset, num_bytes_to_write);
    }
    rb->data_length += num_bytes_to_write;

    return num_bytes_to_write;
}


size_t pictrl_rb_read(pictrl_rb_t *rb, pictrl_read_flag flag, int fd, size_t num) {
    if (num == 0 || rb->data_length == 0) {
        return 0;
    }

    // if not enough data, read as much as we can
    const size_t num_bytes_to_read = (num > rb->data_length) ? rb->data_length : num;
    const size_t cur_data_offset_start = rb->data_start - rb->buffer_start; // TODO: ASSERT THIS IS ALWAYS POSITIVE (how to even detect negative with size_t?)
    const size_t cur_data_offset_end = (cur_data_offset_start + rb->data_length - 1) % rb->num_bytes; // next slot after data_end. offset from buffer_start
    const size_t new_data_offset_start = (cur_data_offset_start + num_bytes_to_read) % rb->num_bytes;

    const bool data_wrapped = cur_data_offset_end < cur_data_offset_start;
    if (data_wrapped) {
        const size_t num_edge_bytes = rb->num_bytes - cur_data_offset_start;
        write(fd, rb->data_start, num_edge_bytes);
        write(fd, rb->buffer_start, num - num_edge_bytes);
    } else {
        write(fd, rb->data_start, num_bytes_to_read);
    }

    if (flag == PICTRL_READ_CONSUME) {
        rb->data_length -= num_bytes_to_read;
        rb->data_start = rb->buffer_start + new_data_offset_start;
    }

    return num_bytes_to_read;
}

void pictrl_rb_clear(pictrl_rb_t *rb) {
    memset(rb->buffer_start, 0, rb->num_bytes*sizeof(uint8_t));
    rb->data_length = 0;
    rb->data_start = rb->buffer_start;
}
