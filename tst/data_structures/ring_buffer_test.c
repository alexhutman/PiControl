#include <stdlib.h>

#include "data_structures/ring_buffer.h"
#include "logging/log_utils.h"
#include "utils/pictrl_test_utils.h"


int test_simple_insert();
int test_simple_read_peek();
void print_ring_buffer(pictrl_rb_t*);
void print_nice_buf(pictrl_rb_t*);
void print_raw_buf(pictrl_rb_t*);
void print_buf(uint8_t*, size_t);


int main(int argc, char *argv[]) {
    const TestCase test_cases[] = {
        {
            .test_name = "Simple insert",
            .test_function = &test_simple_insert,
        },
        {
            .test_name = "Simple read (peek)",
            .test_function = &test_simple_read_peek,
        }
    };

    return run_test_suite(test_cases, pictrl_size(test_cases));
}

int test_simple_insert() {
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

int test_simple_read_peek() {
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

// Using `pictrl_rb_read`
void print_ring_buffer(pictrl_rb_t *rb) {
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

void print_nice_buf(pictrl_rb_t *rb) {
    if (rb->data_length == 0) {
        pictrl_log("(empty)\n");
        return;
    }

    uint8_t *data = malloc(sizeof(uint8_t) * rb->data_length);
    pictrl_rb_read(rb, PICTRL_READ_PEEK, data, rb->data_length);
    print_buf(data, rb->data_length);

    free(data);
}
void print_raw_buf(pictrl_rb_t *rb) {
    print_buf(rb->buffer_start, rb->data_length);
}

void print_buf(uint8_t *data, size_t n) {
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
