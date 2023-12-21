#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "ring_buffer.h"

int test_simple_insert();
int test_simple_read_peek();
void print_ring_buffer(pictrl_rb_t*);
void print_ring_buffer_raw(pictrl_rb_t*);

int main(int argc, char *argv[]) {
	int ret = test_simple_insert();
	if (ret != 0) {
		return ret;
	}

	printf("\n");
	ret = test_simple_read_peek();
	if (ret != 0) {
		return ret;
	}

	return 0;
}

int test_simple_insert() {
	// Arrange
	const size_t ring_buf_size = 8;
	uint8_t data[] = { 4,9,5,6,1 };

	printf("Creating ring buffer\n");
	pictrl_rb_t ring_buffer;
	if (pictrl_rb_init(&ring_buffer, ring_buf_size) == NULL) {
		fprintf(stderr, "Could not create ring buffer\n");
		return 1;
	}
	print_ring_buffer(&ring_buffer);
	print_ring_buffer_raw(&ring_buffer);

	// Act
	size_t num_bytes_to_insert = sizeof(data);
	printf("Inserting %zu bytes\n", num_bytes_to_insert);
	if (pictrl_rb_insert(&ring_buffer, data, num_bytes_to_insert) != num_bytes_to_insert) {
		fprintf(stderr, "Error inserting\n");
		pictrl_rb_destroy(&ring_buffer);
		return 2;
	}
	print_ring_buffer(&ring_buffer);
	print_ring_buffer_raw(&ring_buffer);

	// Assert
	for (size_t cur_byte=0; cur_byte < num_bytes_to_insert; cur_byte++) {
		if (data[cur_byte] != ring_buffer.buffer_start[cur_byte]) {
			fprintf(stderr, "Data mismatch at index %zu\n", cur_byte);
			print_ring_buffer(&ring_buffer);
			print_ring_buffer_raw(&ring_buffer);

			pictrl_rb_destroy(&ring_buffer);
			return 3;
		}
	}
	printf("Data matches!\n");

	// Teardown
	printf("Destroying ring buffer\n");
	pictrl_rb_destroy(&ring_buffer);

	return 0;
}

int test_simple_read_peek() {
	// Arrange
	const size_t ring_buf_size = 8;
	uint8_t orig_data[] = { 4,9,5,6,1 };

	printf("Creating ring buffer\n");
	pictrl_rb_t ring_buffer;
	if (pictrl_rb_init(&ring_buffer, ring_buf_size) == NULL) {
		fprintf(stderr, "Could not create ring buffer\n");
		return 1;
	}
	print_ring_buffer(&ring_buffer);
	print_ring_buffer_raw(&ring_buffer);

	const size_t num_bytes_to_insert = sizeof(orig_data);
	printf("Inserting %zu bytes\n", num_bytes_to_insert);
	if (pictrl_rb_insert(&ring_buffer, orig_data, num_bytes_to_insert) != num_bytes_to_insert) {
		fprintf(stderr, "Error inserting\n");
		pictrl_rb_destroy(&ring_buffer);
		return 2;
	}

	// Act
	uint8_t read_data[sizeof(orig_data)] = { 0 };
	pictrl_rb_read(&ring_buffer, PICTRL_READ_PEEK, read_data, num_bytes_to_insert);
	print_ring_buffer(&ring_buffer);
	print_ring_buffer_raw(&ring_buffer);

	// Assert
	for (size_t cur_byte=0; cur_byte < num_bytes_to_insert; cur_byte++) {
		if (orig_data[cur_byte] != read_data[cur_byte]) {
			fprintf(stderr, "Data mismatch at index %zu\n", cur_byte);
			print_ring_buffer(&ring_buffer);
			print_ring_buffer_raw(&ring_buffer);

			pictrl_rb_destroy(&ring_buffer);
			return 3;
		}
	}
	printf("Data matches!\n");

	// Teardown
	printf("Destroying ring buffer\n");
	pictrl_rb_destroy(&ring_buffer);

	return 0;
}

// Using `pictrl_rb_read`
void print_ring_buffer(pictrl_rb_t *rb) {
	printf("------------------------------\n"
		   "Capacity:      %zu\t"
		   "Buffer length: %p\t"
		   "Data start:    %p\t"
		   "Data length:   %zu\n"
		   "Buffer:        |",
		   rb->num_bytes,
		   rb->buffer_start,
		   rb->data_start,
		   rb->data_length);

	if (rb->data_length == 0) {
		printf("(empty)|\n");
		return;
	}

	uint8_t *data = malloc(sizeof(uint8_t) * rb->data_length);
	pictrl_rb_read(rb, PICTRL_READ_PEEK, data, rb->data_length);

	size_t num_bytes_to_read = rb->data_length == 0 ? 0 : rb->data_length - 1;
	size_t cur = 0;
	while (cur < num_bytes_to_read) {
		printf("%u, ", data[cur]);
		cur++;
	}
	printf("%u|\n", data[cur]);
	free(data);
}

// Just print raw buffer
void print_ring_buffer_raw(pictrl_rb_t *rb) {
	printf("------------------------------\n"
		   "Capacity:      %zu\t"
		   "Buffer length: %p\t"
		   "Data start:    %p\t"
		   "Data length:   %zu\n"
		   "RAW buffer:    |",
		   rb->num_bytes,
		   rb->buffer_start,
		   rb->data_start,
		   rb->data_length);

	if (rb->data_length == 0) {
		printf("(empty)|\n");
		return;
	}

	size_t cur = 0;
	while (cur < rb->num_bytes) {
		printf("%u, ", rb->buffer_start[cur]);
		cur++;
	}
	printf("%u|\n", rb->buffer_start[cur]);
}
