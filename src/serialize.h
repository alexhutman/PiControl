#include "data_structures/ring_buffer.h"
#include "model/picontrol.h"

// Assumes that rb->data_start is pointing at the beginning of the header in the ring buffer already
static inline PiCtrlHeader pictrl_rb_get_header(pictrl_rb_t *rb) {
    const PiCtrlHeader ret = {
        .command = (PiCtrlCmd)pictrl_rb_get(rb, 0),
        .payload_size = (size_t)pictrl_rb_get(rb, 1)
    };

    const size_t new_data_start_idx = (rb->data_start + 2) % rb->capacity;
    rb->data_start = new_data_start_idx;

    rb->num_items -= 2;
    return ret;
}

// Assumes they're the first 2 items in the ring buffer
static inline PiCtrlMouseCoord pictrl_rb_get_mouse_coords(pictrl_rb_t *rb) {
    const PiCtrlMouseCoord ret = {
        .x = (int)pictrl_rb_get(rb, 0),
        .y = (int)pictrl_rb_get(rb, 1)
    };

    const size_t new_data_start_idx = (rb->data_start + 2) % rb->capacity;
    rb->data_start = new_data_start_idx;

    rb->num_items -= 2;
    return ret;
}
