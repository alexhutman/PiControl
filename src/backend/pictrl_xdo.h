#ifndef _PICTRL_XDO_H
#define _PICTRL_XDO_H

#include <xdo.h>

#include "data_structures/ring_buffer.h"
#include "picontrol_common.h"

// Delay between xdo keystrokes in microseconds
#define XDO_KEYSTROKE_DELAY (useconds_t)10000

void handle_mouse_move(pictrl_rb_t *rb, xdo_t *xdo);
void handle_text(pictrl_rb_t *rb, xdo_t *xdo);
void handle_keysym(pictrl_rb_t *rb, xdo_t *xdo);
xdo_t *create_xdo();


// TODO: Move below to some kind of a "serialize" header?
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

#endif
