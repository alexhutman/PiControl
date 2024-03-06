#ifndef _PICTRL_BACKEND_H
#define _PICTRL_BACKEND_H

#include "backend/picontrol_uinput.h"
#include "data_structures/ring_buffer.h"

#ifdef PICTRL_XDO // TODO: Use an xdo definition directly?
#include <xdo.h>
#endif

typedef enum {
    PICTRL_BACKEND_UINPUT,
    PICTRL_BACKEND_XDO
} pictrl_backend_type;

typedef union {
    pictrl_uinput_t uinput;
#ifdef PICTRL_XDO
    xdo_t xdo;
#endif
} pictrl_backend_t;

typedef struct {
    pictrl_backend_type type;
    pictrl_backend_t *backend;
} pictrl_backend;


pictrl_backend *pictrl_backend_new();
void pictrl_backend_free(pictrl_backend *backend);
const char *pictrl_backend_name(pictrl_backend_type type);

void handle_mouse_move(pictrl_rb_t *rb, pictrl_backend *backend);
void handle_text(pictrl_rb_t *rb, pictrl_backend *backend);
void handle_keysym(pictrl_rb_t *rb, pictrl_backend *backend);

#endif
