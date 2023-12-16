#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "ring_buffer.h"

void print_ring_buffer(pictrl_rb_t*);
void print_ring_buffer_raw(pictrl_rb_t*);

int main(int argc, char *argv[]) {
	const size_t ring_buf_size = 8;
	uint8_t data[] = { 4,9,5,6,1 };

	pictrl_rb_t ring_buffer;
	if (pictrl_rb_init(&ring_buffer, ring_buf_size) == NULL) {
		return -1;
	}

	size_t num_bytes_to_insert = sizeof(data);
	printf("Inserting %zu bytes...\n", num_bytes_to_insert);
	if (pictrl_rb_insert(&ring_buffer, data, num_bytes_to_insert) != num_bytes_to_insert) {
		printf("Error inserting\n");
		return -1;
	}

	print_ring_buffer(&ring_buffer);
	print_ring_buffer_raw(&ring_buffer);

	printf("\nDestroying ring buffer...\n\n");
	pictrl_rb_destroy(&ring_buffer);

	print_ring_buffer(&ring_buffer);
	print_ring_buffer_raw(&ring_buffer);
	return 0;
}

// Using `pictrl_rb_read`
void print_ring_buffer(pictrl_rb_t *rb) {
	printf("------------------------------\n");
	printf("Capacity:      %zu\n", rb->num_bytes);
	printf("Buffer length: %p\n", rb->buffer_start);
	printf("Data start:    %p\n", rb->data_start);
	printf("Data length:   %zu\n", rb->data_length);

	printf("Buffer:        |");
	if (rb->data_length == 0) {
		printf("|\n");
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
	printf("------------------------------\n");
	printf("Capacity:      %zu\n", rb->num_bytes);
	printf("Buffer length: %p\n", rb->buffer_start);
	printf("Data start:    %p\n", rb->data_start);
	printf("Data length:   %zu\n", rb->data_length);

	printf("RAW buffer:    |");
	if (rb->data_length == 0) {
		printf("|\n");
		return;
	}

	size_t cur = 0;
	while (cur < rb->num_bytes) {
		printf("%u, ", rb->buffer_start[cur]);
		cur++;
	}
	printf("%u|\n", rb->buffer_start[cur]);
}
