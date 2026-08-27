#include "logging/logger.h"

#include "data_structures/multithread_pool.h"
#include "data_structures/multithread_queue.h"

#include <uv.h>

#include <assert.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#define MAX_LOG_MSG_LEN (251)
#define MSG_POOL_SIZE   (400)

typedef struct {
  LogLevel level;
  struct timespec time;
  char msg[MAX_LOG_MSG_LEN];
} LogMsg;

static uv_mutex_t log_mutex;
static Pool msg_pool;
static Queue msg_queue;
static uv_thread_t log_thread;
static atomic_bool is_running = ATOMIC_VAR_INIT(false);
static const char *log_level_names[] = {
    "DEBUG", "INFO", "WARN", "ERROR", "CRITICAL",
};

static void logger_thread_func(void *arg) {
  (void)arg;
  struct tm local_time;
  char base_time_str[20];
  LogMsg *recvd = NULL;

  while (pictrl_queue_pop(&msg_queue, &recvd)) {
    FILE *stream = recvd->level < LOG_LVL_WARN ? stdout : stderr;
    localtime_r(&recvd->time.tv_sec, &local_time);
    strftime(base_time_str, sizeof(base_time_str), "%Y/%m/%d %H:%M:%S", &local_time);

    long hundred_microsecs = recvd->time.tv_nsec / (1000 * 100);
    fprintf(stream, "[%s:%04ld] [%s] %s", base_time_str, hundred_microsecs,
            log_level_names[recvd->level], recvd->msg);
    fflush(stream);
    pictrl_pool_checkin(&msg_pool, recvd);
  }
}

void _pictrl_log_msg(LogLevel level, const char *format, ...) {
  LogMsg *msg = pictrl_pool_checkout(&msg_pool);
  clock_gettime(CLOCK_REALTIME, &msg->time);
  msg->level = level;

  va_list args;
  va_start(args, format);
  vsnprintf(msg->msg, MAX_LOG_MSG_LEN, format, args);
  va_end(args);

  pictrl_queue_push(&msg_queue, &msg);
}

bool pictrl_logger_init() {
  if (atomic_load(&is_running))
    return true;
  if (uv_mutex_init(&log_mutex) < 0) {
    fprintf(stderr, "Unable to create logger mutex\n");
    return false;
  }

  const PoolOpts pool_opts = {MSG_POOL_SIZE, sizeof(LogMsg), NULL, NULL, NULL};

  if (!pictrl_pool_init(&msg_pool, &pool_opts)) {
    fprintf(stderr, "Unable to create logger message pool\n");
    uv_mutex_destroy(&log_mutex);
    return false;
  }
  if (!pictrl_queue_init(&msg_queue, MSG_POOL_SIZE, sizeof(LogMsg *))) {
    fprintf(stderr, "Unable to create logger thread's print queue\n");
    pictrl_pool_destroy(&msg_pool);
    uv_mutex_destroy(&log_mutex);
    return false;
  }

  atomic_store(&is_running, true);
  uv_thread_create(&log_thread, &logger_thread_func, NULL);
  return true;
}

void pictrl_logger_destroy() {
  pictrl_queue_close(&msg_queue);
  uv_thread_join(&log_thread);
  atomic_store(&is_running, false);

  pictrl_queue_destroy(&msg_queue);
  assert(msg_pool.top == msg_pool.capacity && "Not all messages were put back");
  pictrl_pool_destroy(&msg_pool);
}
