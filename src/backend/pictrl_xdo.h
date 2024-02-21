#ifndef _PICTRL_XDO_H
#define _PICTRL_XDO_H

#include <xdo.h>

#include "data_structures/ring_buffer.h"

// Delay between xdo keystrokes in microseconds
#define XDO_KEYSTROKE_DELAY (useconds_t)10000

void handle_mouse_move(pictrl_rb_t *rb, xdo_t *xdo);
void handle_text(pictrl_rb_t *rb, xdo_t *xdo);
void handle_keysym(pictrl_rb_t *rb, xdo_t *xdo);
xdo_t *create_xdo();

#endif
