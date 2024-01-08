#ifndef _PICTRL_LOGUTILS_H
#define _PICTRL_LOGUTILS_H

#include <stdio.h>


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

#define pictrl_log_test_case(...) printf ("[CASE] " __VA_ARGS__)

#endif
