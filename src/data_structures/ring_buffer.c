#include <stdbool.h>
#include <stddef.h>
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


ssize_t pictrl_rb_write(int fd, size_t num, pictrl_rb_t *rb) {
    const size_t available_bytes = rb->num_bytes - rb->data_length;
    if (num == 0 || available_bytes == 0) {
        return 0;
    }

    const size_t num_bytes_to_write = (num > available_bytes) ? available_bytes : num; // if not enough space, write as much as we can
    const size_t data_offset = rb->data_start - rb->buffer_start; // offset of the start of the data section
    const size_t write_offset_start = (data_offset + rb->data_length) % rb->num_bytes; // next slot after data_end. offset from buffer_start
    const size_t write_offset_end = (write_offset_start + num_bytes_to_write - 1) % rb->num_bytes; // end of data that is to be written. offset from buffer_start

    const bool wrapped = write_offset_end < write_offset_start;
    ssize_t bytes_read = 0;
    if (wrapped) {
        // write from the write offset to the end
        const size_t num_bytes_first_pass = rb->num_bytes - write_offset_start;
        const ssize_t first_pass = read(fd, rb->buffer_start + write_offset_start, num_bytes_first_pass);
        if (first_pass == 0) {
            return 0;
        }
        if (first_pass == -1) {
            // TODO: log strerror
            return -1;
        }
        bytes_read += first_pass;

        // then from the beginning to write_end
        if (first_pass == num_bytes_first_pass) {
            const ssize_t second_pass = read(fd, rb->buffer_start, num_bytes_to_write - num_bytes_first_pass);
            if (second_pass == -1) {
                // TODO: log strerror
            } else {
                bytes_read += second_pass;
            }
        }
    } else {
        const ssize_t only_pass = read(fd, rb->buffer_start + write_offset_start, num_bytes_to_write);
        if (only_pass == 0) {
            return 0;
        }
        if (only_pass < 0) {
            // TODO: log strerror
            return -1;
        }
        bytes_read += only_pass;
    }

    rb->data_length += (size_t)bytes_read;
    return bytes_read;
}

// TODO: Create read/write_log_error
ssize_t pictrl_rb_read(int fd, size_t num, pictrl_rb_t *rb, pictrl_read_flag flag) {
    if (num == 0 || rb->data_length == 0) {
        return 0;
    }

    // if not enough data, read as much as we can
    const size_t num_bytes_to_read = (num > rb->data_length) ? rb->data_length : num;
    const size_t data_offset_start = rb->data_start - rb->buffer_start; // TODO: ASSERT THIS IS ALWAYS POSITIVE (how to even detect negative with size_t?)
    const size_t data_offset_end = (data_offset_start + rb->data_length - 1) % rb->num_bytes; // next slot after data_end. offset from buffer_start
    const size_t new_data_offset_start = (data_offset_start + num_bytes_to_read) % rb->num_bytes;

    const bool wrapped = data_offset_end < data_offset_start;
    ssize_t bytes_written = 0;
    if (wrapped) {
        // Write 'til the end
        const size_t num_edge_bytes = rb->num_bytes - data_offset_start;
        const ssize_t first_pass = write(fd, rb->data_start, num_edge_bytes);
        if (first_pass == -1) {
            // TODO: Log strerror
            return -1;
        }
        bytes_written += first_pass;

        // Then resume from the beginning to attempt to write the rest, if first_pass' write finished fully
        if (first_pass == num_edge_bytes) {
            const ssize_t second_pass = write(fd, rb->buffer_start, num - num_edge_bytes);
            if (second_pass == -1) {
                // TODO: log strerror
            } else {
                bytes_written += second_pass;
            }
        }
    } else {
        const ssize_t only_pass = write(fd, rb->data_start, num_bytes_to_read);
        if (only_pass == -1) {
            // TODO: log strerror
            return -1;
        }
        bytes_written += only_pass;
    }

    if (flag == PICTRL_READ_CONSUME) {
        rb->data_length -= (size_t)bytes_written;
        rb->data_start = rb->buffer_start + new_data_offset_start;
    }

    return bytes_written;
}

void pictrl_rb_clear(pictrl_rb_t *rb) {
    memset(rb->buffer_start, 0, rb->num_bytes*sizeof(uint8_t));
    rb->data_length = 0;
    rb->data_start = rb->buffer_start;
}
