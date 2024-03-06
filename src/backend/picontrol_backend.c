#include <stdlib.h>

#include "backend/picontrol_backend.h"
#include "logging/log_utils.h"
#include "picontrol_config.h"
#include "serialize.h"

#ifdef PICTRL_XDO // TODO: Use an xdo definition directly?
#include <xdo.h>

#include "backend/pictrl_xdo.h"
#endif

static const char *PICTRL_BACKEND_NAMES[] = {
    "uinput",
    "xdo"
};

const char *pictrl_backend_name(pictrl_backend_type type) {
    // TODO: more robust way?
    return PICTRL_BACKEND_NAMES[type];
}

pictrl_backend *pictrl_backend_new() {
    pictrl_backend *new_backend = malloc(sizeof(*new_backend));
#ifdef PICTRL_XDO
    new_backend->backend = (pictrl_backend_t *)pictrl_xdo_backend_new();
    new_backend->type = PICTRL_BACKEND_XDO;
#else // default
    new_backend->backend = (pictrl_backend_t *)pictrl_uinput_backend_new();
    new_backend->type = PICTRL_BACKEND_UINPUT;
#endif
    if (new_backend->backend == NULL) {
        free(new_backend);
        return NULL;
    }

	return new_backend;
}

void pictrl_backend_free(pictrl_backend *backend) {
#ifdef PICTRL_XDO
    pictrl_xdo_backend_free((xdo_t *)backend->backend);
#else // default
    pictrl_uinput_backend_free((pictrl_uinput_t *)backend->backend);
#endif
}

void handle_mouse_move(pictrl_rb_t *rb, pictrl_backend *backend) {
    // extract the relative X and Y mouse locations to move by
    const PiCtrlMouseCoord coords = pictrl_rb_get_mouse_coords(rb);

#ifdef PICTRL_XDO
    pictrl_log_debug("Moving mouse (%d, %d) relative units using xdo.\n\n", coords.x, coords.y);
    if (xdo_move_mouse_relative((xdo_t *)backend->backend, coords.x, coords.y) != 0) {
        pictrl_log_warn("Mouse was unable to be moved (%d, %d) relative units.\n", coords.x, coords.y);
    }
#else
    pictrl_log_stub("UINPUT IS SUPPOSED TO MOVE MOUSE (%d, %d) RELATIVE UNITS.\n\n", coords.x, coords.y);
#endif
}

void handle_text(pictrl_rb_t *rb, pictrl_backend *backend) {
    // `xdo_enter_text_window` expects a null-terminated string, there are more efficient approaches but this works
    static char text[MAX_BUF];
    pictrl_rb_copy(rb, text);

    text[rb->num_items] = 0;

#ifdef PICTRL_XDO
    xdo_enter_text_window((xdo_t *)backend->backend, CURRENTWINDOW, text, XDO_KEYSTROKE_DELAY); // TODO: what if sizeof(char) != sizeof(uint8_t)?
#else
    pictrl_log_stub("UINPUT IS SUPPOSED TO TYPE \"%s\".\n\n", text);
#endif

    const size_t new_data_start_idx = (rb->data_start + rb->num_items) % rb->capacity;
    rb->data_start = new_data_start_idx;
    rb->num_items = 0;
}

void handle_keysym(pictrl_rb_t *rb, pictrl_backend *backend) {
    // `xdo_send_keysequence_window` expects a null-terminated string, there are more efficient approaches but this works
    static char keysym[MAX_BUF];
    pictrl_rb_copy(rb, keysym);

    keysym[rb->num_items] = 0;

#ifdef PICTRL_XDO
    xdo_send_keysequence_window((xdo_t *)backend->backend, CURRENTWINDOW, keysym, XDO_KEYSTROKE_DELAY);
#else
    pictrl_log_stub("UINPUT IS SUPPOSED TO INPUT KEYSYM(S) \"%s\".\n\n", keysym);
#endif

    const size_t new_data_start_idx = (rb->data_start + rb->num_items) % rb->capacity;
    rb->data_start = new_data_start_idx;
    rb->num_items = 0;
}
