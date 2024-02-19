#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "data_structures/ring_buffer.h"


pictrl_rb_t *pictrl_rb_init(pictrl_rb_t *rb, size_t capacity) {
    if (capacity == 0) {
        return NULL;
    }

    uint8_t *buf = calloc(capacity, sizeof(uint8_t)); // TODO: change to malloc?
    if (buf == NULL) {
        return NULL;
    }
    rb->buffer = buf;
    rb->capacity = capacity;
    rb->data_start = 0;
    rb->num_items = 0;

    return rb;
}


void pictrl_rb_destroy(pictrl_rb_t *rb) {
    if (rb == NULL) {
        return;
    }
    free(rb->buffer);

    rb->buffer = NULL;
    rb->capacity = 0;
    rb->data_start = 0;
    rb->num_items = 0;
}


/*
For all failures*, we return right after setting errno, or right after an unsuccessful read() call, where errno is set.
Therefore, if this call fails, you should be able to retrieve errno afterwards.

*When you're writing enough data that we wrap around the ring buffer's internal buffer,
we have to do 2 separate read() calls. If the second one fails, we still return how many bytes we wrote from the first read(), and do not error.
Since no data will have been written on the 2nd read(), if the error persists, you should see it in the next call to pictrl_rb_write() for the remaining `num - bytes_read` bytes.

*/
ssize_t pictrl_rb_write(int fd, size_t num, pictrl_rb_t *rb) {
    const size_t available_bytes = rb->capacity - rb->num_items;
    if (num == 0) {
        return 0;
    }
    if (available_bytes == 0) {
        errno = ENOBUFS;
        return -1;
    }

    const size_t num_bytes_to_write = (num > available_bytes) ? available_bytes : num; // if not enough space, write as much as we can
    const size_t write_offset_start = (rb->data_start + rb->num_items) % rb->capacity; // next slot after data_end. offset from buffer
    const size_t write_offset_end = (write_offset_start + num_bytes_to_write - 1) % rb->capacity; // end of data that is to be written. offset from buffer

    const bool wrapped = write_offset_end < write_offset_start;
    ssize_t bytes_read = 0;
    if (wrapped) {
        // Write from the write offset to the end
        const size_t num_bytes_first_pass = rb->capacity - write_offset_start;
        const ssize_t first_pass = read(fd, rb->buffer + write_offset_start, num_bytes_first_pass);
        if (first_pass <= 0) {
            return first_pass;
        }
        bytes_read += first_pass;

        // Then from the beginning to write_end
        if ((size_t)first_pass == num_bytes_first_pass) {
            const ssize_t second_pass = read(fd, rb->buffer, num_bytes_to_write - num_bytes_first_pass);
            if (second_pass == -1) { // If it is, what should we do? We already wrote some bytes...
                return -1;
            }
            bytes_read += second_pass;
        }
    } else {
        const ssize_t only_pass = read(fd, rb->buffer + write_offset_start, num_bytes_to_write);
        if (only_pass <= 0) {
            return only_pass;
        }
        bytes_read += only_pass;
    }

    rb->num_items += (size_t)bytes_read;
    return bytes_read;
}

/*
For all failures*, we return right after setting errno, or right after an unsuccessful write() call, where errno is set.
Therefore, if this call fails, you should be able to retrieve errno afterwards.

*When you're reading enough data that we wrap around the ring buffer's internal buffer,
we have to do 2 separate write() calls. If the second one fails, we still return how many bytes we read from the first write(), and do not error.
Since no data will have been read on the 2nd write(), if the error persists, you should see it in the next call to pictrl_rb_read() for the remaining `num - bytes_written` bytes.

*/
ssize_t pictrl_rb_read(int fd, size_t num, pictrl_rb_t *rb, pictrl_read_flag flag) {
    if (num == 0 || rb->num_items == 0) {
        return 0;
    }

    // If not enough data, read as much as we can
    const size_t num_bytes_to_read = (num > rb->num_items) ? rb->num_items : num;
    const size_t data_offset_end = (rb->data_start + rb->num_items - 1) % rb->capacity;

    ssize_t bytes_written = 0;
    const bool wrapped = data_offset_end < rb->data_start;
    if (wrapped) {
        // Write 'til the end
        const size_t num_bytes_first_pass = rb->capacity - rb->data_start;
        const ssize_t first_pass = write(fd, pictrl_rb_data_start_address(rb), num_bytes_first_pass);
        if (first_pass == -1) {
            return -1;
        }
        bytes_written += first_pass;

        // Then resume from the beginning to attempt to write the rest, if first_pass' write finished fully
        if ((size_t)first_pass == num_bytes_first_pass) {
            const ssize_t second_pass = write(fd, rb->buffer, num - num_bytes_first_pass);
            if (second_pass != -1) {
                bytes_written += second_pass;
            }
        }
    } else {
        const ssize_t only_pass = write(fd, pictrl_rb_data_start_address(rb), num_bytes_to_read);
        if (only_pass == -1) {
            return -1;
        }
        bytes_written += only_pass;
    }

    if (flag == PICTRL_READ_CONSUME) {
        const size_t new_data_offset_start = (rb->data_start + (size_t)bytes_written) % rb->capacity;
        rb->num_items -= (size_t)bytes_written;
        rb->data_start = new_data_offset_start;
    }

    return bytes_written;
}

void pictrl_rb_clear(pictrl_rb_t *rb) {
    memset(rb->buffer, 0, rb->capacity*sizeof(uint8_t));
    rb->num_items = 0;
    rb->data_start = 0;
}

void pictrl_rb_copy(pictrl_rb_t *rb, void *dest) {
    if (!pictrl_rb_data_wrapped(rb)) {
        memcpy(dest, pictrl_rb_data_start_address(rb), rb->num_items*sizeof(uint8_t));
        return;
    }

    const size_t num_bytes_first_pass = rb->capacity - rb->data_start;
    memcpy(dest, pictrl_rb_data_start_address(rb), num_bytes_first_pass*sizeof(uint8_t));
    memcpy(dest + num_bytes_first_pass, rb->buffer, (rb->num_items - num_bytes_first_pass)*sizeof(uint8_t));
}
