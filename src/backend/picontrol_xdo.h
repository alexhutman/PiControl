#ifndef _PICTRL_XDO_H
#define _PICTRL_XDO_H

#include <xdo.h>

#include "backend/picontrol_backend.h"

// Delay between xdo keystrokes in microseconds
#define XDO_KEYSTROKE_DELAY (useconds_t)10000

xdo_t *pictrl_xdo_backend_new();
void pictrl_xdo_backend_free(xdo_t *backend);

#endif
