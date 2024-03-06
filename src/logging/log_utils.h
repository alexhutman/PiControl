#ifndef _PICTRL_LOGUTILS_H
#define _PICTRL_LOGUTILS_H

#include <stdio.h>


/*
IMPORTANT: *don't* insert anything before those `[f]printf`s --
Sometimes we `pictrl_log_*` `errno` after an error occurred.
Anything done before this could change its value before we print it.
See the "NOTES" section of `man errno` for more info.

NOTE: be careful with strerr() as well: https://stackoverflow.com/q/73167084
*/
#ifdef PI_CTRL_DEBUG
#define pictrl_log_debug(...)     printf ("[DEBUG] " __VA_ARGS__)
#else
#define pictrl_log_debug(...)
#endif

#define pictrl_log(...)           printf (__VA_ARGS__)
#define pictrl_log_info(...)      printf ("[INFO] " __VA_ARGS__)
#define pictrl_log_warn(...)      fprintf (stderr, "[WARN] " __VA_ARGS__)
#define pictrl_log_error(...)     fprintf (stderr, "[ERROR] " __VA_ARGS__)
#define pictrl_log_critical(...)  fprintf (stderr, "[CRITICAL] " __VA_ARGS__)

#define pictrl_log_stub(...)      printf ("[STUB] " __VA_ARGS__)

#define pictrl_log_test_case(...) printf ("[CASE] " __VA_ARGS__)

#endif
