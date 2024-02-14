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
static ssize_t rb_read_until_completion(int fd, size_t count, pictrl_rb_t *rb, pictrl_read_flag flag);
static ssize_t rb_write_until_completion(int fd, size_t count, pictrl_rb_t *rb);
static size_t read_from_test_file(uint8_t *data, size_t count);
static size_t write_to_test_file(uint8_t *data, size_t count);

#define TEST_FILE_TEMPLATE_PREFIX "./ring_buffer_testXXXXXX"
#define TEST_FILE_TEMPLATE_SUFFIX ".tmp"
#define TEST_FILE_TEMPLATE_SUFFIX_LEN (int) (sizeof(TEST_FILE_TEMPLATE_SUFFIX)/sizeof(char) - 1)
static char test_file_name[] = TEST_FILE_TEMPLATE_PREFIX TEST_FILE_TEMPLATE_SUFFIX;

// Fixtures
static FILE *test_file = NULL;

int before_all() {
    const int test_fd = mkstemps(test_file_name, TEST_FILE_TEMPLATE_SUFFIX_LEN);
    if (test_fd < 0) {
        pictrl_log_error("Error creating temp file %s: %s\n", test_file_name, strerror(errno));
        return -1;
    }

    FILE *test_fp = fdopen(test_fd, "w+");
    if (test_fp == NULL) {
        pictrl_log_error("Error getting FILE pointer for open file %s: %s\n", test_file_name, strerror(errno));
        return -1;
    }

    test_file = test_fp;
    pictrl_log_debug("Created temp file %s\n", test_file_name);
    return 0;
}

int before_each() {
    // Clear out file
    rewind(test_file);
    if (ftruncate(fileno(test_file), 0) < 0) {
        pictrl_log_error("Error truncating temp file: %s\n", strerror(errno));
        return -1;
    }
    pictrl_log_debug("Truncated temp file\n");
    return 0;
}

int after_all() {
    // Close test file
    int ret = fclose(test_file);
    if (ret != 0) {
        pictrl_log_error("Was not able to close temp file %s: %s\n", test_file_name, strerror(errno));
        return ret;
    }
    test_file = NULL;

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

    const size_t num_bytes_to_write = sizeof(data);
    if (write_to_test_file(data, num_bytes_to_write) < num_bytes_to_write) {
        return 1;
    }
    rewind(test_file);
    pictrl_log_debug("Wrote all %zu bytes of test data to temp file\n", num_bytes_to_write);

    // Act
    const int test_fd = fileno(test_file);
    if (rb_write_until_completion(test_fd, num_bytes_to_write, &rb) != (ssize_t)num_bytes_to_write) {
        //pictrl_log_error("Error writing to ring buffer\n");
        return 2;
    }
    pictrl_log_debug("Wrote %zu bytes to ring buffer\n", num_bytes_to_write);

    // Assert
    if (!array_equals(rb.buffer, rb.num_items,
                      data, num_bytes_to_write)) {
        // TODO: Make these logs all go to stderr.. d'oh
        pictrl_log_error("Data mismatch. Expected data: ");
        print_buf(data, num_bytes_to_write);
        pictrl_log_error("Received: ");
        print_buf(rb.buffer, num_bytes_to_write);
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
    memcpy(rb.buffer, orig_data, num_bytes_to_read);
    rb.num_items = num_bytes_to_read;
    pictrl_log_debug("Populated ring buffer with %zu bytes of original data\n", num_bytes_to_read);

    // Act
    const int test_fd = fileno(test_file);
    if (rb_read_until_completion(test_fd, num_bytes_to_read, &rb, PICTRL_READ_PEEK) != num_bytes_to_read) {
        pictrl_log_error("Error reading from ring buffer\n");
        return 2;
    }
    rewind(test_file);
    pictrl_log_debug("Read all %zu bytes from ring buffer\n", num_bytes_to_read);


    // Assert
    uint8_t read_data[sizeof(orig_data)] = { 0 };
    if (read_from_test_file(read_data, num_bytes_to_read) < num_bytes_to_read) {
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
                      rb.buffer, num_bytes_to_read)) {
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

    if (write_to_test_file(orig_data, num_bytes_to_write) < num_bytes_to_write) {
        return 1;
    }
    rewind(test_file);
    pictrl_log_debug("Wrote test data to temp file\n");

    // Act
    const int test_fd = fileno(test_file);
    size_t num_bytes_written = rb_write_until_completion(test_fd, num_bytes_to_write, &rb);
    if (errno != ENOBUFS) {
        pictrl_log_error("Expected errno to be set to ENOBUFS (%d), but errno is %d\n", ENOBUFS, errno);
        return 1;
    }
    if (num_bytes_written != ring_buf_size) { // We should've capped out at the rb's capacity
        pictrl_log_error("Expected to write %zu bytes to ring buffer, but %zu bytes were somehow written\n", ring_buf_size, num_bytes_written);
        return 2;
    }
    pictrl_log_debug("Wrote all %zu bytes to ring buffer\n", num_bytes_written);

    // Assert
    if (!array_equals(rb.buffer, rb.capacity,
                      orig_data, ring_buf_size)) {
        // TODO: Make these logs all go to stderr.. d'oh
        pictrl_log_error("Data mismatch. Expected data: ");
        print_buf(orig_data, ring_buf_size);
        pictrl_log_error("Received: ");
        print_buf(rb.buffer, ring_buf_size);
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

    if (write_to_test_file(orig_data, ring_buf_size) < ring_buf_size) {
        return 1;
    }
    rewind(test_file);
    pictrl_log_debug("Wrote test data to temp file\n");

    // Act
    rb.data_start += ring_buf_size - 1;
    const int test_fd = fileno(test_file);
    const ssize_t num_bytes_written = rb_write_until_completion(test_fd, ring_buf_size, &rb);
    if (num_bytes_written != ring_buf_size) {
        pictrl_log_error("Expected to write %zu bytes, but %zd bytes were somehow written\n", ring_buf_size, num_bytes_written);
        return 2;
    }
    pictrl_log_debug("Wrote all %zd bytes to ring buffer, starting at the last position in the internal buffer\n", num_bytes_written);

    // Assert
    uint8_t expected_data[] = { 2,3,4,1 };
    if (!array_equals(rb.buffer, rb.capacity,
                      expected_data, rb.capacity)) {
        // TODO: Make these logs all go to stderr.. d'oh
        pictrl_log_error("Data mismatch. Expected data: ");
        print_buf(expected_data, ring_buf_size);
        pictrl_log_error("Received: ");
        print_buf(rb.buffer, ring_buf_size);

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
    memcpy(rb.buffer, orig_data, ring_buf_size);
    rb.num_items = ring_buf_size;
    pictrl_log_debug("Populated ring buffer with original data\n");

    // Act
    pictrl_rb_clear(&rb);
    pictrl_log_debug("Cleared ring buffer\n");

    // Assert
    uint8_t expected_data[sizeof(orig_data)] = { 0 };
    if (!array_equals(rb.buffer, rb.capacity,
                      expected_data, ring_buf_size)) {
        // TODO: Make these logs all go to stderr.. d'oh
        pictrl_log_error("Data mismatch. Expected data: ");
        print_buf(expected_data, ring_buf_size);
        pictrl_log_error("Received: ");
        print_buf(rb.buffer, ring_buf_size);
        return 3;
    } else if (rb.num_items != 0) {
        pictrl_log_error("Expected num_items to be 0. Received: %zu", rb.num_items);
        return 4;
    } else if (rb.data_start != rb.buffer) {
        pictrl_log_error("Expected data_start to be the same as buffer. Received (data_start, buffer): (%p, %p)", rb.data_start, rb.buffer);
        return 5;
    }

    pictrl_log_debug("Ring buffer is empty as expected\n");
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
           rb->capacity,
           rb->buffer,
           rb->data_start,
           rb->num_items);

    print_rb_in_order(rb);
    pictrl_log("RAW buffer:   ");
    print_raw_buf(rb);
    pictrl_log("\n");
}

static void print_rb_in_order(pictrl_rb_t *rb) {
    const size_t data_offset = rb->data_start - rb->buffer;
    const size_t n_first_pass = rb->capacity - data_offset;

    pictrl_log("[");
    for (size_t cur = 0; cur < n_first_pass; cur++) {
        pictrl_log("%u, ", rb->data_start[cur]);
    }
    for (size_t cur = 0; cur < data_offset - 1; cur++) {
        pictrl_log("%u, ", rb->buffer[cur]);
    }
    pictrl_log("%u]\n", rb->buffer[data_offset - 1]);
}
static void print_raw_buf(pictrl_rb_t *rb) {
    print_buf(rb->buffer, rb->num_items);
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
static ssize_t rb_read_until_completion(int fd, size_t count, pictrl_rb_t *rb, pictrl_read_flag flag) {
    const bool reading_more_than_avail = count > rb->num_items;

    size_t bytes_read = 0;
    while (bytes_read < count) {
        const size_t bytes_left = count - bytes_read;
        const bool no_data_left = rb->num_items <= bytes_left;
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

static ssize_t rb_write_until_completion(int fd, size_t count, pictrl_rb_t *rb) {
    const bool inserting_more_than_avail = count > (rb->capacity - rb->num_items);

    size_t bytes_written = 0;
    while (bytes_written < count) {
        const size_t bytes_left = count - bytes_written;
        const bool is_rb_full = (rb->capacity - rb->num_items) <= bytes_left ;

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
    return (ssize_t)bytes_written;
}

static size_t read_from_test_file(uint8_t *data, size_t count) {
    const size_t num_read = fread(data, sizeof(data[0]), count, test_file);
    if (num_read < count) {
        // Should prob log the exact error...
        pictrl_log_error("Error reading %zu bytes from test file... read %zu bytes instead\n", count, num_read);
        return 0;
    }
    return num_read;
}

static size_t write_to_test_file(uint8_t *data, size_t count) {
    const size_t num_written = fwrite(data, sizeof(data[0]), count, test_file);
    if (num_written < count) {
        // Should prob log the exact error...
        pictrl_log_error("Error writing %zu bytes to test file... wrote %zu bytes instead\n", count, num_written);
        return 0;
    }
    return num_written;
}
