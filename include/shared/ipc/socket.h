#pragma once

#define _GNU_SOURCE
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "shared/logging/logger.h"

static inline bool get_socket_path(char *dst, size_t sz) {
    const char *runtime_dir = secure_getenv("XDG_RUNTIME_DIR");
    if (!runtime_dir) {
        pictrl_log_error("$XDG_RUNTIME_DIR is not set\n");
        return false;
    }
    int written = snprintf(dst, sz, "%s/picontrol.sock", runtime_dir);
    return written > 0 && (size_t)written < sz;
}
