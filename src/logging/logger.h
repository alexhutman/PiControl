#pragma once

#include <stdbool.h>

#ifndef __has_attribute
  #define __has_attribute(x) 0
#endif // __has_attribute

#if __has_attribute(format) || (defined(__GNUC__) && (__GNUC__ >= 2))
  #define PRINTF_FORMAT(fmt, args) __attribute__((format(printf, fmt, args)))
#else
  #define PRINTF_FORMAT(fmt, args)
#endif // __has_attribute(format)

typedef enum {
  LOG_LVL_DEBUG = 0,
  LOG_LVL_INFO,
  LOG_LVL_WARN,
  LOG_LVL_ERROR,
  LOG_LVL_CRITICAL,
} PiLogLevel;

bool pictrl_logger_init();
void _pictrl_log_msg(PiLogLevel level, const char *format, ...) PRINTF_FORMAT(2, 3);
void pictrl_logger_destroy();

#ifdef PI_CTRL_DEBUG
  #define pictrl_log_debug(...) _pictrl_log_msg(LOG_LVL_DEBUG, __VA_ARGS__)
#else
  #define pictrl_log_debug(...)                                                                    \
    do {                                                                                           \
    } while (0)
#endif // PI_CTRL_DEBUG
#define pictrl_log_info(...)     _pictrl_log_msg(LOG_LVL_INFO, __VA_ARGS__)
#define pictrl_log_warn(...)     _pictrl_log_msg(LOG_LVL_WARN, __VA_ARGS__)
#define pictrl_log_error(...)    _pictrl_log_msg(LOG_LVL_ERROR, __VA_ARGS__)
#define pictrl_log_critical(...) _pictrl_log_msg(LOG_LVL_CRITICAL, __VA_ARGS__)
