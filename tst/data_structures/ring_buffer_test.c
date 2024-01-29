#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "data_structures/ring_buffer.h"
#include "logging/log_utils.h"
#include "pitest/api.h"
#include "pitest/api/assertions.h"


static int test_simple_insert();
static int test_simple_read_peek();
static int test_insert_more_than_free();
static int test_simple_wraparound();
static int test_clear_full_buffer();
static void print_ring_buffer(pictrl_rb_t*);
static void print_nice_buf(pictrl_rb_t*);
static void print_raw_buf(pictrl_rb_t*);
static void print_buf(uint8_t*, size_t);


#define TEST_FILE_TEMPLATE_PREFIX "./ring_buffer_testXXXXXX"
#define TEST_FILE_TEMPLATE_SUFFIX ".tmp"
#define TEST_FILE_TEMPLATE_SUFFIX_LEN (int) (sizeof(TEST_FILE_TEMPLATE_SUFFIX)/sizeof(char) - 1)

static char test_file_name[] = TEST_FILE_TEMPLATE_PREFIX TEST_FILE_TEMPLATE_SUFFIX;
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

int after_all() {
    // Close fd
    int ret = close(test_fd);
    if (ret < 0) {
        pictrl_log_warn("Was not able to close open file descriptor %d: %s\n", test_fd, strerror(errno));
        return ret;
    }

    // Remove file
    ret = unlink(test_file_name);
    if (ret < 0) {
        pictrl_log_warn("Could not remove temp file %s: %s\n", test_file_name, strerror(errno));
        return ret;
    }

    test_fd = -1;
    pictrl_log_debug("Removed temp file %s\n", test_file_name);
    return ret;
}

int main() {
    const TestCase test_cases[] = {
        {
            .test_name = "Simple insert",
            .test_function = &test_simple_insert,
        },
        {
            .test_name = "Simple read (peek)",
            .test_function = &test_simple_read_peek,
        },
        {
            .test_name = "Insert more than free",
            .test_function = &test_insert_more_than_free,
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
            .setup = NULL,
            .teardown = NULL
        }
    };

    return run_test_suite(&suite);
}

static int test_simple_insert() {
    // Arrange
    const size_t ring_buf_size = 8;
    uint8_t data[] = { 4,9,5,6,1 };

    pictrl_log_debug("Creating ring buffer of size %zu bytes\n", ring_buf_size);
    pictrl_rb_t ring_buffer;
    if (pictrl_rb_init(&ring_buffer, ring_buf_size) == NULL) {
        pictrl_log_error("Could not create ring buffer\n");
        return 1;
    }

    // Act
    size_t num_bytes_to_insert = sizeof(data);
    pictrl_log_debug("Inserting %zu bytes\n", num_bytes_to_insert);
    if (pictrl_rb_insert(&ring_buffer, data, num_bytes_to_insert) != num_bytes_to_insert) {
        pictrl_log_error("Error inserting\n");
        pictrl_rb_destroy(&ring_buffer);
        return 2;
    }

    // Assert
    for (size_t cur_byte=0; cur_byte < num_bytes_to_insert; cur_byte++) {
        if (data[cur_byte] != ring_buffer.buffer_start[cur_byte]) {
            pictrl_log_error("Data mismatch at index %zu\nExpected data: ", cur_byte);
            print_buf(data, num_bytes_to_insert);
            print_ring_buffer(&ring_buffer);

            pictrl_rb_destroy(&ring_buffer);
            return 3;
        }
    }
    pictrl_log_debug("Data matches!\n");

    // Teardown
    pictrl_log_debug("Destroying ring buffer\n");
    pictrl_rb_destroy(&ring_buffer);

    return 0;
}

static int test_simple_read_peek() {
    // Arrange
    const size_t ring_buf_size = 8;
    uint8_t orig_data[] = { 1,2,3,4,5,6,7 };

    pictrl_log_debug("Creating ring buffer of size %zu bytes\n", ring_buf_size);
    pictrl_rb_t ring_buffer;
    if (pictrl_rb_init(&ring_buffer, ring_buf_size) == NULL) {
        pictrl_log_error("Could not create ring buffer\n");
        return 1;
    }

    const size_t num_bytes_to_insert = sizeof(orig_data);
    pictrl_log_debug("Inserting %zu bytes\n", num_bytes_to_insert);
    if (pictrl_rb_insert(&ring_buffer, orig_data, num_bytes_to_insert) != num_bytes_to_insert) {
        pictrl_log_error("Error inserting\n");
        pictrl_rb_destroy(&ring_buffer);
        return 2;
    }

    // Act
    uint8_t read_data[sizeof(orig_data)] = { 0 };
    pictrl_rb_read(&ring_buffer, PICTRL_READ_PEEK, read_data, num_bytes_to_insert);

    // Assert
    for (size_t cur_byte=0; cur_byte < num_bytes_to_insert; cur_byte++) {
        if (orig_data[cur_byte] != read_data[cur_byte]) {
            pictrl_log_error("Data mismatch at index %zu\nExpected data: ", cur_byte);
            print_buf(orig_data, num_bytes_to_insert);
            print_ring_buffer(&ring_buffer);

            pictrl_rb_destroy(&ring_buffer);
            return 3;
        }
    }
    pictrl_log_debug("Data matches!\n");

    // Teardown
    pictrl_log_debug("Destroying ring buffer\n");
    pictrl_rb_destroy(&ring_buffer);

    return 0;
}

int test_insert_more_than_free() {
    // Arrange
    const size_t ring_buf_size = 4;
    uint8_t orig_data[] = { 1,2,3,4,5,6,7 };

    pictrl_log_debug("Creating ring buffer of size %zu bytes\n", ring_buf_size);
    pictrl_rb_t rb;
    if (pictrl_rb_init(&rb, ring_buf_size) == NULL) {
        pictrl_log_error("Could not create ring buffer\n");
        return 1;
    }

    // Act
    const size_t num_bytes_to_insert = sizeof(orig_data);
    pictrl_log_debug("Attempting to insert %zu bytes\n", num_bytes_to_insert);
    size_t num_bytes_inserted = pictrl_rb_insert(&rb, orig_data, num_bytes_to_insert);
    if (num_bytes_inserted != ring_buf_size) { // We should've capped out at the rb's capacity
        pictrl_log_error("Expected to insert %zu bytes, but %zu bytes were somehow inserted\n", ring_buf_size, num_bytes_inserted);
        pictrl_rb_destroy(&rb);
        return 2;
    }
    pictrl_log_debug("Inserted %zu bytes\n", num_bytes_inserted);

    // Assert
    if (!array_equals(rb.buffer_start, rb.num_bytes,
                      orig_data, rb.num_bytes)) {
        // TODO: Make these logs all go to stderr.. d'oh
        pictrl_log_error("Data mismatch. Expected data: ");
        print_buf(orig_data, ring_buf_size);
        pictrl_log_error("Received: ");
        print_buf(rb.buffer_start, ring_buf_size);

        pictrl_rb_destroy(&rb);
        return 3;
    }
    pictrl_log_debug("Data matches!\n");

    // Teardown
    pictrl_log_debug("Destroying ring buffer\n");
    pictrl_rb_destroy(&rb);

    return 0;
}

int test_simple_wraparound() {
    // Arrange
    uint8_t orig_data[] = { 1,2,3,4 };
    const size_t ring_buf_size = sizeof(orig_data);

    pictrl_log_debug("Creating ring buffer of size %zu bytes\n", ring_buf_size);
    pictrl_rb_t rb;
    if (pictrl_rb_init(&rb, ring_buf_size) == NULL) {
        pictrl_log_error("Could not create ring buffer\n");
        return 1;
    }

    // Act
    rb.data_start += ring_buf_size - 1;
    const size_t num_bytes_to_insert = ring_buf_size;
    pictrl_log_debug("Attempting to insert %zu bytes starting at the last position in the internal buffer\n", num_bytes_to_insert);
    size_t num_bytes_inserted = pictrl_rb_insert(&rb, orig_data, num_bytes_to_insert);
    if (num_bytes_inserted != ring_buf_size) {
        pictrl_log_error("Expected to insert %zu bytes, but %zu bytes were somehow inserted\n", ring_buf_size, num_bytes_inserted);
        pictrl_rb_destroy(&rb);
        return 2;
    }
    pictrl_log_debug("Inserted %zu bytes\n", num_bytes_inserted);

    // Assert
    uint8_t expected_data[] = { 2,3,4,1 };
    if (!array_equals(rb.buffer_start, rb.num_bytes,
                      expected_data, rb.num_bytes)) {
        // TODO: Make these logs all go to stderr.. d'oh
        pictrl_log_error("Data mismatch. Expected data: ");
        print_buf(expected_data, ring_buf_size);
        pictrl_log_error("Received: ");
        print_buf(rb.buffer_start, ring_buf_size);

        pictrl_rb_destroy(&rb);
        return 3;
    }
    pictrl_log_debug("Data matches!\n");

    // Teardown
    pictrl_log_debug("Destroying ring buffer\n");
    pictrl_rb_destroy(&rb);

    return 0;
}

int test_clear_full_buffer() {
    // Arrange
    uint8_t orig_data[] = { 0,1,2,3,4,5,6,7,8,9 };
    const size_t ring_buf_size = sizeof(orig_data);

    pictrl_log_debug("Creating ring buffer of size %zu bytes\n", ring_buf_size);
    pictrl_rb_t rb;
    int ret = -1;
    if (pictrl_rb_init(&rb, ring_buf_size) == NULL) {
        pictrl_log_error("Could not create ring buffer\n");
        ret = 1;
    }

    const size_t num_bytes_to_insert = ring_buf_size;
    pictrl_log_debug("Inserting %zu bytes into the ring buffer\n", num_bytes_to_insert);
    size_t num_bytes_inserted = pictrl_rb_insert(&rb, orig_data, num_bytes_to_insert);
    if (num_bytes_inserted != ring_buf_size) {
        pictrl_log_error("Expected to insert %zu bytes, but %zu bytes were somehow inserted\n", ring_buf_size, num_bytes_inserted);
        ret = 2;
    }
    pictrl_log_debug("Inserted %zu bytes\n", num_bytes_inserted);

    // Act
    pictrl_rb_clear(&rb);
    pictrl_log_debug("Cleared ring buffer\n");

    // Assert
    uint8_t expected_data[sizeof(orig_data)] = { 0 };
    if (!array_equals(rb.buffer_start, rb.num_bytes,
                      expected_data, rb.num_bytes)) {
        // TODO: Make these logs all go to stderr.. d'oh
        pictrl_log_error("Data mismatch. Expected data: ");
        print_buf(expected_data, ring_buf_size);
        pictrl_log_error("Received: ");
        print_buf(rb.buffer_start, ring_buf_size);
        ret = 3;
    } else if (rb.data_length != 0) {
        pictrl_log_error("Expected data_length to be 0. Received: %zu", rb.data_length);
        ret = 4;
    } else if (rb.data_start != rb.buffer_start) {
        pictrl_log_error("Expected data_start to be the same as buffer_start. Received (data_start, buffer_start): (%p, %p)", rb.data_start, rb.buffer_start);
        ret = 5;
    } else {
        pictrl_log_debug("Buffer is empty as expected\n");
        ret = 0;
    }

    // Teardown
    pictrl_log_debug("Destroying ring buffer\n");
    pictrl_rb_destroy(&rb);

    return ret;
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

    print_nice_buf(rb);
    pictrl_log("RAW buffer:   ");
    print_raw_buf(rb);
    pictrl_log("\n");
}

static void print_nice_buf(pictrl_rb_t *rb) {
    if (rb->data_length == 0) {
        pictrl_log("(empty)\n");
        return;
    }

    uint8_t *data = malloc(sizeof(uint8_t) * rb->data_length);
    pictrl_rb_read(rb, PICTRL_READ_PEEK, data, rb->data_length);
    print_buf(data, rb->data_length);

    free(data);
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
    size_t cur = 0;
    while (cur < n - 1) {
        pictrl_log("%u, ", data[cur]);
        cur++;
    }
    pictrl_log("%u|\n", data[cur]);
}
