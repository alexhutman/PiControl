#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "data_structures/ring_buffer.h"


pictrl_rb_t *pictrl_rb_init(pictrl_rb_t *rb, size_t num_bytes) {
    if (num_bytes == 0) {
        return NULL;
    }

    uint8_t *buf = calloc(num_bytes, sizeof(uint8_t)); // TODO: change to malloc?
    if (buf == NULL) {
        return NULL;
    }
    rb->buffer = buf;
    rb->num_bytes = num_bytes;
    rb->data_start = buf;
    rb->data_length = 0;

    return rb;
}


void pictrl_rb_destroy(pictrl_rb_t *rb) {
    free(rb->buffer);

    rb->buffer = NULL;
    rb->num_bytes = 0;
    rb->data_start = NULL;
    rb->data_length = 0;
}


/*
For all failures*, we return right after setting errno, or right after an unsuccessful read() call, where errno is set.
Therefore, if this call fails, you should be able to retrieve errno afterwards.

*When you're writing enough data that we wrap around the ring buffer's internal buffer,
we have to do 2 separate read() calls. If the second one fails, we still return how many bytes we wrote from the first read(), and do not error.
Since no data will have been written on the 2nd read(), if the error persists, you should see it in the next call to pictrl_rb_write() for the remaining `num - bytes_read` bytes.

*/
ssize_t pictrl_rb_write(int fd, size_t num, pictrl_rb_t *rb) {
    const size_t available_bytes = rb->num_bytes - rb->data_length;
    if (num == 0) {
        return 0;
    }
    if (available_bytes == 0) {
        errno = ENOBUFS;
        return -1;
    }

    const size_t num_bytes_to_write = (num > available_bytes) ? available_bytes : num; // if not enough space, write as much as we can
    const size_t data_offset = rb->data_start - rb->buffer; // offset of the start of the data section
    const size_t write_offset_start = (data_offset + rb->data_length) % rb->num_bytes; // next slot after data_end. offset from buffer
    const size_t write_offset_end = (write_offset_start + num_bytes_to_write - 1) % rb->num_bytes; // end of data that is to be written. offset from buffer

    const bool wrapped = write_offset_end < write_offset_start;
    ssize_t bytes_read = 0;
    if (wrapped) {
        // Write from the write offset to the end
        const size_t num_bytes_first_pass = rb->num_bytes - write_offset_start;
        const ssize_t first_pass = read(fd, rb->buffer + write_offset_start, num_bytes_first_pass);
        if (first_pass == -1) {
            return -1;
        }
        if (first_pass == 0) {
            return 0;
        }
        bytes_read += first_pass;

        // Then from the beginning to write_end
        if ((size_t)first_pass == num_bytes_first_pass) {
            const ssize_t second_pass = read(fd, rb->buffer, num_bytes_to_write - num_bytes_first_pass);
            if (second_pass != -1) { // If it is, what should we do? We already wrote some bytes...
                bytes_read += second_pass;
            }
        }
    } else {
        const ssize_t only_pass = read(fd, rb->buffer + write_offset_start, num_bytes_to_write);
        if (only_pass < 0) {
            return -1;
        }
        if (only_pass == 0) {
            return 0;
        }
        bytes_read += only_pass;
    }

    rb->data_length += (size_t)bytes_read;
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
    if (num == 0 || rb->data_length == 0) {
        return 0;
    }

    // If not enough data, read as much as we can
    const size_t num_bytes_to_read = (num > rb->data_length) ? rb->data_length : num;
    const size_t data_offset_start = rb->data_start - rb->buffer;
    const size_t data_offset_end = (data_offset_start + rb->data_length - 1) % rb->num_bytes;

    ssize_t bytes_written = 0;
    const bool wrapped = data_offset_end < data_offset_start;
    if (wrapped) {
        // Write 'til the end
        const size_t num_bytes_first_pass = rb->num_bytes - data_offset_start;
        const ssize_t first_pass = write(fd, rb->data_start, num_bytes_first_pass);
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
        const ssize_t only_pass = write(fd, rb->data_start, num_bytes_to_read);
        if (only_pass == -1) {
            return -1;
        }
        bytes_written += only_pass;
    }

    if (flag == PICTRL_READ_CONSUME) {
        const size_t new_data_offset_start = (data_offset_start + (size_t)bytes_written) % rb->num_bytes;
        rb->data_length -= (size_t)bytes_written;
        rb->data_start = rb->buffer + new_data_offset_start;
    }

    return bytes_written;
}

void pictrl_rb_clear(pictrl_rb_t *rb) {
    memset(rb->buffer, 0, rb->num_bytes*sizeof(uint8_t));
    rb->data_length = 0;
    rb->data_start = rb->buffer;
}

void pictrl_rb_copy(pictrl_rb_t *rb, void *dest) {
    if (!pictrl_rb_data_wrapped(rb)) {
        memcpy(dest, rb->data_start, rb->data_length*sizeof(uint8_t));
        return;
    }

    const size_t data_start_idx = rb->data_start - rb->buffer;
    const size_t num_bytes_first_pass = rb->num_bytes - data_start_idx;
    memcpy(dest, rb->data_start, num_bytes_first_pass*sizeof(uint8_t));
    memcpy(dest + num_bytes_first_pass, rb->buffer, (rb->data_length - num_bytes_first_pass)*sizeof(uint8_t));
}
