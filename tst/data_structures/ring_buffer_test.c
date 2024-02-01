#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "data_structures/ring_buffer.h"
#include "logging/log_utils.h"
#include "pitest/api.h"
#include "pitest/api/assertions.h"
#include "pitest/util/dummy.h"


static int test_simple_write();
static int test_simple_read_peek();
static int test_write_more_than_free();
static int test_simple_wraparound();
static int test_clear_full_buffer();

static void print_ring_buffer(pictrl_rb_t*);
static void print_rb_in_order(pictrl_rb_t*);
static void print_raw_buf(pictrl_rb_t*);
static void print_buf(uint8_t*, size_t);
static ssize_t read_until_completion(int fd, size_t count, pictrl_rb_t *rb, pictrl_read_flag flag);
static ssize_t write_until_completion(int fd, size_t count, pictrl_rb_t *rb);
static ssize_t read_from_file(int fd, size_t count, void *data);
static ssize_t write_to_file(int fd, size_t count, void *data);

#define TEST_FILE_TEMPLATE_PREFIX "./ring_buffer_testXXXXXX"
#define TEST_FILE_TEMPLATE_SUFFIX ".tmp"
#define TEST_FILE_TEMPLATE_SUFFIX_LEN (int) (sizeof(TEST_FILE_TEMPLATE_SUFFIX)/sizeof(char) - 1)
static char test_file_name[] = TEST_FILE_TEMPLATE_PREFIX TEST_FILE_TEMPLATE_SUFFIX;

// Fixtures
static int test_fd = -1;

int before_all() {
    test_fd = mkstemps(test_file_name, TEST_FILE_TEMPLATE_SUFFIX_LEN);
    if (test_fd < 0) {
        pictrl_log_error("Error creating temp file %s: %s\n", test_file_name, strerror(errno));
        return -1;
    }

    pictrl_log_debug("Created temp file %s\n", test_file_name);
    return 0;
}

int before_each() {
    // Clear out file
    if (lseek(test_fd, 0, SEEK_SET) < 0) {
        pictrl_log_error("Error seeking to beginning of temp file: %s\n", strerror(errno));
        return -1;
    }

    if (ftruncate(test_fd, 0) < 0) {
        pictrl_log_error("Error truncating temp file: %s\n", strerror(errno));
        return -1;
    }
    pictrl_log_debug("Truncated temp file\n");
    return 0;
}

int after_all() {
    // Close fd
    int ret = close(test_fd);
    if (ret < 0) {
        pictrl_log_error("Was not able to close open file descriptor %d: %s\n", test_fd, strerror(errno));
        return ret;
    }
    test_fd = -1;

    // Remove file
    ret = unlink(test_file_name);
    if (ret < 0) {
        pictrl_log_warn("Could not remove temp file %s: %s\n", test_file_name, strerror(errno)); // Error + return?
    } else {
        pictrl_log_debug("Removed temp file %s\n", test_file_name);
    }

    return ret;
}

int main() {
    const TestCase test_cases[] = {
        {
            .test_name = "Simple write",
            .test_function = &test_simple_write,
        },
        {
            .test_name = "Simple read (peek)",
            .test_function = &test_simple_read_peek,
        },
        {
            .test_name = "Write more than free",
            .test_function = &test_write_more_than_free,
        },
        {
            .test_name = "Simple wraparound",
            .test_function = &test_simple_wraparound,
        },
        {
            .test_name = "Clear full buffer",
            .test_function = &test_clear_full_buffer,
        }
    };

    const TestSuite suite = {
        .name = "Ring buffer tests",
        .test_cases = test_cases,
        .num_tests = pictrl_size(test_cases),
        .before_after_all = {
            .setup = &before_all,
            .teardown = &after_all
        },
        .before_after_each = {
            .setup = &before_each,
            .teardown = NULL
        }
    };

    return run_test_suite(&suite);
}

static int test_simple_write() {
    // Arrange
    const size_t ring_buf_size = 8;
    uint8_t data[] = { 4,9,5,6,1 };

    // Initialize ring buffer
    pictrl_rb_t rb;
    if (pictrl_rb_init(&rb, ring_buf_size) == NULL) {
        pictrl_log_error("Could not initialize ring buffer\n");
        return 1;
    }
    pictrl_log_debug("Initialized ring buffer to %zu bytes\n", ring_buf_size);

    // Write data to file
    size_t num_bytes_to_write = sizeof(data);
    if (write_to_file(test_fd, num_bytes_to_write, data) != (ssize_t)num_bytes_to_write) {
        pictrl_log_error("Could not write test data to temp file\n");
        return 1;
    }
    if (lseek(test_fd, 0, SEEK_SET) < 0) {
        pictrl_log_error("Error seeking to beginning of temp file: %s\n", strerror(errno));
        return -1;
    }
    pictrl_log_debug("Wrote test data to temp file\n");

    // Act
    if (write_until_completion(test_fd, num_bytes_to_write, &rb) != (ssize_t)num_bytes_to_write) {
        pictrl_log_error("Error writing to ring buffer\n");
        return 2;
    }
    pictrl_log_debug("Wrote %zu bytes\n", num_bytes_to_write);

    // Assert
    if (!array_equals(rb.buffer_start, rb.data_length,
                      data, num_bytes_to_write)) {
        // TODO: Make these logs all go to stderr.. d'oh
        pictrl_log_error("Data mismatch. Expected data: ");
        print_buf(data, num_bytes_to_write);
        pictrl_log_error("Received: ");
        print_buf(rb.buffer_start, num_bytes_to_write);
        return 3;
    }
    pictrl_log_debug("Data matches!\n");
    return 0;
}

static int test_simple_read_peek() {
    // Arrange
    const size_t ring_buf_size = 8;
    uint8_t orig_data[] = { 1,2,3,4,5,6,7 };
    const size_t num_bytes_to_read = sizeof(orig_data);

    pictrl_rb_t rb;
    if (pictrl_rb_init(&rb, ring_buf_size) == NULL) {
        pictrl_log_error("Could not create ring buffer\n");
        return 1;
    }
    pictrl_log_debug("Created ring buffer of size %zu bytes\n", ring_buf_size);

    // Populate ring buffer
    memcpy(rb.buffer_start, orig_data, num_bytes_to_read);
    rb.data_length = num_bytes_to_read;
    pictrl_log_debug("Populated ring buffer with original data\n");

    // Act
    if (read_until_completion(test_fd, num_bytes_to_read, &rb, PICTRL_READ_PEEK) != num_bytes_to_read) {
        pictrl_log_error("Error reading from ring buffer\n");
        return 2;
    }
    if (lseek(test_fd, 0, SEEK_SET) < 0) {
        pictrl_log_error("Error seeking to beginning of temp file: %s\n", strerror(errno));
        return -1;
    }
    pictrl_log_debug("Read %zu bytes\n", num_bytes_to_read);


    // Assert
    uint8_t read_data[sizeof(orig_data)] = { 0 };
    if (read_from_file(test_fd, num_bytes_to_read, read_data) != num_bytes_to_read) {
        pictrl_log_error("Error reading from file\n");
        return 2;
    }

    if (!array_equals(orig_data, num_bytes_to_read,
                      read_data, num_bytes_to_read)) {
        pictrl_log_error("Read data does not match original data.\nExpected data: ");
        print_buf(orig_data, num_bytes_to_read);
        pictrl_log_error("Received: ");
        print_buf(read_data, num_bytes_to_read);
        return 3;
    }

    if (!array_equals(orig_data, num_bytes_to_read,
                      rb.buffer_start, num_bytes_to_read)) {
        pictrl_log_error("Ring buffer data was somehow modified.\nExpected data: ");
        print_buf(orig_data, num_bytes_to_read);
        print_ring_buffer(&rb);
        return 3;
    }

    pictrl_log_debug("Data matches!\n");
    return 0;
}

int test_write_more_than_free() {
    // Arrange
    const size_t ring_buf_size = 4;
    uint8_t orig_data[] = { 1,2,3,4,5,6,7 };
    const size_t num_bytes_to_write = sizeof(orig_data);

    pictrl_rb_t rb;
    if (pictrl_rb_init(&rb, ring_buf_size) == NULL) {
        pictrl_log_error("Could not create ring buffer\n");
        return 1;
    }
    pictrl_log_debug("Created ring buffer of size %zu bytes\n", ring_buf_size);

    if (write_to_file(test_fd, num_bytes_to_write, orig_data) != num_bytes_to_write) {
        pictrl_log_error("Could not write test data to temp file\n");
        return 1;
    }
    if (lseek(test_fd, 0, SEEK_SET) < 0) {
        pictrl_log_error("Error seeking to beginning of temp file: %s\n", strerror(errno));
        return -1;
    }
    pictrl_log_debug("Wrote test data to temp file\n");

    // Act
    size_t num_bytes_written = write_until_completion(test_fd, num_bytes_to_write, &rb);
    if (errno != ENOBUFS) {
        pictrl_log_error("Expected errno to be set to ENOBUFS (%d), but errno is %d.\n", ENOBUFS, errno);
        return 1;
    }
    if (num_bytes_written != ring_buf_size) { // We should've capped out at the rb's capacity
        pictrl_log_error("Expected to write %zu bytes, but %zu bytes were somehow written\n", ring_buf_size, num_bytes_written);
        return 2;
    }
    pictrl_log_debug("Wrote %zu bytes\n", num_bytes_written);

    // Assert
    if (!array_equals(rb.buffer_start, rb.num_bytes,
                      orig_data, ring_buf_size)) {
        // TODO: Make these logs all go to stderr.. d'oh
        pictrl_log_error("Data mismatch. Expected data: ");
        print_buf(orig_data, ring_buf_size);
        pictrl_log_error("Received: ");
        print_buf(rb.buffer_start, ring_buf_size);
        return 3;
    }
    // TODO: Check that rest of array is in the file
    pictrl_log_debug("Data matches!\n");
    return 0;
}

int test_simple_wraparound() {
    // Arrange
    uint8_t orig_data[] = { 1,2,3,4 };
    const size_t ring_buf_size = sizeof(orig_data);

    // Init rb
    pictrl_rb_t rb;
    if (pictrl_rb_init(&rb, ring_buf_size) == NULL) {
        pictrl_log_error("Could not create ring buffer\n");
        return 1;
    }
    pictrl_log_debug("Created ring buffer of size %zu bytes\n", ring_buf_size);

    // Write data to file
    if (write_to_file(test_fd, ring_buf_size, orig_data) != ring_buf_size) {
        pictrl_log_error("Could not write test data to temp file\n");
        return 1;
    }
    if (lseek(test_fd, 0, SEEK_SET) < 0) {
        pictrl_log_error("Error seeking to beginning of temp file: %s\n", strerror(errno));
        return -1;
    }
    pictrl_log_debug("Wrote test data to temp file\n");

    // Act
    rb.data_start += ring_buf_size - 1;
    const ssize_t num_bytes_written = write_until_completion(test_fd, ring_buf_size, &rb);
    if (num_bytes_written != ring_buf_size) {
        pictrl_log_error("Expected to write %zu bytes, but %zd bytes were somehow written\n", ring_buf_size, num_bytes_written);
        return 2;
    }
    pictrl_log_debug("Wrote %zd bytes starting at the last position in the internal buffer\n", num_bytes_written);

    // Assert
    uint8_t expected_data[] = { 2,3,4,1 };
    if (!array_equals(rb.buffer_start, rb.num_bytes,
                      expected_data, rb.num_bytes)) {
        // TODO: Make these logs all go to stderr.. d'oh
        pictrl_log_error("Data mismatch. Expected data: ");
        print_buf(expected_data, ring_buf_size);
        pictrl_log_error("Received: ");
        print_buf(rb.buffer_start, ring_buf_size);

        return 3;
    }
    pictrl_log_debug("Data matches!\n");
    return 0;
}

int test_clear_full_buffer() {
    // Arrange
    uint8_t orig_data[] = { 0,1,2,3,4,5,6,7,8,9 };
    const size_t ring_buf_size = sizeof(orig_data);

    // Initialize ring buffer
    pictrl_rb_t rb;
    if (pictrl_rb_init(&rb, ring_buf_size) == NULL) {
        pictrl_log_error("Could not create ring buffer\n");
        return -1;
    }
    pictrl_log_debug("Initialized ring buffer of size %zu bytes\n", ring_buf_size);

    // Populate ring buffer
    memcpy(rb.buffer_start, orig_data, ring_buf_size);
    rb.data_length = ring_buf_size;
    pictrl_log_debug("Populated ring buffer with original data\n");

    // Act
    pictrl_rb_clear(&rb);
    pictrl_log_debug("Cleared ring buffer\n");

    // Assert
    uint8_t expected_data[sizeof(orig_data)] = { 0 };
    if (!array_equals(rb.buffer_start, rb.num_bytes,
                      expected_data, ring_buf_size)) {
        // TODO: Make these logs all go to stderr.. d'oh
        pictrl_log_error("Data mismatch. Expected data: ");
        print_buf(expected_data, ring_buf_size);
        pictrl_log_error("Received: ");
        print_buf(rb.buffer_start, ring_buf_size);
        return 3;
    } else if (rb.data_length != 0) {
        pictrl_log_error("Expected data_length to be 0. Received: %zu", rb.data_length);
        return 4;
    } else if (rb.data_start != rb.buffer_start) {
        pictrl_log_error("Expected data_start to be the same as buffer_start. Received (data_start, buffer_start): (%p, %p)", rb.data_start, rb.buffer_start);
        return 5;
    }

    pictrl_log_debug("Buffer is empty as expected\n");
    return 0;
}

// Using `pictrl_rb_read`
static void print_ring_buffer(pictrl_rb_t *rb) {
    pictrl_log("\n------------------------------\n"
           "Capacity:     %zu\n"
           "Buffer start: %p\n"
           "Data start:   %p\n"
           "Data length:  %zu\n"
           "Buffer:       ",
           rb->num_bytes,
           rb->buffer_start,
           rb->data_start,
           rb->data_length);

    print_rb_in_order(rb);
    pictrl_log("RAW buffer:   ");
    print_raw_buf(rb);
    pictrl_log("\n");
}

static void print_rb_in_order(pictrl_rb_t *rb) {
    const size_t data_offset = rb->data_start - rb->buffer_start;
    const size_t n_first_pass = rb->num_bytes - data_offset;

    pictrl_log("[");
    for (size_t cur = 0; cur < n_first_pass; cur++) {
        pictrl_log("%u, ", rb->data_start[cur]);
    }
    for (size_t cur = 0; cur < data_offset - 1; cur++) {
        pictrl_log("%u, ", rb->buffer_start[cur]);
    }
    pictrl_log("%u]\n", rb->buffer_start[data_offset - 1]);
}
static void print_raw_buf(pictrl_rb_t *rb) {
    print_buf(rb->buffer_start, rb->data_length);
}

static void print_buf(uint8_t *data, size_t n) {
    if (n == 0) {
        pictrl_log("(empty)\n");
        return;
    }

    pictrl_log("|");
    for (size_t cur = 0; cur < n - 1; cur++) {
        pictrl_log("%u, ", data[cur]);
    }
    pictrl_log("%u|\n", data[n - 1]);
}

// These are surely not thread-safe
static ssize_t read_until_completion(int fd, size_t count, pictrl_rb_t *rb, pictrl_read_flag flag) {
    const bool reading_more_than_avail = count > rb->data_length;

    size_t bytes_read = 0;
    while (bytes_read < count) {
        const size_t bytes_left = count - bytes_read;
        const bool no_data_left = rb->data_length <= bytes_left;
        const ssize_t num_read = pictrl_rb_read(fd, bytes_left, rb, flag);
        if (num_read < 0) {
            int err = errno;
            if (!reading_more_than_avail || !no_data_left) {
                pictrl_log_error("Error reading from temp file: %s\n", strerror(err));
                errno = err;
                return -1;
            }
            return bytes_read;
        }
        bytes_read += (size_t)num_read;
    }
    return (ssize_t)bytes_read;
}

static ssize_t write_until_completion(int fd, size_t count, pictrl_rb_t *rb) {
    const bool inserting_more_than_avail = count > (rb->num_bytes - rb->data_length);

    size_t bytes_written = 0;
    while (bytes_written < count) {
        const size_t bytes_left = count - bytes_written;
        const bool is_rb_full = (rb->num_bytes - rb->data_length) <= bytes_left ;

        const ssize_t written = pictrl_rb_write(fd, bytes_left, rb);
        if (written < 0) {
            int err = errno;
            if (!inserting_more_than_avail || !is_rb_full) {
                pictrl_log_error("Error writing to temp file: %s\n", strerror(err));
                errno = err;
                return -1;
            }
            return bytes_written;
        }
        if (written == 0) {
            return bytes_written;
        }
        bytes_written += (size_t)written;
    }
    return (ssize_t)(bytes_written);
}

static ssize_t read_from_file(int fd, size_t count, void *data) {
    void *cur = data;
    size_t remaining_bytes = count;
    while (remaining_bytes > 0) {
        ssize_t num_read = read(fd, cur, remaining_bytes);
        if (num_read == -1) {
            // TODO: handle err
            return -1;
        }
        if (num_read == 0) {
            break;
        }
        cur += (size_t)num_read;
        remaining_bytes -= (size_t)num_read;
    }
    return (ssize_t)(count - remaining_bytes);
}

static ssize_t write_to_file(int fd, size_t count, void *data) {
    void *cur = data;
    size_t remaining_bytes = count;
    while (remaining_bytes > 0) {
        ssize_t num_written = write(fd, cur, remaining_bytes);
        if (num_written < 0) {
            return -1;
        }
        cur += (size_t)num_written;
        remaining_bytes -= (size_t)num_written;
    }
    return (ssize_t)(count - remaining_bytes);
}
