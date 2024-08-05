#include "backend/pictrl_xdo.h"

#include <stdlib.h>
#include <xdo.h>

xdo_t *pictrl_xdo_backend_new() {
  const char *display = getenv("DISPLAY");
  return xdo_new(display);
}

void pictrl_xdo_backend_free(xdo_t *xdo) { xdo_free(xdo); }
