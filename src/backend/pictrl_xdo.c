#include <stdlib.h>

#include "backend/pictrl_xdo.h"
#include "logging/log_utils.h"
#include "picontrol_common.h"
#include "serialize.h"


xdo_t *create_xdo() {
    const char *display = getenv("DISPLAY"); // TODO: make this configurable?
    return xdo_new(display);
}

void handle_mouse_move(pictrl_rb_t *rb, xdo_t *xdo) {
    // extract the relative X and Y mouse locations to move by
    const PiCtrlMouseCoord coords = pictrl_rb_get_mouse_coords(rb);

    pictrl_log_debug("Moving mouse (%d, %d) relative units.\n\n", coords.x, coords.y);
    if (xdo_move_mouse_relative(xdo, coords.x, coords.y) != 0) {
        pictrl_log_warn("Mouse was unable to be moved (%d, %d) relative units.\n", coords.x, coords.y);
    }
}

void handle_text(pictrl_rb_t *rb, xdo_t *xdo) {
    // `xdo_enter_text_window` expects a null-terminated string, there are more efficient approaches but this works
    static char text[MAX_BUF];
    pictrl_rb_copy(rb, text);

    text[rb->num_items] = 0;

    xdo_enter_text_window(xdo, CURRENTWINDOW, text, XDO_KEYSTROKE_DELAY); // TODO: what if sizeof(char) != sizeof(uint8_t)?

    const size_t new_data_start_idx = (rb->data_start + rb->num_items) % rb->capacity;
    rb->data_start = new_data_start_idx;
    rb->num_items = 0;
}

void handle_keysym(pictrl_rb_t *rb, xdo_t *xdo) {
    // `xdo_send_keysequence_window` expects a null-terminated string, there are more efficient approaches but this works
    static char keysym[MAX_BUF];
    pictrl_rb_copy(rb, keysym);

    keysym[rb->num_items] = 0;

    xdo_send_keysequence_window(xdo, CURRENTWINDOW, keysym, XDO_KEYSTROKE_DELAY);

    const size_t new_data_start_idx = (rb->data_start + rb->num_items) % rb->capacity;
    rb->data_start = new_data_start_idx;
    rb->num_items = 0;
}
