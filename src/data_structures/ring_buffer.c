#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ring_buffer.h"

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


// Insert `num` bytes from `src_start` into the ring buffer
int pictrl_rb_insert(pictrl_rb_t *rb, void *src_start, size_t num) {
	// 2(?) cases (e.g. inserting 8 bytes):
	//	 D=Data, I=Insert
	//   (Case 1) Insert data contiguously:
		//   (Case 1a) Data doesn't wrap:
		//          4   10 11     18
		//          v     vv      v
		//     |----DDDDDDDIIIIIIII--|
		//                 ^
		//                 |start
		//   (Case 1b) Data wraps around:
		//     |DDDIIIIIIIIIIII--DDDD|
		//         ^
		//         |start

	//   (TODO: SHOULD I JUST NOT ALLOW THIS?)
	//   Leaning towards not allowing, because data will be annoying to iterate through at the wrap point
	//
	//   (Case 2) Insert data that wraps around:
		//   (Case 2a) Allowed:
		//               9   15  16
		//               v     vv
		//     |III------DDDDDDDIIIII|
		//                      ^
		//                      |start
		//   (Case 2b) NOT allowed:
		//     |DDDDDDDDDDIIIIIIIIIII|
		//      III       ^
		//                |start

	// check bounds..
		// n_bytes = 499-130+1=370
		// ins_offset = (499-130+1)%n_bytes = 0
	// if:
	//   130             495             499
	//    _               -               _
	//	|DDD|DDD|___|___|___|___|___|___|___|
	//	                  ^               ^
	//                    |data_start     |data_end
	//
	//    ^   ^   ^                       ^
	//    |   |   |insertion_point        |
	//    |   |data_end                   |buf_end
	//    |data_start, buf_start
	static const size_t byte_size = sizeof(uint8_t);
	if (num == 0) {
		// Stupid edge case (hopefully?)
		return 0;
	}
	if (num > ((rb->num_bytes - rb->data_length)/byte_size)) {
		// We don't have enough free space
		// TODO: Copy as much as we can, return size_t of how many bytes we inserted
		return -1;
	}

	const size_t data_offset_from_buf_start = rb->data_start - rb->buffer_start; // TODO: ASSERT THIS IS ALWAYS POSITIVE (how to even detect negative with size_t?)
	const size_t insertion_offset = (data_offset_from_buf_start + byte_size*rb->data_length) % rb->num_bytes; // next slot after data_end. offset from buffer_start
	const size_t insertion_end_offset = (insertion_offset + byte_size*(num - 1)) % rb->num_bytes; // end of data that is to be inserted. offset from buffer_start

	const bool insertion_wrapped = insertion_end_offset < insertion_offset; // (case 2)
	if (insertion_wrapped) {
		// insert from the insertion point to the end
		const size_t num_bytes_to_insert = rb->num_bytes - insertion_offset;
		memcpy(rb->buffer_start + insertion_offset, src_start, num_bytes_to_insert);

		// then from the beginning to insertion_end
		memcpy(rb->buffer_start, src_start + num_bytes_to_insert, num - num_bytes_to_insert);
	} else {
		memcpy(rb->buffer_start + insertion_offset, src_start, num);
	}
	rb->data_length += num*byte_size;

	return 0;
}

size_t pictrl_rb_read(pictrl_rb_t *rb, void *dest, size_t num) {
	if (num == 0 || rb->data_length == 0) {
		return 0;
	}
	// if not enough data to read, read as much as we can
	static const size_t byte_size = sizeof(uint8_t); // TODO: figure out byte_size nonsense
	const size_t num_bytes_to_read = (num > rb->data_length) ? rb->data_length : num;

	const size_t cur_data_offset_start = rb->data_start - rb->buffer_start; // TODO: ASSERT THIS IS ALWAYS POSITIVE (how to even detect negative with size_t?)
	const size_t cur_data_offset_end = (cur_data_offset_start + byte_size*(rb->data_length - 1)) % rb->num_bytes; // next slot after data_end. offset from buffer_start
	const size_t new_data_offset_start = (cur_data_offset_start + byte_size*num_bytes_to_read) % rb->num_bytes;

	const bool data_wrapped = cur_data_offset_end < cur_data_offset_start;
	if (data_wrapped) {
		const size_t num_edge_bytes = rb->num_bytes - cur_data_offset_start;
		memcpy(dest, rb->data_start, num_edge_bytes);

		memcpy(dest+num_edge_bytes, rb->buffer_start, num - num_edge_bytes);
	} else {
		memcpy(dest, rb->data_start, num_bytes_to_read);
	}
	rb->data_length -= num_bytes_to_read;
	rb->data_start = rb->buffer_start + new_data_offset_start;

	return num_bytes_to_read;
}
