#include <stdio.h>

#include "ring_buffer.h"

int main(int argc, char *argv[]) {
	uint8_t data[] = { 4,9,5,6,1 };

	pictrl_rb_t ring_buffer;
	if (pictrl_rb_init(&ring_buffer, 8) == NULL) {
		return -1;
	}

	printf("Capacity: %zu\n", ring_buffer.num_bytes);
	printf("Data length: %zu\n", ring_buffer.data_length);

	size_t num_bytes_to_insert = 5;
	if (pictrl_rb_insert(&ring_buffer, data, num_bytes_to_insert) < 0) {
		printf("Error inserting\n");
	}

	printf("Capacity: %zu\n", ring_buffer.num_bytes);
	printf("Data length: %zu\n", ring_buffer.data_length);

	printf("Buffer: |");
	size_t data_i = 0;
	uint8_t cur_data;
	while (data_i < (num_bytes_to_insert - 1)) {
		size_t num_bytes_read = pictrl_rb_read(&ring_buffer, &cur_data, 1);
		if (num_bytes_read == 0) {
			printf("Read 0 bytes\n");
			return -1;
		}
		if (cur_data != data[data_i++]) {
			printf("Data mismatch at index %zu\n", data_i);
			return -1;
		}
		printf("%u, ", cur_data);
	}
	size_t num_bytes_read = pictrl_rb_read(&ring_buffer, &cur_data, 1);
	printf("%u|\n", cur_data);

	return 0;
}
